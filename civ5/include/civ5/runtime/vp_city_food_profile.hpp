#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
struct VpCityFoodProfileState {
    struct Player {
        std::int64_t non_specialist_change_times100=0,specialist_change=0,
            half_specialist_count=0,half_specialist_capital_count=0,military_production_count=0;
    };
    struct City {
        std::int64_t stored_times100=0,additional=0,no_starvation_non_specialist=0,minimum=0;
        YieldValues specialists;
    };
    std::map<std::int64_t,Player> players;
    std::map<std::pair<std::int64_t,std::int64_t>,City> cities;
};
inline VpCityFoodProfileState &ensure_vp_city_food_profile(CityYieldContext &world){
    if(world.profile!=content::TechnologyProfile::vp542)
        throw CityYieldUnavailable("VP city food requires VP GameCore profile");
    if(auto *state=world.extensions.find<VpCityFoodProfileState>())return *state;
    return world.extensions.emplace<VpCityFoodProfileState>();
}
inline const VpCityFoodProfileState *find_vp_city_food_profile(const CityYieldContext &world){
    return world.profile==content::TechnologyProfile::vp542
        ?world.extensions.find<VpCityFoodProfileState>():nullptr;
}
inline const VpCityFoodProfileState::Player &vp_city_food_player(const CityYieldContext &world,std::int64_t owner){
    const auto *state=find_vp_city_food_profile(world);
    if(!state)throw CityYieldUnavailable("Unresolved VP player food state");
    const auto found=state->players.find(owner);
    if(found==state->players.end())throw CityYieldUnavailable("Unresolved VP player food state");
    return found->second;
}
inline const VpCityFoodProfileState::City &vp_city_food_city(const CityYieldContext &world,std::int64_t owner,std::int64_t city){
    const auto *state=find_vp_city_food_profile(world);
    if(!state)throw CityYieldUnavailable("Unresolved VP city food state");
    const auto found=state->cities.find({owner,city});
    if(found==state->cities.end())throw CityYieldUnavailable("Unresolved VP city food state");
    return found->second;
}
} // namespace civ5::runtime
