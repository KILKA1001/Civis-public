#pragma once

#include "civ5/runtime/bnw_player_trade_turn.hpp"
#include <cstdint>
#include <stdexcept>

namespace civ5::runtime {
struct LiveSessionState;

enum class BnwMilitaryResetBlocker { profile, player, strategy };
class BnwMilitaryResetError : public std::runtime_error {
  public:
    BnwMilitaryResetError(BnwMilitaryResetBlocker blocker,const char *message)
        : std::runtime_error(message),blocker_(blocker) {}
    BnwMilitaryResetBlocker blocker() const noexcept {return blocker_;}
  private:
    BnwMilitaryResetBlocker blocker_;
};
struct BnwMilitaryResetStep {
    std::int64_t player_id=-1,army_type_before=-1;
};

// Expansion2 CvMilitaryAI::ResetCounters at CvPlayer::doTurn, immediately
// after CvPlayerTrade::DoTurn. Resets the saved army type and the two unsaved
// request counters. Does not run GrandStrategyAI or DiplomacyAI.
BnwMilitaryResetStep apply_bnw_player_military_reset_counters(
    LiveSessionState &detached_state,std::int64_t player_id);

struct BnwPlayerTradeStrategyPrefix {
    BnwPlayerTradeTurnStep trade;
    BnwMilitaryResetStep military;
};

// Atomic source-order prefix of player_trade_strategy_diplomacy. The remaining
// GrandStrategy, Diplomacy, request, and post-diplomacy effects are mandatory
// before the enclosing turn can commit.
BnwPlayerTradeStrategyPrefix apply_bnw_player_trade_and_military_prefix(
    LiveSessionState &detached_state,std::int64_t player_id);

} // namespace civ5::runtime
