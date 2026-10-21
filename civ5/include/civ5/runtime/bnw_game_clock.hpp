#pragma once

#include "civ5/runtime/bnw_turn_scheduler.hpp"
#include <functional>
#include <string_view>

namespace civ5::runtime {

// CvGame::setGameTurn publishes these engine/UI effects in source order.
// Services must represent real effects; an empty callback is not a supported
// completion path. The phase only queues publication on detached owned state.
struct BnwGameClockServices {
    std::function<void(std::string_view)> net_debug_log;
    std::function<void()> turn_timer_dirty;
    std::function<void()> game_data_dirty;
    std::function<void(std::int64_t)> gameplay_turn_changed;
    std::function<void()> end_turn_timer_reset;
    bool complete() const noexcept;
};

// Source: Expansion2 CvGame.cpp, doTurn lines 7480+, setGameTurn line 4364,
// incrementGameTurn line 4385, incrementElapsedGameTurns line 4414. Only the
// clock and its decoded mirrors are changed; no other turn phases run here.
LiveActionResult run_bnw_game_clock_phase(
    LiveSessionState &state, const BnwGameClockServices &services);
BnwTurnHandler make_bnw_game_clock_handler(BnwGameClockServices services);

// Call after a standalone apply_live_action; the scheduler invokes this from
// the clock handler's after-commit hook. A throwing callback leaves an
// uncertain publication blocker instead of risking duplicate external effects.
LiveActionResult publish_bnw_game_clock(
    LiveSession &session, const BnwGameClockServices &services);

// Convenience for exercising this phase alone. It does not make a full turn.
LiveActionResult advance_bnw_game_clock(
    LiveSession &session, const BnwGameClockServices &services);

// Runs the complete caller-bound game phase set with the owned clock handler;
// its after-commit hook publishes clock effects after the outer turn. All other
// phases remain mandatory; this wrapper supplies no default gameplay work.
LiveActionResult run_bnw_game_turn_with_clock(
    LiveSession &session, std::span<const BnwTurnHandler> other_handlers,
    const BnwGameClockServices &services);

} // namespace civ5::runtime
