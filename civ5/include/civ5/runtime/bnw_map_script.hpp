#pragma once
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/content/manifest.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/runtime/bnw_setup_plot_grid.hpp"
#include "civ5/runtime/bnw_fresh_yield.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include "civ5/runtime/simulation_random.hpp"
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {
struct BnwFreshGameContentResult;

// Plot categories produced by a BNW map script. Terrain is attached only
// after full selected GenerateTerrain application; later world stages remain.
struct BnwGeneratedPlotMap {
    std::uint64_t width = 0, height = 0;
    bool wrap_x = true, wrap_y = false;
    std::vector<std::uint8_t> plot_types;
    // Present only after GenerateTerrain applied valid selected Terrain Info
    // to every owned plot through the fresh map setter.
    std::optional<std::vector<std::int32_t>> terrain_types;
    // Source-order components and callback traces. Yield cache writes and
    // handles are retained below, but external notifications remain pending.
    std::optional<BnwFreshAreaStructure> area_structure;
    // Executed final CvMap::updateYield cache for the completed fresh grid.
    // Prior setArea and lake-threshold passes are recorded in yield_progress.
    std::optional<std::vector<std::array<std::int16_t, 6>>> final_yields;
    std::optional<BnwFreshYieldProgress> yield_progress;
    // Successful fresh AddRivers setters append ordered updateSymbols events
    // and increment one selected CvArea edge count each. Both counters are
    // committed with the owned grid and final cache, or rolled back together.
    std::uint64_t river_symbol_events = 0, river_edge_writes = 0;
    // AddLakes is committed only after selected Map.CalculateAreas returns.
    std::uint64_t lake_symbol_events = 0, lake_plot_conversions = 0;
    // Owned fresh CvPlot state after verified area/landmass handle assignment.
    // It remains a setup grid while updateSymbols/world effects are pending.
    std::optional<BnwSetupPlotGrid> owned_setup_grid;
};

enum class BnwMapScriptBlockerCode {
    invalid_source_key,
    source_missing,
    source_ambiguous,
    source_hash_mismatch,
    source_io_failed,
    invalid_map_random,
    invalid_setup_context,
    missing_map_option,
    include_missing,
    include_ambiguous,
    include_cycle,
    include_invalid,
    include_io_failed,
    invalid_world_rules,
    unresolved_map_wrap,
    unresolved_map_dimensions,
    unresolved_map_plots,
    fill_budget_exceeded,
    fractal_budget_exceeded,
    unsupported_stock_helper,
    script_failed,
    invalid_dimensions,
    invalid_plot_types,
    terrain_pending,
    features_rivers_pending,
    starting_positions_pending,
    world_setup_pending,
    invalid_terrain_types,
    areas_pending,
    river_generation_pending,
    lake_generation_pending,
    invalid_area_structure,
    yield_replay_budget_exceeded,
};
struct BnwMapScriptBlocker {
    BnwMapScriptBlockerCode code;
    std::string message;
};
struct BnwMapSourceKey {
    // Exact path relative to the resource source, not a basename. Optional
    // pins disambiguate repeated logical paths across selected mounts.
    std::string relative_path;
    std::optional<std::string> source_id, sha256;
};
struct BnwMapScriptSource {
    std::string relative_path, sha256;
    assets::Provenance provenance;
};
struct BnwMapIncludedSource {
    std::string name, sha256;
    assets::Provenance provenance;
};
struct BnwMapWorldRulesSource {
    std::string worlds_sha256;
    std::string defines_sha256;
    ContentProfileKind profile_kind = ContentProfileKind::vanilla;
    std::vector<std::string> selected_requirements;
};
struct BnwMapScriptResult {
    std::optional<BnwGeneratedPlotMap> map;
    std::optional<BnwMapScriptSource> source;
    // Exact helper bytes executed, in include call order. Like the SDK,
    // repeated include calls execute again; active recursion is rejected.
    std::vector<BnwMapIncludedSource> included_sources;
    std::optional<BnwMapWorldRulesSource> world_rules_source;
    // A copy of the caller's BNW map stream after script execution. The
    // caller decides whether to commit it; failed generation never mutates
    // the caller's stream.
    std::optional<SimulationRandomState> map_random_before, map_random_after;
    std::vector<BnwMapScriptBlocker> blockers;
    bool has_blocker(BnwMapScriptBlockerCode code) const;
};
enum class BnwFreshMapHandoffBlockerCode {
    content_unverified,
    map_unverified,
    provenance_unverified,
    update_symbols_delivery_pending,
    river_generation_pending,
    lake_generation_pending,
    features_rivers_pending,
    starting_positions_pending,
    game_state_players_teams_pending,
    live_session_factory_pending,
};
struct BnwFreshMapHandoffBlocker {
    BnwFreshMapHandoffBlockerCode code;
    std::string message;
};
// Move-only boundary between selected map generation and a future fresh
// LiveSession factory. It owns the setup grid and ordered, undelivered
// updateSymbols events; it does not attest to a playable game.
struct BnwFreshMapHandoff {
    std::optional<BnwGeneratedPlotMap> map;
    std::optional<BnwMapScriptSource> source;
    std::vector<BnwMapIncludedSource> included_sources;
    std::optional<BnwMapWorldRulesSource> world_rules_source;
    std::string selected_rules_sha256;
    std::optional<SimulationRandomState> map_random_before, map_random_after;
    std::vector<BnwFreshMapHandoffBlocker> blockers;
    bool has_blocker(BnwFreshMapHandoffBlockerCode code) const;
    bool ready_for_live_session() const noexcept { return blockers.empty(); }
};
BnwFreshMapHandoff prepare_bnw_fresh_map_handoff(
    BnwMapScriptResult &&generated, const BnwFreshGameContentResult &selected);
struct BnwMapScriptBudget {
    std::uint64_t maximum_plots = 1'000'000;
    std::uint64_t maximum_plot_state_bytes = 32ULL * 1024 * 1024;
    std::uint64_t maximum_fractals = 64;
    std::uint64_t maximum_ridge_seeds = 64;
    std::uint64_t maximum_ridge_retries = 4096;
    std::uint64_t maximum_source_bytes = 16ULL * 1024 * 1024;
    std::uint64_t maximum_yield_replay_plot_visits = 20'000'000;
    std::uint64_t maximum_symbol_events = 2'000'000;
    std::uint64_t maximum_river_water_query_visits = 1'000'000;
    ScriptBudget lua{64ULL * 1024 * 1024, 10'000'000};
};
struct BnwMapSetupContext {
    // Exact selected WorldSizes ID passed to GetMapInitData(worldSize) and
    // observed through Map.GetWorldSize after initialization.
    std::optional<std::int64_t> world_size;
    // Already resolved pregame map choices/defaults, keyed by their SDK
    // OptionID strings (stock scripts pass numeric IDs converted to strings).
    // A missing optional means no option service; an empty map means no
    // choices are resolved and every lookup is an explicit blocker.
    std::optional<std::map<std::string, std::int64_t>> custom_options;
    // The activated, selected BNW content boundary. Validated Worlds rows
    // include FeatureGrainChange for FeatureGenerator; selected Feature and
    // Terrain enum IDs are read from the same rules. It must outlive this call.
    const BnwFreshGameContentResult *selected_rules = nullptr;
    // Selected native map generation may run with or without an initialized
    // graphics host. CvPlot::setPlotType's final layout callback is gated by
    // GC.IsGraphicsInitialized; absence here is an explicit blocker.
    std::optional<bool> graphics_initialized;
    // Only session-resolved native limits may be supplied. The map layer
    // never guesses x86 player caps or hardcodes installed DLC values.
    std::vector<content::GameplayDefine> runtime_defines;
    // CvMap::getClimate reads the selected CvPreGame climate, validated
    // against the activated Climates Info table before Lua can observe it.
    std::optional<std::int64_t> climate;
};

// Execute a caller-verified Lua source in an isolated map context. This first
// stage supports self-contained scripts that return Width/Height from
// GetMapInitData and either a dense, 1-based PlotTypes array or a fully
// mutated owned grid from GeneratePlotTypes.
// Selected Lua helpers can load through strict include. Unsupported native
// fractals and world services fail explicitly; no playable world is fabricated.
BnwMapScriptResult generate_bnw_plot_map(std::string_view verified_source,
                                         std::string_view chunk_name,
                                         BnwMapScriptBudget budget = {});
// Map.Rand consumes a copy of the separate BNW map stream with the SDK's
// unsigned-short range conversion, including draws for range zero and one.
BnwMapScriptResult generate_bnw_plot_map(std::string_view verified_source,
                                         std::string_view chunk_name,
                                         SimulationRandom map_random,
                                         BnwMapScriptBudget budget = {});
BnwMapScriptResult generate_bnw_plot_map(std::string_view verified_source,
                                         std::string_view chunk_name,
                                         BnwMapSetupContext setup,
                                         std::optional<SimulationRandom> map_random = {},
                                         BnwMapScriptBudget budget = {});

// Index stock base map scripts and Gameplay/Lua helpers, plus only the
// corresponding directories of selected DLC. This is a script-source view,
// not an entitlement or gameplay activation.
assets::ResourceMountTable build_bnw_map_script_resources(
    const std::filesystem::path &game_root, const SavedContentProfile &profile,
    const content::Catalog &catalog);

// Resolve a unique, exact source from active mounts. Execute the bytes read
// from that handle and retain their SHA-256 and provenance even if Lua fails.
BnwMapScriptResult generate_bnw_plot_map(const assets::ResourceMountTable &resources,
                                         const BnwMapSourceKey &key,
                                         BnwMapScriptBudget budget = {});
BnwMapScriptResult generate_bnw_plot_map(const assets::ResourceMountTable &resources,
                                         const BnwMapSourceKey &key,
                                         SimulationRandom map_random,
                                         BnwMapScriptBudget budget = {});
BnwMapScriptResult generate_bnw_plot_map(const assets::ResourceMountTable &resources,
                                         const BnwMapSourceKey &key,
                                         BnwMapSetupContext setup,
                                         std::optional<SimulationRandom> map_random = {},
                                         BnwMapScriptBudget budget = {});

} // namespace civ5::runtime
