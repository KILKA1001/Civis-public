#pragma once
#include "civ5/runtime/bnw_unit_kill_entry.hpp"
#include <functional>

namespace civ5::runtime {

enum class BnwCanSaveScriptSystem {unknown,absent,present};
enum class BnwCanSaveOutcome {blocked,cancel_kill,proceed_to_selected_ui};

struct BnwCanSaveEffectIntent {
    std::string channel,operation;
    std::vector<ScriptValue> arguments;
};
struct BnwCanSaveStageEvidence {
    BnwCanSaveScriptSystem script_system=BnwCanSaveScriptSystem::unknown;
    bool selected_entrypoints_loaded=false;
    bool test_all_completed=false;
    bool all_effects_buffered=false;
    ScriptTestResult test_all;
};
struct BnwCanSaveResolution {
    BnwCanSaveOutcome outcome=BnwCanSaveOutcome::blocked;
    std::string reason;
    // Kept for BOTH cancel and proceed: SDK Lua callbacks can have effects
    // even when CanSaveUnit saves the unit. Caller must publish this batch
    // atomically after all relevant state has been validated.
    std::vector<BnwCanSaveEffectIntent> pending_effects;
    bool can_commit_kill=false;
};
using BnwCanSaveStage=std::function<BnwCanSaveStageEvidence(
    const BnwUnitKillLuaArguments &,
    std::vector<BnwCanSaveEffectIntent> &)>;

// Expansion2 CvUnit::kill first runs CallTestAll("CanSaveUnit", owner, ID)
// only if gDLL->GetScriptSystem() is present. Stage must execute against an
// isolated owned world and buffer *all* Lua mutations, RNG, save-data and UI
// effects. Current LuaScriptHost lacks such an adapter, so production callers
// must return blocked rather than pass its live host here. This function never
// publishes intents or advances the gameplay kill.
BnwCanSaveResolution stage_bnw_can_save_unit(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitKillEntry &entry,
    const BnwCanSaveStage &stage);

} // namespace civ5::runtime
