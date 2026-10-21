#include "civ5/runtime/bnw_unit_move_pending.hpp"
#include "civ5/runtime/bnw_unit_movement_query.hpp"
#include "civ5/runtime/bnw_plot_movement_cost.hpp"
#include "civ5/runtime/bnw_plot_reveal_read.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_plot_terrain_checkpoint_pair.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/bnw_client_selection_control.hpp"
#include <climits>
#include <iostream>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F>void refuses(F f){bool stopped=false;try{f();}catch(const std::exception&){stopped=true;}check(stopped,"Required negative did not refuse");}
std::int64_t scalar(const saves::UnitCoreObservation& core,std::string_view name){
 for(const auto& f:core.fields)if(f.name&&std::string_view(f.name)==name&&f.values.size()==1)return f.values[0];
 throw std::runtime_error("Actual movement test field absent");
}
template<class T>concept PublicDimBegin=requires(const T& p,const ui::SelectedBnwPreviewHost* h,
 const BnwStockLuaAttemptLease& l){p.begin_stock_dim(h,l);};
template<class T>concept PublicDimOperands=requires(const T& p){p.stock_dim_operands();};
template<class T>concept PublicDimAccept=requires(const T& p,const ui::SelectedBnwPreviewHost* h){p.accept_stock_dim_return(h);};
static_assert(!PublicDimBegin<BnwUnitMovePending>);
static_assert(!PublicDimOperands<BnwUnitMovePending>);
static_assert(!PublicDimAccept<BnwUnitMovePending>);
template<class T>concept PublicPostDim=requires(const T& p,const ui::SelectedBnwPreviewHost* h){p.continue_post_dim_selection(h);};
static_assert(!PublicPostDim<BnwUnitMovePending>);
template<class T>concept PublicFinderReset=requires(T& value){value.force_reset_bnw_common_pathfinder();};
static_assert(!PublicFinderReset<LiveSession>);
template<class T>concept PublicDirtySetter=requires(T& value,const BnwUnitMovePending& pending){value.set_moves_dirty(pending);};
static_assert(!PublicDirtySetter<ui::BnwClientSelectionControl>);
static_assert(static_cast<std::int32_t>(ui::BnwClientUnitDirtyKind::unit_info)==19);
static_assert(static_cast<std::int32_t>(ui::BnwClientUnitDirtyKind::plot_list_buttons)==5);
void detached_math(){
 static_assert(!std::is_default_constructible_v<BnwUnitMovePending>);
 check(detail::bnw_unit_move_after_cost(120,120)==0,"Native moves deduction differs");
 check(detail::bnw_unit_move_after_cost(1,120)==-119,"Native negative moves clamped");
 check(detail::bnw_unit_move_after_cost(INT32_MIN,0)==INT32_MIN,"Native lower bound differs");
 check(detail::bnw_unit_move_after_cost(INT32_MAX,INT32_MAX)==0,"Native cancellation differs");
 check(detail::bnw_unit_move_after_cost(-1,-1)==0,"Native signed cost differs");
 refuses([]{(void)detail::bnw_unit_move_after_cost(INT32_MIN,INT32_MIN);}); // negation BEFORE cancellation
 refuses([]{(void)detail::bnw_unit_move_after_cost(INT32_MIN,1);});
 refuses([]{(void)detail::bnw_unit_move_after_cost(INT32_MAX,-1);});
}
}
void actual_scenario(const char* save,const char* install,bool ordinary){
 auto boot=ui::bootstrap_selected_bnw_preview(save,install,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 check(bool(boot.preview.host),"Actual movement Host absent");
 auto host=std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(boot.preview.host));
 auto& session=host->session();const auto& state=session.state();
 check(!session.bnw_common_pathfinder_force_reset(),"Original actual map Initialize did not issue source false control");
 bool cache_foreign=false;
 std::thread cache_worker([&]{try{(void)session.bnw_common_pathfinder_force_reset();}catch(const BnwCommonPathfinderUnavailable&){cache_foreign=true;}});
 cache_worker.join();check(cache_foreign,"Foreign thread read actual common cache flags");
 const auto actor=detail::bnw_game_active_player_read(state);
 check(actor>=0&&actor<64&&state.world.all_players&&state.world.map_projection,"Actual active unit roster absent");
 const auto& player=state.world.all_players->players.at(static_cast<std::size_t>(actor));
 const auto actual_team=detail::bnw_player_team_read(state,static_cast<std::int32_t>(actor));
 for(std::int32_t team=0;team<64;++team)
  (void)detail::bnw_plot_revealed_read(state.world,0,team); // current raw/owned DWORD boundary, including 31/32/63
 refuses([&]{(void)detail::bnw_plot_revealed_read(state.world,0,-1);});
 refuses([&]{(void)detail::bnw_plot_revealed_read(state.world,0,64);});
 const auto& map=state.world.map_header;const MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,saves::GameStateLayout::expansion2_version1};
 const auto pair_rules=BnwPlotTerrainRulesSnapshot::from_rules(host->database());
 const auto before=session.checkpoint();std::optional<std::pair<BnwUnitOrderKey,std::uint64_t>> selected;
 std::optional<BnwPlotMovementCostQuery> normal_quote;std::optional<std::int32_t> expected_cost;
 std::size_t unavailable_quotes=0;std::string last_cost_gap;
 // Candidate discovery is a real read; it creates neither fog nor a unit.
 // Every reached positive after discovery is mandatory and uncaught.
 for(const auto& u:player.units.units){
  const BnwUnitOrderKey key{actor,scalar(u.core,"m_iID")};
  if(scalar(u.core,"m_bDeathDelay")!=0)continue;
  const auto quote=quote_bnw_unit_movement(state,host->database(),host->types(),host->identity(),key);
  if(!quote.max_moves||!quote.base_moves)continue;
  if(quote.terms.domain==2&&scalar(u.core,"m_iEmbarkAbilityCount")>0)continue;
  const auto origin=bnw_unit_order_plot(state,key);const auto xy=map.grid.coordinates(origin);
  for(std::int64_t d=0;d<6;++d){
   const auto to=geometry.direction(static_cast<std::int64_t>(xy[0]),static_cast<std::int64_t>(xy[1]),d);
   if(!to||static_cast<std::uint64_t>(*to)==origin)continue;
   const auto slot=static_cast<std::uint64_t>(*to);
   const auto type=state.world.map_projection->cells.at(slot).plot_type;
   if(!type||*type==3)continue; // native PLOT_OCEAN3, not mountain0
   const bool revealed=detail::bnw_plot_revealed_read(state.world,slot,actual_team);
   if(ordinary){
    if(!revealed)continue;
    // Discovery reports only a genuine named cost frontier; it cannot replace
    // it with a cost/false predicate or issue a movement owner.
    try{
     auto actual=bind_bnw_plot_movement_cost(host,key,slot);const auto value=actual.value();
     expected_cost=value;normal_quote.emplace(std::move(actual));
    }catch(const BnwPlotMovementCostUnavailable& e){++unavailable_quotes;last_cost_gap=e.what();continue;}
   }else{
    if(revealed)continue;
    if(!detail::bnw_player_human_read(state,static_cast<std::int32_t>(actor),host->database()))continue;
    expected_cost=*quote.max_moves;
   }
   selected=std::pair{key,slot};break;
  }
  if(selected)break;
 }
 if(!selected){
  std::cout<<"Actual "<<(ordinary?"revealed cost":"fog-human maxMoves")<<" nonembarking prefix UNOBSERVED; cost-frontiers="<<unavailable_quotes;
  if(!last_cost_gap.empty())std::cout<<" last="<<last_cost_gap;
  std::cout<<"; no movement completion claimed\n";return;
 }
 check(expected_cost.has_value(),"Actual candidate lost genuine cost");
 if(normal_quote){
  check(normal_quote->value()==*expected_cost,"SAME retained quote changed before movement");
  bool cost_wrong_thread=false;std::thread cost_worker([&]{try{(void)normal_quote->value();}catch(const std::exception&){cost_wrong_thread=true;}});
  cost_worker.join();check(cost_wrong_thread,"Actual cost owner accepted foreign thread");
 }
 const auto key=selected->first;const auto dest=map.grid.coordinates(selected->second);
 const auto old_plot=bnw_unit_order_plot(state,key);const auto old_revision=state.ui_source_revision;
 const auto old_world_revision=bnw_unit_order_world_revision(state,host->identity(),host->database());
 auto pending=session.begin_bnw_unit_move_prefix(host,key,static_cast<std::int32_t>(dest[0]),static_cast<std::int32_t>(dest[1]),false);
 const auto progress=pending->prefix();const auto copied_intents=pending->queued_new_client_intents();
 check(!progress.dim_attempted&&!progress.dim_returned,"Moves-only prefix fabricated actual Dim invocation/return");
 check(!progress.selection_attempted&&!progress.unit_selected.has_value(),
  "Moves-only prefix fabricated post-Dim native selection read");
 check(!progress.unit_info_dirty_stored&&!progress.plot_list_dirty_stored&&
  progress.unit_info_dirty_occurrence==0&&progress.plot_list_dirty_occurrence==0,
  "Pre-Dim moves prefix fabricated native dirty setters");
 check(!progress.common_cache_reset&&!progress.selection_plot_read&&
  progress.set_moves_completed==!progress.moves_stored,
  "Pre-Dim prefix fabricated path reset/selection read/setMoves completion");
 check(progress.before_plot==(progress.moves_stored?std::optional{static_cast<std::int32_t>(old_plot)}:std::nullopt),
  "Original native plot was not captured before the actual moves store");
 check(!session.bnw_common_pathfinder_force_reset(),"Moves store reset finder BEFORE genuine Dim/IsSelected");
 check(progress.cost==*expected_cost,"Writer did not consume SAME actual fog/ordinary cost");
 check(progress.after_moves==detail::bnw_unit_move_after_cost(progress.before_moves,*expected_cost),"Actual moves prefix differs");
 check(bnw_unit_order_plot(session.state(),key)==old_plot,"Partial native prefix relocated unit");
 check(!session.readiness().can_continue(),"Partial unit native prefix granted full continuation");
 if(progress.moves_stored){
  if(normal_quote)refuses([&]{(void)normal_quote->value();}); // real moves/UI store stales exact quote
  const auto& intents=copied_intents;
  check(progress.boundary==BnwUnitMoveBoundary::dim_receiver&&intents.size()==1&&intents[0].unit==key&&
        intents[0].dim==(progress.after_moves<=0)&&intents[0].occurrence==1&&state.ui_source_revision==old_revision+1,
        "Actual store/ordered queued intent/immediate UI revision differs");
  check(old_world_revision!=bnw_unit_order_world_revision(state,host->identity(),host->database()),"Actual native store did not change world");
 }else check(pending->queued_new_client_intents().empty()&&state.ui_source_revision==old_revision,"Unchanged native moves manufactured event");
 refuses([&]{(void)session.begin_bnw_unit_move_prefix(host,key,static_cast<std::int32_t>(dest[0]),static_cast<std::int32_t>(dest[1]),false);});
 bool wrong_thread=false;std::thread worker([&]{try{pending->require_current();}catch(const std::exception&){wrong_thread=true;}});worker.join();
 check(wrong_thread,"Movement retained owner accepted foreign thread");
 auto refuses_history=[&](auto operation){bool blocked=false;try{operation();}catch(const std::exception& e){
  blocked=std::string_view(e.what()).find("unit-move once history")!=std::string_view::npos;}
  check(blocked,"Filepair lost registered unit-move once history guard");};
 refuses_history([&]{(void)make_bnw_plot_terrain_checkpoint_pair(session,host->identity(),pair_rules);});
 const BnwPlotTerrainCheckpointPair diagnostic_pair{};
 refuses_history([&]{(void)restore_bnw_plot_terrain_checkpoint_pair(diagnostic_pair,session,host->identity(),pair_rules);});
 session.restore(before);refuses([&]{pending->require_current();});
 refuses_history([&]{(void)make_bnw_plot_terrain_checkpoint_pair(session,host->identity(),pair_rules);});
 refuses_history([&]{(void)restore_bnw_plot_terrain_checkpoint_pair(diagnostic_pair,session,host->identity(),pair_rules);});
 refuses([&]{(void)session.bnw_common_pathfinder_force_reset();}); // CP value cannot reissue cache authority.
 refuses([&]{(void)session.begin_bnw_unit_move_prefix(host,key,static_cast<std::int32_t>(dest[0]),static_cast<std::int32_t>(dest[1]),false);});
 pending.reset(); // Session quarantine outlives external handle.
 refuses([&]{(void)session.begin_bnw_unit_move_prefix(host,key,static_cast<std::int32_t>(dest[0]),static_cast<std::int32_t>(dest[1]),false);});
 normal_quote.reset();host.reset();check(copied_intents.size()==(progress.moves_stored?1U:0U),"Owned intent diagnostic lost storage on actual Host drop");
 std::cout<<"PASS actual "<<(ordinary?"revealed-cost":"fog-human")<<" partial store/intent/once/thread/checkpoint quarantine; dim receiver/setXY/mission remain incomplete\n";
}

