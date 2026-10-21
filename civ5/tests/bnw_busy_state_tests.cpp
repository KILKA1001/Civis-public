#include "civ5/runtime/bnw_busy_state.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_ui_transient_checkpoint.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <stdexcept>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool v,const char *why) {if(!v)throw std::runtime_error(why);}
template<class F>void rejects(F &&f,const char *why) {
    try{f();}catch(const std::exception&){return;}throw std::runtime_error(why);
}
std::int64_t &field(saves::UnitCoreObservation &core,std::string_view name) {
    for(auto &f:core.fields)if(f.name && std::string_view(f.name)==name)return f.values.at(0);
    throw std::runtime_error("Missing test unit field");
}
std::int64_t &tail(saves::UnitTailObservation &tail,std::string_view name) {
    for(auto &block:tail.blocks)if(block.name && std::string_view(block.name)=="state")
        for(auto &f:block.children)if(f.name && std::string_view(f.name)==name)return f.values.at(0);
    throw std::runtime_error("Missing test unit tail");
}
}
int main(int argc,char **argv) {
    check(argc==3 || argc==4,"Expected selected save, game root and optional SDK CvGameCoreSource directory");
    if(argc==4)for(const auto &file:kBnwBusySourceFiles)
        check(identify_file(std::filesystem::path(argv[3])/file.path).sha256==file.sha256,
              "Pinned SDK busy source bytes differ");
    auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
    check(selected.preview.host && !selected.preview.blocker,"Selected host unavailable");
    auto &host=*selected.preview.host;const auto &state=host.session().state();
    const auto owner=bind_bnw_busy_state_original_import(host.session().source(),host.identity());
    const auto observed=observe_bnw_busy_player(state,owner,host.identity(),0);
    check(!observed.busy(),"Selected Gandhi has unexpected busy objects");
    check(owner.units().contains({0,16385}) && !owner.cities().empty(),"Selected fixture unit/cities changed");
    auto timed=owner;timed.set_unit_mission_timer({0,16385},1);
    check(observe_bnw_busy_player(state,timed,host.identity(),0).busy_units==std::vector<std::int64_t>{16385},
          "Retained positive mission timer was ignored");
    timed.set_unit_mission_timer({0,16385},0);
    check(!observe_bnw_busy_player(state,timed,host.identity(),0).busy(),"Cleared mission timer remained busy");
    rejects([&]{timed.set_unit_mission_timer({0,16385},-1);},"Negative mission timer was accepted");
    rejects([&]{timed.set_unit_mission_timer({0,99999},1);},"Unknown unit acquired a runtime timer");
    auto combat=state;auto &unit=combat.world.all_players->players.at(0).units.units.front();
    check(field(unit.core,"m_iID")==16385,"Selected unit order changed");
    tail(unit.tail,"m_combatUnit.eOwner")=0;tail(unit.tail,"m_combatUnit.iID")=1;
    check(observe_bnw_busy_player(combat,owner,host.identity(),0).busy(),
          "Native low-slot reference did not resolve full-generation unit");
    tail(unit.tail,"m_combatUnit.iID")=16385;
    check(observe_bnw_busy_player(combat,owner,host.identity(),0).busy(),"Full combat unit reference failed");
    tail(unit.tail,"m_combatUnit.iID")=8193;
    check(!observe_bnw_busy_player(combat,owner,host.identity(),0).busy(),
          "Stale free-list generation resolved to a new unit");
    tail(unit.tail,"m_combatUnit.eOwner")=64;
    check(!observe_bnw_busy_player(combat,owner,host.identity(),0).busy(),"Out-of-roster reference was not native null");
    combat=state;auto &attacker=combat.world.all_players->players.at(0).units.units.front().core;
    field(attacker,"m_iAttackPlotX")=0;field(attacker,"m_iAttackPlotY")=0;
    check(observe_bnw_busy_player(combat,owner,host.identity(),0).busy(),"Attack plot was ignored");
    field(attacker,"m_bDeathDelay")=1;
    check(!observe_bnw_busy_player(combat,owner,host.identity(),0).busy(),"Delayed death still counted as attacking");
    field(attacker,"m_iAttackPlotX")=-INT32_MAX;field(attacker,"m_bDeathDelay")=0;
    check(!observe_bnw_busy_player(combat,owner,host.identity(),0).busy(),"Source invalid plot sentinel became wrapped plot");
    field(attacker,"m_iAttackPlotX")=-1;
    rejects([&]{observe_bnw_busy_player(combat,owner,host.identity(),0);},"Unchecked negative attack coordinate wrapped");
    const auto &city=state.world.all_players->players.at(0).cities.cities.front();
    std::int64_t city_id=-1;
    for(const auto &f:city.core.fields)if(f.name && std::string_view(f.name)=="m_iID")city_id=f.values.at(0);
    check(city_id>=0,"Selected city identity unavailable");
    auto city_combat=owner;city_combat.set_city_combat_unit({0,city_id},{0,1});
    check(observe_bnw_busy_player(state,city_combat,host.identity(),0).busy_cities==std::vector<std::int64_t>{city_id},
          "Actual city combat unit lookup was ignored");
    auto unit_city=owner;unit_city.set_unit_combat_city({0,16385},{0,city_id});
    check(observe_bnw_busy_player(state,unit_city,host.identity(),0).busy(),"Unit combat-city runtime link was ignored");
    const auto moves=bind_bnw_update_moves_original_import(host.session().source(),host.identity());
    auto retained=unit_city;
    retained.set_unit_mission_timer({0,16385},7);
    retained.set_city_combat_unit({0,city_id},{0,8193}); // retain stale-generation reference exactly
    const auto checkpoint=make_bnw_ui_transient_checkpoint(state,host.session().source(),host.identity(),moves,retained);
    const auto decoded=restore_bnw_ui_transient_checkpoint(checkpoint.bytes,state,host.session().source(),host.identity());
    check(decoded.busy.units().at({0,16385}).mission_timer==7 &&
          decoded.busy.units().at({0,16385}).combat_city.id==city_id &&
          decoded.busy.cities().at({0,city_id}).id==8193,
          "BNW checkpoint normalized or lost retained busy fields");
    check(observe_bnw_busy_player(state,decoded.busy,host.identity(),0).busy_units==std::vector<std::int64_t>{16385},
          "Restored busy owner changed native predicates");
    check(make_bnw_ui_transient_checkpoint(state,host.session().source(),host.identity(),decoded.moves,decoded.busy).bytes==checkpoint.bytes,
          "BNW transient checkpoint is not stable across restore");
    for(std::size_t at:std::array<std::size_t,9>{0,8,12,76,140,156,172,188,204}) {
        auto corrupt=checkpoint.bytes;corrupt.at(at)^=1;
        rejects([&]{restore_bnw_ui_transient_checkpoint(corrupt,state,host.session().source(),host.identity());},
                "Corrupt BNW transient identity/clock/schedule was accepted");
    }
    auto trailing=checkpoint.bytes;trailing.push_back(0);
    rejects([&]{restore_bnw_ui_transient_checkpoint(trailing,state,host.session().source(),host.identity());},"Trailing transient bytes accepted");
    auto truncated=checkpoint.bytes;truncated.pop_back();
    rejects([&]{restore_bnw_ui_transient_checkpoint(truncated,state,host.session().source(),host.identity());},"Truncated transient roster accepted");
    auto wrong_roster=checkpoint.bytes; // fixed V1 prefix: 8+4+128+48+16+8+512+2+16
    constexpr std::size_t roster_at=742;
    check(wrong_roster.size()>roster_at+40,"Fixture has no checkpoint unit");
    wrong_roster.at(roster_at+8)^=1;
    rejects([&]{restore_bnw_ui_transient_checkpoint(wrong_roster,state,host.session().source(),host.identity());},"Foreign full unit ID accepted");
    auto negative_timer=checkpoint.bytes;
    for(std::size_t i=0;i<8;++i)negative_timer.at(roster_at+16+i)=255;
    rejects([&]{restore_bnw_ui_transient_checkpoint(negative_timer,state,host.session().source(),host.identity());},"Negative checkpoint timer accepted");
    auto forced=moves;forced.set_force_ending_turn(true);
    rejects([&]{make_bnw_ui_transient_checkpoint(state,host.session().source(),host.identity(),forced,retained);},"Unsupported forced-turn history accepted");
    auto advanced=state;++advanced.setup.game_turn;
    rejects([&]{make_bnw_ui_transient_checkpoint(advanced,host.session().source(),host.identity(),moves,retained);},"Advanced clock acquired import transients");
    auto engine_foreign=state;engine_foreign.world.pregame->engine_guid[0]^=1;
    rejects([&]{restore_bnw_ui_transient_checkpoint(checkpoint.bytes,engine_foreign,host.session().source(),host.identity());},"Foreign engine GUID acquired transients");
    auto core_foreign=state;core_foreign.world.pregame->gamecore_guid[0]^=1;
    rejects([&]{restore_bnw_ui_transient_checkpoint(checkpoint.bytes,core_foreign,host.session().source(),host.identity());},"Foreign GameCore GUID acquired transients");

    auto foreign=state;foreign.world.map_header.guid[0]^=1;
    rejects([&]{observe_bnw_busy_player(foreign,owner,host.identity(),0);},"Foreign map reused busy transient owner");
    auto identity=host.identity();identity.payload_sha256[0]^=1;
    rejects([&]{observe_bnw_busy_player(state,owner,identity,0);},"Foreign source reused busy transient owner");
    auto continuation=host.session().source();continuation.source.container=saves::SaveContainerKind::civis;
    rejects([&]{bind_bnw_busy_state_original_import(continuation,host.identity());},"Continuation inferred import busy defaults");
    auto incomplete=state;incomplete.world.all_players->players.at(0).units.units.clear();
    rejects([&]{observe_bnw_busy_player(incomplete,owner,host.identity(),0);},"Missing unit collection appeared idle");
    check(!observe_bnw_busy_player(state,owner,host.identity(),0).busy() &&
          !host.session().readiness().can_continue(),"Queries mutated their source or enabled full turns");
    std::cout<<"PASS selected native busy predicates, mission/combat transients, free-list slot generations and source guards; no missions executed\n";
}
