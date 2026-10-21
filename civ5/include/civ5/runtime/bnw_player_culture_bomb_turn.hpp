#pragma once

#include <cstdint>

namespace civ5::runtime {
struct LiveSessionState;

struct BnwCultureBombTurnStep {
    std::int64_t player_id = -1;
    std::int32_t before = 0;
    std::int32_t after = 0;
};

// CvPlayer::doTurn's first scalar operation after AI_doTurnPre. Imported BNW
// players use the unserialized value established by CvPlayer::Read/reset.
// The caller must have completed the earlier
// player-cache and AI-pre phases; this is not a complete player-turn handler.
// Failure leaves the owned world unchanged. ConversionTimer and later phases
// remain blocked, including their engine UI and GameCore effects.
BnwCultureBombTurnStep apply_bnw_culture_bomb_timer_turn(LiveSessionState &state,
                                                          std::int64_t player_id);

// Rebuild the reset-only scalar after reading player streams. This must not
// insert a synthetic field into PlayerCoreObservation's wire field roster.
void initialize_bnw_player_read_transients(LiveSessionState &state);
} // namespace civ5::runtime
