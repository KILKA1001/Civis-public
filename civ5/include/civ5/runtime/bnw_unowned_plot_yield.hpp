#pragma once
#include "civ5/runtime/bnw_game_plot_extras_load.hpp"
#include "civ5/runtime/bnw_nature_yield.hpp"
#include <optional>

namespace civ5::runtime {

struct BnwUnownedPlotYieldQuote {
    std::int64_t nature=0,extra=0,total=0;
    bool early_zero=false;
};

// SDK suffix after all earlier plot contributions have been proved absent.
BnwUnownedPlotYieldQuote finish_bnw_unowned_plot_yield(
    std::int64_t nature,std::int64_t extra);

// CvPlot::calculateYield(eYield,false) checks these two conditions before it
// reads owner, improvement, route, city or GameCore effects. nullopt means
// this early return did not apply; missing decoded fields remain an error.
std::optional<BnwUnownedPlotYieldQuote> bnw_plot_early_zero_yield(
    const LiveSessionState &world,std::int64_t plot,std::int64_t yield);

// Narrow complete CvPlot::calculateYield(..., bDisplay=false) branch:
// unowned plot, no improvement/route. The SDK only queries a working city
// inside its ePlayer != NO_PLAYER branch. The SDK inserts
// getPlotExtraYield after nature and all player/city/route effects, then
// clamps to zero. Early no-terrain/non-workable checks precede the extra.
// Other plot branches require the remaining full plot-yield calculation.
BnwUnownedPlotYieldQuote bnw_unowned_plot_yield(
    BnwNatureYieldContext &nature,
    const LiveSessionState &world,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::int64_t plot,
    std::int64_t yield);

} // namespace civ5::runtime
