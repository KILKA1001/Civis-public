#include "civ5/ui/bnw_client_selection_control.hpp"
#include "civ5/runtime/bnw_attrition_city_info_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_unit_info_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_unit_selected_read.hpp"
#include "civ5/runtime/bnw_attrition_selected_unit_info_dirty_loan.hpp"
#include "civ5/runtime/bnw_attrition_selection_plot_read.hpp"
#include "civ5/runtime/bnw_attrition_plot_list_dirty_loan.hpp"
#include <algorithm>
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_policy_adoption.hpp"
#include "civ5/runtime/bnw_live_player_gold_pending.hpp"
#include "civ5/runtime/bnw_unit_move_pending.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include <atomic>
#include <climits>
#include <functional>
#include <stdexcept>
#include <thread>
#include <array>
#include <map>
#include <tuple>
namespace civ5::ui {
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::int32_t native(std::int64_t v){need(v>=INT32_MIN&&v<=INT32_MAX,"Client selection native identity overflow");return static_cast<std::int32_t>(v);}
std::int32_t unit_identity_scalar(const saves::UnitCoreObservation& core,const char* name){
 const saves::UnitCoreField* found=nullptr;
 for(const auto& field:core.fields)if(field.name&&std::string_view(field.name)==name){
  need(!found,"Client selected unit has duplicate identity scalar");found=&field;
 }
 need(found&&found->encoding==saves::UnitCoreEncoding::i32&&found->values.size()==1&&
  found->end_offset>=found->start_offset&&found->end_offset-found->start_offset==4,
  "Client selected unit native i32 identity unavailable");
 return native(found->values.front());
}
void require_plot_identity(const runtime::LiveSessionState& state,std::optional<std::int64_t> requested){
 constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
 const auto& world=state.world;const auto& header=world.map_header;const auto& grid=header.grid;
 need(header.layout==bnw&&world.map_plots&&world.map_plots->layout==bnw&&world.map_projection,
  "Canonical plot input BNW map source unavailable");
 need(grid.width>0&&grid.height>0&&grid.width<=INT32_MAX&&grid.height<=INT32_MAX&&
  grid.height<=static_cast<std::uint64_t>(INT32_MAX)/grid.width,
  "Canonical plot input native dimensions/count overflow");
 const auto count=grid.width*grid.height;const auto& map=*world.map_projection;
 need(map.grid.width==grid.width&&map.grid.height==grid.height&&
  map.wrap_x==header.wrap_x&&map.wrap_y==header.wrap_y&&
  world.map_plots->plots.size()==count&&map.cells.size()==count,
  "Canonical plot input raw/projected map shape differs");
 // Validate the entire actual ordered roster: duplicate, shifted or foreign
 // record identity must not become a genuine picked plot by numeric equality.
 for(std::uint64_t i=0;i<count;++i){
  const auto xy=grid.coordinates(i);const auto& raw=world.map_plots->plots.at(i);
  const auto& cell=map.cells.at(i);
  need(raw.index==i&&cell.index==i&&raw.x==xy[0]&&raw.y==xy[1]&&
   cell.x==xy[0]&&cell.y==xy[1],"Canonical plot input full raw/projected roster differs");
 }
 if(requested)need(*requested>=0&&*requested<=INT32_MAX&&
  static_cast<std::uint64_t>(*requested)<count,"Canonical plot input index outside current native map");
}
void require_unit_identity(const runtime::LiveSessionState& state,std::int32_t owner,std::int32_t id){
 constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
 need(owner>=0&&owner<64&&id>=0&&state.world.all_players&&
  state.world.all_players->layout==bnw&&state.world.all_players->players.size()==64,
  "Client selected unit canonical roster unavailable");
 const auto& player=state.world.all_players->players.at(owner);
 need(player.saved_player_id==owner,"Client selected unit raw player identity differs");
 const saves::SavedUnitObservation* found=nullptr;
 for(const auto& unit:player.units.units)if(unit_identity_scalar(unit.core,"m_iID")==id){
  need(!found,"Client selected unit fullID is ambiguous");found=&unit;
 }
 need(found&&found->core.layout==bnw&&found->tail.layout==bnw&&
  unit_identity_scalar(found->core,"m_eOwner")==owner,
  "Client selected unit raw canonical fullID unavailable");
 const auto army=state.maintenance.players.find(owner);
 need(army!=state.maintenance.players.end(),"Client selected unit owned roster unavailable");
 const auto owned=army->second.units.find(id);
 need(owned!=army->second.units.end()&&owned->second.source_core==
  std::optional{std::pair{found->core.start_offset,found->core.end_offset}},
  "Client selected unit raw/owned identity mirror differs");
}
}
struct BnwClientSelectionControl::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::atomic<bool> retired{false};
 std::shared_ptr<const SelectedBnwPreviewHost> host;
 std::shared_ptr<const runtime::LiveSessionState> state;
 std::shared_ptr<const saves::SavedSession> source;
 runtime::SessionCheckpointIdentity identity;
 std::function<bool()> rules_current;
 std::uint64_t generation=0,revision=1;
 bool actual_ui_bound=false;
 std::optional<bool> dont_show_popups;
 std::uint64_t popup_hold_setters=0;
 std::uint64_t garrison_next_occurrence=1;
 std::vector<runtime::BnwSpecificCityInfoEvent> garrison_dirty_events;
 // SDK exposes a virtual setter, not a saved/imported UI flag. Unknown until
 // the genuine retained engine-begin producer performs this Civis store.
 std::optional<bool> no_selection_list_cycle;
 std::optional<bool> score_dirty;
 std::uint64_t score_dirty_setters=0;
 std::optional<bool> city_info_dirty;
 std::uint64_t city_info_dirty_setters=0;
 struct PolicyProcessDirtyReturn {
  std::weak_ptr<const runtime::BnwPolicyAdoptionPending> source;
  std::weak_ptr<const runtime::LiveSessionState> world;
  std::uint64_t generation=0,world_revision=0,before=0,after=0,city_before=0,game_before=0;
  std::uint64_t serial=0;bool city_returned=false,game_returned=false;
 };
 std::optional<PolicyProcessDirtyReturn> policy_process_dirty_return;
 std::optional<bool> policies_dirty;
 std::uint64_t policies_dirty_setters=0;
 struct OuterPolicyDirtyReturn {
  std::weak_ptr<const runtime::BnwPolicyAdoptionPending> source;
  std::weak_ptr<const runtime::LiveSessionState> world;
  std::uint64_t generation=0,world_revision=0,before=0,after=0,game_before=0,policies_before=0,serial=0;
  bool game_returned=false,active_getter_returned=false,actor_active=false,policies_returned=false;
 };
 std::optional<OuterPolicyDirtyReturn> outer_policy_dirty_return;
 struct GoldenAgeDirtyReturn {
  std::weak_ptr<const runtime::BnwPolicyAdoptionPending> source;
  std::weak_ptr<const runtime::LiveSessionState> world;
  std::uint64_t generation=0,world_revision=0,before=0,after=0,game_before=0,city_before=0,policies_before=0,serial=0;
  bool game_returned=false;
 };
 std::optional<GoldenAgeDirtyReturn> golden_age_dirty_return;
 std::optional<bool> research_buttons_dirty;std::uint64_t research_buttons_dirty_setters=0;bool free_tech_dirty_busy=false;
 // Runtime-only graphics transient: never copied into setup.extensions or
 // a save. Unknown imported layout values remain absent until native overwrite.
 using ResourceLayoutKey=std::tuple<std::array<std::uint8_t,16>,std::uint64_t,std::int64_t>;
 std::map<ResourceLayoutKey,bool> resource_plot_layouts;
 std::optional<bool> plot_data_dirty;std::uint64_t plot_data_dirty_setters=0;bool resource_layout_busy=false;
 bool happiness_dirty_busy=false;
 struct HappinessDirtyReturn {
  std::weak_ptr<const runtime::BnwPolicyAdoptionPending> source;std::weak_ptr<const runtime::LiveSessionState> world;
  std::uint64_t generation=0,world_revision=0,before=0,after=0,game_before=0,serial=0;bool stored=false;
 };std::optional<HappinessDirtyReturn> happiness_dirty_return;
 struct ResourceLayoutReturn {
  std::weak_ptr<const runtime::BnwPolicyAdoptionPending> source;std::weak_ptr<const runtime::LiveSessionState> world;
  ResourceLayoutKey key;std::uint64_t generation=0,world_revision=0,before=0,after=0,plot_before=0,serial=0;
  bool ui_stored=false,layout_stored=false;
 };std::optional<ResourceLayoutReturn> resource_layout_return;
 struct FreeTechDirtyReturn {
  std::weak_ptr<const runtime::BnwPolicyAdoptionPending> source;std::weak_ptr<const runtime::LiveSessionState> world;
  std::uint64_t generation=0,world_revision=0,before=0,after=0,research_before=0,game_before=0,city_before=0,policies_before=0,timer_before=0,serial=0;
  std::optional<bool> research_flag_before;bool stored=false;
 };std::optional<FreeTechDirtyReturn> free_tech_dirty_return;
 std::optional<bool> turn_timer_dirty;
 std::uint64_t turn_timer_dirty_setters=0;
 std::optional<bool> misc_buttons_dirty,selection_buttons_dirty;
 std::uint64_t misc_buttons_dirty_setters=0,selection_buttons_dirty_setters=0;
 bool live_gold_dirty_busy=false;
 struct LiveGoldDirtyReturn {
  std::weak_ptr<const runtime::BnwLivePlayerGoldPending> pending;
  std::weak_ptr<const runtime::LiveSessionState> world;
  runtime::TreasuryDirty kind=runtime::TreasuryDirty::misc_buttons;
  std::uint64_t serial=0,generation=0,world_revision=0,before=0,after=0,misc_before=0,selection_before=0,game_before=0;
  std::uint64_t city_before=0,policies_before=0,research_before=0,timer_before=0,score_before=0,league_before=0;
  std::optional<bool> misc_flag,selection_flag,game_flag;
  bool stored=false;
 };std::optional<LiveGoldDirtyReturn> live_gold_dirty_return;
 std::optional<bool> game_data_dirty;
 std::uint64_t game_data_dirty_setters=0;
 std::optional<bool> league_screen_dirty;
 std::uint64_t league_screen_dirty_setters=0;
 // Fresh Civis UI storage only; imported engine flags are not inferred.
 bool unit_info_dirty=false,plot_list_dirty=false;
 std::uint64_t dirty_next_occurrence=1;
 std::vector<BnwClientUnitDirtyEvent> unit_dirty_events;
 // Constructor initializes the real Civis input value empty. No import UI
 // reset, original engine selection membership or supplied boolean is inferred.
 std::optional<std::pair<std::int64_t,std::int64_t>> selected;
 std::optional<std::int64_t> unit;
 std::optional<std::int32_t> unit_owner;
 std::optional<std::int32_t> plot; // actual new-client empty input at creation
 std::array<std::uint8_t,16> plot_map_guid{};
 const saves::PlotObservation* plot_storage=nullptr;
 const saves::SavedMapCellProjection* projection_storage=nullptr;
};
BnwClientSelectionControl::BnwClientSelectionControl(std::unique_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwClientSelectionControl::~BnwClientSelectionControl(){retire();}

void BnwClientSelectionControl::consume_live_gold_dirty(
 const std::shared_ptr<const runtime::BnwLivePlayerGoldPending>& supplied,runtime::LiveSessionState& state,
 runtime::TreasuryDirty kind,const runtime::BnwStockLuaAttemptLease& lease){
 auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.live_gold_dirty_busy,"Live Gold dirty foreign creator/reentry");
 p.live_gold_dirty_busy=true;struct End{Impl& p;~End(){p.live_gold_dirty_busy=false;}}end{p};
 const auto gold=runtime::BnwLivePlayerGoldPending::retain_factory_owner(supplied);
 need(p.host&&p.state.get()==&state,"Live Gold dirty actual Selection Host/world absent");
 const auto serial=lease.operation_serial();
 require_current();gold->require_dirty_source(*p.host,state,kind,lease);lease.require_operation(serial);
 need(p.revision<UINT64_MAX&&p.misc_buttons_dirty_setters<UINT64_MAX&&p.selection_buttons_dirty_setters<UINT64_MAX&&
  p.game_data_dirty_setters<UINT64_MAX,"Live Gold native dirty occurrence/revision exhausted");
 Impl::LiveGoldDirtyReturn r;r.pending=gold;r.world=p.state;r.kind=kind;r.serial=serial;r.generation=p.generation;
 r.world_revision=state.ui_source_revision;r.before=p.revision;r.misc_before=p.misc_buttons_dirty_setters;
 r.selection_before=p.selection_buttons_dirty_setters;r.game_before=p.game_data_dirty_setters;
 r.city_before=p.city_info_dirty_setters;r.policies_before=p.policies_dirty_setters;r.research_before=p.research_buttons_dirty_setters;
 r.timer_before=p.turn_timer_dirty_setters;r.score_before=p.score_dirty_setters;r.league_before=p.league_screen_dirty_setters;
 r.misc_flag=p.misc_buttons_dirty;r.selection_flag=p.selection_buttons_dirty;r.game_flag=p.game_data_dirty;
 require_current();gold->require_dirty_source(*p.host,state,kind,lease);lease.require_operation(serial);
 const auto canonical=p.host->session().city_selection_.lock();
 need(canonical&&canonical.get()==this&&!p.retired.load()&&p.actual_ui_bound&&p.host->session().state_==p.state&&
  p.host->session().source_==p.source&&p.host->session().state_generation_==r.generation&&state.ui_source_revision==r.world_revision&&
  p.revision==r.before&&p.misc_buttons_dirty_setters==r.misc_before&&p.selection_buttons_dirty_setters==r.selection_before&&
  p.game_data_dirty_setters==r.game_before&&p.misc_buttons_dirty==r.misc_flag&&p.selection_buttons_dirty==r.selection_flag&&p.game_data_dirty==r.game_flag&&
  p.city_info_dirty_setters==r.city_before&&p.policies_dirty_setters==r.policies_before&&p.research_buttons_dirty_setters==r.research_before&&
  p.turn_timer_dirty_setters==r.timer_before&&p.score_dirty_setters==r.score_before&&p.league_screen_dirty_setters==r.league_before,
  "Live Gold source changed before actual original dirty setter");
 // Fix the previously earned physical client cursor BEFORE a subsequent
 // native store, so a foreign/manual UI mutation cannot impersonate the
 // consecutive original4->6->11 branch. This diagnostic view is creator-only.
 const auto prior=gold->view();
 const auto ordinal=kind==runtime::TreasuryDirty::misc_buttons?0u:
  kind==runtime::TreasuryDirty::selection_buttons?1u:kind==runtime::TreasuryDirty::game_data?2u:3u;
 need(ordinal<3&&prior.dirty_setters==ordinal&&
  (ordinal==0||prior.dirty_client_revisions[ordinal-1]==r.before),
  "Live Gold original consecutive client cursor changed before setter");
 // Original SetGoldTimes100 executes MiscButtons4, SelectionButtons6,
 // GameData11 in separate native calls. Already-true flags still store true.
 switch(kind){
  case runtime::TreasuryDirty::misc_buttons:++p.misc_buttons_dirty_setters;p.misc_buttons_dirty=true;++p.revision;break;
  case runtime::TreasuryDirty::selection_buttons:++p.selection_buttons_dirty_setters;p.selection_buttons_dirty=true;++p.revision;break;
  case runtime::TreasuryDirty::game_data:store_game_data_dirty_without_callbacks();break;
  default:throw std::runtime_error("Live Gold unknown native dirty kind");
 }
 r.stored=true;r.after=p.revision;p.live_gold_dirty_return=r;
 gold->accept_actual_dirty_return(*this,kind);
 // No return/history reset in failure: native flag and earned receipt survive
 // any later source/provider rejection. Root source quarantines and throws.
 lease.require_operation(serial);(void)require_live_gold_dirty_return(*gold,kind);
}
std::uint64_t BnwClientSelectionControl::require_live_gold_dirty_return(
 const runtime::BnwLivePlayerGoldPending& gold,runtime::TreasuryDirty kind)const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.actual_ui_bound&&p.live_gold_dirty_return,
  "Live Gold actual Selection dirty return absent/foreign");
 const auto& r=*p.live_gold_dirty_return;const auto actual=r.pending.lock();const auto world=r.world.lock();
 const bool misc=kind==runtime::TreasuryDirty::misc_buttons,selection=kind==runtime::TreasuryDirty::selection_buttons,game=kind==runtime::TreasuryDirty::game_data;
 need(actual&&actual.get()==&gold&&r.kind==kind&&(misc||selection||game)&&world&&world.get()==p.state.get()&&
  !world.owner_before(p.state)&&!p.state.owner_before(world)&&p.host->session().city_selection_.lock().get()==this&&
  p.host->session().state_==p.state&&p.host->session().source_==p.source&&p.host->session().state_generation_==r.generation&&
  r.generation==p.generation&&world->ui_source_revision==r.world_revision&&r.serial!=0&&r.before<UINT64_MAX&&r.after==r.before+1&&p.revision==r.after&&
  r.misc_before<UINT64_MAX&&r.selection_before<UINT64_MAX&&r.game_before<UINT64_MAX&&
  p.misc_buttons_dirty_setters==r.misc_before+(misc?1u:0u)&&p.selection_buttons_dirty_setters==r.selection_before+(selection?1u:0u)&&
  p.game_data_dirty_setters==r.game_before+(game?1u:0u)&&p.misc_buttons_dirty==(misc?std::optional<bool>{true}:r.misc_flag)&&
  p.selection_buttons_dirty==(selection?std::optional<bool>{true}:r.selection_flag)&&p.game_data_dirty==(game?std::optional<bool>{true}:r.game_flag)&&
  p.city_info_dirty_setters==r.city_before&&p.policies_dirty_setters==r.policies_before&&p.research_buttons_dirty_setters==r.research_before&&
  p.turn_timer_dirty_setters==r.timer_before&&p.score_dirty_setters==r.score_before&&p.league_screen_dirty_setters==r.league_before&&r.stored,
  "Live Gold SAME physical Selection/world and one concrete native dirty return differs");
 gold.require_dirty_identity_without_callbacks(*p.host,*world,kind,r.serial);return r.after;
}
void BnwClientSelectionControl::store_outer_policies_dirty_without_callbacks(){
 need(impl_->revision<UINT64_MAX&&impl_->policies_dirty_setters<UINT64_MAX,"Actual Policies32 setter exhausted");
 ++impl_->policies_dirty_setters;impl_->policies_dirty=true;++impl_->revision;
}
void BnwClientSelectionControl::consume_outer_policy_dirty(
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& supplied,runtime::LiveSessionState& state,const runtime::BnwStockLuaAttemptLease& reserved){
 auto& p=*impl_;need(p.thread==std::this_thread::get_id(),"Outer policy dirty foreign Selection creator");
 const auto policy=runtime::BnwPolicyAdoptionPending::retain_factory_owner(supplied);const auto lease=reserved;
 need(p.host&&p.state.get()==&state,"Outer policy dirty actual Selection Host/world absent");
 const auto serial=lease.operation_serial();p.outer_policy_dirty_return.reset();
 try{
  require_current();policy->require_outer_policy_dirty_source(*p.host,state);lease.require_operation(serial);
  need(p.revision<UINT64_MAX&&p.game_data_dirty_setters<UINT64_MAX,"Outer policy GameData setter exhausted");
  Impl::OuterPolicyDirtyReturn r;r.source=policy;r.world=p.state;r.generation=p.generation;r.world_revision=state.ui_source_revision;
  r.before=p.revision;r.game_before=p.game_data_dirty_setters;r.policies_before=p.policies_dirty_setters;r.serial=serial;
  require_current();policy->require_outer_policy_dirty_source(*p.host,state);lease.require_operation(serial);
  const auto canonical=p.host->session().city_selection_.lock();
  need(canonical&&canonical.get()==this&&!p.retired.load()&&p.actual_ui_bound&&p.host->session().state_==p.state&&
   p.host->session().state_generation_==r.generation&&state.ui_source_revision==r.world_revision&&p.revision==r.before&&
   p.game_data_dirty_setters==r.game_before&&p.policies_dirty_setters==r.policies_before,
   "Outer policy source changed before first GameData setter");
  store_game_data_dirty_without_callbacks();r.game_returned=true;
  // Original SDK tests GC active player only AFTER GameData11 returned.
  r.actor_active=policy->outer_policy_actor_is_active(*p.host,state);r.active_getter_returned=true;
  require_current();policy->require_outer_policy_dirty_source(*p.host,state);lease.require_operation(serial);
  need(p.host->session().city_selection_.lock().get()==this&&p.host->session().state_==p.state&&!p.retired.load()&&p.actual_ui_bound&&
   p.host->session().state_generation_==r.generation&&state.ui_source_revision==r.world_revision&&p.revision==r.before+1&&
   p.game_data_dirty_setters==r.game_before+1&&p.policies_dirty_setters==r.policies_before,
   "Outer policy UI/world changed during actual active-player read");
  if(r.actor_active){store_outer_policies_dirty_without_callbacks();r.policies_returned=true;}
  r.after=p.revision;p.outer_policy_dirty_return=r;
  policy->accept_actual_outer_policy_dirty_return(*this);
  lease.require_operation(serial);require_outer_policy_dirty_return(*policy);
 }catch(...){policy->fail_outer_policy_dirty_dispatch();p.outer_policy_dirty_return.reset();throw;}
}
void BnwClientSelectionControl::require_outer_policy_dirty_return(const runtime::BnwPolicyAdoptionPending& policy)const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.actual_ui_bound&&p.outer_policy_dirty_return,
  "Outer policy genuine Selection return absent/foreign");
 const auto& r=*p.outer_policy_dirty_return;const auto actual=r.source.lock();const auto world=r.world.lock();
 const auto extra=r.actor_active?1u:0u;
 need(actual&&actual.get()==&policy&&world&&world.get()==p.state.get()&&!world.owner_before(p.state)&&!p.state.owner_before(world)&&
  p.host->session().city_selection_.lock().get()==this&&p.host->session().state_==p.state&&p.host->session().source_==p.source&&
  p.host->session().state_generation_==p.generation&&r.generation==p.generation&&world->ui_source_revision==r.world_revision&&
  r.before<=UINT64_MAX-1-extra&&r.after==r.before+1+extra&&p.revision==r.after&&r.game_before<UINT64_MAX&&
  p.game_data_dirty_setters==r.game_before+1&&r.policies_before<=UINT64_MAX-extra&&p.policies_dirty_setters==r.policies_before+extra&&
  r.game_returned&&r.active_getter_returned&&r.policies_returned==r.actor_active&&p.game_data_dirty==true&&(!r.actor_active||p.policies_dirty==true),
  "Outer policy SAME source/world and ordered GameData/activegetter/Policies return differs");
}

