#pragma once
#include <cstdint>
#include <string_view>
#include <vector>

namespace civ5::runtime {

// This describes the SDK's CvPlot::setPlotType path for a freshly initialized
// BNW cell (PLOT_OCEAN / NO_TERRAIN) when the stock MapGenerator calls it with
// bRecalculate=false and bRebuildGraphics=false. It does not apply effects.
enum class BnwFreshPlotEffect {
    erase,
    remove_sight,
    assign_plot_type,
    update_plot_yield,
    restore_sight,
    assign_land_terrain,
    update_terrain_yield,
    update_impassability,
    reset_step_finder,
    update_neighbor_yields,
    update_potential_city_work,
    increment_land_plot_count,
};

enum class BnwFreshPlotTransitionStatus {
    no_change,
    requires_world_effects,
    outside_fresh_setup_contract,
};

struct BnwFreshPlotTransition {
    BnwFreshPlotTransitionStatus status;
    // Source order, including the effects nested inside setTerrainType.
    std::vector<BnwFreshPlotEffect> effects;
    // First missing service that prevents applying this plan to setup plots.
    std::string_view blocker;
};

BnwFreshPlotTransition inspect_bnw_fresh_plot_transition(
    std::int32_t old_plot_type, std::int32_t old_terrain_type,
    std::int32_t new_plot_type, bool recalculate, bool rebuild_graphics);

} // namespace civ5::runtime
