#pragma once

#include <array>
#include <cstdint>

namespace civ5::runtime {
struct LiveSessionState;

struct BnwTemporaryBonusTurnStep {
    std::int64_t player_id = -1;
    // Attack, culture, tourism, in CvPlayer::doTurnPostDiplomacy order.
    std::array<std::int32_t, 3> before{};
    std::array<std::int32_t, 3> after{};
};

// Expansion2 CvPlayer::doTurnPostDiplomacy's three temporary boosts.
// The caller supplies a detached turn draft. This step does not execute the
// preceding AI or the following golden age, gifts, and city turns.
BnwTemporaryBonusTurnStep apply_bnw_temporary_bonus_turn(
    LiveSessionState &detached_state, std::int64_t player_id);
} // namespace civ5::runtime
