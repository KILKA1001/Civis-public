#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
struct VpCityBaseYieldProfileState {
    struct Player {
        std::int64_t landmarks_tourism=0;
        YieldValues state_religion,non_specialists,capital_tiles;
    };
    struct City {
        std::int64_t world_wonders=0,landmarks_tourism=0;
        YieldValues corporations,events,league,empire_population,happiness,health,previous_golden_ages;
        std::vector<bool> working;
        std::vector<std::int64_t> worked_plots;
    };
    std::map<std::int64_t,Player> players;
    std::map<std::pair<std::int64_t,std::int64_t>,City> cities;
};
inline VpCityBaseYieldProfileState &ensure_vp_city_base_yield_profile(CityYieldContext &world){
    if(world.profile!=content::TechnologyProfile::vp542)
        throw CityYieldUnavailable("VP city base yields require VP GameCore profile");
    if(auto *state=world.extensions.find<VpCityBaseYieldProfileState>())return *state;
    return world.extensions.emplace<VpCityBaseYieldProfileState>();
}
inline const VpCityBaseYieldProfileState *find_vp_city_base_yield_profile(const CityYieldContext &world){
    return world.profile==content::TechnologyProfile::vp542
        ?world.extensions.find<VpCityBaseYieldProfileState>():nullptr;
}
inline const VpCityBaseYieldProfileState::Player &vp_city_base_yield_player(const CityYieldContext &world,std::int64_t owner){
    const auto *state=find_vp_city_base_yield_profile(world);
    if(!state)throw CityYieldUnavailable("Unresolved VP player base yields");
    const auto found=state->players.find(owner);
    if(found==state->players.end())throw CityYieldUnavailable("Unresolved VP player base yields");
    return found->second;
}
inline VpCityBaseYieldProfileState::City &vp_city_base_yield_city(CityYieldContext &world,std::int64_t owner,std::int64_t city){
    auto *state=world.extensions.find<VpCityBaseYieldProfileState>();
    if(world.profile!=content::TechnologyProfile::vp542||!state)
        throw CityYieldUnavailable("Unresolved VP city base yields");
    const auto found=state->cities.find({owner,city});
    if(found==state->cities.end())throw CityYieldUnavailable("Unresolved VP city base yields");
    return found->second;
}
inline const VpCityBaseYieldProfileState::City &vp_city_base_yield_city(const CityYieldContext &world,std::int64_t owner,std::int64_t city){
    const auto *state=find_vp_city_base_yield_profile(world);
    if(!state)throw CityYieldUnavailable("Unresolved VP city base yields");
    const auto found=state->cities.find({owner,city});
    if(found==state->cities.end())throw CityYieldUnavailable("Unresolved VP city base yields");
    return found->second;
}
} // namespace civ5::runtime
