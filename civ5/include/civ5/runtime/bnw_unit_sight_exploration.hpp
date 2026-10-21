#pragma once
#include "civ5/runtime/bnw_unit_sight_detached.hpp"

namespace civ5::runtime {

enum class BnwSightExplorationBlocker : std::uint8_t {
    natural_wonder_lua_rewards_and_notifications,
    sdk_ordered_meet_and_team_diplomacy,
    remaining_ui_and_set_xy_effects,
    complete_world_checkpoint,
};

struct BnwSightExplorationResult {
    bool calculated=false;
    std::string reason;
    BnwDetachedSightResult sight;
    std::vector<std::int64_t> dirty_players;
    std::vector<BnwSightExplorationBlocker> blockers;
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvPlot.cpp:7694-7739: after setRevealed(true) returns and before
// adjacent-owner refresh or any meet, first-time discovery marks EconomicAI
// exploration plots dirty for all living members of the revealing team.
// This applies only that idempotent owned-state effect to a private copy.
// A first-revealed natural wonder requires earlier unported SDK effects and
// therefore fails closed. No Lua, diplomacy or UI is published.
BnwSightExplorationResult apply_bnw_unit_sight_exploration_detached(
    const LiveSessionState &state,const BnwSetXYRequest &request,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime
