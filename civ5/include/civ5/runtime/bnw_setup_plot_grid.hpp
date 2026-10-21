#pragma once
#include "civ5/runtime/bnw_fresh_terrain.hpp"
#include "civ5/runtime/simulation_random.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace civ5::runtime {
enum class BnwFreshTerrainStep { assign_terrain, yield_skipped_without_area, update_impassability };

struct BnwSetupPlot {
    std::int32_t x, y, plot_type = 3, terrain_type = -1;
    bool potential_city_work = false;
    bool impassable = false;
    // CvPlot::reset initializes all three stored river-edge bits to false.
    // Fresh setup owns them even while river writes remain unsupported.
    bool ne_of_river = false, w_of_river = false, nw_of_river = false;
    // CvPlot::reset sets these serialized char FlowDirectionTypes to -1.
    // Their SDK names describe the physical E/SE/SW edge, respectively.
    std::int8_t river_e_flow_direction = -1;
    std::int8_t river_se_flow_direction = -1;
    std::int8_t river_sw_flow_direction = -1;
    // CvPlot::reset clears the six crossing bits and their cached count.
    std::uint8_t river_crossing_mask = 0;
    std::int32_t river_crossing_count = 0;
    // CvMap::InitPlots defaults to ocean. A nil-returning stock map script
    // must still assign every category through SetPlotTypes.
    bool plot_type_assigned_by_script = false;
    bool terrain_type_assigned_by_script = false;
    std::int32_t area_id = -1;
    std::int32_t landmass_id = -1;
    std::array<std::int16_t, 6> yields{};
    bool operator==(const BnwSetupPlot &) const = default;
};

struct BnwFreshArea {
    std::int32_t id = -1;
    bool water = false, mountains = false;
    std::int32_t tiles = 0;
    std::int32_t west = 0, east = 0, south = 0, north = 0;
    // CvArea::reset starts at zero; CvPlot::set*OfRiver changes the source
    // plot's area count after the ordered crossing/yield callbacks.
    std::int32_t river_edges = 0;
};
struct BnwFreshLandmass {
    std::int32_t id = -1;
    bool water = false, lake = false;
    std::int32_t tiles = 0, centroid_x_sum = 0, centroid_y_sum = 0;
    // CvLandmass::changeNumTiles schedules one full-map updateYield when
    // isLake changes while this landmass is being populated.
    std::uint32_t lake_threshold_yield_passes = 0;
};
struct BnwFreshAreaStructure {
    std::vector<std::int32_t> area_ids, landmass_ids;
    // CvAStar ASNL_ADDCLOSED callback order, including the seed's no-op
    // second setArea. Equal-cost nodes enter at the open-list head.
    std::vector<std::size_t> area_assignment_order, landmass_assignment_order;
    std::vector<BnwFreshArea> areas;
    std::vector<BnwFreshLandmass> landmasses;
    std::int32_t lake_max_area_size = 0;
    // Required SDK yield calls, recorded but not executed by this draft.
    std::uint64_t area_assignment_yield_calls = 0;
    std::uint64_t lake_threshold_full_yield_passes = 0;
    bool final_landmass_full_yield_pass_pending = true;
};

// Resolve one selected area handle, rejecting missing or ambiguous IDs.
const BnwFreshArea &find_bnw_fresh_area(
    const BnwFreshAreaStructure &areas, std::int32_t area_id);
class BnwSetupPlotGrid;
struct BnwSelectedFeatureEligibility {
    const content::RulesDatabase *source_rules = nullptr;
    std::int32_t id = -1;
    bool no_coast = false, no_river = false, no_adjacent = false;
    bool requires_flatlands = false, requires_river = false;
    std::vector<std::int32_t> terrain_ids;
};
std::optional<BnwSelectedFeatureEligibility> resolve_bnw_selected_feature_eligibility(
    const content::RulesDatabase &rules, std::int32_t feature_id);
