#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/content/lua_assets.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include "civ5/content/mod_script_registry.hpp"
#include "civ5/content/vp_content_profile.hpp"
#include "civ5/content/vp_lua_assets_profile.hpp"
#include "civ5/runtime/lua_content_services.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
namespace {
std::string json_string(std::string_view value){
    constexpr char hex[]="0123456789abcdef";
    std::string result="\"";
    for(const unsigned char c:value){
        if(c=='\\'||c=='\"'){result+='\\';result+=static_cast<char>(c);}
        else if(c<32){result+="\\u00";result+=hex[c>>4];result+=hex[c&15];}
        else result+=static_cast<char>(c);
    }
    return result+'\"';
}
}
int wmain(int argc,wchar_t **argv) {
    try {
        if(argc<2)throw std::runtime_error("Usage: civis_lua_smoke [--include-plan PLAN.xml] [--mod-registry REGISTRY.xml --save SAVE --load-entrypoints TYPE] [--mods DIR --use-installed-mod-bytes] [--vp-bundle DIR --vp-component NAME] [--ui-notification NAME ...] [--context NAME] source.lua [--context NAME check.lua ...]");
        std::optional<civ5::content::LooseResourcePlan> includes;
        std::optional<std::filesystem::path> registry_plan,save,vp_bundle;
        std::optional<std::string> vp_component_name;
        std::vector<std::pair<std::string,std::filesystem::path>> chunks;
        std::vector<std::string> entrypoint_types;
        std::vector<std::string> ui_notifications;
        std::vector<std::filesystem::path> mod_roots;
        bool use_installed_mod_bytes=false;
        std::string current_context="asset-smoke";
        for(int i=1;i<argc;++i){
            if(std::wstring_view(argv[i])==L"--include-plan"){
                if(includes||i+1>=argc)throw std::runtime_error("Expected one Lua include mount plan");
                includes=civ5::content::read_loose_resource_plan(argv[++i]);
            }else if(std::wstring_view(argv[i])==L"--mod-registry"){
                if(registry_plan||i+1>=argc)throw std::runtime_error("Expected one mod registry plan");
                registry_plan=argv[++i];
            }else if(std::wstring_view(argv[i])==L"--save"){
                if(save||i+1>=argc)throw std::runtime_error("Expected one save selection");save=argv[++i];
            }else if(std::wstring_view(argv[i])==L"--mods"){
                if(i+1>=argc)throw std::runtime_error("Expected installed mod root");mod_roots.emplace_back(argv[++i]);
            }else if(std::wstring_view(argv[i])==L"--use-installed-mod-bytes"){
                use_installed_mod_bytes=true;
            }else if(std::wstring_view(argv[i])==L"--vp-bundle"){
                if(vp_bundle||i+1>=argc)throw std::runtime_error("Expected one verified VP bundle");vp_bundle=argv[++i];
            }else if(std::wstring_view(argv[i])==L"--vp-component"){
                if(vp_component_name||i+1>=argc)throw std::runtime_error("Expected one VP installer component");
                const auto value=std::filesystem::path(argv[++i]).u8string();
                vp_component_name=std::string(value.begin(),value.end());
            }else if(std::wstring_view(argv[i])==L"--load-entrypoints"){
                if(i+1>=argc)throw std::runtime_error("Expected mod entrypoint type");
                const auto type=std::filesystem::path(argv[++i]).u8string();
                if(type.empty())throw std::runtime_error("Expected nonempty mod entrypoint type");
                entrypoint_types.emplace_back(type.begin(),type.end());
            }else if(std::wstring_view(argv[i])==L"--ui-notification"){
                if(i+1>=argc)throw std::runtime_error("Expected UI notification name");
                const auto value=std::filesystem::path(argv[++i]).u8string();
                if(value.empty())throw std::runtime_error("Expected nonempty UI notification name");
                ui_notifications.emplace_back(value.begin(),value.end());
            }else if(std::wstring_view(argv[i])==L"--context"){
                if(i+1>=argc)throw std::runtime_error("Expected Lua context name");
                const auto name=std::filesystem::path(argv[++i]).u8string();
                current_context.assign(name.begin(),name.end());
                if(current_context.empty())throw std::runtime_error("Expected nonempty Lua context name");
            }else chunks.emplace_back(current_context,argv[i]);
        }
        if(chunks.empty()&&entrypoint_types.empty())throw std::runtime_error("Expected a Lua source chunk or mod entrypoint type");
        if(!entrypoint_types.empty()&&!registry_plan)throw std::runtime_error("Mod entrypoint loading requires --mod-registry and --save");
        if(use_installed_mod_bytes&&(!registry_plan||mod_roots.empty()))
            throw std::runtime_error("--use-installed-mod-bytes requires --mod-registry, --save and --mods");
        if(static_cast<bool>(vp_bundle)!=static_cast<bool>(vp_component_name)||
           (vp_bundle&&(!registry_plan||!includes)))
            throw std::runtime_error("VP Lua omissions require bundle, component, mod registry, save and include plan");
        civ5::runtime::ScriptSourceProvider provider;
        std::uint64_t reads=0;
        if(includes){
            civ5::content::mount_base_game_lua(includes->resources,includes->root);
            // Diagnostic inventory only. Game activation must supply its own
            // verified ordered IncludeFileList; keys() is a sorted snapshot.
            provider=civ5::content::lua_asset_sources(includes->resources,[&](std::string_view){
                std::vector<std::string> names;
                for(const auto &name:includes->resources.keys())
                    if(civ5::assets::normalize_asset_name(name).ends_with(".lua"))names.push_back(name);
                return names;
            });
            const auto read=provider.read;
            provider.read=[&,read](std::string_view context,std::string_view name){++reads;return read(context,name);};
        }
        std::unique_ptr<civ5::content::ModScriptRegistry> registry;
        civ5::runtime::ScriptContentProvider content_provider;
        std::uint64_t selected_mods=0;
        std::uint64_t lua_import_declared=0,lua_import_mounted=0,lua_import_profile_omitted=0;
        std::uint64_t lua_import_md5_mismatches=0;
        std::size_t lua_import_failed=0;
        std::vector<std::size_t> installed_byte_packages;
        bool profile_files_complete=false;
        if(static_cast<bool>(registry_plan)!=static_cast<bool>(save))throw std::runtime_error("Registry diagnostics require both --mod-registry and --save");
        if(registry_plan){
            using namespace civ5::content;
            const auto plan=read_xml(*registry_plan);
            if(plan.name!="LuaModRegistry")throw std::runtime_error("Expected LuaModRegistry diagnostic plan");
            Catalog catalog;std::vector<ModRegistration> ordered;
            for(const auto &node:plan.children){
                if(node.name!="Mod"&&node.name!="Dlc")throw std::runtime_error("Unexpected Lua registry plan element");
                const auto value=node.attribute("manifest");
                if(value.empty())throw std::runtime_error("Registry plan requires manifest path");
                auto path=std::filesystem::path(std::u8string(value.begin(),value.end()));
                if(path.is_relative())path=registry_plan->parent_path()/path;
                const auto index=catalog.packages.size();catalog.packages.push_back(read_manifest(path));
                if(node.name=="Mod"){
                    const auto rowid=decimal(node.attribute("rowid"));
                    if(rowid>INT64_MAX)throw std::runtime_error("Registry row ID exceeds int64");
                    ModRegistration record;record.package=index;record.row_id=static_cast<std::int64_t>(rowid);
                    const auto type=node.attribute("type");
                    if(!type.empty()){
                        const auto value=decimal(type);if(value>INT64_MAX)throw std::runtime_error("Registry type exceeds int64");
                        record.type=static_cast<std::int64_t>(value);
                    }
                    const auto flag=[&](const char *name,bool fallback){
                        const auto value=node.attribute(name);
                        if(value.empty())return fallback;
                        if(value!="0"&&value!="1")throw std::runtime_error("Registry flag requires 0 or 1");
                        return value=="1";
                    };
                    record.installed=flag("installed",true);record.enabled=flag("enabled",false);
                    for(const auto &[key,value]:node.attributes)if(key=="last-retrieved")record.last_retrieved=value;
                    ordered.push_back(record);
                }else if(catalog.packages.back().kind!=PackageKind::dlc)throw std::runtime_error("Expected DLC descriptor");
            }
            const auto selection=civ5::runtime::select_save(*save);
            if(!selection.selected)throw std::runtime_error("No readable save selected");
            const auto &header=selection.candidates.at(*selection.selected).header;
            auto profile=civ5::runtime::resolve_saved_content_profile(header,catalog);
            std::vector<civ5::runtime::ManifestFileOmission> file_omissions;
            std::vector<LuaImportOmission> lua_omissions;
            if(vp_bundle&&profile.all_requirements_located){
                const auto installer=build_vp_542_profile(*vp_bundle,vp_component(*vp_component_name));
                file_omissions=vp_manifest_file_omissions(installer,catalog,profile);
                lua_omissions=vp_lua_import_omissions(installer,catalog,profile);
                profile=civ5::runtime::resolve_saved_content_profile(header,catalog,file_omissions);
            }
            selected_mods=profile.enabled_mods.size();
            content_provider=civ5::runtime::lua_content_services(catalog,profile);
            profile_files_complete=profile.files_complete;
            if(use_installed_mod_bytes)for(const auto index:profile.enabled_mods){
                const auto package_root=std::filesystem::canonical(catalog.packages.at(index).path.parent_path());
                for(const auto &mod_root:mod_roots){
                    const auto relative=package_root.lexically_relative(std::filesystem::canonical(mod_root));
                    if(!relative.empty()&&!relative.is_absolute()&&*relative.begin()!=std::filesystem::path("..")){
                        installed_byte_packages.push_back(index);break;
                    }
                }
            }
            registry=std::make_unique<ModScriptRegistry>(catalog,profile,ordered,installed_byte_packages);
            if(includes){
                std::vector<std::size_t> registration_order;
                for(const auto &mod:ordered)registration_order.push_back(mod.package);
                const auto imported=mount_selected_mod_lua(includes->resources,catalog,profile,registration_order,lua_omissions,installed_byte_packages);
                lua_import_declared=imported.declared;lua_import_mounted=imported.mounted;
                lua_import_profile_omitted=imported.profile_omitted;
                lua_import_md5_mismatches=imported.manifest_md5_mismatches;
                lua_import_failed=imported.failures.size();
                if(lua_import_failed){
                    std::cerr<<"Mod Lua imports incomplete: "<<lua_import_mounted<<" mounted, "
                             <<lua_import_profile_omitted<<" profile-omitted, "
                             <<lua_import_failed<<" failed of "<<lua_import_declared<<'\n';
                    for(std::size_t i=0;i<std::min<std::size_t>(3,lua_import_failed);++i)
                        std::cerr<<imported.failures[i]<<'\n';
                }
            }
        }
        civ5::runtime::ScriptUiEventProvider ui;
        ui.notifications=ui_notifications; // Diagnostic subscriptions only; no graphics commands.
        civ5::runtime::LuaScriptHost scripts({256ULL*1024*1024,10000000},std::move(provider),
                                            registry?registry->provider():civ5::runtime::ScriptModdingProvider{},
                                            {},{},{},{},std::move(ui),std::move(content_provider));
        std::size_t index=0;
        std::set<std::string> contexts;
        std::size_t entrypoints_loaded=0,entrypoints_deferred=0,entrypoints_md5_mismatches=0;
        struct EntrypointFailure {std::string mod,path,error;};
        std::vector<EntrypointFailure> entrypoint_failures;
        for(const auto &type:entrypoint_types){
            for(const auto &entry:registry->planned_entry_points(type)){
                auto suffix=entry.file;
                for(auto &c:suffix)if(c>='A'&&c<='Z')c=static_cast<char>(c-'A'+'a');
                if(!suffix.ends_with(".lua")){++entrypoints_deferred;continue;}
                const auto context="mod-entrypoint-"+std::to_string(++index);
                try{
                    entrypoints_md5_mismatches+=!execute_mod_lua_entrypoint(entry,scripts,context);
                    contexts.insert(context);++entrypoints_loaded;
                }catch(const std::exception &error){
                    entrypoint_failures.push_back({entry.mod_id,entry.file,error.what()});
                    std::cerr<<"Mod Lua entrypoint failed: "<<entry.mod_id<<" / "<<entry.file
                             <<": "<<error.what()<<'\n';
                }
            }
        }
        for(const auto &[context,path]:chunks){
            std::ifstream in(path,std::ios::binary);
            if(!in)throw std::runtime_error("Cannot read Lua source");
            const std::string source{std::istreambuf_iterator<char>(in),{}};
            if(in.bad())throw std::runtime_error("Lua source read failed");
            scripts.execute(context,source,"@asset-"+std::to_string(++index));contexts.insert(context);
        }
        for(const auto &error:scripts.source_errors())std::cerr<<error.path<<": "<<error.message<<'\n';
        // This probe selects named entrypoint types, not the complete game/UI
        // load sequence. It cannot attest LuaScriptHost game readiness.
        const bool incomplete=!entrypoint_types.empty()||lua_import_failed!=0||!entrypoint_failures.empty();
        if(!incomplete)scripts.complete_loading();
        std::cout<<"{\"result\":\""<<(incomplete?"INCOMPLETE":"PASS")<<"\",\"runtime_bits\":"<<sizeof(void*)*8<<",\"lua\":\""<<scripts.version()
                <<"\",\"chunks\":"<<chunks.size()<<",\"contexts\":"<<contexts.size()<<",\"sources_read\":"<<reads
                <<",\"inventory_order\":\""<<(includes?"diagnostic-sorted":"none")
                <<"\",\"selected_mods\":"<<selected_mods<<",\"registry_order\":\""<<(registry?"explicit-diagnostic":"none")
                <<"\",\"profile_files_complete\":"<<(profile_files_complete?"true":"false")
                <<",\"use_installed_mod_bytes\":"<<(use_installed_mod_bytes?"true":"false")
                <<",\"installed_byte_packages\":"<<installed_byte_packages.size()
                <<",\"ui_notification_capabilities\":"<<ui_notifications.size()
                <<",\"lua_import_declared\":"<<lua_import_declared<<",\"lua_import_mounted\":"<<lua_import_mounted
                <<",\"lua_import_profile_omitted\":"<<lua_import_profile_omitted
                <<",\"lua_import_md5_mismatches\":"<<lua_import_md5_mismatches
                <<",\"lua_import_failed\":"<<lua_import_failed
                <<",\"entrypoints_loaded\":"<<entrypoints_loaded<<",\"entrypoints_deferred\":"<<entrypoints_deferred
                <<",\"entrypoints_md5_mismatches\":"<<entrypoints_md5_mismatches
                <<",\"entrypoints_failed\":"<<entrypoint_failures.size()<<",\"entrypoint_failure_samples\":[";
        for(std::size_t i=0;i<std::min<std::size_t>(16,entrypoint_failures.size());++i){
            if(i)std::cout<<',';
            const auto &failure=entrypoint_failures[i];
            std::cout<<"{\"mod\":"<<json_string(failure.mod)<<",\"path\":"<<json_string(failure.path)
                     <<",\"error\":"<<json_string(failure.error)<<'}';
        }
        std::cout<<"],\"memory_bytes\":"<<scripts.memory_bytes()<<",\"full_game\":false}\n";
        return incomplete?3:0;
    }catch(const std::exception &e){std::cerr<<"ERROR "<<e.what()<<'\n';return 1;}
}
