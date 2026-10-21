#pragma once

#include "civ5/runtime/bnw_fresh_start_coast.hpp"
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace civ5::runtime {

enum class BnwStartResourcesBlockerCode {
    coastal_tables_pending,
    selected_rules_unverified,
    stock_helper_unverified,
    resource_rows_unavailable,
    luxury_weights_pending,
};
struct BnwStartResourceRow {
    std::int32_t id;
    std::string type;
    bool operator==(const BnwStartResourceRow &) const = default;
};
struct BnwStartResourcesBlocker {
    BnwStartResourcesBlockerCode code;
    std::string message;
};
struct BnwFreshStartResources {
    // GameInfo.Resources() is an unfiltered SELECT * with no ORDER BY. Keep
    // the SQLite visit order; neither ID sorting nor hardcoded BNW IDs apply.
    std::vector<BnwStartResourceRow> resources;
    std::vector<BnwStartResourceRow> stock_shortcuts;
    std::string selected_rules_sha256;
    std::string stock_helper_sha256;
    SimulationRandomState map_random_before{}, map_random_after{};
    std::vector<BnwStartResourcesBlocker> blockers;
    bool resource_scan_complete = false;
    bool has_blocker(BnwStartResourcesBlockerCode code) const;
    void rollback() noexcept;
};

// The resource-ID loop immediately after the coastal tables in stock
// AssignStartingPlots:__Init. Does not run __InitLuxuryWeights or place plots.
BnwFreshStartResources draft_bnw_fresh_start_resources(
    const BnwFreshGameContentResult &selected,
    const BnwFreshStartCoastTables &coastal,
    const SimulationRandom &map_random,
    std::span<const BnwMapIncludedSource> selected_includes,
    std::uint64_t maximum_resource_rows = 4096);

} // namespace civ5::runtime
