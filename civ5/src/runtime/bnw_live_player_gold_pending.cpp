#include "civ5/runtime/bnw_live_player_gold_pending.hpp"
#include "civ5/runtime/bnw_treasury_same_vm_income_loan.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/saves/civis_save.hpp"
#include "civ5/ui/bnw_client_selection_control.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include <limits>
#include <algorithm>
#include <utility>
namespace civ5::runtime {
namespace {
void gold_need(bool ok,const char* why){if(!ok)throw TreasuryUnavailable(why);}
constexpr std::array<TreasuryDirty,3> gold_dirty_order{TreasuryDirty::misc_buttons,TreasuryDirty::selection_buttons,TreasuryDirty::game_data};
std::size_t gold_dirty_index(TreasuryDirty kind){
 for(std::size_t i=0;i<gold_dirty_order.size();++i)if(gold_dirty_order[i]==kind)return i;
 throw TreasuryUnavailable("Live Gold unknown dirty bit");
}
}
struct BnwLivePlayerGoldPending::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 Binding binding;std::int64_t actor;
 std::weak_ptr<const BnwLivePlayerGoldPending> self;
 BnwLivePlayerGoldView observed;
 // Exact Gold-target before-image only. No cloned world or public partial
 // restoration grant; arbitrary Lua effects still require a real checkpoint.
 std::vector<std::int64_t> balance_before,income_before;
 std::uint64_t serial=0,dirty_world_revision=0;bool attempted=false,dirty_reserved=false;
 std::size_t dirty_cursor=0;bool deficit_reserved=false,deficit_attempted=false;
 std::uint64_t deficit_world_revision=0,deficit_rng_world_revision=0;bool deficit_rng_reserved=false;
 SimulationRandomState deficit_rng_map{};
 explicit Impl(Binding actual,std::int64_t player):binding(std::move(actual)),actor(player){}
 void creator_current()const{gold_need(creator==std::this_thread::get_id(),"Live Gold foreign thread before borrow");}
 std::array<std::int64_t,6> digest()const{
  const auto& w=*binding.world;const auto& t=w.treasury.players.at(actor);const auto& m=w.maintenance.players.at(actor);
  gold_need(t.balance_history_times100.size()<=static_cast<std::size_t>(INT32_MAX)&&
   t.income_history_times100.size()<=static_cast<std::size_t>(INT32_MAX),"Live Gold native history count exceeds i32");
  return {t.balance_times100,t.lifetime_gross_gold,m.cached_unit_expense,m.cached_supply_expense,
   static_cast<std::int64_t>(t.balance_history_times100.size()),static_cast<std::int64_t>(t.income_history_times100.size())};
 }
 void require_current()const{
  creator_current();binding.lease.require_operation(serial);
  gold_need(binding.session&&binding.host&&binding.world&&binding.facade_storage&&binding.require_facade,
   "Live Gold actual physical source/session missing");
  binding.require_facade();const auto world=binding.lease.world_lease();
  gold_need(world.get()==binding.world.get()&&!world.owner_before(binding.world)&&!binding.world.owner_before(world)&&
   &binding.session->state()==binding.world.get()&& &binding.host->session()==binding.session,
   "Live Gold source belongs to another/retired physical LiveSession");
  gold_need(!binding.source.expired(),"Live Gold canonical Source factory expired");
  binding.lease.require_operation(serial);
 }
 struct Mirror final:TreasuryNativeMirror {
  Impl& pending;std::unique_ptr<TreasuryNativeMirror> actual;
  mutable std::array<std::int64_t,6> expected{},staged{};
  mutable std::uint64_t before_revision=0,next_revision=0;
  mutable std::shared_ptr<void> native_scope;
  mutable bool prepared=false,changed=false;
  Mirror(Impl& p,std::unique_ptr<TreasuryNativeMirror> raw):pending(p),actual(std::move(raw)),expected(p.digest()){}
  void prepare(std::int64_t actor)const override{
   pending.creator_current();gold_need(actor==pending.actor&&!prepared&&actual,"Live Gold mirror prepare substituted/reentered");
   actual->prepare(actor);staged=pending.digest();changed=staged!=expected;
   before_revision=pending.binding.world->ui_source_revision;next_revision=before_revision;
   if(changed){
    gold_need(before_revision<std::numeric_limits<std::uint64_t>::max(),"Live Gold revision exhausted before pair");
    next_revision=before_revision+1;
    gold_need(pending.binding.prepare_revision&&pending.binding.commit_revision,"Live Gold real pure source adoption absent");
    native_scope=pending.binding.prepare_revision(before_revision,next_revision);
    gold_need(bool(native_scope),"Live Gold actual prepared source allocation absent");
   }
   prepared=true;
  }
  void synchronize(std::int64_t actor)noexcept override{
   // prepare established all targets/capacity/source operands. No provider or
   // allocation occurs inside this raw+revision+source expectation interval.
   if(!prepared||actor!=pending.actor||pending.binding.world->ui_source_revision!=before_revision)std::terminate();
   actual->synchronize(actor);
   if(changed){pending.binding.world->ui_source_revision=next_revision;
    pending.binding.commit_revision(native_scope.get(),next_revision);
    ++pending.observed.paired_stores;}
   expected=staged;pending.observed.after=staged;pending.observed.revision_after=next_revision;
   prepared=false;native_scope.reset();
  }
 };
 struct Services final:TreasuryTurnServices {
  Impl& pending;explicit Services(Impl& p):pending(p){}
  void treasury_dirty(std::int64_t actor,TreasuryDirty kind)override{
   gold_need(actor==pending.actor,"Live Gold dirty actor substituted");
   pending.creator_current();pending.require_current();
   gold_need(pending.observed.phase==BnwLivePlayerGoldPhase::executing&&!pending.dirty_reserved&&
    pending.dirty_cursor<gold_dirty_order.size()&&gold_dirty_order[pending.dirty_cursor]==kind,
    "Live Gold concrete dirty call repeated/reordered");
   pending.observed.required_dirty=kind;pending.observed.phase=BnwLivePlayerGoldPhase::dirty_owner_required;
   gold_need(bool(pending.binding.actual_dirty),"Live Gold requires actual canonical Selection dirty4/6/11 consumer");
   const auto actual=pending.self.lock();gold_need(bool(actual),"Live Gold closed Pending allocation retired before dirty call");
   const auto before=pending.dirty_cursor;
   // Reserve BEFORE retaining the callbackful Engine/UI provider. The same
   // original DoGold call remains on stack; no scheduler phase is invented.
   pending.dirty_reserved=true;pending.dirty_world_revision=pending.binding.world->ui_source_revision;
   pending.observed.phase=BnwLivePlayerGoldPhase::dirty_executing;
   pending.binding.actual_dirty(actual,kind,pending.binding.lease);
   gold_need(pending.dirty_reserved&&pending.dirty_cursor==before+1&&
    pending.observed.dirty_setters==pending.dirty_cursor&&pending.observed.phase==BnwLivePlayerGoldPhase::executing,
    "Live Gold actual dirty receiver did not return its sealed native store");
   pending.require_current();pending.dirty_reserved=false;pending.observed.required_dirty.reset();
  }
  void deficit(std::int64_t actor,content::TechnologyProfile profile)override{
   pending.creator_current();gold_need(actor==pending.actor&&profile==content::TechnologyProfile::bnw,
    "Live Gold deficit source substituted");pending.require_current();
   gold_need(!pending.deficit_attempted&&!pending.deficit_reserved&&!pending.dirty_reserved&&
    pending.observed.phase==BnwLivePlayerGoldPhase::executing,"Live Gold original DoDeficit replay/reentry");
   pending.deficit_attempted=true;pending.observed.phase=BnwLivePlayerGoldPhase::deficit_owner_required;
   gold_need(bool(pending.binding.actual_deficit)&&!pending.binding.deficit_core.expired(),
    "Live Gold actual current Unit Core/city deficit supplier required");
   const auto actual=pending.self.lock();gold_need(bool(actual),"Live Gold closed Pending expired before DoDeficit");
   // Original DoDeficit call is reserved before Core/Source provider callbacks.
   pending.deficit_reserved=true;pending.deficit_world_revision=pending.binding.world->ui_source_revision;
   pending.observed.phase=BnwLivePlayerGoldPhase::deficit_executing;
   pending.binding.actual_deficit(actual,pending.binding.lease);
   if(pending.observed.phase==BnwLivePlayerGoldPhase::deficit_military_owner_required)
    throw TreasuryUnavailable("Actual DoDeficit roll<50 requires genuine MilitaryAI -> notification -> scrap");
   if(pending.observed.phase==BnwLivePlayerGoldPhase::deficit_random_owner_required)
    throw TreasuryUnavailable("Actual DoDeficit combat>cities requires Game RNG100 -> MilitaryAI -> notification -> scrap");
   gold_need(pending.observed.deficit_scan_returned&&pending.observed.deficit_no_disband_returned&&
    pending.observed.phase==BnwLivePlayerGoldPhase::executing,"Live Gold natural DoDeficit return absent");
   pending.require_current();pending.deficit_reserved=false;pending.deficit_rng_reserved=false;
  }
  void unlock_trade_gold_achievement(std::int64_t actor)override{
   gold_need(actor==pending.actor,"Live Gold achievement actor substituted");
   saves::require_supported_standalone_services(pending.binding.world->standalone_services);
   gold_need(pending.binding.world->standalone_services.steam_achievements==saves::StandaloneServiceBackend::backend_disabled,
    "Live Gold unsupported standalone achievement backend");
   pending.observed.achievement_backend_disabled=true; // external unavailable, no success
  }
 };
 void execute(){
  creator_current();gold_need(!attempted,"Live Gold replay denied");attempted=true;
  try{
   serial=binding.lease.operation_serial();require_current();
   auto& w=*binding.world;
   gold_need(actor>=0&&actor<64&&w.setup.profile==content::TechnologyProfile::bnw&&
    w.treasury.profile==content::TechnologyProfile::bnw&&w.cities.profile==content::TechnologyProfile::bnw&&
    w.maintenance.profile==content::TechnologyProfile::bnw&&!w.player_gold_publication&&binding.fresh_income,
    "Live Gold imported canonical component not admitted");
   gold_need(binding.treasury_rules.profile==w.setup.profile&&binding.city_rules.profile==w.setup.profile&&
    binding.maintenance_rules.profile==w.setup.profile,"Live Gold actual rules profiles differ");
   observed.actor=actor;observed.turn=w.setup.game_turn;observed.revision_before=w.ui_source_revision;
   observed.revision_after=w.ui_source_revision;observed.before=digest();observed.after=observed.before;
   const auto& original=w.treasury.players.at(actor);
   balance_before=original.balance_history_times100;income_before=original.income_history_times100;
   // This is the actual raw mirror140, not copied context/world ownership.
   Mirror mirror(*this,BnwLivePlayerGoldPending::make_actual_mirror(w,actor));
   require_current();const auto active=detail::bnw_game_active_player_read(w);
   Services services(*this);
   TreasuryServices income;
   income.founder_benefits_religion=[this](std::int64_t player){
    creator_current();gold_need(player==actor,"Live Gold income actor substituted");require_current();
    const auto actual=binding.fresh_income();gold_need(bool(actual),"Live Gold actual scoped income loan absent");
    ++observed.income_queries;const auto result=actual->founder_benefits_read_only(player,*binding.world);
    require_current();return result;
   };
   MaintenanceWorld world{w.setup,w.player_research,w.cities,w.technologies};
   TreasuryTurnContext context{w.treasury,world,w.maintenance,binding.treasury_rules,binding.city_rules,
    binding.maintenance_rules,binding.host->defines(),nullptr,services,active,std::move(income),&mirror};
   // Allocation and immutable issued-receipt sink already precede stores.
   BnwPlayerGoldPublication publication{actor,{},0,true};publication.dirty_kinds.reserve(3);
   w.player_gold_publication=std::move(publication);observed.phase=BnwLivePlayerGoldPhase::executing;
   (void)do_gold(context,actor);
   // Scalar/history/UI/deficit native calls really returned. Preserve this
   // earned native phase even if a subsequent callbackful bookend fails.
   observed.phase=BnwLivePlayerGoldPhase::native_returned;observed.after=digest();observed.revision_after=w.ui_source_revision;
   require_current();
   // Our native return is complete; unavailable UI/deficit paths threw above
   // and retain their mandatory in-flight publication rather than clearing it.
   w.player_gold_publication.reset();
  }catch(...){
   if(observed.phase!=BnwLivePlayerGoldPhase::native_returned&&
    observed.phase!=BnwLivePlayerGoldPhase::dirty_owner_required&&
    observed.phase!=BnwLivePlayerGoldPhase::deficit_owner_required&&
    observed.phase!=BnwLivePlayerGoldPhase::deficit_random_owner_required&&
    observed.phase!=BnwLivePlayerGoldPhase::deficit_military_owner_required)
    observed.phase=BnwLivePlayerGoldPhase::uncertain;
   throw;
  }
 }
};
BnwLivePlayerGoldPending::BnwLivePlayerGoldPending(Binding binding,std::int64_t actor)
 :impl_(std::make_unique<Impl>(std::move(binding),actor)){}
