#pragma once

#include "civ5/runtime/bnw_setup_plot_grid.hpp"
#include "civ5/content/rules_database.hpp"
#include <array>
#include <cstdint>
#include <map>
#include <span>
#include <vector>

namespace civ5::runtime {

// Expansion2 CvGameCoreEnums.h fixes NUM_YIELD_TYPES at six. These are the
// selected Info values needed by the ownerless, featureless fresh map path.
struct BnwFreshYieldRules {
    struct Yield { std::int32_t hills = 0, lake = 0; };
    struct Terrain {
        std::array<std::int32_t, 6> base{}, hills{}, river{};
    };
    std::array<Yield, 6> yields{};
    std::map<std::int32_t, Terrain> terrains;
    // Non-owning identity for the activated selected rules used by this map
    // generation call. River drafts reject unbound synthetic rule sets.
    const content::RulesDatabase *source_rules = nullptr;
};

struct BnwFreshYieldPassChange {
    std::int32_t trigger_landmass_id = -1;
    std::uint64_t changed_plots = 0;
};
struct BnwFreshYieldProgress {
    std::uint64_t set_area_changed_plots = 0;
    std::vector<BnwFreshYieldPassChange> lake_threshold_passes;
    std::uint64_t final_pass_changed_plots = 0;
    std::uint64_t plot_visits = 0;
};
struct BnwFreshYieldReplay {
    std::vector<std::array<std::int16_t, 6>> final_cache;
    BnwFreshYieldProgress progress;
    std::vector<BnwFreshYieldSymbolEvent> symbol_events;
};

BnwFreshYieldRules load_bnw_fresh_yield_rules(
    const content::RulesDatabase &selected_rules,
    const BnwSetupPlotGrid &grid,
    std::span<const std::int32_t> additional_terrain_ids = {});

// One ownerless fresh plot after its area has been assigned. Used by the
// source-ordered lake neighbor callbacks; the changing lake plot itself has
// area == NULL and therefore does not enter CvPlot::calculateYield yet.
std::array<std::int16_t, 6> calculate_bnw_fresh_plot_yields(
    const BnwSetupPlot &plot, bool lake, const BnwFreshYieldRules &rules);

// Execute the final CvMap::updateYield formula on a complete fresh grid and
// selected area/landmass structure. External updateSymbols remains separate.
std::vector<std::array<std::int16_t, 6>> calculate_bnw_fresh_final_yields(
    const BnwSetupPlotGrid &grid, const BnwFreshAreaStructure &areas,
    const BnwFreshYieldRules &rules);

// Apply fresh cache writes at setArea in A* callback order, then each
// lake-threshold full-map pass and the final map pass in plot-index order.
// Each changed cache stages one ordered updateSymbols notification; delivery
// to the live map host remains unresolved.
BnwFreshYieldReplay replay_bnw_fresh_yield_passes(
    const BnwSetupPlotGrid &grid, const BnwFreshAreaStructure &areas,
    const BnwFreshYieldRules &rules,
    std::uint64_t maximum_plot_visits = 20'000'000,
    std::uint64_t maximum_symbol_events = 2'000'000);

// Recalculate each SDK updateYield call at the river draft's ordered gaps.
// The returned copy owns cache writes and staged updateSymbols notifications;
// no river setter or live world state is committed here.
BnwFreshRiverCrossingDraft replay_bnw_fresh_river_yields(
    const BnwFreshRiverCrossingDraft &crossings,
    const BnwFreshAreaStructure &areas, const BnwFreshYieldRules &rules,
    std::uint64_t maximum_symbol_events = 2'000'000);

// Apply CvArea::changeNumRiverEdges(+1) to the source plot's selected area
// after all river updateYield/updateSymbols callbacks. The returned state is
// still detached: flow direction and persistence remain mandatory.
BnwFreshRiverAreaDraft replay_bnw_fresh_river_area_count(
    const BnwFreshRiverCrossingDraft &yielded,
    const BnwFreshAreaStructure &selected_areas, std::size_t source_plot_index);

// Set the SDK E/SE/SW flow char after the area count in a further detached
// copy. Publishing it still requires an atomic owned-grid/area/yield/symbol
// handoff; post-river query services can read only a published state.
BnwFreshRiverAreaDraft replay_bnw_fresh_river_flow_direction(
    const BnwFreshRiverAreaDraft &counted);

} // namespace civ5::runtime