void BnwClientSelectionControl::store_free_tech_research_buttons_dirty_without_callbacks(){
 need(impl_->revision<UINT64_MAX&&impl_->research_buttons_dirty_setters<UINT64_MAX,"Actual free tech ResearchButtons setter exhausted");
 ++impl_->research_buttons_dirty_setters;impl_->research_buttons_dirty=true;++impl_->revision;
}
void BnwClientSelectionControl::consume_happiness_dirty(
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& supplied,runtime::LiveSessionState& state,const runtime::BnwStockLuaAttemptLease& reserved){
 auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.happiness_dirty_busy,"Policy happiness GameData foreign creator/reentry");
 p.happiness_dirty_busy=true;struct End{Impl& p;~End(){p.happiness_dirty_busy=false;}}end{p};
 const auto policy=runtime::BnwPolicyAdoptionPending::retain_factory_owner(supplied);const auto lease=reserved;
 need(p.host&&p.state.get()==&state&&lease.world_lease()==p.state,"Policy happiness actual Selection Host/world absent");
 const auto serial=lease.operation_serial();p.happiness_dirty_return.reset();
 try{
  require_current();policy->require_happiness_dirty_source(*p.host,state);lease.require_operation(serial);
  need(p.revision<UINT64_MAX&&p.game_data_dirty_setters<UINT64_MAX,"Policy happiness GameData counter/revision exhausted");
  Impl::HappinessDirtyReturn r;r.source=policy;r.world=p.state;r.generation=p.generation;r.world_revision=state.ui_source_revision;
  r.before=p.revision;r.game_before=p.game_data_dirty_setters;r.serial=serial;const auto flag_before=p.game_data_dirty;
  require_current();policy->require_happiness_dirty_source(*p.host,state);lease.require_operation(serial);
  const auto canonical=p.host->session().city_selection_.lock();
  need(canonical&&canonical.get()==this&&!p.retired.load()&&p.actual_ui_bound&&p.host->session().state_==p.state&&p.host->session().source_==p.source&&
   p.host->session().state_generation_==r.generation&&state.ui_source_revision==r.world_revision&&p.revision==r.before&&
   p.game_data_dirty_setters==r.game_before&&p.game_data_dirty==flag_before&&policy->private_happiness_dirty_operation_serial()==serial,
   "Policy happiness source changed before concrete unconditional GameData setter");
  // SDK DoUpdateHappiness ends with this unconditional real setter. Even an
  // already-true flag earns one occurrence/client delta; no active-player gate.
  store_game_data_dirty_without_callbacks();r.stored=true;r.after=p.revision;p.happiness_dirty_return=r;
  policy->accept_actual_happiness_dirty_return(*this);
  lease.require_operation(serial);require_happiness_dirty_return(*policy);
 }catch(...){policy->fail_happiness_dirty_dispatch();throw;}
}
void BnwClientSelectionControl::require_happiness_dirty_return(const runtime::BnwPolicyAdoptionPending& policy)const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.actual_ui_bound&&p.happiness_dirty_return&&p.host,"Policy happiness physical GameData return absent/foreign");
 const auto& r=*p.happiness_dirty_return;const auto actual=r.source.lock();const auto world=r.world.lock();const auto canonical=p.host->session().city_selection_.lock();
 need(actual&&actual.get()==&policy&&world&&world.get()==p.state.get()&&!world.owner_before(p.state)&&!p.state.owner_before(world)&&
  canonical&&canonical.get()==this&&p.host->session().state_==p.state&&p.host->session().source_==p.source&&p.host->session().state_generation_==p.generation&&
  r.generation==p.generation&&world->ui_source_revision==r.world_revision&&r.serial!=0&&r.serial==policy.private_happiness_dirty_operation_serial()&&
  r.before<UINT64_MAX&&r.after==r.before+1&&p.revision==r.after&&r.game_before<UINT64_MAX&&p.game_data_dirty_setters==r.game_before+1&&p.game_data_dirty==true&&r.stored,
  "Policy happiness SAME canonical source/world and concrete GameData return differs");
 // Native flag/count only; this is neither rendered/presented nor Steam ACK.
}

