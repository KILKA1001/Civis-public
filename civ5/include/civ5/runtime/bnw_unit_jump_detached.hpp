#pragma once

#include "civ5/runtime/bnw_plot_unit_validity_preflight.hpp"
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace civ5::runtime {

// The callbacks correspond to the CvUnit.cpp jumpToNearestValidPlot scan.
// An absent callback or an unanswered value stops the scan; it never means
// false. A production binder must use the live GameCore, not saved flags.
using BnwJumpBool = std::function<std::optional<bool>(
    const LiveSessionState &, std::uint64_t, saves::SavedPlotUnitReference)>;
using BnwJumpNumber = std::function<std::optional<std::int64_t>(
    const LiveSessionState &, std::uint64_t, saves::SavedPlotUnitReference)>;
using BnwJumpDistance = std::function<std::optional<std::int64_t>(
    const LiveSessionState &, std::uint64_t, std::uint64_t)>;

struct BnwJumpReadServices {
    BnwJumpNumber nearest_city_plot; // -1 means findCity returned NULL.
    BnwJumpBool valid_domain, can_move_into, can_enter_territory;
    BnwJumpBool barbarian, enemy, mountain, friendly_city, air, sea;
    BnwJumpBool coastal_land, water, revealed;
    BnwJumpNumber friendly_units_of_type, area_id;
    BnwJumpDistance plot_distance; // CvMap::plotDistance, including wrapping.
};

enum class BnwJumpOutcome { unresolved, no_destination, destination };
struct BnwJumpSelection {
    BnwJumpOutcome outcome=BnwJumpOutcome::unresolved;
    std::string reason;
    std::uint64_t source_plot=0;
    std::optional<std::uint64_t> destination_plot;
    std::int64_t best_value=-1;
    std::uint64_t scanned_plots=0;
    // No RNG is used by the SDK selection. A strict '<' keeps the first
    // plot index on ties.
};

// CvUnit::jumpToNearestValidPlot through pBestPlot selection. Read only.
BnwJumpSelection select_bnw_nearest_valid_plot(
    const LiveSessionState &state, std::uint64_t source_plot,
    saves::SavedPlotUnitReference handle, std::int64_t plot_unit_limit,
    const BnwJumpReadServices &services);

struct BnwJumpDetachedServices {
    // These functions may mutate only their detached LiveSessionState. They
    // must implement the full SDK effects of each call; external Lua/UI
    // publication must be escrowed until a complete checkpoint commits.
    std::function<LiveActionResult(LiveSessionState &, saves::SavedPlotUnitReference,
                                   std::uint64_t)> set_xy;
    std::function<LiveActionResult(LiveSessionState &, saves::SavedPlotUnitReference)>
        clear_mission_queue, set_activity_awake;
};
struct BnwJumpDetachedResult {
    BnwJumpSelection selection;
    std::optional<LiveSessionState> detached;
    std::string reason;
    // A detached projection cannot authorize a game commit until all setXY
    // effects and their checkpoint sections have been validated.
    bool can_commit() const noexcept { return false; }
};

// Synthetic effect harness for the SDK call order: setXY, ClearMissionQueue,
// SetActivityType(ACTIVITY_AWAKE). Any failure discards the private copy.
// A failed jump requires kill(false), which is intentionally not called here.
BnwJumpDetachedResult project_bnw_jump_detached(
    const LiveSessionState &state, std::uint64_t source_plot,
    saves::SavedPlotUnitReference handle, std::int64_t plot_unit_limit,
    const BnwJumpReadServices &reads, const BnwJumpDetachedServices &effects);

} // namespace civ5::runtime
