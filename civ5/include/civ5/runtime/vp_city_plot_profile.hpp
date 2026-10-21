#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
struct VpCityPlotYieldState {
    std::int64_t strength=0,garrison=-1;
    // Unsaved VP reset value; overrides are explicit live changes.
    std::int64_t garrison_override=-1;
    YieldValues per_strength_times100;
};
struct VpCityPlotBonusState {
    YieldValues sea,river,lake,sea_resource,connection,luxury;
    // yield -> original ordered key/value vector. Do not merge duplicates.
    YieldModifierRows terrain,feature,improvement,resource,plot;
    YieldModifierRows event_terrain,event_feature,event_improvement,event_resource;
};
struct VpCityPlotProfileState {
    struct City {std::optional<VpCityPlotYieldState> yields;std::optional<VpCityPlotBonusState> bonuses;};
    std::map<std::pair<std::int64_t,std::int64_t>,City> cities;
};
inline VpCityPlotProfileState &ensure_vp_city_plot_profile(CityYieldContext &world){
    if(world.profile!=content::TechnologyProfile::vp542)throw CityYieldUnavailable("VP city plot state requires VP GameCore profile");
    if(auto *state=world.extensions.find<VpCityPlotProfileState>())return *state;
    return world.extensions.emplace<VpCityPlotProfileState>();
}
inline const VpCityPlotProfileState *find_vp_city_plot_profile(const CityYieldContext &world){
    return world.profile==content::TechnologyProfile::vp542?world.extensions.find<VpCityPlotProfileState>():nullptr;
}
inline VpCityPlotProfileState::City &vp_city_plot_profile_city(CityYieldContext &world,std::int64_t owner,std::int64_t city){
    auto *state=world.extensions.find<VpCityPlotProfileState>();
    if(world.profile!=content::TechnologyProfile::vp542||!state)throw CityYieldUnavailable("Unresolved VP city plot state");
    const auto found=state->cities.find({owner,city});if(found==state->cities.end())throw CityYieldUnavailable("Unresolved VP city plot state");
    return found->second;
}
inline const VpCityPlotProfileState::City &vp_city_plot_profile_city(const CityYieldContext &world,std::int64_t owner,std::int64_t city){
    const auto *state=find_vp_city_plot_profile(world);
    if(!state)throw CityYieldUnavailable("Unresolved VP city plot state");
    const auto found=state->cities.find({owner,city});if(found==state->cities.end())throw CityYieldUnavailable("Unresolved VP city plot state");
    return found->second;
}
inline VpCityPlotProfileState::City &ensure_vp_city_plot_profile_city(CityYieldContext &world,std::int64_t owner,std::int64_t city){
    return ensure_vp_city_plot_profile(world).cities[{owner,city}];
}
} // namespace civ5::runtime