void BnwClientSelectionControl::consume_resource_policy_layout(
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& supplied,runtime::LiveSessionState& state,const runtime::BnwStockLuaAttemptLease& reserved){
 auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.resource_layout_busy,"Policy resource layout foreign creator/reentry");
 p.resource_layout_busy=true;struct End{Impl& p;~End(){p.resource_layout_busy=false;}}end{p};
 const auto policy=runtime::BnwPolicyAdoptionPending::retain_factory_owner(supplied);const auto lease=reserved;
 need(p.host&&p.state.get()==&state&&lease.world_lease()==p.state,"Policy resource layout actual Selection Host/world absent");
 const auto serial=lease.operation_serial();p.resource_layout_return.reset();
 try{
  require_current();policy->require_resource_policy_layout_source(*p.host,state);lease.require_operation(serial);
  const auto plot=policy->private_resource_policy_layout_plot();require_plot_identity(state,plot);
  need(p.revision<UINT64_MAX&&p.plot_data_dirty_setters<UINT64_MAX,"Policy resource PlotData counter/revision exhausted");
  Impl::ResourceLayoutReturn r;r.source=policy;r.world=p.state;r.generation=p.generation;r.world_revision=state.ui_source_revision;
  r.key={state.world.map_header.guid,p.generation,plot};r.before=p.revision;r.plot_before=p.plot_data_dirty_setters;r.serial=serial;
  const auto flag_before=p.plot_data_dirty;const auto prior=p.resource_plot_layouts;
  auto desired=prior;desired.insert_or_assign(r.key,true); // Stage allocation, no authoritative native effect yet.
  require_current();policy->require_resource_policy_layout_source(*p.host,state);lease.require_operation(serial);
  const auto canonical=p.host->session().city_selection_.lock();
  need(canonical&&canonical.get()==this&&!p.retired.load()&&p.actual_ui_bound&&p.host->session().state_==p.state&&p.host->session().source_==p.source&&
   p.host->session().state_generation_==r.generation&&state.ui_source_revision==r.world_revision&&p.revision==r.before&&
   p.plot_data_dirty_setters==r.plot_before&&p.plot_data_dirty==flag_before&&p.resource_plot_layouts==prior&&
   std::get<0>(r.key)==state.world.map_header.guid&&policy->private_resource_policy_layout_plot()==plot,
   "Policy resource source/transient changed before concrete native stores");
  require_plot_identity(state,plot);
  // CvPlot::setLayoutDirty(true): the real DLLUI dirty setter is FIRST,
  // followed by m_bPlotLayoutDirty=true on the SAME native indexed plot.
  // Both are runtime-only render effects; gameplay caches/worldrev unchanged.
  p.plot_data_dirty=true;++p.plot_data_dirty_setters;++p.revision;r.ui_stored=true;
  p.resource_plot_layouts.swap(desired);r.layout_stored=true;r.after=p.revision;p.resource_layout_return=r;
  policy->accept_actual_resource_policy_layout_return(*this);
  lease.require_operation(serial);require_resource_policy_layout_return(*policy);
 }catch(...){policy->fail_resource_policy_layout_dispatch();throw;}
}
void BnwClientSelectionControl::require_resource_policy_layout_return(const runtime::BnwPolicyAdoptionPending& policy)const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.actual_ui_bound&&p.resource_layout_return&&p.host,"Policy resource physical return absent/foreign");
 const auto& r=*p.resource_layout_return;const auto actual=r.source.lock();const auto world=r.world.lock();const auto canonical=p.host->session().city_selection_.lock();
 const auto layout=p.resource_plot_layouts.find(r.key);
 need(actual&&actual.get()==&policy&&world&&world.get()==p.state.get()&&!world.owner_before(p.state)&&!p.state.owner_before(world)&&
  canonical&&canonical.get()==this&&p.host->session().state_==p.state&&p.host->session().source_==p.source&&p.host->session().state_generation_==p.generation&&
  r.generation==p.generation&&std::get<0>(r.key)==world->world.map_header.guid&&std::get<1>(r.key)==p.generation&&world->ui_source_revision==r.world_revision&&
  r.serial!=0&&r.serial==policy.private_resource_policy_layout_operation_serial()&&policy.resource_policy_layout_plot()==std::get<2>(r.key)&&
  r.before<UINT64_MAX&&r.after==r.before+1&&p.revision==r.after&&r.plot_before<UINT64_MAX&&p.plot_data_dirty_setters==r.plot_before+1&&
  p.plot_data_dirty==true&&layout!=p.resource_plot_layouts.end()&&layout->second&&r.ui_stored&&r.layout_stored,
  "Policy resource SAME canonical source/world and ordered native PlotData/layout return differs");
 // This proves physical stores only, never delivery/presentation or save ACK.
}
bool BnwClientSelectionControl::resource_policy_plot_layout_matches_without_callbacks(const runtime::LiveSessionState& state,std::uint64_t generation,std::int64_t plot)const{
 const auto& p=*impl_;if(p.thread!=std::this_thread::get_id()||p.retired.load()||!p.actual_ui_bound||!p.host||p.state.get()!=&state||p.generation!=generation)return false;
 const auto canonical=p.host->session().city_selection_.lock();
 if(!canonical||canonical.get()!=this||p.host->session().state_!=p.state||p.host->session().source_!=p.source||p.host->session().state_generation_!=generation)return false;
 const auto found=p.resource_plot_layouts.find({state.world.map_header.guid,generation,plot});return found!=p.resource_plot_layouts.end()&&found->second;
}

void BnwClientSelectionControl::consume_free_tech_dirty(
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& supplied,runtime::LiveSessionState& state,const runtime::BnwStockLuaAttemptLease& reserved){
 auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.free_tech_dirty_busy,"Free tech ResearchButtons foreign creator/reentry");
 p.free_tech_dirty_busy=true;struct End{Impl& p;~End(){p.free_tech_dirty_busy=false;}}end{p};
 const auto policy=runtime::BnwPolicyAdoptionPending::retain_factory_owner(supplied);const auto lease=reserved;
 need(p.host&&p.state.get()==&state&&lease.world_lease()==p.state,"Free tech ResearchButtons actual Selection Host/world absent");
 const auto serial=lease.operation_serial();p.free_tech_dirty_return.reset();
 try{
  // Actual SourceOwner reserves this one native call BEFORE retaining Engine
  // and canonical receiving UI. This receiver issues no second reservation.
  require_current();policy->require_free_tech_dirty_source(*p.host,state);lease.require_operation(serial);
  need(p.revision<UINT64_MAX&&p.research_buttons_dirty_setters<UINT64_MAX,"Free tech actual ResearchButtons counter/revision exhausted");
  Impl::FreeTechDirtyReturn r;r.source=policy;r.world=p.state;r.generation=p.generation;r.world_revision=state.ui_source_revision;
  r.before=p.revision;r.research_before=p.research_buttons_dirty_setters;r.research_flag_before=p.research_buttons_dirty;
  r.game_before=p.game_data_dirty_setters;r.city_before=p.city_info_dirty_setters;r.policies_before=p.policies_dirty_setters;r.timer_before=p.turn_timer_dirty_setters;r.serial=serial;
  require_current();policy->require_free_tech_dirty_source(*p.host,state);lease.require_operation(serial);
  const auto canonical=p.host->session().city_selection_.lock();
  need(canonical&&canonical.get()==this&&!p.retired.load()&&p.actual_ui_bound&&p.host->session().state_==p.state&&p.host->session().source_==p.source&&
   p.host->session().state_generation_==r.generation&&state.ui_source_revision==r.world_revision&&p.revision==r.before&&
   p.research_buttons_dirty_setters==r.research_before&&p.research_buttons_dirty==r.research_flag_before&&p.game_data_dirty_setters==r.game_before&&
   p.city_info_dirty_setters==r.city_before&&p.policies_dirty_setters==r.policies_before&&p.turn_timer_dirty_setters==r.timer_before,
   "Free tech source changed before concrete ResearchButtons setter");
  // Native setDirty(ResearchButtons,true) is unconditional after count store:
  // even an already-true flag receives one concrete setter and client delta.
  store_free_tech_research_buttons_dirty_without_callbacks();r.stored=true;r.after=p.revision;p.free_tech_dirty_return=r;
  policy->accept_actual_free_tech_dirty_return(*this);
  lease.require_operation(serial);require_free_tech_dirty_return(*policy);
 }catch(...){policy->fail_free_tech_dispatch();throw;}
}
void BnwClientSelectionControl::require_free_tech_dirty_return(const runtime::BnwPolicyAdoptionPending& policy)const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.actual_ui_bound&&p.free_tech_dirty_return,
  "Free tech genuine ResearchButtons return absent/foreign");
 const auto& r=*p.free_tech_dirty_return;const auto actual=r.source.lock();const auto world=r.world.lock();const auto canonical=p.host->session().city_selection_.lock();
 need(actual&&actual.get()==&policy&&world&&world.get()==p.state.get()&&!world.owner_before(p.state)&&!p.state.owner_before(world)&&
  canonical&&canonical.get()==this&&p.host->session().state_==p.state&&p.host->session().source_==p.source&&p.host->session().state_generation_==p.generation&&
  r.generation==p.generation&&world->ui_source_revision==r.world_revision&&r.serial!=0&&r.before<UINT64_MAX&&r.after==r.before+1&&p.revision==r.after&&
  r.research_before<UINT64_MAX&&p.research_buttons_dirty_setters==r.research_before+1&&p.research_buttons_dirty==true&&
  p.game_data_dirty_setters==r.game_before&&p.city_info_dirty_setters==r.city_before&&p.policies_dirty_setters==r.policies_before&&p.turn_timer_dirty_setters==r.timer_before&&r.stored,
  "Free tech SAME canonical source/world and concrete ResearchButtons return differs");
 // Pure exact native setter receipt only. It is not a rendered/presented ACK
 // or authorization to grant a technology, reset observers, or skip Add.
}

void BnwClientSelectionControl::consume_golden_age_dirty(
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& supplied,runtime::LiveSessionState& state,const runtime::BnwStockLuaAttemptLease& reserved){
 auto& p=*impl_;need(p.thread==std::this_thread::get_id(),"GoldenAge GameData foreign Selection creator");
 const auto policy=runtime::BnwPolicyAdoptionPending::retain_factory_owner(supplied);const auto lease=reserved;
 need(p.host&&p.state.get()==&state&&lease.world_lease()==p.state,"GoldenAge GameData actual Selection Host/world absent");
 const auto serial=lease.operation_serial();p.golden_age_dirty_return.reset();
 try{
  // SourceOwner reserved the native call BEFORE retaining Engine/UI. This
  // consumer never issues a second begin or a generic supplied success flag.
  require_current();policy->require_golden_age_dirty_source(*p.host,state);lease.require_operation(serial);
  need(p.revision<UINT64_MAX&&p.game_data_dirty_setters<UINT64_MAX,"GoldenAge actual GameData occurrence/revision exhausted");
  Impl::GoldenAgeDirtyReturn r;r.source=policy;r.world=p.state;r.generation=p.generation;r.world_revision=state.ui_source_revision;
  r.before=p.revision;r.game_before=p.game_data_dirty_setters;r.city_before=p.city_info_dirty_setters;r.policies_before=p.policies_dirty_setters;r.serial=serial;
  require_current();policy->require_golden_age_dirty_source(*p.host,state);lease.require_operation(serial);
  const auto canonical=p.host->session().city_selection_.lock();
  need(canonical&&canonical.get()==this&&!p.retired.load()&&p.actual_ui_bound&&p.host->session().state_==p.state&&p.host->session().source_==p.source&&
   p.host->session().state_generation_==r.generation&&state.ui_source_revision==r.world_revision&&p.revision==r.before&&
   p.game_data_dirty_setters==r.game_before&&p.city_info_dirty_setters==r.city_before&&p.policies_dirty_setters==r.policies_before,
   "GoldenAge source changed before its concrete GameData11 setter");
  store_game_data_dirty_without_callbacks();r.game_returned=true;r.after=p.revision;p.golden_age_dirty_return=r;
  policy->accept_actual_golden_age_dirty_return(*this);
  // Policy55 accepts native return without running callbackful reward stores
  // inside this receiver. Its original tail follows only AFTER these bookends.
  lease.require_operation(serial);require_golden_age_dirty_return(*policy);
 }catch(...){policy->fail_golden_age_dirty_dispatch();p.golden_age_dirty_return.reset();throw;}
}
void BnwClientSelectionControl::require_golden_age_dirty_return(const runtime::BnwPolicyAdoptionPending& policy)const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.actual_ui_bound&&p.golden_age_dirty_return,
  "GoldenAge genuine Selection return absent/foreign");
 const auto& r=*p.golden_age_dirty_return;const auto actual=r.source.lock();const auto world=r.world.lock();const auto canonical=p.host->session().city_selection_.lock();
 need(actual&&actual.get()==&policy&&world&&world.get()==p.state.get()&&!world.owner_before(p.state)&&!p.state.owner_before(world)&&
  canonical&&canonical.get()==this&&p.host->session().state_==p.state&&p.host->session().source_==p.source&&p.host->session().state_generation_==p.generation&&
  r.generation==p.generation&&world->ui_source_revision==r.world_revision&&r.serial!=0&&r.before<UINT64_MAX&&r.after==r.before+1&&p.revision==r.after&&
  r.game_before<UINT64_MAX&&p.game_data_dirty_setters==r.game_before+1&&p.city_info_dirty_setters==r.city_before&&p.policies_dirty_setters==r.policies_before&&
  r.game_returned&&p.game_data_dirty==true,"GoldenAge SAME canonical source/world and concrete GameData11 native return differs");
 // Pure sealed scalar return. Never current-world admission, imported dirty
 // flag inference, successful renderer ACK or a replacement notification.
}

