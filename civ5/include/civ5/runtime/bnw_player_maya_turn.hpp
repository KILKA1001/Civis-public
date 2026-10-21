#pragma once

#include "civ5/runtime/session_checkpoint_identity.hpp"
#include <cstdint>
#include <stdexcept>

namespace civ5::content { class RulesDatabase; }
namespace civ5::runtime {
struct LiveSessionState;

enum class BnwMayaTurnBlocker {
    profile, selected_rules, player, leader, maya_calendar_and_boost
};
class BnwMayaTurnError : public std::runtime_error {
  public:
    BnwMayaTurnError(BnwMayaTurnBlocker blocker, const char *message)
        : std::runtime_error(message), blocker_(blocker) {}
    BnwMayaTurnBlocker blocker() const noexcept { return blocker_; }
  private:
    BnwMayaTurnBlocker blocker_;
};
struct BnwMayaTurnStep {
    std::int64_t player_id = -1;
    bool uses_maya_calendar = false;
};

// Expansion2 CvPlayer::doTurn immediately after DoUpdateCityRevolts. This
// resolves IsUsingMayaCalendar's false branch from CvPreGame leader identity
// and the first Maya-calendar trait in selected Info order. The true branch
// needs ComputeMayaDate, trait state and ChangeNumMayaBoosts' notification or
// AI choice; it remains blocked before any mutation.
BnwMayaTurnStep apply_bnw_player_maya_turn(
    LiveSessionState &detached_state, std::int64_t player_id,
    const content::RulesDatabase &selected_rules,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime
