#pragma once
#include "civ5/saves/city_management.hpp"

namespace civ5::saves {
// VP GameCore source widths for strategy and citizen arrays. The source
// SpecialistTypes field uses promotion slots; this is not a host capacity.
struct VpCityManagementWireDimensions {
    std::uint64_t flavors = 0, strategies = 0, promotions = 0, buildings = 0;
};

inline const VpCityManagementWireDimensions *vp_city_management_dimensions(
    const CityManagementContext &context) {
    return context.profile_extensions.find<VpCityManagementWireDimensions>();
}
} // namespace civ5::saves
