#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
// VP saves the majority religion as a city cache. It is not derived from the
// follower rows, and other GameCore profiles need not carry this field.
struct VpCityReligionProfileState {
    std::optional<std::int64_t> majority_religion;
};

inline std::optional<std::int64_t> &vp_city_majority_religion(CityYieldState &city) {
    auto *state = city.extensions.find<VpCityReligionProfileState>();
    if (!state) state = &city.extensions.emplace<VpCityReligionProfileState>();
    return state->majority_religion;
}

inline const std::optional<std::int64_t> &vp_city_majority_religion(const CityYieldState &city) {
    static const std::optional<std::int64_t> unresolved;
    const auto *state = city.extensions.find<VpCityReligionProfileState>();
    return state ? state->majority_religion : unresolved;
}
} // namespace civ5::runtime
