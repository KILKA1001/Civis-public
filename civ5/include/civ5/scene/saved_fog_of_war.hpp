#pragma once
#include "civ5/materials/fog_of_war.hpp"
#include "civ5/saves/map_state.hpp"
#include <cstdint>

namespace civ5::scene {
struct SavedFogOfWarCounts {
    std::uint64_t visible = 0, explored = 0, unexplored = 0;
    std::uint64_t visible_unrevealed = 0;
    bool operator==(const SavedFogOfWarCounts &) const = default;
};
// Replay the serialized active team's map into the original FOW visibility
// owner. The caller owns the world layout, state lifetime and animation tick.
SavedFogOfWarCounts apply_saved_map_fog(const saves::MapGrid &grid,
                                       const saves::SavedMapTeamArtObservation &art,
                                       materials::FogOfWarState &state);
} // namespace civ5::scene
