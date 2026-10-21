#pragma once
#include "civ5/saves/team_state.hpp"
#include "civ5/runtime/profile_extensions.hpp"

namespace civ5::saves {
struct CityManagementContext {
    std::optional<std::uint64_t> bnw_projects;
    runtime::ProfileExtensions profile_extensions;
};
struct CityManagementBlock {
    const char *name;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::vector<NamedSaveInteger> fields;
    std::vector<PlotArrayObservation> arrays;
    std::vector<TeamHashedArray> hashes;
};
struct CityManagementObservation {
    GameStateLayout layout;
    CityManagementContext context;
    std::uint64_t start_offset = 0, end_offset = 0, city_plot_slots = 0;
    std::vector<CityManagementBlock> blocks;
};
// CityStrategyAI, its four production AIs, and CityCitizens; stops before religion.
// Source plot spans (91 VP / 37 BNW) are wire formats, not host city limits.
// Keeps unknown hashes, duplicate weights, source order and saved assignments.
// Does not evaluate AI, redistribute citizens, resolve plots, or rebuild caches.
CityManagementObservation observe_city_management(runtime::BinaryReader &input, GameStateLayout layout,
                                                  CityManagementContext context, GameStateBudget budget = {});
} // namespace civ5::saves
