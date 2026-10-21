#pragma once
#include "civ5/content/lua_assets.hpp"
#include "civ5/content/mod_script_registry.hpp"
#include "civ5/runtime/lua_gameplay_services.hpp"
#include "civ5/runtime/lua_gamecore_enums.hpp"
#include "civ5/runtime/lua_locale_services.hpp"
#include "civ5/runtime/lua_content_services.hpp"
#include "civ5/runtime/lua_map_services.hpp"
#include "civ5/runtime/pregame_roster.hpp"
#include "civ5/runtime/saved_value_fallback.hpp"
#include <algorithm>
#include <climits>
#include <filesystem>
#include <sstream>

namespace civ5::validation {
inline std::string lua_report_json(std::string_view value){
    constexpr char hex[]="0123456789abcdef";
    std::string result="\"";
    for(const unsigned char c:value){
        if(c=='\\'||c=='\"'){result+='\\';result+=static_cast<char>(c);}
        else if(c<32){result+="\\u00";result+=hex[c>>4];result+=hex[c&15];}
        else result+=static_cast<char>(c);
    }
    return result+'\"';
}
// The plan supplies Modding registration order, which the save header does not
// contain. Scripts use the activated save's rules, map and roster, but all
// writable Lua databases are private diagnostic copies. No GameCore is chosen
// here; callers may supply a profile's exact declared Lua omissions.
inline std::string lua_selected_entrypoints_report(
    const std::filesystem::path &registry_plan,std::string_view type,
    const std::filesystem::path &game_root,const assets::ResourceMountTable &resources,
    const content::Catalog &catalog,const runtime::SavedContentProfile &profile,
    const content::RulesDatabase &rules,const saves::SavedSessionSnapshot &snapshot,
    const runtime::ResearchContext &players,const std::vector<runtime::TeamTechnologies> &teams,
    const content::InfoTypeCatalog &types,const content::CustomModOptions *options,
    std::span<const content::LuaImportOmission> omissions,
    std::span<const std::size_t> installed_byte_packages,
    std::span<const std::string> ui_notifications,
    const std::filesystem::path &user_data_root={},std::string_view ui_language={},
    const std::optional<runtime::SavedValueFallbackReport> &fallback_report={},
    std::span<const runtime::VerifiedManifestHash> verified_hashes={}){
    using namespace content;
    const auto plan=read_xml(registry_plan);
    if(plan.name!="LuaModRegistry")throw std::runtime_error("Expected LuaModRegistry diagnostic plan");
    std::vector<ModRegistration> ordered;
    for(const auto &node:plan.children){
        if(node.name!="Mod"&&node.name!="Dlc")throw std::runtime_error("Unexpected Lua registry plan element");
        const auto value=node.attribute("manifest");
        if(value.empty())throw std::runtime_error("Lua registry plan requires manifest path");
        auto path=std::filesystem::path(std::u8string(value.begin(),value.end()));
        if(path.is_relative())path=registry_plan.parent_path()/path;
        const auto descriptor=read_manifest(path);
        if(node.name=="Dlc"){
            if(descriptor.kind!=PackageKind::dlc)throw std::runtime_error("Expected DLC descriptor");
            continue;
        }
        if(descriptor.kind!=PackageKind::mod)throw std::runtime_error("Expected mod descriptor");
        const auto found=std::find_if(profile.enabled_mods.begin(),profile.enabled_mods.end(),[&](auto index){
            const auto &selected=catalog.packages.at(index);
            return selected.id==descriptor.id&&selected.version==descriptor.version;
        });
        if(found==profile.enabled_mods.end())throw std::runtime_error("Registry plan names a mod not selected by the save");
        const auto rowid=decimal(node.attribute("rowid"));
        if(rowid>INT64_MAX)throw std::runtime_error("Registry row ID exceeds int64");
        ModRegistration record;record.package=*found;record.row_id=static_cast<std::int64_t>(rowid);
        const auto source_type=node.attribute("type");
        if(!source_type.empty()){
            const auto parsed=decimal(source_type);
            if(parsed>INT64_MAX)throw std::runtime_error("Registry type exceeds int64");
            record.type=static_cast<std::int64_t>(parsed);
        }
        const auto flag=[&](const char *name,bool fallback){
            const auto value=node.attribute(name);
            if(value.empty())return fallback;
            if(value!="0"&&value!="1")throw std::runtime_error("Registry flag requires 0 or 1");
            return value=="1";
        };
        record.installed=flag("installed",true);record.enabled=flag("enabled",false);
        for(const auto &[key,last]:node.attributes)if(key=="last-retrieved")record.last_retrieved=last;
        ordered.push_back(record);
    }
    ModScriptRegistry registry(catalog,profile,ordered,installed_byte_packages,verified_hashes);
    std::vector<std::size_t> order;
    for(const auto &entry:ordered)order.push_back(entry.package);
    auto lua_resources=resources;
    mount_base_game_lua(lua_resources,game_root);
    const auto imports=mount_selected_mod_lua(lua_resources,catalog,profile,order,omissions,
                                              installed_byte_packages,verified_hashes);
    // The original IncludeFileList is not yet reconstructed. A sorted key
    // snapshot makes this explicit probe reproducible, never game-ready.
    std::vector<std::string> include_files;
    for(const auto &name:lua_resources.keys())
        if(assets::normalize_asset_name(name).ends_with(".lua"))include_files.push_back(name);
    auto sources=lua_asset_sources(lua_resources,[&](std::string_view){return include_files;});
    auto rules_copy=rules.clone_gameplay();
    std::optional<runtime::ScriptLocaleSession> locale;
    if(!ui_language.empty())locale.emplace(*rules_copy,std::string(ui_language));
    auto player_copy=players;
    auto roster=runtime::restore_pregame_roster(*snapshot.pregame,*rules_copy);
    std::optional<runtime::PlotPassability> passability;
    runtime::ScriptMapProvider map;
    if(snapshot.map_projection){
        passability=runtime::restore_plot_passability(snapshot,*rules_copy,types);
        map=runtime::lua_map_services(*snapshot.map_projection,*passability);
    }
    std::shared_ptr<RulesDatabase> save_data;
    if(!snapshot.saved_database.image.empty()&&snapshot.saved_database.quick_check_passed)
        save_data=RulesDatabase::from_saved_image(snapshot.saved_database.image);
    runtime::ScriptUiEventProvider ui;
    ui.notifications.assign(ui_notifications.begin(),ui_notifications.end());
    // This diagnostic supplies only the Firaxis SDK baseline. A selected
    // replacement GameCore can extend or replace these through its module.
    const bool expansion_gamecore=std::any_of(profile.enabled_dlc.begin(),profile.enabled_dlc.end(),
        [&](std::size_t index){
            const auto &id=catalog.packages.at(index).id;
            return id=="0e3751a1-f840-4e1b-9706-519bf484e59d"||
                   id=="6da07636-4123-4018-b643-6575b4ec336b";
        });
    runtime::LuaScriptHost scripts({256ULL*1024*1024,100000000},std::move(sources),registry.provider(),
        runtime::lua_gameplay_services(*rules_copy,player_copy,teams,types,options,&roster),
        save_data?runtime::lua_saved_data_services(save_data):runtime::ScriptSaveDataProvider{},
        runtime::lua_pregame_services(roster,*rules_copy),std::move(map),std::move(ui),
        runtime::lua_content_services(catalog,profile),runtime::lua_user_data_services(user_data_root),
        runtime::lua_civ5_gamecore_enums(expansion_gamecore),
        locale?locale->provider():runtime::ScriptLocaleProvider{});
    std::size_t loaded=0,deferred=0,md5_mismatches=0;
    struct Failure {std::string mod,path,error;};
    std::vector<Failure> failures;
    std::size_t ordinal=0;
    for(const auto &entry:registry.planned_entry_points(type)){
        auto suffix=entry.file;
        for(auto &c:suffix)if(c>='A'&&c<='Z')c=static_cast<char>(c-'A'+'a');
        if(!suffix.ends_with(".lua")){++deferred;continue;}
        try{
            md5_mismatches+=!execute_mod_lua_entrypoint(entry,scripts,"selected-entrypoint-"+std::to_string(++ordinal));
            ++loaded;
        }catch(const std::exception &error){failures.push_back({entry.mod_id,entry.file,error.what()});}
    }
    std::ostringstream out;
    out<<"{\"registry_order\":\"explicit-diagnostic\",\"include_order\":\"diagnostic-sorted\",\"full_game\":false"
       <<",\"selected_mods\":"<<profile.enabled_mods.size()<<",\"registered_mods\":"<<ordered.size()
       <<",\"lua_import_declared\":"<<imports.declared<<",\"lua_import_mounted\":"<<imports.mounted
       <<",\"lua_import_profile_omitted\":"<<imports.profile_omitted
       <<",\"lua_import_md5_mismatches\":"<<imports.manifest_md5_mismatches
       <<",\"lua_import_failed\":"<<imports.failures.size()
       <<",\"entrypoints_loaded\":"<<loaded<<",\"entrypoints_deferred\":"<<deferred
       <<",\"entrypoints_md5_mismatches\":"<<md5_mismatches
       <<",\"entrypoints_failed\":"<<failures.size()<<",\"saved_data_available\":"<<(save_data?"true":"false")
       <<",\"map_available\":"<<(passability?"true":"false")
       <<",\"civ5_sdk_enum_baseline\":"<<lua_report_json(expansion_gamecore?"expansion":"base")
       <<",\"ui_language\":"<<(locale?lua_report_json(ui_language):"null")
       <<",\"saved_value_fallback\":";
    if(fallback_report)
        out<<"{\"copied\":"<<fallback_report->copied<<",\"present\":"<<fallback_report->present
           <<",\"present_null\":"<<fallback_report->present_null
           <<",\"matching_source\":"<<fallback_report->matching_source
           <<",\"missing_source\":"<<fallback_report->missing_source<<'}';
    else out<<"null";
    out
       <<",\"ui_language_revision\":"<<(locale?locale->revision():0)
       <<",\"failure_samples\":[";
    for(std::size_t i=0;i<std::min<std::size_t>(16,failures.size());++i){
        if(i)out<<',';
        const auto &f=failures[i];
        out<<"{\"mod\":"<<lua_report_json(f.mod)<<",\"path\":"<<lua_report_json(f.path)
           <<",\"error\":"<<lua_report_json(f.error)<<'}';
    }
    out<<"],\"import_failure_samples\":[";
    for(std::size_t i=0;i<std::min<std::size_t>(8,imports.failures.size());++i){
        if(i)out<<',';out<<lua_report_json(imports.failures[i]);
    }
    out<<"],\"source_errors\":"<<scripts.source_errors().size()<<",\"memory_bytes\":"<<scripts.memory_bytes()<<'}';
    return out.str();
}
} // namespace civ5::validation
