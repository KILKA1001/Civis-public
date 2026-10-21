#pragma once

#include <cstdint>
#include <stdexcept>

namespace civ5::runtime {
struct LiveSessionState;

enum class BnwPlayerTradeTurnBlocker {
    profile, player, clock, player_trade, relevant_route
};

class BnwPlayerTradeTurnError : public std::runtime_error {
  public:
    BnwPlayerTradeTurnError(BnwPlayerTradeTurnBlocker blocker,const char *message)
        : std::runtime_error(message),blocker_(blocker) {}
    BnwPlayerTradeTurnBlocker blocker() const noexcept {return blocker_;}
  private:
    BnwPlayerTradeTurnBlocker blocker_;
};

struct BnwPlayerTradeTurnStep {
    std::int64_t player_id=-1;
    std::uint64_t expired_cleared=0,plundered_forgotten=0;
    std::uint64_t inspected_routes=0;
};

// Expansion2 CvPlayerTrade::DoTurn. Handles the complete source branch when
// no route (even coordinate-empty) originates from this player and no active
// route has this player as destination: clear recently
// expired routes, refresh (zero relevant) route values, age plunder records,
// then move (zero owned) trade units. A relevant route needs the full yield,
// movement and unit lifecycle implementations and fails closed. This is only
// the trade call inside player_trade_strategy_diplomacy, not the whole phase.
// Mutations are atomic on the supplied detached state.
BnwPlayerTradeTurnStep apply_bnw_player_trade_turn_without_player_routes(
    LiveSessionState &detached_state,std::int64_t player_id);

} // namespace civ5::runtime
