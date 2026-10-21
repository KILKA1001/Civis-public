#pragma once

void lua_content_tests(){
    using namespace civ5;
    content::Catalog catalog;
    content::PackageManifest a{},b{},mod{};
    a.kind=content::PackageKind::dlc;a.id="aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa";a.name="Expansion A";
    b.kind=content::PackageKind::dlc;b.id="bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb";b.name="Expansion B";
    mod.kind=content::PackageKind::mod;mod.id="cccccccc-cccc-cccc-cccc-cccccccccccc";mod.name="A Mod";
    catalog.packages={a,b,mod};
    runtime::SavedContentProfile selected;selected.enabled_dlc={1};
    auto provider=runtime::lua_content_services(catalog,selected);
    check(provider.available&&provider.packages.size()==2&&provider.packages[1].gameplay_active,
          "Content inventory includes installed DLC but activates only the save-selected package");
    runtime::LuaScriptHost host({},{},{},{},{},{},{},{},provider);
    host.execute("first",R"lua(
        local ids=ContentManager.GetAllPackageIDs()
        assert(#ids==2 and ids[1]=='AAAAAAAA-AAAA-AAAA-AAAA-AAAAAAAAAAAA')
        assert(ids[2]=='BBBBBBBB-BBBB-BBBB-BBBB-BBBBBBBBBBBB')
        assert(type(ContentType.GAMEPLAY)=='number')
        assert(not ContentManager.IsActive(ids[1],ContentType.GAMEPLAY))
        assert(ContentManager.IsActive('bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb',ContentType.GAMEPLAY))
        assert(not ContentManager.IsActive(ids[2],ContentType.GAMEPLAY+1))
        assert(not ContentManager.IsActive('cccccccc-cccc-cccc-cccc-cccccccccccc',ContentType.GAMEPLAY))
        assert(ContentManager.GetPackageDescription(ids[2])=='Expansion B')
        assert(ContentManager.GetPackageDescription('unknown')==nil)
        ContentType.GAMEPLAY=300
    )lua","=selected-content");
    host.execute("second","assert(ContentType.GAMEPLAY~=300 and ContentManager.IsActive('BBBBBBBB-BBBB-BBBB-BBBB-BBBBBBBBBBBB',ContentType.GAMEPLAY))","=separate-content-context");
    host.complete_loading();check(host.loading_complete(),"ContentManager must read the selected DLC profile in isolated Lua contexts");
    runtime::SavedContentProfile vanilla;
    auto vanilla_provider=runtime::lua_content_services(catalog,vanilla);
    runtime::LuaScriptHost no_dlc({},{},{},{},{},{},{},{},vanilla_provider);
    no_dlc.execute("vanilla","assert(not ContentManager.IsActive('BBBBBBBB-BBBB-BBBB-BBBB-BBBBBBBBBBBB',ContentType.GAMEPLAY))","=vanilla-content");
    no_dlc.complete_loading();check(no_dlc.loading_complete(),"Installed DLC must not become active in a vanilla save");
    runtime::LuaScriptHost absent;
    absent.execute("absent","assert(ContentManager==nil and ContentType==nil)","=no-content-service");
    absent.complete_loading();check(absent.loading_complete(),"ContentManager is absent without a selected content service");
    auto duplicate=provider;duplicate.packages.push_back({"BBBBBBBB-BBBB-BBBB-BBBB-BBBBBBBBBBBB","Duplicate",false});
    rejects([&]{runtime::LuaScriptHost invalid({},{},{},{},{},{},{},{},duplicate);});
}
