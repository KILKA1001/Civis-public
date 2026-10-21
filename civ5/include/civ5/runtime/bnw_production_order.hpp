#pragma once

#include "civ5/runtime/live_action_transaction.hpp"
#include <functional>
#include <optional>
#include <string_view>

namespace civ5::runtime {

enum class BnwProductionKind : std::int64_t {
    train=0, construct=1, create=2, prepare=3, maintain=4
};

struct BnwProductionChoice {
    BnwProductionKind kind=BnwProductionKind::train;
    std::int64_t item=-1, secondary=-1;
    bool save=false, rush=false;
};

// CvCity::pushOrder and popOrder also modify player/team making counters,
// process yields, and operation commitments. These stages run against the
// detached LiveSessionState; the service must implement them without external
// effects and preserve the target city and queue object while called. A missing
// service rejects the whole action.
enum class BnwProductionOwnedEffect {
    release_commitment,
    uncommit_operation,
    stop_process,
    start_process,
    add_commitment,
};

struct BnwProductionOrderServices {
    std::function<bool()> lua_ready;
    // Complete CvPlayer/CvCity/CvPlot eligibility other than the Info-ID check
    // below. The callback must be observational on the detached state.
    std::function<bool(const LiveSessionState &, std::int64_t, std::int64_t,
                       const BnwProductionChoice &)> core_can_order_read_only;
    // The source calls CityCanTrain/Construct/Create/Prepare/Maintain after
    // native checks. nullopt means no listener; false is a veto.
    std::function<std::optional<bool>(std::string_view, std::int64_t,
                                      std::int64_t, std::int64_t,
                                      const LiveSessionState &)> lua_test_all_read_only;
    std::function<std::optional<std::int64_t>(const LiveSessionState &,
                                              std::int64_t)> default_unit_ai_read_only;
    std::function<void(LiveSessionState &, std::int64_t, std::int64_t,
                       BnwProductionOwnedEffect,
                       const CityYieldState::ProductionOrder &)> apply_owned_effect;
    // UI effects occur only after commit.
    std::function<void(std::int64_t, std::int64_t)> dirty_after_commit;
};

// Replaces the queue with one current order (pushOrder bPop=true,bAppend=false).
// Does not grant production, complete the order, or advance a turn.
LiveActionResult choose_bnw_city_production(
    LiveSession &session, std::int64_t owner, std::int64_t city,
    const BnwProductionChoice &choice, const ProductionProgressRules &rules,
    const BnwProductionOrderServices &services);

} // namespace civ5::runtime
