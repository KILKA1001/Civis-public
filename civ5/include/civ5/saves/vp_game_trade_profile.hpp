#pragma once
#include "civ5/saves/game_trade.hpp"

namespace civ5::saves {
// Fields serialized only by VP's trade route GameCore implementation.
struct VpTradeRouteState {
    std::int64_t origin_id = 0, destination_id = 0, speed_factor = 0;
    bool recalled = false;
};
inline VpTradeRouteState &ensure_vp_trade_route(TradeRouteObservation &route) {
    return route.profile_extensions.emplace<VpTradeRouteState>();
}
inline const VpTradeRouteState *find_vp_trade_route(const TradeRouteObservation &route) {
    return route.profile_extensions.find<VpTradeRouteState>();
}

// VP's saved route-index cache and policy-difference matrix are not required
// by BNW or an arbitrary modded GameCore profile.
struct VpGameTradeState {
    std::vector<std::vector<std::int64_t>> routes_per_player;
    std::vector<std::int64_t> policy_difference;
};
inline VpGameTradeState &ensure_vp_game_trade(GameTradeObservation &trade) {
    return trade.profile_extensions.emplace<VpGameTradeState>();
}
inline const VpGameTradeState *find_vp_game_trade(const GameTradeObservation &trade) {
    return trade.profile_extensions.find<VpGameTradeState>();
}
} // namespace civ5::saves
