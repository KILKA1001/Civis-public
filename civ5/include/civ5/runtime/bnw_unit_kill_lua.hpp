#pragma once

#include "civ5/runtime/lua_script_host.hpp"
#include <cstdint>

namespace civ5::runtime {

struct BnwUnitKillLuaArguments {
    std::int64_t owner=-1,unit_id=-1,unit_type=-1,x=-1,y=-1;
    bool delayed=false;
    std::int64_t killer=-1; // NO_PLAYER for CvUnit::scrap/kill(false).
};
enum class BnwUnitKillLuaDecision { proceed, saved_by_script };
struct BnwUnitKillLuaResult {
    BnwUnitKillLuaDecision decision=BnwUnitKillLuaDecision::proceed;
    bool can_save_handled=false,prekill_handled=false;
};

// Expansion2 CvUnit.cpp:1166 kill: CallTestAll("CanSaveUnit", owner, ID)
// precedes CallHook("UnitPrekill", owner, ID, type, x, y, bDelay, ePlayer).
// A handled true CanSaveUnit cancels this kill and suppresses UnitPrekill;
// false/no listeners proceeds. UnitPrekill's boolean return is ignored.
// This bridge does not mutate an owned world. Lua callbacks may themselves
// cause external effects, so it must not run inside an uncommitted transaction
// until the host has a replay/buffering contract. No DoGold/kill integration.
BnwUnitKillLuaResult call_bnw_unit_kill_lua(
    LuaScriptHost &host,const BnwUnitKillLuaArguments &arguments);

} // namespace civ5::runtime
