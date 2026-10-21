#pragma once

#include "civ5/runtime/bnw_turn_scheduler.hpp"
#include "civ5/content/rules_database.hpp"
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace civ5::runtime {

enum class BnwGameActivationMode { unsupported, sequential_single_player };

// CvPlayer::setTurnActive(true,true), in source order. player_do_turn is a
// boundary marker: it is provided only by the player scheduler, never by a
// service callback. The active-player-only effects are omitted for other slots.
enum class BnwGameActivationStep {
    set_turn_active,
    publish_end_turn_dirty,
    clear_end_turn,
    unit_attrition,
    active_player_deferred_fog,
    active_turn_count_log,
    publish_player_turn_start,
    all_units_unprocessed,
    // Enter/restore preserve all six source finder cache values, including on
    // rejection: common, ignore-units, tactical, influence, route, water-route.
    pathfinder_cache_enter,
    city_connections,
    city_connection_gold,
    happiness,
    builder_tasking,
    // The service must verify whether the owning requests object exists; its
    // source null branch may be empty only with that verified state.
    diplomacy_requests_begin,
    player_do_turn,
    player_do_turn_units,
    pathfinder_cache_restore,
    warnings,
    active_player_unit_cycler,
    active_player_selection_counter,
    active_player_selection_camera,
    active_player_turn_start,
};
struct BnwGameActivationService {
    BnwGameActivationStep step;
    // A real owner must implement the entire source operation. This preflight
    // checks binding only and does not execute or certify the callback.
    std::function<LiveActionResult(LiveSessionState &,std::int64_t)> run;
};
std::string_view bnw_game_activation_step_name(BnwGameActivationStep step);

struct BnwGameActivationTicket {
    BnwGameActivationMode mode=BnwGameActivationMode::unsupported;
    std::int64_t player=-1, active_player=-1, game_turn=-1;
    // The sequential SP branch has no shuffleArray and consumes no order RNG.
    std::uint64_t activation_order_rng_draws=0;
    // These are obligations, not receipts. The player_do_turn boundary must be
    // entered once, followed once by player_do_turn_units, under one transaction.
    std::uint32_t required_player_turn_calls=0;
    std::uint32_t required_unit_turn_calls=0;
    std::vector<BnwGameActivationStep> activation_order;
    std::vector<BnwTurnPhase> player_phase_order;
};
struct BnwGameActivationPreflight {
    bool structurally_composable=false;
    BnwGameActivationTicket ticket;
    std::string blocker;
    // No world/UI/engine mutation or RNG draw occurs in this preflight.
    bool phase_committed=false;
};

// Source: Expansion2 CvGame.cpp:7564-7625; CvPlayer.cpp:15785-15910.
// completed_game_prefix must contain exactly the earlier CvGame::doTurn
// phases, in source order. player_handlers must cover each mandatory player
// phase once. services must cover each applicable setTurnActive operation once,
// excluding player_do_turn, whose only owner is the player scheduler. A bound
// callback is not evidence that a phase has executed. This seam never returns
// a successful game_activation phase or grants continuation readiness.
BnwGameActivationPreflight preflight_bnw_game_activation(
    const LiveSessionState &state,
    const content::RulesDatabase &database,
    std::span<const BnwTurnPhase> completed_game_prefix,
    std::span<const BnwTurnHandler> player_handlers,
    std::span<const BnwGameActivationService> services={});

} // namespace civ5::runtime
