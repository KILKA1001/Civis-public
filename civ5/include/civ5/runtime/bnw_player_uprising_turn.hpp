#pragma once

#include "civ5/runtime/session_checkpoint_identity.hpp"
#include <cstdint>
#include <stdexcept>

namespace civ5::content { class RulesDatabase; }
namespace civ5::runtime {
struct LiveSessionState;

enum class BnwUprisingTurnBlocker {
    profile, selected_rules, player, happiness, game_speed,
    source_integer_range, uprising_spawn
};
class BnwUprisingTurnError : public std::runtime_error {
  public:
    BnwUprisingTurnError(BnwUprisingTurnBlocker blocker,const char *message)
        : std::runtime_error(message),blocker_(blocker) {}
    BnwUprisingTurnBlocker blocker() const noexcept {return blocker_;}
  private:
    BnwUprisingTurnBlocker blocker_;
};
enum class BnwUprisingTurnBranch { not_very_unhappy, decrement, first_seed };
struct BnwUprisingTurnStep {
    std::int64_t player_id=-1;
    std::int32_t before=0,after=0;
    BnwUprisingTurnBranch branch=BnwUprisingTurnBranch::not_very_unhappy;
    std::uint64_t gameplay_rng_calls=0;
};

// Expansion2 CvPlayer::DoUpdateUprisings after DoUpdateCramped. Recomputes
// IsEmpireVeryUnhappy from live BNW inputs and selected rules. The first-time
// reset uses a detached gameplay RNG draft; failures preserve both RNG and
// player state. The expiry branch remains blocked until DoUprising (spawn,
// plot selection, notifications) and its subsequent reset are implemented.
BnwUprisingTurnStep apply_bnw_player_uprising_turn(
    LiveSessionState &detached_state,std::int64_t player_id,
    const content::RulesDatabase &selected_rules,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime
