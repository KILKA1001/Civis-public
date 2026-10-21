#pragma once
#include "civ5/runtime/maintenance.hpp"

namespace civ5::runtime {
// VP's saved supply inputs and its unsaved capacity caches belong to the
// selected GameCore profile. BNW has a separate unit-supply expense formula.
struct VpUnitSupplyProfileState {
    struct Player {
        std::int64_t military=0,free=0,extra_flat=0,city_flat=0,extra_population=0,expended=0,puppet=0;
        std::int64_t trait_base=0,trait_city=0,trait_population=0,trait_puppet=0;
        std::int64_t war_value=0,war_modifier=0,trait_war_modifier=0,minor_type=-1,personality=-1;
        YieldValues war_weariness;
        // Unsaved native caches start unresolved. nullopt replaces source -1.
        std::optional<std::int64_t> base_cache,war_cache;
    };
    std::optional<std::int64_t> world_city_supply_modifier;
    std::map<std::int64_t,Player> players;
};
inline VpUnitSupplyProfileState &ensure_vp_unit_supply_profile(MaintenanceContext &world){
    if(world.profile!=content::TechnologyProfile::vp542)
        throw MaintenanceUnavailable("VP unit supply requires VP GameCore profile");
    if(auto *state=world.extensions.find<VpUnitSupplyProfileState>())return *state;
    return world.extensions.emplace<VpUnitSupplyProfileState>();
}
inline const VpUnitSupplyProfileState *find_vp_unit_supply_profile(const MaintenanceContext &world){
    return world.profile==content::TechnologyProfile::vp542
        ?world.extensions.find<VpUnitSupplyProfileState>():nullptr;
}
inline VpUnitSupplyProfileState::Player &vp_unit_supply_player(MaintenanceContext &world,std::int64_t id){
    if(!world.players.contains(id))throw MaintenanceUnavailable("Unresolved VP supply player identity");
    auto *profile=world.profile==content::TechnologyProfile::vp542
        ?world.extensions.find<VpUnitSupplyProfileState>():nullptr;
    if(!profile)throw MaintenanceUnavailable("Unresolved VP supply profile");
    const auto found=profile->players.find(id);
    if(found==profile->players.end())throw MaintenanceUnavailable("Unresolved VP supply player");
    return found->second;
}
inline const VpUnitSupplyProfileState::Player &vp_unit_supply_player(const MaintenanceContext &world,std::int64_t id){
    if(!world.players.contains(id))throw MaintenanceUnavailable("Unresolved VP supply player identity");
    const auto *profile=find_vp_unit_supply_profile(world);
    if(!profile)throw MaintenanceUnavailable("Unresolved VP supply profile");
    const auto found=profile->players.find(id);
    if(found==profile->players.end())throw MaintenanceUnavailable("Unresolved VP supply player");
    return found->second;
}
inline std::int64_t vp_world_city_supply_modifier(const MaintenanceContext &world){
    const auto *profile=find_vp_unit_supply_profile(world);
    if(!profile||!profile->world_city_supply_modifier)
        throw MaintenanceUnavailable("Unresolved VP world city supply modifier");
    return *profile->world_city_supply_modifier;
}
} // namespace civ5::runtime
