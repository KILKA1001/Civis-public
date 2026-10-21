#pragma once
#include "civ5/runtime/player_research.hpp"
#include <stdexcept>

namespace civ5::runtime {
// These fields are serialized and calculated only by the VP GameCore profile.
struct VpPlayerResearchProfileState {
    std::int64_t city_cost_modifier=0;
    std::int64_t cs_allies=0;
    std::int64_t science_from_league=0;
    std::uint64_t spies=0;
};
struct VpResearchCityProfileState {
    std::set<std::int64_t> assigned_spy_players;
};

inline VpResearchCityProfileState &ensure_vp_research_city_profile(ResearchCityState &city) {
    return city.profile_extensions.emplace<VpResearchCityProfileState>();
}
inline const VpResearchCityProfileState *find_vp_research_city_profile(const ResearchCityState &city) {
    return city.profile_extensions.find<VpResearchCityProfileState>();
}

inline VpPlayerResearchProfileState &vp_player_research_profile(PlayerResearchState &player){
    auto *state=player.profile_extensions.find<VpPlayerResearchProfileState>();
    if(!state)throw std::runtime_error("Missing VP player research profile");
    return *state;
}
inline const VpPlayerResearchProfileState &vp_player_research_profile(const PlayerResearchState &player){
    const auto *state=player.profile_extensions.find<VpPlayerResearchProfileState>();
    if(!state)throw std::runtime_error("Missing VP player research profile");
    return *state;
}

std::int64_t vp_effective_cities(const PlayerResearchState &player, bool include_puppets);
std::int64_t vp_research_city_modifier_times100(const PlayerResearchState &player,
    std::int64_t world_modifier, std::int64_t scaling, bool include_puppets, std::int64_t offset = 0);
} // namespace civ5::runtime