bool can_have_bnw_fresh_feature(
    const BnwSetupPlotGrid &grid, std::size_t plot_index,
    std::int32_t feature_id,
    const std::optional<BnwSelectedFeatureEligibility> &feature,
    const BnwFreshAreaStructure &areas, std::int32_t minimum_water_size,
    std::span<const std::int32_t> feature_types);
// CvMap::findBiggestArea scans FFreeList slots in allocation order and uses
// strict greater-than comparison. Reject a partially assigned owned grid.
std::optional<std::int32_t> find_bnw_fresh_biggest_area(
    const BnwSetupPlotGrid &grid, const BnwFreshAreaStructure &areas,
    bool water);

enum class BnwFreshYieldSymbolPhase : std::uint8_t {
    set_area, lake_threshold, final_map, river_write, lake_conversion,
    feature_mountain_conversion
};
struct BnwFreshYieldSymbolEvent {
    BnwFreshYieldSymbolPhase phase = BnwFreshYieldSymbolPhase::set_area;
    std::uint32_t plot_index = 0;
    std::int32_t trigger_landmass_id = -1;
    // updateSymbols publishes after m_aiYield has changed. Keep that snapshot
    // until a live host can deliver GameplayYieldMightHaveChanged in order.
    std::array<std::int16_t, 6> yields_after{};
};

enum class BnwFreshRiverEdge { north_east, west, north_west };
enum class BnwFreshRiverWriteEffect {
    edge_bit, self_crossings, self_yield,
    neighbor_crossing_then_yield_each_direction, area_river_edge_count,
    flow_direction
};
enum class BnwFreshRiverWriteBlockerCode {
    invalid_plot, unassigned_area, outside_fresh_stock_contract,
    required_services_pending
};
struct BnwFreshRiverWritePreflight {
    BnwFreshRiverWriteBlockerCode blocker;
    // CvPlot::setNEOfRiver/setWOfRiver/setNWOfRiver source order. The neighbor
    // stage repeats in DirectionTypes order, including wrapped duplicates.
    std::array<BnwFreshRiverWriteEffect, 6> effects;
    std::vector<std::string_view> missing_services;
};

enum class BnwFreshRiverDraftEventKind {
    edge_bit_written, crossing_checked, yield_pending,
    yield_recalculated, symbols_staged,
    area_river_edge_count_pending, area_river_edge_count_applied,
    flow_direction_pending, flow_direction_applied
};
struct BnwFreshRiverDraftEvent {
    BnwFreshRiverDraftEventKind kind;
    std::size_t plot_index;
    std::int8_t direction = -1;
    bool crossing_after = false;
    bool operator==(const BnwFreshRiverDraftEvent &) const = default;
};
struct BnwFreshRiverCrossingDraft;
struct BnwFreshRiverAreaDraft;
struct BnwFreshAreaUnassignmentDraft;
struct BnwFreshLakePlotTypePrefixDraft;
struct BnwFreshLakeTerrainDraft;
struct BnwFreshLakeNeighborDraft;
struct BnwFreshLakeCityAreaDraft;
struct BnwFreshLakeAreaDraft;
struct BnwFreshLakePostSetterDraft;
struct BnwFreshFeatureHillDraft;
struct BnwFreshYieldRules;

// Owned CvMap::InitPlots slice. Its draft mutation is deliberately separate
// from the Lua plot handles: a draft lacks sight, complete yields and areas.
class BnwSetupPlotGrid {
public:
    void reset(std::int32_t width, std::int32_t height, bool wrap_x, bool wrap_y);
    std::size_t size() const noexcept { return plots_.size(); }
    std::int64_t land_plot_count() const noexcept { return land_plot_count_; }
    bool step_finder_force_reset() const noexcept { return step_finder_force_reset_; }
    const BnwSetupPlot &at(std::size_t index) const { return plots_.at(index); }
    const std::vector<BnwFreshYieldSymbolEvent> &symbol_events() const noexcept {
        return symbol_events_;
    }
    const std::vector<BnwFreshTerrainStep> &last_terrain_steps() const noexcept {
        return last_terrain_steps_;
    }

