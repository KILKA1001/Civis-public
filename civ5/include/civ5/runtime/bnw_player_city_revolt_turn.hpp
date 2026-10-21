#pragma once

#include "civ5/runtime/session_checkpoint_identity.hpp"
#include <cstdint>
#include <stdexcept>

namespace civ5::content { class RulesDatabase; }
namespace civ5::runtime {
struct LiveSessionState;

enum class BnwCityRevoltTurnBlocker {
    profile, selected_rules, player, happiness,
    city_revolt_and_transfer, city_target_and_notification
};
class BnwCityRevoltTurnError : public std::runtime_error {
  public:
    BnwCityRevoltTurnError(BnwCityRevoltTurnBlocker blocker,const char *message)
        : std::runtime_error(message),blocker_(blocker) {}
    BnwCityRevoltTurnBlocker blocker() const noexcept {return blocker_;}
  private:
    BnwCityRevoltTurnBlocker blocker_;
};
enum class BnwCityRevoltTurnBranch { inactive, decrement };
struct BnwCityRevoltTurnStep {
    std::int64_t player_id=-1;
    std::int32_t before=0,after=0;
    BnwCityRevoltTurnBranch branch=BnwCityRevoltTurnBranch::inactive;
};

// Expansion2 CvPlayer::DoUpdateCityRevolts after DoUpdateUprisings. The
// inactive and non-expiring countdown branches are complete on owned state.
// Expiry/first seed require ideology/city target selection, notifications and
// potentially acquireCity/unit relocation; reject them before any mutation.
BnwCityRevoltTurnStep apply_bnw_player_city_revolt_turn(
    LiveSessionState &detached_state,std::int64_t player_id,
    const content::RulesDatabase &selected_rules,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime
