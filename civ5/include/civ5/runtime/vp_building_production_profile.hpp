#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
// VP's building and wonder production caches belong to its GameCore profile.
struct VpBuildingProductionProfileState {
    struct Player {
        std::int64_t wonder=0,world=0,team=0,national=0,golden_age=0,to_building=0,
            capital_discount=0,conquest_era=0,past_era=0,policy_wonder=0,policy_building=0;
    };
    struct City {
        std::int64_t wonder=0,building=0;
        YieldValues local_resources,unimproved_resources;
    };
    std::map<std::int64_t,Player> players;
    std::map<std::pair<std::int64_t,std::int64_t>,City> cities;
    std::map<std::pair<std::int64_t,std::int64_t>,std::int64_t> wonder_units;
};
inline VpBuildingProductionProfileState &ensure_vp_building_production_profile(CityYieldContext &world){
    if(world.profile!=content::TechnologyProfile::vp542)throw CityYieldUnavailable("VP building production requires VP GameCore profile");
    if(auto *state=world.extensions.find<VpBuildingProductionProfileState>())return *state;
    return world.extensions.emplace<VpBuildingProductionProfileState>();
}
inline const VpBuildingProductionProfileState *find_vp_building_production_profile(const CityYieldContext &world){
    return world.profile==content::TechnologyProfile::vp542?world.extensions.find<VpBuildingProductionProfileState>():nullptr;
}
} // namespace civ5::runtime
