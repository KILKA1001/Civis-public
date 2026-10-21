#pragma once
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/research_checkpoint.hpp"

namespace civ5::runtime {

// Proposed raw-world companion to ID8. Only CvTeamTechs::m_aiResearchProgress
// is supported; technology grant and its world effects remain unavailable.
inline constexpr std::uint32_t bnw_team_progress_checkpoint_section_id=31;

struct BnwTeamProgressCheckpointPair {
    saves::CivisSaveSection raw,owned;
};
struct BnwTeamProgressRestored {
    saves::SavedSessionSnapshot world;
    std::vector<TeamTechnologies> technologies;
    PlayerResearchContext player_research;
};

saves::CivisSaveSection make_bnw_team_progress_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const ResearchCheckpointIdentity &identity);
void restore_bnw_team_progress_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const ResearchCheckpointIdentity &identity);

// baseline_* are restored from the immutable source under the same active
// rules. Pair preparation/restore verifies raw and owned values in both
// directions; callers publish only the returned detached result.
BnwTeamProgressCheckpointPair make_bnw_team_progress_checkpoint_pair(
    const LiveSessionState &state,
    const saves::SavedSessionSnapshot &source,
    const ResearchContext &baseline_setup,
    std::span<const TeamTechnologies> baseline_technologies,
    const PlayerResearchContext &baseline_players,
    const content::TechnologyRules &rules,
    const content::InfoTypeCatalog &types,
    const ResearchCheckpointIdentity &identity);
BnwTeamProgressRestored restore_bnw_team_progress_checkpoint_pair(
    const BnwTeamProgressCheckpointPair &pair,
    const saves::SavedSessionSnapshot &source,
    const ResearchContext &setup,
    std::span<const TeamTechnologies> baseline_technologies,
    const PlayerResearchContext &baseline_players,
    const content::TechnologyRules &rules,
    const content::InfoTypeCatalog &types,
    const ResearchCheckpointIdentity &identity);

} // namespace civ5::runtime