    // Six entries in SDK DirectionTypes order. Entries may repeat on tiny
    // wrapped maps, just as six plotDirection calls do in CvPlot::setPlotType.
    std::array<std::optional<std::size_t>, 6> adjacent(std::size_t index) const;
    // Run only the edge-bit and updateRiverCrossing prefix in a copy, while
    // retaining exact yield/area/flow gaps as ordered pending events.
    BnwFreshRiverCrossingDraft draft_fresh_river_crossings(
        std::size_t index, BnwFreshRiverEdge edge,
        std::int32_t flow_direction) const;
    // CvPlot::isAdjacentToLand checks all six plotDirection neighbors against
    // current plot categories. Wrapped tiny maps may refer back to this plot.
    bool is_adjacent_to_land(std::size_t index) const;
    bool is_adjacent_to_shallow_water(std::size_t index,
                                      std::int32_t shallow_terrain_id) const;
    // Expansion2 CvPlot::isCoastalLand, using already assigned landmass IDs.
    bool is_coastal_land(std::size_t index, std::int32_t minimum_water_size,
                         const BnwFreshAreaStructure &areas) const;
    // Expansion2 CvMap::findWater during fresh AddRivers. With no features
    // yet, isFreshWater reads the river-crossing count then nearby lakes.
    bool find_water_during_rivers(std::size_t index, std::int32_t range,
                                  bool fresh_water,
                                  const BnwFreshAreaStructure &areas,
                                  std::uint64_t &remaining_visits) const;
    // CvPlot::getInlandCorner candidates in shuffleArray source order:
    // self, NORTH (alias NORTHEAST), NORTHWEST, WEST. Water at a candidate's
    // SE corner invalidates it; the caller consumes four map RNG draws.
    std::array<std::optional<std::size_t>, 4> inland_corner_candidates(
        std::size_t index) const;
    std::optional<std::size_t> get_inland_corner(
        std::size_t index, SimulationRandom &map_random) const;
    // Source-order structural result of CvMap::recalculateAreas. The native
    // setter's yield effects are not applied, so this remains a draft.
    BnwFreshAreaStructure draft_fresh_area_structure(
        std::int32_t lake_max_area_size) const;
    // Apply validated fresh handles in CvMap area-then-landmass callback order.
    // Yield cache values come from the separately verified replay.
    void apply_fresh_area_handles(
        const BnwFreshAreaStructure &areas,
        const std::vector<std::array<std::int16_t, 6>> &final_yields,
        std::vector<BnwFreshYieldSymbolEvent> symbol_events);
    // SDK plotCity offsets for NUM_CITY_PLOTS (37), retaining out-of-map
    // entries and duplicates on small wrapped maps in native call order.
    std::array<std::optional<std::size_t>, 37> city_plots(std::size_t index) const;
    // updatePotentialCityWork invokes updateYield whenever this bit changes.
    // Before area assignment those calls return; this records source call order.
    const std::vector<std::size_t> &city_work_yield_calls() const noexcept {
        return city_work_yield_calls_;
    }

