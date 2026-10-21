#pragma once
#include "civ5/runtime/bnw_unit_sight_reveal_plan.hpp"
#include "civ5/runtime/live_session.hpp"

namespace civ5::runtime {

struct BnwDetachedSightResult {
    bool calculated=false;
    std::string reason;
    BnwUnitSightRevealPlan plan;
    // Diagnostic result only. Never publish this partial SDK transition as a
    // successful setXY: owner/improvement/route, meetings, AI, Lua, UI and
    // full checkpoint effects are still absent.
    std::optional<LiveSessionState> detached;
    std::vector<BnwSightBlocker> blockers;
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvUnit::setXY removes old sight before adding new sight. Apply
// only the verified CvPlot::changeVisibilityCount counter, first-reveal bit
// and CvArea::changeNumRevealedTiles effects on a private copied world. A
// failure discards the copy; source/state supplied by the caller stay intact.
BnwDetachedSightResult apply_bnw_unit_sight_detached(
    const LiveSessionState &state,const BnwSetXYRequest &request,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime
