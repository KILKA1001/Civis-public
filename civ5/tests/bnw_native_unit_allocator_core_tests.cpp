#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/runtime/bnw_happiness_buildings.hpp"
#include "civ5/runtime/bnw_unit_move_pending.hpp"
#include "civ5/runtime/bnw_unit_order_owners.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include "civ5/runtime/bnw_unit_movement_query.hpp"
#include "civ5/runtime/bnw_plot_reveal_read.hpp"
#include <set>
#include "civ5/runtime/bnw_plot_unit_validity_preflight.hpp"
#include "civ5/runtime/bnw_unit_support_death.hpp"
#include "civ5/graphics/bnw_initial_center_unit_renderer_loan.hpp"
#include "civ5/runtime/bnw_major_player_traits_owner.hpp"
#include "civ5/runtime/bnw_unit_sight_plan.hpp"
#include "civ5/runtime/bnw_happiness_policies.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/selected_city_production_services.hpp"
#include <type_traits>
#include <typeinfo>
#include <climits>
#include <algorithm>
#include <string_view>
#include <iostream>
#include <thread>
#include <chrono>
#include "civ5/runtime/bnw_unit_checkpoint.hpp"
#include "civ5/content/saved_terrain.hpp"
#include "civ5/runtime/bnw_live_rng_export.hpp"
namespace {
using namespace civ5;
static_assert(!std::is_default_constructible_v<ui::SelectedCityTrainCreateSource>);
static_assert(!std::is_default_constructible_v<runtime::BnwInitialUnitSightOwner>);
static_assert(!std::is_copy_constructible_v<ui::SelectedCityTrainCreateSource>);
static_assert(!std::is_default_constructible_v<runtime::BnwNativeUnitAllocatorCore>);
static_assert(!std::is_default_constructible_v<graphics::BnwInitialCenterUnitRendererLoan>);
static_assert(!std::is_copy_constructible_v<graphics::BnwInitialCenterUnitRendererLoan>);
static_assert(!std::is_default_constructible_v<graphics::BnwInitialCenterUnitVisibilityProof>);
void natural_selected_init(const std::filesystem::path&,const std::filesystem::path&);
void check(bool b,const char*why){if(!b)throw std::runtime_error(why);}
template<class F>void refuses(F&&f){bool no=false;try{f();}catch(const runtime::BnwNativeUnitAllocatorUnavailable&){no=true;}check(no,"Unfinished native unit creation accepted");}
template<class F>void refuses_native_boundary(F&&f,std::string_view reason){bool no=false;try{f();}catch(const runtime::BnwNativeUnitAllocatorUnavailable&e){check(std::string_view(e.what())==reason,"Unrelated native refusal masked actual stale mission snapshot boundary");no=true;}check(no,"Stale native mission snapshot accepted");}
// The canonical save decoder and Game leaf intentionally report malformed
// bytes/unknown imported members as std::runtime_error, separately from the
// typed unfinished native-owner boundary. Only these specific scenarios use it.
template<class F>void refuses_runtime_boundary(F&&f,std::string_view reason){bool no=false;try{f();}catch(const std::runtime_error&e){check(typeid(e)==typeid(std::runtime_error)&&std::string_view(e.what())==reason,"Unrelated exception masked intended canonical decoder/Game boundary");no=true;}check(no,"Malformed save/unknown Game runtime accepted");}
template<class F>void refuses_missing_original_common(F&&f){bool no=false;try{f();}catch(const runtime::BnwCommonPathfinderUnavailable&e){check(std::string_view(e.what())=="Actual original-import common pathfinder service absent/retired","Unrelated owner failure masked missing actual original Common factory");no=true;}check(no,"Plain Civis fabricated an original Common pathfinder");}
// Original CvMap::Read controls require a real sealed activated database.
// The default diagnostic bootstrap intentionally does not issue that owner.
// This fixture requests the existing real producer and checks its current
// physical service, without turning a supplied bool into a path permission.
void require_existing_standalone_fixture(const saves::CivisSaveDocument&document,
 const saves::StandaloneServicesConfig&config){
 const saves::CivisSaveSection*actual=nullptr;std::size_t count=0;
 for(const auto&section:document.sections)if(section.id==static_cast<std::uint32_t>(saves::CivisSaveSectionId::standalone_services)){
  actual=&section;++count;
 }
 check(count==1&&actual,"Original-import fixture must carry exactly one genuine services42 section");
 check(actual->flags==0&&actual->bytes.size()==12,"Original-import fixture services42 flags/shape differ");
 check(saves::read_standalone_services_section(*actual)==config,
  "Original-import fixture services42 configuration differs from actual live source");
 const auto canonical=saves::make_standalone_services_section(config);
 check(actual->id==canonical.id&&actual->flags==canonical.flags&&actual->bytes==canonical.bytes,
  "Original-import fixture services42 is not canonical actual configuration");
}
void require_original_common_fixture(const ui::SelectedBnwReadOnlyPreview&preview){
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  const auto&session=host->session();const auto state=session.read_state_lease();
  check(session.source().source.container==saves::SaveContainerKind::civ5,
   "Natural original Common fixture is not actual Civ5 Read");
  check(host->database().immutable_preview_fingerprints_current(),
   "Natural original Common fixture has no genuine sealed rules owner");
  const auto generation=session.state_generation(),revision=state->ui_source_revision;
  const auto gameplay=state->random.gameplay.state(),map=state->random.map.state();
  const auto reset=session.bnw_common_pathfinder_force_reset();
  check(session.bnw_common_pathfinder_force_reset()==reset&&session.read_state_lease()==state&&
   session.state_generation()==generation&&state->ui_source_revision==revision&&
   state->random.gameplay.state()==gameplay&&state->random.map.state()==map,
   "Original Common service admission changed physical world/source/RNG");
 });
}
void natural_actual_path(const ui::SelectedBnwReadOnlyPreview&preview,const std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore>&owner){
 const auto sibling=runtime::BnwNativeUnitAllocatorCore::from_selected_preview(preview,0);
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  const auto&state=host->session().state();check(state.world.all_players&&state.world.map_projection&&state.world.map_plots,"Natural path source unavailable");const auto&map=*state.world.map_projection;
  const auto scalar=[](const saves::SavedUnitObservation&u,std::string_view key){const saves::UnitCoreField*out=nullptr;for(const auto&f:u.core.fields)if(f.name&&std::string_view(f.name)==key){check(!out,"Natural path duplicate scalar");out=&f;}check(out&&out->values.size()==1,"Natural path scalar absent");return out->values.front();};
  const auto field=[](const saves::PlotObservation&p,std::string_view key){const saves::NamedSaveInteger*out=nullptr;for(const auto&f:p.fields)if(f.name&&std::string_view(f.name)==key){check(!out,"Natural path duplicate plot field");out=&f;}check(out,"Natural path plot field absent");return out->value;};
  std::int32_t denominator=0;unsigned define_rows=0;const auto define_report=host->database().query_rows({"SELECT Value FROM Defines WHERE Name='MOVE_DENOMINATOR'",{}},[&](const auto&r){++define_rows;const auto v=r.integer("Value");check(v>0&&v<=INT32_MAX/1000,"Natural path installed denominator invalid");denominator=static_cast<std::int32_t>(v);});check(define_report.all_succeeded()&&define_rows==1,"Natural path installed denominator unavailable");
  const auto source_team=state.setup.players.at(0).team;check(source_team>=0&&source_team<64,"Natural path actual native team domain invalid");const auto team=static_cast<std::int32_t>(source_team);const auto&h=state.world.map_header;const runtime::MapGeometry geometry{h.grid,h.wrap_x,h.wrap_y,saves::GameStateLayout::expansion2_version1};
  std::optional<std::int32_t>id;std::optional<std::uint64_t>origin,target;std::int32_t moves=0,next_moves=0,next_turns=1,mission_flags=0;
  // Independent scene selection, deliberately simple: one authentic LAND
  // object, an empty revealed flat neighborhood, authored movement=1 and no
  // source promotions/discounts/routes/river/feature. This is a cardinality
  // and installed-SQL oracle, not a copy of PathValid/PathCost predicates.
  for(const auto&u:state.world.all_players->players.at(0).units.units){
   if(scalar(u,"m_iID")<0||scalar(u,"m_iID")>INT32_MAX-8192||scalar(u,"m_eAutomateType")!=-1||scalar(u,"m_bEmbarked")||scalar(u,"m_bImmobile")||scalar(u,"m_iNoRevealMapCount")||scalar(u,"m_iGreatGeneralCount")||scalar(u,"m_iExtraMoveDiscount"))continue;
   const auto initial_moves=scalar(u,"m_iMoves");if(initial_moves<0||(initial_moves>0&&initial_moves<2*denominator)||initial_moves>INT32_MAX)continue;
   bool double_move=false;for(const auto&f:u.core.fields)if(f.name&&std::string_view(f.name)=="m_terrainDoubleMoveCount")for(const auto v:f.values)if(v)double_move=true;if(double_move)continue;
   const auto x=scalar(u,"m_iX"),y=scalar(u,"m_iY");if(x<0||y<0||static_cast<std::uint64_t>(x)>=map.grid.width||static_cast<std::uint64_t>(y)>=map.grid.height)continue;const auto start=map.grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));const auto&cell=map.cells.at(start);
   if(!cell.units||cell.units->size()!=1||cell.units->front().owner!=0||cell.units->front().id!=scalar(u,"m_iID"))continue;
   const auto hash=scalar(u,"idUnitType");if(hash<=0||hash>UINT32_MAX)continue;const auto*info=host->types().find_hash(static_cast<std::uint32_t>(hash));if(!info||info->table!="Units")continue;bool land=false;unsigned info_rows=0;
   const auto report=host->database().query_rows({"SELECT Type,Domain FROM Units WHERE ID=?",{info->id}},[&](const auto&r){++info_rows;check(r.text("Type")==std::optional<std::string>{info->type},"Natural path UnitInfo Type differs");land=r.text("Domain")==std::optional<std::string>{"DOMAIN_LAND"};});check(report.all_succeeded()&&info_rows==1,"Natural path UnitInfo query unavailable");if(!land)continue;
   bool simple=true;std::set<std::uint64_t>neighborhood{start};for(std::int32_t d=0;d<6;++d){const auto n=geometry.direction(x,y,d);if(!n){simple=false;break;}check(*n>=0&&static_cast<std::uint64_t>(*n)<map.grid.size(),"Natural direction native plot outside actual map");const auto ni=static_cast<std::uint64_t>(*n);neighborhood.insert(ni);const auto xy=map.grid.coordinates(ni);check(xy[0]<=INT64_MAX&&xy[1]<=INT64_MAX,"Natural geometry coordinate outside signed domain");for(std::int32_t e=0;e<6;++e){const auto m=geometry.direction(static_cast<std::int64_t>(xy[0]),static_cast<std::int64_t>(xy[1]),e);if(!m){simple=false;break;}check(*m>=0&&static_cast<std::uint64_t>(*m)<map.grid.size(),"Natural second direction native plot outside actual map");neighborhood.insert(static_cast<std::uint64_t>(*m));}}
   if(!simple)continue;
   for(const auto i:neighborhood){const auto&raw=state.world.map_plots->plots.at(i);const auto&c=map.cells.at(i);
    if(!raw.version||*raw.version<5||!c.units||field(raw,"plot_type")!=2||field(raw,"feature_type_hash")!=0||field(raw,"improvement_type_hash")!=0||field(raw,"route_type")!=-1||field(raw,"river_crossing_bits")!=0||field(raw,"plot_city_owner")!=-1||(field(raw,"owner")!=-1&&field(raw,"owner")!=0)) {simple=false;break;}
    if(i!=start&&!c.units->empty()){simple=false;break;}if(!runtime::detail::bnw_plot_revealed_read(state.world,i,team)){simple=false;break;}
    bool plain=false;unsigned rows=0;const auto q=host->database().query_rows({"SELECT Movement,Impassable FROM Terrains WHERE ID=?",{field(raw,"terrain_type")}},[&](const auto&r){++rows;plain=r.integer("Movement")==1&&r.integer("Impassable")==0;});check(q.all_succeeded()&&rows==1,"Natural path installed terrain unavailable");if(!plain){simple=false;break;}
   }
   if(!simple)continue;const auto first=geometry.direction(x,y,0);check(first.has_value(),"Natural simple scene lost direction0");check(*first>=0&&static_cast<std::uint64_t>(*first)<map.grid.size(),"Natural first native plot outside actual map");const auto first_index=static_cast<std::uint64_t>(*first);if(!runtime::bnw_plot_is_visible(state,start,team)||!runtime::bnw_plot_is_visible(state,first_index,team))continue;
   std::int32_t fresh=0;if(initial_moves==0){
    // Independent existing source-bound max/base getter supplies the actual
    // fresh turn budget. The new tree's private scratch is never inspected.
    const auto quote=runtime::quote_bnw_unit_movement(state,host->database(),host->types(),host->identity(),{0,scalar(u,"m_iID")});
    if(!quote.base_moves||!quote.max_moves||*quote.base_moves<2||*quote.base_moves>INT32_MAX/denominator||*quote.max_moves!=*quote.base_moves*denominator)continue;fresh=*quote.max_moves;
   }
   id=static_cast<std::int32_t>(scalar(u,"m_iID"));origin=start;target=first_index;moves=static_cast<std::int32_t>(initial_moves);mission_flags=static_cast<std::int32_t>(scalar(u,"m_iFlags"));next_moves=(moves?moves:fresh)-denominator;next_turns=moves?1:2;break;
  }
  if(!id){std::cout<<"UNAVAILABLE natural actual common path scene: no isolated flat two-ring LAND fixture; positive search UNEXECUTED\n";return;}
  const auto revision=runtime::bnw_unit_order_world_revision(state,host->identity(),host->database());const auto world_revision=state.ui_source_revision;const auto rng=state.random.gameplay.state(),map_rng=state.random.map.state();const auto events=state.city_dirty_events;const auto occurrence=state.city_dirty_next_occurrence;
  // Invalid requests fail before the actual search/control mutation.
  check(!owner->read_native_existing_unit_path(*id+8192,*target),"Natural actual path accepted obsolete serial");refuses([&]{(void)owner->read_native_existing_unit_path(*id,map.grid.size());});bool foreign=false;std::thread thread([&]{try{(void)owner->read_native_existing_unit_path(*id,*target);}catch(const runtime::BnwNativeUnitAllocatorUnavailable&){foreign=true;}});thread.join();check(foreign,"Foreign thread entered actual common path");
  const saves::SavedUnitObservation*actual_unit=nullptr;for(const auto&u:state.world.all_players->players.at(0).units.units)if(scalar(u,"m_iID")==*id)actual_unit=&u;check(actual_unit,"Natural actual source object disappeared");
  const saves::UnitTailNode*queue=nullptr;for(const auto&block:actual_unit->tail.blocks)if(block.name&&std::string_view(block.name)=="m_missionQueue"){check(!queue,"Natural duplicate mission queue");queue=&block;}
  check(queue&&queue->wire_count&&*queue->wire_count>=0&&static_cast<std::uint64_t>(*queue->wire_count)<=queue->values.max_size()/5,"Natural original MissionData native count invalid");const auto queue_count=static_cast<std::size_t>(*queue->wire_count);check(queue->values.size()==queue_count*5,"Natural original MissionData decoded row extent differs");
  const auto mission=owner->read_native_existing_mission_queue(*id);check(mission&&mission->owner==0&&mission->full_id==*id&&mission->queue.size()==queue_count&&mission->unit_flags==mission_flags&&mission->activity==scalar(*actual_unit,"m_eActivityType"),"Actual source mission list/head/native scalar observation differs");
  for(std::size_t i=0;i<mission->queue.size();++i){const auto&m=mission->queue[i];check(m.type==queue->values[i*5]&&m.data1==queue->values[i*5+1]&&m.data2==queue->values[i*5+2]&&m.flags==queue->values[i*5+3]&&m.push_turn==queue->values[i*5+4],"Actual ordered MissionData type/data/flags/pushTurn mapping differs");}
  check(owner->read_native_existing_ready_to_auto(*id)==std::optional<bool>{moves>0&&!mission->queue.empty()},"Actual source ReadyToAuto moves/queue short circuit differs");
  check(owner->read_native_existing_is_doing_partial_move(*id)==std::optional<bool>{false},"Original Read empty path fabricated partial move");
  check(!owner->read_native_existing_mission_queue(*id+8192)&&!owner->read_native_existing_ready_to_auto(*id+8192)&&!owner->read_native_existing_is_doing_partial_move(*id+8192),"Stale serial minted mission/body guard observations");
  const auto mission_runtime=owner->read_native_existing_mission_runtime(*id);
  check(mission_runtime&&mission_runtime->owner==0&&mission_runtime->full_id==*id&&mission_runtime->mission_timer==0&&mission_runtime->visual_locations==0&&mission_runtime->combat_city_owner==-1&&mission_runtime->combat_city_id==-1&&!mission_runtime->registered_lifetime.expired(),"Actual OriginalRead reset/constructor mission timer/vector/combatCity typed source differs");
  const auto sibling_runtime=sibling->read_native_existing_mission_runtime(*id);
  check(sibling_runtime&&!sibling_runtime->registered_lifetime.owner_before(mission_runtime->registered_lifetime)&&!mission_runtime->registered_lifetime.owner_before(sibling_runtime->registered_lifetime),"Same-session sibling reminted per-query mission default owner");
  // Independent source-schema negative IsBusy oracle: authentic explicit
  // NO_PLAYER combat handle plus INVALID_PLOT_COORD attack coordinates. No
  // copy of the private full combat/getAt/territory predicate is introduced.
  const saves::UnitTailNode*combat_owner=nullptr;for(const auto&block:actual_unit->tail.blocks)if(block.name&&std::string_view(block.name)=="m_combatUnit.eOwner")combat_owner=&block;
  check(combat_owner&&combat_owner->values.size()==1,"Natural source combat-owner tail field absent");
  if(combat_owner->values.front()==-1&&(scalar(*actual_unit,"m_iAttackPlotX")==-INT32_MAX||scalar(*actual_unit,"m_iAttackPlotY")==-INT32_MAX))check(!mission_runtime->busy,"Actual OriginalRead idle mission runtime minted busy combat from NULL native source handles");
  else std::cout<<"UNEXECUTED: explicit source NULL combat/INVALID attack idle guard absent in selected scene\n";
  check(!owner->read_native_existing_mission_runtime(*id+8192),"Stale serial obtained actual plain mission node");
  auto detached_runtime=*mission_runtime;detached_runtime.mission_timer=99;detached_runtime.visual_locations=20;detached_runtime.combat_city_owner=0;detached_runtime.combat_city_id=0;detached_runtime.busy=true;
  const auto still_runtime=owner->read_native_existing_mission_runtime(*id);
  check(still_runtime&&still_runtime->mission_timer==0&&still_runtime->visual_locations==0&&still_runtime->combat_city_owner==-1&&still_runtime->combat_city_id==-1&&still_runtime->busy==mission_runtime->busy&&!still_runtime->registered_lifetime.owner_before(mission_runtime->registered_lifetime)&&!mission_runtime->registered_lifetime.owner_before(still_runtime->registered_lifetime),"Detached runtime observation wrote timer/visual queue/combat city or reminted partial original Read");
  const auto actual_moves=owner->bind_native_existing_moves_archive(*id);check(actual_moves&&actual_moves->owner==0&&actual_moves->full_id==*id&&actual_moves->value==moves&&actual_moves->loaded_value==moves&&actual_moves->native_archive_index==10&&actual_moves->binding_name=="CvUnit::m_iMoves"&&actual_moves->binding_member=="m_iMoves"&&actual_moves->call_stack_tracking&&!actual_moves->dirty&&actual_moves->changes==0&&!actual_moves->registered_lifetime.expired(),"Actual imported Load scalar alias/native archive binding differs");
  const auto shared_moves=sibling->bind_native_existing_moves_archive(*id);check(shared_moves&&!actual_moves->registered_lifetime.owner_before(shared_moves->registered_lifetime)&&!shared_moves->registered_lifetime.owner_before(actual_moves->registered_lifetime),"Same-session sibling created a detached imported scalar registration");
  auto detached_moves=*actual_moves;detached_moves.value=INT32_MIN;detached_moves.dirty=true;detached_moves.changes=42;
  const auto retained_moves=owner->bind_native_existing_moves_archive(*id);check(retained_moves&&retained_moves->value==moves&&!retained_moves->dirty&&retained_moves->changes==0,"Detached scalar observation became write/dirty authority");
  check(!owner->bind_native_existing_moves_archive(*id+8192),"Obsolete full ID minted a registered moves field");
  bool foreign_moves=false;std::thread moves_thread([&]{try{(void)owner->bind_native_existing_moves_archive(*id);}catch(const runtime::BnwNativeUnitAllocatorUnavailable&){foreign_moves=true;}});moves_thread.join();check(foreign_moves,"Foreign thread entered imported scalar registration");
  const auto reset_path=owner->read_native_existing_retained_path_cache(*id);check(reset_path&&reset_path->nodes.empty()&&reset_path->destination==UINT32_MAX&&reset_path->generation_operations==0,"Natural actual imported path Reset does not match original Read");
  check(!owner->read_native_existing_path_first_plot(*id),"Actual empty original cache fabricated GetPathFirstPlot");
  const auto same=owner->read_native_existing_unit_path(*id,*origin);check(same&&same->found&&same->nodes.size()==1&&same->nodes.front().plot==*origin&&same->nodes.front().moves==moves&&same->nodes.front().turns==1&&same->nodes.front().known==0,"Natural source-coordinate native path differs");
  const auto one=owner->read_native_existing_unit_path(*id,*target);check(one&&one->found&&one->nodes.size()==2,"Natural actual adjacent path is not direct");const auto&start=one->nodes.front();const auto&end=one->nodes.back();check(start.plot==*origin&&start.moves==moves&&start.turns==1&&start.known==0,"Natural adjacent path lost actual parent/start");check(end.plot==*target&&end.moves==next_moves&&end.turns==next_turns&&end.known==1000*denominator+2&&end.total==end.known,"Natural installed Movement=1 path cost/remaining/turns differ");
  const auto repeated=owner->read_native_existing_unit_path(*id,*target,true);check(repeated==one,"Actual permitted same-source common-cache reuse changed native path");
  check(owner->read_native_existing_retained_path_cache(*id)==reset_path,"Observation fabricated retained Unit GeneratePath effects");
  const auto produced=owner->advance_native_existing_generate_path_cache(*id,*target,true);check(produced==one,"Actual native GeneratePath publication changed physical search result");
  const auto stored=owner->read_native_existing_retained_path_cache(*id);check(stored&&stored->destination==*target&&stored->nodes.size()==2&&stored->generation_operations==1&&stored->clear_operations==reset_path->clear_operations,"Actual generated path destination/counter/CopyPath extent differs");
  const auto a=map.grid.coordinates(*target),b=map.grid.coordinates(*origin);check(a[0]<=INT32_MAX&&a[1]<=INT32_MAX&&b[0]<=INT32_MAX&&b[1]<=INT32_MAX,"Natural retained path coordinate outside native domain");check(stored->nodes[0].x==static_cast<std::int32_t>(a[0])&&stored->nodes[0].y==static_cast<std::int32_t>(a[1])&&stored->nodes[0].data1==next_moves&&stored->nodes[0].data2==next_turns&&stored->nodes[0].flags==0&&stored->nodes[1].x==static_cast<std::int32_t>(b[0])&&stored->nodes[1].y==static_cast<std::int32_t>(b[1])&&stored->nodes[1].data1==moves&&stored->nodes[1].data2==1&&stored->nodes[1].flags==0,"Actual CopyPath is not destination-first with genuine visible flags");
  if(moves>0){check(owner->read_native_existing_retained_step_cost(*id)==std::optional<std::int32_t>{denominator},"Actual canonical first-step cost differs from independent installed flat-terrain/denominator scene");}
  else{refuses([&]{(void)owner->read_native_existing_retained_step_cost(*id);});}
  check(owner->read_native_existing_retained_path_cache(*id)==stored,"Actual first-step cost source modified canonical path");
  check(!owner->read_native_existing_retained_step_cost(*id+8192),"Obsolete full ID produced actual retained-step cost");
  auto detached=*stored;detached.nodes.clear();detached.destination=UINT32_MAX;check(owner->read_native_existing_retained_path_cache(*id)==stored,"Diagnostic path copy became canonical storage authority");
  check(!owner->advance_native_existing_generate_path_cache(*id+8192,*target,true)&&owner->read_native_existing_retained_path_cache(*id)==stored,"Obsolete serial published native path cache");
  check(owner->read_native_existing_path_first_plot(*id)==target,"Authentic GetPathFirstPlot did not select next-to-last destination-first cache node");
  // Native ordinary mode recalculates even an already successful path;
  // actual serialized evaluating mode reuses a visible next node. The test
  // reads the installed/imported flag and never forces a private branch.
  const auto updated=owner->advance_native_existing_update_path_cache(*id,*target);check(updated&&*updated,"Actual native UpdatePathCache rejected simple visible source-derived scene");
  const auto update_path=owner->read_native_existing_retained_path_cache(*id);check(update_path&&update_path->nodes==stored->nodes&&update_path->destination==*target&&update_path->generation_operations==stored->generation_operations+((mission_flags&1)?0:1)&&update_path->clear_operations==stored->clear_operations,"Actual UpdatePathCache ordinary/evaluating search occurrence or retained nodes differ");
  check(!owner->advance_native_existing_update_path_cache(*id+8192,*target)&&owner->read_native_existing_retained_path_cache(*id)==update_path,"Stale serial UpdatePathCache changed canonical storage");
  const auto updated_origin=owner->advance_native_existing_update_path_cache(*id,*origin);check(updated_origin&&*updated_origin,"Actual native UpdatePathCache current-plot/evaluating branch failed");
  const auto current_path=owner->read_native_existing_retained_path_cache(*id);check(current_path.has_value(),"Actual UpdatePathCache lost imported owner");
  if((mission_flags&1)==0){check(current_path->nodes.size()==1&&current_path->destination==*origin&&current_path->generation_operations==update_path->generation_operations+1&&owner->read_native_existing_path_first_plot(*id)==origin,"Ordinary current-plot update did not execute one-node GeneratePath/GetPathFirstPlot");}
  else{check(current_path==update_path&&owner->read_native_existing_path_first_plot(*id)==target,"Evaluating visible-next reuse changed original retained destination or node");}

  const auto current_xy=map.grid.coordinates(*origin),target_xy=map.grid.coordinates(*target);
  // Native at(x,y) early0 precedes unknown flags and ETA addition. Real raw
  // world position, source queue and private Core remain unmodified.
  const auto same_path=owner->read_native_existing_retained_path_cache(*id);
  const auto at_current=owner->advance_native_existing_unit_path_to_prefix(*id,static_cast<std::int32_t>(current_xy[0]),static_cast<std::int32_t>(current_xy[1]),INT32_MIN,INT32_MAX);
  check(at_current&&at_current->native_return==std::optional<std::int32_t>{0}&&at_current->boundary==runtime::BnwNativeUnitPathToBoundary::returned_no_move&&!at_current->first_plot&&owner->read_native_existing_retained_path_cache(*id)==same_path,"Actual UnitPathTo at-coordinate early0 reached finder/flags/ETA effects");
  if(!map.wrap_x||!map.wrap_y){const auto invalid=owner->advance_native_existing_unit_path_to_prefix(*id,map.wrap_x?0:-1,map.wrap_x?-1:0,INT32_MIN,INT32_MAX);check(invalid&&invalid->native_return==std::optional<std::int32_t>{0}&&owner->read_native_existing_retained_path_cache(*id)==same_path,"Actual invalid destination UnitPathTo failed source early0 before flags/ETA/path");}
  check(!owner->advance_native_existing_unit_path_to_prefix(*id+8192,static_cast<std::int32_t>(target_xy[0]),static_cast<std::int32_t>(target_xy[1])),"Stale serial entered actual UnitPathTo body");
  const auto entered=owner->advance_native_existing_unit_path_to_prefix(*id,static_cast<std::int32_t>(target_xy[0]),static_cast<std::int32_t>(target_xy[1]));
  check(entered&&!entered->native_return&&entered->first_plot==target&&entered->boundary==(moves>0?runtime::BnwNativeUnitPathToBoundary::destination_admission:runtime::BnwNativeUnitPathToBoundary::unit_move),"Actual UnitPathTo genuine post-cache boundary fabricated completed movement or ignored native ETA turn");
  const auto entered_path=owner->read_native_existing_retained_path_cache(*id);check(entered_path&&entered_path->nodes.size()==2&&entered_path->destination==*target,"Actual UnitPathTo did not execute real UpdatePathCache destination/CopyPath");
  auto detached_body=*entered;detached_body.native_return=1;detached_body.boundary=runtime::BnwNativeUnitPathToBoundary::returned_no_move;
  refuses([&]{(void)owner->advance_native_existing_unit_path_to_prefix(*id,static_cast<std::int32_t>(target_xy[0]),static_cast<std::int32_t>(target_xy[1]));});
  check(owner->read_native_existing_retained_path_cache(*id)==entered_path,"Detached path/body observation completed private frontier or repeated stores");
  const auto before_step_path=owner->read_native_existing_retained_path_cache(*id);
  const auto before_step_mission=owner->read_native_existing_mission_queue(*id);
  const auto before_step_archive=owner->bind_native_existing_moves_archive(*id);check(before_step_archive.has_value(),"Actual pre-step archive alias absent");
  const auto archive_same=[&]{const auto now=owner->bind_native_existing_moves_archive(*id);return now&&now->owner==before_step_archive->owner&&now->full_id==before_step_archive->full_id&&now->value==before_step_archive->value&&now->loaded_value==before_step_archive->loaded_value&&now->native_archive_index==before_step_archive->native_archive_index&&now->binding_name==before_step_archive->binding_name&&now->binding_member==before_step_archive->binding_member&&now->dirty==before_step_archive->dirty&&now->changes==before_step_archive->changes&&now->call_stack_tracking==before_step_archive->call_stack_tracking&&!now->registered_lifetime.owner_before(before_step_archive->registered_lifetime)&&!before_step_archive->registered_lifetime.owner_before(now->registered_lifetime);};
  const auto mission_same=[&]{const auto now=owner->read_native_existing_mission_queue(*id);return now&&before_step_mission&&now->owner==before_step_mission->owner&&now->full_id==before_step_mission->full_id&&now->unit_flags==before_step_mission->unit_flags&&now->activity==before_step_mission->activity&&now->queue==before_step_mission->queue;};
  check(!owner->advance_native_existing_unit_move_admission(*id+8192),"Stale serial consumed genuine UnitMove body");
  const auto step=owner->advance_native_existing_unit_move_admission(*id);
  check(step&&!step->native_return&&step->first_plot==target,"Actual private UnitMove prefix fabricated native ETA/moved return or detached first path");
  if(moves>0){
   check(step->boundary==runtime::BnwNativeUnitPathToBoundary::movement_store_graphical_owner&&step->move_cost==std::optional<std::int32_t>{denominator}&&step->moves_before==std::optional<std::int32_t>{moves}&&step->moves_requested==std::optional<std::int32_t>{moves-denominator},"Actual grounded UnitMove/native default-cost source did not reach genuine changed-store boundary with installed flat denominator");
  }else{
   check(step->boundary==runtime::BnwNativeUnitPathToBoundary::failed_unit_move_visualization&&!step->move_cost&&!step->moves_before&&!step->moves_requested,"Actual zero-moves UnitMove reached cost/store despite source canMove short circuit or skipped mandatory visualization/quickMovement");
  }
  check(owner->read_native_existing_retained_path_cache(*id)==before_step_path&&mission_same()&&archive_same()&&scalar(*actual_unit,"m_iX")==static_cast<std::int32_t>(current_xy[0])&&scalar(*actual_unit,"m_iY")==static_cast<std::int32_t>(current_xy[1])&&scalar(*actual_unit,"m_iMoves")==moves,"Actual UnitMove admission/cost prematurely changed source path/mission/coordinates/moves or archive dirty lineage");
  auto detached_step=*step;detached_step.moves_requested=0;detached_step.native_return=1;detached_step.boundary=runtime::BnwNativeUnitPathToBoundary::returned_no_move;
  refuses([&]{(void)owner->advance_native_existing_unit_move_admission(*id);});
  check(owner->read_native_existing_retained_path_cache(*id)==before_step_path&&archive_same(),"Detached numeric step observation replayed store/ShouldDim/UI or visual path effects");
  const auto after_paths_moves=owner->bind_native_existing_moves_archive(*id);check(after_paths_moves&&after_paths_moves->value==moves&&!after_paths_moves->dirty&&after_paths_moves->changes==0&&!actual_moves->registered_lifetime.owner_before(after_paths_moves->registered_lifetime)&&!after_paths_moves->registered_lifetime.owner_before(actual_moves->registered_lifetime),"Path preparation changed imported scalar or reminted its registered lineage");
  owner->require_current();check(runtime::bnw_unit_order_world_revision(state,host->identity(),host->database())==revision&&state.ui_source_revision==world_revision&&state.random.gameplay.state()==rng&&state.random.map.state()==map_rng&&state.city_dirty_events==events&&state.city_dirty_next_occurrence==occurrence,"Native search changed world/UI/RNG/event state");
  std::cout<<"PASS actual imported common path: source target, direction0 adjacent native cost/turn/moves/parent, same-control reuse, stale/foreign/out-of-map refusal; unit="<<*id<<" source="<<*origin<<" target="<<*target<<'\n';
 });
}

