#pragma once
#include "civ5/runtime/treasury_turn.hpp"
#include "civ5/runtime/vp_culture_history_turn.hpp"
#include <functional>

namespace civ5::runtime {
// VP GameCore hook invoked before SetAnarchyNumTurns stores the new value.
using VpPlayerAnarchyEvent = std::function<void(std::int64_t,bool,std::int64_t)>;
struct VpAnarchyTurnResult {
    std::int64_t before=0,after=0;
    bool changed=false,event_ended=false,ui_dirty=false;
};
// The CvPlayer::doTurn step immediately after DoUpdateNextPolicyCost. The
// shared treasury flag is kept in sync because later science reads it.
VpAnarchyTurnResult do_vp_anarchy_after_policy_cost(
    ResearchContext &world,TreasuryTurnContext &turn,std::int64_t player,
    const VpPlayerAnarchyEvent &event={});
} // namespace civ5::runtime
