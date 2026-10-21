#include "civ5/ui/civis_selected_civilopedia_text_data_owner.hpp"
#include "civ5/content/game_data_xml.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <thread>

namespace civ5::ui {
namespace {
constexpr std::uint64_t budget=64ULL*1024*1024;
std::string sha(const assets::Bytes &bytes){return runtime::identify_bytes(bytes).sha256;}
bool same(const assets::AssetHandle &a,const assets::AssetHandle &b){
    const auto &x=a.candidate(),&y=b.candidate();
    return x.normalized_name==y.normalized_name&&x.provenance.kind==y.provenance.kind&&
        x.provenance.source_id==y.provenance.source_id&&x.provenance.source_relative_path==y.provenance.source_relative_path&&
        x.provenance.archive_relative_path==y.provenance.archive_relative_path;
}
std::optional<std::uint64_t> exact_mount(const assets::ResourceMountTable &resources,const assets::AssetHandle &asset){
    bool found=false;std::optional<std::uint64_t> id;
    for(const auto &hit:resources.resolve_all(asset.requested_key())){
        if(const auto *handle=std::get_if<assets::AssetHandle>(&hit.asset);handle&&same(*handle,asset)){
            if(found)throw runtime::ScriptError("Selected TextData source mount is ambiguous");
            found=true;id=hit.mount_id;
        }
    }
    if(!found)throw runtime::ScriptError("Selected TextData source is absent from retained mount selection");
    return id;
}
bool same_declarations(const content::PackageManifest &a,const content::PackageManifest &b){
    content::PackageResourceQuery query;query.selector="TextData";query.enabled_categories={true,true,true};
    const auto x=content::package_resources(a,query),y=content::package_resources(b,query);
    if(x.size()!=y.size())return false;
    for(std::size_t i=0;i<x.size();++i)
        if(x[i].value!=y[i].value||x[i].category!=y[i].category||x[i].category_occurrence!=y[i].category_occurrence||x[i].file_list!=y[i].file_list)return false;
    return true;
}
std::optional<std::string> field(const content::XmlNode &row,const std::string &name){
    if(const auto *node=row.child(name)){
        if(!node->children.empty())throw runtime::ScriptError("Selected TextData field requires unsupported nested grammar");
        return node->text;
    }
    for(const auto &[key,value]:row.attributes)if(key==name)return value;
    return std::nullopt;
}
}
struct CivisSelectedCivilopediaTextDataOwner::Impl {
    struct Source {assets::AssetHandle asset;std::string sha;std::optional<std::size_t> package;std::optional<std::uint64_t> mount;};
    struct Manifest {std::filesystem::path path;std::string sha;};
    struct Origin {std::string text;std::size_t source;};
    const std::thread::id thread=std::this_thread::get_id();
    assets::ResourceMountTable resources;
    std::unique_ptr<content::RulesDatabase> database;
    CivilopediaLayoutOwnerKey identity;std::string language;Current current;
    std::vector<Source> sources;std::vector<Manifest> manifests;
    std::map<std::string,Origin> origins;std::set<std::string> ambiguous,unit_tags;
    bool closed=false,busy=false,failed=false;
    bool reads_started=false,buildings_first_form=false;std::set<std::string> building_tags;
    explicit Impl(assets::ResourceMountTable mounts):resources(std::move(mounts)){}
    void require_current(){
        if(std::this_thread::get_id()!=thread)throw runtime::ScriptError("Selected TextData requires its owner thread");
        if(closed||failed)throw runtime::ScriptError("Selected TextData owner is closed or retired");
        bool valid=false;try{valid=current&&current(identity);}catch(...){valid=false;}
        if(!valid||closed||failed){failed=true;throw runtime::ScriptError("Selected TextData owner is stale or current guard failed");}
    }
    void enter(){
        if(std::this_thread::get_id()!=thread)throw runtime::ScriptError("Selected TextData requires its owner thread");
        if(busy){failed=true;throw runtime::ScriptError("Selected TextData callback reentry");}
        busy=true;
    }
    void verify(std::size_t index){
        for(const auto &manifest:manifests)
            if(runtime::identify_file(manifest.path).sha256!=manifest.sha)
                throw runtime::ScriptError("Selected TextData manifest identity changed");
        const auto &source=sources.at(index);
        if(exact_mount(resources,source.asset)!=source.mount||sha(source.asset.read_bytes(budget))!=source.sha)
            throw runtime::ScriptError("Selected TextData asset identity changed");
        require_current();
    }
};
CivisSelectedCivilopediaTextDataOwner::CivisSelectedCivilopediaTextDataOwner(std::unique_ptr<Impl> impl):impl_(std::move(impl)){}
CivisSelectedCivilopediaTextDataOwner::~CivisSelectedCivilopediaTextDataOwner()=default;
std::shared_ptr<CivisSelectedCivilopediaTextDataOwner> CivisSelectedCivilopediaTextDataOwner::open(
    const std::filesystem::path &root,const runtime::SavedContentProfile &profile,const content::Catalog &catalog,
    const assets::ResourceMountTable &resources,const content::RulesDatabase &rules,
    CivilopediaLayoutOwnerKey identity,std::string language,Current current){
    // Copy caller configuration and the attached DB BEFORE any Current or
    // source lookup callback. Callbacks may release/replace their input storage.
    const auto selected_root=root;
    const auto selected_profile=profile;
    const auto selected_catalog=catalog;
    auto selected_resources=resources;
    auto selected_database=rules.clone_gameplay();
    if(!selected_profile.can_activate||!selected_profile.all_requirements_located||!selected_profile.files_complete||!selected_profile.enabled_mods.empty()||
        !current||!identity.frame_number||identity.owner_token.empty()||identity.player_id<0||identity.game_turn<0||
        identity.payload_sha256.size()!=64||identity.rules_sha256.size()!=64||identity.civilopedia_sha256.size()!=64||
        identity.xml_sha256.size()!=64||identity.styles_sha256.size()!=64||
        (language!="en_US"&&language!="ru_RU"))throw runtime::ScriptError("Selected TextData activation/frame/language is unavailable");
    auto impl=std::make_unique<Impl>(std::move(selected_resources));impl->identity=std::move(identity);impl->language=std::move(language);impl->current=std::move(current);
    impl->database=std::move(selected_database);impl->require_current();
    std::set<std::size_t> selected;std::set<std::string> ids;
    for(const auto index:selected_profile.enabled_dlc){
        if(!selected.insert(index).second)throw runtime::ScriptError("Selected TextData package activation is duplicated");
        const auto &package=selected_catalog.packages.at(index);
        if(package.kind!=content::PackageKind::dlc||!ids.insert(content::canonical_guid(package.id)).second)
            throw runtime::ScriptError("Selected TextData package identity is ambiguous");
        const auto before=runtime::identify_file(package.path);
        const auto actual=content::read_manifest(package.path);
        if(actual.kind!=package.kind||content::canonical_guid(actual.id)!=content::canonical_guid(package.id)||
            actual.version!=package.version||!same_declarations(actual,package)||runtime::identify_file(package.path).sha256!=before.sha256)
            throw runtime::ScriptError("Selected TextData manifest differs from activated catalog");
        impl->manifests.push_back({package.path,before.sha256});
    }
    const auto records=content::selected_localization_sources(selected_root,selected_profile.enabled_dlc,selected_catalog,impl->resources);
    const auto language_node=impl->language=="en_US"?"Language_en_US":"Language_RU_RU";
    // Build each source layer independently; cross-file duplicates are rejected
    // rather than guessing which failed native XML insertion supplied the DB row.
    std::map<std::string,std::optional<Impl::Origin>> layer;
    std::optional<std::size_t> group;
    const auto flush=[&]{
        for(const auto &[tag,origin]:layer){
            if(origin){impl->origins.insert_or_assign(tag,*origin);impl->ambiguous.erase(tag);}
            else{impl->origins.erase(tag);impl->ambiguous.insert(tag);}
        }
        layer.clear();
    };
    for(const auto &record:records){
        if(record.package!=group){flush();group=record.package;}
        const auto bytes=record.asset.read_bytes(budget);
        const auto document=content::read_game_data_xml_text(std::string(bytes.begin(),bytes.end()));
        if(document.name!="GameData")throw runtime::ScriptError("Selected TextData source is not GameData");
        const content::XmlNode *node=nullptr;
        for(const auto &child:document.children)if(child.name==language_node){
            if(node)throw runtime::ScriptError("Selected TextData language table is duplicated");node=&child;
        }
        if(!node)continue; // This source declares another language, never fallback.
        const auto index=impl->sources.size();
        impl->sources.push_back({record.asset,sha(bytes),record.package,exact_mount(impl->resources,record.asset)});
        for(const auto &row:node->children){
            const auto tag=field(row,"Tag"),text=field(row,"Text");
            if(row.name!="Row"||!tag||tag->empty()||!text)
                throw runtime::ScriptError("Selected TextData source needs unsupported row operation");
            if(!layer.emplace(*tag,Impl::Origin{*text,index}).second)layer.at(*tag).reset();
        }
    }
    flush();
    if(impl->origins.empty())throw runtime::ScriptError("Selected TextData has no source-bound rows for requested language");
    std::size_t language_rows=0;
    impl->database->query({"SELECT Type FROM Localization.Languages WHERE Type=?",{impl->language}},[&](auto row){
        if(row.size()!=1||!std::get_if<std::string>(&row[0]))throw runtime::ScriptError("Selected TextData language metadata is malformed");++language_rows;
    });
    if(language_rows!=1)throw runtime::ScriptError("Selected TextData language is absent or ambiguous in activated DB");
    impl->database->query({"SELECT Description FROM Units",{}},[&](auto row){
        if(row.size()!=1||!std::get_if<std::string>(&row[0]))throw runtime::ScriptError("Selected Units Description scope is malformed");
        impl->unit_tags.insert(std::get<std::string>(row[0]));
    });
    impl->require_current();
    return std::shared_ptr<CivisSelectedCivilopediaTextDataOwner>(new CivisSelectedCivilopediaTextDataOwner(std::move(impl)));
}
CivisSelectedCivilopediaTextValue CivisSelectedCivilopediaTextDataOwner::raw(std::string_view tag){
    const std::string pinned_tag(tag);tag=pinned_tag;
    const auto keep_alive=shared_from_this();auto &p=*impl_;p.enter();
    struct Exit{bool &busy;~Exit(){busy=false;}}exit{p.busy};p.reads_started=true;p.require_current();
    if(tag.empty()||tag.find('\0')!=tag.npos||p.ambiguous.contains(std::string(tag)))
        throw runtime::ScriptError("Selected TextData key is invalid or source-ambiguous: "+std::string(tag));
    const auto found=p.origins.find(std::string(tag));
    if(found==p.origins.end())throw runtime::ScriptError("Selected TextData key has no authorized source: "+std::string(tag));
    std::optional<std::string> value;std::size_t count=0;
    p.database->query({"SELECT Text FROM Localization.LocalizedText WHERE Language=? AND Tag=?",{p.language,std::string(tag)}},[&](auto row){
        ++count;if(row.size()!=1||!std::get_if<std::string>(&row[0]))throw runtime::ScriptError("Selected TextData DB value is malformed");value=std::get<std::string>(row[0]);
    });
    if(count!=1||!value||*value!=found->second.text)
        throw runtime::ScriptError("Selected TextData source/activated DB value mismatch: "+std::string(tag));
    p.verify(found->second.source);p.require_current();
    const auto &source=p.sources.at(found->second.source);
    return {p.language,std::string(tag),*value,source.asset.candidate().provenance.source_relative_path,source.sha,source.package,source.mount};
}
bool CivisSelectedCivilopediaTextDataOwner::unit_description(std::string_view tag){
    const std::string pinned_tag(tag);tag=pinned_tag;
    const auto keep_alive=shared_from_this();auto &p=*impl_;p.enter();
    struct Exit{bool &busy;~Exit(){busy=false;}}exit{p.busy};p.reads_started=true;p.require_current();return p.unit_tags.contains(std::string(tag));
}
void CivisSelectedCivilopediaTextDataOwner::enable_buildings_description_first_form(){
    const auto keep_alive=shared_from_this();auto &p=*impl_;p.enter();
    struct Exit{bool &busy;~Exit(){busy=false;}}exit{p.busy};p.require_current();
    if(p.buildings_first_form||p.reads_started)
        throw runtime::ScriptError("Buildings Description first form is duplicated or declared after lookup");
    std::set<std::string> tags;std::size_t rows=0;
    try{
        p.database->query({"SELECT Description FROM Buildings",{}},[&](auto row){
            if(++rows>65536||row.size()!=1||!std::get_if<std::string>(&row[0]))
                throw runtime::ScriptError("Selected Buildings Description scope is malformed or exceeds its bounded row budget");
            tags.insert(std::get<std::string>(row[0]));
        });
        p.require_current();
    }catch(...){p.failed=true;throw;}
    p.building_tags=std::move(tags);p.buildings_first_form=true;
}
bool CivisSelectedCivilopediaTextDataOwner::building_description(std::string_view tag){
    const std::string pinned_tag(tag);tag=pinned_tag;
    const auto keep_alive=shared_from_this();auto &p=*impl_;p.enter();
    struct Exit{bool &busy;~Exit(){busy=false;}}exit{p.busy};p.reads_started=true;p.require_current();
    return p.buildings_first_form&&p.building_tags.contains(std::string(tag));
}
void CivisSelectedCivilopediaTextDataOwner::close(){
    if(std::this_thread::get_id()!=impl_->thread)throw runtime::ScriptError("Selected TextData close requires owner thread");impl_->closed=true;
}
} // namespace civ5::ui
