#include "civ5/runtime/bnw_ready_units.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F &&f,const char *why){try{f();}catch(const std::exception&){return;}throw std::runtime_error(why);}
std::int64_t &scalar(saves::UnitCoreObservation &core,std::string_view name) {
    for(auto &field:core.fields)if(field.name && std::string_view(field.name)==name && field.values.size()==1)return field.values[0];
    throw std::runtime_error("Missing ready-unit fixture scalar");
}
saves::UnitTailNode &queue(saves::SavedUnitObservation &unit) {
    for(auto &node:unit.tail.blocks)if(node.name && std::string_view(node.name)=="m_missionQueue")return node;
    throw std::runtime_error("Missing ready-unit fixture queue");
}
}
int main(int argc,char **argv) {
    check(argc==3 || argc==4,"Expected selected save, game root and optional SDK CvGameCoreSource");
    if(argc==4)for(const auto &file:kBnwUpdateMovesSourceFiles)
        check(identify_file(std::filesystem::path(argv[3])/file.path).sha256==file.sha256,"SDK ready-unit source pin differs");
    auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
    check(selected.preview.host && !selected.preview.blocker,"Selected BNW unavailable");
    auto &host=*selected.preview.host;const auto player=host.active_player_id();const auto &state=host.session().state();
    const auto moves=bind_bnw_update_moves_original_import(host.session().source(),host.identity());
    const auto busy=bind_bnw_busy_state_original_import(host.session().source(),host.identity());
    const auto baseline=observe_bnw_ready_player(state,moves,busy,host.identity(),player);
    const auto evidence=observe_bnw_update_moves_units(state,moves,player);
    check(baseline.units.size()==evidence.size() && !evidence.empty(),"Real selected readiness evidence lacks actual units");
    auto awake=state;auto &unit=awake.world.all_players->players.at(player).units.units.front();
    const auto id=scalar(unit.core,"m_iID");
    scalar(unit.core,"m_iMoves")=60;scalar(unit.core,"m_eActivityType")=0;
    scalar(unit.core,"m_eAutomateType")=-1;scalar(unit.core,"m_bDeathDelay")=0;
    queue(unit).wire_count=0;queue(unit).values.clear();
    const auto ready=observe_bnw_ready_player(awake,moves,busy,host.identity(),player);
    check(ready.units.front().id==id && ready.units.front().ready_to_move && !ready.units.front().ready_to_auto &&
          ready.has_ready_unit && std::find(ready.counted_ready_ids.begin(),ready.counted_ready_ids.end(),id)!=ready.counted_ready_ids.end(),
          "Actual idle awake unit was not ready/counted");
    auto busy_timer=busy;busy_timer.set_unit_mission_timer({player,id},1);
    check(!observe_bnw_ready_player(awake,moves,busy_timer,host.identity(),player).units.front().ready_to_move,
          "Retained positive mission timer was skipped");
    auto dying=awake;scalar(dying.world.all_players->players.at(player).units.units.front().core,"m_bDeathDelay")=1;
    const auto death=observe_bnw_ready_player(dying,moves,busy,host.identity(),player);
    check(death.units.front().ready_to_move && death.units.front().delayed_death &&
          std::find(death.counted_ready_ids.begin(),death.counted_ready_ids.end(),id)==death.counted_ready_ids.end(),
          "Player dying-unit exclusion incorrectly changed native unit ReadyToMove");
    auto mission=awake;auto &ordered=mission.world.all_players->players.at(player).units.units.front();
    queue(ordered).wire_count=1;queue(ordered).values={0,0,0,0,0};
    const auto ordered_ready=observe_bnw_ready_player(mission,moves,busy_timer,host.identity(),player);
    check(!ordered_ready.units.front().ready_to_move && ordered_ready.units.front().ready_to_auto && ordered_ready.has_auto_unit,
          "ReadyToAuto incorrectly applied native ReadyToMove busy/queue filters");
    scalar(ordered.core,"m_bDeathDelay")=1;scalar(ordered.core,"m_eAutomateType")=0;scalar(ordered.core,"m_eActivityType")=1;
    check(observe_bnw_ready_player(mission,moves,busy_timer,host.identity(),player).units.front().ready_to_auto,
          "ReadyToAuto invented awake/automation/delayed-death checks");
    scalar(ordered.core,"m_iMoves")=0;
    check(!observe_bnw_ready_player(mission,moves,busy,host.identity(),player).units.front().ready_to_auto,
          "Movement-exhausted unit was ready to auto");
    auto sleep=awake;scalar(sleep.world.all_players->players.at(player).units.units.front().core,"m_eActivityType")=1;
    check(!observe_bnw_ready_player(sleep,moves,busy,host.identity(),player).units.front().ready_to_move,"Sleeping unit was ready to move");
    auto automated=awake;scalar(automated.world.all_players->players.at(player).units.units.front().core,"m_eAutomateType")=0;
    check(!observe_bnw_ready_player(automated,moves,busy,host.identity(),player).units.front().ready_to_move,"Automated unit was ready to move");
    auto missing=awake;missing.world.all_players->players.at(player).units.units.front().tail.blocks.clear();
    rejects([&]{observe_bnw_ready_player(missing,moves,busy,host.identity(),player);},"Unknown mission queue was inferred empty by readiness");
    auto wrong=awake;wrong.world.map_header.guid[0]^=1;
    rejects([&]{observe_bnw_ready_player(wrong,moves,busy,host.identity(),player);},"Foreign map readiness was accepted");
    auto identity=host.identity();identity.rules_sha256[0]^=1;
    rejects([&]{observe_bnw_ready_player(awake,moves,busy,identity,player);},"Foreign rules readiness was accepted");
    BnwGameActivationOwner activation{host.identity()};
    const auto input=bind_bnw_end_turn_import(host.session().source(),activation);
    activation.busy_state=busy_timer; // retained mission_timer=1, not fresh default
    const auto revision=bnw_end_turn_input_revision(awake,activation,input,host.database(),player);
    const auto end=execute_bnw_end_turn_request_draft(awake,activation,input,host.database(),{player,awake.setup.game_turn,true,revision});
    check(bool(end.draft) && !observe_bnw_ready_player(*end.draft,moves,player).units.front().ready_to_move,
          "End-turn readiness rebuilt retained busy state from zero defaults");
    auto absent=*end.draft;absent.owner.busy_state.reset();
    rejects([&]{observe_bnw_ready_player(absent,moves,player);},"Absent end-turn busy owner was fabricated");
    const auto unchanged=observe_bnw_ready_player(state,moves,busy,host.identity(),player);
    check(unchanged.counted_ready_ids==baseline.counted_ready_ids,"Readiness queries mutated the real source world/owners");
    std::cout<<"PASS selected native ReadyToMove/ReadyToAuto/GetCountReadyUnits with actual busy owner; baseline_ready="<<baseline.count_ready
             <<" moves="<<evidence.front().moves<<" activity="<<evidence.front().activity<<" automate="<<evidence.front().automate<<"\n";
}