void BnwClientSelectionControl::store_policy_city_info_dirty_without_callbacks(){
 need(impl_->revision<UINT64_MAX&&impl_->city_info_dirty_setters<UINT64_MAX,"Actual CityInfo18 setter exhausted");
 ++impl_->city_info_dirty_setters;impl_->city_info_dirty=true;++impl_->revision;
}
void BnwClientSelectionControl::store_game_data_dirty_without_callbacks(){
 need(impl_->revision<UINT64_MAX&&impl_->game_data_dirty_setters<UINT64_MAX,"Actual GameData11 setter exhausted");
 ++impl_->game_data_dirty_setters;impl_->game_data_dirty=true;++impl_->revision;
}
void BnwClientSelectionControl::consume_policy_process_dirty(
 const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>& supplied,runtime::LiveSessionState& state,const runtime::BnwStockLuaAttemptLease& reserved){
 auto& p=*impl_;need(p.thread==std::this_thread::get_id(),"Policy dirty foreign Selection creator");
 const auto policy=runtime::BnwPolicyAdoptionPending::retain_factory_owner(supplied);
 need(p.host&&p.state.get()==&state,"Policy dirty Selection actual Host/world absent");
 // SourceOwner reserved this exact operation before retaining the callbackful
 // EngineUI provider. A second begin here would replay an already reserved call.
 const auto lease=reserved;
 const auto serial=lease.operation_serial();p.policy_process_dirty_return.reset();
 try{
  require_current();policy->require_policy_process_dirty_source(*p.host,state);lease.require_operation(serial);
  need(p.revision<=UINT64_MAX-2&&p.city_info_dirty_setters<UINT64_MAX&&p.game_data_dirty_setters<UINT64_MAX,
   "Policy dirty actual paired setter occurrence/revision exhausted");
  Impl::PolicyProcessDirtyReturn receipt;
  receipt.source=policy;receipt.world=p.state;receipt.generation=p.generation;receipt.world_revision=state.ui_source_revision;
  receipt.before=p.revision;receipt.city_before=p.city_info_dirty_setters;receipt.game_before=p.game_data_dirty_setters;receipt.serial=serial;
  // Last callbackful source checks precede the two concrete stores. Both dirty
  // values become true even when a preceding producer had already set true.
  require_current();policy->require_policy_process_dirty_source(*p.host,state);lease.require_operation(serial);
  const auto canonical=p.host->session().city_selection_.lock();
  need(canonical&&canonical.get()==this&&p.host->session().state_==p.state&&p.host->session().state_generation_==receipt.generation&&
   !p.retired.load()&&p.actual_ui_bound&&state.ui_source_revision==receipt.world_revision&&p.revision==receipt.before&&
   p.city_info_dirty_setters==receipt.city_before&&p.game_data_dirty_setters==receipt.game_before,
   "Policy dirty source changed before genuine ordered setters");
  store_policy_city_info_dirty_without_callbacks();receipt.city_returned=true;
  store_game_data_dirty_without_callbacks();receipt.game_returned=true;receipt.after=p.revision;
  p.policy_process_dirty_return=receipt;
  policy->accept_actual_policy_process_dirty_return(*this);
  lease.require_operation(serial);require_policy_process_dirty_return(*policy);
 }catch(...){policy->fail_policy_process_dirty_dispatch();p.policy_process_dirty_return.reset();throw;}
}
void BnwClientSelectionControl::require_policy_process_dirty_return(const runtime::BnwPolicyAdoptionPending& policy)const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.actual_ui_bound&&p.policy_process_dirty_return,
  "Policy dirty genuine Selection return absent/foreign");
 const auto& r=*p.policy_process_dirty_return;const auto actual=r.source.lock();const auto world=r.world.lock();
 const auto canonical=p.host->session().city_selection_.lock();
 need(actual&&actual.get()==&policy&&world&&world.get()==p.state.get()&&!world.owner_before(p.state)&&!p.state.owner_before(world)&&
  canonical&&canonical.get()==this&&p.host->session().state_==p.state&&p.host->session().source_==p.source&&
  p.host->session().state_generation_==p.generation&&r.generation==p.generation&&world->ui_source_revision==r.world_revision&&
  r.before<=UINT64_MAX-2&&r.after==r.before+2&&p.revision==r.after&&r.city_before<UINT64_MAX&&r.game_before<UINT64_MAX&&
  p.city_info_dirty_setters==r.city_before+1&&p.game_data_dirty_setters==r.game_before+1&&
  r.city_returned&&r.game_returned&&p.city_info_dirty==true&&p.game_data_dirty==true,
  "Policy dirty SAME source/world/generation and ordered18/11 return differs");
 // Pure sealed return: no callback, imported flag inference or public ACK.
}

BnwClientGameTurnDirtyPair BnwClientSelectionControl::set_game_turn_dirty_pair(){
 require_current();auto& p=*impl_;
 const BnwClientGameTurnDirtyPair r{p.revision,p.revision+2,p.turn_timer_dirty_setters,p.game_data_dirty_setters};
 need(p.revision<=UINT64_MAX-2&&p.turn_timer_dirty_setters<UINT64_MAX&&p.game_data_dirty_setters<UINT64_MAX,
  "setGameTurn actual TurnTimer13/GameData11 capacity exhausted");
 // Finish callbackful source providers before entering the physical pair.
 require_current();
 need(p.revision==r.before&&p.turn_timer_dirty_setters==r.timer_before&&p.game_data_dirty_setters==r.game_before,
  "setGameTurn actual UI pair changed during source bookends");
 // CvGame::setGameTurn always issues these two setters, in this order.
 // Everything below is scalar, preflighted and callbackless; no draw ACK.
 ++p.turn_timer_dirty_setters;p.turn_timer_dirty=true;++p.revision;
 store_game_data_dirty_without_callbacks();
 return r;
}
bool BnwClientSelectionControl::game_turn_dirty_pair_matches_without_callbacks(const BnwClientGameTurnDirtyPair& r)const{
 require_thread();const auto& p=*impl_;
 return !p.retired.load()&&p.actual_ui_bound&&r.before<=UINT64_MAX-2&&r.after==r.before+2&&p.revision==r.after&&
  r.timer_before<UINT64_MAX&&p.turn_timer_dirty_setters==r.timer_before+1&&p.turn_timer_dirty==true&&
  r.game_before<UINT64_MAX&&p.game_data_dirty_setters==r.game_before+1&&p.game_data_dirty==true;
}
std::optional<bool> BnwClientSelectionControl::turn_timer_dirty()const{require_current();return impl_->turn_timer_dirty;}
std::uint64_t BnwClientSelectionControl::turn_timer_dirty_setter_count()const{require_current();return impl_->turn_timer_dirty_setters;}
void BnwClientSelectionControl::preflight_game_data_dirty_setter()const{
 require_current();need(impl_->revision<UINT64_MAX&&impl_->game_data_dirty_setters<UINT64_MAX,
  "Actual GameData dirty UI occurrence/revision exhausted");
}
void BnwClientSelectionControl::set_game_data_dirty(BnwClientGameDataDirtyKind kind){
 require_current();need(kind==BnwClientGameDataDirtyKind::game_data,"Actual GameData setter received another dirty bit");
 preflight_game_data_dirty_setter();
 store_game_data_dirty_without_callbacks();
}
std::optional<bool> BnwClientSelectionControl::game_data_dirty()const{require_current();return impl_->game_data_dirty;}
std::uint64_t BnwClientSelectionControl::game_data_dirty_setter_count()const{require_current();return impl_->game_data_dirty_setters;}
void BnwClientSelectionControl::preflight_score_dirty_setters(std::size_t count)const{
 require_current();need(count<=63&&count<=UINT64_MAX-impl_->revision&&
  count<=UINT64_MAX-impl_->score_dirty_setters,"Actual score dirty UI occurrence/revision exhausted");
}
void BnwClientSelectionControl::set_score_dirty(BnwClientScoreDirtyKind kind){
 require_current();need(kind==BnwClientScoreDirtyKind::score,"Actual score setter received another dirty bit");
 need(impl_->revision<UINT64_MAX&&impl_->score_dirty_setters<UINT64_MAX,
  "Actual score dirty UI occurrence/revision exhausted");
 ++impl_->score_dirty_setters; // Actual once-history BEFORE the owned flag store.
 impl_->score_dirty=true;++impl_->revision;
 // Concrete Civis-owned InterfaceDirtyBits12 storage. The selected frame
 // provider reads the SAME world; this revision invalidates old UI captures.
}
std::optional<bool> BnwClientSelectionControl::score_dirty()const{
 require_current();return impl_->score_dirty;
}
std::uint64_t BnwClientSelectionControl::score_dirty_setter_count()const{require_current();return impl_->score_dirty_setters;}
std::optional<bool> BnwClientSelectionControl::popups_suppressed()const{
 require_current();return impl_->dont_show_popups;
}
std::uint64_t BnwClientSelectionControl::popup_hold_setter_count()const{
 require_current();return impl_->popup_hold_setters;
}
void BnwClientSelectionControl::begin_initial_set_xy_popup_hold(const runtime::BnwUnitMovePending& pending){
 require_current();need(impl_->revision<UINT64_MAX&&impl_->popup_hold_setters<UINT64_MAX,
  "Actual SetDontShowPopups setter occurrence/revision exhausted");
 // Actual registered Pending authorizes this fixed native true setter, not
 // a public caller boolean. Its pure final source checks precede all stores.
 pending.require_initial_popup_store(*this);
 ++impl_->popup_hold_setters;impl_->dont_show_popups=true;++impl_->revision;
 pending.record_initial_popup_store(impl_->popup_hold_setters,impl_->revision);
}

void BnwClientSelectionControl::finish_initial_set_xy_popup_hold(const runtime::BnwUnitMovePending&pending){
 require_current();auto&p=*impl_;need(p.revision<UINT64_MAX&&p.popup_hold_setters<UINT64_MAX,
  "Actual popup release setter occurrence/revision exhausted");
 pending.require_initial_popup_release_store(*this);
 ++p.popup_hold_setters;p.dont_show_popups=false;++p.revision;
 pending.record_initial_popup_release_store(p.popup_hold_setters,p.revision);
}
std::vector<runtime::BnwSpecificCityInfoEvent>BnwClientSelectionControl::initial_garrison_dirty_events()const{
 require_current();const auto result=impl_->garrison_dirty_events;require_current();return result;
}
std::vector<runtime::BnwSpecificCityInfoEvent>BnwClientSelectionControl::take_initial_garrison_dirty_events(){
 require_current();auto result=impl_->garrison_dirty_events;
 if(result.empty())return result;
 need(impl_->revision<UINT64_MAX,"GARRISON delivery client revision exhausted");
 require_current();impl_->garrison_dirty_events.clear();++impl_->revision;return result;
}
void BnwClientSelectionControl::set_initial_garrison_dirty(const runtime::BnwUnitMovePending&pending){
 require_current();const auto facts=pending.require_initial_garrison_dirty_store(*this);auto&p=*impl_;
 need(p.revision==facts.input_revision&&p.revision<UINT64_MAX&&p.garrison_next_occurrence>0&&
  p.garrison_next_occurrence<UINT64_MAX&&p.garrison_dirty_events.size()<p.garrison_dirty_events.max_size(),
  "Actual GARRISON queue/occurrence/revision exhausted");
 const auto before=p.garrison_dirty_events;const auto occurrence=p.garrison_next_occurrence;auto prepared=before;
 prepared.push_back({occurrence,facts.generation,facts.owner,facts.city,runtime::BnwSpecificCityInfoKind::garrison});
 require_current();const auto final=pending.require_initial_garrison_dirty_store(*this);
 need(final.owner==facts.owner&&final.city==facts.city&&final.generation==facts.generation&&
  final.world_revision==facts.world_revision&&final.input_revision==facts.input_revision&&p.revision==facts.input_revision&&
  p.garrison_next_occurrence==occurrence&&p.garrison_dirty_events==before,"Actual GARRISON inputs changed while staging queue");
 // DLLUI void SetSpecificCityInfoDirty(GARRISON=4): actual canonical client
 // event store, distinct from CityInfo18 and from a renderer return.
 p.garrison_dirty_events.swap(prepared);++p.garrison_next_occurrence;++p.revision;
 pending.record_initial_garrison_dirty_return(facts.city,occurrence);
}

std::optional<std::pair<std::int32_t,std::int64_t>>BnwClientSelectionControl::initial_center_selection()const{
 require_current();need(impl_->unit.has_value()==impl_->unit_owner.has_value(),"Center canonical selected unit identity incomplete");
 if(!impl_->unit)return {};
 require_unit_identity(*impl_->state,*impl_->unit_owner,native(*impl_->unit));
 const auto result=std::pair{*impl_->unit_owner,*impl_->unit};require_current();return result;
}
std::uint64_t BnwClientSelectionControl::initial_center_revision_without_callbacks()const{
 require_thread();const auto&p=*impl_;const auto&s=p.host->session();
 need(!p.retired.load()&&p.actual_ui_bound&&s.city_selection_.lock().get()==this&&s.state_==p.state&&s.source_==p.source&&
  s.state_generation_==p.generation&&p.state->world.map_header.guid==p.plot_map_guid&&p.host->identity().rules_sha256==p.identity.rules_sha256,
  "Center actual canonical UI source controls changed");return p.revision;
}
void BnwClientSelectionControl::preflight_initial_center_info_dirty_setters(std::size_t count)const{
 require_current();const auto&p=*impl_;
 need(count<=3&&count<=UINT64_MAX-p.revision&&p.dirty_next_occurrence>0&&
  count<=UINT64_MAX-p.dirty_next_occurrence&&count<=p.unit_dirty_events.max_size()-p.unit_dirty_events.size(),
  "Center actual UnitInfo receiver ordinal/revision/capacity exhausted");
}
std::uint64_t BnwClientSelectionControl::set_initial_center_info_dirty(const runtime::BnwNativeUnitAllocatorCore&core,std::size_t setter){
 require_current();const auto facts=core.initial_center_dirty_operands(setter);auto&p=*impl_;
 need(p.revision==facts.input_revision&&p.revision<UINT64_MAX&&p.dirty_next_occurrence>0&&p.dirty_next_occurrence<UINT64_MAX&&
  p.unit_dirty_events.size()<p.unit_dirty_events.max_size(),"Center actual UnitInfo queue/revision exhausted");
 const auto before=p.unit_dirty_events;const auto ordinal=p.dirty_next_occurrence;auto staged=before;
 staged.push_back({ordinal,0,facts.generation,facts.world_revision,p.revision+1,facts.owner,facts.full_id,
  BnwClientUnitDirtyKind::unit_info,p.unit_info_dirty});
 require_current();const auto final=core.initial_center_dirty_operands(setter);
 need(final.owner==facts.owner&&final.full_id==facts.full_id&&final.generation==facts.generation&&final.world_revision==facts.world_revision&&
  final.input_revision==facts.input_revision&&initial_center_revision_without_callbacks()==facts.input_revision&&
  p.dirty_next_occurrence==ordinal&&p.unit_dirty_events==before,"Center actual UnitInfo operands changed during queue staging");
 p.unit_info_dirty=true;p.unit_dirty_events.swap(staged);++p.dirty_next_occurrence;++p.revision;
 core.record_initial_center_dirty_store(setter,ordinal);return ordinal;
}
bool BnwClientSelectionControl::initial_moves_dirty_matches_without_callbacks(const BnwClientUnitDirtySnapshot&expected)const{
 require_thread();(void)initial_center_revision_without_callbacks();const auto&p=*impl_;
 return p.unit_info_dirty==expected.unit_info&&p.plot_list_dirty==expected.plot_list_buttons&&
  p.dirty_next_occurrence==expected.next_occurrence&&p.revision==expected.client_revision&&p.unit_dirty_events==expected.events;
}
void BnwClientSelectionControl::set_initial_moves_dirty(const runtime::BnwNativeUnitAllocatorCore&core,BnwClientUnitDirtyKind kind){
 require_current();const auto facts=core.initial_moves_dirty_operands(*this,kind);auto&p=*impl_;
 need(p.revision==facts.input_revision&&p.dirty_next_occurrence==facts.occurrence&&p.revision<UINT64_MAX&&
  p.dirty_next_occurrence>0&&p.dirty_next_occurrence<UINT64_MAX&&p.unit_dirty_events.size()<p.unit_dirty_events.max_size(),
  "Initial moves actual native UI revision/queue exhausted");
 const auto selected=p.selected;const auto selected_unit=p.unit;const auto selected_owner=p.unit_owner;const auto selected_plot=p.plot;
 need(p.unit.has_value()==p.unit_owner.has_value(),"Initial moves selected identity incomplete");
 if(p.unit){need(*p.unit_owner==active_player(),"Initial moves selected actor retired");require_unit_identity(*p.state,*p.unit_owner,native(*p.unit));}
 const bool info=kind==BnwClientUnitDirtyKind::unit_info;
 if(info)need(p.unit_owner==std::optional{facts.owner}&&p.unit==std::optional<std::int64_t>{facts.full_id},"Initial moves UnitInfo actual selected identity changed");
 // The exact picked plot equality is already proven by the closed Core facts;
 // the physical actual input value is retained and rechecked around staging.
 const auto before=p.unit_dirty_events;auto prepared=before;const auto occurrence=p.dirty_next_occurrence;
 const bool previous=info?p.unit_info_dirty:p.plot_list_dirty;
 prepared.push_back({occurrence,0,facts.generation,facts.world_revision,p.revision+1,facts.owner,facts.full_id,kind,previous});
 require_current();const auto final=core.initial_moves_dirty_operands(*this,kind);
 need(final.owner==facts.owner&&final.full_id==facts.full_id&&final.generation==facts.generation&&final.world_revision==facts.world_revision&&
  final.input_revision==facts.input_revision&&final.occurrence==occurrence&&final.kind==kind&&p.revision==facts.input_revision&&
  p.dirty_next_occurrence==occurrence&&p.unit_dirty_events==before&&p.selected==selected&&p.unit==selected_unit&&p.unit_owner==selected_owner&&p.plot==selected_plot,
  "Initial moves native dirty operands/source changed during queue preparation");
 // Actual SDK setter port: native flags, ordered duplicate notifications and
 // client revision. No graphics ACK/callback or throwing getter follows stores.
 if(info)p.unit_info_dirty=true;else p.plot_list_dirty=true;
 p.unit_dirty_events.swap(prepared);++p.dirty_next_occurrence;++p.revision;
 core.record_initial_moves_ui_dirty_store(kind,occurrence);
}

