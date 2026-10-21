#pragma once

#include "civ5/runtime/live_action_transaction.hpp"
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
#include <vector>

namespace civ5::runtime {

// Ordered boundaries in the installed Expansion2 SDK's CvGame::doTurn and
// CvPlayer::doTurn / doTurnPostDiplomacy. A handler owns its entire named
// source operation, including required GameCore, engine and Lua effects.
// player_achievements records the SDK position but is excluded from this
// standalone milestone: StartTurn only invokes external Steam achievements.
enum class BnwTurnPhase {
    game_engine_begin,
    game_barbarians_begin,
    game_caches_and_score,
    game_deals,
    game_teams,
    game_map,
    game_interface_turn,
    game_barbarian_camps,
    game_barbarian_units,
    game_religion,
    game_trade,
    game_leagues,
    game_culture,
    game_clock,
    game_activation,
    game_victory,
    game_publish,
    player_cache,
    player_ai_pre,
    player_timers_and_revolts,
    player_trade_strategy_diplomacy,
    player_post_diplomacy_ai,
    player_golden_age_and_gifts,
    player_cities,
    player_gold,
    player_culture,
    player_policies,
    player_research,
    player_espionage,
    player_faith,
    player_leagues,
    player_anarchy_units_replay,
    player_ai_post,
    player_lua,
    player_achievements,
};

std::string_view bnw_turn_phase_name(BnwTurnPhase phase);
std::span<const BnwTurnPhase> bnw_game_turn_order();
std::span<const BnwTurnPhase> bnw_player_turn_order();

struct BnwTurnHandler {
    BnwTurnPhase phase;
    // -1 for game phases; saved player ID for player phases. Implementations
    // must change only the owned state until this entire turn commits.
    std::function<LiveActionResult(LiveSessionState &,std::int64_t)> run;
    // Optional external effects. Called in source phase order only after the
    // whole owned turn commits. Implementations must record uncertain delivery
    // before calling an external service that can throw.
    std::function<LiveActionResult(LiveSession &)> after_commit = {};
};

struct BnwTurnPreflight {
    std::vector<BnwTurnPhase> missing, duplicates, unbound, unexpected;
    bool ready() const noexcept {
        return missing.empty() && duplicates.empty() && unbound.empty() && unexpected.empty();
    }
};

BnwTurnPreflight preflight_bnw_game_turn(std::span<const BnwTurnHandler> handlers);
BnwTurnPreflight preflight_bnw_player_turn(std::span<const BnwTurnHandler> handlers);

// Preflight runs before the first mutable-state access. Every required phase
// must have exactly one real caller-supplied handler, except the excluded
// Steam achievement phase, for which handlers are rejected. The scheduler
// provides no successful default or partial economy/research substitute. Rejection or an
// exception rolls back the whole owned state and both RNG streams. Neither
// function changes LiveSession::readiness or claims playable continuation.
// Publication stops at the first rejected/throwing receiver. Earlier external
// effects and the committed owned state remain; no automatic replay occurs.
// This diagnostic batch API does not provide durable publication recovery or
// the interleaved external effects required by a production GameCore turn.
LiveActionResult run_bnw_game_turn(LiveSession &session,
                                   std::span<const BnwTurnHandler> handlers);
LiveActionResult run_bnw_player_turn(LiveSession &session,std::int64_t player,
                                     std::span<const BnwTurnHandler> handlers);

} // namespace civ5::runtime
