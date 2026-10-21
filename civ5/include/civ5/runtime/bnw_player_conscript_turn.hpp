#pragma once

#include <cstdint>

namespace civ5::runtime {
struct LiveSessionState;

struct BnwConscriptTurnStep {
    std::int64_t player_id = -1;
    std::int32_t before = 0;
    std::int32_t after = 0;
};

// Expansion2 CvPlayer::doTurn resets this scalar immediately after
// TurnsSinceSettledLastCity. The setter has no engine/Lua callback, so this
// detached-state step stages no external intent. The caller must preserve
// source order and must not treat it as a complete player phase.
BnwConscriptTurnStep apply_bnw_conscript_count_turn(LiveSessionState &detached_state,
                                                    std::int64_t player_id);
} // namespace civ5::runtime
