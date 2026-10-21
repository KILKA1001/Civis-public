#include "civ5/runtime/bnw_unit_move_pending.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/runtime/bnw_map_occupied_preflight.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include "civ5/runtime/bnw_unit_movement_query.hpp"
#include "civ5/runtime/bnw_plot_movement_cost.hpp"
#include "civ5/runtime/bnw_plot_reveal_read.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/bnw_client_selection_control.hpp"
#include <climits>
#include <functional>
#include <thread>
#include <type_traits>
#include <map>
#include <vector>
#include <string_view>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool ok,const char*why){if(!ok)throw BnwUnitMoveUnavailable(why);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Move operand exceeds native signed int");return static_cast<std::int32_t>(n);}
template<class Core>auto& field(Core& core,std::string_view key,saves::UnitCoreEncoding encoding){
 using Field=std::conditional_t<std::is_const_v<Core>,const saves::UnitCoreField,saves::UnitCoreField>;
 Field* found=nullptr;
 for(auto& f:core.fields)if(f.name&&std::string_view(f.name)==key){need(!found,"Duplicate movement core field");found=&f;}
 need(found&&found->encoding==encoding&&found->values.size()==1,"Movement scalar source schema unavailable");return *found;
}
template<class Core>std::int32_t scalar(Core& core,std::string_view key){return native(field(core,key,saves::UnitCoreEncoding::i32).values[0]);}
saves::SavedUnitObservation& unit(LiveSessionState& state,BnwUnitOrderKey key){
 need(key.first>=0&&key.first<64&&key.second>=0&&state.world.all_players&&state.world.all_players->layout==bnw,"Movement native player/unit roster unavailable");
 auto& p=state.world.all_players->players.at(static_cast<std::size_t>(key.first));
 need(p.saved_player_id==key.first&&p.index==static_cast<std::uint64_t>(key.first),"Movement player identity differs");
 saves::SavedUnitObservation* result=nullptr;
 for(auto& u:p.units.units)if(scalar(u.core,"m_iID")==key.second){need(!result,"Movement duplicate unit ID");result=&u;}
 need(result&&result->core.layout==bnw&&scalar(result->core,"m_eOwner")==key.first,"Movement full unit identity unavailable");
 const auto& owned=state.maintenance.players.at(key.first).units.at(key.second);
 need(owned.source_core==std::optional{std::pair{result->core.start_offset,result->core.end_offset}},"Movement raw/owned unit allocation provenance differs");
 return *result;
}
bool water(const LiveSessionState& state,std::uint64_t slot){
 const auto& world=state.world;need(world.map_plots&&world.map_projection,"Movement actual map unavailable");
 const auto& grid=world.map_header.grid;need(grid.width&&grid.height&&world.map_plots->plots.size()==grid.size()&&world.map_projection->cells.size()==grid.size(),"Movement actual map shape differs");
 const auto xy=grid.coordinates(slot);const auto& raw=world.map_plots->plots.at(slot);const auto& cell=world.map_projection->cells.at(slot);
 need(raw.index==slot&&raw.x==xy[0]&&raw.y==xy[1]&&cell.index==slot&&cell.x==xy[0]&&cell.y==xy[1],"Movement canonical plot identity differs");
 const saves::NamedSaveInteger* found=nullptr;
 for(const auto& f:raw.fields)if(f.name&&std::string_view(f.name)=="plot_type"){need(!found,"Duplicate movement plot type");found=&f;}
 need(found&&found->value>=0&&found->value<4&&cell.plot_type==found->value,"Movement raw/owned plot type differs");
 return found->value==3; // CvEnums.h PlotTypes: Mountain0/Hills1/Land2/Ocean3.
}
// Native own-city collision branch, not generic combat/capture execution.
// One semantic traversal records finite operands and later compares them
// without allocating, after all callbackful/source preparation is complete.
std::int64_t collision_plot_field(const saves::PlotObservation& p,std::string_view key){
 const saves::NamedSaveInteger* f=nullptr;for(const auto& e:p.fields)if(e.name&&std::string_view(e.name)==key){need(!f,"Duplicate initial collision plot field");f=&e;}
 need(f,"Initial collision plot field absent");return f->value;
}
const saves::PlotArrayObservation& collision_plot_units(const saves::PlotObservation& p){
 const saves::PlotArrayObservation* f=nullptr;for(const auto& e:p.arrays)if(e.name&&std::string_view(e.name)=="units"){need(!f,"Duplicate initial collision plot roster");f=&e;}
 need(f&&f->columns==2&&f->values.size()%2==0,"Initial collision raw plot roster absent");return *f;
}
const saves::SavedUnitObservation* collision_unit(const LiveSessionState&s,saves::SavedPlotUnitReference h,
 const saves::SavedUnitObservation*found){
 const auto&player=s.world.all_players->players.at(static_cast<std::size_t>(h.owner));
 const auto&owned=s.maintenance.players.at(native(h.owner));
 if(found){const auto id=scalar(found->core,"m_iID");const auto mirror=owned.units.find(id);
  need(found->core.layout==bnw&&found->tail.layout==bnw&&scalar(found->core,"m_eOwner")==h.owner&&
   mirror!=owned.units.end()&&mirror->second.source_core==std::optional{std::pair{found->core.start_offset,found->core.end_offset}}&&
   player.saved_player_id==h.owner,"Initial collision actual native GetAt raw/owned source differs");
 }else need(owned.units.find(h.id)==owned.units.end(),"Initial collision native NULL has mismatched owned raw identity");
 return found;
}
template<class Emit>bool collision_war(const LiveSessionState&s,std::int32_t ours,std::int32_t other,Emit& emit){
 need(ours>=-1&&ours<64&&other>=-1&&other<64,"Initial collision native team domain invalid");
 emit(ours);emit(other);
 if(ours==-1||other==-1)return false; // CvGameCoreUtils::atWar NO_TEAM short-circuit.
 need(s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->teams.size()==64,
  "Initial collision actual team roster unavailable");
 const auto& team=s.world.teams->teams.at(static_cast<std::size_t>(ours));need(team.index==static_cast<std::uint64_t>(ours),"Initial collision team identity changed");
 const saves::PlotArrayObservation* flags=nullptr;
 for(const auto&a:team.arrays)if(a.name&&std::string_view(a.name)=="at_war"){need(!flags,"Initial collision duplicate war relation");flags=&a;}
 need(flags&&flags->columns==1&&flags->values.size()==64,"Initial collision current native war array absent");
 emit(reinterpret_cast<std::uintptr_t>(&team));
 for(auto value:flags->values){need(value==0||value==1,"Initial collision native war flag is not boolean");emit(value);}
 return flags->values.at(static_cast<std::size_t>(other))!=0;
}
template<class Emit>std::int32_t collision_team(const LiveSessionState&s,std::int32_t id,Emit& emit){
 const auto&w=s.world;
 need(id>=0&&id<64&&w.pregame&&w.pregame->layout==bnw&&w.all_players&&w.all_players->layout==bnw&&
  w.all_players->context.player_count==64&&w.all_players->context.strategy.civ_players==63&&w.all_players->players.size()==64&&
  w.game_state.roster&&w.game_state.roster->dimensions.player_slots==64&&w.game_state.roster->dimensions.team_slots==64,
  "Initial collision genuine native player/team roster changed");
 const auto&player=w.all_players->players.at(static_cast<std::size_t>(id));
 const saves::PlayerCoreField* identity=nullptr;
 for(const auto&f:player.core.fields)if(f.name&&std::string_view(f.name)=="m_eID"){need(!identity,"Initial collision duplicate player identity");identity=&f;}
 need(player.index==static_cast<std::uint64_t>(id)&&player.saved_player_id==id&&player.core.layout==bnw&&identity&&
  identity->encoding==saves::PlayerCoreEncoding::i32&&identity->values.size()==1&&identity->values[0]==id,
  "Initial collision actual CvPlayer ID changed");
 const std::vector<std::int64_t>* teams=nullptr;
 // No temporary std::string key, allocation, or source callback in final visitor.
 for(const auto&[key,value]:w.pregame->archive.integer_arrays)if(std::string_view(key)=="team_types")teams=&value;
 need(teams&&teams->size()==64,"Initial collision actual team types absent");
 const auto team=native(teams->at(static_cast<std::size_t>(id)));const auto owned=s.setup.players.find(id);
 need(team>=-1&&team<64&&owned!=s.setup.players.end()&&owned->second.team==team,
  "Initial collision current raw/owned team differs");
 emit(reinterpret_cast<std::uintptr_t>(&player));emit(player.index);emit(player.saved_player_id);emit(identity->values[0]);emit(team);
 return team;
}
template<class Emit,class Lookup>std::uint64_t initial_collision_semantics(const LiveSessionState&s,
 const saves::SavedCityObservation* expected_city,
 std::int32_t owner,std::int32_t city,std::int32_t x,std::int32_t y,std::int32_t combat,Emit emit,Lookup lookup){
 const auto& w=s.world;const auto& grid=w.map_header.grid;
 need(owner>=0&&owner<64&&s.setup.profile==content::TechnologyProfile::bnw&&w.map_header.layout==bnw&&w.map_plots&&w.map_projection&&
  w.map_plots->plots.size()==grid.size()&&w.map_projection->cells.size()==grid.size()&&x>=0&&y>=0&&
  static_cast<std::uint64_t>(x)<grid.width&&static_cast<std::uint64_t>(y)<grid.height&&combat>=0,
  "Initial collision genuine BNW destination/map absent");
 const auto index=grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));
 const auto& raw=w.map_plots->plots.at(index);const auto& cell=w.map_projection->cells.at(index);
 need(raw.index==index&&cell.index==index&&raw.x==static_cast<std::uint64_t>(x)&&raw.y==static_cast<std::uint64_t>(y)&&
  cell.x==raw.x&&cell.y==raw.y&&cell.plot_city&&cell.plot_city->owner==owner&&cell.plot_city->id==city&&
  collision_plot_field(raw,"plot_city_owner")==owner&&collision_plot_field(raw,"plot_city_id")==city,
  "Initial collision requires genuine same own-city TRAIN destination");
 // Genuine allocating city registry lookup happened during preparation. Here
 // only compare a current roster address with its private issued address;
 // never dereference an old address after a roster allocation/replacement.
 need(w.all_players&&w.all_players->players.size()==64,"Initial collision native city roster absent");
 const auto& city_roster=w.all_players->players.at(static_cast<std::size_t>(owner)).cities;
 const saves::SavedCityObservation* source_city=nullptr;
 for(const auto& entry:city_roster.cities)if(&entry==expected_city)source_city=&entry;
 need(source_city&&source_city->core.layout==bnw,"Initial collision genuine city storage changed");
 const auto city_scalar=[&](std::string_view key){
  const auto* f=static_cast<const saves::CityCoreField*>(nullptr);
  for(const auto&e:source_city->core.fields)if(e.name&&std::string_view(e.name)==key){need(!f,"Initial collision duplicate city operand");f=&e;}
  need(f&&f->encoding==saves::CityCoreEncoding::i32&&f->values.size()==1,"Initial collision native city operand absent");return native(f->values[0]);
 };
 const auto& owned_city=s.cities.players.at(owner).cities.at(city);
 need(city_scalar("m_iID")==city&&city_scalar("m_eOwner")==owner&&city_scalar("m_iX")==x&&city_scalar("m_iY")==y&&
  owned_city.owner==owner&&owned_city.id==city&&owned_city.x==x&&owned_city.y==y&&cell.units&&
  owned_city.source_core==std::optional{std::pair{source_city->core.start_offset,source_city->core.end_offset}},
  "Initial collision owned source city changed");
 emit(reinterpret_cast<std::uintptr_t>(&raw));emit(reinterpret_cast<std::uintptr_t>(&cell));emit(reinterpret_cast<std::uintptr_t>(source_city));
 emit(source_city->core.start_offset);emit(source_city->core.end_offset);
 emit(index);emit(grid.width);emit(grid.height);emit(owner);emit(city);emit(x);emit(y);emit(combat);
 const auto& raw_handles=collision_plot_units(raw);const auto& handles=*cell.units;
 need(handles.size()<=INT32_MAX&&raw_handles.values.size()==handles.size()*2,"Initial collision raw/projected roster extent differs");
 emit(handles.size());for(std::size_t i=0;i<handles.size();++i){
  need(raw_handles.values[2*i]==handles[i].owner&&raw_handles.values[2*i+1]==handles[i].id,
   "Initial collision ordered raw/projected handles differ");emit(handles[i].owner);emit(handles[i].id);
 }
 // IsCombatUnit is exactly m_iBaseCombat>0. Civilians never enter this loop.
 if(combat==0)return 0;
 const auto ours=collision_team(s,owner,emit);emit(ours);
 // Preserve all raw team-type operands consumed by the existing native getter.
 need(w.pregame&&w.pregame->layout==bnw,"Initial collision native Pregame absent");
 const std::vector<std::int64_t>* raw_teams=nullptr;
 for(const auto&[key,value]:w.pregame->archive.integer_arrays)if(std::string_view(key)=="team_types")raw_teams=&value;
 need(raw_teams&&raw_teams->size()==64,"Initial collision raw native team types absent");
 for(auto team:*raw_teams)emit(team);
 // SDK copies the complete current list before ::getUnit/null/death/war.
 // This traversal reads that same immutable-order list; final matcher seals it.
 for(const auto handle:handles){
  const auto* u=collision_unit(s,handle,lookup(handle));emit(u!=nullptr);emit(reinterpret_cast<std::uintptr_t>(u));
  if(!u)continue;
  const auto death=field(u->core,"m_bDeathDelay",saves::UnitCoreEncoding::boolean).values[0];
  need(death==0||death==1,"Initial collision delayed-death flag invalid");emit(death);if(death)continue;
  const auto ux=scalar(u->core,"m_iX"),uy=scalar(u->core,"m_iY");emit(ux);emit(uy);
  need(ux==x&&uy==y,"Initial collision linked live unit is not at the actual destination");
  const auto other=collision_team(s,native(handle.owner),emit);emit(other);
  emit(u->core.start_offset);emit(u->core.end_offset);
  // On a genuine city, isAlwaysHostile returns false BEFORE its promotion
  // count. getCombatOwner therefore keeps each real owner; no barbarian remap.
  const bool first=collision_war(s,ours,other,emit);emit(first);
  bool second=false;if(!first){second=collision_war(s,other,ours,emit);emit(second);} // Exact || short-circuit.
  need(!first&&!second,"Mandatory initial enemy coexist/capture/displace/kill branch unavailable");
 }
 return static_cast<std::uint64_t>(handles.size());
}
}
std::int32_t detail::bnw_unit_move_after_cost(std::int32_t moves,std::int32_t cost){
 // Native changeMoves(-cost) reaches negation before addition, even when the
 // eventual mathematical difference would fit. Reject UB instead of clamp.
 need(cost!=INT32_MIN,"Movement native cost negation overflows");
 return native(static_cast<std::int64_t>(moves)+(-cost));
}
struct BnwUnitMoveRegistry::Impl {
 std::thread::id thread=std::this_thread::get_id();bool retired=false;
 std::map<BnwUnitOrderKey,std::shared_ptr<BnwUnitMovePending>> pending;
 std::vector<BnwUnitMoveDimIntent> queued;std::uint64_t next_occurrence=0;
};
struct BnwUnitMovePending::Impl {
 std::thread::id thread=std::this_thread::get_id();bool retired=false,attempted=false;
 LiveSession* session=nullptr;std::weak_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<LiveSessionState> state;std::shared_ptr<const saves::SavedSession> source;
 std::shared_ptr<LiveSession::BnwGameDebugReadStorage> control;
 std::weak_ptr<BnwUnitMoveRegistry> registry;SessionCheckpointIdentity identity;
 std::uint64_t generation=0,ui_revision=0;std::function<bool()> rules_current;
 BnwUnitOrderKey key;std::int32_t x=0,y=0;bool show=false;
 std::weak_ptr<const BnwNativeUnitAllocatorCore> initial_core;
 std::weak_ptr<const BnwUnitMovePending> initial_factory_self;
 std::weak_ptr<ui::BnwClientSelectionControl> initial_input;
 std::int32_t initial_city=-1,initial_combat=0;
 const saves::SavedCityObservation* initial_source_city=nullptr;
 std::vector<std::uint64_t> initial_collision_seal;
 std::vector<BnwNativeUnitAllocatorCore::InitialFltaOwnerSeal> initial_flta_seals;
 std::string world_before,world_after;BnwUnitMovePrefix prefix;
 std::uint64_t dim_occurrence=0,dim_serial=0,dirty_input_revision=0;
 std::optional<BnwStockLuaAttemptLease> dim_loan;
};
BnwUnitMoveRegistry::BnwUnitMoveRegistry():impl_(std::make_unique<Impl>()){}
BnwUnitMoveRegistry::~BnwUnitMoveRegistry(){retire();}
void BnwUnitMoveRegistry::retire()noexcept{impl_->retired=true;for(auto& [key,p]:impl_->pending){(void)key;p->retire();}}
bool BnwUnitMoveRegistry::has_history()const noexcept{return !impl_->pending.empty();}
BnwUnitMovePending::BnwUnitMovePending(std::unique_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwUnitMovePending::~BnwUnitMovePending()=default;
void BnwUnitMovePending::retire()noexcept{impl_->retired=true;}
void BnwUnitMovePending::require_current()const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired,"Movement owner thread/retirement differs");
 const auto registry=p.registry.lock();need(registry&&!registry->impl_->retired&&registry->impl_->thread==p.thread,"Movement session registry retired");
 const auto host=p.host.lock();need(host&&&host->session()==p.session,"Movement actual Host lifetime/session differs");
 const auto& s=*p.session;
 need(s.state_==p.state&&s.source_==p.source&&s.state_generation_==p.generation&&s.unit_move_registry_==registry&&s.debug_==p.control&&!p.control->retired.load()&&s.bnw_game_debug_owner_current(),"Movement SAME world/source/Game allocation retired");
 need(p.rules_current&&p.rules_current()&&s.debug_->identity.payload_sha256==p.identity.payload_sha256&&s.debug_->identity.rules_sha256==p.identity.rules_sha256,"Movement immutable DB/source identity retired");
 need(registry->impl_->pending.at(p.key).get()==this&&p.state->ui_source_revision==p.ui_revision,"Movement exact registered attempt/world revision differs");
 if(p.prefix.initial_unit){
  const auto core=p.initial_core.lock();need(bool(core),"Initial SetXY genuine unpublished Core expired");
  core->require_initial_placement_current(this);
  if(p.prefix.popup_hold_stored){const auto input=p.initial_input.lock();need(input&&p.session->city_selection_.lock()==input,
    "Initial SetXY popup canonical input retired");input->require_current();
   need(input->popups_suppressed()==std::optional<bool>{!p.prefix.initial_popup_released}&&input->popup_hold_setter_count()==(p.prefix.initial_popup_released?p.prefix.popup_release_setter_occurrence:p.prefix.popup_setter_occurrence),
    "Initial SetXY actual popup hold overwritten");}
  return; // Never resolve an unpublished node through the raw unit roster.
 }
 const auto& expected=p.attempted?p.world_after:p.world_before;
 need(bnw_unit_order_world_revision(*p.state,p.identity,host->database())==expected,"Movement canonical unit/world prefix changed");
}
std::shared_ptr<BnwUnitMovePending> BnwUnitMovePending::create(LiveSession& session,
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host,BnwUnitOrderKey key,std::int32_t x,std::int32_t y,bool show){
 need(host&&&host->session()==&session&&session.plot_yield_refresh_thread_==std::this_thread::get_id(),"Movement genuine Host/session/thread mismatch");
 need(!session.unit_move_registry_||(!session.unit_move_registry_->impl_->retired&&session.unit_move_registry_->impl_->pending.empty()),
      "Movement unfinished native call/once quarantine already retained");
 need(session.bnw_game_debug_owner_current(),"Movement actual Game read owner absent");(void)session.bnw_game_debug_mode();
 auto p=std::make_unique<Impl>();p->session=&session;p->host=host;p->state=session.state_;p->source=session.source_;p->control=session.debug_;p->generation=session.state_generation_;p->ui_revision=p->state->ui_source_revision;p->identity=session.debug_->identity;p->key=key;p->x=x;p->y=y;p->show=show;
 p->rules_current=host->database().immutable_preview_fingerprint_currentness();
 need(p->rules_current&&p->rules_current()&&host->identity().payload_sha256==p->identity.payload_sha256&&host->identity().rules_sha256==p->identity.rules_sha256,"Movement SAME immutable Host DB differs");
 need(detail::bnw_game_active_player_read(*p->state)==key.first,"Movement command is not for the actual active Game actor");
 auto& u=unit(*p->state,key);
 need(field(u.core,"m_bDeathDelay",saves::UnitCoreEncoding::boolean).values[0]==0,"Movement delayed-death unit unavailable");
 const auto origin=bnw_unit_order_plot(*p->state,key);const auto& grid=p->state->world.map_header.grid;
 need(x>=0&&y>=0&&static_cast<std::uint64_t>(x)<grid.width&&static_cast<std::uint64_t>(y)<grid.height,"Movement destination outside current map");
 const auto target=grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));need(target!=origin,"Movement target equals source plot");
 const auto xy=grid.coordinates(origin);const auto& map=p->state->world.map_header;
 const MapGeometry geometry{grid,map.wrap_x,map.wrap_y,bnw};bool neighbor=false;
 for(std::int64_t dir=0;dir<6;++dir){const auto next=geometry.direction(static_cast<std::int64_t>(xy[0]),static_cast<std::int64_t>(xy[1]),dir);if(next&&static_cast<std::uint64_t>(*next)==target)neighbor=true;}
 need(neighbor,"Movement request is not an actual native hex neighbor");
 // CvUnit::move's canMoveOrAttackInto/isOutOfAttacks expression is a debug
 // assertion, not a Release gameplay predicate. This native-member diagnostic
 // prefix is never a mission/path dispatcher and does not certify that assertion.
 // Root must keep actual production command dispatch separate until its source
 // movement/combat admission is implemented.
 const auto quote=quote_bnw_unit_movement(*p->state,host->database(),host->types(),p->identity,key);
 need(quote.max_moves&&quote.base_moves,quote.blocker.empty()?"Movement current base/max getters unavailable":quote.blocker.c_str());
 need(!water(*p->state,target),"Movement destination-water baseMoves(DOMAIN_SEA) is an unimplemented reached getter");
 const auto team=detail::bnw_player_team_read(*p->state,native(key.first));
 // Native movementCost first returns maxMoves only for !revealed && human.
 // Revealed must not evaluate IsHuman. Its retained source-backed cost owner
 // proves nonfog predecessors, flat/regular/routes/border and named ZOC gaps.
 if(!detail::bnw_plot_revealed_read(p->state->world,target,team)){
  need(detail::bnw_player_human_read(*p->state,native(key.first),host->database()),"Movement unrevealed nonhuman cost continues to unavailable native predicates");
  p->prefix.cost=*quote.max_moves;
 }else{
  p->prefix.cost=inspect_bnw_plot_movement_cost(host,key,target);
 }
 // The synchronous read returns BEFORE CanEverEmbark/transition/moves.
 // Do not retain its strong Host in the Session-owned Pending backedge.
 need(session.state_==p->state&&session.source_==p->source&&session.state_generation_==p->generation&&
      session.debug_==p->control&&session.bnw_game_debug_owner_current()&&p->state->ui_source_revision==p->ui_revision&&
      p->rules_current&&p->rules_current(),"Movement source retired during actual cost read");
 // CvUnit::move evaluates CanEverEmbark before the water-transition chain.
 // Support only genuine CanEverEmbark false. Otherwise the source water/
 // AllowsWalkWater chain and embark visualization are mandatory before moves.
 need(quote.terms.domain!=2||scalar(u.core,"m_iEmbarkAbilityCount")<=0,"Movement actual embark transition/WalkWater owner required");
 p->prefix.before_moves=scalar(u.core,"m_iMoves");p->prefix.after_moves=detail::bnw_unit_move_after_cost(p->prefix.before_moves,p->prefix.cost);
 p->prefix.moves_stored=p->prefix.before_moves!=p->prefix.after_moves;
 p->prefix.boundary=p->prefix.moves_stored?BnwUnitMoveBoundary::dim_receiver:(show?BnwUnitMoveBoundary::visible_to_watching_human:BnwUnitMoveBoundary::set_xy_popup_owner);
 p->world_before=bnw_unit_order_world_revision(*p->state,p->identity,host->database());
 // Hash staging creates no issuer: actual source/rules/cost above supplied
 // every operand. All allocation and post-prefix seal calculation precede
 // the canonical store; this temporary never becomes a gameplay world.
 auto staging=*p->state;field(unit(staging,key).core,"m_iMoves",saves::UnitCoreEncoding::i32).values[0]=p->prefix.after_moves;
 p->world_after=bnw_unit_order_world_revision(staging,p->identity,host->database());
 auto registry=session.unit_move_registry_;
 if(!registry){registry=std::shared_ptr<BnwUnitMoveRegistry>(new BnwUnitMoveRegistry);session.unit_move_registry_=registry;}
 need(!registry->impl_->retired&&registry->impl_->pending.empty(),"Movement unfinished native call/once quarantine already retained");
 p->registry=registry;auto result=std::shared_ptr<BnwUnitMovePending>(new BnwUnitMovePending(std::move(p)));
 registry->impl_->pending.emplace(key,result);result->require_current();return result;
}
std::shared_ptr<BnwUnitMovePending> BnwUnitMovePending::create_for_initial_unit(
 const std::shared_ptr<const BnwNativeUnitAllocatorCore>& supplied){
 need(bool(supplied),"Initial SetXY genuine allocator absent");
 const auto core=supplied->retain_factory_owner();
 need(core&&core.get()==supplied.get()&&!core.owner_before(supplied)&&!supplied.owner_before(core),
  "Initial SetXY rejects supplied alias allocator control block");
 const auto args=core->initial_placement_operands();
 need(args.host&&args.session&&args.state&&&args.host->session()==args.session,"Initial SetXY source Host/session absent");
 auto& session=*args.session;
 need(session.plot_yield_refresh_thread_==std::this_thread::get_id()&&session.state_==args.state&&session.state_generation_==args.generation&&
  session.bnw_game_debug_owner_current()&&args.state->ui_source_revision==args.revision,"Initial SetXY SAME current world retired");
 need(!session.unit_move_registry_||(!session.unit_move_registry_->impl_->retired&&session.unit_move_registry_->impl_->pending.empty()),
  "Initial SetXY unfinished real movement journal already retained");
 auto p=std::make_unique<Impl>();p->session=&session;p->host=args.host;p->state=args.state;p->source=session.source_;p->control=session.debug_;
 p->generation=args.generation;p->ui_revision=args.revision;p->identity=session.debug_->identity;p->key={args.owner,args.id};p->x=args.x;p->y=args.y;
 p->initial_core=core;p->initial_city=args.city;p->initial_combat=args.base_combat;p->prefix.initial_unit=true;p->prefix.boundary=BnwUnitMoveBoundary::set_xy_popup_owner;
 p->rules_current=args.host->database().immutable_preview_fingerprint_currentness();
 need(p->rules_current&&p->rules_current()&&args.host->identity().payload_sha256==p->identity.payload_sha256&&
  args.host->identity().rules_sha256==p->identity.rules_sha256,"Initial SetXY genuine DB/import differs");
 const auto& grid=p->state->world.map_header.grid;
 need(p->x>=0&&p->y>=0&&static_cast<std::uint64_t>(p->x)<grid.width&&static_cast<std::uint64_t>(p->y)<grid.height,
  "Initial SetXY actual city destination outside current map");
 // Init is a direct setXY(...false,false,false,false,false), not a neighbor
 // movement. Cost/Dim/origin-roster getters are not called by this SDK path.
 auto registry=session.unit_move_registry_;
 if(!registry)registry=std::shared_ptr<BnwUnitMoveRegistry>(new BnwUnitMoveRegistry);
 need(!registry->impl_->retired&&registry->impl_->pending.empty(),"Initial SetXY existing attempt quarantine changed");
 p->registry=registry;auto result=std::shared_ptr<BnwUnitMovePending>(new BnwUnitMovePending(std::move(p)));
 result->impl_->initial_factory_self=result; // Actual private factory control, before callbacks.
 core->require_initial_placement_current(result.get());
 // Allocate journal entry before attaching it to Session. No unit/plot list
 // is published here. This is the existing native call owner and once ledger.
 registry->impl_->pending.emplace(result->impl_->key,result);
 session.unit_move_registry_=registry;result->require_current();return result;
}
void BnwUnitMovePending::require_initial_popup_store(const ui::BnwClientSelectionControl& input)const{
 require_current();const auto& p=*impl_;
 need(p.prefix.initial_unit&&!p.attempted&&!p.prefix.popup_hold_stored&&p.prefix.boundary==BnwUnitMoveBoundary::set_xy_popup_owner,
  "Initial SetXY popup setter wrong once/source phase");
 need(p.session->city_selection_.lock().get()==&input,"Initial SetXY popup requires actual registered UI input");
 input.require_current();
 need(input.active_player()==p.key.first&&detail::bnw_game_active_player_read(*p.state)==p.key.first,
  "Initial SetXY popup active-owner branch differs");
 const auto core=p.initial_core.lock();need(bool(core),"Initial SetXY actual Core expired before popup store");
 core->require_initial_placement_current(this);
}
void BnwUnitMovePending::record_initial_popup_store(std::uint64_t occurrence,std::uint64_t revision)const noexcept{
 impl_->prefix.popup_hold_stored=true;impl_->prefix.popup_setter_occurrence=occurrence;impl_->prefix.popup_client_revision=revision;
 impl_->attempted=true;impl_->prefix.boundary=BnwUnitMoveBoundary::initial_destination_collision_owner;
}
void BnwUnitMovePending::execute_initial_set_xy_prefix(){
 require_current();auto& p=*impl_;need(p.prefix.initial_unit&&!p.attempted,"Initial SetXY prefix already attempted/wrong path");
 // SDK CvUnit::setXY first calls SetDontShowPopups(true) for actual active
 // owner, before GetActivityType/GetGarrisonedCity/ranged setup. The genuine
 // Core proves newborn Reset predicates (no garrison/setup) before stores.
 need(detail::bnw_game_active_player_read(*p.state)==p.key.first,
  "Initial SetXY nonactive Traits placement branch is not this TRAIN source");
 const auto input=p.session->city_selection_.lock();need(bool(input),"Initial SetXY actual client popup owner absent");
 input->require_current();require_initial_popup_store(*input);p.initial_input=input;
 input->begin_initial_set_xy_popup_hold(*this);
 require_current();
 // Actual hold is now stored. Destination collision/transport/war/kill,
 // plot sight+area+roster, Lua and popup release must precede publication.
}
void BnwUnitMovePending::require_initial_visibility_source_without_callbacks()const{
 const auto&p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired,"Foreign/retired newborn visibility phase");
 need(p.session&&p.prefix.initial_unit&&p.attempted&&p.prefix.popup_hold_stored&&p.prefix.initial_coordinates_stored&&
  !p.initial_core.expired(),
  "Initial visibility requires actual recorded coordinate/popup prefix");
 const auto&s=*p.session;const auto&registry=s.unit_move_registry_;
 need(registry&&!p.registry.expired()&&!p.registry.owner_before(registry)&&!registry.owner_before(p.registry)&&
  !registry->impl_->retired&&registry->impl_->thread==p.thread&&s.state_==p.state&&s.source_==p.source&&
  s.state_generation_==p.generation&&s.debug_==p.control&&!p.control->retired.load()&&
  p.state->ui_source_revision==p.ui_revision,"Initial visibility actual Session registry/source controls changed");
 const auto i=registry->impl_->pending.find(p.key);
 need(i!=registry->impl_->pending.end()&&i->second.get()==this&&!p.initial_factory_self.expired()&&
  !p.initial_factory_self.owner_before(i->second)&&!i->second.owner_before(p.initial_factory_self),
  "Initial visibility exact registered Pending control changed");
 // Core caller separately proves canonical Core/node controls. No weak.lock,
 // SQL/currentness/client/provider callback or source operand read here.
}
void BnwUnitMovePending::require_initial_visibility_phase_without_callbacks()const{
 require_initial_visibility_source_without_callbacks();const auto&p=*impl_;
 need(!p.prefix.initial_visibility_returned&&p.prefix.boundary==BnwUnitMoveBoundary::initial_city_visibility_owner,
  "Initial visibility exact pre-receiver phase changed");
}
void BnwUnitMovePending::require_initial_visibility_hidden_slot_phase_without_callbacks()const{
 require_initial_visibility_source_without_callbacks();const auto&p=*impl_;
 need(p.prefix.initial_visibility_returned&&
  ((p.prefix.boundary==BnwUnitMoveBoundary::initial_city_garrison_policy_owner&&!p.prefix.initial_garrison_policy_returned&&!p.prefix.initial_fortify_returned)||
   (p.prefix.boundary==BnwUnitMoveBoundary::initial_new_sight_owner&&p.prefix.initial_garrison_policy_returned&&p.prefix.initial_fortify_returned&&!p.prefix.initial_sight_returned)||
   (p.prefix.boundary==BnwUnitMoveBoundary::initial_plot_add_unit_owner&&p.prefix.initial_garrison_policy_returned&&p.prefix.initial_fortify_returned&&p.prefix.initial_sight_returned&&!p.prefix.initial_plot_area_returned)||
   (p.prefix.boundary==BnwUnitMoveBoundary::initial_city_strength_owner&&p.prefix.initial_garrison_policy_returned&&p.prefix.initial_fortify_returned&&p.prefix.initial_sight_returned&&p.prefix.initial_plot_area_returned&&!p.prefix.initial_city_strength_returned)||
   ((p.prefix.boundary==BnwUnitMoveBoundary::initial_post_city_setxy_owner||p.prefix.boundary==BnwUnitMoveBoundary::initial_set_position_owner||
    ((p.prefix.boundary==BnwUnitMoveBoundary::initial_post_position_owner||
     ((p.prefix.boundary==BnwUnitMoveBoundary::initial_enemy_city_range_owner||
      ((p.prefix.boundary==BnwUnitMoveBoundary::initial_happiness_prefix_owner||
       ((p.prefix.boundary==BnwUnitMoveBoundary::initial_happiness_founder_owner||
        ((p.prefix.boundary==BnwUnitMoveBoundary::initial_happiness_excess_owner||
         (p.prefix.boundary==BnwUnitMoveBoundary::initial_unit_setxy_owner&&p.prefix.initial_happiness_returned)||
         ((p.prefix.boundary==BnwUnitMoveBoundary::initial_popup_release_owner||(p.prefix.boundary==BnwUnitMoveBoundary::initial_garrison_dirty_owner&&p.prefix.initial_popup_released)||((p.prefix.boundary==BnwUnitMoveBoundary::initial_center_unit_owner&&p.prefix.initial_popup_released&&p.prefix.initial_garrison_dirty_returned)||((p.prefix.boundary==BnwUnitMoveBoundary::initial_unit_name_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_unit_class_count_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_team_unit_class_count_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_player_unit_class_count_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_num_builders_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_resource_used_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_military_support_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_free_unit_promotions_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_promotion_native_remainder_owner||p.prefix.boundary==BnwUnitMoveBoundary::initial_traits_promotions_owner)&&p.prefix.initial_unit_info_promotion_prefix_stored))&&p.prefix.initial_military_support_returned))&&p.prefix.initial_resource_nuke_zero_returned))&&p.prefix.initial_support_zero_returned))&&p.prefix.initial_player_class_counter_returned))&&p.prefix.initial_team_class_counter_returned))&&p.prefix.initial_game_class_counter_returned))&&p.prefix.initial_name_game_counter_returned))&&p.prefix.initial_popup_released&&p.prefix.initial_garrison_dirty_returned&&p.prefix.initial_center_unit_returned)))&&p.prefix.initial_unit_setxy_returned))&&p.prefix.initial_happiness_native_suffix_stored))&&p.prefix.initial_happiness_prefix_stored))&&p.prefix.initial_enemy_city_range_returned))&&p.prefix.initial_info_bar_dirty_stored))&&p.prefix.initial_set_position_returned))&&p.prefix.initial_garrison_policy_returned&&p.prefix.initial_fortify_returned&&p.prefix.initial_sight_returned&&p.prefix.initial_plot_area_returned&&p.prefix.initial_city_strength_returned)),
  "Initial hidden slot exact issued post-receiver phase changed");
}
void BnwUnitMovePending::record_initial_garrison_fortify_return(std::int32_t modifier,std::uint64_t before,std::uint64_t after)const noexcept{
 // Core-only after source bookends+actual raw city store, no new external receipt.
 (void)before;auto&p=*impl_;p.ui_revision=after;p.prefix.initial_garrison_culture_modifier=modifier;
 p.prefix.initial_garrison_policy_returned=true;p.prefix.initial_fortify_returned=true;
 p.prefix.boundary=BnwUnitMoveBoundary::initial_new_sight_owner;
}
void BnwUnitMovePending::record_initial_sight_return(std::uint64_t revision)const noexcept{
 impl_->ui_revision=revision;impl_->prefix.initial_sight_returned=true;
 impl_->prefix.boundary=BnwUnitMoveBoundary::initial_plot_add_unit_owner;
}
void BnwUnitMovePending::record_initial_plot_area_return(std::uint64_t revision)const noexcept{
 impl_->ui_revision=revision;impl_->prefix.initial_plot_area_returned=true;
 impl_->prefix.boundary=BnwUnitMoveBoundary::initial_city_strength_owner;
}
void BnwUnitMovePending::require_initial_city_strength_phase_without_callbacks()const{
 require_initial_visibility_hidden_slot_phase_without_callbacks();const auto&p=*impl_;
 need(p.prefix.initial_plot_area_returned&&
  ((p.prefix.boundary==BnwUnitMoveBoundary::initial_city_strength_owner&&!p.prefix.initial_city_strength_returned)||
   ((p.prefix.boundary==BnwUnitMoveBoundary::initial_post_city_setxy_owner||p.prefix.boundary==BnwUnitMoveBoundary::initial_set_position_owner||
    ((p.prefix.boundary==BnwUnitMoveBoundary::initial_post_position_owner||
     ((p.prefix.boundary==BnwUnitMoveBoundary::initial_enemy_city_range_owner||
      ((p.prefix.boundary==BnwUnitMoveBoundary::initial_happiness_prefix_owner||
       ((p.prefix.boundary==BnwUnitMoveBoundary::initial_happiness_founder_owner||
        ((p.prefix.boundary==BnwUnitMoveBoundary::initial_happiness_excess_owner||
         (p.prefix.boundary==BnwUnitMoveBoundary::initial_unit_setxy_owner&&p.prefix.initial_happiness_returned)||
         ((p.prefix.boundary==BnwUnitMoveBoundary::initial_popup_release_owner||(p.prefix.boundary==BnwUnitMoveBoundary::initial_garrison_dirty_owner&&p.prefix.initial_popup_released)||((p.prefix.boundary==BnwUnitMoveBoundary::initial_center_unit_owner&&p.prefix.initial_popup_released&&p.prefix.initial_garrison_dirty_returned)||((p.prefix.boundary==BnwUnitMoveBoundary::initial_unit_name_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_unit_class_count_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_team_unit_class_count_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_player_unit_class_count_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_num_builders_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_resource_used_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_military_support_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_free_unit_promotions_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_promotion_native_remainder_owner||p.prefix.boundary==BnwUnitMoveBoundary::initial_traits_promotions_owner)&&p.prefix.initial_unit_info_promotion_prefix_stored))&&p.prefix.initial_military_support_returned))&&p.prefix.initial_resource_nuke_zero_returned))&&p.prefix.initial_support_zero_returned))&&p.prefix.initial_player_class_counter_returned))&&p.prefix.initial_team_class_counter_returned))&&p.prefix.initial_game_class_counter_returned))&&p.prefix.initial_name_game_counter_returned))&&p.prefix.initial_popup_released&&p.prefix.initial_garrison_dirty_returned&&p.prefix.initial_center_unit_returned)))&&p.prefix.initial_unit_setxy_returned))&&p.prefix.initial_happiness_native_suffix_stored))&&p.prefix.initial_happiness_prefix_stored))&&p.prefix.initial_enemy_city_range_returned))&&p.prefix.initial_info_bar_dirty_stored))&&p.prefix.initial_set_position_returned))&&p.prefix.initial_city_strength_returned)),
  "Initial city strength exact genuine pending phase changed");
}
void BnwUnitMovePending::record_initial_post_city_checks_return(std::uint64_t meet,std::uint64_t adjacent)const noexcept{
 auto&p=*impl_;p.prefix.initial_no_effect_meet_returns=meet;p.prefix.initial_adjacent_plots_checked=adjacent;
 p.prefix.initial_post_city_checks_returned=true;p.prefix.boundary=BnwUnitMoveBoundary::initial_set_position_owner;
}
void BnwUnitMovePending::require_initial_set_position_phase_without_callbacks()const{
 require_initial_city_strength_phase_without_callbacks();const auto&p=*impl_;
 need(p.prefix.initial_city_strength_returned&&p.prefix.initial_post_city_checks_returned&&
  (p.prefix.boundary==BnwUnitMoveBoundary::initial_set_position_owner||
   ((p.prefix.boundary==BnwUnitMoveBoundary::initial_post_position_owner||
     ((p.prefix.boundary==BnwUnitMoveBoundary::initial_enemy_city_range_owner||
      ((p.prefix.boundary==BnwUnitMoveBoundary::initial_happiness_prefix_owner||
       ((p.prefix.boundary==BnwUnitMoveBoundary::initial_happiness_founder_owner||
        ((p.prefix.boundary==BnwUnitMoveBoundary::initial_happiness_excess_owner||
         (p.prefix.boundary==BnwUnitMoveBoundary::initial_unit_setxy_owner&&p.prefix.initial_happiness_returned)||
         ((p.prefix.boundary==BnwUnitMoveBoundary::initial_popup_release_owner||(p.prefix.boundary==BnwUnitMoveBoundary::initial_garrison_dirty_owner&&p.prefix.initial_popup_released)||((p.prefix.boundary==BnwUnitMoveBoundary::initial_center_unit_owner&&p.prefix.initial_popup_released&&p.prefix.initial_garrison_dirty_returned)||((p.prefix.boundary==BnwUnitMoveBoundary::initial_unit_name_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_unit_class_count_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_team_unit_class_count_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_player_unit_class_count_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_num_builders_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_resource_used_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_military_support_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_free_unit_promotions_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_promotion_native_remainder_owner||p.prefix.boundary==BnwUnitMoveBoundary::initial_traits_promotions_owner)&&p.prefix.initial_unit_info_promotion_prefix_stored))&&p.prefix.initial_military_support_returned))&&p.prefix.initial_resource_nuke_zero_returned))&&p.prefix.initial_support_zero_returned))&&p.prefix.initial_player_class_counter_returned))&&p.prefix.initial_team_class_counter_returned))&&p.prefix.initial_game_class_counter_returned))&&p.prefix.initial_name_game_counter_returned))&&p.prefix.initial_popup_released&&p.prefix.initial_garrison_dirty_returned&&p.prefix.initial_center_unit_returned)))&&p.prefix.initial_unit_setxy_returned))&&p.prefix.initial_happiness_native_suffix_stored))&&p.prefix.initial_happiness_prefix_stored))&&p.prefix.initial_enemy_city_range_returned))&&p.prefix.initial_info_bar_dirty_stored))&&p.prefix.initial_set_position_returned)),"Initial SetPosition requires actual post-city checks");
}
void BnwUnitMovePending::require_initial_position_returned_without_callbacks()const{
 require_initial_city_strength_phase_without_callbacks();const auto&p=*impl_;
 need(p.prefix.initial_post_city_checks_returned&&p.prefix.initial_set_position_returned&&
  (p.prefix.boundary==BnwUnitMoveBoundary::initial_post_position_owner||
   ((p.prefix.boundary==BnwUnitMoveBoundary::initial_enemy_city_range_owner||
      ((p.prefix.boundary==BnwUnitMoveBoundary::initial_happiness_prefix_owner||
       ((p.prefix.boundary==BnwUnitMoveBoundary::initial_happiness_founder_owner||
        ((p.prefix.boundary==BnwUnitMoveBoundary::initial_happiness_excess_owner||
         (p.prefix.boundary==BnwUnitMoveBoundary::initial_unit_setxy_owner&&p.prefix.initial_happiness_returned)||
         ((p.prefix.boundary==BnwUnitMoveBoundary::initial_popup_release_owner||(p.prefix.boundary==BnwUnitMoveBoundary::initial_garrison_dirty_owner&&p.prefix.initial_popup_released)||((p.prefix.boundary==BnwUnitMoveBoundary::initial_center_unit_owner&&p.prefix.initial_popup_released&&p.prefix.initial_garrison_dirty_returned)||((p.prefix.boundary==BnwUnitMoveBoundary::initial_unit_name_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_unit_class_count_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_team_unit_class_count_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_player_unit_class_count_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_num_builders_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_resource_used_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_military_support_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_free_unit_promotions_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_promotion_native_remainder_owner||p.prefix.boundary==BnwUnitMoveBoundary::initial_traits_promotions_owner)&&p.prefix.initial_unit_info_promotion_prefix_stored))&&p.prefix.initial_military_support_returned))&&p.prefix.initial_resource_nuke_zero_returned))&&p.prefix.initial_support_zero_returned))&&p.prefix.initial_player_class_counter_returned))&&p.prefix.initial_team_class_counter_returned))&&p.prefix.initial_game_class_counter_returned))&&p.prefix.initial_name_game_counter_returned))&&p.prefix.initial_popup_released&&p.prefix.initial_garrison_dirty_returned&&p.prefix.initial_center_unit_returned)))&&p.prefix.initial_unit_setxy_returned))&&p.prefix.initial_happiness_native_suffix_stored))&&p.prefix.initial_happiness_prefix_stored))&&p.prefix.initial_enemy_city_range_returned))&&p.prefix.initial_info_bar_dirty_stored)),"Initial position requires genuine renderer return once");
}
BnwUnitMovePending::InitialUnitInfoDirtyOperands BnwUnitMovePending::require_initial_unit_info_dirty_store(const ui::BnwClientSelectionControl&input)const{
 require_current();const auto&p=*impl_;
 need(p.prefix.initial_unit&&p.prefix.initial_set_position_returned&&!p.prefix.initial_info_bar_dirty_stored&&
  p.prefix.boundary==BnwUnitMoveBoundary::initial_post_position_owner,
  "Initial UnitInfo requires genuine once returned Position");
 need(p.initial_input.lock().get()==&input&&p.session->city_selection_.lock().get()==&input,
  "Initial UnitInfo requires SAME actual registered popup/selection owner");
 const auto core=p.initial_core.lock();need(bool(core),"Initial UnitInfo genuine Core expired");
 const auto revision=input.revision();core->require_initial_unit_info_predecessors_without_callbacks();
 require_initial_visibility_source_without_callbacks();
 return {native(p.key.first),native(p.key.second),p.dim_occurrence,p.generation,p.ui_revision,revision};
}
void BnwUnitMovePending::require_initial_happiness_prefix_without_callbacks()const{
 require_initial_visibility_source_without_callbacks();const auto&p=*impl_;
 need(p.prefix.initial_set_position_returned&&p.prefix.initial_info_bar_dirty_stored&&p.prefix.initial_enemy_city_range_returned&&
  p.prefix.initial_happiness_prefix_stored&&(p.prefix.boundary==BnwUnitMoveBoundary::initial_happiness_founder_owner||
   ((p.prefix.boundary==BnwUnitMoveBoundary::initial_happiness_excess_owner||
         (p.prefix.boundary==BnwUnitMoveBoundary::initial_unit_setxy_owner&&p.prefix.initial_happiness_returned)||
         ((p.prefix.boundary==BnwUnitMoveBoundary::initial_popup_release_owner||(p.prefix.boundary==BnwUnitMoveBoundary::initial_garrison_dirty_owner&&p.prefix.initial_popup_released)||((p.prefix.boundary==BnwUnitMoveBoundary::initial_center_unit_owner&&p.prefix.initial_popup_released&&p.prefix.initial_garrison_dirty_returned)||((p.prefix.boundary==BnwUnitMoveBoundary::initial_unit_name_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_unit_class_count_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_team_unit_class_count_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_player_unit_class_count_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_num_builders_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_resource_used_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_military_support_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_free_unit_promotions_owner||((p.prefix.boundary==BnwUnitMoveBoundary::initial_promotion_native_remainder_owner||p.prefix.boundary==BnwUnitMoveBoundary::initial_traits_promotions_owner)&&p.prefix.initial_unit_info_promotion_prefix_stored))&&p.prefix.initial_military_support_returned))&&p.prefix.initial_resource_nuke_zero_returned))&&p.prefix.initial_support_zero_returned))&&p.prefix.initial_player_class_counter_returned))&&p.prefix.initial_team_class_counter_returned))&&p.prefix.initial_game_class_counter_returned))&&p.prefix.initial_name_game_counter_returned))&&p.prefix.initial_popup_released&&p.prefix.initial_garrison_dirty_returned&&p.prefix.initial_center_unit_returned)))&&p.prefix.initial_unit_setxy_returned))&&p.prefix.initial_happiness_native_suffix_stored)),
  "Initial happiness requires genuine paired prefix stores");
}
std::uint64_t BnwUnitMovePending::preflight_initial_happiness_game_data()const{
 require_initial_happiness_prefix_without_callbacks();const auto&p=*impl_;
 need(p.prefix.initial_happiness_native_suffix_stored&&!p.prefix.initial_happiness_excess_stored,
  "Initial happiness GameData requires actual pending suffix");
 const auto input=p.initial_input.lock();need(input&&p.session->city_selection_.lock().get()==input.get(),
  "Initial happiness GameData canonical input expired");
 input->preflight_game_data_dirty_setter();return input->game_data_dirty_setter_count();
}
std::uint64_t BnwUnitMovePending::execute_initial_happiness_game_data_dirty(std::uint64_t expected_before)const{
 require_initial_happiness_prefix_without_callbacks();const auto&p=*impl_;
 need(p.prefix.initial_happiness_excess_stored&&!p.prefix.initial_happiness_returned,
  "Initial happiness GameData lacks actual excess stage");
 const auto input=p.initial_input.lock();need(input&&p.session->city_selection_.lock().get()==input.get(),
  "Initial happiness GameData actual input changed");
 need(input->game_data_dirty_setter_count()==expected_before&&expected_before<UINT64_MAX,
  "Initial happiness GameData actual once occurrence changed");
 input->set_game_data_dirty(ui::BnwClientGameDataDirtyKind::game_data);
 require_initial_visibility_source_without_callbacks();
 need(input->game_data_dirty_setter_count()==expected_before+1&&input->game_data_dirty()==std::optional<bool>{true},
  "Initial happiness genuine GameData flag did not store");return expected_before+1;
}
void BnwUnitMovePending::record_initial_happiness_excess_store(std::uint64_t revision)const noexcept{
 auto&p=*impl_;p.ui_revision=revision;p.prefix.initial_happiness_excess_stored=true;
}
void BnwUnitMovePending::record_initial_happiness_return(std::uint64_t occurrence)const noexcept{
 auto&p=*impl_;p.prefix.initial_game_data_occurrence=occurrence;p.prefix.initial_happiness_returned=true;
 p.prefix.boundary=BnwUnitMoveBoundary::initial_unit_setxy_owner;
}

