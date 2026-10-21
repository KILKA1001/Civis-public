#include "civ5/runtime/bnw_setup_plot_grid.hpp"
#include "civ5/runtime/bnw_fresh_terrain.hpp"
#include "civ5/runtime/bnw_plot_type_transition.hpp"
#include <stdexcept>

namespace civ5::runtime {

void BnwSetupPlotGrid::apply_stock_fresh_plot_type(
    std::size_t index, std::int32_t new_plot_type, bool recalculate,
    bool rebuild_graphics, bool erase_units_if_water,
    const BnwSelectedLandTerrain &land_terrain) {
    const auto &old = plots_.at(index);
    const auto plan = inspect_bnw_fresh_plot_transition(
        old.plot_type, old.terrain_type, new_plot_type,
        recalculate, rebuild_graphics);
    if (plan.status == BnwFreshPlotTransitionStatus::outside_fresh_setup_contract ||
        !erase_units_if_water || old.area_id != -1)
        throw std::invalid_argument("Plot.SetPlotType is outside the verified fresh BNW path");
    if (plan.status == BnwFreshPlotTransitionStatus::no_change) {
        plots_[index].plot_type_assigned_by_script = true;
        return;
    }
    if (land_terrain.id < 0 || land_terrain.water)
        throw std::invalid_argument("Selected BNW LAND_TERRAIN is unresolved or water");
    for (const auto neighbor : adjacent(index))
        if (neighbor && plots_[*neighbor].area_id != -1)
            throw std::invalid_argument("Fresh BNW neighbor already has an area");

    // Expansion2 CvPlot::setPlotType with MapGenerator's (false, false):
    // erase() finds no city, units, resources, improvements, routes, features
    // or rivers in CvMap::InitPlots state. updateSeeFromSight(false/true) has
    // no owned plots, units or recon sources. updateYield on this plot and
    // each neighbor returns at its area()==NULL guard. Those exact empty
    // state invariants cease to apply once a live world is introduced.
    auto &plot = plots_[index];
    plot.plot_type = new_plot_type;
    plot.terrain_type_assigned_by_script = false;
    // setTerrainType from NO_TERRAIN: its sight branch does not run, its
    // updateYield also returns, then updateImpassable reads the terrain
    // because the fresh plot has NO_FEATURE. Waterness matches, so there is
    // no recursive setPlotType call.
    plot.terrain_type = land_terrain.id;
    plot.impassable = land_terrain.impassable;
    // CvStepPathFinder::ForceReset sets m_bForceReset=true.
    step_finder_force_reset_ = true;
    // bRecalculate=false skips adjacent coast/area mutations. The six
    // neighbor updateYield calls return with no area. Each plotCity call
    // updates m_bPotentialCityWork and invokes the same early yield return.
    update_potential_city_work_after_land_added(index);
    ++land_plot_count_;
    plot.plot_type_assigned_by_script = true;
    // Fresh plots have NO_RESOURCE and NO_PLAYER; bRecalculate=false and
    // bRebuildGraphics=false skip the remaining area and layout branches.
}

} // namespace civ5::runtime
