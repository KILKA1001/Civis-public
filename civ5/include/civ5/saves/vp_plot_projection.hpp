#pragma once
#include "civ5/saves/map_state.hpp"

namespace civ5::saves {
struct VpPlotRevealedTypes {
    std::int64_t owner=-1, improvement=-1, route=-1;
    bool operator==(const VpPlotRevealedTypes &) const = default;
};

// VP's team-known numeric types and reset-on-load extra-yield cache belong to
// its GameCore profile, not every Civ V map cell.
struct VpPlotProjectionState {
    std::optional<std::map<std::int64_t, VpPlotRevealedTypes>> revealed_types;
    std::vector<std::pair<std::int64_t, std::int64_t>> extra_yields;
};

inline VpPlotProjectionState &ensure_vp_plot_projection(SavedMapCellProjection &cell) {
    if (auto *state = cell.profile_extensions.find<VpPlotProjectionState>()) return *state;
    return cell.profile_extensions.emplace<VpPlotProjectionState>();
}
inline const VpPlotProjectionState *find_vp_plot_projection(const SavedMapCellProjection &cell) {
    return cell.profile_extensions.find<VpPlotProjectionState>();
}
} // namespace civ5::saves
