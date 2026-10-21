#include "civ5/runtime/bnw_first_contact_relationship_preflight.hpp"
#include <stdexcept>

namespace civ5::runtime {

BnwFirstContactRelationshipPreflight preflight_bnw_first_contact_relationship(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwFirstContactRelationshipPreflight result;
    const auto target=preflight_bnw_first_contact_target(
        source,state,request,database,types,defines,identity);
    result.source_verified=target.source_verified;
    result.recipient=target.recipient;
    result.target=target.target;
    result.elapsed_turns=target.elapsed_turns;
    result.target_city_count=target.target_city_count;
    result.target_gate=target.gate;
    if(!target.target_value_call_completed ||
       target.gate!=
           BnwFirstContactTargetGate::next_relationship_branch_unimplemented) {
        result.reason=target.reason.empty()
            ? "BNW first-contact relationship requires completed target value"
            : target.reason;
        return result;
    }
    try {
        constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
        if(!state.world.pregame || state.world.pregame->layout!=bnw ||
           !state.world.teams ||
           state.world.teams->dimensions.major!=22 ||
           result.target<0 || result.target>=63 ||
           state.cities.profile!=content::TechnologyProfile::bnw ||
           state.player_research.profile!=content::TechnologyProfile::bnw ||
           !state.cities.players.contains(result.target) ||
           !state.player_research.players.contains(result.target))
            throw std::runtime_error("BNW first-contact player class roster is unavailable");
        const auto &flags=state.world.pregame->archive.flag_arrays;
        const auto minors=flags.find("minor_nation_civs");
        if(minors==flags.end() || minors->second.size()<64)
            throw std::runtime_error("BNW first-contact pregame minor flags are unavailable");
        result.target_minor=minors->second[static_cast<std::size_t>(result.target)];
        if(state.cities.players.at(result.target).minor!=result.target_minor ||
           state.player_research.players.at(result.target).pregame_minor!=
               result.target_minor ||
           state.player_research.major_players.contains(result.target)!=
               (result.target<22))
            throw std::runtime_error("BNW first-contact player class mirrors disagree");
        if(result.target_minor) {
            result.gate=BnwFirstContactRelationshipGate::minor_approach_unimplemented;
            result.reason="BNW first-contact minor branch writes MINOR_CIV_APPROACH_IGNORE";
            return result;
        }
        if(result.target>=22)
            throw std::runtime_error("BNW major opinion target exceeds MAX_MAJOR_CIVS");
        if(result.target_city_count!=0) {
            result.gate=BnwFirstContactRelationshipGate::major_opinion_unimplemented;
            result.reason="BNW major opinion requires complete diplomacy state";
            return result;
        }
        result.major_opinion_call_completed=true;
        result.gate=BnwFirstContactRelationshipGate::major_approach_unimplemented;
        result.reason="BNW major opinion skips cityless target; GetBestApproachTowardsMajorCiv is next";
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactRelationshipGate::player_class_inconsistent;
        result.reason=error.what();
    }
    return result;
}

} // namespace civ5::runtime
