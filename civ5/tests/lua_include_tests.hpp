#pragma once
void lua_include_tests() {
    using namespace civ5::runtime;
    using namespace civ5::assets;
    std::vector<std::string> names{"Lib/One.lua","Lib/Two.lua","Lib/Nested.lua"};
    std::map<std::string,std::string> sources{
        {"Lib/One.lua","hits=(hits or 0)+1; return 'discarded'"},
        {"Lib/Two.lua","hits=(hits or 0)+10"},
        {"Lib/Nested.lua","nested=include('One%.lua$'); assert(#nested==1)"}};
    std::vector<std::pair<std::string,std::string>> reads;
    ScriptSourceProvider provider{
        [&](std::string_view){return names;},
        [&](std::string_view context,std::string_view path){reads.emplace_back(context,path);return sources.at(std::string(path));}};
    rejects([&]{LuaScriptHost partial({}, {provider.files,{}});});
    LuaScriptHost host({},provider);
    host.execute("A",R"lua(
        assert(type(IncludeFileList)=='table' and #IncludeFileList==3)
        assert(#include('missing')==0)
        local result=include('Lib/.*%.lua$')
        assert(#result==1 and result[1]=='Lib/One.lua' and hits==1)
        assert(#include('One%.lua$')==1 and hits==2)
        assert(#include('one%.lua$')==0) -- Lua patterns are case-sensitive.
        assert(#include('Nested')==1 and hits==3 and #nested==1)
        assert(#include('Lib/[OT].*',true)==2 and hits==14)
        assert(#include('Lib/[OT].*',0)==2 and hits==25) -- zero is truthy
    )lua","=include.lua");
    check(reads.size()==8&&reads.front()==std::pair(std::string("A"),std::string("Lib/One.lua")),"Include must repeat, nest, use Lua patterns and return matched paths rather than chunk values");
    host.execute("B","assert(hits==nil); include('Two'); assert(hits==10)","=context-b.lua");
    host.execute("A","assert(hits==25)","=context-a.lua");
    check(reads.back().first=="B","Included code and provider must receive the owning context");
    names.push_back("Lib/Late.lua");sources["Lib/Late.lua"]="late=true";
    host.execute("A",R"lua(
        assert(#include('Late')==0)
        local original=IncludeFileList
        assert(RefreshIncludeFileList()==nil and original==IncludeFileList)
        assert(#include('Late')==1 and late==true)
        IncludeFileList={"Lib/Two.lua"}
        assert(#include('One')==1) -- captured original survives replacement
        IncludeFileList=original
        IncludeFileList[1]=false
        assert(#include('One')==0)
        RefreshIncludeFileList()
        local find=string.find
        string.find=function() return false end
        assert(#include('anything')==1) -- native checks nil, not truthiness
        string.find=find
    )lua","=refresh.lua");
    host.complete_loading();check(host.loading_complete()&&host.source_errors().empty(),"Refresh preserves table identity; replacement, nonstring slots and native match test must work");
    names={"bad.lua","after.lua","syntax.lua","binary.lua","bom.lua","vanished.lua"};
    sources["bad.lua"]="partial=7; error('included failure')";sources["after.lua"]="after=true";
    sources["syntax.lua"]="local =";sources["binary.lua"]=std::string("\x1bLua",4);
    sources["bom.lua"]="\xef\xbb\xbf" "bom=true";
    // A real chunk failure quarantines subsequent provider/compiler operations.
    // This Civis fail-stop contract after a terminal source error is not a
    // claim of full native Civ5 include continuation parity. Matched paths
    // and the already reached outer Lua effects still remain.
    const auto broken_reads_before=reads.size();
    LuaScriptHost broken({},provider);
    broken.execute("Broken",R"lua(
        local result=include('.*',true)
        assert(#result==6 and result[1]=='bad.lua' and result[6]=='vanished.lua')
        local expected={'bad.lua','after.lua','syntax.lua','binary.lua','bom.lua','vanished.lua'}
        for i,path in ipairs(expected) do assert(result[i]==path) end
        assert(partial==7 and after==nil and bom==nil)
        outerAfterTraversal=13
    )lua","=failed-includes.lua");
    check(reads.size()==broken_reads_before+1&&reads.back()==std::pair(std::string("Broken"),std::string("bad.lua")),
          "A failed source must quarantine later compiler/provider reads, not pretend unseen files were compiled");
    check(broken.source_errors().size()==6&&broken.source_errors()[0].context=="Broken"&&broken.source_errors()[0].path=="bad.lua"&&
          broken.source_errors()[0].message.find("included failure")!=std::string::npos,
          "Actual reached chunk error and five subsequent refused compiler operations must remain distinct");
    for(std::size_t i=1;i<6;++i)check(broken.source_errors()[i].context=="Broken"&&broken.source_errors()[i].path==names[i]&&
          broken.source_errors()[i].message=="Include provider lender has no same compiler operation",
          "Historical failure must refuse the actual next provider/compiler lender before source execution");
    rejects([&]{broken.complete_loading();});
    const auto failed_journal=broken.execution_snapshot();
    std::vector<std::string> attempted_paths,failed_paths;bool reached_bad=false,outer_retained=false;
    for(const auto &record:failed_journal.records) {
        check(record.outcome!=ScriptLuaActionOutcome::open,"Failed include traversal cannot leave an open actual journal action");
        if(record.action==ScriptLuaAction::include) {
            attempted_paths.push_back(record.name);
            check(record.outcome==ScriptLuaActionOutcome::error,"Each poisoned traversal attempt must preserve its actual compiler/chunk refusal");
            if(record.outcome==ScriptLuaActionOutcome::error)failed_paths.push_back(record.name);
            if(record.name=="bad.lua")reached_bad=record.source_bytes==sources.at("bad.lua").size()&&record.source_sha256.size()==64;
            else check(record.source_bytes==0&&record.source_sha256.empty(),"Quarantined/unread source must not receive a compiled-byte witness");
        }
        if(record.action==ScriptLuaAction::context_execute&&record.name=="=failed-includes.lua")
            outer_retained=record.outcome==ScriptLuaActionOutcome::uncertain;
    }
    check(failed_journal.failed&&attempted_paths==names&&failed_paths==names&&reached_bad&&outer_retained,
          "Reached failure, ordered matched-path refusals and outer uncertainty must preserve the real quarantine boundary");
    broken.execute("Broken","assert(partial==7 and after==nil and bom==nil and outerAfterTraversal==13)","=failed-includes-reached-effects");
    rejects([&]{broken.complete_loading();});

    // These compiler cases require fresh healthy operations. Following a
    // poisoned include traversal would exercise only the prior-failure guard.
    const auto compiler_failure=[&](const char* context,const char* path,std::string_view pattern,std::size_t bytes,bool captured){
        LuaScriptHost vm({},provider);const auto before=reads.size();
        vm.execute(context,"assert(#include('"+std::string(pattern)+"')==1); reachedOuter=true","=fresh-include-compiler-failure");
        check(reads.size()==before+1&&reads.back()==std::pair(std::string(context),std::string(path))&&
              vm.source_errors().size()==1&&vm.source_errors()[0].context==context&&vm.source_errors()[0].path==path,
              "Fresh failed compiler must actually reach its selected source/provider");
        const auto journal=vm.execution_snapshot();bool reached=false;
        for(const auto& record:journal.records)if(record.action==ScriptLuaAction::include&&record.name==path)
            reached=record.outcome==ScriptLuaActionOutcome::error&&record.source_bytes==bytes&&
                (captured?record.source_sha256.size()==64:record.source_sha256.empty());
        check(journal.failed&&reached&&!vm.loading_complete(),"Real source/parse/bytecode/IO errors must retain accurate byte witnesses and sticky failure");
        rejects([&]{vm.complete_loading();});
    };
    compiler_failure("SyntaxProbe","syntax.lua","syntax%.lua$",sources.at("syntax.lua").size(),true);
    compiler_failure("BytecodeProbe","binary.lua","binary%.lua$",4,true);
    compiler_failure("IoProbe","vanished.lua","vanished%.lua$",0,false);
    {
        LuaScriptHost vm({},provider);const auto before=reads.size();
        vm.execute("BomProbe","assert(#include('bom%.lua$')==1 and bom==true)","=fresh-bom-include");
        const auto journal=vm.execution_snapshot();bool reached=false;
        for(const auto& record:journal.records)if(record.action==ScriptLuaAction::include&&record.name=="bom.lua")
            reached=record.outcome==ScriptLuaActionOutcome::succeeded&&record.source_bytes==8&&record.source_sha256.size()==64;
        vm.complete_loading();
        check(reads.size()==before+1&&reads.back()==std::pair(std::string("BomProbe"),std::string("bom.lua"))&&
              reached&&vm.loading_complete()&&vm.source_errors().empty(),
              "Fresh genuine BOM source executes and records only the actual BOM-adjusted compiler bytes");
    }
    LuaScriptHost deferred({},provider);
    deferred.execute("Deferred","GameEvents.Check.Add(function() include('bad'); return true end)","=deferred.lua");
    deferred.complete_loading();
    const auto result=deferred.test_all("Check");
    check(result.value==true&&result.errors.size()==1&&!deferred.loading_complete(),"Runtime include failure must reach the rules bridge and invalidate loading readiness");
    LuaScriptHost malformed({},provider);rejects([&]{malformed.execute("Bad","include('[')","=pattern.lua");});
    LuaScriptHost enumeration({}, {[](std::string_view)->std::vector<std::string>{throw std::runtime_error("enumeration failed");},provider.read});
    rejects([&]{enumeration.execute("A","assert(false)","=enumeration.lua");});
    rejects([&]{enumeration.complete_loading();});
    bool fail_refresh=false;
    LuaScriptHost refresh_failure({}, {[&](std::string_view){if(fail_refresh)throw std::runtime_error("refresh failed");return names;},provider.read});
    refresh_failure.execute("A","GameEvents.Refresh.Add(function() pcall(RefreshIncludeFileList); return true end)","=refresh-handler.lua");
    refresh_failure.complete_loading();fail_refresh=true;
    LuaResearchSelectionServices refresh_bridge(refresh_failure,[](std::int64_t){});
    rejects([&]{refresh_bridge.test_all("Refresh",0,0);});
    LuaScriptHost nonstandard({}, {provider.files,[](std::string_view,std::string_view)->std::string{throw 7;}});
    nonstandard.execute("A","assert(#include('bad')==1)","=foreign-exception.lua");
    check(nonstandard.source_errors().size()==1,"Provider exceptions must stay inside the protected Lua boundary");
    names={"append.lua","last.lua"};sources["append.lua"]="IncludeFileList[3]='Lib/One.lua'";sources["last.lua"]="last=true";
    LuaScriptHost mutation({},provider);
    mutation.execute("A","assert(#include('.*',true)==2 and hits==nil); assert(#include('.*',true)==3 and hits==1)","=list-mutation.lua");
    mutation.complete_loading();check(mutation.loading_complete(),"Include traversal must snapshot count but read live list entries");

    const auto root=std::filesystem::temp_directory_path()/("civis-lua-assets-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root/"base");std::filesystem::create_directories(root/"mod");
    {std::ofstream out(root/"base/Rules.lua");out<<"chosen='base'; value=5000000001";}
    {std::ofstream out(root/"mod/Rules.lua");out<<"chosen='mod'; value=5000000002";}
    auto base=std::make_shared<LooseResourceIndex>(root),mod=std::make_shared<LooseResourceIndex>(root);
    base->add_file("base/Rules.lua");mod->add_file("mod/Rules.lua");
    ResourceMountTable mounts(resource_lookup(base));
    mounts.mount({5000000001ULL,0,0,resource_lookup(mod)});
    const auto ordered=[](std::string_view){return std::vector<std::string>{"Rules.lua"};};
    LuaScriptHost mounted({},lua_asset_sources(mounts,ordered));
    mounted.execute("A","include('Rules'); assert(chosen=='mod' and value==5000000002)","=mounted.lua");
    mounts.set_enabled(5000000001ULL,false);
    mounted.execute("A","include('Rules'); assert(chosen=='base' and value==5000000001)","=unmounted.lua");
    mounted.complete_loading();check(mounted.loading_complete(),"Lua must use the shared mount resolver with wide mount IDs and live override selection");
    LuaScriptHost bounded({},lua_asset_sources(mounts,ordered,4));
    bounded.execute("A","assert(#include('Rules')==1)","=source-budget.lua");
    check(bounded.source_errors().size()==1,"Asset source budget failures must be explicit include diagnostics");
    rejects([&]{bounded.complete_loading();});
    std::filesystem::create_directories(root/"Assets/Gameplay/Lua");
    {std::ofstream out(root/"Assets/Gameplay/Lua/FLuaVector.lua");
     out<<"function Vector4(x,y,z,w) return {x=x,y=y,z=z,w=w} end";}
    const auto base_lua_files=mount_base_game_lua(mounts,root);
    ResourceMountTable high_mounts(resource_lookup(base));
    high_mounts.mount({UINT64_MAX,0,0,resource_lookup(base),true,true});
    check(mount_base_game_lua(high_mounts,root)==1&&high_mounts.mounts().back().id!=UINT64_MAX,
          "Base Lua must use a free mount ID when the selected resource table uses UINT64_MAX");
    mounts.set_enabled(5000000001ULL,true);
    LuaScriptHost game_lua({},lua_asset_sources(mounts,[&](std::string_view){
        auto names=mounts.keys();names.push_back("Rules.lua");return names;}));
    game_lua.execute("A",R"lua(
        assert(#include('FLuaVector')==1 and Vector4(1,2,3,4).w==4)
        local selected=include('Rules')
        assert(#selected==1, 'include count '..#selected)
        assert(chosen=='mod', 'selected '..tostring(chosen))
    )lua","=base-game-lua.lua");
    game_lua.complete_loading();
    check(base_lua_files==1&&game_lua.loading_complete()&&game_lua.source_errors().empty(),
          "Base game Lua must load from the selected install without overriding selected mod mounts");
    {std::ofstream out(root/"mod/FLuaVector.lua");
     out<<"function Vector4(x,y,z,w) return {x=x,y=y,z=z,w=w+1} end";}
    Catalog lua_catalog;
    PackageManifest lua_package;lua_package.kind=PackageKind::mod;
    lua_package.path=root/"mod/manifest.modinfo";lua_package.id="mod-lua";lua_package.version=5000000001ULL;
    lua_package.files={{"FLuaVector.lua",file_md5(root/"mod/FLuaVector.lua"),true}};
    lua_catalog.packages.push_back(lua_package);
    SavedContentProfile lua_profile;lua_profile.all_requirements_located=true;lua_profile.enabled_mods={0};
    const std::array<std::size_t,1> mod_order{0};
    auto altered_catalog=lua_catalog;altered_catalog.packages[0].files[0].md5=std::string(32,'0');
    const auto rejected_import=mount_selected_mod_lua(mounts,altered_catalog,lua_profile,mod_order);
    check(rejected_import.declared==1&&rejected_import.mounted==0&&rejected_import.failures.size()==1,
          "Changed imported Lua must not enter the selected source mounts");
    const std::vector<VerifiedManifestHash> verified_lua_hashes{{
        0,"FLuaVector.lua",std::string(32,'0'),file_md5(root/"mod/FLuaVector.lua")}};
    ResourceMountTable pinned_mounts(resource_lookup(base));
    const auto pinned_import=mount_selected_mod_lua(pinned_mounts,altered_catalog,lua_profile,
                                                     mod_order,{}, {},verified_lua_hashes);
    check(pinned_import.complete()&&pinned_import.mounted==1&&
          pinned_import.manifest_md5_mismatches==1,
          "Pinned imported Lua bytes must mount without local-edit override");
    ResourceMountTable installed_mounts(resource_lookup(base));
    installed_mounts.mount({UINT64_MAX,0,0,resource_lookup(base),true,true});
    const auto installed_import=mount_selected_mod_lua(installed_mounts,altered_catalog,lua_profile,mod_order,{},mod_order);
    check(installed_import.complete()&&installed_import.mounted==1&&
          installed_import.manifest_md5_mismatches==1&&installed_mounts.mounts().back().id!=UINT64_MAX,
          "Explicit installed-byte Lua import must retain its MD5 mismatch diagnostic");
    ResourceMountTable unrelated_mounts(resource_lookup(base));
    const std::array<std::size_t,0> no_installed_packages{};
    const auto still_rejected=mount_selected_mod_lua(unrelated_mounts,altered_catalog,lua_profile,mod_order,{},no_installed_packages);
    check(!still_rejected.complete()&&still_rejected.manifest_md5_mismatches==1,
          "Unselected Lua package must keep strict manifest MD5 enforcement");
    const auto accepted_import=mount_selected_mod_lua(mounts,lua_catalog,lua_profile,mod_order);
    LuaScriptHost mod_lua({},lua_asset_sources(mounts,[&](std::string_view){return mounts.keys();}));
    mod_lua.execute("A","assert(#include('FLuaVector')==1 and Vector4(1,2,3,4).w==5)","=mod-lua-override.lua");
    mod_lua.complete_loading();
    check(accepted_import.complete()&&accepted_import.mounted==1&&mod_lua.loading_complete(),
          "Selected matching mod Lua must override the original game library");
    std::filesystem::remove(root/"mod/FLuaVector.lua");
    ResourceMountTable omitted_mounts(resource_lookup(base));
    const auto missing_import=mount_selected_mod_lua(omitted_mounts,lua_catalog,lua_profile,mod_order);
    check(missing_import.declared==1&&missing_import.mounted==0&&
          missing_import.profile_omitted==0&&!missing_import.complete(),
          "A missing arbitrary mod Lua import remains a failure without profile attestation");
    const std::array<LuaImportOmission,1> omissions{{{lua_package.id,"FLuaVector.lua",
        lua_package.files[0].md5,lua_package.version}}};
    const auto profile_import=mount_selected_mod_lua(omitted_mounts,lua_catalog,lua_profile,mod_order,omissions);
    check(profile_import.complete()&&profile_import.declared==1&&profile_import.mounted==0&&
          profile_import.profile_omitted==1&&omitted_mounts.mounts().empty(),
          "An exact profile omission must not mount or require an intentionally absent Lua file");
    auto wrong_omission=omissions;wrong_omission[0].md5="wrong";
    rejects([&]{mount_selected_mod_lua(omitted_mounts,lua_catalog,lua_profile,mod_order,wrong_omission);});
    SavedContentProfile vanilla_profile;vanilla_profile.all_requirements_located=true;
    rejects([&]{mount_selected_mod_lua(omitted_mounts,lua_catalog,vanilla_profile,mod_order,omissions);});
    std::filesystem::remove(root/"Assets/Gameplay/Lua/FLuaVector.lua");
    std::filesystem::remove(root/"Assets/Gameplay/Lua");
    std::filesystem::remove(root/"Assets/Gameplay");
    std::filesystem::remove(root/"Assets");
    std::filesystem::remove(root/"base/Rules.lua");std::filesystem::remove(root/"mod/Rules.lua");
    std::filesystem::remove(root/"base");std::filesystem::remove(root/"mod");std::filesystem::remove(root);
}
