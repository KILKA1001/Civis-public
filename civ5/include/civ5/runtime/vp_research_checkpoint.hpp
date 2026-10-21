#pragma once
#include "civ5/runtime/research_checkpoint.hpp"

namespace civ5::runtime {
// The VP module alone owns the layout and validation of its checkpoint section.
// The common section is passed only to bind the two byte streams together.
saves::CivisSaveSection make_vp_research_checkpoint(
    std::span<const TeamTechnologies> teams, const PlayerResearchContext &players,
    const saves::CivisSaveSection &common, ResearchCheckpointBudget budget);
void restore_vp_research_checkpoint(
    const saves::CivisSaveSection &profile, const saves::CivisSaveSection &common,
    std::vector<TeamTechnologies> &teams, PlayerResearchContext &players,
    ResearchCheckpointBudget budget);
} // namespace civ5::runtime
