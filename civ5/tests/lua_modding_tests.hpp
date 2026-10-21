#pragma once
void lua_modding_tests(){
    using namespace civ5::runtime;
    const auto root=std::filesystem::temp_directory_path()/("civis-lua-modding-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root/"Lua");
    {std::ofstream file(root/"Lua/Entry.lua");file<<"this_is_not_automatically_executed=true";}
    auto manifest=[&](std::string id,std::uint64_t version){
        PackageManifest p;p.kind=PackageKind::mod;p.path=root/(id+".modinfo");p.id=id;p.version=version;
        p.document=read_xml_metadata("<Mod><EntryPoints>"
            "<EntryPoint type='InGameUIAddin' file='Lua/Entry.lua'><Name> first </Name><Description>\n detail\n </Description></EntryPoint>"
            "<EntryPoint type='InGameUIAddin' file='Lua/Entry.lua'><Name>first</Name><Description>detail</Description></EntryPoint>"
            "<EntryPoint type='MapScript' file='Lua/Entry.lua'><Name>map</Name></EntryPoint></EntryPoints></Mod>");
        p.files={{"Lua/Entry.lua","AUTHORED-HASH",false},{"Lua/Entry.lua","AUTHORED-HASH",false}};return p;
    };
    Catalog catalog;catalog.packages={manifest("mod-a",5000000001ULL),manifest("mod-b",9223372036854775807ULL),manifest("not-selected",7)};
    SavedContentProfile profile;profile.all_requirements_located=true;profile.enabled_mods={0,1};
    const std::vector<ModRegistration> order{{1,5000000002LL},{2,8},{0,5000000001LL}};
    ModScriptRegistry registry(catalog,profile,order);
    auto provider=registry.provider();
    LuaScriptHost host({}, {},provider);
    host.execute("A",R"lua(
        assert(this_is_not_automatically_executed==nil)
        assert(select('#',Modding.GetActivatedModEntryPoints('InGameUIAddin'))==1)
        local it=Modding.GetActivatedModEntryPoints('InGameUIAddin')
        local a,b,c,d=it(),it(),it(),it()
        assert(a.ModID=='mod-b' and b.ModID=='mod-b' and c.ModID=='mod-a' and d.ModID=='mod-a')
        assert(a~=b and a.Name=='first' and b.Name=='first' and a.Description=='detail')
        assert(a.Version=='9223372036854775807' and c.Version=='5000000001')
        assert(a.Type==1 and a.RowID==5000000002 and a.Installed==1 and a.Enabled==0 and a.Activated==1)
        assert(type(a.Installed)=='number' and select('#',it())==1)
        a.Name='local edit'; local restarted=it(); assert(restarted.Name=='first') -- reset, fresh row
        assert(getmetatable(a)~=getmetatable(restarted))
        assert(not pcall(function() a.NewField=3 end))
        assert(not pcall(function() a.LastRetrieved='now' end)) -- nil column still invokes __newindex
        local messages={}; print=function(s) messages[#messages+1]=s end
        assert(a.LastRetrieved==nil and #messages==0)
        assert(a.DoesNotExist==nil and #messages==1 and string.find(messages[1],'Cannot find key %- DoesNotExist'))
        local count=0; for row in Modding.GetActivatedModEntryPoints('MapScript') do count=count+1 end
        assert(count==2 and Modding.GetActivatedModEntryPoints('ingameuiaddin')()==nil)
        assert(Modding.GetActivatedModEntryPoints('InGameUIAddin\0ignored')().Name=='first')
        local file=Modding.GetEvaluatedFilePath(c.ModID,c.Version,c.File)
        assert(file.ModID=='mod-a' and file.Version=='5000000001' and file.Path=='Lua/Entry.lua')
        assert(file.md5=='AUTHORED-HASH' and string.find(file.EvaluatedPath,'Lua/Entry.lua',1,true))
        file.extra=true -- file result is a plain mutable table
        assert(Modding.GetEvaluatedFilePath(c.ModID,5000000001,c.File).Version==5000000001)
        local nulfile=Modding.GetEvaluatedFilePath(c.ModID..'\0ignored',c.Version,c.File..'\0ignored')
        assert(nulfile and nulfile.ModID==c.ModID..'\0ignored' and nulfile.Path==c.File..'\0ignored')
        assert(Modding.GetEvaluatedFilePath(a.ModID,a.Version,a.File).Version=='9223372036854775807')
        assert(select('#',Modding.GetEvaluatedFilePath('mod-a',1,c.File))==0)
        assert(select('#',Modding.GetEvaluatedFilePath('MOD-A',c.Version,c.File))==0)
        assert(select('#',Modding.GetEvaluatedFilePath(c.ModID,c.Version,'lua/Entry.lua'))==0)
        assert(select('#',Modding.GetEvaluatedFilePath(c.ModID,c.Version,'Lua\\Entry.lua'))==0)
        assert(Modding.GetEvaluatedFilePath('not-selected',7,c.File)~=nil) -- lookup alone does not activate it
        assert(not pcall(Modding.GetEvaluatedFilePath,c.ModID,math.huge,c.File))
        assert(not pcall(Modding.GetEvaluatedFilePath,c.ModID,9007199254740994,c.File))
        assert(not pcall(Modding.GetEvaluatedFilePath,c.ModID,{},c.File))
        assert(not pcall(Modding.GetActivatedModEntryPoints,{}))
    )lua","=registry.lua");
    host.execute("B","assert(Modding.GetActivatedModEntryPoints('InGameUIAddin')().Name=='first')","=isolated.lua");
    host.complete_loading();check(host.loading_complete(),"Native Modding queries must preserve SQL shape, exact versions, iteration and row/file behavior");
    check(registry.entry_points("InGameUIAddin").size()==4,"Duplicate authored entrypoints must remain; inactive mods must not run");
    const auto declared_plan=registry.planned_entry_points("InGameUIAddin");
    check(declared_plan.size()==4&&declared_plan[0].mod_id=="mod-b"&&declared_plan[2].mod_id=="mod-a"&&
          registry.planned_entry_points("MapScript").size()==2,
          "Lua entrypoint plan must preserve selected registration and manifest order");
    LuaScriptHost mismatched;
    rejects([&]{execute_mod_lua_entrypoint(declared_plan[0],mismatched,"mod-b-first");});
    auto pinned_catalog=catalog;
    for(auto &file:pinned_catalog.packages[1].files)file.md5=std::string(32,'0');
    const std::vector<VerifiedManifestHash> pinned_lua_hashes{{
        1,"Lua/Entry.lua",std::string(32,'0'),file_md5(root/"Lua/Entry.lua")}};
    ModScriptRegistry pinned_registry(pinned_catalog,profile,order,{},pinned_lua_hashes);
    const auto pinned_plan=pinned_registry.planned_entry_points("InGameUIAddin");
    LuaScriptHost pinned_host;
    check(!execute_mod_lua_entrypoint(pinned_plan[0],pinned_host,"pinned")&&
          !pinned_plan[0].use_installed_bytes,
          "Verified archive Lua entrypoint must run with its exact actual MD5");
    auto wrong_pinned=pinned_plan[0];wrong_pinned.verified_actual_md5=std::string(32,'f');
    rejects([&]{execute_mod_lua_entrypoint(wrong_pinned,pinned_host,"wrong-pin");});
    const std::array<std::size_t,1> local_package{1};
    ModScriptRegistry installed_registry(catalog,profile,order,local_package);
    const auto local_plan=installed_registry.planned_entry_points("InGameUIAddin");
    LuaScriptHost installed_host;
    check(!execute_mod_lua_entrypoint(local_plan[0],installed_host,"installed")&&
          local_plan[0].use_installed_bytes&&!local_plan[2].use_installed_bytes,
          "Only explicitly selected installed Lua entrypoints may bypass stale manifest MD5");
    installed_host.execute("installed","assert(this_is_not_automatically_executed)","=installed-check.lua");
    rejects([&]{execute_mod_lua_entrypoint(local_plan[2],installed_host,"other-package");});
    auto xml_entry=declared_plan[0];xml_entry.file="UI/Thing.xml";
    rejects([&]{execute_mod_lua_entrypoint(xml_entry,mismatched,"ui");});
    auto undeclared=declared_plan[0];undeclared.declared=false;
    rejects([&]{execute_mod_lua_entrypoint(undeclared,mismatched,"absent");});
    {std::ofstream file(root/"Lua/Entry.lua");
     file<<"this_is_not_automatically_executed=true; GameEvents.ModEntrypoint.Add(function(v) return v==42 end)";}
    auto executable=catalog;
    const auto actual_md5=file_md5(root/"Lua/Entry.lua");
    for(auto &package:executable.packages)for(auto &file:package.files)file.md5=actual_md5;
    ModScriptRegistry executable_registry(executable,profile,order);
    const auto planned=executable_registry.planned_entry_points("InGameUIAddin");
    LuaScriptHost loaded;
    execute_mod_lua_entrypoint(planned[0],loaded,"mod-b-first");
    execute_mod_lua_entrypoint(planned[2],loaded,"mod-a-first");
    loaded.execute("mod-b-first","assert(this_is_not_automatically_executed)","=loaded-b.lua");
    loaded.execute("mod-a-first","assert(this_is_not_automatically_executed)","=loaded-a.lua");
    loaded.complete_loading();
    const ScriptValue accepted[]={std::int64_t(42)},rejected[]={std::int64_t(1)};
    check(loaded.loading_complete()&&loaded.test_all("ModEntrypoint",accepted).value==true&&
          loaded.test_all("ModEntrypoint",rejected).value==false,
          "Selected manifest Lua must execute in independent contexts and register live game hooks");
    auto missing=profile;missing.all_requirements_located=false;
    rejects([&]{ModScriptRegistry r(catalog,missing,order);});
    rejects([&]{ModScriptRegistry r(catalog,profile,std::span(order).subspan(1));});
    auto duplicate=order;duplicate.push_back(order[0]);rejects([&]{ModScriptRegistry r(catalog,profile,duplicate);});
    auto duplicate_id=order;duplicate_id[1].row_id=duplicate_id[0].row_id;
    rejects([&]{ModScriptRegistry r(catalog,profile,duplicate_id);});
    auto bad_files=catalog;bad_files.packages[0].files.back().md5="CONFLICT";
    rejects([&]{ModScriptRegistry r(bad_files,profile,order);});
    auto outside=catalog;outside.packages[0].files[0].path="../escape.lua";
    rejects([&]{ModScriptRegistry r(outside,profile,order);});
    auto absent=catalog;absent.packages[0].files[0].path="absent.lua";
    ModScriptRegistry absent_registry(absent,profile,order);
    check(absent_registry.evaluated_file("mod-a",5000000001LL,"absent.lua").has_value(),"Registry lookup must preserve an absent-file declaration; source loading checks existence separately");
    auto too_wide=catalog;too_wide.packages[0].version=UINT64_MAX;
    rejects([&]{ModScriptRegistry r(too_wide,profile,order);});
    SavedContentProfile vanilla;vanilla.all_requirements_located=true;
    ModScriptRegistry vanilla_registry(catalog,vanilla,order);
    LuaScriptHost vanilla_host({}, {},vanilla_registry.provider());
    vanilla_host.execute("vanilla","assert(Modding.GetActivatedModEntryPoints('InGameUIAddin')()==nil)","=vanilla.lua");
    vanilla_host.complete_loading();check(vanilla_host.loading_complete(),"An empty selection must not activate installed mods");
    rejects([&]{LuaScriptHost partial({}, {},{provider.entry_points,{}});});
    LuaScriptHost bad_provider({}, {},{[](std::string_view)->std::vector<ScriptRow>{throw std::runtime_error("query failure");},provider.evaluated_file});
    bad_provider.execute("bad","assert(not pcall(Modding.GetActivatedModEntryPoints,'InGameUIAddin'))","=caught.lua");
    rejects([&]{bad_provider.complete_loading();});
    LuaScriptHost bad_file({}, {},{provider.entry_points,[](std::string_view,std::int64_t,std::string_view)->std::optional<ScriptEvaluatedFile>{throw 7;}});
    bad_file.execute("bad","assert(not pcall(Modding.GetEvaluatedFilePath,'mod-a',5000000001,'Lua/Entry.lua'))","=caught-file.lua");
    rejects([&]{bad_file.complete_loading();});
    LuaScriptHost inexact({}, {},{[](std::string_view){return std::vector<ScriptRow>{{{"RowID",INT64_MAX}}};},provider.evaluated_file});
    rejects([&]{inexact.execute("bad","Modding.GetActivatedModEntryPoints('any')","=inexact.lua");});
    rejects([&]{inexact.complete_loading();});
    std::filesystem::remove(root/"Lua/Entry.lua");std::filesystem::remove(root/"Lua");std::filesystem::remove(root);
}
