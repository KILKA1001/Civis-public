#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
struct VpCitySupplyProfileState {
    struct City {std::int64_t flat=0,population_modifier=0;};
    std::map<std::pair<std::int64_t,std::int64_t>,City> cities;
};
inline VpCitySupplyProfileState &ensure_vp_city_supply_profile(CityYieldContext &world){
    if(world.profile!=content::TechnologyProfile::vp542)throw CityYieldUnavailable("VP city supply requires VP GameCore profile");
    if(auto *state=world.extensions.find<VpCitySupplyProfileState>())return *state;
    return world.extensions.emplace<VpCitySupplyProfileState>();
}
inline const VpCitySupplyProfileState *find_vp_city_supply_profile(const CityYieldContext &world){
    return world.profile==content::TechnologyProfile::vp542?world.extensions.find<VpCitySupplyProfileState>():nullptr;
}
inline const VpCitySupplyProfileState::City &vp_city_supply(const CityYieldContext &world,std::int64_t owner,std::int64_t city){
    const auto *state=find_vp_city_supply_profile(world);if(!state)throw CityYieldUnavailable("Unresolved VP city supply");
    const auto found=state->cities.find({owner,city});if(found==state->cities.end())throw CityYieldUnavailable("Unresolved VP city supply");return found->second;
}
} // namespace civ5::runtime