BnwLivePlayerGoldPending::~BnwLivePlayerGoldPending()=default;
std::shared_ptr<const BnwLivePlayerGoldPending> BnwLivePlayerGoldPending::execute_owned(
 Binding binding,std::int64_t actor,std::shared_ptr<const BnwLivePlayerGoldPending>& issued){
 binding.lease.require_active(); // authentic creator gate before registry borrow
 gold_need(!issued&&binding.source_registry_slot&&!(*binding.source_registry_slot),"Live Gold receipt/registry slot is not empty");
 auto actual=std::shared_ptr<BnwLivePlayerGoldPending>(new BnwLivePlayerGoldPending(std::move(binding),actor));
 actual->impl_->self=actual;issued=actual; // no-throw retention BEFORE all native writes
 *actual->impl_->binding.source_registry_slot=actual;
 actual->impl_->binding.source_registry_slot=nullptr; // never dereference after source lifetime
 actual->impl_->execute();return actual;
}

std::shared_ptr<const BnwLivePlayerGoldPending> BnwLivePlayerGoldPending::retain_factory_owner(
 const std::shared_ptr<const BnwLivePlayerGoldPending>& supplied){
 gold_need(bool(supplied),"Live Gold Pending receiver absent");supplied->impl_->creator_current();
 const auto actual=supplied->impl_->self.lock();
 gold_need(actual&&actual.get()==supplied.get()&&!actual.owner_before(supplied)&&!supplied.owner_before(actual),
  "Live Gold forged Pending allocation/control");return actual;
}
void BnwLivePlayerGoldPending::require_dirty_identity_without_callbacks(const ui::SelectedBnwPreviewHost& host,
 const LiveSessionState& state,TreasuryDirty kind,std::uint64_t serial)const{
 const auto& p=*impl_;p.creator_current();const auto at=gold_dirty_index(kind);const auto actual=p.self.lock();
 gold_need(actual&&actual.get()==this&&p.binding.host.get()==&host&&p.binding.world.get()==&state&&
  p.binding.session==&host.session()&& &p.binding.session->state()==&state&&!p.binding.source.expired()&&
  p.serial!=0&&serial==p.serial&&p.dirty_reserved&&p.observed.required_dirty==kind&&
  state.ui_source_revision==p.dirty_world_revision&&p.observed.revision_after==p.dirty_world_revision&&
  state.setup.game_turn==p.observed.turn&&p.digest()==p.observed.after&&
  ((p.observed.phase==BnwLivePlayerGoldPhase::dirty_executing&&p.dirty_cursor==at)||
   (p.observed.phase==BnwLivePlayerGoldPhase::executing&&p.dirty_cursor==at+1)),
  "Live Gold SAME source/world/reserved original dirty call differs");
 const auto& publication=state.player_gold_publication;
 gold_need(publication&&publication->player==p.actor&&publication->uncertain&&
  publication->next_step==p.dirty_cursor&&publication->dirty_kinds.size()==p.dirty_cursor&&publication->dirty_kinds.capacity()>=3&&
  std::equal(publication->dirty_kinds.begin(),publication->dirty_kinds.end(),gold_dirty_order.begin(),
   [](std::uint8_t actual,TreasuryDirty expected){return actual==static_cast<std::uint8_t>(expected);}),
  "Live Gold genuine dirty publication prefix differs");
}
void BnwLivePlayerGoldPending::require_dirty_source(const ui::SelectedBnwPreviewHost& host,
 const LiveSessionState& state,TreasuryDirty kind,const BnwStockLuaAttemptLease& lease)const{
 const auto& p=*impl_;p.creator_current();p.require_current();lease.require_operation(p.serial);
 const auto world=lease.world_lease();
 gold_need(world.get()==p.binding.world.get()&&!world.owner_before(p.binding.world)&&!p.binding.world.owner_before(world)&&
  p.observed.phase==BnwLivePlayerGoldPhase::dirty_executing&&detail::bnw_game_active_player_read(state)==p.actor,
  "Live Gold native active actor/operation was substituted");
 require_dirty_identity_without_callbacks(host,state,kind,p.serial);lease.require_operation(p.serial);
}
void BnwLivePlayerGoldPending::accept_actual_dirty_return(const ui::BnwClientSelectionControl& selection,TreasuryDirty kind)const{
 auto& p=*impl_;p.creator_current();const auto at=gold_dirty_index(kind);
 gold_need(p.dirty_cursor==at&&p.observed.phase==BnwLivePlayerGoldPhase::dirty_executing,
  "Live Gold actual Selection return replayed/reordered");
 const auto client_revision=selection.require_live_gold_dirty_return(*this,kind);
 auto& publication=*p.binding.world->player_gold_publication;
 gold_need(publication.dirty_kinds.capacity()>=3&&publication.dirty_kinds.size()==at&&publication.next_step==at,
  "Live Gold publication capacity/prefix changed before actual return");
 // Callbackless exact store receipt. Preserve this earned history if a later
 // source bookend fails; no UI presentation or full-player-turn ACK is made.
 publication.dirty_kinds.push_back(static_cast<std::uint8_t>(kind));++publication.next_step;
 p.observed.dirty_client_revisions[at]=client_revision;++p.dirty_cursor;
 p.observed.dirty_setters=p.dirty_cursor;p.observed.phase=BnwLivePlayerGoldPhase::executing;
}

void BnwLivePlayerGoldPending::require_deficit_source(const ui::SelectedBnwPreviewHost& host,
 const LiveSessionState& state,const BnwStockLuaAttemptLease& lease)const{
 const auto& p=*impl_;p.creator_current();p.require_current();lease.require_operation(p.serial);
 const auto world=lease.world_lease();
 gold_need(world.get()==p.binding.world.get()&&!world.owner_before(p.binding.world)&&!p.binding.world.owner_before(world)&&
  p.binding.host.get()==&host&& &state==p.binding.world.get()&&p.deficit_attempted&&p.deficit_reserved&&!p.dirty_reserved&&
  p.observed.phase==BnwLivePlayerGoldPhase::deficit_executing&&state.ui_source_revision==p.deficit_world_revision&&
  p.observed.revision_after==p.deficit_world_revision&&state.setup.game_turn==p.observed.turn&&p.digest()==p.observed.after,
  "Live Gold deficit SAME source/reserved original call changed");lease.require_operation(p.serial);
}
void BnwLivePlayerGoldPending::require_deficit_identity_without_callbacks(const ui::SelectedBnwPreviewHost& host,
 const LiveSessionState& state,const std::weak_ptr<const BnwNativeUnitAllocatorCore>& core,std::uint64_t serial)const{
 const auto& p=*impl_;p.creator_current();const auto actual=p.self.lock();
 gold_need(actual&&actual.get()==this&&!core.expired()&&!core.owner_before(p.binding.deficit_core)&&
  !p.binding.deficit_core.owner_before(core)&&p.binding.host.get()==&host&&p.binding.world.get()==&state&&
  p.binding.session==&host.session()&& &p.binding.session->state()==&state&&!p.binding.source.expired()&&p.serial!=0&&p.serial==serial&&
  p.deficit_attempted&&p.deficit_reserved&&!p.dirty_reserved&&p.observed.phase==BnwLivePlayerGoldPhase::deficit_executing&&
  state.ui_source_revision==p.deficit_world_revision&&p.observed.revision_after==p.deficit_world_revision&&
  state.setup.game_turn==p.observed.turn&&p.digest()==p.observed.after,
  "Live Gold actual deficit Core/source/world store-free interval differs");
 const auto& publication=state.player_gold_publication;
 gold_need(publication&&publication->player==p.actor&&publication->uncertain&&publication->next_step==p.dirty_cursor&&
  publication->dirty_kinds.size()==p.dirty_cursor,"Live Gold actual deficit publication prefix differs");
}
void BnwLivePlayerGoldPending::accept_actual_deficit_scan_return(const BnwSelectedStockLuaSourceOwner& source,const BnwNativeUnitAllocatorCore& core)const{
 auto& p=*impl_;p.creator_current();
 gold_need(p.observed.phase==BnwLivePlayerGoldPhase::deficit_executing&&!p.observed.deficit_scan_returned,
  "Live Gold actual deficit scan acceptance replayed");
 const auto counts=source.require_live_gold_deficit_scan_return(*this,core);
 gold_need(counts[0]>=0&&counts[1]>=0,"Live Gold sealed native deficit counts invalid");
 p.observed.deficit_combat_units=counts[0];p.observed.deficit_cities=counts[1];p.observed.deficit_scan_returned=true;
 p.observed.deficit_no_disband_returned=counts[0]<=counts[1];
 p.observed.phase=p.observed.deficit_no_disband_returned?BnwLivePlayerGoldPhase::executing:BnwLivePlayerGoldPhase::deficit_random_owner_required;
}

