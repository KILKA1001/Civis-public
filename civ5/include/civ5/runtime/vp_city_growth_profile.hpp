#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
struct VpCityGrowthProfileState {
    struct Player {
        std::int64_t city=0,capital=0,boon=0;
        bool population_boost_religion=false;
        YieldValues golden_age,trait_golden_age;
    };
    struct City {
        std::int64_t happiness_delta=0,event=0,tourism=0;
        YieldValues golden_age;
    };
    std::map<std::int64_t,Player> players;
    std::map<std::pair<std::int64_t,std::int64_t>,City> cities;
};
inline VpCityGrowthProfileState &ensure_vp_city_growth_profile(CityYieldContext &world){
    if(world.profile!=content::TechnologyProfile::vp542)
        throw CityYieldUnavailable("VP city growth requires VP GameCore profile");
    if(auto *state=world.extensions.find<VpCityGrowthProfileState>())return *state;
    return world.extensions.emplace<VpCityGrowthProfileState>();
}
inline const VpCityGrowthProfileState *find_vp_city_growth_profile(const CityYieldContext &world){
    return world.profile==content::TechnologyProfile::vp542
        ?world.extensions.find<VpCityGrowthProfileState>():nullptr;
}
inline const VpCityGrowthProfileState::Player &vp_city_growth_player(const CityYieldContext &world,std::int64_t owner){
    const auto *state=find_vp_city_growth_profile(world);
    if(!state)throw CityYieldUnavailable("Unresolved VP player growth state");
    const auto found=state->players.find(owner);
    if(found==state->players.end())throw CityYieldUnavailable("Unresolved VP player growth state");
    return found->second;
}
inline const VpCityGrowthProfileState::City &vp_city_growth_city(const CityYieldContext &world,std::int64_t owner,std::int64_t city){
    const auto *state=find_vp_city_growth_profile(world);
    if(!state)throw CityYieldUnavailable("Unresolved VP city growth state");
    const auto found=state->cities.find({owner,city});
    if(found==state->cities.end())throw CityYieldUnavailable("Unresolved VP city growth state");
    return found->second;
}
} // namespace civ5::runtime
