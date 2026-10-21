#include "civ5/ui/selected_civilopedia_locale.hpp"
#include "civ5/ui/civis_locale_reference_text.hpp"
#include "civ5/ui/civis_selected_civilopedia_text_data_owner.hpp"
#include "civ5/ui/bnw_info_default_form.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <map>
#include <set>
#include <thread>

namespace civ5::ui {
namespace {
assets::MountedResource exact_resource(const assets::ResourceMountTable &resources,const std::string &key){
    const assets::AssetKey requested(key);std::optional<assets::MountedResource> found;
    for(const auto &match:resources.resolve_all(requested)){
        if(const auto *handle=std::get_if<assets::AssetHandle>(&match.asset);
           handle&&handle->candidate().normalized_name==requested.normalized_name()){
            if(found)throw runtime::ScriptError("Civilopedia localization source is ambiguous: "+key);
            found=match;
        }
    }
    if(!found)throw runtime::ScriptError("Civilopedia localization source is missing: "+key);
    return *found;
}
std::string bytes_sha(const assets::Bytes &bytes){return runtime::identify_bytes(bytes).sha256;}
std::string pinned_source(const std::filesystem::path &path,std::string_view sha){
    const auto before=runtime::identify_file(path);
    if(before.sha256!=sha||before.bytes>64ULL*1024*1024)throw runtime::ScriptError("Civilopedia locale source identity changed");
    std::ifstream file(path,std::ios::binary);if(!file)throw runtime::ScriptError("Civilopedia locale source cannot be read");
    std::string text{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    const auto bytes=std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(text.data()),text.size());
    if(runtime::identify_bytes(bytes).sha256!=sha)throw runtime::ScriptError("Civilopedia locale source changed during read");
    return text;
}
void tooltip_tags(const content::XmlNode &node,std::set<std::string> &tags){
    if(node.attribute("ID").starts_with("CategoryButton")){
        const auto tag=node.attribute("ToolTip");if(tag.empty())throw runtime::ScriptError("Civilopedia category tooltip key is absent");
        tags.insert(tag);
    }
    for(const auto &child:node.children)tooltip_tags(child,tags);
}
}
struct SelectedCivilopediaLocale::Impl {
    struct Source {std::string key,sha,source_id,relative;std::optional<std::uint64_t> mount;};
    const assets::ResourceMountTable &resources;
    CivilopediaLayoutOwnerKey identity;CivisCivilopediaCallbackOwner owner;
    std::thread::id thread=std::this_thread::get_id();
    std::string language,language_root;TopPanelLocalizationCatalog catalog;
    std::vector<Source> sources;std::map<std::string,std::size_t> origins;
    std::set<std::string> ambiguous;std::vector<std::string> lookup_tags;
    std::vector<CivilopediaLocaleObservation> observed;
    bool references_enabled=false;
    std::shared_ptr<CivisSelectedCivilopediaTextDataOwner> selected_text_data;
    bool selected_text_data_busy=false,selected_text_data_failed=false;
    bool buildings_first_form=false;
    Impl(const assets::ResourceMountTable &mounts,CivilopediaLayoutOwnerKey selected,std::string type)
        :resources(mounts),identity(std::move(selected)),owner(identity),language(std::move(type)){}
    void require_current()const{
        if(std::this_thread::get_id()!=thread)throw runtime::ScriptError("Civilopedia locale must run on its owner thread");
        if(!owner.current(identity))throw runtime::ScriptError("Civilopedia locale owner is stale or closed");
    }
    void verify(const Source &source)const{
        const auto mounted=exact_resource(resources,source.key);
        const auto &handle=std::get<assets::AssetHandle>(mounted.asset);const auto &provenance=handle.candidate().provenance;
        if(mounted.mount_id!=source.mount||provenance.source_id!=source.source_id||
           provenance.source_relative_path!=source.relative||bytes_sha(handle.read_bytes(64ULL*1024*1024))!=source.sha)
            throw runtime::ScriptError("Civilopedia localized text source changed: "+source.key);
    }
    std::string format(std::string_view tag,std::span<const runtime::ScriptValue> args){
        // Snapshot every borrowed value before Current/source callbacks can
        // invalidate caller storage. The span below references owned copies.
        const std::string pinned_tag(tag);
        const std::vector<runtime::ScriptValue> pinned_args(args.begin(),args.end());
        tag=pinned_tag;args=pinned_args;
        require_current();
        if(selected_text_data_busy){selected_text_data_failed=true;throw runtime::ScriptError("Civilopedia conversion during selected TextData declaration");}
        std::string expanded,source_key,source_sha;
        bool selected_unit_name=false;
        if(selected_text_data){
            const auto value=selected_text_data->raw(tag);
            source_key=value.source_key;source_sha=value.source_sha256;
            selected_unit_name=args.empty()&&(selected_text_data->unit_description(tag)||
                (buildings_first_form&&selected_text_data->building_description(tag)));
            if(references_enabled)expanded=expand_civis_locale_references(tag,[&](std::string_view referenced){
                return selected_text_data->raw(referenced).text;
            });
            else expanded=value.text;
        }else{
        if(tag.empty()||tag.find('\0')!=tag.npos||ambiguous.contains(std::string(tag)))
            throw runtime::ScriptError("Civilopedia localized key is invalid or ambiguous: "+std::string(tag));
        const auto origin=origins.find(std::string(tag));
        if(origin==origins.end())throw runtime::ScriptError("Civilopedia localized key is absent: "+std::string(tag));
        const auto &source=sources.at(origin->second);
        source_key=source.key;source_sha=source.sha;
        std::set<std::size_t> verified_sources;
        if(references_enabled)expanded=expand_civis_locale_references(tag,[&](std::string_view referenced){
            if(referenced.empty()||referenced.find('\0')!=referenced.npos||ambiguous.contains(std::string(referenced)))
                throw runtime::ScriptError("Civilopedia localized reference is invalid or ambiguous: "+std::string(referenced));
            const auto found=origins.find(std::string(referenced));
            if(found==origins.end())throw runtime::ScriptError("Civilopedia localized reference is absent: "+std::string(referenced));
            if(verified_sources.insert(found->second).second)verify(sources.at(found->second));
            return catalog.rows.at(std::string(referenced));
        });
        else{verify(source);expanded=catalog.rows.at(std::string(tag));}
        }
        std::vector<std::string> arguments;
        for(const auto &arg:args){
            if(const auto *text=std::get_if<std::string>(&arg))arguments.push_back(*text);
            else if(const auto *number=std::get_if<std::int64_t>(&arg))arguments.push_back(std::to_string(*number));
            else throw runtime::ScriptError("Civilopedia formatter argument requires a verified string or integer representation");
        }
        TopPanelLocalizationCatalog resolved;resolved.rows.emplace(tag,expanded);
        const auto text=selected_unit_name?bnw_info_default_form(resolved,tag):resolved.format(tag,arguments);
        if(text.empty()||text==tag)throw runtime::ScriptError("Civilopedia localized payload is empty or an unresolved key echo");
        require_current();
        observed.push_back({language,std::string(tag),text,source_key,source_sha});return text;
    }
};
SelectedCivilopediaLocale::SelectedCivilopediaLocale(std::unique_ptr<Impl> impl):impl_(std::move(impl)){}
SelectedCivilopediaLocale::~SelectedCivilopediaLocale()=default;
std::shared_ptr<SelectedCivilopediaLocale> SelectedCivilopediaLocale::open(
    const std::filesystem::path &game_root,const assets::ResourceMountTable &resources,
    const SelectedBnwCivilopediaLayoutSources &sources,CivilopediaLayoutOwnerKey identity,std::string language){
    if(!sources.verified||!sources.control_tree.verified||identity.frame_number==0||identity.owner_token.empty()||
       identity.player_id<0||identity.game_turn<0||identity.payload_sha256.size()!=64||identity.rules_sha256.size()!=64||
       identity.civilopedia_sha256!=sources.civilopedia_sha256||identity.xml_sha256!=sources.xml_sha256||identity.styles_sha256!=sources.styles_sha256)
        throw runtime::ScriptError("Civilopedia locale selected source/frame identity is unavailable");
    auto impl=std::make_unique<Impl>(resources,std::move(identity),std::move(language));
    const bool english=impl->language=="en_US";
    if(!english&&impl->language!="ru_RU")throw runtime::ScriptError("Civilopedia selected language is unsupported");
    impl->language_root=english?"Language_en_US":"Language_RU_RU";
    const auto pedia=game_root/"Assets/DLC/Expansion2/UI/Civilopedia";
    const auto parent=pinned_source(pedia/"CivilopediaScreen.lua",sources.civilopedia_sha256);
    const auto xml=content::read_xml_metadata(pinned_source(pedia/"CivilopediaScreen.xml",sources.xml_sha256));
    std::set<std::string> category_tags;tooltip_tags(xml,category_tags);
    // Ground the bounded Lookup surface in the two actual bare-tag calls and
    // their authored Button tooltip identities. It does not claim general
    // Locale.Lookup grammar or original engine formatting equivalence.
    std::size_t cursor=0;
    constexpr std::string_view lookup=".labelString = Locale.Lookup(\"";
    while((cursor=parent.find(lookup,cursor))!=parent.npos){
        cursor+=lookup.size();const auto end=parent.find("\");",cursor);
        if(end==parent.npos)throw runtime::ScriptError("Civilopedia bare category Lookup source changed");
        const auto tag=parent.substr(cursor,end-cursor);
        if(!category_tags.contains(tag))throw runtime::ScriptError("Civilopedia Lookup tag differs from selected Button tooltip");
        impl->lookup_tags.push_back(tag);cursor=end+3;
    }
    if(impl->lookup_tags.empty())throw runtime::ScriptError("Civilopedia selected bare Lookup block is absent");
    const std::string language_dir=english?"en_US":"RU_RU";
    const std::array<std::string,3> prefixes={"Assets/Gameplay/XML/NewText/"+std::string(english?"EN_US":"RU_RU"),
        "Assets/DLC/Expansion/Gameplay/XML/Text/"+language_dir,"Assets/DLC/Expansion2/Gameplay/XML/Text/"+language_dir};
    for(const auto &prefix:prefixes){
        const auto keys=resources.keys(prefix);if(keys.empty())throw runtime::ScriptError("Civilopedia localization layer is missing: "+prefix);
        std::map<std::string,std::optional<std::pair<std::string,std::size_t>>> layer;
        for(const auto &key:keys){
            const auto normalized=assets::normalize_asset_name(key);
            if(!normalized.ends_with(".xml")||normalized.find("/leaderdialog/")!=std::string::npos||normalized.find("scenario")!=std::string::npos)continue;
            const auto mounted=exact_resource(resources,key);const auto &handle=std::get<assets::AssetHandle>(mounted.asset);
            const auto bytes=handle.read_bytes(64ULL*1024*1024);const auto document=content::read_xml_metadata(std::string(bytes.begin(),bytes.end()));
            if(document.name!="GameData")throw runtime::ScriptError("Civilopedia localization root differs from GameData: "+key);
            const content::XmlNode *language_node=nullptr;
            for(const auto &child:document.children)if(child.name==impl->language_root){
                if(language_node)throw runtime::ScriptError("Civilopedia language node is duplicated: "+key);language_node=&child;
            }
            if(!language_node)throw runtime::ScriptError("Civilopedia selected language node is missing: "+key);
            const auto &provenance=handle.candidate().provenance;const auto source_index=impl->sources.size();
            impl->sources.push_back({key,bytes_sha(bytes),provenance.source_id,provenance.source_relative_path,mounted.mount_id});
            for(const auto &row:language_node->children){
                const auto tag=row.attribute("Tag");const auto *text=row.child("Text");
                if(row.name!="Row"||tag.empty()||!text||!text->children.empty())throw runtime::ScriptError("Civilopedia localized row is malformed: "+key);
                if(!layer.emplace(tag,std::pair{text->text,source_index}).second)layer.at(tag).reset();
            }
        }
        for(const auto &[tag,value]:layer){
            if(value){impl->catalog.rows[tag]=value->first;impl->origins[tag]=value->second;impl->ambiguous.erase(tag);}
            else{impl->catalog.rows.erase(tag);impl->origins.erase(tag);impl->ambiguous.insert(tag);}
        }
    }
    for(const auto &tag:category_tags){
        if(impl->ambiguous.contains(tag)||!impl->catalog.rows.contains(tag))throw runtime::ScriptError("Civilopedia category text is absent or ambiguous: "+tag);
        const auto &text=impl->catalog.rows.at(tag);
        if(text.empty()||text==tag||text.find('{')!=text.npos||text.find('}')!=text.npos)
            throw runtime::ScriptError("Civilopedia category label is not a resolved plain payload: "+tag);
    }
    return std::shared_ptr<SelectedCivilopediaLocale>(new SelectedCivilopediaLocale(std::move(impl)));
}
runtime::ScriptLocaleProvider SelectedCivilopediaLocale::provider(){
    const std::weak_ptr<SelectedCivilopediaLocale> weak=shared_from_this();
    const auto lock=[weak]{const auto owner=weak.lock();if(!owner)throw runtime::ScriptError("Civilopedia locale owner expired");owner->impl_->require_current();return owner;};
    return {[lock]{const auto owner=lock();return runtime::ScriptLanguage{owner->impl_->language,owner->impl_->language=="en_US"?"English":"Русский"};},
        [lock](std::string_view requested){return lock()->impl_->language==requested;},
        [lock](std::string_view tag,std::span<const runtime::ScriptValue> args){return lock()->impl_->format(tag,args);}};
}
void SelectedCivilopediaLocale::enable_selected_text_data(
    const std::filesystem::path &game_root,const runtime::SavedContentProfile &profile,
    const content::Catalog &catalog,const assets::ResourceMountTable &mounts,const content::RulesDatabase &rules){
    impl_->require_current();
    if(impl_->selected_text_data_busy){impl_->selected_text_data_failed=true;throw runtime::ScriptError("Civilopedia selected TextData declaration reentry");}
    if(impl_->selected_text_data||impl_->selected_text_data_failed||!impl_->observed.empty())
        throw runtime::ScriptError("Civilopedia selected TextData is duplicated, retired or declared after conversion");
    impl_->selected_text_data_busy=true;
    struct Exit{bool &busy;~Exit(){busy=false;}}exit{impl_->selected_text_data_busy};
    const std::weak_ptr<SelectedCivilopediaLocale> weak=shared_from_this();
    auto selected=CivisSelectedCivilopediaTextDataOwner::open(game_root,profile,catalog,mounts,rules,impl_->identity,impl_->language,
        [weak](const CivilopediaLayoutOwnerKey &identity){const auto owner=weak.lock();return owner&&owner->current(identity);});
    impl_->require_current();
    if(impl_->selected_text_data_failed||!impl_->observed.empty())
        throw runtime::ScriptError("Civilopedia selected TextData declaration changed during callbacks");
    impl_->selected_text_data=std::move(selected);
}
void SelectedCivilopediaLocale::enable_buildings_description_first_form(){
    const auto keep_alive=shared_from_this();impl_->require_current();
    if(impl_->selected_text_data_busy){impl_->selected_text_data_failed=true;throw runtime::ScriptError("Buildings Description declaration reentry");}
    if(!impl_->selected_text_data||impl_->selected_text_data_failed||impl_->buildings_first_form||!impl_->observed.empty())
        throw runtime::ScriptError("Buildings Description first form requires selected TextData before conversion and cannot be duplicated");
    impl_->selected_text_data_busy=true;
    struct Exit{bool &busy;~Exit(){busy=false;}}exit{impl_->selected_text_data_busy};
    impl_->selected_text_data->enable_buildings_description_first_form();impl_->require_current();
    if(impl_->selected_text_data_failed||!impl_->observed.empty())
        throw runtime::ScriptError("Buildings Description declaration changed during callbacks");
    impl_->buildings_first_form=true;
}
bool SelectedCivilopediaLocale::buildings_description_first_form_enabled()const{
    impl_->require_current();return impl_->buildings_first_form;
}
void SelectedCivilopediaLocale::enable_plain_reference_expansion(){
    impl_->require_current();
    if(impl_->selected_text_data_busy){impl_->selected_text_data_failed=true;throw runtime::ScriptError("Civilopedia reference declaration during selected TextData declaration");}
    if(impl_->references_enabled||!impl_->observed.empty())
        throw runtime::ScriptError("Civilopedia reference capability is duplicated or declared after text conversion");
    impl_->references_enabled=true;
}
const std::vector<std::string> &SelectedCivilopediaLocale::plain_lookup_tags()const{return impl_->lookup_tags;}
const std::vector<CivilopediaLocaleObservation> &SelectedCivilopediaLocale::observations()const{return impl_->observed;}
bool SelectedCivilopediaLocale::current(const CivilopediaLayoutOwnerKey &identity)const{return impl_->owner.current(identity);}
void SelectedCivilopediaLocale::observe(const CivilopediaLayoutOwnerKey &identity){impl_->require_current();impl_->owner.observe(identity);}
void SelectedCivilopediaLocale::close(){impl_->require_current();if(impl_->selected_text_data)impl_->selected_text_data->close();impl_->owner.close();}
} // namespace civ5::ui
