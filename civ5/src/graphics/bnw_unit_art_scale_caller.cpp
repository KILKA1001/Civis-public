#include "civ5/graphics/bnw_unit_art_scale_caller.hpp"
#include "civ5/graphics/bnw_unit_art_scale_source_fields.hpp"
#include "civ5/graphics/bnw_unit_vis_ini_source_serializer.hpp"
#include "civ5/ui/bnw_native_user_directory_owner.hpp"
#include <fstream>
#include <bit>
#include <functional>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace civ5::graphics {
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
struct UserFile {bool present=false;std::vector<std::uint8_t> bytes;runtime::FileIdentity identity;std::filesystem::path path;};
UserFile read_user_file(const ui::BnwNativeUserDirectoryOwner& source) {
    UserFile out;out.path=source.current()/L"UnitVisConfig.ini";auto attrs=GetFileAttributesW(out.path.c_str());
    if(attrs==INVALID_FILE_ATTRIBUTES){auto error=GetLastError();need(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND,"Native user UnitVisConfig probe failed");(void)source.current();return out;}
    need((attrs&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))==0,"Native user UnitVisConfig backing unsupported");out.present=true;const auto before=runtime::identify_file(out.path);need(before.bytes<=65536,"Native user UnitVisConfig budget exceeded");std::ifstream in(out.path,std::ios::binary);need(bool(in),"Native user UnitVisConfig read failed");out.bytes.assign(std::istreambuf_iterator<char>(in),{});need(!in.bad()&&out.bytes.size()==before.bytes,"Native user UnitVisConfig read incomplete");out.identity=runtime::identify_bytes(out.bytes);const auto after=runtime::identify_file(out.path);need(out.identity.sha256==before.sha256&&after.sha256==before.sha256&&after.bytes==before.bytes,"Native user UnitVisConfig changed during read");(void)source.current();return out;
}
std::string fingerprint(const ui::SelectedBnwPreviewHost& host){return runtime::identify_bytes(host.database().gameplay_fingerprint_data()).sha256;}
bool same_provenance(const assets::Provenance& a,const assets::Provenance& b){return a.kind==b.kind&&a.source_id==b.source_id&&a.source_relative_path==b.source_relative_path&&a.archive_relative_path==b.archive_relative_path;}
const assets::AssetHandle& selected(const assets::MountedResource& resource){const auto* h=std::get_if<assets::AssetHandle>(&resource.asset);need(h,"Native unit art selected source missing/ambiguous");return *h;}
}
struct BnwUnitArtScaleCaller::Impl {
    std::shared_ptr<const ui::BnwNativePackageDiscoveryOwner> packages;
    std::shared_ptr<const ui::BnwNativeUserDirectoryOwner> directory;
    std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
    const assets::ResourceMountTable* resources=nullptr;
    BnwUnitArtScaleFrame frame;
    void validate()const {
        directory->require_same_resources(*packages);packages->require_same_selected_host(*host);need(&packages->resources()==resources,"Unit scale SAME resource table replaced");need(fingerprint(*host)==frame.rules_sha256,"Unit scale selected rules epoch changed");
        auto source=resources->resolve(assets::AssetKey("Assets/Units/UnitVisConfig.ini"));auto cfg=read_bnw_unit_visual_configuration(selected(source));need(source.mount_id==frame.initial_config_mount&&same_provenance(cfg.provenance,frame.initial_config_provenance)&&cfg.config_sha256==frame.initial_config_sha256&&std::bit_cast<std::uint32_t>(cfg.scene_spacing)==std::bit_cast<std::uint32_t>(frame.initial_asset_global_scale),"Unit scale initial asset configuration changed");
        if(frame.caller_fields_owned){auto current=read_user_file(*directory);need(current.path==frame.caller_configuration.source_path&&current.present==frame.caller_configuration.file_present&&current.identity.sha256==frame.caller_configuration.file.sha256&&current.identity.bytes==frame.caller_configuration.file.bytes,"Unit scale user config source changed; ReloadUnitSystem required");}
    }
    void validate_sources_for_command()const {
        directory->require_same_resources(*packages);packages->require_same_selected_host(*host);need(&packages->resources()==resources&&fingerprint(*host)==frame.rules_sha256,"Unit command SAME resources/rules epoch changed");auto source=resources->resolve(assets::AssetKey("Assets/Units/UnitVisConfig.ini"));auto cfg=read_bnw_unit_visual_configuration(selected(source));need(source.mount_id==frame.initial_config_mount&&same_provenance(cfg.provenance,frame.initial_config_provenance)&&cfg.config_sha256==frame.initial_config_sha256,"Unit command initial asset config changed");
    }
};
struct BnwUnitCompiledMemberScaleQuote::Data {
    std::shared_ptr<const void> owner_identity;
    std::function<void()> validate;
    BnwUnitMemberTransformSource xml;
    BnwUnitMemberTransformRules compiled;
    std::string model,rules_sha;
};
BnwUnitArtScaleCaller::BnwUnitArtScaleCaller(std::shared_ptr<Impl> i):impl_(std::move(i)){}
BnwUnitArtScaleCaller::~BnwUnitArtScaleCaller()=default;
std::shared_ptr<BnwUnitArtScaleCaller> BnwUnitArtScaleCaller::create(std::shared_ptr<const ui::BnwNativePackageDiscoveryOwner> p,std::shared_ptr<const ui::BnwNativeUserDirectoryOwner> d,std::shared_ptr<const ui::SelectedBnwPreviewHost> h) {
    need(p&&d&&h,"Unit scale sealed source owners absent");d->require_same_resources(*p);p->require_same_selected_host(*h);auto i=std::make_shared<Impl>();i->packages=std::move(p);i->directory=std::move(d);i->host=std::move(h);i->resources=&i->packages->resources();auto source=i->resources->resolve(assets::AssetKey("Assets/Units/UnitVisConfig.ini"));auto config=read_bnw_unit_visual_configuration(selected(source));i->frame.initial_asset_global_scale=config.scene_spacing;i->frame.current_global_scale=config.scene_spacing;i->frame.initial_config_sha256=config.config_sha256;i->frame.initial_config_provenance=config.provenance;i->frame.initial_config_mount=source.mount_id;i->frame.rules_sha256=fingerprint(*i->host);i->validate();return std::shared_ptr<BnwUnitArtScaleCaller>(new BnwUnitArtScaleCaller(std::move(i)));
}
void BnwUnitArtScaleCaller::apply_reload_unit_system_scale_stage() {
    impl_->validate_sources_for_command();auto input=read_user_file(*impl_->directory);
    auto fields=unit_art_scale_detail::read_user_scale_fields(std::string_view(reinterpret_cast<const char*>(input.bytes.data()),input.bytes.size()));
    auto draft=impl_->frame;need(draft.source_command_revision!=UINT64_MAX&&draft.library_reload_revision!=UINT64_MAX,"Unit scale command revision overflow");
    // Source successful load resets dirty0 at82752B. All four existing keys cause no
    // default insertion/write. A nonempty partial file requires full source
    // parser/serializer semantics and refuses before staging a command.
    need(input.bytes.empty()||(!fields.scale_defaulted&&!fields.multiplier_defaulted&&!fields.seed_defaulted&&!fields.threaded_defaulted),"Nonempty partial unit INI source serializer unavailable");
    BnwUnitArtScaleConfigRecord record;record.file_present=input.present;record.input_file_present=input.present;record.file=input.identity;record.input_file=input.identity;record.source_path=input.path;record.scale_defaulted=fields.scale_defaulted;record.multiplier_defaulted=fields.multiplier_defaulted;record.seed_defaulted=fields.seed_defaulted;record.threaded_defaulted=fields.threaded_defaulted;
    if(input.bytes.empty()) {
        record.source_ini_flush_requests=4;
        for(unsigned n=1;n<=4;++n){auto bytes=unit_art_scale_detail::missing_unit_vis_scale_ini(n);const auto sha=runtime::identify_bytes(std::span(reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size())).sha256;record.pending_effects.push_back({input.path,n,std::move(bytes),sha});}
    }
    draft.caller_placement_seed=fields.seed;draft.caller_threaded_render=fields.threaded;draft.caller_scale=fields.scale;draft.caller_member_multiplier=fields.multiplier;draft.caller_fields_owned=true;draft.publish_global=true;draft.source_transition_applied=record.source_ini_flush_requests==0;
    // Pending native filesystem effects are explicit, and never acknowledge
    // themselves. No native directory/file is created or overwritten here.
    if(draft.source_transition_applied)draft.current_global_scale=fields.scale;
    draft.caller_configuration=std::move(record);++draft.source_command_revision;++draft.library_reload_revision;impl_->validate_sources_for_command();auto recheck=read_user_file(*impl_->directory);need(recheck.present==input.present&&recheck.identity.sha256==input.identity.sha256&&recheck.identity.bytes==input.identity.bytes,"User unit config changed during source staging");impl_->frame=std::move(draft);impl_->validate();
}
void BnwUnitArtScaleCaller::reload_unit_libraries(){impl_->validate();need(impl_->frame.caller_fields_owned&&impl_->frame.source_transition_applied,"Native61EF00 retained applied caller fields unavailable; source effects pending");need(impl_->frame.library_reload_revision!=UINT64_MAX,"Unit library revision overflow");++impl_->frame.library_reload_revision;}
const BnwUnitArtScaleFrame& BnwUnitArtScaleCaller::current()const{impl_->validate();return impl_->frame;}
BnwUnitCompiledMemberScaleQuote::BnwUnitCompiledMemberScaleQuote(std::shared_ptr<const Data> d):data_(std::move(d)){}
BnwUnitCompiledMemberScaleQuote BnwUnitArtScaleCaller::compile_member(std::string_view type)const {
    impl_->validate();need(impl_->frame.caller_fields_owned,"Unit compiled Scale caller transition not owned");need(impl_->frame.source_transition_applied&&impl_->frame.caller_configuration.source_ini_flush_requests==0,"Native user INI writeback effects pending; compiled scale not authoritative");need(!type.empty()&&type.find('\0')==type.npos,"Unit member Type absent");auto source=impl_->resources->resolve(assets::AssetKey("Assets/Units/Civ5ArtDefines_UnitMembers.xml"));auto xml=read_bnw_unit_member_transform_rules(selected(source),type,impl_->frame.caller_scale);
    std::optional<float> authored_scale,z;std::string model;unsigned rows=0;
    auto report=impl_->host->database().query_rows({"SELECT Type,Scale,ZOffset,Model,typeof(Scale) AS scale_storage,typeof(ZOffset) AS z_storage FROM ArtDefine_UnitMemberInfos WHERE Type=?",{std::string(type)}},[&](const content::RulesRowView& row){need(++rows==1,"Duplicate selected ArtDefine_UnitMemberInfos row");const auto actual=row.text("Type"),s=row.text("scale_storage"),v=row.text("z_storage"),m=row.text("Model");need(actual&&*actual==type&&s&&v&&m&&!m->empty(),"Selected compiled unit art row incomplete");if(*s!="null")authored_scale=float(row.real("Scale"));if(*v!="null")z=float(row.real("ZOffset"));model=*m;},2);
    need(report.all_succeeded()&&rows==1,"Native ArtDefine_UnitMemberInfos compiled row producer unavailable");auto data=std::make_shared<BnwUnitCompiledMemberScaleQuote::Data>();data->owner_identity=impl_;data->xml=std::move(xml);data->compiled=compile_bnw_unit_member_transform_rules(authored_scale,z,impl_->frame.caller_scale);data->model=std::move(model);data->rules_sha=impl_->frame.rules_sha256;const auto command=impl_->frame.source_command_revision,library=impl_->frame.library_reload_revision;const auto xml_sha=data->xml.xml_sha256;const auto mount=source.mount_id;const auto provenance=selected(source).candidate().provenance;
    data->validate=[i=impl_,command,library,xml_sha,mount,provenance]{i->validate();need(i->frame.source_command_revision==command&&i->frame.library_reload_revision==library,"Compiled unit art quote stale after source reload");auto s=i->resources->resolve(assets::AssetKey("Assets/Units/Civ5ArtDefines_UnitMembers.xml"));need(s.mount_id==mount&&same_provenance(selected(s).candidate().provenance,provenance)&&runtime::identify_bytes(selected(s).read_bytes(64*1024*1024)).sha256==xml_sha,"Compiled unit art XML source changed");};data->validate();return BnwUnitCompiledMemberScaleQuote(std::move(data));
}
void BnwUnitArtScaleCaller::validate_quote(const BnwUnitCompiledMemberScaleQuote& quote)const{impl_->validate();need(quote.data_&&quote.data_->owner_identity.get()==impl_.get(),"Compiled scale quote foreign caller owner");quote.data_->validate();}
const assets::ResourceMountTable& BnwUnitArtScaleCaller::resources_for_quote(const BnwUnitCompiledMemberScaleQuote& quote)const{validate_quote(quote);return *impl_->resources;}
const BnwUnitMemberTransformSource& BnwUnitCompiledMemberScaleQuote::xml_declaration()const{need(bool(data_),"Compiled scale moved quote");data_->validate();return data_->xml;}
const BnwUnitMemberTransformRules& BnwUnitCompiledMemberScaleQuote::compiled_rules()const{need(bool(data_),"Compiled scale moved quote");data_->validate();return data_->compiled;}
const std::string& BnwUnitCompiledMemberScaleQuote::model()const{need(bool(data_),"Compiled scale moved quote");data_->validate();return data_->model;}
BnwUnitMemberTransformRules BnwUnitCompiledMemberScaleQuote::placement_after_resource_conversion()const{return {1.f,compiled_rules().z_offset};}
} // namespace civ5::graphics


