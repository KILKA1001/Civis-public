#include "civ5/runtime/bnw_plot_type_transition.hpp"

namespace civ5::runtime {

BnwFreshPlotTransition inspect_bnw_fresh_plot_transition(
    std::int32_t old_plot_type, std::int32_t old_terrain_type,
    std::int32_t new_plot_type, bool recalculate, bool rebuild_graphics) {
    // CvPlot::reset in Expansion2 creates PLOT_OCEAN / NO_TERRAIN. The stock
    // MapGenerator.SetPlotTypes loop uses (false, false), without the area or
    // graphics paths. Any later mutation needs the full live-world setter.
    if (old_plot_type != 3 || old_terrain_type != -1 ||
        new_plot_type < 0 || new_plot_type > 3 || recalculate || rebuild_graphics)
        return {BnwFreshPlotTransitionStatus::outside_fresh_setup_contract, {},
                "CvPlot::setPlotType requires the full live-world transition"};
    if (new_plot_type == old_plot_type)
        return {BnwFreshPlotTransitionStatus::no_change, {}, {}};

    // Expansion2 CvPlot.cpp:5238-5319. erase() is mandatory because the old
    // category is ocean, even when bEraseUnitsIfWater is false. All three
    // target categories are land; the fresh terrain is NO_TERRAIN, so the
    // selected LAND_TERRAIN must be assigned through setTerrainType. That
    // nested setter also updates yield and impassability before the outer
    // setter resets pathfinding and updates surrounding plots.
    return {BnwFreshPlotTransitionStatus::requires_world_effects,
            {BnwFreshPlotEffect::erase,
             BnwFreshPlotEffect::remove_sight,
             BnwFreshPlotEffect::assign_plot_type,
             BnwFreshPlotEffect::update_plot_yield,
             BnwFreshPlotEffect::restore_sight,
             BnwFreshPlotEffect::assign_land_terrain,
             BnwFreshPlotEffect::update_terrain_yield,
             BnwFreshPlotEffect::update_impassability,
             BnwFreshPlotEffect::reset_step_finder,
             BnwFreshPlotEffect::update_neighbor_yields,
             BnwFreshPlotEffect::update_potential_city_work,
             BnwFreshPlotEffect::increment_land_plot_count},
            "CvPlot::erase and live plot side-effect services are unresolved"};
}

} // namespace civ5::runtime
