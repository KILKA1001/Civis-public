#pragma once
#include "civ5/runtime/bnw_unit_sight_detached.hpp"

namespace civ5::runtime {

enum class BnwSightDetailBlocker : std::uint8_t {
    sdk_ordered_meet_and_diplomacy,
    exploration_and_natural_wonder,
    lua_notifications_and_ui,
    full_set_xy_effects,
    visibility_and_world_checkpoint,
};

struct BnwSightRevealDetailsResult {
    bool calculated=false;
    std::string reason;
    BnwDetachedSightResult sight;
    std::vector<BnwSightDetailBlocker> blockers;
    bool can_commit() const noexcept {return false;}
};

// Diagnostic continuation of the detached sight transition. Applies only
// CvPlot::setRevealed's known owner/improvement/route/city state, the team's
// found-territory flags, and CvPlot::updateRevealedOwner for each newly
// visible plot's six neighbors. The caller's session and source are const;
// any failure discards the entire detached result.
BnwSightRevealDetailsResult apply_bnw_unit_sight_reveal_details(
    const LiveSessionState &state,const BnwSetXYRequest &request,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime
