#include "civ5/runtime/bnw_player_trade_strategy_prefix.hpp"
#include "civ5/runtime/live_session.hpp"
#include <string_view>
#include <utility>

namespace civ5::runtime {
namespace {
using B=BnwMilitaryResetBlocker;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool ok,B blocker,const char *message) {
    if(!ok) throw BnwMilitaryResetError(blocker,message);
}
bool named(const char *actual,std::string_view expected) {
    return actual && std::string_view(actual)==expected;
}
saves::NamedSaveInteger &field(saves::PlayerStrategyBlock &block,
                               std::string_view name) {
    saves::NamedSaveInteger *found=nullptr;
    for(auto &candidate:block.fields)
        if(named(candidate.name,name)) {
            require(!found,B::strategy,"duplicate BNW military AI field");
            found=&candidate;
        }
    require(found,B::strategy,"BNW military AI field unavailable");
    return *found;
}
BnwMilitaryResetStep reset_in_place(LiveSessionState &state,std::int64_t player_id) {
    require(state.setup.profile==content::TechnologyProfile::bnw &&
            state.world.game_state.layout==bnw && state.world.all_players &&
            state.world.all_players->layout==bnw,
            B::profile,"BNW military reset requires restored Expansion2 world");
    require(player_id>=0 &&
            static_cast<std::uint64_t>(player_id)<
                state.world.all_players->context.strategy.major,
            B::player,"BNW military reset requires major player ID");
    const auto setup=state.setup.players.find(player_id);
    require(setup!=state.setup.players.end() && setup->second.alive,
            B::player,"BNW military reset requires living player");
    saves::SavedPlayerObservation *saved=nullptr;
    for(auto &candidate:state.world.all_players->players)
        if(candidate.saved_player_id==player_id) {
            require(!saved,B::player,"duplicate saved BNW military AI player");
            saved=&candidate;
        }
    require(saved && saved->strategy.layout==bnw,
            B::player,"BNW military AI strategy state unavailable");
    saves::PlayerStrategyBlock *military=nullptr;
    for(auto &block:saved->strategy.blocks)
        if(named(block.name,"military_ai")) {
            require(!military,B::strategy,"duplicate BNW military AI block");
            military=&block;
        }
    require(military,B::strategy,"BNW military AI block unavailable");
    require(field(*military,"version").value==1,
            B::strategy,"BNW military AI version unsupported");
    auto &army=field(*military,"army_type_building").value;
    require(army>=-1 && army<=1,B::strategy,"BNW army type invalid");
    const BnwMilitaryResetStep result{player_id,army};
    // Source inline ResetCounters writes both unsaved fields before army.
    // Absent retained counters become known HERE by real overwrite, never
    // an invented constructor/Read initial value. Outer trial is atomic.
    auto &counters=state.military_request_counters[player_id];
    counters.land=0;
    counters.naval=0;
    army=-1; // NO_ARMY_TYPE
    return result;
}
} // namespace

BnwMilitaryResetStep apply_bnw_player_military_reset_counters(
    LiveSessionState &state,std::int64_t player_id) {
    auto draft=state;
    auto result=reset_in_place(draft,player_id);
    state=std::move(draft);
    return result;
}

BnwPlayerTradeStrategyPrefix apply_bnw_player_trade_and_military_prefix(
    LiveSessionState &state,std::int64_t player_id) {
    auto draft=state;
    BnwPlayerTradeStrategyPrefix result;
    result.trade=apply_bnw_player_trade_turn_without_player_routes(draft,player_id);
    result.military=reset_in_place(draft,player_id);
    state=std::move(draft);
    return result;
}
} // namespace civ5::runtime
