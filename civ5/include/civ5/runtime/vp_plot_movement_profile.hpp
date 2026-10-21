#pragma once
#include "civ5/runtime/plot_movement.hpp"

namespace civ5::runtime {
// Unsaved VP updateTeamStatus cache. CvTeam::reset starts with false; the
// selected GameCore module updates it explicitly when team status changes.
struct VpPlotMovementProfileState {
    std::map<std::int64_t, bool> minor_teams;
};

inline bool vp_plot_movement_minor(const PlotMovementState &state, std::int64_t team) {
    (void)state.teams.at(team);
    const auto *profile = state.extensions.find<VpPlotMovementProfileState>();
    if (!profile) return false;
    const auto found = profile->minor_teams.find(team);
    return found != profile->minor_teams.end() && found->second;
}

inline void set_vp_plot_movement_minor(PlotMovementState &state, std::int64_t team, bool minor) {
    (void)state.teams.at(team);
    auto *profile = state.extensions.find<VpPlotMovementProfileState>();
    if (!profile) profile = &state.extensions.emplace<VpPlotMovementProfileState>();
    profile->minor_teams[team] = minor;
}
} // namespace civ5::runtime
