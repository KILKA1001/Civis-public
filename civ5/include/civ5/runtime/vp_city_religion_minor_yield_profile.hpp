#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
struct VpCityReligionMinorYieldProfileState {
    struct Player {std::int64_t allies=0,friends=0;YieldValues own_pantheon;};
    struct City {
        YieldValues special_religion,alliance,friendship,per_ally,per_friend,from_minors;
        YieldModifierRows terrain_religion,feature_religion;
    };
    std::map<std::int64_t,Player> players;
    std::map<std::pair<std::int64_t,std::int64_t>,City> cities;
};
inline VpCityReligionMinorYieldProfileState &ensure_vp_city_religion_minor_yield_profile(CityYieldContext &world){
    if(world.profile!=content::TechnologyProfile::vp542)throw CityYieldUnavailable("VP religion/minor yields require VP GameCore profile");
    if(auto *state=world.extensions.find<VpCityReligionMinorYieldProfileState>())return *state;
    return world.extensions.emplace<VpCityReligionMinorYieldProfileState>();
}
inline const VpCityReligionMinorYieldProfileState *find_vp_city_religion_minor_yield_profile(const CityYieldContext &world){
    return world.profile==content::TechnologyProfile::vp542?world.extensions.find<VpCityReligionMinorYieldProfileState>():nullptr;
}
inline const VpCityReligionMinorYieldProfileState::Player &vp_city_religion_minor_player(const CityYieldContext &world,std::int64_t owner){
    const auto *state=find_vp_city_religion_minor_yield_profile(world);if(!state)throw CityYieldUnavailable("Unresolved VP player religion/minor yields");
    const auto found=state->players.find(owner);if(found==state->players.end())throw CityYieldUnavailable("Unresolved VP player religion/minor yields");return found->second;
}
inline const VpCityReligionMinorYieldProfileState::City &vp_city_religion_minor_city(const CityYieldContext &world,std::int64_t owner,std::int64_t city){
    const auto *state=find_vp_city_religion_minor_yield_profile(world);if(!state)throw CityYieldUnavailable("Unresolved VP city religion/minor yields");
    const auto found=state->cities.find({owner,city});if(found==state->cities.end())throw CityYieldUnavailable("Unresolved VP city religion/minor yields");return found->second;
}
} // namespace civ5::runtime