std::uint64_t BnwClientSelectionControl::set_initial_best_land_dirty(const runtime::BnwNativeUnitAllocatorCore& core){
 require_current();
 // This closed Core getter authenticates the exact receiving canonical UI,
 // current Pending/source/world and earned changed Game store BEFORE effects.
 // A setMoves phase or caller-supplied dirty number cannot issue this setter.
 const auto facts=core.initial_best_land_dirty_operands(*this);auto& p=*impl_;
 need(facts.kind==BnwClientUnitDirtyKind::unit_info&&p.revision==facts.input_revision&&p.dirty_next_occurrence==facts.occurrence&&p.revision<UINT64_MAX&&
  p.dirty_next_occurrence>0&&p.dirty_next_occurrence<UINT64_MAX&&p.unit_dirty_events.size()<p.unit_dirty_events.max_size(),"Initial best-land actual UnitInfo occurrence/revision exhausted");
 const auto selected=p.selected;const auto selected_unit=p.unit;const auto selected_owner=p.unit_owner;const auto selected_plot=p.plot;
 const auto before=p.unit_dirty_events;auto prepared=before;const auto occurrence=p.dirty_next_occurrence;const bool previous=p.unit_info_dirty;
 // Native Game setter is unconditional after its genuine changed branch; no
 // selected-unit==newborn or picked-plot condition exists in this SDK call.
 prepared.push_back({occurrence,0,facts.generation,facts.world_revision,p.revision+1,facts.owner,facts.full_id,BnwClientUnitDirtyKind::unit_info,previous});
 require_current();const auto final=core.initial_best_land_dirty_operands(*this);
 need(final.owner==facts.owner&&final.full_id==facts.full_id&&final.generation==facts.generation&&final.world_revision==facts.world_revision&&
  final.input_revision==facts.input_revision&&final.occurrence==occurrence&&final.kind==BnwClientUnitDirtyKind::unit_info&&p.revision==facts.input_revision&&
  p.dirty_next_occurrence==occurrence&&p.unit_info_dirty==previous&&p.unit_dirty_events==before&&p.selected==selected&&p.unit==selected_unit&&p.unit_owner==selected_owner&&p.plot==selected_plot,
  "Initial best-land concrete UnitInfo source changed during queue preparation");
 // Stage allocation and all callbackful checks precede the actual native store.
 // The private noexcept recorder observes the prepared exact snapshot, not a
 // public caller ordinal. No renderer/display ACK or world mutation is issued.
 p.unit_info_dirty=true;p.unit_dirty_events.swap(prepared);++p.dirty_next_occurrence;++p.revision;
 core.record_initial_best_land_dirty_store(occurrence);return occurrence;
}

bool BnwClientSelectionControl::initial_unit_game_data_matches_without_callbacks(
 std::optional<bool> flag,std::uint64_t setters,std::uint64_t client_revision)const{
 require_thread();const auto revision=initial_center_revision_without_callbacks();const auto& p=*impl_;
 return revision==client_revision&&p.game_data_dirty_setters==setters&&p.game_data_dirty==flag;
}
void BnwClientSelectionControl::set_initial_unit_game_data_dirty(const runtime::BnwNativeUnitAllocatorCore& core){
 require_current();const auto facts=core.initial_unit_game_data_dirty_operands(*this);auto& p=*impl_;
 need(p.revision==facts.input_revision&&p.game_data_dirty_setters==facts.setters_before&&p.game_data_dirty==facts.flag_before&&
  p.revision<UINT64_MAX&&p.game_data_dirty_setters<UINT64_MAX,"Initial Unit actual GameData11 source/capacity differs");
 const auto selected=p.selected;const auto selected_unit=p.unit;const auto selected_owner=p.unit_owner;const auto selected_plot=p.plot;
 const auto unit_events=p.unit_dirty_events;const auto unit_next=p.dirty_next_occurrence;
 const auto unit_info=p.unit_info_dirty,plot_buttons=p.plot_list_dirty;
 const auto city_count=p.city_info_dirty_setters,policies_count=p.policies_dirty_setters;
 require_current();const auto final=core.initial_unit_game_data_dirty_operands(*this);
 need(final.owner==facts.owner&&final.full_id==facts.full_id&&final.generation==facts.generation&&final.world_revision==facts.world_revision&&
  final.input_revision==facts.input_revision&&final.setters_before==facts.setters_before&&final.flag_before==facts.flag_before&&
  p.revision==facts.input_revision&&p.game_data_dirty_setters==facts.setters_before&&p.game_data_dirty==facts.flag_before&&
  p.selected==selected&&p.unit==selected_unit&&p.unit_owner==selected_owner&&p.plot==selected_plot&&
  p.unit_dirty_events==unit_events&&p.dirty_next_occurrence==unit_next&&p.unit_info_dirty==unit_info&&p.plot_list_dirty==plot_buttons&&
  p.city_info_dirty_setters==city_count&&p.policies_dirty_setters==policies_count,
  "Initial Unit actual GameData source changed before native dirty store");
 // Actual active Init branch only. No selected-newborn condition, supplied
 // counter, callback success or renderer ACK substitutes for this store.
 store_game_data_dirty_without_callbacks();core.record_initial_unit_game_data_dirty_store();
}

std::optional<bool> BnwClientSelectionControl::initial_unit_maintenance_city_info_dirty()const{
 require_current();return impl_->city_info_dirty;
}
std::uint64_t BnwClientSelectionControl::initial_unit_maintenance_city_info_setter_count()const{
 require_current();return impl_->city_info_dirty_setters;
}
bool BnwClientSelectionControl::initial_unit_maintenance_matches_without_callbacks(
 std::optional<bool> city_flag,std::uint64_t city_count,std::optional<bool> game_flag,std::uint64_t game_count,std::uint64_t client_revision)const{
 require_thread();const auto revision=initial_center_revision_without_callbacks();const auto& p=*impl_;
 return revision==client_revision&&p.city_info_dirty==city_flag&&p.city_info_dirty_setters==city_count&&p.game_data_dirty==game_flag&&p.game_data_dirty_setters==game_count;
}
void BnwClientSelectionControl::set_initial_unit_maintenance_dirty(const runtime::BnwNativeUnitAllocatorCore& core){
 using Kind=runtime::BnwNativeUnitAllocatorCore::InitialUnitMaintenanceDirtyKind;
 require_current();const auto first=core.initial_unit_maintenance_dirty_operands(*this);auto& p=*impl_;
 need(first.kind==Kind::city_info&&p.revision==first.input_revision&&p.city_info_dirty==first.city_flag&&p.game_data_dirty==first.game_flag&&
  p.city_info_dirty_setters==first.city_count&&p.game_data_dirty_setters==first.game_count&&p.revision<=UINT64_MAX-2&&first.city_count<UINT64_MAX&&first.game_count<UINT64_MAX,
  "Init maintenance actual paired dirty phase/source/capacity differs");
 const auto selected=p.selected;const auto selected_unit=p.unit;const auto selected_owner=p.unit_owner;const auto selected_plot=p.plot;
 const auto unit_events=p.unit_dirty_events;const auto unit_next=p.dirty_next_occurrence;
 const auto unit_info=p.unit_info_dirty,plot_buttons=p.plot_list_dirty;
 const auto policy_flag=p.policies_dirty;const auto policy_count=p.policies_dirty_setters;
 for(unsigned step=0;step<2;++step){
  const auto kind=step==0?Kind::city_info:Kind::game_data;
  require_current();const auto facts=core.initial_unit_maintenance_dirty_operands(*this);
  need(facts.kind==kind&&facts.owner==first.owner&&facts.full_id==first.full_id&&facts.generation==first.generation&&facts.world_revision==first.world_revision&&
   facts.input_revision==first.input_revision+step&&facts.city_count==first.city_count+step&&facts.game_count==first.game_count&&
   facts.city_flag==(step==0?first.city_flag:std::optional<bool>{true})&&facts.game_flag==first.game_flag,
   "Init maintenance actual receiving Core ordered dirty facts differ");
  require_current();const auto final=core.initial_unit_maintenance_dirty_operands(*this);
  need(final.owner==facts.owner&&final.full_id==facts.full_id&&final.generation==facts.generation&&final.world_revision==facts.world_revision&&
   final.input_revision==facts.input_revision&&final.kind==facts.kind&&final.city_flag==facts.city_flag&&final.game_flag==facts.game_flag&&
   final.city_count==facts.city_count&&final.game_count==facts.game_count&&
   p.revision==facts.input_revision&&p.city_info_dirty==facts.city_flag&&p.game_data_dirty==facts.game_flag&&
   p.city_info_dirty_setters==facts.city_count&&p.game_data_dirty_setters==facts.game_count&&
   p.selected==selected&&p.unit==selected_unit&&p.unit_owner==selected_owner&&p.plot==selected_plot&&
   p.unit_dirty_events==unit_events&&p.dirty_next_occurrence==unit_next&&p.unit_info_dirty==unit_info&&p.plot_list_dirty==plot_buttons&&
   p.policies_dirty==policy_flag&&p.policies_dirty_setters==policy_count,
   "Init maintenance source changed before genuine CityInfo18/GameData11 store");
  // The prepared real Core recorder advances its own typed phase only AFTER
  // each actual scalar setter. No callback/count/selection ACK is supplied.
  if(kind==Kind::city_info)store_policy_city_info_dirty_without_callbacks();
  else store_game_data_dirty_without_callbacks();
  core.record_initial_unit_maintenance_dirty_store(kind);
 }
 need(initial_unit_maintenance_matches_without_callbacks(true,first.city_count+1,true,first.game_count+1,first.input_revision+2),
  "Init maintenance actual ordered dirty return differs");
}

