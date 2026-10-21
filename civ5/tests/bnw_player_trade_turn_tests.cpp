#include "civ5/runtime/bnw_player_trade_turn.hpp"
#include "civ5/runtime/bnw_player_trade_strategy_prefix.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
using namespace civ5::runtime;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void check(bool ok,const char *message) {
    if(!ok) throw std::runtime_error(message);
}
std::int64_t field(const saves::PlayerTradeCultureRecord &record,
                   std::string_view name) {
    for(const auto &entry:record.fields)
        if(entry.name && std::string_view(entry.name)==name) return entry.value;
    throw std::runtime_error("fixture player trade field missing");
}
std::int64_t field(const saves::PlayerStrategyBlock &block,
                   std::string_view name) {
    for(const auto &entry:block.fields)
        if(entry.name && std::string_view(entry.name)==name) return entry.value;
    throw std::runtime_error("fixture military AI field missing");
}
LiveSessionState fixture() {
    LiveSessionState state{saves::SavedSessionSnapshot{},ResearchContext{}, {},
        PlayerResearchContext{},CityYieldContext{},BnwUnhappinessContext{},
        MaintenanceContext{},TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{1,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{2,0,0})}};
    state.setup.profile=content::TechnologyProfile::bnw;
    state.setup.players[5].alive=true;
    state.setup.game_turn=40;
    state.world.game_state.layout=state.world.trade.layout=bnw;
    state.world.trade.dimensions={22,6};
    auto &pregame=state.world.pregame.emplace();pregame.layout=bnw;
    pregame.archive.integers["game_turn"]=40;
    auto &all=state.world.all_players.emplace();all.layout=bnw;
    auto &saved=all.players.emplace_back();saved.saved_player_id=5;
    saved.strategy.layout=bnw;
    auto &military=saved.strategy.blocks.emplace_back();
    military.name="military_ai";
    military.fields={{"version",1},{"army_type_building",1}};
    saved.trade_culture.layout=bnw;
    auto &trade=saved.trade_culture.blocks.emplace_back();
    trade.name="player_trade";
    trade.fields={{"version",2},
                  {"m_aRecentlyExpiredConnections_wire_count",1},
                  {"m_aTradeConnectionWasPlundered_wire_count",2}};
    auto &expired=trade.children.emplace_back();expired.name="expired_route";
    for(const auto turn:{10,20}) {
        auto &record=trade.children.emplace_back();
        record.name="plundered_route";
        record.fields={{"m_iTurnPlundered",turn}};
        record.children.emplace_back().name="m_kTradeConnection";
    }
    auto &empty=state.world.trade.routes.emplace_back();
    empty.origin_x=empty.origin_y=empty.destination_x=empty.destination_y=-1;
    auto &other=state.world.trade.routes.emplace_back();
    other.origin_x=1;other.origin_y=2;
    other.destination_x=3;other.destination_y=4;
    other.origin_owner=6;other.destination_owner=7;
    state.military_request_counters[5]={2,3};
    return state;
}
const saves::PlayerTradeCultureRecord &trade(const LiveSessionState &state) {
    return state.world.all_players->players.front().trade_culture.blocks.front();
}
void synthetic() {
    auto state=fixture();
    const auto step=apply_bnw_player_trade_turn_without_player_routes(state,5);
    check(step.player_id==5 && step.expired_cleared==1 &&
          step.plundered_forgotten==1 && step.inspected_routes==2 &&
          field(trade(state),"m_aRecentlyExpiredConnections_wire_count")==0 &&
          trade(state).children.size()==2 &&
          field(trade(state).children[0],"m_iTurnPlundered")==-1 &&
          field(trade(state).children[1],"m_iTurnPlundered")==20 &&
          state.random.gameplay.state().calls==0,
          "BNW no-player-route trade turn differs from source order");
    for(const bool origin:{true,false}) {
        auto blocked=fixture();
        auto &route=blocked.world.trade.routes.back();
        if(origin) route.origin_owner=5;
        else route.destination_owner=5;
        bool relevant=false;
        try {(void)apply_bnw_player_trade_turn_without_player_routes(blocked,5);}
        catch(const BnwPlayerTradeTurnError &error) {
            relevant=error.blocker()==BnwPlayerTradeTurnBlocker::relevant_route;
        }
        check(relevant && trade(blocked).children.size()==3 &&
              field(trade(blocked),"m_aRecentlyExpiredConnections_wire_count")==1 &&
              field(trade(blocked).children[1],"m_iTurnPlundered")==10,
              "relevant BNW route did not roll back earlier trade effects");
    }
    // Native MoveUnits checks origin before emptiness, and MoveUnit always
    // calls visual direction. The generic caller must reject this boundary too.
    auto empty_owned=fixture();empty_owned.world.trade.routes.front().origin_owner=5;
    bool empty_blocked=false;
    try {(void)apply_bnw_player_trade_turn_without_player_routes(empty_owned,5);}
    catch(const BnwPlayerTradeTurnError &error){empty_blocked=error.blocker()==BnwPlayerTradeTurnBlocker::relevant_route;}
    check(empty_blocked&&trade(empty_owned).children.size()==3&&field(trade(empty_owned),"m_aRecentlyExpiredConnections_wire_count")==1&&field(trade(empty_owned).children[1],"m_iTurnPlundered")==10&&empty_owned.random.gameplay.state().calls==0,"Empty own-origin native MoveUnit/visual barrier was skipped or leaked earlier writes");
    auto malformed=fixture();
    malformed.world.all_players->players.front().trade_culture.blocks.front().
        fields[1].value=2;
    bool count_blocked=false;
    try {(void)apply_bnw_player_trade_turn_without_player_routes(malformed,5);}
    catch(const BnwPlayerTradeTurnError &error) {
        count_blocked=error.blocker()==BnwPlayerTradeTurnBlocker::player_trade;
    }
    check(count_blocked && trade(malformed).children.size()==3,
          "malformed BNW expired-route count changed state");
    auto wrong_clock=fixture();
    wrong_clock.world.pregame->archive.integers["game_turn"]=41;
    bool clock_blocked=false;
    try {(void)apply_bnw_player_trade_turn_without_player_routes(wrong_clock,5);}
    catch(const BnwPlayerTradeTurnError &error) {
        clock_blocked=error.blocker()==BnwPlayerTradeTurnBlocker::clock;
    }
    check(clock_blocked,"BNW trade turn accepted mismatched clock owners");
    auto combined=fixture();
    const auto prefix=apply_bnw_player_trade_and_military_prefix(combined,5);
    check(prefix.trade.expired_cleared==1 &&
          prefix.military.player_id==5 && prefix.military.army_type_before==1 &&
          field(trade(combined),"m_aRecentlyExpiredConnections_wire_count")==0 &&
          field(combined.world.all_players->players.front().strategy.blocks.front(),
                "army_type_building")==-1 &&
          combined.military_request_counters.at(5).land==0 &&
          combined.military_request_counters.at(5).naval==0,
          "BNW trade/military source-order prefix did not reset owned counters");
    auto missing_military=fixture();
    missing_military.world.all_players->players.front().strategy.blocks.clear();
    bool military_blocked=false;
    try {(void)apply_bnw_player_trade_and_military_prefix(missing_military,5);}
    catch(const BnwMilitaryResetError &error) {
        military_blocked=error.blocker()==BnwMilitaryResetBlocker::strategy;
    }
    check(military_blocked && trade(missing_military).children.size()==3 &&
          field(trade(missing_military),"m_aRecentlyExpiredConnections_wire_count")==1 &&
          missing_military.military_request_counters.at(5).land==2,
          "late military rejection leaked earlier BNW trade mutations");
    auto legacy=fixture();
    auto &legacy_trade=legacy.world.all_players->players.front().trade_culture.blocks.front();
    legacy_trade.fields={{"version",0},{"legacy_empty_value",0}};
    legacy_trade.children.clear();
    const auto legacy_step=apply_bnw_player_trade_and_military_prefix(legacy,5);
    check(legacy_step.trade.expired_cleared==0 &&
          legacy_step.trade.plundered_forgotten==0 &&
          legacy_step.military.army_type_before==1,
          "BNW version-zero player trade branch failed");
    auto version_one=fixture();
    auto &v1=version_one.world.all_players->players.front().trade_culture.blocks.front();
    v1.fields={{"version",1},{"m_aRecentlyExpiredConnections_wire_count",1}};
    v1.children.erase(v1.children.begin()+1,v1.children.end());
    const auto v1_step=apply_bnw_player_trade_turn_without_player_routes(version_one,5);
    check(v1_step.expired_cleared==1 && v1_step.plundered_forgotten==0 &&
          trade(version_one).children.empty(),
          "BNW version-one player trade branch failed");
}
void selected(const std::filesystem::path &save,const std::filesystem::path &root) {
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,root,0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.preview.host!=nullptr,"selected BNW host unavailable");
    auto &host=*bootstrap.preview.host;
    const auto id=host.active_player_id();
    const auto lease=host.session().read_state_lease();
    const auto &routes=lease->world.trade.routes;
    const auto relevant=std::any_of(routes.begin(),routes.end(),[&](const auto &route) {
        const bool empty=route.origin_x==-1 && route.origin_y==-1 &&
                         route.destination_x==-1 && route.destination_y==-1;
        return route.origin_owner==id || (!empty && route.destination_owner==id);
    });
    auto detached=*lease;
    if(relevant) {
        bool blocked=false;
        try {(void)apply_bnw_player_trade_turn_without_player_routes(detached,id);}
        catch(const BnwPlayerTradeTurnError &error) {
            blocked=error.blocker()==BnwPlayerTradeTurnBlocker::relevant_route;
        }
        check(blocked,"selected BNW active trade route was silently skipped");
    } else {
        const auto prefix=apply_bnw_player_trade_and_military_prefix(detached,id);
        check(prefix.trade.player_id==id &&
              prefix.trade.inspected_routes==routes.size() &&
              prefix.military.player_id==id,
              "selected BNW trade/military source-order prefix failed");
    }
    std::cout << "selected BNW trade routes=" << routes.size()
              << " relevant=" << relevant << '\n';
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==1 || argc==3,
              "Supply no paths or selected BNW save and installed game root");
        synthetic();
        if(argc==3) selected(argv[1],argv[2]);
        std::cout << "PASS BNW player trade no-route turn\n";
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW player trade no-route turn: " << error.what() << '\n';
        return 1;
    }
}
