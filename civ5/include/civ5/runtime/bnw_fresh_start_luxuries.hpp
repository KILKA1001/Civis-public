#pragma once

#include "civ5/runtime/bnw_fresh_start_resources.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace civ5::runtime {

enum class BnwStartLuxuriesBlockerCode {
    resource_scan_pending,
    selected_luxury_missing,
    custom_init_pending,
};
struct BnwStartLuxuryWeight {
    std::int32_t resource_id;
    std::uint8_t weight;
    bool operator==(const BnwStartLuxuryWeight &) const = default;
};
struct BnwStartLuxuriesBlocker {
    BnwStartLuxuriesBlockerCode code;
    std::string message;
};
struct BnwFreshStartLuxuryWeights {
    // Lua regions 1..8 map to indices 0..7 here. Insertion order is the
    // literal source order, including duplicate weights across regions.
    std::array<std::vector<BnwStartLuxuryWeight>, 8> regions;
    std::vector<BnwStartLuxuryWeight> fallback, city_states;
    std::string selected_rules_sha256;
    std::string stock_helper_sha256;
    SimulationRandomState map_random_before{}, map_random_after{};
    std::vector<BnwStartLuxuriesBlocker> blockers;
    bool luxury_weights_complete = false;
    bool has_blocker(BnwStartLuxuriesBlockerCode code) const;
    void rollback() noexcept;
};

// Expansion2 AssignStartingPlots:__InitLuxuryWeights only. Requires the exact
// selected helper bytes attested by the preceding resource scan.
BnwFreshStartLuxuryWeights draft_bnw_fresh_start_luxury_weights(
    const BnwFreshStartResources &resources,
    const SimulationRandom &map_random);

} // namespace civ5::runtime
