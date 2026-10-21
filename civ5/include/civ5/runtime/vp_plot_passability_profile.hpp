#pragma once
#include "civ5/runtime/lua_map_services.hpp"

namespace civ5::runtime {
// VP's per-team plot flags belong to its GameCore profile. Explicit false
// entries distinguish a known passable team from an unresolved saved team ID.
// Sparse wide IDs do not recreate the source GameCore's fixed team array.
struct VpPlotPassabilityProfileState {
    std::map<std::uint64_t,std::map<std::int64_t,bool>> teams_by_plot;
};

// Parse only VP's saved GameCore fields. The common importer owns plot identity
// and the shared BNW reconstruction path.
void restore_vp_plot_passability_flags(PlotPassability &state,std::uint64_t plot,
    const saves::PlotObservation &saved,std::uint64_t team_slots);

inline std::map<std::int64_t,bool> &ensure_vp_plot_team_flags(PlotPassability &state,
                                                              std::uint64_t plot) {
    if(state.layout!=saves::GameStateLayout::vp542_corpus||plot>=state.plots.size())
        throw ScriptError("VP plot team flags require a valid VP plot");
    auto *profile=state.extensions.find<VpPlotPassabilityProfileState>();
    if(!profile)profile=&state.extensions.emplace<VpPlotPassabilityProfileState>();
    return profile->teams_by_plot[plot];
}

inline bool vp_plot_team_impassable(const PlotPassability &state,std::int64_t plot,
                                   std::int64_t team) {
    if(state.layout!=saves::GameStateLayout::vp542_corpus||plot<0||
       static_cast<std::uint64_t>(plot)>=state.plots.size())
        throw ScriptError("VP plot team flags require a valid VP plot");
    const auto *profile=state.extensions.find<VpPlotPassabilityProfileState>();
    if(!profile)throw ScriptError("Unresolved VP plot team flags");
    return profile->teams_by_plot.at(static_cast<std::uint64_t>(plot)).at(team);
}
} // namespace civ5::runtime
