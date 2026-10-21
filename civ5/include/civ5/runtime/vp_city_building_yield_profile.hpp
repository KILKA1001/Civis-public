#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
struct VpCityBuildingYieldProfileState {
    struct Player {std::int64_t strategic_from_minors=0;YieldValues per_religion_times100;};
    struct City {
        std::int64_t buildings=0;
        std::set<std::int64_t> plots;
        YieldValues era_scaling_times100,passing_trade;
        std::array<std::map<std::int64_t,YieldFraction>,3> fractions;
        YieldModifierRows per_x_terrain,per_x_feature;
    };
    std::map<std::int64_t,Player> players;
    std::map<std::pair<std::int64_t,std::int64_t>,City> cities;
};
inline VpCityBuildingYieldProfileState &ensure_vp_city_building_yield_profile(CityYieldContext &world){
    if(world.profile!=content::TechnologyProfile::vp542)throw CityYieldUnavailable("VP building yields require VP GameCore profile");
    if(auto *state=world.extensions.find<VpCityBuildingYieldProfileState>())return *state;
    return world.extensions.emplace<VpCityBuildingYieldProfileState>();
}
inline const VpCityBuildingYieldProfileState *find_vp_city_building_yield_profile(const CityYieldContext &world){
    return world.profile==content::TechnologyProfile::vp542?world.extensions.find<VpCityBuildingYieldProfileState>():nullptr;
}
inline const VpCityBuildingYieldProfileState::Player &vp_city_building_yield_player(const CityYieldContext &world,std::int64_t owner){
    const auto *state=find_vp_city_building_yield_profile(world);if(!state)throw CityYieldUnavailable("Unresolved VP player building yields");
    const auto found=state->players.find(owner);if(found==state->players.end())throw CityYieldUnavailable("Unresolved VP player building yields");return found->second;
}
inline const VpCityBuildingYieldProfileState::City &vp_city_building_yield_city(const CityYieldContext &world,std::int64_t owner,std::int64_t city){
    const auto *state=find_vp_city_building_yield_profile(world);if(!state)throw CityYieldUnavailable("Unresolved VP city building yields");
    const auto found=state->cities.find({owner,city});if(found==state->cities.end())throw CityYieldUnavailable("Unresolved VP city building yields");return found->second;
}
} // namespace civ5::runtime
