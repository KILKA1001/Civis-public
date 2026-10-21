#pragma once
#include "civ5/runtime/maintenance.hpp"

namespace civ5::runtime {
// VP's plot and city yield caches for a unit are not common maintenance state.
struct VpUnitYieldProfileState {
    struct PlotYieldState {
        // Legacy off-map sentinels are translated at import, not host bounds.
        std::optional<std::pair<std::int64_t,std::int64_t>> position;
        std::int64_t base_combat=0;
        bool death_delay=false;
        YieldValues garrison,fortification;
    };
    std::optional<PlotYieldState> plot_yields;
    std::optional<YieldValues> city_yield_change;
};
inline VpUnitYieldProfileState &ensure_vp_unit_yield_profile(
    MaintenanceUnit &unit, content::TechnologyProfile profile) {
    if(profile!=content::TechnologyProfile::vp542)
        throw MaintenanceUnavailable("VP unit yields require VP GameCore profile");
    if(auto *state=unit.profile_extensions.find<VpUnitYieldProfileState>())return *state;
    return unit.profile_extensions.emplace<VpUnitYieldProfileState>();
}
inline VpUnitYieldProfileState &vp_unit_yield_profile(MaintenanceUnit &unit) {
    auto *state=unit.profile_extensions.find<VpUnitYieldProfileState>();
    if(!state)throw MaintenanceUnavailable("Missing VP unit yield profile");
    return *state;
}
inline const VpUnitYieldProfileState &vp_unit_yield_profile(const MaintenanceUnit &unit) {
    const auto *state=unit.profile_extensions.find<VpUnitYieldProfileState>();
    if(!state)throw MaintenanceUnavailable("Missing VP unit yield profile");
    return *state;
}
} // namespace civ5::runtime
