#include "civ5/runtime/detail/bnw_dynamic_turn_mode.hpp"
#include "civ5/runtime/bnw_retained_turn_cursor.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <type_traits>
using namespace civ5;
using namespace civ5::runtime;
namespace {
void require(bool v,const char* why){if(!v)throw std::runtime_error(why);}
template<class F>void refuses(F&& f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}require(rejected,"Required refusal absent");}
auto& member(LiveSessionState& s,std::size_t slot,std::string_view name){
 for(auto& f:s.world.all_players->players[slot].core.fields)if(f.name&&std::string_view(f.name)==name)return f;
 throw std::runtime_error("Actual player field absent");
}
std::int64_t named(const auto& fields,std::string_view name){
 for(const auto& f:fields)if(f.name&&std::string_view(f.name)==name)return f.value;
 throw std::runtime_error("Actual team field absent");
}
auto& war(LiveSessionState& s,std::size_t slot){
 for(auto& a:s.world.teams->teams[slot].arrays)if(a.name&&std::string_view(a.name)=="at_war")return a.values;
 throw std::runtime_error("Actual war array absent");
}
// Independent SDK oracle: collect human teams first, then use actual directed
// war rows. Does not call the production capture, human predicate or plan.
std::array<bool,64> expected_modes(LiveSessionState& s,std::int64_t pitboss){
 std::array<bool,64> human{},simultaneous{};simultaneous.fill(true);
 const auto& pg=s.world.pregame->archive.integer_arrays;
 for(std::size_t slot=0;slot<64;++slot){
  if(!member(s,slot,"m_bAlive").values.at(0)||member(s,slot,"m_eID").values.at(0)==-1)continue;
  const auto status=pg.at("slot_status").at(slot),claim=pg.at("slot_claims").at(slot),team=pg.at("team_types").at(slot);
  if(status==3||(status==0&&pitboss!=0)||(status==4&&claim==2))human.at(static_cast<std::size_t>(team))=true;
 }
 for(std::size_t team=0;team<64;++team)if(human[team]){
  for(std::size_t opponent=0;opponent<64;++opponent)
   if(named(s.world.teams->teams[opponent].fields,"alive_count")>0&&human[opponent]&&war(s,team).at(opponent)){
    simultaneous[team]=false;break;
   }
 }
 return simultaneous;
}
void set_equal_inputs(LiveSessionState& s,const std::array<bool,64>& expected){
 const auto& teams=s.world.pregame->archive.integer_arrays.at("team_types");
 for(std::size_t slot=0;slot<64;++slot)if(member(s,slot,"m_bAlive").values.at(0))
  member(s,slot,"m_bDynamicTurnsSimultMode").values.at(0)=expected.at(static_cast<std::size_t>(teams.at(slot)))?1:0;
}
void unchanged64(LiveSessionState& s,std::int64_t pitboss){
 const auto expected=expected_modes(s,pitboss);set_equal_inputs(s,expected);
 const auto before=detail::capture_dynamic_mode_operands(s);
 const auto map=s.random.map.state(),game=s.random.gameplay.state();const auto revision=s.ui_source_revision;const auto turn=s.setup.game_turn;
 std::uint64_t living=0;for(const auto& p:before.players)living+=p.alive?1u:0u;
 auto plan=detail::DynamicModeComponentPlan::prepare(s,pitboss);plan.require_current(s,false);
 require(plan.teams_visited()==64&&plan.returned_setters()==living&&plan.stores()==0&&!plan.blocked(),"Native64-team unchanged traversal incomplete");
 std::int32_t last_team=-1,last_player=-1;
 for(const auto& call:plan.calls()){
  require(call.team>last_team||(call.team==last_team&&call.player>last_player),"Native team/player traversal order differs");
  require(call.before==call.requested&&!call.notification_required,"Equal branch looked up notification/store");
  last_team=call.team;last_player=call.player;
 }
 require(plan.apply_prepared_without_callbacks(s),"Actual equal plan refused");plan.require_current(s,true);
 require(!plan.apply_prepared_without_callbacks(s),"Repeated component plan replayed stores");
 require(detail::capture_dynamic_mode_operands(s)==before&&s.ui_source_revision==revision&&s.setup.game_turn==turn&&
  s.random.map.state()==map&&s.random.gameplay.state()==game,"Equal component changed world/clock/RNG");
}
void component_cases(const LiveSessionState& imported){
 // These are explicit controlled component inputs copied from a real selected
 // save, not playable Session/VM/cursor issuers or elapsed-turn receipts.
 auto state=imported;unchanged64(state,0);unchanged64(state,2);
 // Controlled ordinary-game OPEN/PitBoss and assigned OBSERVER inputs test
 // native human-team war semantics, not a guessed always-human OPEN slot.
 auto human_case=imported;std::optional<std::size_t> open,observer;
 auto& statuses=human_case.world.pregame->archive.integer_arrays.at("slot_status");
 auto& claims=human_case.world.pregame->archive.integer_arrays.at("slot_claims");
 const auto& teams=human_case.world.pregame->archive.integer_arrays.at("team_types");
 for(std::size_t slot=0;slot<64;++slot)if(member(human_case,slot,"m_bAlive").values[0]){
  statuses[slot]=2;human_case.setup.players.at(static_cast<std::int64_t>(slot)).slot_status=2;
  if(!open)open=slot;else if(!observer&&teams[slot]!=teams[*open])observer=slot;
 }
 require(open&&observer,"Use real fixture with living players on two teams");
 statuses[*open]=0;human_case.setup.players.at(static_cast<std::int64_t>(*open)).slot_status=0;
 statuses[*observer]=4;claims[*observer]=2;
 human_case.setup.players.at(static_cast<std::int64_t>(*observer)).slot_status=4;
 human_case.setup.players.at(static_cast<std::int64_t>(*observer)).slot_claim=2;
 const auto a=static_cast<std::size_t>(teams[*open]),b=static_cast<std::size_t>(teams[*observer]);war(human_case,a).at(b)=1;
 require(expected_modes(human_case,0)[a]&&!expected_modes(human_case,2)[a],"Independent ordinary OPEN/PitBoss oracle wrong");
 unchanged64(human_case,0);unchanged64(human_case,2);
 auto expected=expected_modes(state,0);set_equal_inputs(state,expected);
 const auto physical=detail::capture_dynamic_mode_operands(state);
 std::optional<std::size_t> null_slot,present_slot;
 for(std::size_t slot=0;slot<64;++slot)if(physical.players[slot].alive){
  if(slot>=22&&physical.players[slot].notification==0&&!null_slot)null_slot=slot;
  if(physical.players[slot].notification==1&&!present_slot)present_slot=slot;
 }
 require(null_slot&&present_slot,"Use a real BNW save with living NULL minor/barbarian and notification major");
 const auto n=*null_slot;const auto old=member(state,n,"m_bDynamicTurnsSimultMode").values[0];
 member(state,n,"m_bDynamicTurnsSimultMode").values[0]=old?0:1;
 const auto before=detail::capture_dynamic_mode_operands(state);auto plan=detail::DynamicModeComponentPlan::prepare(state,0);
 require(plan.stores()==1&&!plan.blocked()&&plan.teams_visited()==64,"Actual NULL branch failed to complete scalar");
 plan.require_current(state,false);require(plan.apply_prepared_without_callbacks(state),"NULL branch store refused");plan.require_current(state,true);
 require(member(state,n,"m_bDynamicTurnsSimultMode").values[0]==old,"NULL branch did not physically assign native mode");
 require(!plan.apply_prepared_without_callbacks(state),"NULL branch replayed earned store");
 auto after=detail::capture_dynamic_mode_operands(state);auto wanted=before;wanted.players[n].mode=old;
 require(after==wanted,"NULL branch changed another native operand/storage identity");
 const auto late_war=war(state,0).at(1);war(state,0).at(1)=late_war?0:1;
 refuses([&]{plan.require_current(state,true);});
 require(plan.applied()&&!plan.apply_prepared_without_callbacks(state)&&member(state,n,"m_bDynamicTurnsSimultMode").values[0]==old,
  "Late source failure lost actual store history or permitted replay");war(state,0).at(1)=late_war;
 // Real present pointer: the expected actual Add must precede its mode store.
 const auto p=*present_slot;set_equal_inputs(state,expected);const auto target_old=member(state,p,"m_bDynamicTurnsSimultMode").values[0];
 member(state,p,"m_bDynamicTurnsSimultMode").values[0]=target_old?0:1;
 const auto present_before=detail::capture_dynamic_mode_operands(state);auto blocked=detail::DynamicModeComponentPlan::prepare(state,0);
 require(blocked.blocked()&&blocked.blocked()->second==static_cast<std::int32_t>(p)&&blocked.stores()==0,"Present pointer notification not mandatory before store");
 blocked.require_current(state,false);require(blocked.apply_prepared_without_callbacks(state),"Prior no-effect prefix refused");blocked.require_current(state,true);
 require(detail::capture_dynamic_mode_operands(state)==present_before&&!blocked.apply_prepared_without_callbacks(state),"Mandatory notification was skipped/replayed");
 // Input callback mutation must fail the final seal before any mode write.
 set_equal_inputs(state,expected);member(state,n,"m_bDynamicTurnsSimultMode").values[0]=old?0:1;
 auto stale=detail::DynamicModeComponentPlan::prepare(state,0);const auto prior_mode=member(state,n,"m_bDynamicTurnsSimultMode").values[0];
 auto& changed_war=war(state,0).at(1);changed_war=changed_war?0:1;
 refuses([&]{stale.require_current(state,false);});
 require(!stale.applied()&&member(state,n,"m_bDynamicTurnsSimultMode").values[0]==prior_mode,"Stale preflight performed native store");
}
void shuffle_cases(const LiveSessionState& imported){
 auto state=imported;const auto map=state.random.map.state(),before=state.random.gameplay.state();
 const auto revision=state.ui_source_revision;const auto turn=state.setup.game_turn;
 std::array<std::int32_t,64> expected{};std::array<std::uint64_t,64> values{};
 for(std::size_t n=0;n<64;++n)expected[n]=static_cast<std::int32_t>(n);
 auto seed=before.seed;
 // Independent direct native32 LCG oracle. Final bound1 also advances RNG.
 for(std::size_t n=0;n<64;++n){seed=(seed*1103515245ULL+12345ULL)&0xffffffffULL;
  const auto sample=(seed>>16)&0xffffULL;values[n]=(sample*(64ULL-static_cast<std::uint64_t>(n)))/65536ULL;
  const auto other=n+static_cast<std::size_t>(values[n]);if(n!=other)std::swap(expected[n],expected[other]);}
 auto& statuses=state.world.pregame->archive.integer_arrays.at("slot_status");
 for(std::size_t n=0;n<64;++n){statuses[n]=2;state.setup.players.at(static_cast<std::int64_t>(n)).slot_status=2;member(state,n,"m_bTurnActive").values[0]=1;}
 auto plan=detail::TurnShuffleComponent::prepare(state);plan.require_current(state,false);plan.draw_without_callbacks(state);
 require(plan.draws()==64&&plan.permutation()==expected&&plan.values()==values&&values[63]==0&&
  state.random.gameplay.state().seed==seed&&state.random.gameplay.state().calls==before.calls+64&&
  state.random.gameplay.state().resets==before.resets&&state.random.map.state()==map,"Actual shuffle64/bound1/source32 oracle mismatch");
 std::uint64_t living=0;for(std::size_t n=0;n<64;++n)living+=member(state,n,"m_bAlive").values[0]?1u:0u;
 while(plan.scanned()<64){const auto result=plan.scan_one_without_callbacks(state,0);require(result!=detail::TurnShuffleComponent::ScanReturn::changed_active_required,"Equal active native setter not no-op");}
 require(plan.loop_returned()&&plan.equal_returns()==living&&!plan.pending()&&state.ui_source_revision==revision&&state.setup.game_turn==turn,
  "Shuffled64 traversal/equal active bypass changed clock/native fields");
 const auto after=state.random.gameplay.state();refuses([&]{plan.draw_without_callbacks(state);});
 require(state.random.gameplay.state()==after&&plan.draws()==64,"Repeated shuffle consumed RNG");
 // Controlled imported components cannot mint live Source/cursor predecessors.
 auto changed=imported;auto& next_status=changed.world.pregame->archive.integer_arrays.at("slot_status");
 for(std::size_t n=0;n<64;++n){next_status[n]=2;changed.setup.players.at(static_cast<std::int64_t>(n)).slot_status=2;member(changed,n,"m_bTurnActive").values[0]=0;}
 auto pending=detail::TurnShuffleComponent::prepare(changed);pending.draw_without_callbacks(changed);std::optional<std::int32_t> wanted;
 for(auto n:expected)if(member(changed,static_cast<std::size_t>(n),"m_bAlive").values[0]){wanted=n;break;}
 require(wanted.has_value(),"Real save needs living imported player");
 while(pending.scanned()<64&&!pending.pending())(void)pending.scan_one_without_callbacks(changed,0);
 require(pending.pending()==wanted&&!pending.loop_returned()&&pending.equal_returns()==0&&member(changed,static_cast<std::size_t>(*wanted),"m_bTurnActive").values[0]==0,
  "Changed setTurnActive skipped or reached outside shuffled order");
 refuses([&]{(void)pending.scan_one_without_callbacks(changed,0);});
 auto humans=imported;auto& hs=humans.world.pregame->archive.integer_arrays.at("slot_status");
 for(std::size_t n=0;n<64;++n){hs[n]=3;humans.setup.players.at(static_cast<std::int64_t>(n)).slot_status=3;}
 auto human=detail::TurnShuffleComponent::prepare(humans);human.draw_without_callbacks(humans);
 while(human.scanned()<64)require(human.scan_one_without_callbacks(humans,0)!=detail::TurnShuffleComponent::ScanReturn::changed_active_required,"Human/dead AI short circuit failed");
 require(human.loop_returned()&&human.equal_returns()==0,"Human-only loop invoked AI setter");
 auto stale=imported;auto blocked=detail::TurnShuffleComponent::prepare(stale);auto& active=member(stale,0,"m_bTurnActive").values[0];active=active?0:1;
 const auto stale_rng=stale.random.gameplay.state();refuses([&]{blocked.draw_without_callbacks(stale);});
 require(!blocked.attempted()&&blocked.draws()==0&&stale.random.gameplay.state()==stale_rng,"Stale preflight consumed RNG");
 auto& old_active=member(state,0,"m_bTurnActive").values[0];old_active=old_active?0:1;
 refuses([&]{plan.require_current(state,true);});require(plan.draws()==64&&state.random.gameplay.state()==after,"Late failed seal erased RNG history");
 auto capacity=imported;capacity.random.gameplay=SimulationRandom(RandomAlgorithm::bnw,{before.seed,UINT32_MAX-63ULL,before.resets});
 const auto overflow_before=capacity.random.gameplay.state();refuses([&]{(void)detail::TurnShuffleComponent::prepare(capacity);});
 require(capacity.random.gameplay.state()==overflow_before,"Counter capacity preflight consumed partial RNG");
}

}
int main(int argc,char** argv){try{
 require(argc==3,"Supply actual selected BNW save and installed game root");
 const auto source_before=identify_file(argv[1]);
 auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0,true);
 require(static_cast<bool>(selected.preview.host),"Actual selected BNW import failed");
 std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(selected.preview.host));
 const auto state=host->session().read_state_lease();const auto rng=state->random.gameplay.state();const auto revision=state->ui_source_revision;
 component_cases(*state);
 shuffle_cases(*state);
 // Genuine production caller link and early admission refusal, no fake elapsed
 // receipt, no initialized second VM, no manufactured scheduler phase.
 const auto catalog=content::discover_packages({std::filesystem::path(argv[2])/"Assets/DLC"});
 require(catalog.diagnostics.empty(),"Actual selected content discovery failed");
 auto source=BnwSelectedStockLuaSourceOwner::create_selected(argv[2],catalog,selected.profile,host->identity());
 require(source&&source->evidence().selected_profile,"Actual selected Source factory failed before admission test");
 auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
 const auto view=cursor->view();refuses([&]{(void)source->run_game_turn_activation_prefix(cursor);});
 require(!cursor->dynamic_turn_mode_progress().attempted&&cursor->view().operation==view.operation&&cursor->view().frontier==view.frontier,
  "Missing initialized source/earned elapsed changed scheduler");
 refuses([&]{(void)source->run_game_turn_shuffle_prefix(cursor);});
  require(!cursor->turn_shuffle_progress().attempted&&cursor->view().operation==view.operation&&cursor->view().frontier==view.frontier,"Missing genuine elapsed/dynamic shuffle issuer changed scheduler");
 // Real public caller must refuse absent actual elapsed/dynamic/shuffle
 // lineage without storing a Player flag or crediting UI publication.
 const auto actual_players_before=state->world.all_players->players;
 refuses([&]{(void)source->run_game_turn_player_dirty_prefix(cursor);});
 refuses([&]{(void)source->resume_game_turn_player_after_dirty_delivery(cursor);});
 refuses([&]{(void)source->run_game_turn_player_end_turn_prefix(cursor);});
 refuses([&]{(void)source->run_game_turn_player_end_turn_prefix(cursor);});
 const auto missing_end=cursor->player_end_turn_progress();
 require(!missing_end.attempted&&!missing_end.end_flag_stored&&!missing_end.source_adopted&&!missing_end.setter_returned&&
  !missing_end.busy_guard_required&&!missing_end.processed_auto_owner_required,
  "Absent genuine SAME69 delivery minted EndTurn/reset/attrition history");
 // A caller weak pointer cannot manufacture actual Core/mission runtime.
 const std::weak_ptr<const BnwNativeUnitAllocatorCore> absent_core;
 refuses([&]{(void)source->run_game_turn_player_busy_prefix(cursor,absent_core);});
 refuses([&]{(void)source->run_game_turn_player_busy_prefix(cursor,absent_core);});
 const auto missing_busy=cursor->player_busy_units_progress();
 require(!missing_busy.attempted&&!missing_busy.issued&&!missing_busy.native_prefix_returned&&!missing_busy.units_query_completed&&
  !missing_busy.has_busy_unit&&!missing_busy.required_unit&&!missing_busy.setter_early_returned&&missing_busy.queried_units==0,
  "Missing physical Core/actual scheduler minted busy false/early native return or city skip");
 refuses([&]{(void)source->run_game_turn_player_busy_city_prefix(cursor,absent_core);});
 refuses([&]{(void)source->run_game_turn_player_busy_city_prefix(cursor,absent_core);});
 const auto missing_city=cursor->player_busy_cities_progress();
 require(!missing_city.attempted&&!missing_city.issued&&!missing_city.native_prefix_returned&&!missing_city.cities_query_completed&&
  !missing_city.has_busy_city&&!missing_city.required_city&&!missing_city.setter_early_returned&&missing_city.queried_cities==0,
  "Absent actual unit-false/OriginalRead city owner minted false or skipped remote/AI mandatory effects");
 refuses([&]{(void)source->run_game_turn_player_end_after_busy_prefix(cursor,absent_core);});
 refuses([&]{(void)source->run_game_turn_player_end_after_busy_prefix(cursor,absent_core);});
 const auto missing_ai_end=cursor->player_end_after_busy_progress();
 require(!missing_ai_end.attempted&&!missing_ai_end.issued&&!missing_ai_end.end_flag_stored&&!missing_ai_end.source_adopted&&
  !missing_ai_end.setter_returned&&!missing_ai_end.processed_auto_owner_required,
  "Absent actual AI/native city-false or Event69 predecessor minted EndTurn/AutoMoves return");
 refuses([&]{(void)source->run_processed_auto_moves_after_busy(cursor,absent_core);});
 refuses([&]{(void)source->run_processed_auto_moves_after_busy(cursor,absent_core);});
 const auto missing_processed=cursor->processed_auto_moves_progress();
 require(!missing_processed.attempted&&!missing_processed.issued&&!missing_processed.auto_stored&&!missing_processed.processed_stored&&
  !missing_processed.source_adopted&&!missing_processed.setter_returned,
  "Absent actual AI changed-auto/busy/69 predecessor minted ProcessedAutoMoves return");
 refuses([&]{(void)source->run_player_attrition_first_prefix(cursor,absent_core);});
 refuses([&]{(void)source->run_player_attrition_first_prefix(cursor,absent_core);});
 refuses([&]{(void)source->run_player_attrition_mountain_prefix(cursor,absent_core);});
 refuses([&]{(void)source->run_player_attrition_mountain_prefix(cursor,absent_core);});
 const auto missing_mountain=cursor->attrition_mountain_progress();
 require(!missing_mountain.issued&&!missing_mountain.executed&&!missing_mountain.damage_required,"Missing real Core/first prefix minted mountain getter/effect");
 const auto missing_attrition=cursor->attrition_first_progress();
 require(!missing_attrition.issued&&!missing_attrition.executed&&!missing_attrition.rng_stored&&!missing_attrition.outer_returned,
  "Absent actual native setter/Core/69 predecessor minted attrition or RNG history");
 const auto missing_dirty=cursor->player_turn_active_progress();
 require(!missing_dirty.publication_attempted&&!missing_dirty.queued&&!missing_dirty.delivery_attempted&&!missing_dirty.delivered,
  "Missing genuine activation/VM callback created Event69 history");
 refuses([&]{(void)source->run_game_turn_player_active_prefix(cursor);});
 const auto active_history=cursor->player_turn_active_progress();
 require(!active_history.attempted&&!active_history.stored&&!active_history.source_adopted&&!active_history.publication_returned&&
  cursor->view().operation==view.operation&&cursor->view().frontier==view.frontier,
  "Missing genuine activation predecessor admitted changed Player store");
 for(std::size_t n=0;n<64;++n){
  const auto& old=detail::dynamic_core(actual_players_before[n],"m_bTurnActive");
  const auto& now=detail::dynamic_core(state->world.all_players->players[n],"m_bTurnActive");
  require(old.values==now.values,"Refused public activation changed actual Player flag");
  require(detail::dynamic_core(actual_players_before[n],"m_bEndTurn").values==detail::dynamic_core(state->world.all_players->players[n],"m_bEndTurn").values&&
   detail::dynamic_core(actual_players_before[n],"m_bAutoMoves").values==detail::dynamic_core(state->world.all_players->players[n],"m_bAutoMoves").values,
   "Refused EndTurn caller/replay changed actual saved flag operands");

 }
 refuses([&]{(void)source->run_game_turn_player_active_prefix(cursor);});
 require(!cursor->player_turn_active_progress().attempted&&cursor->view().operation==view.operation,
  "Refused activation replay created an effect receipt");
 require(state->ui_source_revision==revision&&state->random.gameplay.state()==rng&&identify_file(argv[1]).sha256==source_before.sha256,
  "Component/negative caller changed actual imported state or source save");
 std::cout<<"PASS dynamic-mode and shuffle64/bound1/permutation/alive-human/equal-active/changed-frontier/replay/stale-input/counter-capacity components and genuine early Source refusal; full scheduler untested\n";
 return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
