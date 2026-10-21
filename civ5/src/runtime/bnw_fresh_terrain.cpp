#include "civ5/runtime/bnw_fresh_terrain.hpp"
#include "civ5/runtime/bnw_setup_plot_grid.hpp"
#include "civ5/content/rules_database.hpp"
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

namespace civ5::runtime {

BnwSelectedTerrain resolve_bnw_selected_terrain_by_id(
    const content::RulesDatabase &rules, std::int32_t id) {
    if (id < 0 || id > std::numeric_limits<std::int8_t>::max())
        throw std::invalid_argument("Fresh BNW terrain ID is outside CvPlot's stored range");
    std::optional<BnwSelectedTerrain> selected;
    const auto report = rules.query_rows(
        {"SELECT ROWID AS CivisRowId, ID, Water, Impassable FROM Terrains "
         "WHERE ID = ? ORDER BY ROWID", {static_cast<std::int64_t>(id)}},
        [&](const content::RulesRowView &row) {
            if (selected || !row.has_column("CivisRowId") || !row.has_column("ID") ||
                !row.has_column("Water") || !row.has_column("Impassable"))
                throw std::runtime_error("Selected BNW terrain row is ambiguous or incomplete");
            if (row.integer("CivisRowId") != id || row.integer("ID") != id)
                throw std::runtime_error("Selected BNW terrain ROWID differs from Info slot");
            selected = BnwSelectedTerrain{id, row.boolean("Water"),
                                          row.boolean("Impassable")};
        }, 2);
    if (!report.all_succeeded() || !selected)
        throw std::runtime_error("Selected BNW terrain ID is unresolved: " + report.error);
    return *selected;
}

BnwSelectedLandTerrain resolve_bnw_selected_land_terrain(
    const content::RulesDatabase &rules) {
    // The selected-content bootstrap has not run the global PostDefines write
    // stage yet. Resolve only this stock source key, read-only: BNW's
    // postprocess writes the matched Terrains ROWID into Defines.
    std::optional<std::string> key;
    const auto source = rules.query_rows(
        {"SELECT Key, \"Table\" AS SourceTable FROM PostDefines "
         "WHERE Name = 'LAND_TERRAIN' ORDER BY ROWID", {}},
        [&](const content::RulesRowView &row) {
            if (key || !row.has_column("Key") || !row.has_column("SourceTable") ||
                row.text("SourceTable") != std::optional<std::string>("Terrains"))
                throw std::runtime_error("Selected BNW LAND_TERRAIN PostDefine is ambiguous");
            key = row.text("Key");
        }, 2);
    if (!source.all_succeeded() || key != std::optional<std::string>("TERRAIN_GRASS"))
        throw std::runtime_error("Selected BNW LAND_TERRAIN PostDefine is unresolved or changed");

    std::optional<BnwSelectedLandTerrain> selected;
    const auto terrain = rules.query_rows(
        {"SELECT ROWID AS CivisRowId, ID, Water, Impassable FROM Terrains "
         "WHERE Type = ?", {*key}},
        [&](const content::RulesRowView &row) {
            if (selected || !row.has_column("CivisRowId") || !row.has_column("ID") ||
                !row.has_column("Water") || !row.has_column("Impassable"))
                throw std::runtime_error("Selected BNW LAND_TERRAIN row is ambiguous or incomplete");
            const auto rowid = row.integer("CivisRowId");
            if (rowid < 0 || rowid > std::numeric_limits<std::int32_t>::max() ||
                row.integer("ID") != rowid)
                throw std::runtime_error("Selected BNW terrain ROWID differs from its Info slot");
            selected = BnwSelectedLandTerrain{static_cast<std::int32_t>(rowid),
                                              row.boolean("Water"), row.boolean("Impassable")};
        }, 2);
    if (!terrain.all_succeeded() || !selected || selected->water)
        throw std::runtime_error("Selected BNW LAND_TERRAIN is missing or water");
    return *selected;
}

void BnwSetupPlotGrid::apply_stock_fresh_terrain_type(
    std::size_t index, const BnwSelectedTerrain &terrain,
    bool recalculate, bool rebuild_graphics) {
    auto &plot = plots_.at(index);
    if (recalculate || rebuild_graphics || plot.area_id != -1 ||
        plot.plot_type < 0 || plot.plot_type > 3 ||
        terrain.id < 0 || terrain.id > std::numeric_limits<std::int8_t>::max() ||
        terrain.water != (plot.plot_type == 3))
        throw std::invalid_argument("Plot.SetTerrainType is outside verified fresh BNW setup");
    if (plot.terrain_type == terrain.id) {
        plot.terrain_type_assigned_by_script = true;
        return;
    }

    // Expansion2 CvPlot::setTerrainType: sight is recalculated only when both
    // old and new terrain are valid and their sight levels differ. Every
    // updateSight visits empty fresh plots: no owner, city, units or recon
    // source can change visibility. updateYield returns at area()==NULL.
    // Matching waterness avoids recursive setPlotType entirely.
    last_terrain_steps_.clear();
    plot.terrain_type = terrain.id;
    last_terrain_steps_.push_back(BnwFreshTerrainStep::assign_terrain);
    last_terrain_steps_.push_back(BnwFreshTerrainStep::yield_skipped_without_area);
    plot.impassable = terrain.impassable; // NO_FEATURE in CvMap::InitPlots
    plot.terrain_type_assigned_by_script = true;
    last_terrain_steps_.push_back(BnwFreshTerrainStep::update_impassability);
    // CvPlot::setTerrainType performs no stepfinder or coast invalidation.
}

BnwSetupPlotGrid BnwSetupPlotGrid::draft_assign_selected_land_terrain(
    std::size_t index, const BnwSelectedLandTerrain &terrain) const {
    const auto &old = plots_.at(index);
    if (old.plot_type < 0 || old.plot_type > 2 || old.terrain_type != -1 ||
        old.area_id != -1 || terrain.id < 0 || terrain.water)
        throw std::invalid_argument("Land terrain assignment is outside fresh BNW setup");
    auto draft = *this;
    // CvPlot::setTerrainType: assignment, updateYield (returns immediately
    // with no area), then updateImpassable. Fresh plots have NO_FEATURE, so
    // updateImpassable reads the selected terrain's Impassable property.
    draft.last_terrain_steps_.clear();
    draft.plots_[index].terrain_type = terrain.id;
    draft.last_terrain_steps_.push_back(BnwFreshTerrainStep::assign_terrain);
    draft.last_terrain_steps_.push_back(BnwFreshTerrainStep::yield_skipped_without_area);
    draft.plots_[index].impassable = terrain.impassable;
    draft.last_terrain_steps_.push_back(BnwFreshTerrainStep::update_impassability);
    return draft;
}

} // namespace civ5::runtime
