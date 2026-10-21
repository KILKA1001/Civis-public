#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
struct VpCityHappinessProfileState {
    struct Player {
        bool no_annexing=false;
        std::int64_t garrison=0,strength=0,per_x_works=0,specialists=0,per_city=0,per_x_population=0,per_route=0,to_culture=0;
        std::int64_t policy_capital=0,policy_city=0;
        YieldFraction per_population,per_policy_from_policies,per_policy;
    };
    struct City {
        bool occupied=false;
        std::int64_t no_occupied_unhappiness=0,empire=0,buildings=0,building_classes=0,religion=0,unmodded_buildings=0;
        YieldValues policies,crime,development;
    };
    struct TradeStats {
        std::optional<std::int64_t> turn_slice;
        std::int64_t international_out=0,international_in=0,internal=0,minors=0;
        YieldValues from_city,to_city;
    };
    std::map<std::int64_t,Player> players;
    std::map<std::pair<std::int64_t,std::int64_t>,City> cities;
    std::map<std::int64_t,TradeStats> trade_stats;
};

inline VpCityHappinessProfileState &ensure_vp_city_happiness_profile(CityYieldContext &world){
    if(world.profile!=content::TechnologyProfile::vp542)
        throw CityYieldUnavailable("VP city happiness requires VP GameCore profile");
    if(auto *state=world.extensions.find<VpCityHappinessProfileState>())return *state;
    return world.extensions.emplace<VpCityHappinessProfileState>();
}
inline const VpCityHappinessProfileState *find_vp_city_happiness_profile(const CityYieldContext &world){
    return world.profile==content::TechnologyProfile::vp542
        ?world.extensions.find<VpCityHappinessProfileState>():nullptr;
}
inline const VpCityHappinessProfileState::Player &vp_city_happiness_player(const CityYieldContext &world,std::int64_t owner){
    const auto *state=find_vp_city_happiness_profile(world);if(!state)throw CityYieldUnavailable("Unresolved VP player happiness");
    const auto found=state->players.find(owner);if(found==state->players.end())throw CityYieldUnavailable("Unresolved VP player happiness");return found->second;
}
inline VpCityHappinessProfileState::Player &vp_city_happiness_player(CityYieldContext &world,std::int64_t owner){
    auto &state=ensure_vp_city_happiness_profile(world);const auto found=state.players.find(owner);
    if(found==state.players.end())throw CityYieldUnavailable("Unresolved VP player happiness");return found->second;
}
inline const VpCityHappinessProfileState::City &vp_city_happiness_city(const CityYieldContext &world,std::int64_t owner,std::int64_t city){
    const auto *state=find_vp_city_happiness_profile(world);if(!state)throw CityYieldUnavailable("Unresolved VP city happiness");
    const auto found=state->cities.find({owner,city});if(found==state->cities.end())throw CityYieldUnavailable("Unresolved VP city happiness");return found->second;
}
inline VpCityHappinessProfileState::City &vp_city_happiness_city(CityYieldContext &world,std::int64_t owner,std::int64_t city){
    auto &state=ensure_vp_city_happiness_profile(world);const auto found=state.cities.find({owner,city});
    if(found==state.cities.end())throw CityYieldUnavailable("Unresolved VP city happiness");return found->second;
}
inline VpCityHappinessProfileState::TradeStats &vp_city_happiness_trade_stats(CityYieldContext &world,std::int64_t owner){
    auto &state=ensure_vp_city_happiness_profile(world);const auto found=state.trade_stats.find(owner);
    if(found==state.trade_stats.end())throw CityYieldUnavailable("Unresolved VP trade statistics");return found->second;
}
inline const VpCityHappinessProfileState::TradeStats &vp_city_happiness_trade_stats(const CityYieldContext &world,std::int64_t owner){
    const auto *state=find_vp_city_happiness_profile(world);if(!state)throw CityYieldUnavailable("Unresolved VP trade statistics");
    const auto found=state->trade_stats.find(owner);if(found==state->trade_stats.end())throw CityYieldUnavailable("Unresolved VP trade statistics");return found->second;
}
} // namespace civ5::runtime
