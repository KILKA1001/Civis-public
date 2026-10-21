#pragma once
#include "civ5/saves/city_tail.hpp"

namespace civ5::saves {
// VP GameCore source widths for serialized city emphasis arrays.
struct VpCityTailWireDimensions {
    std::uint64_t yields = 0, emphases = 0;
};

inline const VpCityTailWireDimensions *vp_city_tail_dimensions(
    const CityTailContext &context) {
    return context.profile_extensions.find<VpCityTailWireDimensions>();
}
} // namespace civ5::saves
