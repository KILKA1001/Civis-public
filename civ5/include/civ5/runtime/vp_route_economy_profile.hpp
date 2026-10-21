#pragma once
#include "civ5/runtime/route_economy.hpp"

namespace civ5::runtime {
// VP alone saves per-player route-responsibility counts. Resource-use totals
// remain common, since both BNW and VP store and mutate them.
struct VpRouteEconomyProfileState {
    std::map<std::int64_t,std::map<std::int64_t,std::int64_t>> responsible_routes;
};
inline VpRouteEconomyProfileState &ensure_vp_route_economy_profile(RouteEconomyState &state){
    if(state.profile!=content::TechnologyProfile::vp542)
        throw std::runtime_error("VP route counters require VP GameCore profile");
    if(auto *profile=state.profile_extensions.find<VpRouteEconomyProfileState>())return *profile;
    return state.profile_extensions.emplace<VpRouteEconomyProfileState>();
}
inline std::map<std::int64_t,std::int64_t> &vp_route_counts(RouteEconomyState &state,std::int64_t player){
    if(state.profile!=content::TechnologyProfile::vp542)
        throw std::runtime_error("VP route counters require VP GameCore profile");
    auto *profile=state.profile_extensions.find<VpRouteEconomyProfileState>();
    if(!profile)throw std::runtime_error("Missing VP route economy profile");
    const auto found=profile->responsible_routes.find(player);
    if(found==profile->responsible_routes.end())throw std::runtime_error("Missing VP player route counters");
    return found->second;
}
inline const std::map<std::int64_t,std::int64_t> &vp_route_counts(const RouteEconomyState &state,std::int64_t player){
    if(state.profile!=content::TechnologyProfile::vp542)
        throw std::runtime_error("VP route counters require VP GameCore profile");
    const auto *profile=state.profile_extensions.find<VpRouteEconomyProfileState>();
    if(!profile)throw std::runtime_error("Missing VP route economy profile");
    const auto found=profile->responsible_routes.find(player);
    if(found==profile->responsible_routes.end())throw std::runtime_error("Missing VP player route counters");
    return found->second;
}
} // namespace civ5::runtime
