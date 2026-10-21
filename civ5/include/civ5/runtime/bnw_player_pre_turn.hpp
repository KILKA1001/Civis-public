#pragma once

#include "civ5/runtime/bnw_player_scalar_prefix.hpp"
#include "civ5/runtime/bnw_turn_scheduler.hpp"
#include <array>
#include <functional>
#include <stdexcept>

namespace civ5::runtime {

enum class BnwPlayerPreTurnBlocker {
    profile, selected_rules, player, pregame_identity, ai_pre_effects
};

class BnwPlayerPreTurnError : public std::runtime_error {
  public:
    BnwPlayerPreTurnError(BnwPlayerPreTurnBlocker blocker,const char *message)
        : std::runtime_error(message),blocker_(blocker) {}
    BnwPlayerPreTurnBlocker blocker() const noexcept {return blocker_;}
  private:
    BnwPlayerPreTurnBlocker blocker_;
};

// Expansion2 CvPlayer::doUpdateCacheOnTurn is empty. The following
// CvPlayerAI::AI_doTurnPre asserts leader/civilization identity, then returns
// immediately for human players. AI players still require found-value,
// research and annexation effects and are rejected before any state change.
void preflight_bnw_player_cache_and_ai_pre(
    const LiveSessionState &state,std::int64_t player_id,
    const content::RulesDatabase &selected_rules,
    const SessionCheckpointIdentity &identity);

// Covers the source-order no-op human prelude and the detached early segment
// through the Maya check. Later player phases and full turn publication remain
// unavailable.
BnwPlayerEarlyTurnPrefix apply_bnw_human_player_early_turn(
    LiveSessionState &detached_state,std::int64_t player_id,
    std::int64_t active_player_id,
    const content::RulesDatabase &selected_rules,
    const SessionCheckpointIdentity &identity);

// Bind the first three CvPlayer::doTurn phases to the full-turn scheduler.
struct BnwPlayerEarlyTurnServices {
    std::function<void(std::int64_t,BnwConversionTimerDirtyBit)> dirty_after_commit;
};

// Selected rules must outlive the handlers. An active-player conversion timer
// change stages its UI dirty signal in the owned state. The scheduler sends it
// only after the complete turn commits, and rejects the phase if no service
// can deliver it.
std::array<BnwTurnHandler,3> make_bnw_human_player_early_turn_handlers(
    const content::RulesDatabase &selected_rules,
    SessionCheckpointIdentity identity,std::int64_t active_player_id,
    BnwPlayerEarlyTurnServices services={});

LiveActionResult publish_bnw_player_early_turn(
    LiveSession &session,const BnwPlayerEarlyTurnServices &services);

} // namespace civ5::runtime