    // Apply only selected owned state changes to a copy.
    // The caller cannot commit this as a live plot transition until all other
    // effects in inspect_bnw_fresh_plot_transition are implemented.
    // SDK SetPlotTypes path only; no live entities or assigned areas exist.
    void apply_stock_fresh_terrain_type(std::size_t index,
                                        const BnwSelectedTerrain &terrain,
                                        bool recalculate, bool rebuild_graphics);
    void apply_stock_fresh_plot_type(std::size_t index, std::int32_t new_plot_type,
                                     bool recalculate, bool rebuild_graphics,
                                     bool erase_units_if_water,
                                     const BnwSelectedLandTerrain &land_terrain);
    BnwSetupPlotGrid draft_assign_selected_land_terrain(
        std::size_t index, const BnwSelectedLandTerrain &terrain) const;
    BnwSetupPlotGrid draft_fresh_category_change(std::size_t index,
                                                 std::int32_t new_plot_type) const;
    // CvPlot::setArea(FFreeList::INVALID_INDEX) during fresh AddLakes only.
    // Detached until the enclosing Lua phase has committed every later effect.
    BnwFreshAreaUnassignmentDraft draft_fresh_area_unassignment(
        std::size_t index, const BnwFreshAreaStructure &areas) const;
    // Prefix only of CvPlot::setPlotType(PLOT_OCEAN,true,true,true) after
    // SetArea(-1). Terrain, neighbors, city work and later effects remain
    // unresolved, so the result must never be published as a lake.
    BnwFreshLakePlotTypePrefixDraft draft_fresh_lake_plot_type_prefix(
        std::size_t index, const BnwFreshAreaStructure &areas,
        const BnwSelectedTerrain &current_terrain) const;
    // Continue a detached lake prefix through the selected water terrain
    // and coast cache, stopping before CvStepFinder::ForceReset.
    static BnwFreshLakeTerrainDraft advance_fresh_lake_terrain(
        BnwFreshLakePlotTypePrefixDraft prefix, std::size_t index,
        const BnwSelectedTerrain &selected_water_terrain);
    // Continue on a detached owned map through ForceReset, then the two
    // DirectionTypes neighbor loops. City work and area effects remain blocked.
    static BnwFreshLakeNeighborDraft advance_fresh_lake_neighbors(
        BnwFreshLakeTerrainDraft terrain, std::size_t index,
        const BnwFreshAreaStructure &areas, const BnwFreshYieldRules &rules,
        const BnwSelectedTerrain &shallow_terrain,
        const BnwSelectedTerrain &deep_terrain,
        std::uint64_t maximum_symbol_events);
    // CvPlot::setPlotType next visits all 37 city slots, decrements the map
    // land count, then selects an area path. Keep the path as a preflight:
    // neither CvMap::recalculateAreas nor incremental setArea is applied.
    static BnwFreshLakeCityAreaDraft advance_fresh_lake_city_area_preflight(
        BnwFreshLakeNeighborDraft neighbors, std::size_t index,
        const BnwFreshAreaStructure &areas, const BnwFreshYieldRules &rules,
        std::uint64_t maximum_symbol_events);
    // The source-selected non-recalculate branch only. Performs the no-op
    // SetArea(-1), FFreeList addArea when needed, setArea/processArea(+1),
    // and updateYield on a detached copy. Full Map.RecalculateAreas is still
    // a typed blocker when the preflight selects it.
    static BnwFreshLakeAreaDraft advance_fresh_lake_incremental_area(
        BnwFreshLakeCityAreaDraft preflight, std::size_t index,
        BnwFreshAreaStructure areas, const BnwFreshYieldRules &rules,
        std::uint64_t maximum_symbol_events);
    // Validate the SDK's deliberately unchanged landmass handles and the
    // final graphics guard. A true graphics guard stages the required layout
    // callback; unknown state stays blocked. No draft is Lua-published here.
    static BnwFreshLakePostSetterDraft inspect_fresh_lake_post_setter(
        BnwFreshLakeAreaDraft area, const BnwSetupPlotGrid &before,
        const BnwFreshAreaStructure &before_areas, std::size_t index,
        std::optional<bool> graphics_initialized);
    // CvPlot::setPlotType(PLOT_HILLS,false,true,true) after AddLakes and
    // before features. This exact non-water transition leaves area and
    // landmass handles stale until the later Map.RecalculateAreas.
    BnwFreshFeatureHillDraft draft_fresh_feature_mountain_to_hill(
        std::size_t index, const BnwFreshAreaStructure &areas,
        const BnwFreshYieldRules &rules, std::optional<bool> graphics_initialized,
        std::uint64_t maximum_symbol_events) const;

private:
    friend BnwFreshRiverCrossingDraft replay_bnw_fresh_river_yields(
        const BnwFreshRiverCrossingDraft &, const BnwFreshAreaStructure &,
        const BnwFreshYieldRules &, std::uint64_t);
    friend BnwFreshRiverAreaDraft replay_bnw_fresh_river_flow_direction(
        const BnwFreshRiverAreaDraft &);
    std::int32_t width_ = 0, height_ = 0;
    bool wrap_x_ = false, wrap_y_ = false;
    std::int64_t land_plot_count_ = 0;
    bool step_finder_force_reset_ = false;
    std::vector<BnwSetupPlot> plots_;
    std::vector<BnwFreshTerrainStep> last_terrain_steps_;
    std::vector<std::size_t> city_work_yield_calls_;
    std::vector<BnwFreshYieldSymbolEvent> symbol_events_;
    std::optional<std::size_t> hex_offset(std::size_t index,
                                           std::int32_t dx,
                                           std::int32_t dy) const;
    void update_potential_city_work_after_land_added(std::size_t index);
};

