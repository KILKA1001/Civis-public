#pragma once
#include "civ5/runtime/vp_gathering_intel_turn.hpp"
#include "civ5/runtime/vp_schmooze_turn.hpp"
#include "civ5/runtime/vp_travelling_turn.hpp"

namespace civ5::runtime {
struct VpEspionagePlayerTurnSegment {
    std::size_t processed_spies=0;
    std::optional<std::size_t> blocking_spy;
    std::int64_t blocking_state=-1;
    std::string unresolved;
    std::vector<VpGatheringIntelStep> gathering_steps;
    std::vector<VpSchmoozeStep> diplomat_steps;
    std::vector<VpTravellingStep> travelling_steps;
    bool complete=false;
};
// CvPlayerEspionage::DoTurn gameplay order: queued notifications first, then
// spies by index. Advances only branches whose world effects are resolved.
// A blocked return keeps earlier spy mutations, including safe actions already
// completed by the blocking spy, in the supplied live world;
// callers doing a tentative whole turn must use a disposable copy.
VpEspionagePlayerTurnSegment process_vp_espionage_player_turn_segment(
    ResearchContext &setup,CityYieldContext &cities,
    const PlayerResearchContext &diplomacy,std::span<const TeamTechnologies> technologies,
    const TreasuryContext &treasury,const TreasuryRules &treasury_rules,
    const content::RulesDatabase &database,const content::GlobalDefineCache &defines,
    const content::CustomModOptions &options,std::int64_t player);
} // namespace civ5::runtime
