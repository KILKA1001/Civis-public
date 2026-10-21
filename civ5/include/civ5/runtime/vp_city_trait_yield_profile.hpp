#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
struct VpCityTraitYieldProfileState {
    struct PlayerTraitYield {
        bool capital_only=false;
        std::int64_t historic_events=0,city_culture=0,golden_age_tourism=0;
        YieldValues improvements_built,historic,per_partner,land_connections,connection,trait_connection;
        std::map<std::int64_t,YieldValues> per_improvement;
        std::vector<std::int64_t> improvement_types;
    };
    struct CityTraitYield {
        bool connected=false,industrial=false;
        std::int64_t connection_modifier=0;
        YieldValues added,unimproved,permanent_wltkd;
    };
    struct TraitEraState {std::int64_t fractional_era=0;bool era_scaling=false,odd_era=false;};
    std::map<std::int64_t,PlayerTraitYield> players;
    std::map<std::pair<std::int64_t,std::int64_t>,CityTraitYield> cities;
    std::map<std::int64_t,TraitEraState> eras;
};

inline VpCityTraitYieldProfileState &ensure_vp_city_trait_yield_profile(CityYieldContext &world){
    if(world.profile!=content::TechnologyProfile::vp542)
        throw CityYieldUnavailable("VP city trait yields require VP GameCore profile");
    if(auto *state=world.extensions.find<VpCityTraitYieldProfileState>())return *state;
    return world.extensions.emplace<VpCityTraitYieldProfileState>();
}
inline const VpCityTraitYieldProfileState *find_vp_city_trait_yield_profile(const CityYieldContext &world){
    return world.profile==content::TechnologyProfile::vp542
        ?world.extensions.find<VpCityTraitYieldProfileState>():nullptr;
}
inline const VpCityTraitYieldProfileState::PlayerTraitYield &vp_city_trait_yield_player(const CityYieldContext &world,std::int64_t owner){
    const auto *state=find_vp_city_trait_yield_profile(world);
    if(!state)throw CityYieldUnavailable("Unresolved VP player trait yields");
    const auto found=state->players.find(owner);
    if(found==state->players.end())throw CityYieldUnavailable("Unresolved VP player trait yields");
    return found->second;
}
inline VpCityTraitYieldProfileState::PlayerTraitYield &vp_city_trait_yield_player(CityYieldContext &world,std::int64_t owner){
    auto &state=ensure_vp_city_trait_yield_profile(world);const auto found=state.players.find(owner);
    if(found==state.players.end())throw CityYieldUnavailable("Unresolved VP player trait yields");return found->second;
}
inline const VpCityTraitYieldProfileState::CityTraitYield &vp_city_trait_yield_city(const CityYieldContext &world,std::int64_t owner,std::int64_t city){
    const auto *state=find_vp_city_trait_yield_profile(world);
    if(!state)throw CityYieldUnavailable("Unresolved VP city trait yields");
    const auto found=state->cities.find({owner,city});
    if(found==state->cities.end())throw CityYieldUnavailable("Unresolved VP city trait yields");
    return found->second;
}
inline VpCityTraitYieldProfileState::CityTraitYield &vp_city_trait_yield_city(CityYieldContext &world,std::int64_t owner,std::int64_t city){
    auto &state=ensure_vp_city_trait_yield_profile(world);const auto found=state.cities.find({owner,city});
    if(found==state.cities.end())throw CityYieldUnavailable("Unresolved VP city trait yields");return found->second;
}
inline const VpCityTraitYieldProfileState::TraitEraState &vp_city_trait_era(const CityYieldContext &world,std::int64_t owner){
    const auto *state=find_vp_city_trait_yield_profile(world);
    if(!state)throw CityYieldUnavailable("Unresolved VP trait era state");
    const auto found=state->eras.find(owner);
    if(found==state->eras.end())throw CityYieldUnavailable("Unresolved VP trait era state");
    return found->second;
}
inline VpCityTraitYieldProfileState::TraitEraState &vp_city_trait_era(CityYieldContext &world,std::int64_t owner){
    auto &state=ensure_vp_city_trait_yield_profile(world);const auto found=state.eras.find(owner);
    if(found==state.eras.end())throw CityYieldUnavailable("Unresolved VP trait era state");return found->second;
}
} // namespace civ5::runtime