std::shared_ptr<const SelectedBnwPreviewHost> BnwClientSelectionControl::retained_host_for_tech_transport()const{
 // Immutable creator check BEFORE touching mutable source/canonical handles.
 require_thread();
 const auto host=impl_->host;const auto state=impl_->state;const auto source=impl_->source;
 const auto generation=impl_->generation;
 need(bool(host),"Technology client canonical Host absent");
 require_current();
 need(impl_->host.get()==host.get()&&!impl_->host.owner_before(host)&&!host.owner_before(impl_->host)&&
  impl_->state==state&&impl_->source==source&&impl_->generation==generation,
  "Technology client canonical source changed during provider callback");
 require_current();return host;
}
void BnwClientSelectionControl::retire()noexcept{impl_->retired.store(true);}
std::shared_ptr<BnwClientSelectionControl> BnwClientSelectionControl::create(
 std::shared_ptr<const SelectedBnwPreviewHost> host){
 auto p=std::make_unique<Impl>();need(bool(host),"Actual selected client Host absent");
 p->host=std::move(host);
 const auto& session=p->host->session();
 need(session.plot_yield_refresh_thread_==p->thread,"Client selection session belongs to another thread");
 p->rules_current=p->host->database().immutable_preview_fingerprint_currentness();
 need(p->rules_current&&p->rules_current(),"Client selection actual immutable rules retired");
 need(session.bnw_game_debug_owner_current(),"Client selection genuine imported Game unavailable");
 p->identity=session.debug_->identity;
 need(p->identity.payload_sha256==p->host->identity().payload_sha256&&
  p->identity.rules_sha256==p->host->identity().rules_sha256&&
  p->host->database().gameplay_fingerprint_sha256()==p->identity.rules_sha256,
  "Client selection SAME Host/import/rules identity differs");
 (void)session.bnw_game_debug_mode();
 p->state=session.state_;p->source=session.source_;p->generation=session.state_generation_;
 p->plot_map_guid=p->state->world.map_header.guid;
 if(p->state->world.map_plots)p->plot_storage=p->state->world.map_plots->plots.data();
 if(p->state->world.map_projection)p->projection_storage=p->state->world.map_projection->cells.data();
 if(const auto previous=session.city_selection_.lock())
  need(previous->impl_->retired.load(),"Session already has an actual client selection owner");
 auto result=std::shared_ptr<BnwClientSelectionControl>(new BnwClientSelectionControl(std::move(p)));
 result->require_current(false);session.city_selection_=result;
 return result;
}
void BnwClientSelectionControl::require_retained_cycle_session(const runtime::LiveSession& session)const{
 require_current();need(&impl_->host->session()==&session,
  "Retained cycle receiver belongs to another actual session");
 need(session.city_selection_.lock().get()==this&&session.bnw_game_debug_owner_current(),
  "Retained cycle canonical UI/Game registration retired");
 (void)session.bnw_game_debug_mode();require_current();
}
void BnwClientSelectionControl::reset_no_selection_list_cycle(){
 require_current();need(impl_->revision<UINT64_MAX,"Retained cycle UI revision exhausted");
 // Cursor has already retained its once attempt before these nonthrowing stores.
 impl_->no_selection_list_cycle=false;++impl_->revision;
 require_current();
}
bool BnwClientSelectionControl::no_selection_list_cycle()const{
 require_current();need(impl_->no_selection_list_cycle.has_value(),
  "NoSelectionListCycle original import value is unknown before actual setter");
 const bool value=*impl_->no_selection_list_cycle;require_current();return value;
}
void BnwClientSelectionControl::bind_actual_ui_host(){
 require_current(false);need(!impl_->actual_ui_bound,"Actual client UI selection owner already bound");
 impl_->actual_ui_bound=true;
}
void BnwClientSelectionControl::require_thread()const{
 need(impl_->thread==std::this_thread::get_id(),"Client selection belongs to another constructor thread");
}
void BnwClientSelectionControl::require_current(bool ui)const{
 const auto& p=*impl_;
 need(p.thread==std::this_thread::get_id(),"Client selection belongs to another constructor thread");
 need(!p.retired.load()&&p.host&&(!ui||p.actual_ui_bound),"Actual client selection input owner absent/retired/unbound");
 need(p.rules_current&&p.rules_current(),"Client selection SAME immutable rules retired");
 const auto& session=p.host->session();
 need(session.bnw_game_debug_owner_current()&&session.state_==p.state&&session.source_==p.source&&
  session.state_generation_==p.generation,"Client selection actual Game/source/generation retired");
 need(session.debug_->identity.payload_sha256==p.identity.payload_sha256&&
  session.debug_->identity.rules_sha256==p.identity.rules_sha256,
  "Client selection actual imported identity changed");
 (void)session.bnw_game_debug_mode();
 need(p.state->world.map_header.guid==p.plot_map_guid&&p.state->world.map_plots&&
  p.state->world.map_projection&&p.state->world.map_plots->plots.data()==p.plot_storage&&
  p.state->world.map_projection->cells.data()==p.projection_storage,
  "Client selection actual map allocation/GUID retired");
 // ui_source_revision intentionally is not an input-storage retirement key:
 // authenticated same-world gameplay stores invalidate frames, not input.
}
std::uint64_t BnwClientSelectionControl::revision()const{require_current();return impl_->revision;}
std::uint64_t BnwClientSelectionControl::world_revision()const{require_current();return impl_->state->ui_source_revision;}
std::int32_t BnwClientSelectionControl::active_player()const{
 require_current(false);const auto n=runtime::detail::bnw_game_active_player_read(*impl_->state);require_current(false);return n;
}
std::int32_t BnwClientSelectionControl::active_team()const{
 require_current();const auto n=runtime::detail::bnw_game_active_team_read(*impl_->state);require_current();return n;
}
std::int32_t BnwClientSelectionControl::city_team(std::int32_t owner,std::int32_t city)const{
 require_current();need(runtime::resolve_bnw_city_registry(impl_->state->world,impl_->state->cities,owner,city),
  "Client selection actual city team identity unavailable");
 const auto team=runtime::detail::bnw_player_team_read(*impl_->state,owner);require_current();return team;
}
std::optional<std::pair<std::int64_t,std::int64_t>> BnwClientSelectionControl::city()const{
 require_current();const auto selected=impl_->selected;
 if(selected)need(runtime::resolve_bnw_city_registry(impl_->state->world,impl_->state->cities,
  native(selected->first),native(selected->second)),"Actual selected city is no longer current");
 require_current();return selected;
}
void BnwClientSelectionControl::select(std::optional<std::pair<std::int64_t,std::int64_t>> selected){
 require_current();select_input(selected,impl_->unit);
}
void BnwClientSelectionControl::select_input(std::optional<std::pair<std::int64_t,std::int64_t>> selected,
 std::optional<std::int64_t> unit){
 require_current();
 if(selected){
  const auto owner=native(selected->first),handle=native(selected->second);
  need(owner==active_player(),"Actual client selection is not owned by current active Game player");
  const auto* value=runtime::resolve_bnw_city_yield_registry(impl_->state->world,impl_->state->cities,owner,handle);
  need(value,"Actual client selection city handle unavailable");
  selected=std::pair<std::int64_t,std::int64_t>{owner,value->id};
 }
 std::optional<std::int32_t> unit_owner;
 if(unit){
  unit_owner=active_player();require_unit_identity(*impl_->state,*unit_owner,native(*unit));
 }
 if(selected==impl_->selected&&unit==impl_->unit&&unit_owner==impl_->unit_owner)return;
 need(impl_->revision!=UINT64_MAX,"Actual selection revision exhausted");
 require_current();impl_->selected=selected;impl_->unit=unit;impl_->unit_owner=unit_owner;++impl_->revision;
}
bool BnwClientSelectionControl::is_selected(std::int32_t owner,std::int32_t handle)const{
 require_current();const auto* candidate=runtime::resolve_bnw_city_yield_registry(
  impl_->state->world,impl_->state->cities,owner,handle);
 need(candidate,"Native isCitySelected actual canonical city absent");
 const auto selected=city();const bool result=selected&&selected->first==owner&&selected->second==candidate->id;
 require_current();return result;
}
std::optional<std::int64_t> BnwClientSelectionControl::unit()const{
 require_current();const auto unit=impl_->unit;
 need(unit.has_value()==impl_->unit_owner.has_value(),"Canonical selected unit identity incomplete");
 if(unit){
  need(*impl_->unit_owner==active_player(),"Canonical selected unit actor differs");
  require_unit_identity(*impl_->state,*impl_->unit_owner,native(*unit));
 }
 require_current();return unit;
}
bool BnwClientSelectionControl::is_unit_selected(std::int32_t owner,std::int32_t full_id)const{
 require_current();require_unit_identity(*impl_->state,owner,full_id);
 need(impl_->unit.has_value()==impl_->unit_owner.has_value(),"Actual selected unit input identity incomplete");
 if(impl_->unit){
  need(*impl_->unit_owner==active_player(),"Actual selected unit input actor retired");
  require_unit_identity(*impl_->state,*impl_->unit_owner,native(*impl_->unit));
 }
 const bool result=impl_->unit&&*impl_->unit_owner==owner&&*impl_->unit==full_id;
 require_current();return result;
}
void BnwClientSelectionControl::select_plot(std::optional<std::int64_t> plot){
 require_current();require_plot_identity(*impl_->state,plot);
 const std::optional<std::int32_t> canonical=plot?std::optional{native(*plot)}:std::nullopt;
 if(canonical==impl_->plot)return;
 need(impl_->revision!=UINT64_MAX,"Actual selection revision exhausted");
 require_current();impl_->plot=canonical;++impl_->revision;
}
std::optional<std::uint64_t> BnwClientSelectionControl::selection_plot()const{
 require_current();const auto selected=impl_->plot;
 require_plot_identity(*impl_->state,selected?std::optional<std::int64_t>{*selected}:std::nullopt);
 require_current();return selected?std::optional{static_cast<std::uint64_t>(*selected)}:std::nullopt;
}
BnwClientUnitDirtySnapshot BnwClientSelectionControl::unit_dirty_state()const{
 require_current();const auto& p=*impl_;
 for(const auto& e:p.unit_dirty_events)need(e.generation==p.generation&&e.occurrence>0&&
  e.occurrence<p.dirty_next_occurrence,"Actual unit dirty occurrence/generation differs");
 BnwClientUnitDirtySnapshot out{p.unit_info_dirty,p.plot_list_dirty,p.dirty_next_occurrence,p.revision,p.unit_dirty_events};
 require_current();return out;
}
std::vector<BnwClientUnitDirtyEvent> BnwClientSelectionControl::take_unit_dirty_events(){
 require_current();auto result=unit_dirty_state().events;
 if(result.empty())return result;
 need(impl_->revision<UINT64_MAX,"Unit dirty transfer client revision exhausted");
 require_current();impl_->unit_dirty_events.clear();++impl_->revision;
 // Transfer is only owned event delivery. Flags stay true: there is no caller
 // reset/renderer acknowledgment and no restored engine fanout claim.
 return result;
}
std::uint64_t BnwClientSelectionControl::set_initial_newborn_info_dirty(const runtime::BnwUnitMovePending&pending){
 require_current();const auto facts=pending.require_initial_unit_info_dirty_store(*this);auto&p=*impl_;
 const auto selected=p.selected;const auto selected_unit=p.unit;const auto selected_owner=p.unit_owner;const auto selected_plot=p.plot;
 need(p.unit.has_value()==p.unit_owner.has_value(),"Initial UnitInfo canonical selected identity incomplete");
 if(p.unit){need(*p.unit_owner==active_player(),"Initial UnitInfo actual selected actor retired");
  require_unit_identity(*p.state,*p.unit_owner,native(*p.unit));}
 // IsSelected compares the actual canonical client selection. A new native
 // node is NOT looked up as an already published raw/owned roster unit.
 need(!(p.unit&&*p.unit_owner==facts.owner&&*p.unit==facts.full_id),
  "Mandatory genuine initial selected minimap receiver required before UnitInfo");
 need(p.revision==facts.input_revision&&p.revision<UINT64_MAX&&p.dirty_next_occurrence<UINT64_MAX&&
  p.unit_dirty_events.size()<p.unit_dirty_events.max_size(),"Initial UnitInfo queue/revision exhausted or stale");
 const bool previous=p.unit_info_dirty;auto prepared=p.unit_dirty_events;
 prepared.push_back({p.dirty_next_occurrence,facts.occurrence,facts.generation,facts.world_revision,
  p.revision+1,facts.owner,facts.full_id,BnwClientUnitDirtyKind::unit_info,previous});
 require_current();const auto final=pending.require_initial_unit_info_dirty_store(*this);
 need(final.owner==facts.owner&&final.full_id==facts.full_id&&final.occurrence==facts.occurrence&&
  final.generation==facts.generation&&final.world_revision==facts.world_revision&&final.input_revision==facts.input_revision&&
  p.revision==facts.input_revision&&p.selected==selected&&p.unit==selected_unit&&p.unit_owner==selected_owner&&p.plot==selected_plot,
  "Initial UnitInfo canonical input changed during queue staging");
 // Native setInfoBarDirty(true) is unconditional UnitInfo_DIRTY_BIT19. This
 // real Civis queue/flag store needs no synchronous GPU ACK; all allocating
 // getters and genuine source checks have finished before these no-throw stores.
 p.unit_info_dirty=true;p.unit_dirty_events.swap(prepared);++p.dirty_next_occurrence;++p.revision;
 pending.record_initial_unit_info_dirty_store(p.dirty_next_occurrence-1);return p.revision;
}
std::uint64_t BnwClientSelectionControl::set_moves_dirty(const runtime::BnwUnitMovePending& pending){
 require_current();const auto facts=pending.require_dirty_store(*this);
 require_unit_identity(*impl_->state,facts.owner,facts.full_id);
 auto& p=*impl_;const auto selected=p.selected;const auto selected_unit=p.unit;
 const auto selected_owner=p.unit_owner;const auto selected_plot=p.plot;
 need(p.revision==facts.input_revision,"Unit dirty input revision differs");
 if(facts.kind==BnwClientUnitDirtyKind::unit_info)need(p.unit_owner==std::optional{facts.owner}&&
  p.unit==std::optional<std::int64_t>{facts.full_id},"UnitInfo dirty canonical unit input differs");
 else need(p.plot==std::optional{facts.before_plot},"PlotList dirty canonical picked plot differs");
 const bool previous=facts.kind==BnwClientUnitDirtyKind::unit_info?p.unit_info_dirty:p.plot_list_dirty;
 need(p.revision<UINT64_MAX&&p.dirty_next_occurrence<UINT64_MAX,
  "Actual unit dirty client revision/ordinal exhausted");
 need(p.unit_dirty_events.size()<p.unit_dirty_events.max_size(),"Actual unit dirty queue capacity exhausted");
 auto prepared=p.unit_dirty_events;
 prepared.push_back({p.dirty_next_occurrence,facts.occurrence,facts.generation,facts.world_revision,
  p.revision+1,facts.owner,facts.full_id,facts.kind,previous});
 require_current();need(p.revision==facts.input_revision&&p.selected==selected&&p.unit==selected_unit&&
  p.unit_owner==selected_owner&&p.plot==selected_plot,"Unit dirty canonical input changed during preflight");
 need(pending.require_dirty_store(*this).kind==facts.kind,
  "Actual unit dirty native phase changed before store");
 // All allocations/currentness checks precede the effect. This native client
 // setDirty port has no callback; flags, queue, revision and Pending once facts
 // are published without throwing. Duplicate true notifications remain ordered.
 if(facts.kind==BnwClientUnitDirtyKind::unit_info)p.unit_info_dirty=true;
 else p.plot_list_dirty=true;
 p.unit_dirty_events.swap(prepared);++p.dirty_next_occurrence;++p.revision;
 pending.record_dirty_store(facts.kind,p.dirty_next_occurrence-1);
 require_current();pending.require_same_registered_pending(p.host.get());
 return p.revision;
}
std::vector<runtime::BnwCityDirtyEvent> BnwClientSelectionControl::take_events(){
 require_current();auto& state=*impl_->host->session().state_;
 // A restored checkpoint retains old generation values. It cannot silently
 // reissue notifications already consumed by the old external UI owner.
 for(const auto& event:state.city_dirty_events)
  need(event.generation==impl_->generation,"Checkpoint UI notifications require actual generation reconciliation");
 auto result=state.city_dirty_events;require_current();state.city_dirty_events.clear();return result;
}
}

