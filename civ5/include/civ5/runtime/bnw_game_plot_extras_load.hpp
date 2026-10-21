#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/game_history.hpp"
#include <cstdint>
#include <vector>

namespace civ5::saves { struct SavedSession; }

namespace civ5::runtime {

struct LiveSessionState;

struct BnwGamePlotExtras {
    std::vector<saves::PlotYieldObservation> yields;
    std::vector<saves::PlotCostObservation> costs;
    bool current=false;

    // Expansion2 CvGame.cpp:9745/9819: linear scan, first coordinate match.
    std::int64_t extra_yield(std::int64_t x,std::int64_t y,
                             std::int64_t yield) const;
    std::int64_t extra_cost(std::int64_t x,std::int64_t y) const;
};

// CvGame::Read owns the two vectors, independent of map plot records. This
// restores their exact getter semantics from a source-bound raw observation.
// No setPlotExtraYield/remove/changePlotExtraCost action is provided: those
// require CvPlot::updateYield and a matching GameHistory checkpoint.
BnwGamePlotExtras restore_bnw_game_plot_extras(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