struct BnwFreshRiverCrossingDraft {
    BnwSetupPlotGrid grid;
    std::vector<BnwFreshRiverDraftEvent> source_order;
    std::vector<std::string_view> remaining_services;
    BnwFreshRiverEdge edge = BnwFreshRiverEdge::north_east;
    std::int32_t requested_flow_direction = -1;
};

// Detached result of CvArea::changeNumRiverEdges. Neither the grid nor the
// area structure in the selected map is modified by this phase.
struct BnwFreshRiverAreaDraft {
    BnwFreshRiverCrossingDraft river;
    BnwFreshAreaStructure areas;
};

enum class BnwFreshAreaUnassignmentEventKind {
    old_lake_read, old_area_tiles_decremented,
    old_area_river_edge_decremented, area_handle_cleared
};
struct BnwFreshAreaUnassignmentEvent {
    BnwFreshAreaUnassignmentEventKind kind;
    // River decrements follow CvPlot::processArea: NE, NW, W.
    std::optional<BnwFreshRiverEdge> river_edge;
    bool old_lake = false;
    bool operator==(const BnwFreshAreaUnassignmentEvent &) const = default;
};
struct BnwFreshAreaUnassignmentDraft {
    BnwSetupPlotGrid grid;
    BnwFreshAreaStructure areas;
    std::vector<BnwFreshAreaUnassignmentEvent> source_order;
};

