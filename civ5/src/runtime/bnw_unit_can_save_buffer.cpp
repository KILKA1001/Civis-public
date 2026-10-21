#include "civ5/runtime/bnw_unit_can_save_buffer.hpp"
#include <exception>
#include <stdexcept>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
void same_entry(const BnwUnitKillEntry &a,const BnwUnitKillEntry &b) {
    const auto &r=a.removal,&s=b.removal;
    const auto &x=a.lua_arguments,&y=b.lua_arguments;
    require(r.player_index==s.player_index && r.unit_index==s.unit_index &&
            r.plot_index==s.plot_index && r.area_index==s.area_index &&
            r.owner==s.owner && r.id==s.id && r.area_id==s.area_id &&
            x.owner==y.owner && x.unit_id==y.unit_id &&
            x.unit_type==y.unit_type && x.x==y.x && x.y==y.y &&
            x.delayed==y.delayed && x.killer==y.killer &&
            a.army_id==b.army_id && a.requires_script_system_query &&
            a.requires_can_save_unit && a.requires_selected_ui_query &&
            a.requires_army_service==b.requires_army_service &&
            a.blocker==BnwUnitKillEntryBlocker::
                script_system_and_can_save_unit_unresolved &&
            !a.can_advance,
            "BNW CanSaveUnit entry is stale or bypassed");
}
BnwCanSaveResolution blocked(std::string reason) {
    BnwCanSaveResolution result;
    result.reason=std::move(reason);return result;
}
} // namespace

BnwCanSaveResolution stage_bnw_can_save_unit(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitKillEntry &entry,
    const BnwCanSaveStage &stage) {
    try {
        same_entry(entry,plan_bnw_unit_kill_entry(source,entry.removal,
            entry.lua_arguments.killer));
        if(!stage)
            return blocked("BNW CanSaveUnit detached Lua stage unavailable");
        std::vector<BnwCanSaveEffectIntent> effects;
        const auto evidence=stage(entry.lua_arguments,effects);
        if(evidence.script_system==BnwCanSaveScriptSystem::unknown)
            return blocked("BNW CanSaveUnit script-system presence uncertain");
        if(evidence.script_system==BnwCanSaveScriptSystem::absent) {
            if(evidence.test_all_completed ||
               evidence.test_all.value.has_value() ||
               !evidence.test_all.errors.empty() || !effects.empty())
                return blocked("BNW absent script system reported Lua effects");
            return {BnwCanSaveOutcome::proceed_to_selected_ui,{}, {},false};
        }
        if(!evidence.selected_entrypoints_loaded ||
           !evidence.test_all_completed)
            return blocked("BNW CanSaveUnit Lua result/effects unconfirmed");
        if(!evidence.test_all.errors.empty())
            return blocked("BNW CanSaveUnit Lua callback failed: "+
                           evidence.test_all.errors.front());
        if(!evidence.all_effects_buffered)
            return blocked("BNW CanSaveUnit Lua result/effects unconfirmed");
        BnwCanSaveResolution result;
        result.outcome=evidence.test_all.value.value_or(false)?
            BnwCanSaveOutcome::cancel_kill:
            BnwCanSaveOutcome::proceed_to_selected_ui;
        result.pending_effects=std::move(effects);
        return result;
    } catch(const std::exception &error) {
        return blocked(std::string("BNW CanSaveUnit stage rejected: ")+
                       error.what());
    } catch(...) {
        return blocked("BNW CanSaveUnit stage threw unknown exception");
    }
}
} // namespace civ5::runtime
