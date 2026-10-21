#include "civ5/runtime/bnw_player_do_turn_prefix.hpp"
#include "civ5/runtime/bnw_player_pre_turn.hpp"
#include "civ5/runtime/bnw_unit_order_owners.hpp"
#include "civ5/runtime/bnw_live_plot_impassability.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>
#include <type_traits>
namespace civ5::runtime {
namespace {
void need(bool v,const char *why){if(!v)throw std::runtime_error(why);}
// Typed digest of the actual prefix operands/stores. Unit-order revision alone
// does NOT include player timer scalars, so cannot certify this source journal.
std::string prefix_revision(const LiveSessionState &s,const SessionCheckpointIdentity &identity,const content::RulesDatabase &rules) {
    std::vector<std::uint8_t> bytes;
    const auto integer=[&](std::int64_t v){const auto u=static_cast<std::uint64_t>(v);for(unsigned b=0;b<8;++b)bytes.push_back(static_cast<std::uint8_t>(u>>(b*8)));};
    const auto text=[&](std::string_view v){integer(v.size());bytes.insert(bytes.end(),v.begin(),v.end());};
    text(bnw_unit_order_world_revision(s,identity,rules));
    for(const auto &player:s.world.all_players->players){integer(player.saved_player_id);integer(player.core.fields.size());for(const auto &f:player.core.fields){text(f.name?f.name:"");integer(static_cast<unsigned>(f.encoding));integer(f.values.size());for(auto v:f.values)integer(v);}}
    integer(s.setup.game_type);integer(s.setup.pitboss.has_value());if(s.setup.pitboss)integer(*s.setup.pitboss);
    for(const auto &[id,p]:s.setup.players){integer(id);integer(p.alive);integer(p.slot_status);integer(p.slot_claim);integer(p.team);integer(p.handicap);}
    for(const auto *name:{"leader_keys","civilization_keys"}){const auto &keys=s.world.pregame->slots.string_arrays.at(name);text(name);integer(keys.size());for(const auto &key:keys)text(key);}
    for(const auto &player:s.world.all_players->players)for(const auto &city:player.cities.cities){integer(city.core.fields.size());for(const auto &f:city.core.fields){text(f.name?f.name:"");integer(static_cast<unsigned>(f.encoding));integer(f.values.size());for(auto v:f.values)integer(v);}}
    for(const auto &[id,p]:s.cities.players){integer(id);integer(p.capital);integer(p.cities.size());for(const auto &[city_id,c]:p.cities){integer(city_id);integer(c.owner);integer(c.x);integer(c.y);integer(c.population);integer(c.puppet);}}
    integer(s.player_culture_bomb_timers.size());for(const auto &[player,timer]:s.player_culture_bomb_timers){integer(player);integer(timer);}
    const auto &map=*s.world.map_projection;integer(map.wrap_x);integer(map.wrap_y);
    for(const auto &cell:map.cells){integer(cell.index);integer(cell.owner.has_value());if(cell.owner)integer(*cell.owner);integer(cell.plot_type.has_value());if(cell.plot_type)integer(*cell.plot_type);}
    const auto &pass=bnw_live_plot_impassability(s);text(s.plot_impassability.rules_sha256);integer(pass.plots.size());for(const auto &plot:pass.plots)integer(plot.impassable);
    return identify_bytes(bytes).sha256;
}
}
std::string bnw_player_do_turn_prefix_revision(const LiveSessionState &s,const SessionCheckpointIdentity &identity,const content::RulesDatabase &rules){return prefix_revision(s,identity,rules);}
BnwPlayerDoTurnPrefixJournal stage_bnw_player_do_turn_prefix(LiveSessionState &state,
    const content::RulesDatabase &rules,const SessionCheckpointIdentity &identity,
    std::int64_t player,std::int64_t active_player) {
    need(state.world.pregame.has_value(),"Player DoTurn prefix current active-player owner unavailable");
    const auto active=state.world.pregame->archive.integers.find("active_player");
    need(active!=state.world.pregame->archive.integers.end()&&active_player>=0&&active_player<64&&active->second==active_player,
         "Player DoTurn prefix active player changed after activation ticket");
    const auto clock=state.world.pregame->archive.integers.find("game_turn");
    need(clock!=state.world.pregame->archive.integers.end()&&clock->second>=0&&clock->second==state.setup.game_turn,"Player DoTurn prefix current game-clock mirrors disagree");
    need(!state.player_early_publication,"Player DoTurn prefix preceding timer publication still pending");
    // Cache method is actually empty (22864). AI_pre is empty ONLY after the
    // native identity assertions and actual human branch (CvPlayerAI:89-96).
    preflight_bnw_player_cache_and_ai_pre(state,player,rules,identity);
    need(state.player_culture_bomb_timers.contains(player),"Player DoTurn prefix requires SAME retained Read culture-bomb transient; no synthetic wire/default");
    const auto key=[&](const char *name){const auto &wire=state.world.pregame->slots.string_arrays.at(name).at(player);return wire.substr(0,wire.find('\0'));};
    const auto resolved=[&](const std::string &sql,const std::vector<content::DatabaseValue> &params){
        std::size_t count=0;const auto report=rules.query_rows({sql,params},[&](const auto &row){need(row.integer_at(0)>=0,"Player DoTurn native Info ID invalid");++count;},2);
        need(report.all_succeeded()&&count==1,"Player DoTurn native personality/leader/civilization Info unavailable");
    };
    std::int64_t personality=-1;
    for(const auto &saved:state.world.all_players->players)if(saved.saved_player_id==player)
        for(const auto &f:saved.core.fields)if(f.name&&std::string_view(f.name)=="m_ePersonalityType")personality=f.values[0];
    resolved("SELECT ID FROM Leaders WHERE ID=?",{personality});
    resolved("SELECT ID FROM Leaders WHERE Type=?",{key("leader_keys")});
    resolved("SELECT ID FROM Civilizations WHERE Type=?",{key("civilization_keys")});
    BnwPlayerDoTurnPrefixJournal journal;journal.player_=player;journal.turn_=state.setup.game_turn;
    journal.before_=prefix_revision(state,identity,rules);
    auto trial=state;using S=BnwPlayerDoTurnPrefixStep;
    journal.steps_={S::cache,S::human_ai_pre};
    journal.native_.scalars.culture_bomb=apply_bnw_culture_bomb_timer_turn(trial,player);
    journal.steps_.push_back(S::culture_bomb);
    journal.native_.scalars.conversion=apply_bnw_conversion_timer_turn(trial,player,active_player);
    journal.steps_.push_back(S::conversion);
    const auto &conversion=journal.native_.scalars.conversion;
    if(conversion.post_commit_dirty) {
        // Native setter changes timer THEN setDirty(MiscButtons,true), BEFORE
        // the settled-city/conscript/cramped operations. Real standalone owned
        // receiver records that exact packet and bit, without dispatching UI.
        const auto &packet=*conversion.post_commit_dirty;
        need(packet.player_id==player&&packet.bit==BnwConversionTimerDirtyBit::misc_buttons&&packet.value&&conversion.before>0&&conversion.after==conversion.before-1&&player==active_player,
             "Player DoTurn MiscButtons packet differs from native setter");
        journal.misc_receipts_.push_back({player,conversion.before,conversion.after,true});
        journal.misc_dirty_=true;journal.steps_.push_back(S::misc_buttons_dirty);
    }
    journal.native_.scalars.settled_city=apply_bnw_turns_since_settled_city_turn(trial,player);
    journal.steps_.push_back(S::settled_city);
    journal.native_.scalars.conscript=apply_bnw_conscript_count_turn(trial,player);
    journal.steps_.push_back(S::conscript);
    journal.native_.cramped=apply_bnw_player_cramped_turn(trial,player,rules,identity);
    journal.steps_.push_back(S::cramped);
    journal.after_=prefix_revision(trial,identity,rules);
    // Every fallible operation precedes the grouped stores at our caller. No
    // original session or live UI state was touched. Lua world address survives.
    static_assert(std::is_nothrow_move_assignable_v<LiveSessionState>);
    static_assert(std::is_nothrow_move_constructible_v<BnwPlayerDoTurnPrefixJournal>);
    state=std::move(trial);return journal;
}
}