enum class BnwFreshLakePlotTypePrefixEventKind {
    erase_empty_fresh_plot, old_water_read, sight_removed_empty,
    plot_type_written, yield_skipped_without_area, sight_added_empty,
    terrain_conversion_required, adjacent_land_read,
    selected_water_terrain_written,
    terrain_yield_skipped_without_area, terrain_impassability_updated,
    terrain_recursion_skipped_matching_water, coast_cache_written,
    step_finder_reset, neighbor_terrain_pass, neighbor_yield_pass,
    city_work_pass, land_plot_count_decremented, area_preflight_complete,
    incremental_area_applied, graphics_guard_checked
};
struct BnwFreshLakePlotTypePrefixDraft {
    BnwSetupPlotGrid grid;
    std::vector<BnwFreshLakePlotTypePrefixEventKind> source_order;
    std::vector<std::string_view> remaining_services;
};
struct BnwFreshLakeTerrainDraft {
    BnwSetupPlotGrid grid;
    // Staged CvPlot::m_bIsAdjacentToLand value; it is committed only with
    // the later complete setter and map handoff.
    bool shallow_water = false;
    std::vector<BnwFreshLakePlotTypePrefixEventKind> source_order;
    std::vector<std::string_view> remaining_services;
};
enum class BnwFreshLakeNeighborEventKind {
    terrain_written, terrain_yield_checked, terrain_symbols_staged,
    terrain_impassability_updated, coast_cache_written,
    yield_checked, symbols_staged
};
struct BnwFreshLakeNeighborEvent {
    BnwFreshLakeNeighborEventKind kind;
    std::size_t plot_index;
    std::uint8_t direction;
    bool operator==(const BnwFreshLakeNeighborEvent &) const = default;
};
struct BnwFreshLakeNeighborDraft {
    BnwSetupPlotGrid grid;
    bool shallow_water = false;
    std::vector<BnwFreshLakePlotTypePrefixEventKind> source_order;
    std::vector<BnwFreshLakeNeighborEvent> neighbor_order;
    std::vector<std::string_view> remaining_services;
};
enum class BnwFreshLakeCityEventKind {
    city_plot_checked, potential_city_work_written,
    yield_skipped_without_area, yield_checked, symbols_staged
};
struct BnwFreshLakeCityEvent {
    BnwFreshLakeCityEventKind kind;
    std::uint8_t city_slot;
    std::optional<std::size_t> plot_index;
    bool operator==(const BnwFreshLakeCityEvent &) const = default;
};
struct BnwFreshLakeCityAreaDraft {
    BnwSetupPlotGrid grid;
    bool shallow_water = false;
    std::vector<BnwFreshLakePlotTypePrefixEventKind> source_order;
    std::vector<BnwFreshLakeNeighborEvent> neighbor_order;
    std::vector<BnwFreshLakeCityEvent> city_order;
    // CvPlot::setPlotType's first same-water area and cyclic transition count.
    std::optional<std::int32_t> adjacent_water_area_id;
    std::int32_t adjacent_area_transitions = 0;
    bool recalculate_areas = false;
    std::vector<std::string_view> remaining_services;
};
enum class BnwFreshLakeAreaEventKind {
    invalid_set_area_noop, old_area_delete_skipped,
    neighboring_area_selected, new_area_allocated, new_area_initialized,
    old_lake_read, old_process_skipped, area_handle_written,
    area_tiles_incremented, yield_checked, symbols_staged
};
struct BnwFreshLakeAreaEvent {
    BnwFreshLakeAreaEventKind kind;
    std::int32_t area_id = -1;
    bool operator==(const BnwFreshLakeAreaEvent &) const = default;
};
struct BnwFreshLakeAreaDraft {
    BnwSetupPlotGrid grid;
    BnwFreshAreaStructure areas;
    std::vector<BnwFreshLakePlotTypePrefixEventKind> source_order;
    std::vector<BnwFreshLakeAreaEvent> area_order;
    std::vector<std::string_view> remaining_services;
};
enum class BnwFreshLakePostSetterEventKind {
    landmass_handle_preserved, landmass_pool_preserved,
    graphics_state_unresolved, graphics_guard_false,
    graphics_layout_dirty_pending
};
struct BnwFreshLakePostSetterDraft {
    BnwSetupPlotGrid grid;
    BnwFreshAreaStructure areas;
    std::vector<BnwFreshLakePlotTypePrefixEventKind> source_order;
    std::vector<BnwFreshLakePostSetterEventKind> post_order;
    std::vector<std::string_view> remaining_services;
};
enum class BnwFreshFeatureHillEventKind {
    empty_erase_skipped, sight_removed_empty, plot_type_written,
    yield_checked, symbols_staged, sight_restored_empty,
    terrain_preserved, step_finder_reset, water_branch_skipped,
    graphics_guard_false
};
struct BnwFreshFeatureHillDraft {
    BnwSetupPlotGrid grid;
    std::vector<BnwFreshFeatureHillEventKind> source_order;
};

// Inspect a fresh AddRivers write without changing any owned plot. A full
// transaction must supply every returned service before exposing the setter.
BnwFreshRiverWritePreflight inspect_bnw_fresh_river_write(
    const BnwSetupPlotGrid &grid, std::size_t index, BnwFreshRiverEdge edge,
    bool enabled, std::int32_t flow_direction);

} // namespace civ5::runtime
