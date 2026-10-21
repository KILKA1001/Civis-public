#pragma once

#include "civ5/runtime/session_checkpoint_identity.hpp"
#include <cstdint>
#include <stdexcept>

namespace civ5::content { class RulesDatabase; }
namespace civ5::runtime {
struct LiveSessionState;

enum class BnwCrampedTurnBlocker {
    profile, selected_rules, player, city_roster, map, plot_impassability,
    source_integer_range
};
class BnwCrampedTurnError : public std::runtime_error {
  public:
    BnwCrampedTurnError(BnwCrampedTurnBlocker blocker, const char *message)
        : std::runtime_error(message), blocker_(blocker) {}
    BnwCrampedTurnBlocker blocker() const noexcept { return blocker_; }
  private:
    BnwCrampedTurnBlocker blocker_;
};
struct BnwCrampedTurnStep {
    std::int64_t player_id=-1;
    bool before=false, after=false, wrote=false;
    std::int32_t total_nearby=0, usable_nearby=0;
};

// Expansion2 CvPlayer::DoUpdateCramped, after the conscript reset. Uses the
// current owned city/map state and a current post-load plot impassability cache
// under the same activated rules identity. No external callback is issued.
// The caller owns a detached turn state; later uprising/revolt phases remain
// mandatory before this can be used as a complete player-turn handler.
BnwCrampedTurnStep apply_bnw_player_cramped_turn(
    LiveSessionState &detached_state, std::int64_t player_id,
    const content::RulesDatabase &selected_rules,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime
