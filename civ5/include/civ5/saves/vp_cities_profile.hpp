#pragma once
#include <cstdint>

namespace civ5::saves {
// VP GameCore widths for a serialized city collection. These are source wire
// dimensions, not limits on the number of Civis cities or buildings.
struct VpCitiesWireDimensions {
    std::uint64_t flavors = 0;
    std::uint64_t strategies = 0;
    std::uint64_t promotions = 0;
    std::uint64_t buildings = 0;
    std::uint64_t yields = 0;
    std::uint64_t emphases = 0;
};
} // namespace civ5::saves
