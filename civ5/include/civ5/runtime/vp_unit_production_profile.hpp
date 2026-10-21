#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
// VP GameCore state is deliberately absent from CityYieldPlayer/State and
// MaintenanceUnit. IDs are live 64-bit identities, never source array limits.
struct VpUnitProductionProfileState {
    struct Player {
        std::int64_t military=0,military_per_major_war=0,settler=0;
        YieldValues combat;
    };
    struct CityPlayer {
        std::int64_t capital_settler=0;
        YieldValues classes,pledge_domains;
        std::set<std::int64_t> protected_by;
    };
    struct City {
        std::int64_t military=0;
        YieldValues domains,combats;
    };
    std::map<std::int64_t,Player> players;
    std::map<std::int64_t,CityPlayer> city_players;
    std::map<std::pair<std::int64_t,std::int64_t>,City> cities;
    std::map<std::pair<std::int64_t,std::int64_t>,std::int64_t> military_units;
    // Index follows CityYieldContext::leagues; active-resolution order is kept.
    std::vector<std::vector<std::int64_t>> league_space_ship_production;
};
inline VpUnitProductionProfileState &ensure_vp_unit_production_profile(CityYieldContext &world){
    if(world.profile!=content::TechnologyProfile::vp542)throw CityYieldUnavailable("VP unit production requires VP GameCore profile");
    if(auto *state=world.extensions.find<VpUnitProductionProfileState>())return *state;
    return world.extensions.emplace<VpUnitProductionProfileState>();
}
inline const VpUnitProductionProfileState *find_vp_unit_production_profile(const CityYieldContext &world){
    return world.profile==content::TechnologyProfile::vp542?world.extensions.find<VpUnitProductionProfileState>():nullptr;
}
} // namespace civ5::runtime
