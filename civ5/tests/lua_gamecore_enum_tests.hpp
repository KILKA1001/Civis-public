#pragma once
void lua_gamecore_enum_tests(){
    using namespace civ5::runtime;
    const auto host_with=[](ScriptEnumProvider enums){
        return std::make_unique<LuaScriptHost>(ScriptBudget{},ScriptSourceProvider{},ScriptModdingProvider{},
            ScriptGameplayProvider{},ScriptSaveDataProvider{},ScriptPregameProvider{},ScriptMapProvider{},
            ScriptUiEventProvider{},ScriptContentProvider{},ScriptUserDataProvider{},std::move(enums));
    };
    auto base=host_with(lua_civ5_gamecore_enums(false));
    base->execute("first",R"lua(
        assert(CityUpdateTypes.NO_CITY_UPDATE_TYPE==-1)
        assert(CityUpdateTypes.CITY_UPDATE_TYPE_BANNER==0)
        assert(CityUpdateTypes.CITY_UPDATE_TYPE_ENEMY_IN_RANGE==3)
        assert(CityUpdateTypes.CITY_UPDATE_TYPE_GARRISON==nil)
        assert(CityUpdateTypes.NUM_CITY_UPDATE_TYPES==4)
        assert(MinorCivPersonalityTypes.NO_MINOR_CIV_PERSONALITY_TYPE==-1)
        assert(MinorCivPersonalityTypes.MINOR_CIV_PERSONALITY_FRIENDLY==0)
        assert(MinorCivPersonalityTypes.MINOR_CIV_PERSONALITY_IRRATIONAL==3)
        assert(MinorCivPersonalityTypes.NUM_MINOR_CIV_PERSONALITY_TYPES==4)
        CityUpdateTypes.CITY_UPDATE_TYPE_BANNER=99
    )lua","=base-gamecore-enums");
    base->execute("second","assert(CityUpdateTypes.CITY_UPDATE_TYPE_BANNER==0)",
        "=independent-gamecore-context");
    base->complete_loading();check(base->loading_complete(),"Base GameCore enum tables are isolated by Lua context");
    auto expansion=host_with(lua_civ5_gamecore_enums(true));
    expansion->execute("bnw",R"lua(
        assert(CityUpdateTypes.CITY_UPDATE_TYPE_GARRISON==4)
        assert(CityUpdateTypes.NUM_CITY_UPDATE_TYPES==5)
    )lua","=expansion-gamecore-enums");
    expansion->complete_loading();check(expansion->loading_complete(),"Expansion GameCore exposes SDK garrison enum");
    auto absent=host_with({});
    absent->execute("none","assert(CityUpdateTypes==nil)","=no-selected-gamecore-enums");
    absent->complete_loading();check(absent->loading_complete(),"Generic Lua host does not select a GameCore");
    rejects([&]{(void)host_with({{{"CityUpdateTypes",{{"A",1},{"A",2}}}}});});
    rejects([&]{(void)host_with({{{"CityUpdateTypes",{{"A",1}}},{"CityUpdateTypes",{{"B",2}}}}});});
    rejects([&]{(void)host_with({{{"Wide",{{"TooWide",9007199254740993LL}}}}});});
}
