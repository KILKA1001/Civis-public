#pragma once
#include "civ5/saves/city_production.hpp"

namespace civ5::saves {
// The VP source GameCore serializes building arrays by its activated Buildings
// collection span. This wire width does not limit Civis's own building IDs.
struct VpCityProductionWireDimensions {
    std::uint64_t building_slots = 0;
};

inline std::optional<std::uint64_t> vp_city_production_building_slots(
    const CityProductionContext &context) {
    const auto *dimensions = context.profile_extensions.find<VpCityProductionWireDimensions>();
    return dimensions ? std::optional<std::uint64_t>(dimensions->building_slots) : std::nullopt;
}
} // namespace civ5::saves