void actual_cache_lifecycle(const char* save,const char* install){
 auto diagnostic=ui::bootstrap_selected_bnw_preview(save,install,0);
 if(diagnostic.preview.blocker)throw std::runtime_error(diagnostic.preview.blocker->reason);
 check(bool(diagnostic.preview.host),"Legacy mutable diagnostic import refused");
 refuses([&]{(void)diagnostic.preview.host->session().bnw_common_pathfinder_force_reset();});
 check(!diagnostic.preview.host->session().readiness().can_continue(),"Unsealed import minted runtime readiness");
 auto boot=ui::bootstrap_selected_bnw_preview(save,install,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 check(bool(boot.preview.host),"Actual common-cache lifecycle Host absent");
 auto host=std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(boot.preview.host));
 auto generic=LiveSession::from_restored_bnw(host->session().source(),host->session().state());
 refuses([&]{(void)generic.bnw_common_pathfinder_force_reset();});
 check(!host->session().bnw_common_pathfinder_force_reset(),"Original common cache initial state differs");
 LiveSession moved(std::move(host->session()));
 refuses([&]{(void)host->session().bnw_common_pathfinder_force_reset();});
 check(!moved.bnw_common_pathfinder_force_reset(),"Move failed to transfer SAME original-import cache");
 generic=std::move(moved);
 refuses([&]{(void)moved.bnw_common_pathfinder_force_reset();});
 check(!generic.bnw_common_pathfinder_force_reset(),"Assignment reinitialized or lost actual common cache");
 (void)generic.mutable_state();
 refuses([&]{(void)generic.bnw_common_pathfinder_force_reset();});
 check(!generic.readiness().can_continue(),"Scoped cache ownership minted continuation");
 std::cout<<"Prepared actual import/generic/move/assignment/thread/write/checkpoint cache guards; private ForceReset after genuine Dim UNOBSERVED\n";
}
int main(int argc,char** argv){
 check(argc==3,"Expected actual BNW save and installation");detached_math();actual_cache_lifecycle(argv[1],argv[2]);
 // Independent real Sessions: earlier partial-call quarantine is not cleared
 // or bypassed to manufacture the second scenario on the same world.
 actual_scenario(argv[1],argv[2],false);actual_scenario(argv[1],argv[2],true);
}
