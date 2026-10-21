#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
// GameCore-specific production modifiers. The shared city/player keep only
// their identities and mechanics that do not require this VP profile.
struct VpGeneralProductionProfileState {
    struct Player {
        std::int64_t puppet=0,occupied=0,conquest_turns=0,conquest=0,space=0;
        bool from_specialists=false;
    };
    struct City {std::int64_t space=0;};
    std::map<std::int64_t,Player> players;
    std::map<std::pair<std::int64_t,std::int64_t>,City> cities;
};
inline VpGeneralProductionProfileState &ensure_vp_general_production_profile(CityYieldContext &world){
    if(world.profile!=content::TechnologyProfile::vp542)throw CityYieldUnavailable("VP production requires VP GameCore profile");
    if(auto *state=world.extensions.find<VpGeneralProductionProfileState>())return *state;
    return world.extensions.emplace<VpGeneralProductionProfileState>();
}
inline const VpGeneralProductionProfileState *find_vp_general_production_profile(const CityYieldContext &world){
    return world.profile==content::TechnologyProfile::vp542?world.extensions.find<VpGeneralProductionProfileState>():nullptr;
}
} // namespace civ5::runtime
