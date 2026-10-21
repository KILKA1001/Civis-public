#pragma once

#include <cstdint>

namespace civ5::runtime {
struct LiveSessionState;

struct BnwSettledCityTurnStep {
    std::int64_t player_id = -1;
    std::int32_t before = 0;
    std::int32_t after = 0;
};

// Third scalar operation after AI_doTurnPre in Expansion2 CvPlayer::doTurn.
// The earlier culture-bomb and conversion timers must already have completed
// in source order. This setter has no engine/Lua effect, so no external intent
// is staged. The caller must apply this only to a detached turn state; it does
// not finish the player_timers_and_revolts phase.
BnwSettledCityTurnStep apply_bnw_turns_since_settled_city_turn(
    LiveSessionState &detached_state, std::int64_t player_id);
} // namespace civ5::runtime
