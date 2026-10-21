#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
// VP stores a separate state-religion cache for the city-yield and religion
// formulas. BNW's shared player state has no need for this field.
struct VpPlayerReligionProfileState {
    std::optional<std::int64_t> state_religion;
};

inline std::optional<std::int64_t> &vp_player_state_religion(CityYieldPlayer &player) {
    auto *state = player.extensions.find<VpPlayerReligionProfileState>();
    if (!state) state = &player.extensions.emplace<VpPlayerReligionProfileState>();
    return state->state_religion;
}

inline const std::optional<std::int64_t> &vp_player_state_religion(const CityYieldPlayer &player) {
    static const std::optional<std::int64_t> unresolved;
    const auto *state = player.extensions.find<VpPlayerReligionProfileState>();
    return state ? state->state_religion : unresolved;
}
} // namespace civ5::runtime
