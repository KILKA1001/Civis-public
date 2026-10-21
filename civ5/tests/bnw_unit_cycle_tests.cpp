#include "civ5/runtime/bnw_unit_cycle.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace civ5;
using namespace civ5::runtime;
namespace {
void need(bool v,const char *why){if(!v)throw std::runtime_error(why);}
template<class F>void refuses(F&& f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}need(rejected,"Invalid cycle accepted");}
template<class Core>void store(Core& core,std::string_view name,std::int64_t value){
    for(auto &field:core.fields)if(field.name && std::string_view(field.name)==name){need(field.values.size()==1,"Test scalar shape");field.values[0]=value;return;}
    throw std::runtime_error("Test scalar absent");
}
void ready_unit(saves::SavedUnitObservation &unit){
    store(unit.core,"m_iMoves",60);store(unit.core,"m_eActivityType",0);store(unit.core,"m_eAutomateType",-1);
    store(unit.core,"m_bDeathDelay",0);store(unit.core,"m_iAttackPlotX",-INT32_MAX);store(unit.core,"m_iAttackPlotY",-INT32_MAX);
    for(auto &block:unit.tail.blocks){
        if(block.name && std::string_view(block.name)=="m_missionQueue"){block.wire_count=0;block.values.clear();}
        if(block.name && std::string_view(block.name)=="state")for(auto &node:block.children)
            if(node.name && (std::string_view(node.name)=="m_combatUnit.eOwner" || std::string_view(node.name)=="m_combatUnit.iID"))node.values.at(0)=-1;
    }
}
void list(LiveSessionState &state,std::int64_t player,std::vector<std::int64_t> ids,std::optional<std::int64_t> count={}){
    auto &admin=state.world.all_players->players.at(static_cast<std::size_t>(player)).administration;
    for(auto &block:admin.blocks)if(block.name && std::string_view(block.name)=="pre_city"){
        for(auto &field:block.fields)if(field.name && std::string_view(field.name)=="m_UnitCycle_wire_count")
            field.value=count?*count:static_cast<std::int64_t>(ids.size());
        for(auto &array:block.arrays)if(array.name && std::string_view(array.name)=="m_UnitCycle")array.values=std::move(ids);
        return;
    }
    throw std::runtime_error("Test cycle block absent");
}
void exercise(const ui::SelectedBnwPreviewHost &host){
    auto state=host.session().state();const auto identity=host.identity();
    auto moves=bind_bnw_update_moves_original_import(host.session().source(),identity);
    auto busy=bind_bnw_busy_state_original_import(host.session().source(),identity);
    std::int64_t player=-1;
    for(std::size_t p=0;p<state.world.all_players->players.size();++p)
        if(state.setup.players.at(static_cast<std::int64_t>(p)).alive && state.world.all_players->players[p].units.units.size()>=2){player=static_cast<std::int64_t>(p);break;}
    need(player>=0,"Actual imported fixture needs two same-player units");
    auto evidence=observe_bnw_update_moves_units(state,moves,player);
    const auto a=evidence[0].id,b=evidence[1].id;
    auto &units=state.world.all_players->players.at(static_cast<std::size_t>(player)).units.units;
    ready_unit(units[0]);ready_unit(units[1]);
    busy.set_unit_mission_timer({player,a},0);busy.set_unit_mission_timer({player,b},0);
    busy.set_unit_combat_city({player,a},{-1,-1});busy.set_unit_combat_city({player,b},{-1,-1});
    const auto observe=[&](std::optional<BnwUnitOrderKey> current={},BnwUnitCycleDirection d=BnwUnitCycleDirection::forward,
                           BnwUnitCycleFilter f=BnwUnitCycleFilter::all){return observe_bnw_unit_cycle(state,moves,busy,identity,player,current,d,f);};
    const auto check=[&](const BnwUnitCycleObservation &value,std::optional<std::int64_t> id,bool wrap){
        need(value.wrapped==wrap && value.next==(id?std::optional<BnwUnitOrderKey>{{player,*id}}:std::nullopt),"Cycle source result differs");
    };
    list(state,player,{b,a});check(observe(),b,false);
    // SDK147-195: initial HeadNode stays HEAD with nullptr even backward.
    check(observe({},BnwUnitCycleDirection::backward),b,false);
    check(observe(BnwUnitOrderKey{player,b}),a,false);
    check(observe(BnwUnitOrderKey{player,a}),b,true);
    check(observe(BnwUnitOrderKey{player,b},BnwUnitCycleDirection::backward),a,true);
    check(observe(BnwUnitOrderKey{player,a},BnwUnitCycleDirection::backward),b,false);
    list(state,player,{b});check(observe(BnwUnitOrderKey{player,a}),b,true);
    check(observe(BnwUnitOrderKey{player,a},BnwUnitCycleDirection::backward),b,true);
    list(state,player,{});check(observe(),{},true);
    list(state,player,{},-7);const auto negative=observe({},BnwUnitCycleDirection::backward);
    check(negative,{},true);need(negative.source_wire_count==-7,"Signed negative wire count discarded");
    list(state,player,{a,a,b});check(observe(BnwUnitOrderKey{player,a}),a,true);
    // Same native pointer appears at the FIRST matching list node; duplicate
    // entries are not erased or sorted into a different cycle.
    const auto slot_a=a&8191;
    const auto stale=(a&8191)|((a&~std::int64_t{8191})==8192?16384:8192);
    list(state,player,{stale,-1,slot_a,b});check(observe(),a,false);
    list(state,player,{a,b});busy.set_unit_mission_timer({player,a},1);check(observe(),b,false);
    busy.set_unit_mission_timer({player,b},1);check(observe(),{},true);
    busy.set_unit_mission_timer({player,a},0);busy.set_unit_mission_timer({player,b},0);
    store(units[0].core,"m_bDeathDelay",1);
    const auto ready=observe_bnw_ready_player(state,moves,busy,identity,player);
    need(ready.units[0].ready_to_move && ready.units[0].delayed_death &&
         std::find(ready.counted_ready_ids.begin(),ready.counted_ready_ids.end(),a)==ready.counted_ready_ids.end(),
         "Fixture did not distinguish unit readiness from player count");
    check(observe(),a,false);
    store(units[0].core,"m_eUnitAIType",0);store(units[1].core,"m_eUnitAIType",2);
    check(observe({},BnwUnitCycleDirection::forward,BnwUnitCycleFilter::workers),b,false);
    store(units[1].core,"m_eUnitAIType",17);check(observe({},BnwUnitCycleDirection::forward,BnwUnitCycleFilter::workers),b,false);
    store(units[1].core,"m_eUnitAIType",4);check(observe({},BnwUnitCycleDirection::forward,BnwUnitCycleFilter::workers),{},true);
    list(state,player,{a});
    const auto old_ai=observe_bnw_update_moves_units(state,moves,player)[0].unit_ai;
    store(units[0].core,"m_eUnitAIType",INT64_MAX);refuses([&]{(void)observe();});
    store(units[0].core,"m_eUnitAIType",old_ai);
    refuses([&]{(void)observe(BnwUnitOrderKey{player+1,a});});
    refuses([&]{(void)observe(BnwUnitOrderKey{player,-1});});
    refuses([&]{(void)observe({},static_cast<BnwUnitCycleDirection>(91));});
    refuses([&]{(void)observe({},BnwUnitCycleDirection::forward,static_cast<BnwUnitCycleFilter>(91));});
    list(state,player,{a},2);refuses([&]{(void)observe();});
    list(state,player,{INT64_MAX});refuses([&]{(void)observe();});
    list(state,player,{},INT64_MAX);refuses([&]{(void)observe();});
    list(state,player,{a});
    auto wrong=identity;wrong.rules_sha256[0]=wrong.rules_sha256[0]=='0'?'1':'0';
    auto empty=state;empty.world.all_players.reset();refuses([&]{(void)observe_bnw_unit_cycle(empty,moves,busy,wrong,player,{},BnwUnitCycleDirection::forward,BnwUnitCycleFilter::all);});
    auto changed=state;changed.world.map_header.guid[0]^=1;
    refuses([&]{(void)observe_bnw_unit_cycle(changed,moves,busy,identity,player,{},BnwUnitCycleDirection::forward,BnwUnitCycleFilter::all);});
    auto malformed=state;
    for(auto &block:malformed.world.all_players->players.at(static_cast<std::size_t>(player)).administration.blocks)
        if(block.name && std::string_view(block.name)=="pre_city")for(const auto &array:block.arrays)
            if(array.name && std::string_view(array.name)=="m_UnitCycle"){auto duplicate=array;block.arrays.push_back(duplicate);break;}
    refuses([&]{(void)observe_bnw_unit_cycle(malformed,moves,busy,identity,player,{},BnwUnitCycleDirection::forward,BnwUnitCycleFilter::all);});
    BnwEndTurnRequestDraft draft(state,host.activation_owner());draft.owner.identity=identity;draft.owner.busy_state.reset();
    refuses([&]{(void)observe_bnw_unit_cycle(draft,moves,player,{},BnwUnitCycleDirection::forward,BnwUnitCycleFilter::all);});
    draft.owner.busy_state=busy;check(observe_bnw_unit_cycle(draft,moves,player,{},BnwUnitCycleDirection::forward,BnwUnitCycleFilter::all),a,false);
    // No-head Game.CycleUnits uses distance, not the cycle-list head. Fixtures
    // alter detached source coordinates only; they do not attest selectUnit.
    const auto saved_state=state;const auto saved_busy=busy;
    const auto nearest=[&](BnwUnitCycleFilter f=BnwUnitCycleFilter::all){
        return observe_bnw_cycle_start_unit(state,moves,busy,identity,player,f);
    };
    for(std::size_t i=0;i<units.size();++i){
        ready_unit(units[i]);busy.set_unit_mission_timer({player,evidence[i].id},i<2?0:1);
        busy.set_unit_combat_city({player,evidence[i].id},{-1,-1});
    }
    need(state.world.map_header.grid.width>=8 && state.world.map_header.grid.height>=8,"Nearest fixture needs map8x8");
    store(units[0].core,"m_iX",3);store(units[0].core,"m_iY",0);
    store(units[1].core,"m_iX",1);store(units[1].core,"m_iY",0);
    state.world.map_header.wrap_x=false;state.world.map_header.wrap_y=false;
    list(state,player,{a,b});need(nearest().next==std::optional<BnwUnitOrderKey>{{player,b}} && nearest().distance==1,
        "No-head lookup incorrectly selected cycle-list head");
    store(units[0].core,"m_iX",1);
    const auto lower_slot=(a&8191)<(b&8191)?a:b;
    need(nearest().next==std::optional<BnwUnitOrderKey>{{player,lower_slot}},"Equal distance did not retain first native slot");
    std::swap(units[0],units[1]);
    need(nearest().next==std::optional<BnwUnitOrderKey>{{player,lower_slot}},"Wire vector order replaced FLTA iteration");
    std::swap(units[0],units[1]);
    store(units[0].core,"m_iX",1);store(units[0].core,"m_eUnitAIType",0);
    store(units[1].core,"m_iX",3);store(units[1].core,"m_eUnitAIType",2);
    need(nearest(BnwUnitCycleFilter::workers).next==std::optional<BnwUnitOrderKey>{{player,b}},"Nearest worker ignored native UnitAI filter");
    store(units[1].core,"m_eUnitAIType",17);
    need(nearest(BnwUnitCycleFilter::workers).next==std::optional<BnwUnitOrderKey>{{player,b}},"Nearest sea worker unavailable");
    busy.set_unit_mission_timer({player,b},1);
    need(!nearest(BnwUnitCycleFilter::workers).next && !nearest(BnwUnitCycleFilter::workers).distance,"Busy worker became a candidate");
    busy.set_unit_mission_timer({player,b},0);
    store(units[0].core,"m_iX",static_cast<std::int64_t>(state.world.map_header.grid.width)-1);store(units[1].core,"m_iX",2);
    need(nearest().next==std::optional<BnwUnitOrderKey>{{player,b}},"Non-wrapped nearest distance differs");
    state.world.map_header.wrap_x=true;
    need(nearest().next==std::optional<BnwUnitOrderKey>{{player,a}} && nearest().distance==1,"Wrapped X nearest distance differs");
    state.world.map_header.wrap_x=false;state.world.map_header.wrap_y=true;
    store(units[0].core,"m_iX",0);store(units[0].core,"m_iY",static_cast<std::int64_t>(state.world.map_header.grid.height)-1);
    store(units[1].core,"m_iX",0);store(units[1].core,"m_iY",2);
    need(nearest().next==std::optional<BnwUnitOrderKey>{{player,a}} && nearest().distance==1,"Wrapped Y hex-space distance differs");
    store(units[0].core,"m_bDeathDelay",1);
    need(nearest().next==std::optional<BnwUnitOrderKey>{{player,a}},"Nearest lookup incorrectly used player counted-ready filter");
    store(units[0].core,"m_iX",-1);refuses([&]{(void)nearest();});
    store(units[0].core,"m_iX",0);
    refuses([&]{(void)nearest(static_cast<BnwUnitCycleFilter>(91));});
    auto dead=state;store(dead.world.all_players->players.at(static_cast<std::size_t>(player)).core,"m_bAlive",0);dead.setup.players.at(player).alive=false;
    dead.world.all_players->players.at(static_cast<std::size_t>(player)).units.units.clear();
    need(!observe_bnw_cycle_start_unit(dead,moves,busy,identity,player,BnwUnitCycleFilter::all).next,
        "Native dead-player branch borrowed its unit collection");
    auto stale_state=state;stale_state.world.map_header.guid[0]^=1;
    refuses([&]{(void)observe_bnw_cycle_start_unit(stale_state,moves,busy,identity,player,BnwUnitCycleFilter::all);});
    BnwEndTurnRequestDraft nearest_draft(state,host.activation_owner());nearest_draft.owner.identity=identity;
    nearest_draft.owner.busy_state.reset();refuses([&]{(void)observe_bnw_cycle_start_unit(nearest_draft,moves,player,BnwUnitCycleFilter::all);});
    nearest_draft.owner.busy_state=busy;
    need(observe_bnw_cycle_start_unit(nearest_draft,moves,player,BnwUnitCycleFilter::all).next==nearest().next,"Nearest draft substituted busy owner");
    // Game::cycleUnits distinguishes absent head from present foreign head.
    // These observations expose required effects; no UI receipt is fabricated.
    for(std::size_t i=0;i<units.size();++i){
        ready_unit(units[i]);busy.set_unit_mission_timer({player,evidence[i].id},i<2?0:1);
        busy.set_unit_combat_city({player,evidence[i].id},{-1,-1});
    }
    store(units[0].core,"m_iX",3);store(units[0].core,"m_iY",0);
    store(units[1].core,"m_iX",1);store(units[1].core,"m_iY",0);
    state.world.map_header.wrap_x=false;state.world.map_header.wrap_y=false;
    list(state,player,{a,b});
    const auto game_cycle=[&](std::optional<BnwUnitOrderKey> head={},BnwUnitCycleDirection direction=BnwUnitCycleDirection::forward){
        return observe_bnw_game_unit_cycle(state,moves,busy,identity,player,head,direction,BnwUnitCycleFilter::all);
    };
    const auto no_head=game_cycle({},BnwUnitCycleDirection::backward);
    need(no_head.branch==BnwGameUnitCycleBranch::no_selected_head && no_head.next==std::optional<BnwUnitOrderKey>{{player,b}} &&
         no_head.nearest_distance==1 && !no_head.source_wire_count && !no_head.wrapped &&
         !no_head.send_auto_moves_required && !no_head.clear_selection_required,"Game no-head branch became PlayerCycle");
    list(state,player,{},7);need(game_cycle().next==no_head.next,"No-head branch unnecessarily borrowed malformed cycle list");
    list(state,player,{a,b});
    std::optional<BnwUnitOrderKey> foreign_head;
    for(std::int64_t foreign_player=0;foreign_player<64;++foreign_player){
        if(foreign_player==player || state.world.all_players->players.at(static_cast<std::size_t>(foreign_player)).units.units.empty())continue;
        const auto foreign_units=observe_bnw_update_moves_units(state,moves,foreign_player);
        foreign_head=BnwUnitOrderKey{foreign_player,foreign_units.front().id};break;
    }
    need(foreign_head.has_value(),"Actual imported game fixture needs a foreign unit");
    const auto foreign_cycle=game_cycle(foreign_head,BnwUnitCycleDirection::backward);
    need(foreign_cycle.branch==BnwGameUnitCycleBranch::present_selected_head && !foreign_cycle.cycle_unit &&
         foreign_cycle.next==std::optional<BnwUnitOrderKey>{{player,a}} && !foreign_cycle.nearest_distance &&
         foreign_cycle.source_wire_count==2 && !foreign_cycle.wrapped && !foreign_cycle.clear_selection_required,
         "Present foreign head incorrectly used absent-head nearest branch");
    need(game_cycle(BnwUnitOrderKey{player,a}).next==std::optional<BnwUnitOrderKey>{{player,b}},"Owned head did not use ordered cycle");
    const auto wrapped_cycle=game_cycle(BnwUnitOrderKey{player,b});
    need(wrapped_cycle.wrapped && wrapped_cycle.next==std::optional<BnwUnitOrderKey>{{player,a}} &&
         !wrapped_cycle.send_auto_moves_required && !wrapped_cycle.clear_selection_required,"Wrapped cycle obligation differs");
    // ReadyToAuto requires a movable queued mission, regardless of mission
    // timer/awake/automated/deathDelay flags; automation alone is insufficient.
    busy.set_unit_mission_timer({player,a},1);busy.set_unit_mission_timer({player,b},1);
    for(auto &block:units[0].tail.blocks)if(block.name && std::string_view(block.name)=="m_missionQueue"){
        block.wire_count=1;block.values={0,0,0,0,0};
    }
    store(units[0].core,"m_eActivityType",1);store(units[0].core,"m_eAutomateType",0);store(units[0].core,"m_bDeathDelay",1);
    const auto exhausted_cycle=game_cycle(BnwUnitOrderKey{player,a});
    need(!exhausted_cycle.next && exhausted_cycle.wrapped && exhausted_cycle.send_auto_moves_required &&
         exhausted_cycle.clear_selection_required && exhausted_cycle.cycle_unit==std::optional<BnwUnitOrderKey>{{player,a}},
         "Exhausted own-head branch lost mandatory auto-moves/clear/center-unit effects");
    const auto foreign_exhausted=game_cycle(foreign_head);
    need(foreign_exhausted.wrapped && foreign_exhausted.send_auto_moves_required && !foreign_exhausted.clear_selection_required,
         "Foreign head incorrectly cleared owned selection or lost auto-moves");
    const auto absent_exhausted=game_cycle();
    need(!absent_exhausted.next && !absent_exhausted.wrapped && !absent_exhausted.send_auto_moves_required &&
         !absent_exhausted.clear_selection_required,"Absent head executed present-head obligations");
    store(units[0].core,"m_iMoves",0);
    need(!game_cycle(BnwUnitOrderKey{player,a}).send_auto_moves_required,"Zero moves still requested auto-moves");
    ready_unit(units[0]);store(units[0].core,"m_eAutomateType",0);
    need(!game_cycle(BnwUnitOrderKey{player,a}).send_auto_moves_required,"Automation without queued mission invented auto-moves");
    refuses([&]{(void)game_cycle(BnwUnitOrderKey{player,INT32_MAX});});
    refuses([&]{(void)game_cycle(BnwUnitOrderKey{64,0});});
    refuses([&]{(void)game_cycle({},static_cast<BnwUnitCycleDirection>(91));});
    BnwEndTurnRequestDraft game_draft(state,host.activation_owner());game_draft.owner.identity=identity;
    game_draft.owner.busy_state.reset();
    refuses([&]{(void)observe_bnw_game_unit_cycle(game_draft,moves,player,{},BnwUnitCycleDirection::forward,BnwUnitCycleFilter::all);});
    game_draft.owner.busy_state=busy;
    need(observe_bnw_game_unit_cycle(game_draft,moves,player,{},BnwUnitCycleDirection::forward,BnwUnitCycleFilter::all).next==game_cycle().next,
         "Game cycle draft substituted its retained busy owner");
    state=saved_state;busy=saved_busy;
    const auto original=host.session().state().random.gameplay.state();(void)observe();
    need(host.session().state().random.gameplay.state()==original && !host.session().readiness().can_continue(),"Pure getter granted continuation or changed genuine RNG");
}
}
int main(int argc,char **argv){try{
    need(argc==3,"Supply actual BNW save and shared installation view");
    const auto before=identify_file(argv[1]).sha256;
    auto bootstrap=ui::bootstrap_selected_bnw_preview(argv[1],std::filesystem::path(argv[2]),0);
    if(bootstrap.preview.blocker)throw std::runtime_error(bootstrap.preview.blocker->reason);
    need(static_cast<bool>(bootstrap.preview.host),"Actual selected host absent");
    exercise(*bootstrap.preview.host);
    need(identify_file(argv[1]).sha256==before,"Original save changed");
    std::cout<<"BNW pure source-ordered unit cycle PASS; selection/engine phase unavailable\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