const BnwUnitMovePrefix&BnwUnitMovePending::initial_setxy_tail_prefix_without_callbacks()const{
 require_initial_visibility_source_without_callbacks();return impl_->prefix;
}
void BnwUnitMovePending::require_initial_popup_release_store(const ui::BnwClientSelectionControl&input)const{
 require_initial_visibility_source_without_callbacks();const auto&p=*impl_;
 need(p.prefix.initial_unit_setxy_returned&&!p.prefix.initial_popup_released&&
  p.prefix.boundary==BnwUnitMoveBoundary::initial_popup_release_owner,"Initial popup release requires actual UnitSetXY return once");
 const auto current=p.initial_input.lock();need(current&&current.get()==&input&&p.session->city_selection_.lock()==current,
  "Initial popup release actual canonical input changed");
 input.require_current();need(input.active_player()==p.key.first&&detail::bnw_game_active_player_read(*p.state)==p.key.first&&
  input.popups_suppressed()==std::optional<bool>{true}&&input.popup_hold_setter_count()==p.prefix.popup_setter_occurrence,
  "Initial popup release actual owner/hold changed");
 const auto core=p.initial_core.lock();need(bool(core),"Initial popup release genuine Core expired");
 core->require_initial_setxy_tail_without_callbacks();require_initial_visibility_source_without_callbacks();
}
void BnwUnitMovePending::record_initial_popup_release_store(std::uint64_t occurrence,std::uint64_t revision)const noexcept{
 auto&p=*impl_;p.prefix.initial_popup_released=true;p.prefix.popup_release_setter_occurrence=occurrence;
 p.prefix.popup_release_client_revision=revision;p.prefix.boundary=BnwUnitMoveBoundary::initial_garrison_dirty_owner;
}
BnwUnitMovePending::InitialGarrisonDirtyOperands BnwUnitMovePending::require_initial_garrison_dirty_store(const ui::BnwClientSelectionControl&input)const{
 require_initial_visibility_source_without_callbacks();const auto&p=*impl_;
 need(p.prefix.initial_popup_released&&!p.prefix.initial_garrison_dirty_returned&&
  p.prefix.boundary==BnwUnitMoveBoundary::initial_garrison_dirty_owner,"Initial GARRISON requires actual popup release once");
 const auto current=p.initial_input.lock();need(current&&current.get()==&input&&p.session->city_selection_.lock()==current,
  "Initial GARRISON actual canonical input changed");
 input.require_current();need(input.popups_suppressed()==std::optional<bool>{false}&&
  input.popup_hold_setter_count()==p.prefix.popup_release_setter_occurrence,"Initial GARRISON popup release overwritten");
 const auto core=p.initial_core.lock();need(bool(core),"Initial GARRISON genuine Core expired");
 core->require_initial_setxy_tail_without_callbacks();const auto city=core->initial_actual_garrison_city();
 need(city.has_value(),"Actual newborn is not the city's native best defender");
 const auto revision=input.revision();require_initial_visibility_source_without_callbacks();
 core->require_initial_setxy_tail_without_callbacks();
 return {native(p.key.first),*city,p.generation,p.ui_revision,revision};
}
void BnwUnitMovePending::record_initial_garrison_dirty_return(std::optional<std::int32_t>city,std::uint64_t occurrence)const noexcept{
 auto&p=*impl_;p.prefix.initial_new_garrison_city=city;p.prefix.initial_garrison_dirty_occurrence=occurrence;
 p.prefix.initial_garrison_dirty_returned=true;p.prefix.boundary=BnwUnitMoveBoundary::initial_center_unit_owner;
}
void BnwUnitMovePending::execute_initial_popup_release_and_garrison(){
 require_current();const auto input=impl_->initial_input.lock();need(bool(input),"Initial popup/GARRISON actual client owner retired");
 input->finish_initial_set_xy_popup_hold(*this);require_current();
 const auto core=impl_->initial_core.lock();need(bool(core),"Initial GARRISON genuine Core expired");
 const auto city=core->initial_actual_garrison_city();
 if(city)input->set_initial_garrison_dirty(*this);
 else{
  // Native GetGarrisonedCity is NULL when another actual defender wins. The
  // newborn's previous city was genuinely NULL at Reset, so BOTH dirty branches
  // have no receiver. Prove again after allocating/native checks before marker.
  core->require_initial_setxy_tail_without_callbacks();need(!core->initial_actual_garrison_city(),"Initial actual garrison changed during NULL branch");
  require_initial_visibility_source_without_callbacks();record_initial_garrison_dirty_return({},0);
 }
 require_current();
}
void BnwUnitMovePending::record_initial_center_unit_return(std::uint64_t setters,std::uint64_t receivers)const noexcept{
 auto&p=*impl_;p.prefix.initial_center_setters=setters;p.prefix.initial_center_visibility_receivers=receivers;
 p.prefix.initial_center_unit_returned=true;p.prefix.boundary=BnwUnitMoveBoundary::initial_unit_name_owner;
}
void BnwUnitMovePending::record_initial_name_game_counter_return(std::int32_t turn,std::int32_t before,std::int32_t after,std::uint64_t revision)const noexcept{
 auto&p=*impl_;p.ui_revision=revision;p.prefix.initial_game_turn_created=turn;p.prefix.initial_unit_count_before=before;p.prefix.initial_unit_count_after=after;
 p.prefix.initial_name_game_counter_returned=true;p.prefix.boundary=BnwUnitMoveBoundary::initial_unit_class_count_owner;
}
void BnwUnitMovePending::record_initial_game_class_counter_return(std::int32_t unit_class,std::int32_t before,std::int32_t after,std::uint64_t revision)const noexcept{
 auto&p=*impl_;p.ui_revision=revision;p.prefix.initial_unit_class=unit_class;p.prefix.initial_game_class_before=before;p.prefix.initial_game_class_after=after;
 p.prefix.initial_game_class_counter_returned=true;p.prefix.boundary=BnwUnitMoveBoundary::initial_team_unit_class_count_owner;
}
void BnwUnitMovePending::record_initial_team_class_counter_return(std::int32_t team,std::int32_t before,std::int32_t after,std::uint64_t revision)const noexcept{
 auto&p=*impl_;p.ui_revision=revision;p.prefix.initial_unit_class_team=team;p.prefix.initial_team_class_before=before;p.prefix.initial_team_class_after=after;
 p.prefix.initial_team_class_counter_returned=true;p.prefix.boundary=BnwUnitMoveBoundary::initial_player_unit_class_count_owner;
}
void BnwUnitMovePending::record_initial_player_class_counter_return(std::int32_t before,std::int32_t after,std::uint64_t revision)const noexcept{
 auto&p=*impl_;p.ui_revision=revision;p.prefix.initial_player_class_before=before;p.prefix.initial_player_class_after=after;
 p.prefix.initial_player_class_counter_returned=true;p.prefix.boundary=BnwUnitMoveBoundary::initial_num_builders_owner;
}
void BnwUnitMovePending::record_initial_support_zero_return()const noexcept{
 auto&p=*impl_;p.prefix.initial_support_zero_returned=true;p.prefix.boundary=BnwUnitMoveBoundary::initial_resource_used_owner;
}
void BnwUnitMovePending::record_initial_resource_nuke_zero_return(std::size_t slots)const noexcept{
 auto&p=*impl_;p.prefix.initial_resource_slots=slots;p.prefix.initial_resource_nuke_zero_returned=true;p.prefix.boundary=BnwUnitMoveBoundary::initial_military_support_owner;
}

