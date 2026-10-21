#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
// Corporation caches and active records belong to the VP GameCore profile.
struct VpCorporationProfileState {
    struct Player {
        std::int64_t founded=-1,offices=0,franchises=0,additional_franchises=0,
            additional_franchises_modifier=0,offices_as_franchises=0,
            random_foreign_franchise_modifier=0,free_franchise_above_popular=0,
            no_foreign_corporations_in_cities=0,no_franchises_in_foreign_cities=0;
        std::vector<std::int64_t> franchises_per_improvement;
    };
    std::map<std::int64_t,Player> players;
    std::optional<std::vector<saves::CorporationObservation>> active;
};
inline VpCorporationProfileState &ensure_vp_corporation_profile(CityYieldContext &world){
    if(world.profile!=content::TechnologyProfile::vp542)
        throw CityYieldUnavailable("VP corporations require VP GameCore profile");
    if(auto *state=world.extensions.find<VpCorporationProfileState>())return *state;
    return world.extensions.emplace<VpCorporationProfileState>();
}
inline const VpCorporationProfileState *find_vp_corporation_profile(const CityYieldContext &world){
    return world.profile==content::TechnologyProfile::vp542
        ?world.extensions.find<VpCorporationProfileState>():nullptr;
}
} // namespace civ5::runtime
