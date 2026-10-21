#pragma once
#include "civ5/runtime/lua_script_host.hpp"

namespace civ5::runtime {
// Civ V SDK CvEnums.h, CvMinorCivAI.h and CvLuaEnums.cpp register these values
// in the base and Expansion GameCores. The caller selects the GameCore; VP and other DLL
// profiles can supply different tables through ScriptEnumProvider.
inline ScriptEnumProvider lua_civ5_gamecore_enums(bool expansion){
    ScriptEnumTable city{"CityUpdateTypes",{
        {"NO_CITY_UPDATE_TYPE",-1},
        {"CITY_UPDATE_TYPE_BANNER",0},
        {"CITY_UPDATE_TYPE_SPECIALISTS",1},
        {"CITY_UPDATE_TYPE_PRODUCTION",2},
        {"CITY_UPDATE_TYPE_ENEMY_IN_RANGE",3}
    }};
    if(expansion)city.members.emplace_back("CITY_UPDATE_TYPE_GARRISON",4);
    city.members.emplace_back("NUM_CITY_UPDATE_TYPES",expansion?5:4);
    ScriptEnumTable personality{"MinorCivPersonalityTypes",{
        {"NO_MINOR_CIV_PERSONALITY_TYPE",-1},
        {"MINOR_CIV_PERSONALITY_FRIENDLY",0},
        {"MINOR_CIV_PERSONALITY_NEUTRAL",1},
        {"MINOR_CIV_PERSONALITY_HOSTILE",2},
        {"MINOR_CIV_PERSONALITY_IRRATIONAL",3},
        {"NUM_MINOR_CIV_PERSONALITY_TYPES",4}
    }};
    return {{std::move(city),std::move(personality)}};
}
} // namespace civ5::runtime