namespace civ5::ui {
void BnwClientSelectionControl::preflight_league_screen_dirty_setter()const{
 require_current();need(impl_->revision<UINT64_MAX&&impl_->league_screen_dirty_setters<UINT64_MAX,
  "Actual LeagueScreen UI occurrence/revision exhausted");
}
void BnwClientSelectionControl::set_league_screen_dirty(BnwClientLeagueDirtyKind kind){
 require_current();need(kind==BnwClientLeagueDirtyKind::league_screen,"Actual LeagueScreen setter received another dirty bit");
 preflight_league_screen_dirty_setter();
 ++impl_->league_screen_dirty_setters;impl_->league_screen_dirty=true;++impl_->revision;
}
std::optional<bool> BnwClientSelectionControl::league_screen_dirty()const{require_current();return impl_->league_screen_dirty;}
std::uint64_t BnwClientSelectionControl::league_screen_dirty_setter_count()const{require_current();return impl_->league_screen_dirty_setters;}
}

namespace civ5::ui {
BnwClientSelectionControl::AttritionCityInfoState BnwClientSelectionControl::capture_attrition_city_info_without_callbacks()const{
 require_thread();const auto& p=*impl_;
 need(!p.retired.load()&&p.actual_ui_bound&&p.host&&p.state,"Attrition CityInfo canonical Selection unavailable");
 return {p.revision,p.city_info_dirty_setters,p.generation,p.city_info_dirty,p.host.get(),p.state.get(),p.source.get()};
}
bool BnwClientSelectionControl::attrition_city_info_state_matches_without_callbacks(const AttritionCityInfoState& s,bool after)const{
 require_thread();const auto& p=*impl_;
 const auto canonical=p.host->session().city_selection_.lock();
 return !p.retired.load()&&p.actual_ui_bound&&canonical.get()==this&&p.host.get()==s.host&&p.state.get()==s.state&&p.source.get()==s.source&&
  p.generation==s.generation&&p.host->session().state_==p.state&&p.host->session().source_==p.source&&p.host->session().state_generation_==s.generation&&
  s.revision<UINT64_MAX&&s.city_count<UINT64_MAX&&p.revision==s.revision+(after?1U:0U)&&
  p.city_info_dirty_setters==s.city_count+(after?1U:0U)&&p.city_info_dirty==(after?std::optional<bool>{true}:s.flag);
}
}

namespace civ5::ui {
void BnwClientSelectionControl::consume_attrition_city_info_dirty(
 const runtime::BnwAttritionCityInfoDirtyLoan& loan,runtime::LiveSessionState& state,
 const runtime::BnwStockLuaAttemptLease& lease){
 loan.require_alive_without_callbacks();
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id(),"Attrition CityInfo foreign Selection creator");
 try{
  require_current();loan.require_alive_without_callbacks();
  loan.require_setter_source(*this,*p.host,state,lease);loan.require_alive_without_callbacks();
  const auto facts=loan.operands_without_callbacks();
  need(p.state.get()==&state&&p.generation==facts.generation&&state.ui_source_revision==facts.world_revision&&
   p.revision==facts.client_before&&p.city_info_dirty_setters==facts.city_count_before&&
   facts.client_before<UINT64_MAX&&facts.city_count_before<UINT64_MAX,
   "Attrition CityInfo actual Selection/source/capacity differs");
  // No city/team selection predicate: updateStrengthValue always calls 18.
  // Finish callbackful admission before this exact callbackless store seam.
  require_current();loan.require_alive_without_callbacks();
  loan.require_setter_source(*this,*p.host,state,lease);loan.require_alive_without_callbacks();
  loan.require_final_setter_without_callbacks(*this,*p.host,state,lease,p.revision,p.city_info_dirty_setters);
  store_policy_city_info_dirty_without_callbacks();
  // Do NOT increment world revision, synthesize a queued event, clear dirty,
  // or wait for/render a frame. This is only physical DLLUI setter return.
  loan.record_setter_and_adopt_without_callbacks(*this,p.revision,p.city_info_dirty_setters);
  loan.require_return_without_callbacks(*this,*p.host,state,p.revision,p.city_info_dirty_setters);
 }catch(...){loan.fail_without_callbacks();throw;}
}
void BnwClientSelectionControl::require_attrition_city_info_dirty_return(
 const runtime::BnwAttritionCityInfoDirtyLoan& loan)const{
 loan.require_alive_without_callbacks();
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.actual_ui_bound&&p.city_info_dirty==true,
  "Attrition CityInfo Selection retired or actual flag differs");
 loan.require_return_without_callbacks(*this,*p.host,*p.state,p.revision,p.city_info_dirty_setters);
}
}

namespace civ5::ui {
BnwClientSelectionControl::AttritionUnitInfoState BnwClientSelectionControl::capture_attrition_unit_info_without_callbacks()const{
 require_thread();const auto&p=*impl_;
 need(!p.retired.load()&&p.actual_ui_bound&&p.host&&p.state,"UnitInfo canonical Selection unavailable");
 return {{p.unit_info_dirty,p.plot_list_dirty,p.dirty_next_occurrence,p.revision,p.unit_dirty_events},p.city_info_dirty,p.city_info_dirty_setters,
  p.generation,p.host.get(),p.state.get(),p.source.get(),p.selected,p.unit,p.unit_owner,p.plot};
}
bool BnwClientSelectionControl::attrition_unit_info_state_matches_without_callbacks(const AttritionUnitInfoState&s,
 const BnwClientUnitDirtyEvent&event,bool after)const{
 require_thread();const auto&p=*impl_;const auto canonical=p.host->session().city_selection_.lock();
 if(p.retired.load()||!p.actual_ui_bound||canonical.get()!=this||p.host.get()!=s.host||p.state.get()!=s.state||p.source.get()!=s.source||
  p.generation!=s.generation||p.host->session().state_!=p.state||p.host->session().source_!=p.source||p.host->session().state_generation_!=s.generation||
  p.selected!=s.selected||p.unit!=s.selected_unit||p.unit_owner!=s.selected_owner||p.plot!=s.selected_plot||
  p.city_info_dirty!=s.city_flag||p.city_info_dirty_setters!=s.city_count||p.plot_list_dirty!=s.unit.plot_list_buttons||
  s.unit.client_revision==UINT64_MAX||s.unit.next_occurrence==0||s.unit.next_occurrence==UINT64_MAX||
  p.revision!=s.unit.client_revision+(after?1U:0U)||p.dirty_next_occurrence!=s.unit.next_occurrence+(after?1U:0U)||
  p.unit_info_dirty!=(after?true:s.unit.unit_info))return false;
 if(!after)return p.unit_dirty_events==s.unit.events;
 return s.unit.events.size()<s.unit.events.max_size()&&p.unit_dirty_events.size()==s.unit.events.size()+1&&
  std::equal(s.unit.events.begin(),s.unit.events.end(),p.unit_dirty_events.begin())&&p.unit_dirty_events.back()==event;
}
void BnwClientSelectionControl::consume_attrition_unit_info_dirty(const runtime::BnwAttritionUnitInfoDirtyLoan&loan,
 runtime::LiveSessionState&state,const runtime::BnwStockLuaAttemptLease&lease){
 loan.require_alive_without_callbacks();auto&p=*impl_;need(p.thread==std::this_thread::get_id(),"UnitInfo foreign Selection creator");
 try{
  require_current();loan.require_alive_without_callbacks();loan.require_setter_source(*this,*p.host,state,lease);loan.require_alive_without_callbacks();
  const auto facts=loan.operands_without_callbacks();const auto event=loan.event_without_callbacks();
  need(p.state.get()==&state&&p.generation==facts.generation&&state.ui_source_revision==facts.world_revision&&
   p.revision==facts.client_before&&p.dirty_next_occurrence==facts.ordinal_before&&p.revision<UINT64_MAX&&p.dirty_next_occurrence>0&&
   p.dirty_next_occurrence<UINT64_MAX&&p.unit_dirty_events.size()<p.unit_dirty_events.max_size(),"UnitInfo actual source/client/journal capacity differs");
  auto prepared=p.unit_dirty_events;prepared.push_back(event);
  // Allocation and callbackful currentness finish BEFORE exact pure seal.
  require_current();loan.require_alive_without_callbacks();loan.require_setter_source(*this,*p.host,state,lease);loan.require_alive_without_callbacks();
  loan.require_final_setter_without_callbacks(*this,*p.host,state,lease,p.revision,p.dirty_next_occurrence);
  static_assert(noexcept(p.unit_dirty_events.swap(prepared)));
  p.unit_info_dirty=true;p.unit_dirty_events.swap(prepared);++p.dirty_next_occurrence;++p.revision;
  loan.record_setter_and_adopt_without_callbacks(*this,p.revision,p.dirty_next_occurrence);
  loan.require_return_without_callbacks(*this,*p.host,state,p.revision,p.dirty_next_occurrence);
 }catch(...){loan.fail_without_callbacks();throw;}
}
void BnwClientSelectionControl::require_attrition_unit_info_dirty_return(const runtime::BnwAttritionUnitInfoDirtyLoan&loan)const{
 loan.require_alive_without_callbacks();const auto&p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.actual_ui_bound&&p.unit_info_dirty,
  "UnitInfo real Selection return absent/retired");loan.require_return_without_callbacks(*this,*p.host,*p.state,p.revision,p.dirty_next_occurrence);
}
}

namespace civ5::ui {
void BnwClientSelectionControl::require_attrition_unit_selected_current(const runtime::BnwAttritionUnitSelectedRead&loan)const{
 // This child cannot call require_current()/active_player() as opaque helpers:
 // each callback is followed by immutable creator/facade FIRST before raw use.
 loan.require_alive_without_callbacks();const auto&p=*impl_;
 need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.host&&p.actual_ui_bound,"IsSelected canonical input owner retired/unbound");
 need(bool(p.rules_current),"IsSelected immutable rules callback absent");const bool rules=p.rules_current();loan.require_alive_without_callbacks();
 need(rules&&!p.retired.load(),"IsSelected SAME immutable rules/input retired");
 const auto&session=p.host->session();const bool current=session.bnw_game_debug_owner_current();loan.require_alive_without_callbacks();
 need(!p.retired.load()&&current&&session.state_==p.state&&session.source_==p.source&&session.state_generation_==p.generation,
  "IsSelected canonical Game/source/generation retired");
 need(session.debug_->identity.payload_sha256==p.identity.payload_sha256&&session.debug_->identity.rules_sha256==p.identity.rules_sha256,
  "IsSelected actual imported identity changed");
 (void)session.bnw_game_debug_mode();loan.require_alive_without_callbacks();
 need(!p.retired.load()&&p.state->world.map_header.guid==p.plot_map_guid&&p.state->world.map_plots&&p.state->world.map_projection&&
  p.state->world.map_plots->plots.data()==p.plot_storage&&p.state->world.map_projection->cells.data()==p.projection_storage,
  "IsSelected actual map allocation/GUID retired");
}
bool BnwClientSelectionControl::consume_attrition_unit_selected_read(const runtime::BnwAttritionUnitSelectedRead&loan,
 const runtime::LiveSessionState&state,const runtime::BnwStockLuaAttemptLease&lease)const{
 loan.require_alive_without_callbacks();const auto&p=*impl_;need(p.thread==std::this_thread::get_id(),"IsSelected foreign input creator");
 try{
  require_attrition_unit_selected_current(loan);loan.require_query_source(*this,*p.host,state,lease);loan.require_alive_without_callbacks();
  const auto facts=loan.operands_without_callbacks();need(p.state.get()==&state&&p.generation==facts.generation&&state.ui_source_revision==facts.world_revision,
   "IsSelected actual current Source/world differs");
  require_unit_identity(state,facts.unit_owner,facts.unit_full_id);
  need(p.unit.has_value()==p.unit_owner.has_value(),"IsSelected actual input membership identity incomplete");
  if(p.unit){
   require_attrition_unit_selected_current(loan);
   const auto active=runtime::detail::bnw_game_active_player_read(state);
   require_attrition_unit_selected_current(loan);loan.require_query_source(*this,*p.host,state,lease);loan.require_alive_without_callbacks();
   need(*p.unit_owner==active,"IsSelected actual selected owner differs from genuine active Game player");
   require_unit_identity(state,*p.unit_owner,native(*p.unit));
  }
  const bool result=p.unit&&*p.unit_owner==facts.unit_owner&&*p.unit==facts.unit_full_id;
  require_attrition_unit_selected_current(loan);loan.require_query_source(*this,*p.host,state,lease);loan.require_alive_without_callbacks();
  loan.require_final_query_without_callbacks(*this,*p.host,state,lease,result);
  // The actual read return/branch is adopted before any Engine callbackful POST.
  loan.record_query_and_adopt_without_callbacks(*this,result);
  loan.require_return_without_callbacks(*this,*p.host,state,result);return result;
 }catch(...){loan.fail_without_callbacks();throw;}
}
void BnwClientSelectionControl::require_attrition_unit_selected_read_return(const runtime::BnwAttritionUnitSelectedRead&loan)const{
 loan.require_alive_without_callbacks();const auto&p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.actual_ui_bound,
  "IsSelected actual input return retired");loan.require_return_without_callbacks(*this,*p.host,*p.state,loan.result_without_callbacks());
}
}

