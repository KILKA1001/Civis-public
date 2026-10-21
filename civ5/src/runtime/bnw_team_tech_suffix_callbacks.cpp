#include "civ5/runtime/bnw_team_tech_suffix_callbacks.hpp"
#include <stdexcept>

namespace civ5::runtime {

BnwTeamTechSuffixCallbackResult quote_bnw_team_tech_suffix_callbacks(
    LiveSessionState &state,const BnwTeamTechLuaAttempt &lua,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const SessionCheckpointIdentity &identity,
    BnwTeamTechSuffixCallbackOwner *owner,
    std::int64_t team,std::int64_t technology) {
    BnwTeamTechSuffixCallbackResult result;
    const auto plan=plan_bnw_team_tech_suffix(
        state,lua,technologies,database,types,team,technology);
    if(plan.first_pending!=BnwTeamTechSuffixStep::reward_popup_option ||
       plan.proven_no_effect.size()!=4 ||
       plan.ui_intents.size()!=1 ||
       plan.ui_intents[0].kind!=
           BnwTeamTechSuffixUiKind::query_no_reward_popups ||
       !plan.later_engine_intent ||
       !plan.later_engine_intent->gameplay_tech_acquired) {
        result.blocker=plan.blocker.empty() ?
            "BNW selected tech suffix UI/engine branch is not proven" :
            plan.blocker;
        return result;
    }
    if(state.setup.extensions.find<BnwTeamTechSuffixCallbackMarker>()) {
        result.blocker="BNW tech suffix callbacks already attempted";
        return result;
    }
    if(!owner || owner->getter_world!=&state ||
       owner->identity.payload_sha256!=identity.payload_sha256 ||
       owner->identity.rules_sha256!=identity.rules_sha256 ||
       identity.payload_sha256.empty() || identity.rules_sha256.empty() ||
       !owner->selected_ui_option_owner ||
       !owner->selected_engine_owner ||
       !owner->external_writes_escrowed ||
       !owner->query_no_reward_popups ||
       !owner->gameplay_tech_acquired) {
        result.blocker="BNW selected UI option and GameplayTechAcquired owners are unavailable";
        return result;
    }
    BnwTeamTechSuffixCallbackMarker initial_marker;
    initial_marker.team=team;
    initial_marker.technology=technology;
    auto &marker=state.setup.extensions.emplace<BnwTeamTechSuffixCallbackMarker>(
        initial_marker);
    try {
        marker.ui_query_attempted=true;
        const auto option=owner->query_no_reward_popups(state);
        if(!option)
            throw std::runtime_error(
                "DLLUI::IsOptionNoRewardPopups did not return a confirmed value");
        marker.no_reward_popups=*option;
        marker.ui_query_confirmed=true;
        result.no_reward_popups=*option;
        result.ui_query_confirmed=true;
        // For team 63 in the proven selected branch, AddNotification scans
        // zero major-team recipients. The SP AddPopup condition is false.
        // Network mode cannot change this zero-effect result.
        marker.engine_attempted=true;
        if(!owner->gameplay_tech_acquired(team,technology,state))
            throw std::runtime_error(
                "gDLL::GameplayTechAcquired completion is unconfirmed");
        marker.engine_confirmed=true;
        result.engine_confirmed=true;
        result.status=BnwTeamTechSuffixCallbackStatus::quoted;
    } catch(const std::exception &error) {
        marker.uncertain=true;
        result.status=BnwTeamTechSuffixCallbackStatus::uncertain;
        result.blocker=error.what();
    } catch(...) {
        marker.uncertain=true;
        result.status=BnwTeamTechSuffixCallbackStatus::uncertain;
        result.blocker="BNW tech suffix callback failed with unknown external effect";
    }
    return result;
}
} // namespace civ5::runtime
