#include "civ5/runtime/bnw_can_declare_war_lua.hpp"
#include <array>

namespace civ5::runtime {

BnwCanDeclareWarLuaResult call_bnw_can_declare_war_lua(
    LuaScriptHost *host,std::int64_t own_team,std::int64_t target_team) {
    // Expansion2 CvTeam.cpp:1007-1025. The original Lua argument handle
    // pushes own team first, then target team, and CallTestAll only denies
    // when a listener handled the event with an aggregate false result.
    if(!host || !host->loading_complete())
        throw ScriptError("BNW CanDeclareWar Lua host has not loaded selected entrypoints");
    if(own_team<0 || own_team>=63 || target_team<0 || target_team>=63 ||
       own_team==target_team)
        throw ScriptError("BNW CanDeclareWar source team IDs are invalid");
    const std::array<ScriptValue,2> args{own_team,target_team};
    const auto test=host->test_all("CanDeclareWar",args);
    if(!test.errors.empty())
        throw ScriptError("BNW CanDeclareWar listener failed: "+test.errors.front());
    return {test.value && !*test.value
        ? BnwCanDeclareWarLuaDecision::veto
        : BnwCanDeclareWarLuaDecision::allow,
        test.value.has_value()};
}

} // namespace civ5::runtime