void natural_friendly_stack(const ui::SelectedBnwReadOnlyPreview&preview,const std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore>&owner){
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  const auto&state=host->session().state();check(state.world.all_players&&state.world.map_projection,"Natural friendly source roster absent");const auto&map=*state.world.map_projection;
  const auto scalar=[](const saves::SavedUnitObservation&unit,std::string_view key){const saves::UnitCoreField*out=nullptr;for(const auto&f:unit.core.fields)if(f.name&&std::string_view(f.name)==key){check(!out,"Natural friendly duplicate source scalar");out=&f;}check(out&&out->values.size()==1,"Natural friendly source scalar absent");return out->values.front();};
  const auto no_transport=[](const saves::SavedUnitObservation&unit){const saves::UnitTailNode*out=nullptr;const auto walk=[&](auto&&self,const saves::UnitTailNode&n)->void{if(n.name&&std::string_view(n.name)=="m_transportUnit.eOwner"){check(!out,"Natural friendly duplicate transport source");out=&n;}for(const auto&child:n.children)self(self,child);};for(const auto&block:unit.tail.blocks)walk(walk,block);check(out&&out->values.size()==1,"Natural friendly transport source absent");return out->values.front()==-1;};
  std::optional<std::uint64_t>single,empty,water;std::optional<std::int32_t>full_id;
  for(std::size_t index=0;index<map.cells.size();++index){const auto&cell=map.cells[index];check(cell.plot_type&&cell.units,"Natural friendly actual plot type/list absent");
   if(*cell.plot_type==3){if(!water)water=index;continue;}if(cell.units->empty()&&!empty)empty=index;
  }
  // Independent natural scenario: one actual linked object, no transport,
  // native Land and nontrade SQL metadata. There is no duplicate count/type/
  // enemy/cargo algorithm in this test; the source cardinality gives oracle1.
  for(const auto&unit:state.world.all_players->players.at(0).units.units){const auto id=scalar(unit,"m_iID"),x=scalar(unit,"m_iX"),y=scalar(unit,"m_iY");if(id<0||id>INT32_MAX-8192||x<0||y<0||!no_transport(unit))continue;
   check(static_cast<std::uint64_t>(x)<map.grid.width&&static_cast<std::uint64_t>(y)<map.grid.height,"Natural friendly source coordinate invalid");const auto index=map.grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));const auto&cell=map.cells.at(index);
   if(!cell.plot_type||*cell.plot_type==3||!cell.units||cell.units->size()!=1||cell.units->front().owner!=0||cell.units->front().id!=id)continue;
   const auto hash=scalar(unit,"idUnitType");check(hash>0&&hash<=UINT32_MAX,"Natural friendly source UnitInfo hash absent");const auto*info=host->types().find_hash(static_cast<std::uint32_t>(hash));check(info&&info->table=="Units","Natural friendly selected UnitInfo absent");bool land_nontrade=false;unsigned rows=0;
   const auto report=host->database().query_rows({"SELECT ID,Type,Domain,Trade FROM Units WHERE ID=?",{info->id}},[&](const auto&row){++rows;check(row.integer("ID")==info->id&&row.text("Type")==std::optional<std::string>{info->type},"Natural friendly independent SQL identity differs");land_nontrade=row.text("Domain")==std::optional<std::string>{"DOMAIN_LAND"}&&row.integer("Trade")==0;});check(report.all_succeeded()&&rows==1,"Natural friendly independent selected SQL unavailable");
   if(land_nontrade){single=index;full_id=static_cast<std::int32_t>(id);break;}
  }
  check(single&&empty&&water&&full_id,"Natural selected fixture lacks actual isolated land unit, empty land or water scenario");
  const auto before=runtime::bnw_unit_order_world_revision(state,host->identity(),host->database());const auto revision=state.ui_source_revision;const auto random=state.random.gameplay.state(),map_random=state.random.map.state();const auto events=state.city_dirty_events;const auto occurrence=state.city_dirty_next_occurrence;
  // Independent installed SQL define plus the actual one-object source
  // cardinality supplies this IsHasNoValidMove short-circuit scenario.
  unsigned limit_rows=0;const auto limit_report=host->database().query_rows({"SELECT Value FROM Defines WHERE Name='PLOT_UNIT_LIMIT'",{}},[&](const auto&row){++limit_rows;check(row.integer("Value")>=1,"Natural installed BNW stack limit does not permit one actual object");});check(limit_report.all_succeeded()&&limit_rows==1,"Natural installed BNW stack limit source unavailable");
  const auto services=owner->bind_native_existing_plot_unit_reads({});check(services.friendly_units_of_type&&services.cargo,"Canonical actual plot callbacks absent");const saves::SavedPlotUnitReference actual_handle{0,*full_id};
  check(services.friendly_units_of_type(state,*single,actual_handle)==std::optional<std::int64_t>{1},"Bound actual source plot count differs from independent one-object cardinality");
  check(services.cargo(state,*single,actual_handle)==std::optional<bool>{false},"Actual no-transport source object became cargo");
  check(!services.friendly_units_of_type(state,*single,{1,*full_id})&&!services.cargo(state,*single,{1,*full_id}),"Canonical actor0 callbacks fabricated a foreign actor answer");
  check(!services.friendly_units_of_type(state,*single,{0,*full_id+8192})&&!services.cargo(state,*single,{0,*full_id+8192}),"Bound plot callbacks accepted an obsolete serial");
  auto foreign_world=state; // Genuine compatible state copy, physically distinct from the canonical owner.
 refuses([&]{(void)services.friendly_units_of_type(foreign_world,*single,actual_handle);});refuses([&]{(void)services.cargo(foreign_world,*single,actual_handle);});
  bool bound_foreign_refused=false;std::thread bound_foreign([&]{try{(void)services.friendly_units_of_type(state,*single,actual_handle);}catch(const runtime::BnwNativeUnitAllocatorUnavailable&){bound_foreign_refused=true;}});bound_foreign.join();check(bound_foreign_refused,"Foreign thread entered genuine bound plot callback");
  check(owner->read_native_existing_has_no_valid_move(*full_id)==std::optional<bool>{false},"Actual isolated unit required a pathfinder despite genuine under-limit position");
  check(!owner->read_native_existing_has_no_valid_move(*full_id+8192),"Actual IsHasNoValidMove accepted obsolete native serial");
  check(owner->read_native_existing_friendly_stack_count(*full_id,*single)==std::optional<std::int32_t>{1},"Actual isolated native friendly count is not one");
  check(owner->read_native_existing_friendly_stack_count(*full_id,*single,false)==std::optional<std::int32_t>{1},"Actual full native friendly scan differs on isolated object");
  check(owner->read_native_existing_friendly_stack_count(*full_id,*empty)==std::optional<std::int32_t>{0},"Actual empty land plot acquired friendly object");
  check(!owner->read_native_existing_friendly_stack_count(*full_id+8192,*single),"Actual obsolete native serial was accepted as current object");
  refuses([&]{(void)owner->read_native_existing_friendly_stack_count(*full_id,*water);}); // Mandatory actual canEmbarkOnto, no invented false.
  bool foreign_refused=false;std::thread foreign([&]{try{(void)owner->read_native_existing_friendly_stack_count(*full_id,*single);}catch(const runtime::BnwNativeUnitAllocatorUnavailable&){foreign_refused=true;}});foreign.join();check(foreign_refused,"Foreign thread entered actual friendly source query");
  bool no_valid_foreign_refused=false;std::thread no_valid_foreign([&]{try{(void)owner->read_native_existing_has_no_valid_move(*full_id);}catch(const runtime::BnwNativeUnitAllocatorUnavailable&){no_valid_foreign_refused=true;}});no_valid_foreign.join();check(no_valid_foreign_refused,"Foreign thread entered actual IsHasNoValidMove source query");
  owner->require_current();check(runtime::bnw_unit_order_world_revision(state,host->identity(),host->database())==before&&state.ui_source_revision==revision&&state.random.gameplay.state()==random&&state.random.map.state()==map_random&&state.city_dirty_events==events&&state.city_dirty_next_occurrence==occurrence,"Natural friendly numeric query/refusals changed actual units/map/UI/events/RNG");
 });
}

