#pragma once
#include "civ5/runtime/bnw_unit_roster_checkpoint.hpp"
#include "civ5/runtime/bnw_unit_kill_lua.hpp"

namespace civ5::runtime {

enum class BnwUnitKillEntryBlocker {
    script_system_and_can_save_unit_unresolved
};

struct BnwUnitKillEntry {
    BnwUnitRemoval removal;
    BnwUnitKillLuaArguments lua_arguments;
    std::int64_t army_id=-1;
    // Expansion2 kill's first conditional call is CanSaveUnit(owner, ID).
    // It is skipped only when gDLL->GetScriptSystem() is null.
    // The selected UI state is not in the save; a real live query is required
    // before VisuallyDeselectUnit. Army membership needs a live CvArmyAI.
    bool requires_script_system_query=true;
    bool requires_can_save_unit=true;
    bool requires_selected_ui_query=true;
    bool requires_army_service=false;
    BnwUnitKillEntryBlocker blocker=
        BnwUnitKillEntryBlocker::script_system_and_can_save_unit_unresolved;
    bool can_advance=false;
};

// Pure preflight for CvUnit.cpp:1166-1200. It does not invoke Lua: the current
// Lua bridge can cause unbuffered external effects, while CanSaveUnit may
// cancel the kill. UnitPrekill comes later, after army/cargo/diplomacy effects.
BnwUnitKillEntry plan_bnw_unit_kill_entry(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitRemoval &removal,
    std::int64_t killer);

} // namespace civ5::runtime
