#pragma once
#include "civ5/saves/team_state.hpp"
#include "civ5/runtime/profile_extensions.hpp"

namespace civ5::saves {
struct PlayerStrategyWireDimensions {
    // Selected source layout only, never capacities for the host game.
    std::uint64_t major = 22, civ_players = 63, yields = 6, grand = 4;
    runtime::ProfileExtensions profile_extensions;
};
struct PlayerStrategyBlock {
    const char *name;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::vector<NamedSaveInteger> fields;
    // Exploration VP: [plot, range1, range2, range3]; BNW: [plot, rating].
    // Goody assignments: [plot, unit ID]. Savings: [type, amount, priority].
    // Weights: [type ID, weight]. Attack targets: [army type, muster plot,
    // staging plot, target plot, path length, approach score, preferred].
    std::vector<PlotArrayObservation> arrays;
    std::vector<TeamHashedArray> hashes;
};
struct PlayerStrategyObservation {
    GameStateLayout layout;
    PlayerStrategyWireDimensions dimensions;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::vector<PlayerStrategyBlock> blocks;
};
// Reads Policies (with PolicyAI), EconomicAI, CitySpecializationAI,
// WonderProductionAI, MilitaryAI and GrandStrategyAI for one player.
// Next is DiplomacyAI, not the next player. No strategy evaluation, discarded
// BNW plot limits, hash remapping or load-time cache updates are applied.
PlayerStrategyObservation observe_player_strategy(runtime::BinaryReader &input, GameStateLayout layout,
                                                  PlayerStrategyWireDimensions dimensions = {},
                                                  GameStateBudget budget = {});
} // namespace civ5::saves
