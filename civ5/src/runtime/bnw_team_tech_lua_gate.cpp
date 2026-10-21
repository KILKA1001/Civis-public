#include "civ5/runtime/bnw_team_tech_lua_gate.hpp"
#include <array>

namespace civ5::runtime {

BnwTeamTechLuaAttempt quote_bnw_team_tech_researched(
    LiveSessionState &state,const SavedContentProfile &selected,
    const SessionCheckpointIdentity &identity,BnwTeamTechLuaEpoch *epoch,
    std::int64_t technology) {
    BnwTeamTechLuaAttempt result;
    const auto *prior=state.setup.extensions.find<BnwBarbarianPostTraitsTransient>();
    if(!prior || prior->team!=63 || prior->player!=63 ||
       prior->free_unit_prereq_tech!=technology ||
       !prior->free_unit_query_done || !prior->map_loops_done ||
       technology!=0 || state.technologies.size()<=63 ||
       !state.technologies[63].has_tech(technology)) {
        result.reason="BNW TeamTechResearched requires completed Agriculture post-trait effects";
        return result;
    }
    if(state.setup.extensions.find<BnwTeamTechLuaAttemptMarker>()) {
        result.reason="BNW TeamTechResearched already attempted; reconcile before retry";
        return result;
    }
    if(!selected.can_activate || !selected.all_requirements_located ||
       !selected.enabled_mods.empty() || identity.payload_sha256.empty() ||
       identity.rules_sha256.empty()) {
        result.reason="BNW TeamTechResearched selected stock profile or identity unavailable";
        return result;
    }
    if(!epoch) {
        result.reason="BNW TeamTechResearched has no selected gameplay Lua context epoch";
        return result;
    }
    if(!epoch->world || !epoch->host ||
       epoch->source_world!=&state || epoch->getter_world!=epoch->world.get() ||
       epoch->identity.payload_sha256!=identity.payload_sha256 ||
       epoch->identity.rules_sha256!=identity.rules_sha256 ||
       epoch->selected_dlc!=selected.enabled_dlc ||
       epoch->selected_mods!=selected.enabled_mods ||
       !epoch->all_selected_contexts_loaded ||
       !epoch->all_listeners_registered ||
       !epoch->all_external_writes_escrowed ||
       !epoch->host->loading_complete() ||
       !epoch->host->source_errors().empty() ||
       !epoch->world->setup.extensions.find<BnwBarbarianPostTraitsTransient>() ||
       epoch->world->technologies.size()<=63 ||
       !epoch->world->technologies[63].has_tech(technology)) {
        result.reason="BNW TeamTechResearched detached selected Lua epoch is unverified";
        return result;
    }

    // Mark before invoking callbacks: the VM or an external bridge can have
    // changed state even if the next callback throws. No retry is safe here.
    auto &marker=state.setup.extensions.emplace<BnwTeamTechLuaAttemptMarker>(
        BnwTeamTechLuaAttemptMarker{63,technology,true,true,false});
    try {
        const std::array<ScriptValue,3> args{
            std::int64_t{63},technology,std::int64_t{1}};
        const auto hook=epoch->host->call_hook("TeamTechResearched",args);
        if(!hook.errors.empty()) {
            epoch->world->setup.extensions.emplace<BnwTeamTechLuaAttemptMarker>(marker);
            result.status=BnwTeamTechLuaStatus::uncertain;
            result.reason="BNW TeamTechResearched callback failed: " +
                          hook.errors.front();
            return result;
        }
        marker.uncertain=false;
        marker.quoted=true;
        epoch->world->setup.extensions.emplace<BnwTeamTechLuaAttemptMarker>(marker);
        result.status=BnwTeamTechLuaStatus::quoted;
        result.hook_value=hook.value;
        result.reason="BNW TeamTechResearched quoted; Lua/world publication and later setHasTech effects remain";
    } catch(const std::exception &error) {
        epoch->world->setup.extensions.emplace<BnwTeamTechLuaAttemptMarker>(marker);
        result.status=BnwTeamTechLuaStatus::uncertain;
        result.reason=error.what();
    }
    return result;
}

} // namespace civ5::runtime
