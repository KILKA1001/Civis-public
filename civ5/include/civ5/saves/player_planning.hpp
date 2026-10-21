#pragma once
#include "civ5/saves/map_state.hpp"

namespace civ5::saves {
struct PlayerPlanningWireDimensions {
    std::uint64_t major = 22, civ_teams = 63, legacy_teams = 80;
};
struct PlayerPlanningBlob {
    const char *name;
    std::vector<std::uint8_t> bytes;
};
struct PlayerPlanningRecord {
    const char *name;
    // Index is container position, not an ID synthesized from a source field.
    std::uint64_t index = 0, start_offset = 0, end_offset = 0;
    std::vector<NamedSaveInteger> fields;
    std::vector<PlotArrayObservation> arrays;
    std::vector<PlayerPlanningBlob> blobs;
    std::vector<PlayerPlanningRecord> children;
};
struct PlayerPlanningObservation {
    GameStateLayout layout;
    PlayerPlanningWireDimensions dimensions;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::vector<PlayerPlanningRecord> blocks;
};
// Full TacticalAI, HomelandAI, MinorCivAI (quests and incoming gifts), DealAI,
// BuilderTaskingAI, CityConnections, DangerPlots. Next is Traits of this player.
// No cache reset, target recomputation, ID remapping or gameplay side effects.
PlayerPlanningObservation observe_player_planning(runtime::BinaryReader &input, GameStateLayout layout,
                                                  PlayerPlanningWireDimensions dimensions = {},
                                                  GameStateBudget budget = {});
} // namespace civ5::saves
