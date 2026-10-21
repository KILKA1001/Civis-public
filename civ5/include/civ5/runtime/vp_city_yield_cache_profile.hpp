#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
// CvCity's static yield cache belongs to the selected VP GameCore. Other
// profiles can supply their own cache without adding fields to CityYieldState.
struct VpCityYieldCacheProfileState {
    std::map<std::pair<std::int64_t, std::int64_t>, YieldValues> cities;
};

inline YieldValues &vp_city_cached_yields(CityYieldContext &world, std::int64_t owner,
                                          std::int64_t city) {
    if (world.profile != content::TechnologyProfile::vp542)
        throw CityYieldUnavailable("VP city yield cache requires VP GameCore profile");
    auto *state = world.extensions.find<VpCityYieldCacheProfileState>();
    if (!state) state = &world.extensions.emplace<VpCityYieldCacheProfileState>();
    return state->cities[{owner, city}];
}

inline const YieldValues &vp_city_cached_yields(const CityYieldContext &world,
                                                std::int64_t owner, std::int64_t city) {
    if (world.profile != content::TechnologyProfile::vp542)
        throw CityYieldUnavailable("VP city yield cache requires VP GameCore profile");
    static const YieldValues empty;
    const auto *state = world.extensions.find<VpCityYieldCacheProfileState>();
    if (!state) return empty;
    const auto found = state->cities.find({owner, city});
    return found == state->cities.end() ? empty : found->second;
}

inline std::int64_t vp_city_cached_yield(const CityYieldContext &world, std::int64_t owner,
                                         std::int64_t city, std::int64_t yield) {
    const auto &values = vp_city_cached_yields(world, owner, city);
    const auto found = values.find(yield);
    return found == values.end() ? 0 : found->second;
}

void restore_vp_city_yield_cache(CityYieldContext &world, std::int64_t owner,
                                 std::int64_t city, const saves::CityCoreObservation &source);
CityYieldQuote vp_city_cached_yield_quote(const CityYieldContext &world, std::int64_t owner,
                                          std::int64_t city, std::int64_t yield,
                                          bool ignore_trade);
} // namespace civ5::runtime
