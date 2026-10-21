#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
struct VpGreatWorkProfileState {
    struct GreatWorkSlot {
        std::int64_t building_class=-1,slot=-1,work=-1;
        bool operator==(const GreatWorkSlot&)const=default;
    };
    struct Player {
        std::int64_t tourism_modifier=0,capital_theming_modifier=0,theming_policy_modifier=0;
        YieldValues per_work,trait_per_work;
        std::array<YieldValues,6> type_bonus;
        std::array<YieldValues,4> trait_type_bonus;
    };
    struct City {
        std::int64_t tourism_modifier=0;
        YieldValues per_work,per_local_theme,theming_indices;
        std::vector<GreatWorkSlot> works;
        std::vector<std::int64_t> existing_buildings;
        std::optional<YieldValues> building_by_class;
        YieldValues yield_cache;
    };
    std::map<std::int64_t,Player> players;
    std::map<std::pair<std::int64_t,std::int64_t>,City> cities;
};
inline VpGreatWorkProfileState &ensure_vp_great_work_profile(CityYieldContext &world){
    if(world.profile!=content::TechnologyProfile::vp542)throw CityYieldUnavailable("VP great works require VP GameCore profile");
    if(auto *state=world.extensions.find<VpGreatWorkProfileState>())return *state;
    return world.extensions.emplace<VpGreatWorkProfileState>();
}
inline const VpGreatWorkProfileState *find_vp_great_work_profile(const CityYieldContext &world){
    return world.profile==content::TechnologyProfile::vp542?world.extensions.find<VpGreatWorkProfileState>():nullptr;
}
inline const VpGreatWorkProfileState::Player &vp_great_work_player(const CityYieldContext &world,std::int64_t owner){
    const auto *state=find_vp_great_work_profile(world);if(!state)throw CityYieldUnavailable("Unresolved VP player great-work state");
    const auto found=state->players.find(owner);if(found==state->players.end())throw CityYieldUnavailable("Unresolved VP player great-work state");return found->second;
}
inline VpGreatWorkProfileState::Player &vp_great_work_player(CityYieldContext &world,std::int64_t owner){
    auto &state=ensure_vp_great_work_profile(world);const auto found=state.players.find(owner);
    if(found==state.players.end())throw CityYieldUnavailable("Unresolved VP player great-work state");return found->second;
}
inline const VpGreatWorkProfileState::City &vp_great_work_city(const CityYieldContext &world,std::int64_t owner,std::int64_t city){
    const auto *state=find_vp_great_work_profile(world);if(!state)throw CityYieldUnavailable("Unresolved VP city great-work state");
    const auto found=state->cities.find({owner,city});if(found==state->cities.end())throw CityYieldUnavailable("Unresolved VP city great-work state");return found->second;
}
inline VpGreatWorkProfileState::City &vp_great_work_city(CityYieldContext &world,std::int64_t owner,std::int64_t city){
    auto &state=ensure_vp_great_work_profile(world);const auto found=state.cities.find({owner,city});
    if(found==state.cities.end())throw CityYieldUnavailable("Unresolved VP city great-work state");return found->second;
}
} // namespace civ5::runtime
