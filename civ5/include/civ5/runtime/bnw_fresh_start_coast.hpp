#pragma once

#include "civ5/runtime/bnw_fresh_game_content.hpp"
#include "civ5/runtime/bnw_setup_plot_grid.hpp"
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace civ5::runtime {
struct BnwMapIncludedSource;

enum class BnwStartCoastBlockerCode {
    selected_rules_unverified,
    stock_helper_unverified,
    post_feature_areas_pending,
    owned_map_incomplete,
    start_placement_pending,
};
struct BnwStartCoastBlocker {
    BnwStartCoastBlockerCode code;
    std::string message;
};
enum class BnwStartCoastQueryStage : std::uint8_t {
    adjacent_to_salt_water,
    next_to_coastal_land,
};
struct BnwStartCoastQuery {
    BnwStartCoastQueryStage stage;
    std::size_t plot_index;
    std::uint8_t direction;
    std::optional<std::size_t> neighbor;
    bool operator==(const BnwStartCoastQuery &) const = default;
};
struct BnwFreshStartCoastTables {
    // Lua tables are indexed by y * width + x + 1; these vectors use the
    // corresponding zero-based plot index. They are detached from the map.
    std::vector<bool> coastal, next_to_coast;
    std::vector<BnwStartCoastQuery> queries;
    std::string selected_rules_sha256;
    SimulationRandomState map_random_before{}, map_random_after{};
    std::vector<BnwStartCoastBlocker> blockers;
    bool coastal_tables_complete = false;
    bool has_blocker(BnwStartCoastBlockerCode code) const;
    // A failed subsequent placement stage can discard this detached result
    // without changing the owned grid or its map RNG.
    void rollback() noexcept;
};

// Expansion2 MapmakerUtilities.GenerateNextToCoastalLandDataTables, the first
// substantive call of AssignStartingPlots:__Init. The caller must attest to
// the SECOND Map.RecalculateAreas after AddFeatures, and bind the same selected
// rules fingerprint as map generation. This stage makes zero RNG calls.
BnwFreshStartCoastTables draft_bnw_fresh_start_coast_tables(
    const BnwFreshGameContentResult &selected,
    const BnwSetupPlotGrid &grid, const BnwFreshAreaStructure &areas,
    const SimulationRandom &map_random,
    std::span<const BnwMapIncludedSource> selected_includes,
    std::string_view map_selected_rules_sha256,
    bool post_feature_area_recalculation_complete,
    std::uint64_t maximum_plots = 1'000'000);

} // namespace civ5::runtime
