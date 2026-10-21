#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
// VP's completion counters are saved GameCore state, not a requirement of
// ordinary Civ V cities or arbitrary XML/SQL/Lua mod profiles.
struct VpPlayerAccomplishmentsProfileState {
    std::map<std::int64_t, YieldValues> players;
};

inline VpPlayerAccomplishmentsProfileState &ensure_vp_player_accomplishments_profile(
    CityYieldContext &world) {
    if (world.profile != content::TechnologyProfile::vp542)
        throw CityYieldUnavailable("VP accomplishments require VP GameCore profile");
    if (auto *state = world.extensions.find<VpPlayerAccomplishmentsProfileState>())
        return *state;
    return world.extensions.emplace<VpPlayerAccomplishmentsProfileState>();
}

inline const YieldValues &vp_player_accomplishments(const CityYieldContext &world,
                                                    std::int64_t player) {
    if (world.profile != content::TechnologyProfile::vp542 || !world.players.contains(player))
        throw CityYieldUnavailable("Unresolved VP accomplishment player");
    const auto *state = world.extensions.find<VpPlayerAccomplishmentsProfileState>();
    if (!state) throw CityYieldUnavailable("Unresolved VP accomplishments");
    const auto found = state->players.find(player);
    if (found == state->players.end())
        throw CityYieldUnavailable("Unresolved VP player accomplishments");
    return found->second;
}
} // namespace civ5::runtime
