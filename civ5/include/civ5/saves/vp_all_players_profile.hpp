#pragma once
#include <cstdint>

namespace civ5::saves {
// Source wire dimensions owned by the VP GameCore compatibility profile.
// They describe serialized arrays, never host-side capacity limits.
struct VpAllPlayersWireDimensions {
    std::uint64_t city_strategies = 0;
    std::uint64_t city_emphases = 0;
    std::uint64_t buildings = 0;
};
} // namespace civ5::saves
