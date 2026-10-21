#include "civ5/runtime/vp_player_research_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::runtime {
std::int64_t vp_effective_cities(const PlayerResearchState &player, bool include_puppets) {
    if (player.cities.size() > INT64_MAX)
        throw std::overflow_error("Research collection exceeds int64");
    auto cities = static_cast<std::int64_t>(player.cities.size());
    for (const auto &[id,c] : player.cities) {
        if (!include_puppets && c.puppet) --cities;
        if (c.ignore_happiness) --cities;
    }
    return std::max<std::int64_t>(1,cities);
}
std::int64_t vp_research_city_modifier_times100(const PlayerResearchState &p,
    std::int64_t world, std::int64_t scaling, bool include_puppets, std::int64_t offset) {
    const auto base = wide::add(world,wide::multiply_divide(vp_player_research_profile(p).city_cost_modifier,100,1));
    if (world > 0) {
        scaling = wide::multiply_add_divide(scaling,base,world/2,world);
    }
    auto cities = p.cities.empty() ? 0 : vp_effective_cities(p,include_puppets);
    cities = std::max<std::int64_t>(0,wide::add(cities,offset));
    const auto linear = wide::multiply_divide(base,cities,1);
    if (scaling == 0) return linear;
    // Divide one even factor before multiplying; retain the exact source C(n,2).
    const auto pairs = cities%2 == 0 ? wide::multiply_divide(cities/2,wide::subtract(cities,1),1)
                                    : wide::multiply_divide(cities,(cities-1)/2,1);
    return wide::add(linear,wide::multiply_divide(scaling,pairs,1));
}
} // namespace civ5::runtime