namespace civ5::ui {
void BnwClientSelectionControl::require_attrition_selected_unit_info_current(const runtime::BnwAttritionSelectedUnitInfoDirtyLoan&loan)const{
 // This child cannot call require_current()/active_player() as opaque helpers:
 // each callback is followed by immutable creator/facade FIRST before raw use.
 loan.require_alive_without_callbacks();const auto&p=*impl_;
 need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.host&&p.actual_ui_bound,"IsSelected canonical input owner retired/unbound");
 need(bool(p.rules_current),"IsSelected immutable rules callback absent");const bool rules=p.rules_current();loan.require_alive_without_callbacks();
 need(rules&&!p.retired.load(),"IsSelected SAME immutable rules/input retired");
 const auto&session=p.host->session();const bool current=session.bnw_game_debug_owner_current();loan.require_alive_without_callbacks();
 need(!p.retired.load()&&current&&session.state_==p.state&&session.source_==p.source&&session.state_generation_==p.generation,
  "IsSelected canonical Game/source/generation retired");
 need(session.debug_->identity.payload_sha256==p.identity.payload_sha256&&session.debug_->identity.rules_sha256==p.identity.rules_sha256,
  "IsSelected actual imported identity changed");
 (void)session.bnw_game_debug_mode();loan.require_alive_without_callbacks();
 need(!p.retired.load()&&p.state->world.map_header.guid==p.plot_map_guid&&p.state->world.map_plots&&p.state->world.map_projection&&
  p.state->world.map_plots->plots.data()==p.plot_storage&&p.state->world.map_projection->cells.data()==p.projection_storage,
  "IsSelected actual map allocation/GUID retired");
}
void BnwClientSelectionControl::consume_attrition_selected_unit_info_dirty(const runtime::BnwAttritionSelectedUnitInfoDirtyLoan&loan,
 runtime::LiveSessionState&state,const runtime::BnwStockLuaAttemptLease&lease){
 loan.require_alive_without_callbacks();auto&p=*impl_;need(p.thread==std::this_thread::get_id(),"UnitInfo foreign Selection creator");
 try{
  require_attrition_selected_unit_info_current(loan);loan.require_alive_without_callbacks();loan.require_setter_source(*this,*p.host,state,lease);loan.require_alive_without_callbacks();
  const auto facts=loan.operands_without_callbacks();const auto event=loan.event_without_callbacks();
  need(p.state.get()==&state&&p.generation==facts.generation&&state.ui_source_revision==facts.world_revision&&
   p.revision==facts.client_before&&p.dirty_next_occurrence==facts.ordinal_before&&p.revision<UINT64_MAX&&p.dirty_next_occurrence>0&&
   p.dirty_next_occurrence<UINT64_MAX&&p.unit_dirty_events.size()<p.unit_dirty_events.max_size(),"UnitInfo actual source/client/journal capacity differs");
  auto prepared=p.unit_dirty_events;prepared.push_back(event);
  // Allocation and callbackful currentness finish BEFORE exact pure seal.
  require_attrition_selected_unit_info_current(loan);loan.require_alive_without_callbacks();loan.require_setter_source(*this,*p.host,state,lease);loan.require_alive_without_callbacks();
  loan.require_final_setter_without_callbacks(*this,*p.host,state,lease,p.revision,p.dirty_next_occurrence);
  static_assert(noexcept(p.unit_dirty_events.swap(prepared)));
  p.unit_info_dirty=true;p.unit_dirty_events.swap(prepared);++p.dirty_next_occurrence;++p.revision;
  loan.record_setter_and_adopt_without_callbacks(*this,p.revision,p.dirty_next_occurrence);
  loan.require_return_without_callbacks(*this,*p.host,state,p.revision,p.dirty_next_occurrence);
 }catch(...){loan.fail_without_callbacks();throw;}
}
void BnwClientSelectionControl::require_attrition_selected_unit_info_dirty_return(const runtime::BnwAttritionSelectedUnitInfoDirtyLoan&loan)const{
 loan.require_alive_without_callbacks();const auto&p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.actual_ui_bound&&p.unit_info_dirty,
  "UnitInfo real Selection return absent/retired");loan.require_return_without_callbacks(*this,*p.host,*p.state,p.revision,p.dirty_next_occurrence);
}
}

namespace civ5::ui {
void BnwClientSelectionControl::require_attrition_selection_plot_current(const runtime::BnwAttritionSelectionPlotRead&loan)const{
 // This child cannot call require_current()/active_player() as opaque helpers:
 // each callback is followed by immutable creator/facade FIRST before raw use.
 loan.require_alive_without_callbacks();const auto&p=*impl_;
 need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.host&&p.actual_ui_bound,"IsSelected canonical input owner retired/unbound");
 need(bool(p.rules_current),"IsSelected immutable rules callback absent");const bool rules=p.rules_current();loan.require_alive_without_callbacks();
 need(rules&&!p.retired.load(),"IsSelected SAME immutable rules/input retired");
 const auto&session=p.host->session();const bool current=session.bnw_game_debug_owner_current();loan.require_alive_without_callbacks();
 need(!p.retired.load()&&current&&session.state_==p.state&&session.source_==p.source&&session.state_generation_==p.generation,
  "IsSelected canonical Game/source/generation retired");
 need(session.debug_->identity.payload_sha256==p.identity.payload_sha256&&session.debug_->identity.rules_sha256==p.identity.rules_sha256,
  "IsSelected actual imported identity changed");
 (void)session.bnw_game_debug_mode();loan.require_alive_without_callbacks();
 need(!p.retired.load()&&p.state->world.map_header.guid==p.plot_map_guid&&p.state->world.map_plots&&p.state->world.map_projection&&
  p.state->world.map_plots->plots.data()==p.plot_storage&&p.state->world.map_projection->cells.data()==p.projection_storage,
  "IsSelected actual map allocation/GUID retired");
}
std::optional<std::int32_t> BnwClientSelectionControl::consume_attrition_selection_plot_read(const runtime::BnwAttritionSelectionPlotRead&loan,
 const runtime::LiveSessionState&state,const runtime::BnwStockLuaAttemptLease&lease)const{
 loan.require_alive_without_callbacks();const auto&p=*impl_;need(p.thread==std::this_thread::get_id(),"SelectionPlot foreign input creator");
 try{
  require_attrition_selection_plot_current(loan);loan.require_query_source(*this,*p.host,state,lease);loan.require_alive_without_callbacks();
  const auto facts=loan.operands_without_callbacks();need(p.state.get()==&state&&p.generation==facts.generation&&state.ui_source_revision==facts.world_revision,
   "SelectionPlot actual current Source/world differs");
  require_plot_identity(state,p.plot?std::optional<std::int64_t>{*p.plot}:std::nullopt);
  const auto selected=p.plot;std::optional<std::int32_t>result;
  if(selected){const auto&raw=state.world.map_plots->plots.at(static_cast<std::size_t>(*selected));
   const auto index=state.world.map_header.grid.index(raw.x,raw.y);
   need(index==static_cast<std::uint64_t>(*selected)&&raw.index==index&&index<=static_cast<std::uint64_t>(INT32_MAX),
    "SelectionPlot selected canonical plot pointer/GetPlotIndex differs");result=static_cast<std::int32_t>(index);}
  // Real nullable input; only an actual present plot calls GetPlotIndex.
  require_attrition_selection_plot_current(loan);loan.require_query_source(*this,*p.host,state,lease);loan.require_alive_without_callbacks();
  const auto unit_plot=loan.require_final_query_without_callbacks(*this,*p.host,state,lease,result);
  loan.record_query_and_adopt_without_callbacks(*this,result,unit_plot);
  loan.require_return_without_callbacks(*this,*p.host,state,result);return result;
 }catch(...){loan.fail_without_callbacks();throw;}
}
void BnwClientSelectionControl::require_attrition_selection_plot_read_return(const runtime::BnwAttritionSelectionPlotRead&loan)const{
 loan.require_alive_without_callbacks();const auto&p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.actual_ui_bound,
  "SelectionPlot actual input return retired");loan.require_return_without_callbacks(*this,*p.host,*p.state,loan.result_without_callbacks());
}
}

namespace civ5::ui {
bool BnwClientSelectionControl::attrition_plot_list_state_matches_without_callbacks(const AttritionUnitInfoState&s,
 const BnwClientUnitDirtyEvent&event,bool after)const{
 require_thread();const auto&p=*impl_;const auto canonical=p.host->session().city_selection_.lock();
 if(p.retired.load()||!p.actual_ui_bound||canonical.get()!=this||p.host.get()!=s.host||p.state.get()!=s.state||p.source.get()!=s.source||
  p.generation!=s.generation||p.host->session().state_!=p.state||p.host->session().source_!=p.source||p.host->session().state_generation_!=s.generation||
  p.selected!=s.selected||p.unit!=s.selected_unit||p.unit_owner!=s.selected_owner||p.plot!=s.selected_plot||
  p.city_info_dirty!=s.city_flag||p.city_info_dirty_setters!=s.city_count||p.unit_info_dirty!=s.unit.unit_info||
  s.unit.client_revision==UINT64_MAX||s.unit.next_occurrence==0||s.unit.next_occurrence==UINT64_MAX||
  p.revision!=s.unit.client_revision+(after?1U:0U)||p.dirty_next_occurrence!=s.unit.next_occurrence+(after?1U:0U)||
  p.plot_list_dirty!=(after?true:s.unit.plot_list_buttons))return false;
 if(!after)return p.unit_dirty_events==s.unit.events;
 return s.unit.events.size()<s.unit.events.max_size()&&p.unit_dirty_events.size()==s.unit.events.size()+1&&
  std::equal(s.unit.events.begin(),s.unit.events.end(),p.unit_dirty_events.begin())&&p.unit_dirty_events.back()==event;
}
void BnwClientSelectionControl::require_attrition_plot_list_current(const runtime::BnwAttritionPlotListDirtyLoan&loan)const{
 // This child cannot call require_current()/active_player() as opaque helpers:
 // each callback is followed by immutable creator/facade FIRST before raw use.
 loan.require_alive_without_callbacks();const auto&p=*impl_;
 need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.host&&p.actual_ui_bound,"IsSelected canonical input owner retired/unbound");
 need(bool(p.rules_current),"IsSelected immutable rules callback absent");const bool rules=p.rules_current();loan.require_alive_without_callbacks();
 need(rules&&!p.retired.load(),"IsSelected SAME immutable rules/input retired");
 const auto&session=p.host->session();const bool current=session.bnw_game_debug_owner_current();loan.require_alive_without_callbacks();
 need(!p.retired.load()&&current&&session.state_==p.state&&session.source_==p.source&&session.state_generation_==p.generation,
  "IsSelected canonical Game/source/generation retired");
 need(session.debug_->identity.payload_sha256==p.identity.payload_sha256&&session.debug_->identity.rules_sha256==p.identity.rules_sha256,
  "IsSelected actual imported identity changed");
 (void)session.bnw_game_debug_mode();loan.require_alive_without_callbacks();
 need(!p.retired.load()&&p.state->world.map_header.guid==p.plot_map_guid&&p.state->world.map_plots&&p.state->world.map_projection&&
  p.state->world.map_plots->plots.data()==p.plot_storage&&p.state->world.map_projection->cells.data()==p.projection_storage,
  "IsSelected actual map allocation/GUID retired");
}
void BnwClientSelectionControl::consume_attrition_plot_list_dirty(const runtime::BnwAttritionPlotListDirtyLoan&loan,
 runtime::LiveSessionState&state,const runtime::BnwStockLuaAttemptLease&lease){
 loan.require_alive_without_callbacks();auto&p=*impl_;need(p.thread==std::this_thread::get_id(),"UnitInfo foreign Selection creator");
 try{
  require_attrition_plot_list_current(loan);loan.require_alive_without_callbacks();loan.require_setter_source(*this,*p.host,state,lease);loan.require_alive_without_callbacks();
  const auto facts=loan.operands_without_callbacks();const auto event=loan.event_without_callbacks();
  need(p.state.get()==&state&&p.generation==facts.generation&&state.ui_source_revision==facts.world_revision&&
   p.revision==facts.client_before&&p.dirty_next_occurrence==facts.ordinal_before&&p.revision<UINT64_MAX&&p.dirty_next_occurrence>0&&
   p.dirty_next_occurrence<UINT64_MAX&&p.unit_dirty_events.size()<p.unit_dirty_events.max_size(),"UnitInfo actual source/client/journal capacity differs");
  auto prepared=p.unit_dirty_events;prepared.push_back(event);
  // Allocation and callbackful currentness finish BEFORE exact pure seal.
  require_attrition_plot_list_current(loan);loan.require_alive_without_callbacks();loan.require_setter_source(*this,*p.host,state,lease);loan.require_alive_without_callbacks();
  loan.require_final_setter_without_callbacks(*this,*p.host,state,lease,p.revision,p.dirty_next_occurrence);
  static_assert(noexcept(p.unit_dirty_events.swap(prepared)));
  p.plot_list_dirty=true;p.unit_dirty_events.swap(prepared);++p.dirty_next_occurrence;++p.revision;
  loan.record_setter_and_adopt_without_callbacks(*this,p.revision,p.dirty_next_occurrence);
  loan.require_return_without_callbacks(*this,*p.host,state,p.revision,p.dirty_next_occurrence);
 }catch(...){loan.fail_without_callbacks();throw;}
}
void BnwClientSelectionControl::require_attrition_plot_list_dirty_return(const runtime::BnwAttritionPlotListDirtyLoan&loan)const{
 loan.require_alive_without_callbacks();const auto&p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.actual_ui_bound&&p.plot_list_dirty,
  "UnitInfo real Selection return absent/retired");loan.require_return_without_callbacks(*this,*p.host,*p.state,p.revision,p.dirty_next_occurrence);
}
}
