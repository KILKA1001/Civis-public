#pragma once
#include "civ5/runtime/lua_script_host.hpp"

namespace civ5::runtime {

enum class BnwCanDeclareWarLuaDecision : std::uint8_t {allow,veto};
struct BnwCanDeclareWarLuaResult {
    BnwCanDeclareWarLuaDecision decision=BnwCanDeclareWarLuaDecision::allow;
    bool handled=false;
};

// Expansion2 CvTeam.cpp:970-1025. This bridge is called only after every
// native early-return check in canDeclareWar: CallTestAll("CanDeclareWar",
// own_team, target_team). A handled false vetoes; true or no listeners allows.
// Listener exceptions and an absent/incomplete host fail closed. Invoking
// selected Lua can mutate its VM or external providers, so this bridge must
// not be used in an uncommitted first-contact quote. The caller must attest
// selected entrypoint loading and provide a replay/buffering boundary.
BnwCanDeclareWarLuaResult call_bnw_can_declare_war_lua(
    LuaScriptHost *host,std::int64_t own_team,std::int64_t target_team);

} // namespace civ5::runtime
