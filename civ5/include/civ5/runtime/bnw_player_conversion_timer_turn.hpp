#pragma once

#include <cstdint>
#include <optional>

namespace civ5::runtime {
struct LiveSessionState;

enum class BnwConversionTimerDirtyBit { misc_buttons };
struct BnwConversionTimerDirtyIntent {
    std::int64_t player_id = -1;
    BnwConversionTimerDirtyBit bit = BnwConversionTimerDirtyBit::misc_buttons;
    bool value = true;
};
struct BnwConversionTimerTurnStep {
    std::int64_t player_id = -1;
    std::int32_t before = 0;
    std::int32_t after = 0;
    // Source setter issues this after writing the timer, only for the active
    // player. Publish only after the enclosing detached turn commits. A
    // throwing or rejected turn must discard this intent with its state.
    std::optional<BnwConversionTimerDirtyIntent> post_commit_dirty;
};

// Expansion2 CvPlayer::doTurn's second timer operation. active_player_id must
// come from the current engine/game owner; saved pregame is insufficient after
// active-player changes. The caller supplies a detached LiveSessionState and
// retains the returned intent until the whole turn commits. This does not
// complete player_timers_and_revolts or publish an engine effect by itself.
BnwConversionTimerTurnStep apply_bnw_conversion_timer_turn(
    LiveSessionState &detached_state, std::int64_t player_id,
    std::int64_t active_player_id);
} // namespace civ5::runtime
