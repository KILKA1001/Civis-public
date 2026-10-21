#include "civ5/runtime/bnw_first_contact_approach_preflight.hpp"
#include <climits>
#include <stdexcept>
#include <variant>

namespace civ5::runtime {

BnwFirstContactApproachPreflight preflight_bnw_first_contact_approach(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwFirstContactApproachPreflight result;
    const auto relationship=preflight_bnw_first_contact_relationship(
        source,state,request,database,types,defines,identity);
    result.source_verified=relationship.source_verified;
    result.recipient=relationship.recipient;
    result.target=relationship.target;
    result.relationship_gate=relationship.gate;
    if(relationship.target_minor || !relationship.major_opinion_call_completed ||
       relationship.gate!=
           BnwFirstContactRelationshipGate::major_approach_unimplemented) {
        result.reason=relationship.reason.empty()
            ? "BNW major approach requires completed major opinion call"
            : relationship.reason;
        return result;
    }
    result.major_branch_verified=true;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    if(!state.world.pregame || !state.world.teams ||
       !state.world.all_players ||
       state.world.pregame->layout!=bnw ||
       state.world.teams->layout!=bnw ||
       state.world.all_players->layout!=bnw ||
       state.world.all_players->players.size()!=64 ||
       result.target<0 || result.target>=22 ||
       result.recipient<0 || result.recipient>=63 ||
       !state.setup.players.contains(result.recipient) ||
       !state.setup.players.contains(result.target)) {
        result.gate=BnwFirstContactApproachGate::team_or_diplomacy_roster_inconsistent;
        result.reason="BNW major approach player/team profile is unavailable";
        return result;
    }
    const auto &mapping=state.world.pregame->archive.integer_arrays;
    const auto teams=mapping.find("team_types");
    const auto &players=state.world.all_players->players;
    if(teams==mapping.end() || teams->second.size()<64 ||
       players.at(static_cast<std::size_t>(result.target)).saved_player_id!=
           result.target ||
       players.at(static_cast<std::size_t>(result.recipient)).saved_player_id!=
           result.recipient ||
       players.at(static_cast<std::size_t>(result.target)).diplomacy.layout!=bnw ||
       players.at(static_cast<std::size_t>(result.recipient)).diplomacy.layout!=bnw) {
        result.gate=BnwFirstContactApproachGate::team_or_diplomacy_roster_inconsistent;
        result.reason="BNW major approach team mapping or diplomacy snapshot differs";
        return result;
    }
    result.target_team=teams->second[static_cast<std::size_t>(result.target)];
    result.recipient_team=teams->second[static_cast<std::size_t>(result.recipient)];
    if(result.target_team<0 || result.target_team>=63 ||
       result.recipient_team<0 || result.recipient_team>=63 ||
       state.setup.players.at(result.target).team!=result.target_team ||
       state.setup.players.at(result.recipient).team!=result.recipient_team) {
        result.gate=BnwFirstContactApproachGate::team_or_diplomacy_roster_inconsistent;
        result.reason="BNW major approach team mirrors disagree";
        return result;
    }
    const auto *neutral=defines.find("APPROACH_NEUTRAL_DEFAULT");
    if(defines.profile()!=content::GlobalDefineProfile::bnw || !neutral ||
       neutral->lookup.sqlite_code!=0 ||
       !std::holds_alternative<std::int64_t>(neutral->lookup.value) ||
       std::get<std::int64_t>(neutral->lookup.value)<INT32_MIN ||
       std::get<std::int64_t>(neutral->lookup.value)>INT32_MAX) {
        result.gate=BnwFirstContactApproachGate::neutral_rule_unavailable;
        result.reason="BNW major approach neutral default is unavailable from active rules";
        return result;
    }
    result.neutral_rule_found=neutral->lookup.found;
    result.neutral_default=std::get<std::int64_t>(neutral->lookup.value);
    result.gate=BnwFirstContactApproachGate::scratchpad_unrestored;
    result.reason="BNW GetBestApproach reads transient m_paeApproachScratchPad before other weights, RNG and logging";
    return result;
}

} // namespace civ5::runtime