void BnwUnitMovePending::require_initial_military_game_data_identity_without_callbacks(const std::shared_ptr<ui::BnwClientSelectionControl>&input)const{
 require_initial_visibility_source_without_callbacks();const auto&p=*impl_;
 need(input&&!p.initial_input.expired()&&!p.initial_input.owner_before(input)&&!input.owner_before(p.initial_input)&&
  !p.session->city_selection_.expired()&&!p.session->city_selection_.owner_before(input)&&!input.owner_before(p.session->city_selection_),
  "Initial military canonical client input changed");
}
std::shared_ptr<ui::BnwClientSelectionControl>BnwUnitMovePending::retain_initial_military_game_data_input()const{
 require_initial_visibility_source_without_callbacks();const auto&p=*impl_;
 need(p.prefix.initial_resource_nuke_zero_returned&&p.prefix.boundary==BnwUnitMoveBoundary::initial_military_support_owner,
  "Initial military input lacks genuine resource/nuke predecessor");
 const auto input=p.initial_input.lock();require_initial_military_game_data_identity_without_callbacks(input);return input;
}
void BnwUnitMovePending::record_initial_military_count_store(bool incremented,std::int32_t before,std::int32_t after,std::uint64_t revision)const noexcept{
 auto&p=*impl_;p.ui_revision=revision;p.prefix.initial_military_support_incremented=incremented;p.prefix.initial_military_count_before=before;p.prefix.initial_military_count_after=after;
}
void BnwUnitMovePending::record_initial_military_support_return(std::uint64_t occurrence)const noexcept{
 auto&p=*impl_;p.prefix.initial_military_game_data_occurrence=occurrence;p.prefix.initial_military_support_returned=true;p.prefix.boundary=BnwUnitMoveBoundary::initial_free_unit_promotions_owner;
}
void BnwUnitMovePending::record_initial_immobile_moves_computed(std::int32_t before,std::int32_t requested)const noexcept{auto&p=*impl_;p.prefix.initial_immobile_moves_computed=true;p.prefix.initial_moves_before=before;p.prefix.initial_moves_requested=requested;p.prefix.boundary=BnwUnitMoveBoundary::initial_moves_dim_owner;}
void BnwUnitMovePending::record_initial_moves_store(bool changed)const noexcept{
 auto&p=*impl_;p.prefix.initial_moves_value_changed=changed;p.prefix.initial_moves_stored=changed;
}
void BnwUnitMovePending::record_initial_should_dim_return(bool changed)const noexcept{
 auto&p=*impl_;p.prefix.initial_should_dim_enqueue_returned=changed;p.prefix.initial_moves_setter_returned=true;
 p.prefix.boundary=changed?BnwUnitMoveBoundary::initial_moves_selection_owner:BnwUnitMoveBoundary::initial_post_moves_owner;
}
void BnwUnitMovePending::record_initial_moves_selection(bool selected)const noexcept{
 auto&p=impl_->prefix;p.initial_moves_selection_checked=true;p.initial_moves_unit_selected=selected;
}
void BnwUnitMovePending::record_initial_moves_common_reset()const noexcept{impl_->prefix.initial_moves_common_reset=true;}
void BnwUnitMovePending::record_initial_moves_selection_plot(std::optional<std::uint64_t>plot)const noexcept{
 auto&p=impl_->prefix;p.initial_moves_selection_plot_checked=true;p.initial_moves_selected_plot=plot;
}
void BnwUnitMovePending::record_initial_moves_ui_dirty(bool info,std::uint64_t occurrence)const noexcept{
 if(info)impl_->prefix.initial_moves_unit_info_occurrence=occurrence;else impl_->prefix.initial_moves_plot_list_occurrence=occurrence;
}
void BnwUnitMovePending::record_initial_moves_tail_return()const noexcept{
 impl_->prefix.initial_moves_tail_returned=true;impl_->prefix.boundary=BnwUnitMoveBoundary::initial_post_moves_owner;
}
void BnwUnitMovePending::record_initial_religion_return(bool applied,std::int32_t majority,std::int32_t value,std::int32_t strength,std::int32_t spreads)const noexcept{
 auto&p=impl_->prefix;p.initial_religion_returned=true;p.initial_religion_applied=applied;p.initial_religion_majority=majority;p.initial_religion_value=value;p.initial_religion_strength=strength;p.initial_religion_spreads=spreads;
 // post_moves_owner aggregates the mandatory remaining Init suffix.
}
void BnwUnitMovePending::record_initial_tourism_return(std::int32_t one_shot,std::int32_t creation)const noexcept{
 auto&p=impl_->prefix;p.initial_tourism_returned=true;p.initial_tourism_one_shot=one_shot;p.initial_tourism_creation=creation;
}
void BnwUnitMovePending::record_initial_recon_return(std::int32_t count)const noexcept{
 auto&p=impl_->prefix;p.initial_recon_returned=true;p.initial_recon_count=count;
}
void BnwUnitMovePending::record_initial_best_land_return(bool read,bool setter,bool changed,std::int32_t best)const noexcept{
 auto&p=impl_->prefix;p.initial_best_land_returned=true;p.initial_best_land_game_read=read;p.initial_best_land_setter_called=setter;p.initial_best_land_changed=changed;p.initial_best_land_type=best;
}
void BnwUnitMovePending::record_initial_best_land_game_store(std::int32_t type,std::uint64_t revision)const noexcept{
 impl_->ui_revision=revision;auto&p=impl_->prefix;p.initial_best_land_stored=true;p.initial_best_land_changed=true;p.initial_best_land_type=type;
}
void BnwUnitMovePending::record_initial_best_land_dirty_store(std::uint64_t occurrence)const noexcept{impl_->prefix.initial_best_land_unit_info_occurrence=occurrence;}
void BnwUnitMovePending::record_initial_active_game_data_store(std::uint64_t occurrence)const noexcept{impl_->prefix.initial_active_game_data_occurrence=occurrence;}
void BnwUnitMovePending::record_initial_active_game_data_return(bool applied)const noexcept{auto&p=impl_->prefix;p.initial_active_game_data_returned=true;p.initial_active_game_data_applied=applied;}
void BnwUnitMovePending::record_initial_unit_cycler_return(bool applied,std::size_t units,std::size_t stores)const noexcept{auto&p=impl_->prefix;p.initial_unit_cycler_returned=true;p.initial_unit_cycler_applied=applied;p.initial_unit_cycler_units=units;p.initial_unit_cycler_plain_stores=stores;}
void BnwUnitMovePending::record_initial_world_unit_message_return(std::int32_t unit_class,std::int32_t max_global)const noexcept{auto&p=impl_->prefix;p.initial_world_unit_message_returned=true;p.initial_world_unit_class=unit_class;p.initial_world_unit_max_global=max_global;}
void BnwUnitMovePending::record_initial_second_city_strength_return(bool applied,std::int32_t before,std::int32_t after,std::uint64_t occurrence,std::uint64_t revision)const noexcept{auto&p=impl_->prefix;p.initial_second_city_strength_returned=true;p.initial_second_city_strength_applied=applied;p.initial_second_city_strength_before=before;p.initial_second_city_strength_after=after;p.initial_second_city_strength_occurrence=occurrence;impl_->ui_revision=revision;}
void BnwUnitMovePending::record_initial_setup_graphical_return()const noexcept{impl_->prefix.initial_setup_graphical_returned=true;}
void BnwUnitMovePending::record_initial_minor_quest_return(bool tested,std::uint32_t minors,std::uint32_t entries,std::uint32_t gp)const noexcept{auto&p=impl_->prefix;p.initial_minor_quest_returned=true;p.initial_minor_quest_tested=tested;p.initial_minor_quest_minors=minors;p.initial_minor_quest_entries=entries;p.initial_minor_quest_gp_checks=gp;}
void BnwUnitMovePending::record_initial_maintenance_return(bool active,std::int32_t value)const noexcept{auto&p=impl_->prefix;p.initial_maintenance_returned=true;p.initial_maintenance_active=active;p.initial_maintenance_value=value;}
void BnwUnitMovePending::record_initial_army_ai_return(std::int32_t army_before,std::int32_t ai_before,std::int32_t ai_after)const noexcept{auto&p=impl_->prefix;p.initial_army_ai_returned=true;p.initial_army_before=army_before;p.initial_army_after=-1;p.initial_unit_ai_before=ai_before;p.initial_unit_ai_after=ai_after;}
void BnwUnitMovePending::record_initial_exotic_xp_return()const noexcept{auto&p=*impl_;p.prefix.initial_exotic_xp_returned=true;p.prefix.boundary=BnwUnitMoveBoundary::initial_immobile_moves_owner;}
void BnwUnitMovePending::record_initial_embark_promotions_return(std::size_t grants)const noexcept{auto&p=*impl_;p.prefix.initial_embark_promotions_returned=true;p.prefix.initial_embark_promotion_grants=grants;p.prefix.boundary=BnwUnitMoveBoundary::initial_exotic_goods_owner;}
void BnwUnitMovePending::record_initial_player_promotions_return(std::size_t grants)const noexcept{auto&p=*impl_;p.prefix.initial_player_promotions_returned=true;p.prefix.initial_player_promotion_grants=grants;p.prefix.boundary=BnwUnitMoveBoundary::initial_embark_promotions_owner;}
void BnwUnitMovePending::record_initial_traits_promotions_return(std::size_t grants)const noexcept{auto&p=*impl_;p.prefix.initial_traits_promotions_returned=true;p.prefix.initial_traits_promotion_grants=grants;p.prefix.boundary=BnwUnitMoveBoundary::initial_player_free_promotions_owner;}
void BnwUnitMovePending::record_initial_unit_info_promotions_return(std::size_t grants)const noexcept{auto&p=*impl_;p.prefix.initial_unit_info_promotions_returned=true;p.prefix.initial_unit_info_promotion_grants=grants;p.prefix.boundary=BnwUnitMoveBoundary::initial_traits_promotions_owner;}
void BnwUnitMovePending::record_initial_unit_info_promotion_prefix(std::int32_t id,std::size_t slots)const noexcept{
 auto&p=*impl_;p.prefix.initial_unit_info_promotion_prefix_stored=true;p.prefix.initial_unit_info_promotion_id=id;p.prefix.initial_unit_info_promotion_slots=slots;
 p.prefix.boundary=id>=0?BnwUnitMoveBoundary::initial_promotion_native_remainder_owner:BnwUnitMoveBoundary::initial_traits_promotions_owner;
}
void BnwUnitMovePending::record_initial_unit_setxy_return()const noexcept{
 auto&p=*impl_;p.prefix.initial_unit_setxy_returned=true;p.prefix.boundary=BnwUnitMoveBoundary::initial_popup_release_owner;
}
void BnwUnitMovePending::record_initial_happiness_native_suffix_return(std::uint64_t revision)const noexcept{
 auto&p=*impl_;p.ui_revision=revision;p.prefix.initial_happiness_native_suffix_stored=true;
 p.prefix.boundary=BnwUnitMoveBoundary::initial_happiness_excess_owner;
}
void BnwUnitMovePending::record_initial_happiness_prefix_return(std::uint64_t revision)const noexcept{
 auto&p=*impl_;p.ui_revision=revision;p.prefix.initial_happiness_prefix_stored=true;
 p.prefix.boundary=BnwUnitMoveBoundary::initial_happiness_founder_owner;
}
void BnwUnitMovePending::record_initial_enemy_city_range_return(std::uint64_t visited)const noexcept{
 auto&p=*impl_;p.prefix.initial_enemy_city_range_returned=true;p.prefix.initial_enemy_city_plots_checked=visited;
 p.prefix.boundary=BnwUnitMoveBoundary::initial_happiness_prefix_owner;
}
void BnwUnitMovePending::record_initial_unit_info_dirty_store(std::uint64_t occurrence)const noexcept{
 auto&p=*impl_;p.prefix.initial_info_bar_dirty_stored=true;p.prefix.initial_info_bar_dirty_occurrence=occurrence;
 p.prefix.boundary=BnwUnitMoveBoundary::initial_enemy_city_range_owner;
}
void BnwUnitMovePending::execute_initial_unit_info_dirty_prefix(){
 require_current();const auto input=impl_->initial_input.lock();need(bool(input),"Initial UnitInfo actual client owner retired");
 (void)input->set_initial_newborn_info_dirty(*this);
}
void BnwUnitMovePending::record_initial_set_position_return()const noexcept{
 auto&p=*impl_;p.prefix.initial_set_position_returned=true;p.prefix.boundary=BnwUnitMoveBoundary::initial_post_position_owner;
}
void BnwUnitMovePending::record_initial_city_strength_return(std::uint64_t revision)const noexcept{
 impl_->ui_revision=revision;impl_->prefix.initial_city_strength_returned=true;
 impl_->prefix.boundary=BnwUnitMoveBoundary::initial_post_city_setxy_owner;
}
void BnwUnitMovePending::record_initial_visibility_return()const noexcept{
 impl_->prefix.initial_visibility_returned=true;impl_->prefix.boundary=BnwUnitMoveBoundary::initial_city_garrison_policy_owner;
}
void BnwUnitMovePending::require_initial_visibility_core_identity_without_callbacks(const BnwNativeUnitAllocatorCore&core)const{
 require_initial_visibility_source_without_callbacks();const auto&p=*impl_;
 need(!p.initial_core.owner_before(core.factory_self_)&&!core.factory_self_.owner_before(p.initial_core),
  "Initial visibility Pending canonical Core control changed");
}
void BnwUnitMovePending::require_initial_coordinate_store(const BnwNativeUnitAllocatorCore& supplied)const{
 require_current();const auto& p=*impl_;const auto core=p.initial_core.lock();
 need(core&&core.get()==&supplied&&p.prefix.initial_unit&&p.prefix.popup_hold_stored&&p.attempted&&
  !p.prefix.initial_coordinates_stored&&p.prefix.boundary==BnwUnitMoveBoundary::initial_destination_collision_owner,
  "Initial coordinate store wrong genuine source/once phase");
 core->require_initial_placement_current(this);
 core->require_initial_flta_current(*this,p.initial_flta_seals);
 std::size_t at=0;
 const auto visited=initial_collision_semantics(*p.state,p.initial_source_city,native(p.key.first),p.initial_city,p.x,p.y,p.initial_combat,[&](auto value){
  need(at<p.initial_collision_seal.size()&&p.initial_collision_seal[at++]==static_cast<std::uint64_t>(value),
   "Initial collision live semantic operand changed during staging");
 },[&](auto handle){return core->initial_native_unit_at(*this,native(handle.owner),native(handle.id));});
 need(at==p.initial_collision_seal.size()&&visited==p.prefix.initial_collision_entries&&
  !p.retired&&p.session->state_==p.state&&p.session->state_generation_==p.generation&&p.state->ui_source_revision==p.ui_revision&&
  p.session->unit_move_registry_==p.registry.lock(),"Initial collision final noalloc source bookend changed");
}
void BnwUnitMovePending::record_initial_coordinate_store()const noexcept{
 impl_->prefix.initial_coordinates_stored=true;impl_->prefix.boundary=BnwUnitMoveBoundary::initial_city_visibility_owner;
}
void BnwUnitMovePending::execute_initial_destination_collision_prefix(){
 require_current();auto& p=*impl_;
 need(p.prefix.initial_unit&&p.attempted&&p.prefix.popup_hold_stored&&!p.prefix.initial_coordinates_stored&&
  p.prefix.boundary==BnwUnitMoveBoundary::initial_destination_collision_owner,
  "Initial destination collision already executed/wrong native phase");
 const auto core=p.initial_core.lock();need(bool(core),"Initial collision genuine Core expired");
 auto managers=core->prepare_initial_destination_flta_lookup(*this);
 const auto roster=preflight_bnw_map_occupied(p.state->world);
 need(roster.shape_valid,"Initial collision actual raw/projected world roster unresolved");
 const auto* source_city=resolve_bnw_current_city(*p.state,native(p.key.first),p.initial_city);
 need(source_city,"Initial collision genuine native city registry absent");
 core->require_initial_flta_current(*this,managers);
 std::vector<std::uint64_t> seal;
 std::size_t size=0;
 (void)initial_collision_semantics(*p.state,source_city,native(p.key.first),p.initial_city,p.x,p.y,p.initial_combat,
  [&](auto){need(size<seal.max_size(),"Initial collision semantic extent overflow");++size;},
  [&](auto handle){return core->initial_native_unit_at(*this,native(handle.owner),native(handle.id));});
 seal.reserve(size); // Finish allocating storage BEFORE the semantic capture.
 require_current();core->require_initial_flta_current(*this,managers);
 const auto visited=initial_collision_semantics(*p.state,source_city,native(p.key.first),p.initial_city,p.x,p.y,p.initial_combat,
  [&](auto value){need(seal.size()<size,"Initial collision semantic extent changed during preparation");seal.push_back(static_cast<std::uint64_t>(value));},
  [&](auto handle){return core->initial_native_unit_at(*this,native(handle.owner),native(handle.id));});
 need(seal.size()==size,"Initial collision semantic extent changed during preparation");
 require_current();core->require_initial_flta_current(*this,managers);p.initial_flta_seals.swap(managers);p.initial_source_city=source_city;p.initial_collision_seal.swap(seal);p.prefix.initial_collision_entries=visited;
 // No collision effect was skipped: every copied handle resolved/null/death
 // and both reached native war predicates returned false. Source transport
 // is genuinely NULL from Reset. Nonzero unsupported branches failed above.
 core->execute_initial_coordinate_store(*this);
 require_current(); // Actual city visibility receiver remains mandatory next.
}
void BnwUnitMovePending::execute_prefix(){
 require_current();auto& p=*impl_;need(!p.prefix.initial_unit,"Initial SetXY cannot execute move cost/setMoves path");need(!p.attempted,"Movement prefix already attempted; no replay");
 auto registry=p.registry.lock();need(bool(registry),"Movement registered journal absent");
 auto& moves=field(unit(*p.state,p.key).core,"m_iMoves",saves::UnitCoreEncoding::i32).values[0];
 need(moves==p.prefix.before_moves,"Movement native pre-store value changed");
 if(p.prefix.moves_stored){
  need(p.ui_revision<UINT64_MAX&&registry->impl_->next_occurrence<UINT64_MAX,"Movement UI/occurrence revision exhausted");
  need(registry->impl_->queued.size()<registry->impl_->queued.max_size(),"Movement queued intent capacity exhausted");
  registry->impl_->queued.reserve(registry->impl_->queued.size()+1);
 }
 if(p.prefix.moves_stored){
  // SDK setMoves captures plot() BEFORE assigning m_iMoves or calling Dim.
  const auto index=bnw_unit_order_plot(*p.state,p.key);const auto& map=p.state->world;
  need(index<=INT32_MAX&&map.map_plots&&map.map_projection,
   "setMoves original native plot index unavailable");
  const auto xy=map.map_header.grid.coordinates(index);const auto& raw=map.map_plots->plots.at(index);
  const auto& cell=map.map_projection->cells.at(index);
  need(raw.index==index&&cell.index==index&&raw.x==xy[0]&&raw.y==xy[1]&&
   cell.x==xy[0]&&cell.y==xy[1],"setMoves original raw/projected plot identity differs");
  p.prefix.before_plot=static_cast<std::int32_t>(index);
 }
 // No callback or allocation after this actual native attempt marker/store.
 p.attempted=true;
 if(p.prefix.moves_stored){
  moves=p.prefix.after_moves;++p.state->ui_source_revision;++p.ui_revision;
  p.dim_occurrence=++registry->impl_->next_occurrence;
  registry->impl_->queued.push_back({p.key,p.dim_occurrence,p.generation,p.ui_revision,moves<=0});
 }else p.prefix.set_moves_completed=true; // Native no-change setMoves returns without UI getters.
 // Missing actual new-client dim receiver is mandatory BEFORE IsSelected.
 // No dispatch acknowledgement, coordinate relocation or completed move.
}
void BnwUnitMovePending::require_same_registered_pending(const ui::SelectedBnwPreviewHost* selected)const{
 require_current();const auto& p=*impl_;const auto held=p.host.lock();
 need(selected&&held.get()==selected&&&selected->session()==p.session,
  "Dim requires SAME actual selected Host/session");
 need(p.attempted&&p.prefix.moves_stored&&p.dim_occurrence>0,
  "Dim requires actual changed setMoves store/issued occurrence");
 need(detail::bnw_game_active_player_read(*p.state)==p.key.first,
  "Dim actual Game actor differs from registered unit owner");
 const auto& actual=unit(*p.state,p.key);
 need(scalar(actual.core,"m_iMoves")==p.prefix.after_moves,
  "Dim actual canonical stored moves changed");
 const auto registry=p.registry.lock();need(bool(registry),"Dim registered journal absent");
 std::size_t matches=0;
 for(const auto& intent:registry->impl_->queued)if(intent.occurrence==p.dim_occurrence){
  ++matches;need(intent.unit==p.key&&intent.generation==p.generation&&
   intent.world_revision==p.ui_revision&&intent.dim==(p.prefix.after_moves<=0),
   "Dim actual queued occurrence lineage differs");
 }
 need(matches==1&&registry->impl_->next_occurrence>=p.dim_occurrence,
  "Dim actual occurrence absent/ambiguous");
 require_current();
}
void BnwUnitMovePending::begin_stock_dim(const ui::SelectedBnwPreviewHost* selected,
 const BnwStockLuaAttemptLease& loan)const{
 require_same_registered_pending(selected);auto& p=*impl_;
 need(p.prefix.boundary==BnwUnitMoveBoundary::dim_receiver&&
  !p.prefix.dim_attempted&&!p.prefix.dim_returned&&!p.dim_loan,
  "Dim native receiver already attempted; no replay");
 loan.require_active();const auto serial=loan.operation_serial();
 need(loan.world_lease().get()==p.state.get(),"Dim loan actual world allocation differs");
 require_same_registered_pending(selected);loan.require_operation(serial);
 // Authentic weak lease only. Marker precedes the first receiver execute.
 p.dim_loan=loan;p.dim_serial=serial;p.prefix.dim_attempted=true;
}
BnwUnitMovePending::StockDimOperands BnwUnitMovePending::stock_dim_operands()const{
 const auto& p=*impl_;
 need(p.thread==std::this_thread::get_id(),"Dim native operands belong to another thread");
 need(p.dim_loan&&p.prefix.dim_attempted&&!p.prefix.dim_returned,
  "Dim native operands require the actual active receiver attempt");
 p.dim_loan->require_operation(p.dim_serial);
 const auto host=p.host.lock();require_same_registered_pending(host.get());
 const auto& actual=unit(*p.state,p.key);
 StockDimOperands result{native(p.key.first),native(p.key.second),scalar(actual.core,"m_iMoves"),
  p.dim_occurrence,p.generation,p.ui_revision};
 p.dim_loan->require_operation(p.dim_serial);require_same_registered_pending(host.get());return result;
}
void BnwUnitMovePending::accept_stock_dim_return(const ui::SelectedBnwPreviewHost* selected)const{
 require_same_registered_pending(selected);auto& p=*impl_;
 (void)stock_dim_operands();
 need(p.prefix.boundary==BnwUnitMoveBoundary::dim_receiver,
  "Dim actual native return phase changed");
 // SourceOwner calls this only AFTER the real SAME-VM receiver returned and
 // its active serial, VM health and canonical operands were checked again.
 p.prefix.dim_returned=true;p.prefix.boundary=BnwUnitMoveBoundary::selected_ui_tail;
}
void BnwUnitMovePending::continue_post_dim_selection(const ui::SelectedBnwPreviewHost* selected)const{
 require_same_registered_pending(selected);auto& p=*impl_;
 need(p.prefix.dim_attempted&&p.prefix.dim_returned&&p.dim_loan&&
  p.prefix.boundary==BnwUnitMoveBoundary::selected_ui_tail&&!p.prefix.selection_attempted,
  "Post-Dim selection requires actual once receiver return");
 p.dim_loan->require_operation(p.dim_serial);
 // Resolve only the SAME Session-registered canonical Frame/UI input owner.
 // Missing or retired input is unknown, not an unselected unit.
 const auto input=p.session->city_selection_.lock();
 need(bool(input),"Post-Dim actual canonical input owner absent");
 input->require_current();auto revision=input->revision();
 require_same_registered_pending(selected);p.dim_loan->require_operation(p.dim_serial);
 p.prefix.selection_attempted=true; // Before the actual native getter.
 const bool is_selected=input->is_unit_selected(native(p.key.first),native(p.key.second));
 input->require_current();need(input->revision()==revision,
  "Post-Dim canonical input changed during IsUnitSelected");
 require_same_registered_pending(selected);p.dim_loan->require_operation(p.dim_serial);
 p.prefix.unit_selected=is_selected;
 if(is_selected){
  p.prefix.boundary=BnwUnitMoveBoundary::common_pathfinder_reset_owner;
  try{p.session->force_reset_bnw_common_pathfinder();}
  catch(const BnwCommonPathfinderUnavailable&){
   require_same_registered_pending(selected);p.dim_loan->require_operation(p.dim_serial);return;
  }
  p.prefix.common_cache_reset=true;
  require_same_registered_pending(selected);p.dim_loan->require_operation(p.dim_serial);
  // Native UnitInfo setter returns BEFORE GetSelectionPlot. Its real new-client
  // owner updates flags/ordered delivery and invalidates publication immediately.
  p.prefix.boundary=BnwUnitMoveBoundary::unit_info_dirty_owner;
  p.dirty_input_revision=revision;
  revision=input->set_moves_dirty(*this);
  require_same_registered_pending(selected);p.dim_loan->require_operation(p.dim_serial);
 }
 p.prefix.boundary=BnwUnitMoveBoundary::selection_plot_owner;
 const auto plot=input->selection_plot();
 input->require_current();need(input->revision()==revision,
  "Post-Dim canonical input changed during GetSelectionPlot");
 require_same_registered_pending(selected);p.dim_loan->require_operation(p.dim_serial);
 need(p.prefix.before_plot&&*p.prefix.before_plot>=0&&(!plot||*plot<=INT32_MAX),
  "Post-Dim original/selection native plot index unavailable");
 p.prefix.selection_plot_read=true;
 p.prefix.selected_plot=plot?std::optional{static_cast<std::int32_t>(*plot)}:std::nullopt;
 const auto native_selection=plot?static_cast<std::int32_t>(*plot):-1;
 if(*p.prefix.before_plot==native_selection){
  p.prefix.boundary=BnwUnitMoveBoundary::plot_list_dirty_owner;
  p.dirty_input_revision=revision;
  revision=input->set_moves_dirty(*this);
  input->require_current();need(input->revision()==revision,"Canonical input changed during PlotList dirty");
  require_same_registered_pending(selected);p.dim_loan->require_operation(p.dim_serial);
 }
 // Actual NULL (-1) or a distinct actual input plot skips PlotList dirty.
 // Only this setMoves component returned; CvUnit::move still needs its tail.
 p.prefix.set_moves_completed=true;
 p.prefix.boundary=p.show?BnwUnitMoveBoundary::visible_to_watching_human:BnwUnitMoveBoundary::set_xy_popup_owner;
}
BnwUnitMovePending::DirtyOperands BnwUnitMovePending::require_dirty_store(
 const ui::BnwClientSelectionControl& input)const{
 const auto& p=*impl_;const auto host=p.host.lock();
 require_same_registered_pending(host.get());
 need(p.dim_loan&&p.prefix.dim_attempted&&p.prefix.dim_returned&&p.prefix.selection_attempted,
  "Unit dirty requires genuine post-Dim/current-selection operation");
 p.dim_loan->require_operation(p.dim_serial);input.require_current();
 const auto registered=p.session->city_selection_.lock();
 need(registered.get()==&input&&input.revision()==p.dirty_input_revision,
  "Unit dirty requires exact SAME registered client input revision");
 ui::BnwClientUnitDirtyKind kind;
 if(p.prefix.boundary==BnwUnitMoveBoundary::unit_info_dirty_owner){
  need(p.prefix.unit_selected==std::optional{true}&&p.prefix.common_cache_reset&&
   !p.prefix.unit_info_dirty_stored&&!p.prefix.selection_plot_read,
   "UnitInfo dirty native order/once differs");kind=ui::BnwClientUnitDirtyKind::unit_info;
 }else{
  need(p.prefix.boundary==BnwUnitMoveBoundary::plot_list_dirty_owner&&p.prefix.selection_plot_read&&
   p.prefix.before_plot&&p.prefix.selected_plot==p.prefix.before_plot&&!p.prefix.plot_list_dirty_stored&&
   p.prefix.unit_selected.has_value()&&(!*p.prefix.unit_selected||p.prefix.unit_info_dirty_stored),
   "PlotList dirty native comparison/order/once differs");kind=ui::BnwClientUnitDirtyKind::plot_list_buttons;
 }
 p.dim_loan->require_operation(p.dim_serial);require_same_registered_pending(host.get());
 return {native(p.key.first),native(p.key.second),p.prefix.after_moves,
  p.dim_occurrence,p.generation,p.ui_revision,kind,
  p.prefix.before_plot.value_or(-1),p.dirty_input_revision};
}
void BnwUnitMovePending::record_dirty_store(ui::BnwClientUnitDirtyKind kind,std::uint64_t occurrence)const noexcept{
 // Called only by the private registered input setter, after its actual stores.
 if(kind==ui::BnwClientUnitDirtyKind::unit_info){impl_->prefix.unit_info_dirty_stored=true;impl_->prefix.unit_info_dirty_occurrence=occurrence;}
 else {impl_->prefix.plot_list_dirty_stored=true;impl_->prefix.plot_list_dirty_occurrence=occurrence;}
}
void BnwUnitMovePending::fail_stock_dim()const noexcept{
 // Only SourceOwner on the genuine constructor thread may invoke this.
 // Preserve stores/queue/loan history and nonrollback Lua effects.
 impl_->retired=true;
}
const BnwUnitMovePrefix& BnwUnitMovePending::prefix()const{require_current();return impl_->prefix;}
std::vector<BnwUnitMoveDimIntent> BnwUnitMovePending::queued_new_client_intents()const{
 require_current();auto result=impl_->registry.lock()->impl_->queued;require_current();return result;
}
// Root must merge the narrow declarations/journal retirement hooks described
// in ROOT-INTEGRATION.txt before this NEW TU can be built or adopted.
std::shared_ptr<const BnwUnitMovePending> LiveSession::begin_bnw_unit_move_prefix(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,BnwUnitOrderKey key,std::int32_t x,std::int32_t y,bool show){
 auto p=BnwUnitMovePending::create(*this,host,key,x,y,show);p->execute_prefix();return p;
}
void LiveSession::retire_bnw_unit_move()noexcept{if(unit_move_registry_)unit_move_registry_->retire();}
}
