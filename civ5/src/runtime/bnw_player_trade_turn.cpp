#include "civ5/runtime/bnw_player_trade_turn.hpp"
#include "civ5/runtime/live_session.hpp"
#include <algorithm>
#include <limits>
#include <string_view>
#include <utility>

namespace civ5::runtime {
namespace {
using B=BnwPlayerTradeTurnBlocker;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool ok,B blocker,const char *message) {
    if(!ok) throw BnwPlayerTradeTurnError(blocker,message);
}
bool named(const char *actual,std::string_view expected) {
    return actual && std::string_view(actual)==expected;
}
saves::NamedSaveInteger &field(saves::PlayerTradeCultureRecord &record,
                               std::string_view name) {
    saves::NamedSaveInteger *found=nullptr;
    for(auto &candidate:record.fields)
        if(named(candidate.name,name)) {
            require(!found,B::player_trade,"duplicate BNW player trade field");
            found=&candidate;
        }
    require(found,B::player_trade,"BNW player trade field unavailable");
    return *found;
}
std::uint64_t count(saves::PlayerTradeCultureRecord &record,
                    std::string_view field_name,std::string_view child_name) {
    const auto n=field(record,field_name).value;
    require(n>=0 && n<=std::numeric_limits<std::int32_t>::max(),
            B::player_trade,"BNW player trade collection count invalid");
    const auto observed=std::count_if(record.children.begin(),record.children.end(),
        [&](const auto &child){return named(child.name,child_name);});
    require(static_cast<std::uint64_t>(n)==static_cast<std::uint64_t>(observed),
            B::player_trade,"BNW player trade collection count differs from roster");
    return static_cast<std::uint64_t>(n);
}
bool empty(const saves::TradeRouteObservation &route) {
    return route.origin_x==-1 && route.origin_y==-1 &&
           route.destination_x==-1 && route.destination_y==-1;
}
} // namespace

BnwPlayerTradeTurnStep apply_bnw_player_trade_turn_without_player_routes(
    LiveSessionState &state,std::int64_t player_id) {
    require(state.setup.profile==content::TechnologyProfile::bnw &&
            state.world.game_state.layout==bnw &&
            state.world.trade.layout==bnw &&
            state.world.all_players && state.world.all_players->layout==bnw &&
            state.world.trade.dimensions.major_slots>0 &&
            state.world.trade.dimensions.yield_slots>0,
            B::profile,"BNW player trade turn requires restored Expansion2 world");
    const auto setup=state.setup.players.find(player_id);
    require(player_id>=0 &&
            static_cast<std::uint64_t>(player_id)<
                state.world.trade.dimensions.major_slots &&
            setup!=state.setup.players.end() && setup->second.alive,
            B::player,"BNW player trade turn requires a living major player");
    require(state.world.pregame && state.world.pregame->layout==bnw,
            B::clock,"BNW player trade turn requires restored pregame clock");
    const auto turn=state.world.pregame->archive.integers.find("game_turn");
    require(turn!=state.world.pregame->archive.integers.end() &&
            turn->second==state.setup.game_turn && turn->second>=0 &&
            turn->second<=std::numeric_limits<std::int32_t>::max(),
            B::clock,"BNW player trade turn clock owners differ");

    auto draft=state;
    saves::SavedPlayerObservation *saved=nullptr;
    for(auto &candidate:draft.world.all_players->players)
        if(candidate.saved_player_id==player_id) {
            require(!saved,B::player,"duplicate saved BNW player trade ID");
            saved=&candidate;
        }
    require(saved && saved->trade_culture.layout==bnw,
            B::player,"BNW player trade state unavailable");
    saves::PlayerTradeCultureRecord *trade=nullptr;
    for(auto &block:saved->trade_culture.blocks)
        if(named(block.name,"player_trade")) {
            require(!trade,B::player_trade,"duplicate BNW player trade block");
            trade=&block;
        }
    require(trade,B::player_trade,"BNW player trade block unavailable");
    const auto version=field(*trade,"version").value;
    require(version>=0 && version<=2,B::player_trade,
            "BNW player trade version unsupported");
    for(const auto &child:trade->children)
        require(named(child.name,"expired_route") ||
                named(child.name,"plundered_route"),
                B::player_trade,"unknown BNW player trade record");
    BnwPlayerTradeTurnStep result;
    result.player_id=player_id;
    if(version>=1) {
        result.expired_cleared=count(*trade,
            "m_aRecentlyExpiredConnections_wire_count","expired_route");
        trade->children.erase(std::remove_if(trade->children.begin(),trade->children.end(),
            [](const auto &child){return named(child.name,"expired_route");}),
            trade->children.end());
        field(*trade,"m_aRecentlyExpiredConnections_wire_count").value=0;
    } else {
        require(std::none_of(trade->children.begin(),trade->children.end(),
            [](const auto &child){return named(child.name,"expired_route");}),
            B::player_trade,"BNW version-zero trade has expired routes");
    }

    // UpdateTradeConnectionValues visits every nonempty route but changes
    // yields only when this player is an endpoint. MoveUnits changes only
    // routes originating from this player, EVEN coordinate-empty entries: native
    // MoveUnits does not use IsTradeRouteIndexEmpty before its owner check.
    result.inspected_routes=draft.world.trade.routes.size();
    for(const auto &route:draft.world.trade.routes)
        require(route.origin_owner!=player_id &&
                (empty(route) || route.destination_owner!=player_id),
                B::relevant_route,
                "BNW player trade route yield, movement or expiry effects unavailable");

    if(version>=2) {
        (void)count(*trade,"m_aTradeConnectionWasPlundered_wire_count",
                    "plundered_route");
        for(auto &record:trade->children) {
            if(!named(record.name,"plundered_route")) continue;
            require(record.children.size()==1 &&
                    named(record.children.front().name,"m_kTradeConnection"),
                    B::player_trade,"BNW plunder history route missing");
            auto &plundered=field(record,"m_iTurnPlundered").value;
            require(plundered>=-1 &&
                    plundered<=std::numeric_limits<std::int32_t>::max(),
                    B::player_trade,"BNW plunder turn invalid");
            if(plundered!=-1 && plundered+30<=turn->second) {
                plundered=-1;
                ++result.plundered_forgotten;
            }
        }
    } else {
        require(std::none_of(trade->children.begin(),trade->children.end(),
            [](const auto &child){return named(child.name,"plundered_route");}),
            B::player_trade,"BNW pre-v2 trade has plunder records");
    }
    state=std::move(draft);
    return result;
}
} // namespace civ5::runtime