void natural(const std::filesystem::path&save,const std::filesystem::path&root){
 std::cerr<<"Native allocator reached: bootstrap\n";
 auto loaded=ui::bootstrap_selected_bnw_preview(save,root,0,true);check(loaded.preview.host&&!loaded.preview.blocker,"Actual selected BNW allocator fixture unavailable");
 ui::SelectedBnwReadOnlyPreview preview(std::move(loaded.preview));require_original_common_fixture(preview);
 std::cerr<<"Native allocator reached: allocator factory\n";
 auto owner=runtime::BnwNativeUnitAllocatorCore::from_selected_preview(preview,0);
 std::cerr<<"Native allocator reached: sibling factory\n";
 auto sibling=runtime::BnwNativeUnitAllocatorCore::from_selected_preview(preview,0);
 natural_actual_path(preview,owner);
 natural_friendly_stack(preview,owner);
 // These are real selected rule rows, not a synthetic unit/prefix fixture.
 // Matching rule metadata proves the zero branch is naturally available;
 // actual execution still requires the genuine TRAIN predecessor chain.
 preview.with_owned_host_camera_input([&](auto host,const auto*){
 std::cerr<<"Native allocator reached: Scout class rules\n";
  const auto classes=runtime::load_bnw_unit_class_death_rules(host->database(),host->types());
 std::cerr<<"Native allocator reached: Scout support rules\n";
  // Independent actual selected SQL oracle: original ID holes are NULL
  // Info slots and have no hashed-array payload. Never densify IDs.
  std::map<std::int64_t,std::uint32_t>class_oracle;std::uint64_t native_span=0;
  const auto class_sql=host->database().query_rows({"SELECT ID,Type FROM UnitClasses WHERE ID>-1 ORDER BY ID",{}},[&](const auto&row){
   const auto id=row.integer("ID");const auto type=row.text("Type");const auto*entry=type?host->types().find(*type):nullptr;
   check(id>=0&&id<INT32_MAX&&entry&&entry->table=="UnitClasses"&&entry->id==id&&entry->hash!=0&&
    class_oracle.emplace(id,entry->hash).second,"Natural UnitClass SQL identity/unique ID differs");native_span=static_cast<std::uint64_t>(id)+1;
  });check(class_sql.all_succeeded()&&native_span==classes.class_slots&&classes.class_hashes.size()==native_span,"Natural UnitClass cardinality was confused with native slot span");
  std::size_t actual_null_slots=0;
  for(std::size_t slot=0;slot<classes.class_hashes.size();++slot){const auto found=class_oracle.find(static_cast<std::int64_t>(slot));
   if(found==class_oracle.end()){++actual_null_slots;check(classes.class_hashes[slot]==0,"Natural native NULL Info slot acquired a fake type/hash");}
   else{check(classes.class_hashes[slot]==found->second,"Natural native UnitClass ID was densified/remapped");}
  }
  check(class_oracle.size()+actual_null_slots==native_span,"Natural UnitClass slot universe incomplete");
  const auto support=runtime::load_bnw_unit_support_rules(host->database(),host->types(),classes);
 std::cerr<<"Native allocator reached: Scout rule lookup\n";
  const auto*scout=host->types().find("UNIT_SCOUT");check(scout&&scout->table=="Units"&&support.units.contains(scout->id),"Natural actual Scout support rule absent");
  const auto&r=support.units.at(scout->id);std::size_t rows=0;
 std::cerr<<"Native allocator reached: Scout unit SQL\n";
  const auto query=host->database().query_rows({"SELECT Domain,WorkRate,Unhappiness,ExtraMaintenanceCost,NukeDamageLevel,MilitarySupport FROM Units WHERE Type='UNIT_SCOUT'",{}},[&](const auto&row){
   ++rows;check(row.text("Domain")==std::optional<std::string>{"DOMAIN_LAND"}&&r.land&&row.integer("WorkRate")==r.work_rate&&
    row.integer("Unhappiness")==r.unhappiness&&row.integer("ExtraMaintenanceCost")==r.extra_maintenance_cost,"Natural Scout support metadata differs from SQL");
   check(row.integer("NukeDamageLevel")==r.nuke_damage_level&&r.nuke_damage_level==-1,"Natural Scout nuke counter branch is not zero-effect");
   check(row.integer("MilitarySupport")==1&&r.military_support,"Natural Scout actual military increment metadata differs from SQL");
  });check(query.all_succeeded()&&rows==1&&r.work_rate==0&&r.unhappiness==0&&r.extra_maintenance_cost==0,
   "Natural Scout does not reach the exact native first-three zero-counter branch");
 std::cerr<<"Native allocator reached: Scout resource SQL\n";
  const auto resources=host->database().query_rows({"SELECT ResourceType,Cost FROM Unit_ResourceQuantityRequirements WHERE UnitType='UNIT_SCOUT'",{}},[&](const auto&row){
   const auto key=row.text("ResourceType");const auto*info=key?host->types().find(*key):nullptr;
   check(info&&info->table=="Resources"&&row.integer("Cost")<=0,"Natural Scout has a reached ResourceUsed/notification effect");
  });check(resources.all_succeeded(),"Natural Scout actual resource relation query failed");
 });
 // SDK unsaved center handle has real NULL cache birth. This does NOT claim
 // parity with an already painted Original imported center. Both Core facades
 // share the same actual Session cache, rather than two renderer center maps.
 std::cerr<<"Native allocator reached: center cache birth\n";
 std::size_t actual_center_plots=0;
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  const auto&grid=host->session().state().world.map_header.grid;
  for(const auto&city:host->session().state().cities.players.at(0).cities){const auto&v=city.second;
   check(v.x>=0&&v.y>=0&&static_cast<std::uint64_t>(v.x)<grid.width&&static_cast<std::uint64_t>(v.y)<grid.height,"Natural center city plot absent");
   check(!owner->read_native_cached_plot_center(static_cast<std::int32_t>(v.x),static_cast<std::int32_t>(v.y))&&
    !sibling->read_native_cached_plot_center(static_cast<std::int32_t>(v.x),static_cast<std::int32_t>(v.y)),"Actual native cache did not begin NULL on SAME Session");++actual_center_plots;
  }
 });
 check(actual_center_plots>0,"Natural center cache birth test has no genuine city");
 std::cerr<<"Native allocator reached: garrison culture getter\n";
 const auto numeric=owner->read_native_garrison_policy_culture();
 std::cerr<<"Native allocator reached: garrison culture oracle\n";
 std::int64_t oracle=0;
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  const auto rules=runtime::BnwPolicyHappinessRules::load(host->database());
  const auto&player=host->session().state().world.all_players->players.at(0);
  const saves::TeamHashedArray*blocked=nullptr;
  for(const auto&block:player.strategy.blocks)if(block.name&&std::string_view(block.name)=="policies")
   for(const auto&row:block.hashes)if(row.name&&std::string_view(row.name)=="branch_blocked"){check(!blocked,"Duplicate natural blocked flags");blocked=&row;}
  check(blocked,"Natural blocked policy flags absent");const auto flags=rules.flags(*blocked,true);
  const auto query=host->database().query_rows({"SELECT ID,CulturePerGarrisonedUnit FROM Policies ORDER BY ID",{}},[&](const auto&row){
   const auto id=row.integer_at(0);if(!host->session().state().setup.players.at(0).policies.contains(id))return;
   const auto branch=rules.policies().at(id).blocked_branch;if(branch>=0&&flags.at(static_cast<std::size_t>(branch)))return;
   oracle+=row.integer_at(1);check(oracle>=INT32_MIN&&oracle<=INT32_MAX,"Natural garrison oracle overflow");
  });check(query.all_succeeded(),"Actual natural policy culture query failed");
 });
 check(numeric==oracle&&numeric==sibling->read_native_garrison_policy_culture(),"Actual policy garrison numeric modifier differs from SQL active/unblocked oracle");
 std::cerr<<"Native allocator reached: garrison missing predecessor\n";
 refuses([&]{(void)owner->advance_initial_city_garrison_fortify_prefix();}); // No genuine hidden-slot predecessor.
 std::cout<<"Actual native garrison policy culture: "<<numeric<<"\n";
 const auto sight_counts_before=preview.session().state().plot_visibility.counts;
 const auto sight_source_before=preview.session().state().plot_visibility.source_raw;
 const auto sight_revision_before=preview.session().state().ui_source_revision;
 refuses([&]{(void)owner->advance_initial_no_receiver_sight_prefix();}); // No real returned garrison/hidden slot.
 check(preview.session().state().plot_visibility.counts==sight_counts_before&&
  preview.session().state().plot_visibility.source_raw==sight_source_before&&
  preview.session().state().ui_source_revision==sight_revision_before,"Missing genuine sight predecessor changed actual map counters/revision");
 const auto plot_area_view=[&]{std::vector<std::int64_t> values;const auto&w=preview.session().state().world;
  check(w.map_plots&&w.map_projection&&w.map_regions,"Natural plot/area source unavailable");
  for(std::size_t i=0;i<w.map_plots->plots.size();++i){const saves::PlotArrayObservation*raw=nullptr;
   for(const auto&array:w.map_plots->plots[i].arrays)if(array.name&&std::string_view(array.name)=="units"){check(!raw,"Duplicate natural plotlist");raw=&array;}
   check(raw&&w.map_projection->cells.at(i).units,"Natural actual plotlist projection unavailable");
   values.push_back(static_cast<std::int64_t>(raw->values.size()));for(auto n:raw->values)values.push_back(n);
   values.push_back(static_cast<std::int64_t>(w.map_projection->cells[i].units->size()));for(const auto&handle:*w.map_projection->cells[i].units){values.push_back(handle.owner);values.push_back(handle.id);}
  }
  for(const auto&container:w.map_regions->containers)if(container.name&&std::string_view(container.name)=="areas")
   for(const auto&record:container.records){for(const auto&field:record.fields)if(field.name&&(std::string_view(field.name)=="id"||std::string_view(field.name)=="units"))values.push_back(field.value);
    for(const auto&array:record.arrays)if(array.name&&std::string_view(array.name)=="units_per_player")for(auto n:array.values)values.push_back(n);}
  return values;
 };
 const auto plot_area_snapshot=plot_area_view();
 const auto plot_area_before=preview.session().state().ui_source_revision;
 refuses([&]{(void)owner->advance_initial_plot_area_prefix();}); // real sight/journal/source required.
 check(preview.session().state().ui_source_revision==plot_area_before,"Missing actual plot/area predecessor changed source revision");
 check(plot_area_view()==plot_area_snapshot,"Missing actual plot/area predecessor changed real raw/projected lists or area counters");
 const auto city_strength_snapshot=[&]{std::vector<std::int64_t>v;
  for(const auto&city:preview.session().state().world.all_players->players.at(0).cities.cities)
   for(const auto&f:city.core.fields)if(f.name&&std::string_view(f.name)=="m_iStrengthValue"){check(f.values.size()==1,"Natural city strength scalar absent");v.push_back(f.values[0]);}
  return v;
 }();
 const auto city_events_before=preview.session().state().city_dirty_events;
 const auto city_occurrence_before=preview.session().state().city_dirty_next_occurrence;
 refuses([&]{(void)owner->advance_initial_city_strength_prefix();}); // No genuine actual linked pending predecessor.
 refuses([&]{(void)owner->advance_initial_post_city_checks_prefix();}); // Every actual city/renderer predecessor is mandatory.
  refuses([&]{(void)owner->advance_initial_happiness_prefix();}); // Actual six-stage stores need earned enemy-city/Position prefix.
 owner->require_current(); // Genuine no-effect admission refusal preserves actual Bank.
  refuses([&]{(void)owner->advance_initial_happiness_native_suffix_prefix();});
 owner->require_current(); // Genuine no-effect admission refusal preserves actual Bank.
  refuses([&]{(void)owner->advance_initial_happiness_return();});
 const auto actual_name_count_view=[&]{std::vector<std::uint64_t>v;
  const auto&w=preview.session().state().world;for(const auto&a:w.progression.arrays)if(a.name&&(std::string_view(a.name)=="unit_created_count"||std::string_view(a.name)=="unitclass_created_count")){
   v.push_back(a.columns);v.push_back(a.rows.size());for(const auto&r:a.rows){v.push_back(r.type_hash.has_value());if(r.type_hash)v.push_back(*r.type_hash);v.push_back(r.values.size());for(auto n:r.values)v.push_back(static_cast<std::uint64_t>(n));}}
  return v;
 };
 const auto name_counts_before=actual_name_count_view();const auto name_clock_before=preview.session().state().setup.game_turn;
 const auto name_rng_before=preview.session().state().random.gameplay.state();
 refuses([&]{(void)owner->advance_initial_name_game_counter_prefix();});
 check(actual_name_count_view()==name_counts_before&&preview.session().state().setup.game_turn==name_clock_before&&
  preview.session().state().random.gameplay.state()==name_rng_before&&preview.session().state().ui_source_revision==plot_area_before,
  "Missing actual center changed native created-count arrays/clock/RNG/revision");
 owner->require_current(); // Genuine absent-center admission has no effects or poison.
 refuses([&]{(void)owner->advance_initial_game_class_counter_prefix();});
 owner->require_current();
 check(actual_name_count_view()==name_counts_before&&preview.session().state().ui_source_revision==plot_area_before,
  "Missing actual name prefix changed UnitClass/Game counters or revision");
 const auto actual_team_counts=[&]{std::vector<std::uint64_t>v;check(preview.session().state().world.teams.has_value(),"Natural actual team roster absent");
  for(const auto&t:preview.session().state().world.teams->teams)for(const auto&a:t.hashes)if(a.name&&std::string_view(a.name)=="unit_class_count"){
   v.push_back(t.index);v.push_back(static_cast<std::uint64_t>(a.wire_count));v.push_back(a.rows.size());for(const auto&r:a.rows){v.push_back(r.type_hash);v.push_back(r.values.size());for(auto n:r.values)v.push_back(static_cast<std::uint64_t>(n));}}
  return v;
 };
 const auto team_counts_before=actual_team_counts();refuses([&]{(void)owner->advance_initial_team_class_counter_prefix();});owner->require_current();
 check(actual_team_counts()==team_counts_before&&preview.session().state().ui_source_revision==plot_area_before,
  "Missing actual Game class prefix changed native Team UnitClass counts/revision");
 const auto player_counts=[&]{std::vector<std::int64_t>v;for(const auto&p:preview.session().state().world.all_players->players){for(const auto&f:p.core.fields){
  if(f.name&&std::string_view(f.name)=="m_paiUnitClassCount"){v.push_back(static_cast<std::int64_t>(p.index));check(!f.wire_count||*f.wire_count<=static_cast<std::uint64_t>(INT64_MAX),"Natural class vector extent outside signed range");v.push_back(f.wire_count?static_cast<std::int64_t>(*f.wire_count):-1);v.insert(v.end(),f.values.begin(),f.values.end());}}}
  return v;};
 const auto player_counts_before=player_counts();refuses([&]{(void)owner->advance_initial_player_class_counter_prefix();});owner->require_current();
 check(player_counts()==player_counts_before&&actual_team_counts()==team_counts_before&&preview.session().state().ui_source_revision==plot_area_before,
  "Missing actual Team class prefix changed native Player/Team counts/revision");
 refuses([&]{(void)owner->advance_initial_support_zero_prefix();});owner->require_current();
 check(player_counts()==player_counts_before&&actual_team_counts()==team_counts_before&&preview.session().state().ui_source_revision==plot_area_before,
  "Missing actual Player class prefix changed native counters/revision");
 refuses([&]{(void)owner->advance_initial_resource_nuke_zero_prefix();});owner->require_current();
 check(player_counts()==player_counts_before&&actual_team_counts()==team_counts_before&&preview.session().state().ui_source_revision==plot_area_before,
  "Missing actual support prefix changed native counters/revision");
 const auto military_before=[&]{std::vector<std::int64_t>v;for(const auto&p:preview.session().state().world.all_players->players){for(const auto&f:p.core.fields){
  if(f.name&&std::string_view(f.name)=="m_iNumMilitaryUnits")v.insert(v.end(),f.values.begin(),f.values.end());}}
  return v;};
 const auto actual_military_before=military_before();refuses([&]{(void)owner->advance_initial_military_support_prefix();});owner->require_current();
 check(military_before()==actual_military_before&&player_counts()==player_counts_before&&preview.session().state().ui_source_revision==plot_area_before,
  "Missing actual resources prefix changed military/native counts/revision");

 refuses([&]{(void)owner->advance_initial_unit_info_promotion_prefix();});owner->require_current();
 check(military_before()==actual_military_before&&player_counts()==player_counts_before&&preview.session().state().ui_source_revision==plot_area_before,
  "Missing genuine military predecessor changed promotion/native world operands");

 refuses([&]{(void)owner->advance_initial_unit_info_promotions_return();});owner->require_current();
 check(military_before()==actual_military_before&&player_counts()==player_counts_before&&preview.session().state().ui_source_revision==plot_area_before,
  "Missing actual first promotion prefix changed native state during loop return");
 refuses([&]{(void)owner->advance_initial_traits_promotions_return();});owner->require_current();
 check(military_before()==actual_military_before&&player_counts()==player_counts_before&&preview.session().state().ui_source_revision==plot_area_before,
  "Missing actual UnitInfo return changed native state in Traits promotion loop");
 refuses([&]{(void)owner->advance_initial_player_promotions_return();});owner->require_current();
 check(military_before()==actual_military_before&&player_counts()==player_counts_before&&preview.session().state().ui_source_revision==plot_area_before,
  "Missing actual Traits return changed native state in player promotion loop");
 refuses([&]{(void)owner->advance_initial_embark_promotions_return();});owner->require_current();
 check(military_before()==actual_military_before&&player_counts()==player_counts_before&&preview.session().state().ui_source_revision==plot_area_before,
  "Missing actual Player promotion return changed native state in embark interval");
 refuses([&]{(void)owner->advance_initial_exotic_xp_return();});owner->require_current();
 check(military_before()==actual_military_before&&player_counts()==player_counts_before&&preview.session().state().ui_source_revision==plot_area_before,
  "Missing actual Embark return changed native state in Exotic/XP interval");
 refuses([&]{(void)owner->advance_initial_immobile_max_moves_prefix();});owner->require_current();
 check(military_before()==actual_military_before&&player_counts()==player_counts_before&&preview.session().state().ui_source_revision==plot_area_before,
  "Missing actual Exotic/XP return changed native state in Immobile/maxMoves interval");
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  // CvDllDatabaseUtility::PrefetchCollection selects ID > -1 ORDER BY ID
  // and appends NULL for every absent ID. A SQL visit ordinal is not an ID.
  // Verify the genuine native extent and all present Type/ID/hash bindings;
  // retain holes as absent slots rather than minting replacement promotions.
  const auto native=host->database().prefetch_info("UnitPromotions");
  check(native.all_succeeded()&&!native.overlapping_ids&&native.rows>0&&native.slots>=native.rows&&
   native.slots<=4'000'000&&native.holes==native.slots-native.rows,"Natural promotion native Info extent unavailable");
  std::set<std::int64_t>promotion_ids;std::uint64_t span=0;std::int64_t ignore=-1;std::size_t scout_grants=0;
  const auto rows=host->database().query_rows({"SELECT ID,Type,HoveringUnit FROM UnitPromotions WHERE ID > -1 ORDER BY ID",{}},[&](const auto&r){
   const auto id=r.integer("ID");const auto type=r.text("Type");const auto*entry=type?host->types().find(*type):nullptr;
    const auto*hashed=entry?host->types().find_hash(entry->hash):nullptr;
   if(id<0||static_cast<std::uint64_t>(id)<span||static_cast<std::uint64_t>(id)>=native.slots||!entry||
    entry->table!="UnitPromotions"||entry->id!=id||entry->hash!=content::saved_info_type_hash(*type)||
    !hashed||hashed->type!=entry->type||hashed->table!=entry->table||hashed->id!=entry->id||hashed->hash!=entry->hash||!promotion_ids.insert(id).second){
    std::cerr<<"Natural promotion native binding mismatch id="<<id<<" priorSpan="<<span<<" type="<<type.value_or("<NULL>")
     <<" nativeRows="<<native.rows<<" nativeSlots="<<native.slots<<" nativeHoles="<<native.holes
      <<" nameHash="<<(entry?entry->hash:0)<<" hashType="<<(hashed?hashed->type:"<ABSENT>")
      <<" hashTable="<<(hashed?hashed->table:"<ABSENT>")<<" hashID="<<(hashed?hashed->id:-1)<<" hashValue="<<(hashed?hashed->hash:0)<<'\n';
    check(false,"Natural promotion actual Type/ID/hash native slot binding differs");
   }
   span=static_cast<std::uint64_t>(id)+1;
  });
  check(rows.all_succeeded()&&rows.rows==native.rows&&promotion_ids.size()==native.rows&&span==native.slots,
   "Natural promotion complete native sparse slot traversal differs");
  std::size_t catalog_rows=0;for(const auto&[type,entry]:host->types().entries())if(entry.table=="UnitPromotions"&&entry.id>=0){
   ++catalog_rows;check(promotion_ids.contains(entry.id)&&entry.type==type,"Natural promotion catalog contains no actual native SQL slot");
  }
  check(catalog_rows==promotion_ids.size(),"Natural promotion native SQL/catalog cardinality differs");
  std::cerr<<"Natural promotion native universe firstID="<<*promotion_ids.begin()<<" rows="<<native.rows
   <<" slots="<<native.slots<<" holes="<<native.holes<<'\n';
  const auto scout=host->database().query_rows({"SELECT p.ID,p.IgnoreTerrainCost,p.HoveringUnit,p.Invisible,p.SeeInvisible FROM Unit_FreePromotions f JOIN UnitPromotions p ON p.Type=f.PromotionType WHERE f.UnitType='UNIT_SCOUT' ORDER BY p.ID",{}},[&](const auto&r){
   ++scout_grants;check(promotion_ids.contains(r.integer("ID"))&&r.integer("HoveringUnit")==0,"Natural Scout reached promotion slot/hover differs");
   if(r.integer("IgnoreTerrainCost")!=0){ignore=r.integer("ID");check(r.text("Invisible").value_or("").empty()&&r.text("SeeInvisible").value_or("").empty(),"Natural Scout ignore-terrain promotion has reached invisible dependency");}
  });check(scout.all_succeeded()&&scout_grants>0&&ignore>=0,"Natural Scout positive IgnoreTerrainCost effect absent in selected rules");
 }); // Actual SQL metadata only; genuine earned TRAIN prefix not executed here.
 owner->require_current(); // Genuine no-effect admission refusal preserves actual Bank.
  refuses([&]{(void)owner->advance_initial_popup_release_and_garrison();}); // No earned actual SAMEVM return; no false UI/dirty store.
 owner->require_current(); // No genuine returned UnitSetXY: admission has no effects.
  refuses([&]{(void)owner->advance_initial_enemy_city_range_prefix();}); // Actual UnitInfo/range order cannot be skipped.
 owner->require_current(); // Genuine no-effect admission refusal preserves actual Bank.
  refuses([&]{(void)owner->advance_initial_unit_info_dirty_prefix();}); // Genuine unborn node cannot skip Position/SetXY effects.
 owner->require_current(); // Genuine no-effect admission refusal preserves actual Bank.
  refuses([&]{(void)owner->advance_initial_set_position_prefix({});}); // No genuine renderer/earned position predecessor.
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  const auto state=host->session().read_state_lease();
  const auto rules=runtime::BnwBuildingHappinessRules::load(host->database());
  refuses([&]{(void)runtime::detail::bnw_happiness_buildings_reached_math_for_initial_unit(
   *state,rules,host->identity(),0,runtime::detail::BnwBuildingHappinessReachedStage::local_cities,*owner);});
 }); // Genuine imported world and rules cannot forge newborn position/garrison authority.

 std::vector<std::int64_t>city_strength_after;
 for(const auto&city:preview.session().state().world.all_players->players.at(0).cities.cities)
  for(const auto&f:city.core.fields)if(f.name&&std::string_view(f.name)=="m_iStrengthValue")city_strength_after.push_back(f.values.at(0));
 check(city_strength_after==city_strength_snapshot&&preview.session().state().city_dirty_events==city_events_before&&
  preview.session().state().city_dirty_next_occurrence==city_occurrence_before&&preview.session().state().ui_source_revision==plot_area_before,
  "Missing genuine city-strength predecessor changed native cache/event/occurrence/revision");
 const auto&source=preview.session().state().world.all_players->players.at(0).units;
 // Same native lookup used by newborn collision. Read actual imported
 // managers: full ID/slot-only agreement, obsolete serial and genuine holes.
 check(!owner->read_native_existing_unit_id(-1),"Native INVALID_INDEX GetAt did not return NULL");
 std::size_t actual_lookup_cases=0;
 for(const auto&saved:source.units){
  std::optional<std::int32_t> id;
  for(const auto&field:saved.core.fields)if(field.name&&std::string_view(field.name)=="m_iID"){
   check(!id&&field.encoding==saves::UnitCoreEncoding::i32&&field.values.size()==1&&
    field.values[0]>=0&&field.values[0]<=INT32_MAX,"Natural native lookup full ID invalid");id=static_cast<std::int32_t>(field.values[0]);
  }
  check(id.has_value(),"Natural native lookup full ID missing");
  check(owner->read_native_existing_unit_id(*id)==id&&owner->read_native_existing_unit_id(*id&8191)==id,
   "Actual native imported full/slot-only lookup differs");
  const auto alternate=(*id&8191)+((*id&~8191)==8192?16384:8192); // Real slot, a DIFFERENT nonzero serial.
  check(!owner->read_native_existing_unit_id(alternate),"Native obsolete full serial resolved to wrong current unit");
  ++actual_lookup_cases;
 }
 for(std::size_t slot=0;slot<source.header.source_slots;++slot){
  bool occupied=false;for(const auto&saved:source.units)for(const auto&field:saved.core.fields)
   if(field.name&&std::string_view(field.name)=="m_iID"&&(field.values.at(0)&8191)==static_cast<std::int64_t>(slot))occupied=true;
  if(!occupied)check(!owner->read_native_existing_unit_id(static_cast<std::int32_t>(slot)),"Native actual empty/unreached slot resolved a unit");
 }
 std::cout<<"Actual imported native GetAt occupied cases: "<<actual_lookup_cases<<"\n";
 const auto header=source.header;const auto count=source.units.size();const auto generation=preview.session().state_generation();
 std::int64_t slots=header.fields[0].value,last=header.fields[1].value,head=header.fields[2].value,freed=header.fields[3].value;
 auto serial=header.fields[4].value;auto next=header.next_free_indices;
 if(slots==0){slots=8;last=-1;head=-1;freed=0;serial=8192;next.assign(8,-1);}
 if(last==slots-1&&freed==0){check(slots<=4096,"Natural allocator fixture reached max capacity");slots*=2;next.resize(static_cast<std::size_t>(slots),-1);}
 const auto slot=freed?head:last+1;if(freed){head=next.at(static_cast<std::size_t>(head));--freed;}else ++last;
 auto pending=owner->reserve_constructor_reset_prefix();
 check(pending.player==0&&pending.slot==slot&&pending.reserved_id==serial+slot&&pending.slots==slots&&pending.last==last&&
 pending.head==head&&pending.free_count==freed&&pending.current_id==serial&&pending.next_free==next,"Actual FLTA reservation differs from source-order Add");
 check(pending.constructor_reset_scalars.at("m_iID")==0&&pending.constructor_reset_scalars.at("m_iX")==-INT32_MAX&&
 pending.constructor_reset_scalars.at("m_iY")==-INT32_MAX&&pending.constructor_reset_scalars.at("m_iDeployFromOperationTurn")==-100&&
 pending.constructor_reset_scalars.at("m_iLevel")==1&&pending.constructor_reset_scalars.at("m_iDamage")==0,
 "Actual allocated unit Reset prefix differs from native constructor");
 owner->require_current();sibling->require_current();
 const auto initial_archive=owner->pending_constructor_archive();
 check(!initial_archive.registered_lifetime.expired()&&initial_archive.binding_members.size()>100&&
  initial_archive.binding_members.front()=="m_iID"&&initial_archive.scalar_values.at("m_iID")==0&&
  initial_archive.scalar_values.at("m_iMissionAIX")==0&&initial_archive.vector_clear_members.empty(),
  "Actual registered constructor/archive initialization order differs");
 const auto storage=owner->execute_constructor_storage_tail();
 const auto archive=owner->pending_constructor_archive();
 check(archive.binding_names==initial_archive.binding_names&&archive.binding_members==initial_archive.binding_members&&
  !archive.registered_lifetime.expired(),"Native archive binding was rebuilt/detached during reset tail");
 for(const auto&[key,value]:archive.scalar_values)check(storage.scalars.at(key)==value,"Actual archive aliases stale copied scalar storage");
 check(storage.scalars.at("m_iHealOnPillageCount")==0,"Actual native constructor HealOnPillage plain member seed differs");
 check(archive.vector_clear_members==std::vector<std::string>{"m_featureDoubleMoveCount","m_terrainImpassableCount","m_featureImpassableCount",
  "m_extraTerrainAttackPercent","m_extraTerrainDefensePercent","m_extraFeatureAttackPercent","m_extraFeatureDefensePercent","m_extraUnitCombatModifier","m_unitClassModifier"},
  "Actual empty vector CLEAR archive actions differ from native uninitInfos order");
 check(std::find(archive.dirty_members.begin(),archive.dirty_members.end(),"m_eOwner")!=archive.dirty_members.end()&&
  std::find(archive.dirty_members.begin(),archive.dirty_members.end(),"m_eFacingDirection")!=archive.dirty_members.end(),
  "Actual Reset scalar assignments omitted native archive deltas");
 check(storage.slot==pending.slot&&storage.scalars.at("m_iID")==0&&storage.scalars.at("m_eOwner")==-1&&
  storage.scalars.at("m_eOriginalOwner")==-1&&storage.scalars.at("m_eUnitType")==-1&&storage.scalars.at("m_iBaseCombat")==0&&
  storage.scalars.at("m_eFacingDirection")==2&&storage.scalars.at("m_iNumAttacks")==1&&storage.scalars.at("m_iNumInterceptions")==1&&
  storage.scalars.at("m_iTacticalAIPlotX")==-INT32_MAX&&storage.scalars.at("m_iLastGameTurnAtFullHealth")==-1,
  "Actual constructor storage scalar tail differs from SDK Reset(true)");
 check(storage.indexed.at("m_extraDomainModifiers")==std::vector<std::int32_t>(5,0)&&
  storage.last_path_destination==UINT32_MAX&&storage.religion==-1&&storage.religion_strength==0&&storage.religion_spreads==0,
  "Actual constructor domain/religion/path storage differs from native defaults");
 for(const auto*key:{"m_combatUnit.owner","m_combatUnit.id","m_transportUnit.owner","m_transportUnit.id"})
  check(storage.scalars.at(key)==-1,"Actual constructor IDInfo reset differs");
 for(const auto&[key,values]:storage.indexed)if(key!="m_extraDomainModifiers")check(values.empty(),"Constructor incorrectly ran nonconstructor indexed reset");
 for(const auto&[key,value]:storage.strings){(void)key;check(value.empty(),"Actual constructor string clear missing");}
 check(storage.scalars.at("m_iMissionAIX")==0&&storage.scalars.at("m_iMissionAIY")==0&&storage.scalars.at("m_eMissionAIType")==0&&
  storage.scalars.at("m_eUnitAIType")==0&&storage.scalars.at("m_iArmyId")==0&&storage.promotion_bits.empty(),
  "Constructor archive.reset default was confused with Init(false) sentinels or allocated promotion span bits");
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  // Original PrefetchCollection keeps native ID slots and NULL holes.
  const auto native=host->database().prefetch_info("UnitPromotions");
  check(native.all_succeeded()&&native.rows>0&&native.slots>=native.rows&&native.slots<=4'000'000&&
   native.holes==native.slots-native.rows,"Constructor promotion native Info extent unavailable");
  std::vector<std::int32_t> actual(static_cast<std::size_t>(native.slots),-1);std::uint64_t span=0;
  const auto q=host->database().query_rows({"SELECT ID,Type FROM UnitPromotions WHERE ID > -1 ORDER BY ID",{}},[&](const auto&row){
   const auto id=row.integer("ID");const auto type=row.text("Type");const auto* named=type?host->types().find(*type):nullptr;
   const auto* hashed=named?host->types().find_hash(named->hash):nullptr;
   check(id>=0&&id<=INT32_MAX&&static_cast<std::uint64_t>(id)>=span&&static_cast<std::uint64_t>(id)<native.slots&&
    named&&named->table=="UnitPromotions"&&named->id==id&&named->hash==content::saved_info_type_hash(*type)&&hashed&&
    hashed->type==named->type&&hashed->table==named->table&&hashed->id==named->id&&hashed->hash==named->hash&&
    actual[static_cast<std::size_t>(id)]==-1,"Constructor promotion independent SQL Type/ID/hash binding differs");
   actual[static_cast<std::size_t>(id)]=static_cast<std::int32_t>(id);span=static_cast<std::uint64_t>(id)+1;
  });check(q.all_succeeded()&&q.rows==native.rows&&span==native.slots&&
   static_cast<std::uint64_t>(std::count(actual.begin(),actual.end(),-1))==native.holes,
   "Constructor promotion complete independent native slots/NULL holes differ");
  if(actual.size()!=storage.promotion_info_ids.size())
   std::cerr<<"Constructor promotion span mismatch expected="<<actual.size()<<" actual="<<storage.promotion_info_ids.size()
    <<" rows="<<native.rows<<" holes="<<native.holes<<'\n';
  check(actual.size()==storage.promotion_info_ids.size(),"Actual constructor promotion native span differs");
  for(std::size_t slot=0;slot<actual.size();++slot)if(actual[slot]!=storage.promotion_info_ids[slot]){
   std::cerr<<"Constructor promotion slot mismatch slot="<<slot<<" expected="<<actual[slot]
    <<" actual="<<storage.promotion_info_ids[slot]<<" nativeRows="<<native.rows<<" nativeSlots="<<native.slots
    <<" nativeHoles="<<native.holes<<'\n';
   check(false,"Actual constructor promotion binding differs from complete genuine Info rows");
  }
 });
 owner->require_current();sibling->require_current();
 refuses([&]{(void)owner->first_trait_free_unit(0);});
 owner->require_current(); // Missing Traits cannot invalidate actual allocator.
 check(source.units.size()==count&&source.header.next_free_indices==header.next_free_indices&&
 preview.session().state_generation()==generation,"Partial unit constructor published world/roster");
 for(std::size_t i=0;i<header.fields.size();++i)check(header.fields[i].value==source.header.fields[i].value,"Partial constructor published FLTA header/serial");
 refuses([&]{owner->require_completed_unit_initialization();});
 bool foreign=false;std::thread other([&]{try{owner->require_current();}catch(const runtime::BnwNativeUnitAllocatorUnavailable&){foreign=true;}});other.join();
 check(foreign,"Foreign allocator borrow accepted");owner->require_current();
 const auto old_lifetime=archive.registered_lifetime;
 owner->discard_unpublished_constructor();
 check(old_lifetime.expired(),"Actual unpublished constructor remains alive after weak registration cleanup");
 owner->require_current();sibling->require_current();
 const auto retry=owner->reserve_constructor_reset_prefix();
 check(retry.slot==pending.slot&&retry.reserved_id==pending.reserved_id&&retry.current_id==pending.current_id,
  "Unpublished constructor abort did not restore SAME original FLTA reservation");
 const auto retry_archive=owner->pending_constructor_archive();
 check(!retry_archive.registered_lifetime.expired()&&old_lifetime.expired(),"Recycled slot revived stale constructor handle");
 const auto same_world=preview.session().read_state_lease();
 const auto map_rng=same_world->random.map.state(),game_rng=same_world->random.gameplay.state();
 const auto map_algorithm=same_world->random.map.algorithm(),game_algorithm=same_world->random.gameplay.algorithm();
 const auto old_revision=same_world->ui_source_revision;
 (void)owner->execute_constructor_storage_tail();
 const auto assigned=owner->assign_pending_unit_native_id();
 check(assigned.player==0&&assigned.slot==retry.slot&&assigned.full_id==retry.reserved_id&&assigned.serial_before==retry.current_id&&
  assigned.serial_after==assigned.serial_before+8192&&source.header.fields[4].value==assigned.serial_after,
  "Actual SetID/currentID advance differs from primary native serial/slot oracle");
 refuses([&]{(void)owner->execute_selected_trait_unit_init_reset(preview);}); // No public result grants selected Init.
 refuses([&]{(void)owner->execute_bound_unit_init_reset(preview);}); // No opaque TRAIN producer bound either.
 refuses([&]{(void)owner->execute_bound_unit_initial_facing_hover_prefix();});
 refuses([&]{(void)owner->begin_initial_set_xy_prefix();}); // No completed TRAIN/init authority.
 refuses([&]{(void)owner->advance_initial_destination_collision_prefix();}); // Actual entered SetXY is required.
 refuses([&]{(void)owner->advance_initial_city_visibility_prefix({});}); // Null is not a genuine actual renderer.
 const auto assigned_archive=owner->pending_constructor_archive();
 check(assigned_archive.scalar_values.at("m_iID")==assigned.full_id&&
  std::find(assigned_archive.dirty_members.begin(),assigned_archive.dirty_members.end(),"m_iID")!=assigned_archive.dirty_members.end(),
  "Actual SetID omitted SAME node/registered archive write");
 check(preview.session().read_state_lease()==same_world&&preview.session().state_generation()==generation&&
  same_world->ui_source_revision==old_revision+1&&source.units.size()==count&&source.header.next_free_indices==header.next_free_indices,
  "Native ID prefix replaced world or prematurely published raw unit/FLTA membership");
 for(std::size_t i=0;i<4;++i)check(source.header.fields[i].value==header.fields[i].value,"Native ID prefix changed raw unpublished FLTA membership");
 check(same_world->random.map.state()==map_rng&&same_world->random.gameplay.state()==game_rng&&
  same_world->random.map.algorithm()==map_algorithm&&same_world->random.gameplay.algorithm()==game_algorithm,
  "Native ID assignment spent RNG");
 refuses([&]{(void)owner->assign_pending_unit_native_id();});
 check(source.header.fields[4].value==assigned.serial_after&&same_world->ui_source_revision==old_revision+1,
  "Repeated SetID burned another serial or changed revision");
 owner->require_current();sibling->require_current();refuses([&]{owner->require_completed_unit_initialization();});
 const auto burned_lifetime=assigned_archive.registered_lifetime;
 owner->discard_unpublished_constructor();check(burned_lifetime.expired(),"Assigned unpublished object survived actual weak cleanup");
 check(source.header.fields[4].value==assigned.serial_after,"Unpublished abort rewound burned global serial");
 const auto fresh=owner->reserve_constructor_reset_prefix();
 check(fresh.slot==assigned.slot&&fresh.reserved_id==assigned.full_id+8192&&fresh.current_id==assigned.serial_after,
  "Reused unpublished slot revived burned full ID");
 // The second genuine facade shares SAME bank, so cannot allocate a duplicate
 // ID by rebuilding an independent source header while constructor is pending.
 refuses([&]{(void)sibling->reserve_constructor_reset_prefix();});
 refuses([&]{owner->require_current();});
 {
  auto loaded_b=ui::bootstrap_selected_bnw_preview(save,root,0,true);check(loaded_b.preview.host&&!loaded_b.preview.blocker,"Second genuine allocator Host unavailable");
  ui::SelectedBnwReadOnlyPreview other_preview(std::move(loaded_b.preview));
  auto child=runtime::BnwNativeUnitAllocatorCore::from_selected_preview(other_preview,0);
  const auto before=other_preview.session().state().world.all_players->players.at(0).units.units.size();
  (void)child->reserve_constructor_reset_prefix();
  other_preview.with_owned_host_camera_input([&](auto host,const auto*){
   auto&actual=const_cast<ui::SelectedBnwPreviewHost&>(*host).session().mutable_state();
   auto&serial_field=actual.world.all_players->players.at(0).units.header.fields.at(4);
   check(serial_field.value<=INT32_MAX-8192,"Natural stale serial operand overflows");serial_field.value+=8192;
  });
  refuses([&]{(void)child->reserve_constructor_reset_prefix();});
  refuses([&]{(void)child->execute_constructor_storage_tail();});
  const auto changed_serial=other_preview.session().state().world.all_players->players.at(0).units.header.fields.at(4).value;
  refuses([&]{(void)child->assign_pending_unit_native_id();});
  check(other_preview.session().state().world.all_players->players.at(0).units.header.fields.at(4).value==changed_serial,"Stale SetID burned serial after actual source retirement");
  check(other_preview.session().state().world.all_players->players.at(0).units.units.size()==before,"Stale allocator refusal inserted unit");
 }
 std::cout<<"PASS natural native FLTA reservation and constructor owned storage/archive/weak registration; SetID/serial burn; full Init/placement UNSUPPORTED\n";
}
void natural_selected_init(const std::filesystem::path&save,const std::filesystem::path&root){
 auto loaded=ui::bootstrap_selected_bnw_preview(save,root,0,true);check(loaded.preview.host&&!loaded.preview.blocker,"Actual selected Init fixture unavailable");
 ui::SelectedBnwReadOnlyPreview preview(std::move(loaded.preview));require_original_common_fixture(preview);const auto state=preview.session().read_state_lease();bool reached=false;
 for(const auto&[actor,player]:state->setup.players){
  if(!player.ever_alive||state->science.minor_players.contains(actor)||state->science.barbarian_players.contains(actor)||actor<0||actor>INT32_MAX)continue;
  std::shared_ptr<const runtime::BnwMajorPlayerTraitsOwner> traits;
  try{traits=runtime::BnwMajorPlayerTraitsOwner::from_selected_preview(preview,static_cast<std::int32_t>(actor));}
  catch(const runtime::BnwMajorPlayerTraitsUnavailable&e){std::cout<<"UNAVAILABLE actual Init Trait binding: "<<e.what()<<'\n';continue;}
  const auto snap=traits->snapshot();std::int64_t selected_tech=-1;
  for(const auto&field:snap.cache)if(field.name&&std::string_view(field.name)=="m_aFreeTraitUnits"){
   check(field.values.size()%2==0,"Actual free-unit cache pair extent differs");
   for(std::size_t i=1;i<field.values.size();i+=2)if(field.values[i]>=0)
    for(const auto&tech:state->technologies)if(tech.team()==player.team&&tech.has_tech(field.values[i]))selected_tech=field.values[i];
  }
  if(selected_tech<0)continue;
  auto owner=runtime::BnwNativeUnitAllocatorCore::from_selected_preview(preview,static_cast<std::int32_t>(actor));
  owner->bind_trait_iterator_from_selected_preview(preview);const auto selected=owner->first_trait_free_unit(selected_tech);
  check(selected.unit>=0,"Genuine free Trait unit source disappeared");
  const auto&raw=state->world.all_players->players.at(static_cast<std::size_t>(actor)).units;
  const auto count=raw.units.size();const auto old_serial=raw.header.fields[4].value;const auto rng=state->random.gameplay.state(),map_rng=state->random.map.state();
  (void)owner->reserve_constructor_reset_prefix();(void)owner->execute_constructor_storage_tail();const auto id=owner->assign_pending_unit_native_id();
  // Independent SQL aggregate oracle, captured BEFORE the real earned
  // Reset(false) call. It does not call the production span helper/prefetch.
  std::uint64_t sql_class_span=0,sql_class_rows=0;std::set<std::int64_t>sql_class_ids;
  preview.with_owned_host_camera_input([&](auto host,const auto*){
   std::size_t aggregates=0;const auto shape=host->database().query_rows({"SELECT COUNT(*),COALESCE(MAX(ID),-1)+1 FROM UnitClasses WHERE ID>-1",{}},[&](const auto&row){
    ++aggregates;const auto rows=row.integer_at(0),span=row.integer_at(1);check(rows>0&&span>=rows&&span<=4'000'000,"Actual installed UnitClass SQL aggregate shape invalid");
    sql_class_rows=static_cast<std::uint64_t>(rows);sql_class_span=static_cast<std::uint64_t>(span);
   });check(shape.all_succeeded()&&aggregates==1,"Actual installed UnitClass SQL extent oracle failed");
   const auto ids=host->database().query_rows({"SELECT ID FROM UnitClasses WHERE ID>-1 ORDER BY ID",{}},[&](const auto&row){
    const auto id=row.integer_at(0);check(id>=0&&static_cast<std::uint64_t>(id)<sql_class_span&&sql_class_ids.insert(id).second,"Actual installed UnitClass SQL duplicate/outside span");
   });check(ids.all_succeeded()&&sql_class_ids.size()==sql_class_rows,"Actual installed UnitClass SQL IDs differ from independent aggregate");
  });
  const auto initialized=owner->execute_selected_trait_unit_init_reset(preview);
  check(initialized.scalars.at("m_iID")==id.full_id&&initialized.scalars.at("m_eOwner")==actor&&
   initialized.scalars.at("m_eOriginalOwner")==actor&&initialized.scalars.at("m_eUnitType")==selected.unit&&
   initialized.scalars.at("m_iMissionAIX")==-INT32_MAX&&initialized.scalars.at("m_eMissionAIType")==-1&&
   initialized.scalars.at("m_eUnitAIType")==-1&&initialized.promotion_bits.empty()&&initialized.religion==-1,
   "Actual selected Init Reset(false) native scalar/religion/promotions differ");
  preview.with_owned_host_camera_input([&](auto host,const auto*){
   const auto validate=[&](const char*table,const char*key){const auto info=host->database().prefetch_info(table);
    check(info.all_succeeded()&&initialized.indexed.at(key).size()==info.slots,"Actual indexed Init span differs from genuine native Info universe");
    for(auto value:initialized.indexed.at(key))check(value==0,"Actual indexed Init not reset to native zero");};
   validate("Terrains","m_terrainDoubleMoveCount");validate("Features","m_featureDoubleMoveCount");
   validate("UnitCombatInfos","m_extraUnitCombatModifier");validate("UnitClasses","m_unitClassModifier");
   const auto&class_modifiers=initialized.indexed.at("m_unitClassModifier");
   check(class_modifiers.size()==sql_class_span,"Actual Reset(false) densified installed UnitClass NULL Info holes");
   std::uint64_t observed_holes=0;for(std::uint64_t slot=0;slot<sql_class_span;++slot){
    check(class_modifiers.at(static_cast<std::size_t>(slot))==0,"Actual Reset(false) native UnitClass vector resize/reset is not zero");
    if(!sql_class_ids.contains(static_cast<std::int64_t>(slot))){++observed_holes;}
   }
   check(observed_holes==sql_class_span-sql_class_rows,"Actual Reset(false) installed SQL NULL-slot universe changed");
   if(observed_holes){std::cout<<"PASS actual installed SQL NULL UnitClass holes through earned Reset(false): "<<observed_holes<<'\n';}
   else{std::cout<<"UNAVAILABLE installed UnitClass hole scenario: selected SQL table is dense\n";}

   std::int64_t combat=-1;const auto q=host->database().query_rows({"SELECT Combat FROM Units WHERE ID=?",{std::int64_t(selected.unit)}},[&](const auto&row){combat=row.integer_at(0);});
   check(q.all_succeeded()&&combat==initialized.scalars.at("m_iBaseCombat"),"Actual selected UnitInfo Combat differs");
  });
  bool hover_grant=false;preview.with_owned_host_camera_input([&](auto host,const auto*){
   const auto q=host->database().query_rows({"SELECT p.HoveringUnit FROM Unit_FreePromotions f JOIN Units u ON u.Type=f.UnitType JOIN UnitPromotions p ON p.Type=f.PromotionType WHERE u.ID=?",{std::int64_t(selected.unit)}},[&](const auto&row){
    const auto value=row.integer_at(0);check(value==0||value==1,"Natural initial-hover oracle has unknown flag");hover_grant=hover_grant||value!=0;
   });check(q.all_succeeded(),"Natural actual free-hover oracle failed");
  });
  const auto before_facing=owner->pending_constructor_archive();
  if(hover_grant){
   refuses([&]{(void)owner->execute_bound_unit_initial_facing_hover_prefix();});
   const auto unchanged=owner->pending_constructor_archive();check(unchanged.scalar_values==before_facing.scalar_values&&unchanged.dirty_members==before_facing.dirty_members,
    "Reached unsupported hover promotion partially stored facing/archive state");
  }else{
   const auto facing=owner->execute_bound_unit_initial_facing_hover_prefix();
   check(facing.scalars.at("m_eFacingDirection")==2&&facing.scalars.at("m_iMapLayer")==0&&facing.scalars.at("m_iNumGoodyHutsPopped")==0&&
    facing.promotion_bits.empty()&&facing.scalars.at("m_iX")==-INT32_MAX&&facing.scalars.at("m_iY")==-INT32_MAX,
    "Actual no-hover Init loop/default-facing/layer/goody prefix differs from SDK");
   refuses([&]{(void)owner->execute_bound_unit_initial_facing_hover_prefix();});
   refuses([&]{(void)owner->begin_initial_set_xy_prefix();}); // Actual Trait placement/RNG source is not completed TRAIN.
   std::cout<<"PASS actual no-hover Info loop/facing prefix; Trait initial placement source remains unavailable\n";
  }
  check(raw.units.size()==count&&raw.header.fields[4].value==old_serial+8192&&state->random.gameplay.state()==rng&&state->random.map.state()==map_rng,
   "Native Init Reset prefix published roster, advanced another serial or spent placement RNG");
  refuses([&]{(void)owner->execute_selected_trait_unit_init_reset(preview);});owner->require_current();
  refuses([&]{owner->require_completed_unit_initialization();});owner->discard_unpublished_constructor();
  reached=true;std::cout<<"PASS actual selected Trait unit Init promotions/religion/Reset(false); hovering/setXY/full publication UNSUPPORTED\n";break;
 }
 if(!reached)std::cout<<"UNAVAILABLE actual naturally acquired Trait free-unit binding; positive Init Reset case UNEXECUTED\n";
}
}
void natural_continue_mission_cache(const std::filesystem::path&save,const std::filesystem::path&root){
 auto loaded=ui::bootstrap_selected_bnw_preview(save,root,0,true);check(loaded.preview.host&&!loaded.preview.blocker,"Actual ContinueMission cache fixture unavailable");
 ui::SelectedBnwReadOnlyPreview preview(std::move(loaded.preview));require_original_common_fixture(preview);const auto owner=runtime::BnwNativeUnitAllocatorCore::from_selected_preview(preview,0);
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  const auto&state=host->session().state();const auto&units=state.world.all_players->players.at(0).units.units;
  if(units.empty()){std::cout<<"UNEXECUTED: actual imported ContinueMission first-effect unit absent\n";return;}
  const auto scalar=[](const saves::SavedUnitObservation&u,std::string_view key){const saves::UnitCoreField*f=nullptr;for(const auto&v:u.core.fields)if(v.name&&std::string_view(v.name)==key){check(!f,"Natural continue duplicate source field");f=&v;}check(f&&f->values.size()==1,"Natural continue actual source scalar absent");return f->values.front();};
  const saves::SavedUnitObservation*chosen=&units.front();
  for(const auto&candidate:units){const auto candidate_id=scalar(candidate,"m_iID");check(candidate_id>=0&&candidate_id<=INT32_MAX,"Natural head candidate fullID unavailable");const auto actual=owner->read_native_existing_mission_queue(static_cast<std::int32_t>(candidate_id));check(actual.has_value(),"Natural actual head supplier absent");if(actual->queue.empty()&&actual->activity==0){chosen=&candidate;break;}}
  const auto&unit=*chosen;
  const auto wide=scalar(unit,"m_iID");check(wide>=0&&wide<=INT32_MAX-8192,"Natural continue actual source fullID unavailable");const auto id=static_cast<std::int32_t>(wide);
  const auto before_path=owner->read_native_existing_retained_path_cache(id);const auto before_mission=owner->read_native_existing_mission_queue(id);const auto before_runtime=owner->read_native_existing_mission_runtime(id);
  check(before_path&&before_mission&&before_runtime,"Natural continue actual providers absent");
  const auto flags=scalar(unit,"m_iFlags"),activity=scalar(unit,"m_eActivityType"),moves=scalar(unit,"m_iMoves");
  const auto before=runtime::bnw_unit_order_world_revision(state,host->identity(),host->database());const auto game_rng=state.random.gameplay.state(),map_rng=state.random.map.state();
  check(!owner->advance_native_existing_continue_mission_cache_prefix(id+8192),"Stale serial cleared genuine mission cache");
  const auto entry=owner->advance_native_existing_continue_mission_cache_prefix(id);const auto after_path=owner->read_native_existing_retained_path_cache(id);
  check(entry&&entry->owner==0&&entry->full_id==id&&entry->path_clear_operation==before_path->clear_operations+1&&after_path&&after_path->nodes.empty()&&after_path->destination==UINT32_MAX&&after_path->clear_operations==before_path->clear_operations+1&&after_path->generation_operations==before_path->generation_operations&&after_path->path_to_reject_clear_operations==before_path->path_to_reject_clear_operations,"Actual ContinueMission first ClearPathCache did not clear BOTH native fields exactly once on canonical owner");
  const auto after_mission=owner->read_native_existing_mission_queue(id);const auto after_runtime=owner->read_native_existing_mission_runtime(id);
  check(after_mission&&after_mission->queue==before_mission->queue&&after_mission->unit_flags==before_mission->unit_flags&&after_mission->activity==before_mission->activity&&after_runtime&&after_runtime->mission_timer==before_runtime->mission_timer&&after_runtime->visual_locations==before_runtime->visual_locations&&after_runtime->busy==before_runtime->busy&&!after_runtime->registered_lifetime.owner_before(before_runtime->registered_lifetime)&&!before_runtime->registered_lifetime.owner_before(after_runtime->registered_lifetime)&&scalar(unit,"m_iFlags")==flags&&scalar(unit,"m_eActivityType")==activity&&scalar(unit,"m_iMoves")==moves&&runtime::bnw_unit_order_world_revision(state,host->identity(),host->database())==before&&state.random.gameplay.state()==game_rng&&state.random.map.state()==map_rng,"ContinueMission cache-only prefix fabricated evaluating flag/archive/body/activity/timer/visual/moves/RNG effects");
  auto detached=*entry;detached.path_clear_operation=UINT64_MAX;
  refuses([&]{(void)owner->advance_native_existing_continue_mission_cache_prefix(id);});
  refuses([&]{(void)owner->advance_native_existing_unit_path_to_prefix(id,static_cast<std::int32_t>(scalar(unit,"m_iX")),static_cast<std::int32_t>(scalar(unit,"m_iY")));});
  check(owner->read_native_existing_retained_path_cache(id)==after_path,"Detached mission entry count or command bypass repeated native cache/flag/body effects");
  const auto cache_checkpoint=owner->checkpoint_native_existing_continue_mission(id);check(cache_checkpoint&&cache_checkpoint->section().id==43,"Actual canonical mission cache journal did not mint reserved section43");
  const auto phase=owner->advance_native_existing_continue_mission_flags_prefix(id);const auto expected=static_cast<std::int32_t>(flags)|1;
  check(phase.owner==0&&phase.full_id==id&&phase.phase==runtime::BnwUnitMissionContinuationPhase::evaluating_flags_stored_before_head&&phase.flags==expected&&phase.loaded_flags==flags&&phase.archive_changes==(expected!=flags?1U:0U)&&phase.archive_dirty==(expected!=flags)&&scalar(unit,"m_iFlags")==expected,"Real Flags|1 phase or equal-aware archive dirty lineage differs from SDK");
  check(owner->read_native_existing_retained_path_cache(id)==after_path&&owner->read_native_existing_mission_queue(id)->queue==before_mission->queue&&owner->read_native_existing_mission_queue(id)->activity==before_mission->activity&&scalar(unit,"m_iMoves")==moves&&state.random.gameplay.state()==game_rng&&state.random.map.state()==map_rng,"Evaluating Flags store fabricated path/head/activity/moves/RNG effects");
  refuses([&]{(void)owner->advance_native_existing_continue_mission_flags_prefix(id);});const auto flag_checkpoint=owner->checkpoint_native_existing_continue_mission(id);check(flag_checkpoint->section().bytes==owner->checkpoint_native_existing_continue_mission(id)->section().bytes,"Stable canonical mission checkpoint changed on repeated read");
  const auto replay_fresh=[&](const saves::CivisSaveSection&section,bool evaluating){
   auto restart=ui::bootstrap_selected_bnw_preview(save,root,0,true);check(restart.preview.host&&!restart.preview.blocker,"Actual pending-resume restart fixture unavailable");ui::SelectedBnwReadOnlyPreview reloaded(std::move(restart.preview));require_original_common_fixture(reloaded);auto restored=runtime::BnwNativeUnitAllocatorCore::from_selected_preview(reloaded,0);
   reloaded.with_owned_host_camera_input([&](auto restarted_host,const auto*){
    const auto&restarted_state=restarted_host->session().state();const auto empty=restored->read_native_existing_retained_path_cache(id);const auto original=restored->read_native_existing_mission_queue(id);check(empty&&original&&original->unit_flags==flags,"Fresh original Read mission state unavailable");const auto world_before=runtime::bnw_unit_order_world_revision(restarted_state,restarted_host->identity(),restarted_host->database());
    for(unsigned kind=0;kind<4;++kind){auto bad=section;if(kind==0)bad.bytes.pop_back();if(kind==1)bad.bytes.push_back(0);if(kind==2)bad.id=18;if(kind==3)bad.bytes.at(12)=static_cast<std::uint8_t>(bad.bytes.at(12)=='0'?'1':'0');constexpr std::array<std::string_view,4>reasons{"Truncated mission continuation checkpoint","Trailing mission continuation bytes","Wrong mission continuation section identity/flags","Foreign mission continuation payload/rules"};refuses_runtime_boundary([&]{(void)restored->restore_native_existing_continue_mission(bad);},reasons.at(kind));check(restored->read_native_existing_retained_path_cache(id)==empty&&restored->read_native_existing_mission_queue(id)->unit_flags==flags&&runtime::bnw_unit_order_world_revision(restarted_state,restarted_host->identity(),restarted_host->database())==world_before,"Rejected mission checkpoint changed world/path/Flags before commit");}
    const auto receipt=restored->restore_native_existing_continue_mission(section);check(receipt.full_id==id&&receipt.flags==(evaluating?expected:flags)&&receipt.path_clear_operation==after_path->clear_operations&&restored->read_native_existing_retained_path_cache(id)==after_path&&restored->read_native_existing_mission_queue(id)->queue==original->queue,"Fresh canonical pending-resume did not restore earned physical field/cache/phase");
    check(restored->checkpoint_native_existing_continue_mission(id)->section().bytes==section.bytes,"Pending-resume roundtrip changed canonical phase/journal/dirty lineage");refuses([&]{(void)restored->restore_native_existing_continue_mission(section);});refuses([&]{(void)restored->advance_native_existing_continue_mission_cache_prefix(id);});
    if(!evaluating){const auto next=restored->advance_native_existing_continue_mission_flags_prefix(id);check(next.flags==expected&&restored->checkpoint_native_existing_continue_mission(id)->section().bytes==flag_checkpoint->section().bytes,"Resumed cache phase repeated clear or failed genuine once evaluating store");}
    else refuses([&]{(void)restored->advance_native_existing_continue_mission_flags_prefix(id);});
    check(restarted_state.random.gameplay.state()==game_rng&&restarted_state.random.map.state()==map_rng,"Pending mission restore consumed RNG");
   });
  };
  replay_fresh(cache_checkpoint->section(),false);replay_fresh(flag_checkpoint->section(),true);
  if(host->session().source().source.container==saves::SaveContainerKind::civ5){
   // A plain Civis payload/Flags carrier is real source data, but lacks the
   // earned section43 unpublished join. It must refuse at the actual Common
   // owner, before any later Game leaf; this is not a positive mission restore.
   const bool scalar_changed=expected!=flags;
   auto document=saves::make_civis_import_document(host->session().source().source,runtime::identify_file(save).sha256);
   if(scalar_changed){
    const auto features=content::saved_terrain_features(host->database(),host->types());
    auto pair=runtime::make_bnw_map_unit_scalar_checkpoint_pair(state.world,host->session().source().snapshot,
     host->identity(),features.resolve,&host->database(),&host->types(),&host->defines());
    document.sections.push_back(std::move(pair.map));document.sections.push_back(std::move(pair.units));
   }
   require_existing_standalone_fixture(document,state.standalone_services);
   const auto serial=std::chrono::steady_clock::now().time_since_epoch().count();const auto path=std::filesystem::temp_directory_path()/("civis-mission-scope-"+std::to_string(serial)+".CivisSave");
   struct OwnedFixtureFile {std::filesystem::path path;bool created=false;~OwnedFixtureFile(){if(created){std::error_code ignored;std::filesystem::remove(path,ignored);}}}file{path};
   saves::write_civis_save(path,document);file.created=true;const auto container=saves::read_loaded_save(path);check(container.container==saves::SaveContainerKind::civis&&static_cast<bool>(container.live_bnw_units)==scalar_changed&&static_cast<bool>(container.live_bnw_map_plots)==scalar_changed&&container.source_prefix&&container.standalone_services&&!container.live_bnw_unit_mission_continuation,"Actual plain Civis fixture lost original payload/optional Flags/config or invented a journal");
   auto restarted=ui::bootstrap_selected_bnw_preview(path,root,0);check(restarted.preview.host&&!restarted.preview.blocker,"Actual plain Civis source bootstrap failed");ui::SelectedBnwReadOnlyPreview reopened(std::move(restarted.preview));auto live=runtime::BnwNativeUnitAllocatorCore::from_selected_preview(reopened,0);
   reopened.with_owned_host_camera_input([&](auto reopened_host,const auto*){
    const auto raw=live->read_native_existing_mission_queue(id);const auto actual_runtime=live->read_native_existing_mission_runtime(id);check(raw&&actual_runtime&&raw->unit_flags==expected&&actual_runtime->mission_timer==0&&actual_runtime->combat_city_owner==-1&&actual_runtime->combat_city_id==-1&&actual_runtime->visual_locations==0,"Plain Civis partial original Read/reset or actual Flags carrier is not coherent");
    const auto&restored_state=reopened_host->session().state();const auto plain_before=runtime::bnw_unit_order_world_revision(restored_state,reopened_host->identity(),reopened_host->database());
    refuses([&]{(void)live->checkpoint_native_existing_continue_mission(id);});
    refuses_missing_original_common([&]{(void)live->read_native_existing_retained_path_cache(id);});
    refuses_missing_original_common([&]{(void)live->restore_native_existing_continue_mission(flag_checkpoint->section());});
    check(live->read_native_existing_mission_queue(id)->unit_flags==expected&&runtime::bnw_unit_order_world_revision(restored_state,reopened_host->identity(),reopened_host->database())==plain_before&&restored_state.random.map.state()==map_rng&&restored_state.random.gameplay.state()==game_rng,"Rejected plain Civis mission restore changed Flags/world/RNG before actual Common admission");
    refuses([&]{(void)live->checkpoint_native_existing_continue_mission(id);});
   });
   std::cout<<"PASS actual plain Civis partial Read/Flags carrier and exact missing Common owner refusal; pending phase2 Civis command join UNSUPPORTED\n";
  }else std::cout<<"UNEXECUTED actual Civis creation fixture: original C5 input required\n";


  if(before_mission->queue.empty()&&activity==0){
   const auto exit=owner->advance_native_existing_continue_mission_empty_head_exit_flags_prefix(id);const auto exit_value=static_cast<std::int32_t>(expected&~1);
   check(exit.phase==runtime::BnwUnitMissionContinuationPhase::empty_head_awake_exit_flags_stored_before_combat_warned&&exit.flags==exit_value&&exit.archive_changes==phase.archive_changes+1&&exit.archive_dirty&&scalar(unit,"m_iFlags")==exit_value,"Actual NULL-head equal-AWAKE exit failed native second Flags assignment/dirty history");
   check(owner->read_native_existing_mission_queue(id)->queue.empty()&&scalar(unit,"m_eActivityType")==activity&&scalar(unit,"m_iMoves")==moves&&owner->read_native_existing_retained_path_cache(id)==after_path&&state.random.map.state()==map_rng&&state.random.gameplay.state()==game_rng,"Equal-AWAKE exit fabricated activity/fortify/queue/move/path/RNG effects");
   if(exit_value==flags)check(exit.archive_changes==2&&runtime::bnw_unit_order_world_revision(state,host->identity(),host->database())==before,"Net-equal Flags lost two real dirty assignments or changed world");
   const auto exit_checkpoint=owner->checkpoint_native_existing_continue_mission(id);refuses([&]{(void)owner->advance_native_existing_continue_mission_empty_head_exit_flags_prefix(id);});check(owner->checkpoint_native_existing_continue_mission(id)->section().bytes==exit_checkpoint->section().bytes,"Repeated exit replay changed actual journal");
   auto restart=ui::bootstrap_selected_bnw_preview(save,root,0,true);check(restart.preview.host&&!restart.preview.blocker,"Actual phase3 restart unavailable");ui::SelectedBnwReadOnlyPreview fresh(std::move(restart.preview));require_original_common_fixture(fresh);const auto resumed=runtime::BnwNativeUnitAllocatorCore::from_selected_preview(fresh,0);
   fresh.with_owned_host_camera_input([&](auto restarted_host,const auto*){
    const auto recovered=resumed->restore_native_existing_continue_mission(exit_checkpoint->section());check(recovered.phase==exit.phase&&recovered.flags==exit.flags&&recovered.archive_changes==exit.archive_changes&&recovered.archive_dirty&&resumed->checkpoint_native_existing_continue_mission(id)->section().bytes==exit_checkpoint->section().bytes,"Phase3 OriginalRead restore lost authentic NULL/AWAKE exit/Flags dirty lineage");
    refuses([&]{(void)resumed->advance_native_existing_continue_mission_empty_head_exit_flags_prefix(id);});refuses([&]{(void)resumed->advance_native_existing_continue_mission_flags_prefix(id);});const auto&fresh_state=restarted_host->session().state();check(fresh_state.random.map.state()==map_rng&&fresh_state.random.gameplay.state()==game_rng,"Phase3 restore consumed RNG");
    if(restarted_host->session().source().source.container==saves::SaveContainerKind::civ5){
     const auto returned=resumed->advance_native_existing_continue_mission_empty_head_native_return(id);check(returned.owner==0&&returned.full_id==id&&returned.flags==exit.flags&&returned.archive_changes==exit.archive_changes&&returned.archive_dirty&&returned.path_clear_operation==after_path->clear_operations&&returned.game_store_ordinal==1,"OriginalRead restored phase3 did not execute one genuine Game false setter/Destroy native empty return");
     refuses([&]{(void)resumed->advance_native_existing_continue_mission_empty_head_native_return(id);});const auto complete=resumed->checkpoint_native_existing_continue_mission(id);runtime::validate_bnw_unit_mission_continuation_checkpoint_shape(complete->section());check(complete->section().bytes==resumed->checkpoint_native_existing_continue_mission(id)->section().bytes&&complete->section().bytes!=exit_checkpoint->section().bytes,"Restored phase3 native return did not earn stable distinct phase4 history");
    }else{
     refuses_runtime_boundary([&]{(void)resumed->advance_native_existing_continue_mission_empty_head_native_return(id);},"Game SetCombatWarned actual original Read member/once capacity unavailable");refuses([&]{(void)resumed->advance_native_existing_continue_mission_empty_head_native_return(id);});refuses([&]{(void)resumed->checkpoint_native_existing_continue_mission(id);});
    }

   });

   if(host->session().source().source.container==saves::SaveContainerKind::civ5){
    const auto returned=owner->advance_native_existing_continue_mission_empty_head_native_return(id);check(returned.owner==0&&returned.full_id==id&&returned.flags==exit.flags&&returned.archive_changes==exit.archive_changes&&returned.archive_dirty&&returned.path_clear_operation==after_path->clear_operations&&returned.game_store_ordinal==1,"Actual NULL/equal-AWAKE branch did not finish canonical Game false/ref0 once");
    refuses([&]{(void)owner->advance_native_existing_continue_mission_empty_head_native_return(id);});const auto complete=owner->checkpoint_native_existing_continue_mission(id);runtime::validate_bnw_unit_mission_continuation_checkpoint_shape(complete->section());check(complete->section().bytes==owner->checkpoint_native_existing_continue_mission(id)->section().bytes&&complete->section().bytes!=exit_checkpoint->section().bytes,"Actual completed Game native return did not preserve stable phase4 journal");check(scalar(unit,"m_iFlags")==exit.flags&&scalar(unit,"m_eActivityType")==activity&&scalar(unit,"m_iMoves")==moves&&owner->read_native_existing_mission_queue(id)->queue.empty()&&owner->read_native_existing_retained_path_cache(id)==after_path&&state.random.map.state()==map_rng&&state.random.gameplay.state()==game_rng,"Actual empty native return or repeated attempt changed unrelated world/path/RNG");
    const auto restore_complete=[&](const std::filesystem::path&input,bool journal_in_file){
     std::shared_ptr<ui::SelectedBnwPreviewHost> retained_host;std::weak_ptr<const runtime::BnwNativeUnitAllocatorCore> installed_lineage;std::optional<saves::CivisSaveDocument> next_document;
     {
      auto result=journal_in_file?ui::bootstrap_selected_bnw_preview(input,root,0):ui::bootstrap_selected_bnw_preview(input,root,0,true);if(result.preview.blocker)throw std::runtime_error(std::string("Actual phase4 bootstrap refused: ")+result.preview.blocker->reason);check(static_cast<bool>(result.preview.host),"Actual phase4 bootstrap returned no Host");
      retained_host=result.preview.host;const auto issued=result.preview.restored_mission_core;check(static_cast<bool>(issued)==journal_in_file,"Normal bootstrap failed to install actual section43 before Host publication or invented a completion");installed_lineage=issued;
      ui::SelectedBnwReadOnlyPreview restored(std::move(result.preview));if(!journal_in_file)require_original_common_fixture(restored);const auto core=runtime::BnwNativeUnitAllocatorCore::from_selected_preview(restored,0);
      if(journal_in_file)check(core.get()==issued.get()&&!core.owner_before(issued)&&!issued.owner_before(core),"Public selected factory minted a sibling Core instead of the privately restored canonical owner");
      restored.with_owned_host_camera_input([&](auto actual_host,const auto*){
       const auto&actual_state=actual_host->session().state();const auto actual_map=actual_state.random.map.state(),actual_game=actual_state.random.gameplay.state();const auto original_path=core->read_native_existing_retained_path_cache(id);
       if(journal_in_file){
        check(!actual_host->session().has_bnw_unit_mission_continuation_quarantine()&&original_path==after_path&&core->read_native_existing_mission_queue(id)->unit_flags==exit.flags,"Published Host omitted genuine restored Flags/path or retained unresolved section43 quarantine");
        check(core->checkpoint_native_existing_continue_mission(id)->section().bytes==complete->section().bytes,"Ordinary bootstrap did not earn exact completed Game journal before publication");
        const auto same=runtime::BnwNativeUnitAllocatorCore::from_selected_preview(restored,0);check(same.get()==core.get()&&!same.owner_before(core)&&!core.owner_before(same),"Repeated selected factory lost the SAME completed Core lineage");
       }else{
        check(original_path&&*original_path==runtime::BnwUnitRetainedPath{},"Phase4 fresh C5 source invented retained path history");refuses([&]{(void)core->checkpoint_native_existing_continue_mission(id);});
        const auto recovered=core->restore_native_existing_continue_mission(complete->section());check(recovered.phase==runtime::BnwUnitMissionContinuationPhase::empty_head_awake_native_return_completed&&recovered.flags==exit.flags&&recovered.archive_changes==exit.archive_changes&&recovered.archive_dirty&&core->read_native_existing_retained_path_cache(id)==after_path,"Phase4 C5 actual Game installer lost genuine Flags/path/history");
       }
       const auto roundtrip=core->checkpoint_native_existing_continue_mission(id);check(roundtrip->section().bytes==complete->section().bytes&&core->checkpoint_native_existing_continue_mission(id)->section().bytes==complete->section().bytes,"Phase4 actual restored-history Game receipt failed exact canonical roundtrip or changed ordinal");
       if(journal_in_file){
        // The next save comes from THIS normal restored Host/source and the
        // genuine current Core journal, not from the original C5 fixture.
        next_document.emplace(saves::make_civis_import_document(actual_host->session().source().source,runtime::identify_file(input).sha256));
        unsigned journal_count=0;for(auto&section:next_document->sections)if(section.id==runtime::bnw_unit_mission_continuation_section_id){check(section.flags==0&&section.bytes==roundtrip->section().bytes,"Actual first restore source lost coherent carried completion journal");section=roundtrip->section();++journal_count;}
        check(journal_count==1,"Actual completed Civis source did not carry exactly one section43 into the second document");
        check(core->checkpoint_native_existing_continue_mission(id)->section().bytes==roundtrip->section().bytes,"Preparing the genuine second save changed live Game history");
       }
       refuses([&]{(void)core->restore_native_existing_continue_mission(complete->section());});refuses([&]{(void)core->advance_native_existing_continue_mission_empty_head_native_return(id);});refuses([&]{(void)core->advance_native_existing_continue_mission_empty_head_exit_flags_prefix(id);});refuses([&]{(void)core->advance_native_existing_continue_mission_flags_prefix(id);});refuses([&]{(void)core->advance_native_existing_continue_mission_cache_prefix(id);});
       check(core->checkpoint_native_existing_continue_mission(id)->section().bytes==complete->section().bytes&&core->read_native_existing_retained_path_cache(id)==after_path&&actual_state.random.map.state()==actual_map&&actual_state.random.gameplay.state()==actual_game&&actual_map==map_rng&&actual_game==game_rng,"Phase4 rejection/repeated capture replayed Game/history/path/RNG");
      });
     }
     if(journal_in_file){
      check(installed_lineage.expired()&&retained_host&&retained_host->session().has_bnw_unit_mission_continuation_quarantine(),"Host/Session illegally retained a strong Core cycle or lost quarantine after the external holder retired");
      refuses_runtime_boundary([&]{(void)retained_host->preview_input();},"Mission Host requires the live external restored Core holder and earned join");
     }
     return next_document;
    };
    std::cerr<<"Native mission reached: original completed restore\n";
    (void)restore_complete(save,false);
    // The actual file includes the authentic completed journal and only a net
    // scalar delta. Normal bootstrap performs the private SAME Core/Game join;
    // its public consumer observes completion and never runs restore twice.
    auto completed_document=saves::make_civis_import_document(host->session().source().source,runtime::identify_file(save).sha256);const bool completion_scalar_changed=exit.flags!=flags;
    if(completion_scalar_changed){
     const auto features=content::saved_terrain_features(host->database(),host->types());
     auto pair=runtime::make_bnw_map_unit_scalar_checkpoint_pair(state.world,host->session().source().snapshot,
      host->identity(),features.resolve,&host->database(),&host->types(),&host->defines());
     completed_document.sections.push_back(std::move(pair.map));completed_document.sections.push_back(std::move(pair.units));
    }
    completed_document.sections.push_back(complete->section());
    require_existing_standalone_fixture(completed_document,state.standalone_services);const auto complete_serial=std::chrono::steady_clock::now().time_since_epoch().count();const auto complete_path=std::filesystem::temp_directory_path()/("civis-completed-mission-"+std::to_string(complete_serial)+".CivisSave");
    struct CompletedFixtureFile{std::filesystem::path path;bool created=false;~CompletedFixtureFile(){if(created){std::error_code ignored;std::filesystem::remove(path,ignored);}}}completed_file{complete_path};saves::write_civis_save(complete_path,completed_document);completed_file.created=true;const auto completed_loaded=saves::read_loaded_save(complete_path);
    check(completed_loaded.container==saves::SaveContainerKind::civis&&static_cast<bool>(completed_loaded.live_bnw_units)==completion_scalar_changed&&static_cast<bool>(completed_loaded.live_bnw_map_plots)==completion_scalar_changed&&completed_loaded.source_prefix&&completed_loaded.standalone_services&&completed_loaded.live_bnw_unit_mission_continuation&&*completed_loaded.live_bnw_unit_mission_continuation==complete->section().bytes,"Actual completed Civis file lost original provenance, net-only Flags carrier or genuine section43");std::cerr<<"Native mission reached: first ordinary completed Civis restore\n";const auto next_document=restore_complete(complete_path,true);check(next_document.has_value(),"First normal completed restore did not produce its own authentic next save document");
    const auto second_path=std::filesystem::temp_directory_path()/("civis-completed-mission-cycle2-"+std::to_string(complete_serial)+".CivisSave");CompletedFixtureFile second_file{second_path};saves::write_civis_save(second_path,*next_document);second_file.created=true;const auto second_loaded=saves::read_loaded_save(second_path);
    check(second_loaded.container==saves::SaveContainerKind::civis&&second_loaded.source_sha256==completed_loaded.source_sha256&&second_loaded.bytes==completed_loaded.bytes&&second_loaded.source_prefix==completed_loaded.source_prefix&&static_cast<bool>(second_loaded.live_bnw_units)==completion_scalar_changed&&second_loaded.live_bnw_units==completed_loaded.live_bnw_units&&second_loaded.live_bnw_unit_mission_continuation&&*second_loaded.live_bnw_unit_mission_continuation==complete->section().bytes,"Second actual save changed original payload identity, net Flags carrier or genuine Game journal");
    std::cerr<<"Native mission reached: second ordinary completed Civis restore\n";const auto third_document=restore_complete(second_path,true);check(third_document.has_value(),"Second normal completed restore failed authentic current re-save capture");

    // An independent third ordinary load exercises real gameplay cache
    // evolution; it does not alter either successful two-cycle save fixture.
    {
     auto continued_load=ui::bootstrap_selected_bnw_preview(complete_path,root,0);
     check(continued_load.preview.host&&!continued_load.preview.blocker&&continued_load.preview.restored_mission_core,
           "Actual ongoing restored-owner natural bootstrap unavailable");
     const auto issued_core=continued_load.preview.restored_mission_core;
     ui::SelectedBnwReadOnlyPreview continued(std::move(continued_load.preview));
     const auto ongoing=runtime::BnwNativeUnitAllocatorCore::from_selected_preview(continued,0);
     check(ongoing.get()==issued_core.get()&&!ongoing.owner_before(issued_core)&&!issued_core.owner_before(ongoing),
           "Ongoing natural query minted a different privately restored Core");
     continued.with_owned_host_camera_input([&](auto actual_host,const auto* actual_input){
      const auto&current=actual_host->session().state();
      const auto path_before=ongoing->read_native_existing_retained_path_cache(id);
      check(path_before&&path_before->nodes.empty()&&path_before->generation_operations<UINT64_MAX,
            "Ongoing natural restored cache is not actual empty earned path");
      check(!ongoing->read_native_existing_path_first_plot(id),"Restored empty path fabricated first plot");
      const auto partial=ongoing->read_native_existing_is_doing_partial_move(id);
      check(partial&&!*partial,"Restored real NULL mission head fabricated partial movement");
      const auto origin=runtime::bnw_unit_order_plot(current,{0,id});
      const auto before_world=runtime::bnw_unit_order_world_revision(current,actual_host->identity(),actual_host->database());
      const auto before_map=current.random.map.state(),before_game=current.random.gameplay.state();
      const auto before_ui=current.ui_source_revision;
      const auto before_mission=ongoing->read_native_existing_mission_queue(id);
      std::cerr<<"Native mission reached: third ongoing GeneratePath\n";
      const auto produced=ongoing->advance_native_existing_generate_path_cache(id,origin,false);
      check(produced&&produced->found&&produced->nodes.size()==1&&produced->nodes.front().plot==origin,
            "Actual restored native GeneratePath to current coordinate did not execute real search");
      const auto stored=ongoing->read_native_existing_retained_path_cache(id);
      check(stored&&stored->nodes.size()==1&&stored->destination==origin&&
            stored->generation_operations==path_before->generation_operations+1&&
            stored->clear_operations==path_before->clear_operations&&
            stored->path_to_reject_clear_operations==path_before->path_to_reject_clear_operations,
            "Actual restored GeneratePath did not publish SAME cache exactly once");
      const auto reacquired=runtime::BnwNativeUnitAllocatorCore::from_selected_preview(continued,0);
      check(reacquired.get()==ongoing.get()&&!reacquired.owner_before(ongoing)&&!ongoing.owner_before(reacquired)&&
            !actual_host->session().has_bnw_unit_mission_continuation_quarantine(),
            "Legitimate current native cache update retired SAME restored Core/Host");
      const auto first=ongoing->read_native_existing_path_first_plot(id);
      check(first&&*first==origin,"Actual retained one-node path first plot unavailable after real store");
      // The owning preview already obtained this real input from the mutable
      // canonical Host before lending its const facade to this callback.
      check(actual_input&&&actual_input->session==&actual_host->session()&&actual_input->active_player_id==0,
            "Ongoing physical callback lacks SAME actual Host/session input");
      continued.with_owned_host_camera_input([&](auto same_host,const auto* same_input){
       check(same_host.get()==actual_host.get()&&!same_host.owner_before(actual_host)&&!actual_host.owner_before(same_host),
             "Ongoing physical input changed canonical restored Host");
       check(same_input&&&same_input->session==&same_host->session()&&same_input->active_player_id==0&&
             same_input->scene_identity&&same_input->scene_identity->payload_sha256==same_host->identity().payload_sha256&&
             same_input->scene_identity->rules_sha256==same_host->identity().rules_sha256,
             "Fresh post-GeneratePath physical input lost SAME restored session/scene identity");
      });
      refuses_native_boundary([&]{(void)ongoing->checkpoint_native_existing_continue_mission(id);},
                              "Mission checkpoint actual current node/path/world changed");
      refuses_native_boundary([&]{(void)actual_host->export_import_clock_checkpoint();},
                              "Restored mission final physical path/world seal changed");
      const auto after_mission=ongoing->read_native_existing_mission_queue(id);
      check(before_mission&&after_mission&&before_mission->queue==after_mission->queue&&
            before_mission->unit_flags==after_mission->unit_flags&&before_mission->activity==after_mission->activity&&
            ongoing->read_native_existing_retained_path_cache(id)==stored&&current.ui_source_revision==before_ui&&
            current.random.map.state()==before_map&&current.random.gameplay.state()==before_game&&
            runtime::bnw_unit_order_world_revision(current,actual_host->identity(),actual_host->database())==before_world,
            "Native cache-only progression or stale-save refusal changed units/world/RNG");
     });
    }
    std::cout<<"PASS RESTORED_MISSION_OWNER_CONTINUES_AFTER_NATIVE_PATH_CACHE_STORE exact SAME Core/Host input, actual GetPathFirstPlot/NULL-head queries, real GeneratePath +1, stale checkpoint/export refused\n";

    std::cout<<"PASS actual completed NULL/AWAKE Game journal C5 restore and real Civis section43 unpublished Host join/SAME Core/Destroy roundtrip/no replay/holder retirement. Full mission/turn remain UNSUPPORTED\n";
    std::cout<<"PASS TWO_SEQUENTIAL_NORMAL_CIVIS_MISSION_SAVE_RESTORE_CYCLES actual first restored Host source -> second file -> normal bootstrap exact journal/path/Flags/RNG and no replay\n";
   }else{
    refuses_runtime_boundary([&]{(void)owner->advance_native_existing_continue_mission_empty_head_native_return(id);},"Game SetCombatWarned actual original Read member/once capacity unavailable");refuses([&]{(void)owner->advance_native_existing_continue_mission_empty_head_native_return(id);});refuses([&]{(void)owner->checkpoint_native_existing_continue_mission(id);});check(state.random.map.state()==map_rng&&state.random.gameplay.state()==game_rng&&scalar(unit,"m_iFlags")==exit.flags,"Unknown Civis Game runtime invented a store or consumed RNG");
    std::cout<<"UNEXECUTED positive Game runtime/native return: original C5 Read qualifier required; unknown Civis runtime refuses retry/stale save\n";
   }

  }else{
   const auto bytes=owner->checkpoint_native_existing_continue_mission(id)->section().bytes;refuses([&]{(void)owner->advance_native_existing_continue_mission_empty_head_exit_flags_prefix(id);});check(owner->checkpoint_native_existing_continue_mission(id)->section().bytes==bytes&&scalar(unit,"m_iFlags")==expected,"Unsupported nonempty/changed-activity head changed earned phase before mandatory boundary");
   std::cout<<"UNEXECUTED positive NULL-head equal-AWAKE fixture: no genuine imported candidate; mandatory branch refusal PASS\n";
  }
  std::cout<<"PASS actual ClearPathCacheРІвЂ вЂ™Flags evaluating once; finite section43 original restart/roundtrip/replay refusal. Mission body/SetCombatWarned still mandatory\n";

 });
}
int main(int argc,char**argv){try{if(argc!=3)throw std::runtime_error("Use actual BNW save installation");natural(argv[1],argv[2]);natural_selected_init(argv[1],argv[2]);natural_continue_mission_cache(argv[1],argv[2]);return 0;}
catch(const std::exception&e){std::cerr<<"FAIL native unit allocator: "<<e.what()<<'\n';return 1;}}
