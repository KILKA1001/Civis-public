#pragma once

#include "civ5/runtime/bnw_team_turn_checkpoint.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/research_checkpoint.hpp"
#include <span>

namespace civ5::runtime {

// ID40's raw CvTeam values and ID8's owned CvTeamTechs values form one
// transaction. This pair owns only no_trade changes in ID8. A separate ID31
// research-progress owner cannot coexist until a joint three-section
// coordinator applies and verifies both raw changes against one owned ID8.
struct BnwTeamTurnCheckpointPair {
    saves::CivisSaveSection raw, owned;
};
struct BnwTeamTurnRestored {
    saves::SavedSessionSnapshot world;
    std::vector<TeamTechnologies> technologies;
    PlayerResearchContext player_research;
};

BnwTeamTurnCheckpointPair make_bnw_team_turn_checkpoint_pair(
    const LiveSessionState &state,
    const saves::SavedSessionSnapshot &source,
    std::span<const TeamTechnologies> baseline_technologies,
    const PlayerResearchContext &baseline_players,
    const content::TechnologyRules &rules,
    const content::InfoTypeCatalog &types,
    const SessionCheckpointIdentity &identity);

// Returns a detached result; the caller publishes it only after both sections
// and all raw/owned comparisons succeed.
BnwTeamTurnRestored restore_bnw_team_turn_checkpoint_pair(
    const BnwTeamTurnCheckpointPair &pair,
    const saves::SavedSessionSnapshot &source,
    const ResearchContext &setup,
    std::span<const TeamTechnologies> baseline_technologies,
    const PlayerResearchContext &baseline_players,
    const content::TechnologyRules &rules,
    const content::InfoTypeCatalog &types,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
