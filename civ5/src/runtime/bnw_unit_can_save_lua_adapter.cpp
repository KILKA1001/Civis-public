#include "civ5/runtime/bnw_unit_can_save_lua_adapter.hpp"
#include <array>

namespace civ5::runtime {

BnwCanSaveUnitLuaAttempt stage_bnw_can_save_unit_lua_host(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitKillEntry &entry,
    BnwCanSaveUnitLuaEscrow *escrow) {
    BnwCanSaveUnitLuaAttempt attempt;
    if(!escrow) {
        attempt.resolution.reason=
            "BNW CanSaveUnit requires an exact detached Lua VM/effect escrow";
        return attempt;
    }
    std::unique_ptr<LuaScriptHost> detached;
    attempt.resolution=stage_bnw_can_save_unit(source,entry,
        [&](const BnwUnitKillLuaArguments &args,
            std::vector<BnwCanSaveEffectIntent> &effects) {
            BnwCanSaveStageEvidence evidence;
            evidence.script_system=escrow->script_system(source,entry);
            if(evidence.script_system!=BnwCanSaveScriptSystem::present)
                return evidence;
            detached=escrow->fork_exact_detached(source,entry);
            if(!detached || !detached->loading_complete())
                return evidence;
            evidence.selected_entrypoints_loaded=true;
            const std::array<ScriptValue,2> lua_args{
                args.owner,args.unit_id};
            evidence.test_all=detached->test_all("CanSaveUnit",lua_args);
            evidence.test_all_completed=true;
            if(!evidence.test_all.errors.empty()) return evidence;
            auto captured=escrow->collect_all_effects(*detached);
            if(!captured) return evidence;
            effects=std::move(*captured);
            evidence.all_effects_buffered=true;
            return evidence;
        });
    if(attempt.resolution.outcome!=BnwCanSaveOutcome::blocked)
        attempt.detached_host=std::move(detached);
    return attempt;
}

} // namespace civ5::runtime