void BnwLivePlayerGoldPending::begin_actual_deficit_random(const ui::SelectedBnwPreviewHost& host,
 const LiveSessionState& state,const BnwStockLuaAttemptLease& lease)const{
 auto& p=*impl_;p.creator_current();p.require_current();lease.require_operation(p.serial);
 const auto world=lease.world_lease();
 gold_need(world.get()==&state&&p.binding.world.get()==&state&&p.binding.host.get()==&host&&
  !world.owner_before(p.binding.world)&&!p.binding.world.owner_before(world)&&
  p.deficit_reserved&&p.deficit_attempted&&!p.deficit_rng_reserved&&p.observed.deficit_scan_returned&&
  !p.observed.deficit_no_disband_returned&&p.observed.deficit_combat_units>p.observed.deficit_cities&&
  p.observed.phase==BnwLivePlayerGoldPhase::deficit_random_owner_required&&!p.observed.deficit_rng_stored&&
  state.ui_source_revision==p.deficit_world_revision&&state.ui_source_revision==p.observed.revision_after,
  "Live Gold actual original RNG100 branch source/reentry differs");
 p.deficit_rng_reserved=true;p.deficit_rng_world_revision=state.ui_source_revision;p.deficit_rng_map=state.random.map.state();
 p.observed.deficit_rng_before=state.random.gameplay.state();p.observed.phase=BnwLivePlayerGoldPhase::deficit_random_executing;
}
void BnwLivePlayerGoldPending::require_deficit_rng_identity_without_callbacks(const ui::SelectedBnwPreviewHost& host,
 const LiveSessionState& state,std::uint64_t serial)const{
 const auto& p=*impl_;p.creator_current();const auto actual=p.self.lock();
 gold_need(actual&&actual.get()==this&&p.binding.host.get()==&host&&p.binding.world.get()==&state&&
  p.binding.session==&host.session()&& &p.binding.session->state()==&state&&!p.binding.source.expired()&&
  p.serial==serial&&serial!=0&&p.deficit_attempted&&p.deficit_reserved&&p.deficit_rng_reserved&&!p.dirty_reserved&&
  p.observed.deficit_scan_returned&&p.observed.deficit_combat_units>p.observed.deficit_cities&&
  state.setup.game_turn==p.observed.turn&&state.random.map.algorithm()==RandomAlgorithm::bnw&&
  state.random.gameplay.algorithm()==RandomAlgorithm::bnw&&state.random.map.state()==p.deficit_rng_map&&
  p.digest()==p.observed.after,"Live Gold SAME actual RNG/Game/Pending identity differs");
 const bool stored=p.observed.deficit_rng_stored;
 gold_need(state.ui_source_revision==p.observed.revision_after&&
  (!stored?(p.observed.phase==BnwLivePlayerGoldPhase::deficit_random_executing&&
    state.ui_source_revision==p.deficit_rng_world_revision&&state.random.gameplay.state()==p.observed.deficit_rng_before):
   (p.deficit_rng_world_revision<UINT64_MAX&&state.ui_source_revision==p.deficit_rng_world_revision+1&&
    state.random.gameplay.state()==p.observed.deficit_rng_after&&
    (p.observed.phase==BnwLivePlayerGoldPhase::deficit_random_executing||p.observed.phase==BnwLivePlayerGoldPhase::executing||
     p.observed.phase==BnwLivePlayerGoldPhase::deficit_military_owner_required))),
  "Live Gold actual RNG100 store/earned return interval differs");
}
std::shared_ptr<void> BnwLivePlayerGoldPending::prepare_actual_deficit_rng_store(const ui::SelectedBnwPreviewHost& host,
 const LiveSessionState& state,std::uint64_t serial)const{
 const auto& p=*impl_;require_deficit_rng_identity_without_callbacks(host,state,serial);
 gold_need(!p.observed.deficit_rng_stored&&p.binding.prepare_revision&&p.binding.commit_revision&&
  state.ui_source_revision<UINT64_MAX&&p.observed.paired_stores<UINT64_MAX,"Live Gold RNG store preparation unavailable");
 const auto scope=p.binding.prepare_revision(state.ui_source_revision,state.ui_source_revision+1);
 gold_need(bool(scope),"Live Gold actual RNG Source revision allocation absent");return scope;
}
void BnwLivePlayerGoldPending::commit_actual_deficit_rng_store(LiveSessionState& state,std::uint64_t serial,void* scope,
 std::uint64_t next,const SimulationRandomState& before,const SimulationRandomState& after,std::uint64_t value)const noexcept{
 auto& p=*impl_;
 if(p.creator!=std::this_thread::get_id()||p.binding.world.get()!=&state||p.serial!=serial||!scope||
  !p.binding.commit_revision||!p.deficit_rng_reserved||p.observed.deficit_rng_stored||
  p.observed.phase!=BnwLivePlayerGoldPhase::deficit_random_executing||state.ui_source_revision!=p.deficit_rng_world_revision||
  state.ui_source_revision==UINT64_MAX||next!=state.ui_source_revision+1)std::terminate();
 // Native generator really changed. Capture earned history before any future
 // provider/bookend; this interval contains no callback or allocation.
 state.ui_source_revision=next;p.binding.commit_revision(scope,next);
 p.observed.deficit_rng_before=before;p.observed.deficit_rng_after=after;p.observed.deficit_rng_value=value;
 p.observed.deficit_rng_civis_trace_retained=true;p.observed.deficit_rng_stored=true;
 ++p.observed.paired_stores;p.observed.revision_after=next;
}
void BnwLivePlayerGoldPending::accept_actual_deficit_rng_return(const BnwLiveGameContext::GameReference& game)const{
 auto& p=*impl_;p.creator_current();
 gold_need(p.observed.phase==BnwLivePlayerGoldPhase::deficit_random_executing&&!p.observed.deficit_rng_returned,
  "Live Gold actual RNG return acceptance replay");
 const auto& r=game.require_actual_deficit_random_return(*this);
 gold_need(r.value<100&&r.value==p.observed.deficit_rng_value,"Live Gold sealed RNG100 returned value differs");
 p.observed.deficit_rng_returned=true;p.observed.deficit_no_disband_returned=r.value>=50;
 p.observed.phase=r.value>=50?BnwLivePlayerGoldPhase::executing:BnwLivePlayerGoldPhase::deficit_military_owner_required;
}
BnwLivePlayerGoldView BnwLivePlayerGoldPending::view()const{
 impl_->creator_current();return impl_->observed;
}
}
