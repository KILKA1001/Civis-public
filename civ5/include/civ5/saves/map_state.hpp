#pragma once
#include "civ5/saves/game_state.hpp"
#include "civ5/runtime/profile_extensions.hpp"
#include <functional>
#include <map>
#include <set>

namespace civ5::saves {
struct PregameObservation;
// Shared logical grid; source wire coordinates and graphics buffers never
// determine the capacity of this layout. Cell (x,y) has index y*width+x.
struct MapGrid {
    std::uint64_t width = 0, height = 0;
    std::uint64_t size() const;
    std::uint64_t index(std::uint64_t x, std::uint64_t y) const;
    std::array<std::uint64_t, 2> coordinates(std::uint64_t index) const;
};
struct MapResourceCount {
    std::optional<std::uint64_t> type_hash;
    std::optional<std::int64_t> count;
};
struct MapHeaderObservation {
    GameStateLayout layout;
    std::optional<std::uint64_t> version;
    std::uint64_t start_offset = 0, resources_offset = 0, end_offset = 0;
    MapGrid grid;
    std::int64_t land_plots = 0, owned_plots = 0, natural_wonders = 0, top_latitude = 0, bottom_latitude = 0;
    bool wrap_x = false, wrap_y = false;
    std::array<std::uint8_t, 16> guid{};
    std::optional<bool> map_generated;
    std::array<std::vector<MapResourceCount>, 2> resources;
};
MapHeaderObservation observe_map_header(runtime::BinaryReader &input, GameStateLayout layout,
                                        std::optional<std::uint64_t> vp_resource_slots = {},
                                        GameStateBudget budget = {});
struct PlotWireDimensions {
    std::uint64_t players = 0, teams = 0, major_civilizations = 0, yields = 0, invisible_types = 0;
};
struct PlotArrayObservation {
    const char *name;
    std::uint64_t columns = 1;
    std::vector<std::int64_t> values;
};
struct PlotIndexedValues {
    std::int64_t key = 0;
    std::uint64_t wire_count = 0;
    std::vector<std::int64_t> values;
};
struct PlotObservation {
    std::uint64_t index = 0, x = 0, y = 0, start_offset = 0, end_offset = 0;
    std::optional<std::uint64_t> version;
    // All serialized scalars, including legacy source x/y, raw flags, hashes
    // and unknown IDs. Names distinguish hashes from unresolved numeric IDs.
    std::vector<NamedSaveInteger> fields;
    std::vector<PlotArrayObservation> arrays;
    std::optional<std::vector<std::uint8_t>> script_data;
    std::vector<MapResourceCount> legacy_build_progress;
    std::vector<PlotIndexedValues> invisible_visibility;
};
struct MapPlotsObservation {
    GameStateLayout layout;
    PlotWireDimensions dimensions;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::vector<PlotObservation> plots;
};
// Active-team values read from the save, before content hash resolution and
// before the game's visibility, tech, policy and debug-mode load hooks.
struct SavedPlotTeamArtObservation {
    std::uint64_t plot_index = 0;
    std::int64_t visibility_count = 0;
    // Both engines address reveal bits by team; VP allocates player-slot capacity.
    std::optional<bool> revealed;
    std::optional<std::int64_t> revealed_improvement;
    std::optional<std::uint64_t> revealed_improvement_hash;
    std::int64_t revealed_route = -1;
    bool operator==(const SavedPlotTeamArtObservation &) const = default;
};
SavedPlotTeamArtObservation observe_saved_plot_team_art(
    const PlotObservation &plot, GameStateLayout layout, std::uint64_t active_team);
struct SavedMapTeamArtObservation {
    std::uint64_t active_player = 0, active_team = 0;
    std::vector<SavedPlotTeamArtObservation> plots;
};
// Join the saved setup's active player/team with every serialized map plot.
// This still contains raw art inputs, not resolved runtime graphics state.
SavedMapTeamArtObservation observe_saved_map_team_art(
    const PregameObservation &setup, const MapPlotsObservation &map);
struct SavedRiverPlot {
    bool is_river = false, west = false, northwest = false, northeast = false;
    std::int32_t flow_e = -1, flow_se = -1, flow_sw = -1;
    bool operator==(const SavedRiverPlot &) const = default;
};
struct SavedTerrainAuxiliaryPlot {
    std::int32_t terrain, plot, continent, feature;
    bool operator==(const SavedTerrainAuxiliaryPlot &) const = default;
};
struct SavedRoutePlot {
    std::int64_t type=-1, responsible=-1;
    // BNW does not serialize the VP builder field.
    std::optional<std::int64_t> builder;
    bool pillaged=false;
    bool operator==(const SavedRoutePlot &) const = default;
};
struct PlotWaterCache {
    bool lake=false, fresh=false, adjacent_land=false, adjacent_water=false;
    bool operator==(const PlotWaterCache &) const = default;
};
struct SavedPlotCityReference {
    std::int64_t owner=-1,id=-1;
    bool operator==(const SavedPlotCityReference &) const = default;
};
struct SavedPlotUnitReference {
    std::int64_t owner=-1,id=-1;
    bool operator==(const SavedPlotUnitReference &) const = default;
};
struct SavedMapCellProjection {
    std::uint64_t index = 0, x = 0, y = 0;
    std::optional<std::int64_t> owner, plot_type, terrain_type, feature_type, resource_type,
        improvement_type, continent_type;
    std::optional<std::uint64_t> feature_type_hash, resource_type_hash, improvement_type_hash;
    bool is_city = false;
    // Final logical-cell mask/category after terrain, relief and feature rules.
    // Without feature metadata the base mask is retained, but is not final.
    std::uint32_t native_terrain_mask = 0;
    bool native_terrain_mask_known = false;
    std::optional<std::uint8_t> native_category;
    // Original accessors: crossing count > 0 gates the three owned sides.
    // Missing serialized inputs remain unresolved, not an invented dry plot.
    std::optional<SavedRiverPlot> rivers;
    // Original auxiliary predicates use the resolved numeric feature ID,
    // terrain/plot values and raw continent byte, before mask overrides.
    std::optional<SavedTerrainAuxiliaryPlot> auxiliary;
    // Shared logical route state. Missing wire fields remain unresolved.
    std::optional<SavedRoutePlot> route;
    std::optional<std::int64_t> landmass;
    std::optional<bool> improvement_pillaged, improvement_passable;
    // VP explicitly does not serialize these caches. The runtime initializes
    // and updates them in source order on this shared logical map.
    std::optional<PlotWaterCache> water_cache;
    // Sparse live team flags. Absence means the source field was not decoded.
    std::optional<std::set<std::int64_t>> force_resource_reveal;
    std::optional<std::int64_t> artifact_era,trade_route_units,river_crossing_bits;
    // BNW working city / VP owning city; a decoded NO_PLAYER is known absence.
    std::optional<SavedPlotCityReference> yield_city,plot_city,yield_city_override;
    // Ordered live handles, including stale/duplicate entries from the save.
    std::optional<std::vector<SavedPlotUnitReference>> units;
    std::optional<std::set<std::int64_t>> revealed_teams;
    std::optional<bool> potential_city_work;
    std::optional<std::map<std::int64_t,std::int64_t>> cached_yields;
    // GameCore-specific logical state is owned by its selected profile.
    runtime::ProfileExtensions profile_extensions;
};
struct SavedTerrainFeature {
    bool natural_wonder = false;
    std::string type;
    std::optional<std::int64_t> id = {};
};
// Resolve against the active saved-content profile, by numeric ID or hash as
// present in the cell. nullopt means unresolved. A known missing definition
// may return an empty non-wonder value, matching the original null-info branch.
using SavedTerrainFeatureResolver =
    std::function<std::optional<SavedTerrainFeature>(const SavedMapCellProjection &)>;
struct SavedMapProjection {
    MapGrid grid;
    bool wrap_x = false, wrap_y = false;
    std::vector<SavedMapCellProjection> cells;
};
struct SavedTerrainCellSource {
    std::uint32_t native_terrain_mask = 0;
    std::optional<std::uint8_t> native_category;
    bool native_terrain_mask_known = false;
    std::optional<SavedRiverPlot> rivers;
    std::optional<SavedTerrainAuxiliaryPlot> auxiliary;
};
struct SavedMapTerrainSource {
    MapGrid grid;
    bool wrap_x = false, wrap_y = false;
    // Graphics-facing x-major order: x*height+y. This is a typed source
    // projection, not the eight-byte native markup buffer consumed by the
    // terrain occurrence selector.
    std::vector<SavedTerrainCellSource> cells;
    std::uint64_t known_terrain_masks = 0;
    bool native_markup_complete = false;
};
// Build an owned, render-neutral read model from one saved map. This is the
// shared logic/graphics boundary: no pointers, 32-bit capacities, hash
// resolution, visibility recomputation or terrain-cell padding is guessed.
SavedMapProjection project_saved_map(const MapHeaderObservation &header,
                                     const MapPlotsObservation &plots,
                                     const SavedTerrainFeatureResolver &features = {});
// Adapt the read model to graphics-facing cell order. This does not construct
// the transformed/padded native grid or establish its claim/reserved storage.
// Do not pass the result to TerrainLogicalClaims before native_markup_complete.
SavedMapTerrainSource project_saved_map_terrain(const SavedMapProjection &projection);
struct MapPlotBudget {
    std::uint64_t plots = 1024 * 1024;
    GameStateBudget data{64 * 1024 * 1024, 16 * 1024 * 1024, 64 * 1024 * 1024};
};
// Stops before CvArea allocations. Does not rebuild visibility/impassability,
// resolve hashes, discard removed types or execute scripts and load hooks.
MapPlotsObservation observe_map_plots(runtime::BinaryReader &input, const MapHeaderObservation &header,
                                      PlotWireDimensions dimensions, MapPlotBudget budget = {});
} // namespace civ5::saves
