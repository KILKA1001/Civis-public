#include "civ5/runtime/bnw_live_game_context.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/bnw_unit_combat_warned_restore_plan.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_live_player_gold_pending.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include <type_traits>
#include <limits>
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <stdexcept>
#include <string_view>
#include <exception>
namespace civ5::runtime {
namespace {
bool final_member(const saves::GameStateObservation& state) {
 const saves::NamedSaveFlag* found=nullptr;
 for(const auto& flag:state.flags)if(flag.name&&std::string_view(flag.name)=="final_initialized"){
  if(found)throw std::runtime_error("Game final member is duplicated");found=&flag;
 }
 if(!found)throw std::runtime_error("Game final member unavailable");
 return found->value;
}
}
struct BnwLiveGameContext::LogicalGame final {
 const LiveSession* facade;
 std::shared_ptr<const saves::SavedSession> source;
 std::shared_ptr<LiveSessionState> state;
 const std::uint64_t owner,generation;
 const bool final_initialized;
 std::shared_ptr<LiveSession::RetainedFacadeLifetime> lifetime;
 std::vector<DeficitRandomDiagnostic> deficit_random_trace;
 // SDK CvGame::Read calls reset(), which sets unsaved m_bCombatWarned=false.
 // Only the actual original Read factory qualifies this member; arbitrary
 // restored DTO/Civis overlays do not fill a missing runtime value.
 std::optional<bool> combat_warned;
 std::uint64_t combat_warned_store_sequence=0;
 bool combat_warned_original_payload_read_qualified=false;
 bool combat_warned_restore_prepared=false,combat_warned_restore_history_installed=false;
 const GameReference* combat_warned_restore_reference=nullptr;
 LogicalGame(const LiveSession& session):facade(&session),source(session.source_),
  state(session.state_),owner(session.owner_),generation(session.state_generation_),
  final_initialized(session.bnw_game_read_final_initialized_),
  lifetime(session.retain_bnw_facade_lifetime()) {
   if(session.source_->source.container==saves::SaveContainerKind::civ5&&
      session.bnw_game_read_final_qualified_&&session.bnw_game_debug_owner_current()){combat_warned=false;combat_warned_original_payload_read_qualified=true;}
  }
};
BnwLiveGameContext::BnwLiveGameContext(const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host)
 :host_(host),creator_(std::this_thread::get_id()) {
 if(!host)throw std::runtime_error("Game factory requires canonical live Host");
 if(std::this_thread::get_id()!=host->saved_database_thread_)
  throw std::runtime_error("Game factory foreign immutable Host creator before session borrow");
 if(!host->session_||host->session_->plot_yield_refresh_thread_!=creator_)
  throw std::runtime_error("Game factory Host/session creators differ");
 const auto& s=*host->session_;
 if(!s.bnw_game_read_final_qualified_||!s.bnw_game_debug_owner_current()||
    s.bnw_game_read_final_state_!=s.state_.get()||
    s.bnw_game_read_final_generation_!=s.state_generation_)
  throw std::runtime_error("Game factory requires actual completed BNW Read qualifier");
 game_=std::make_unique<LogicalGame>(s);require_current();
}
BnwLiveGameContext::~BnwLiveGameContext()=default;
void BnwLiveGameContext::require_current() const {
 // Creator check precedes every Host/world borrow, including reference release.
 if(std::this_thread::get_id()!=creator_)throw std::runtime_error("Game factory foreign creator");
 if(!game_||!game_->lifetime)throw std::runtime_error("Game facade issuer missing");
 game_->lifetime->require_current();
 auto host=host_.lock();if(!host||!host->session_||!game_)throw std::runtime_error("Game factory lifetime expired");
 const auto& s=*host->session_;
 if(&s!=game_->facade||s.source_!=game_->source||s.state_!=game_->state||
    s.owner_!=game_->owner||s.state_generation_!=game_->generation||
    !s.bnw_game_read_final_qualified_||!s.bnw_game_debug_owner_current()||
    s.bnw_game_read_final_state_!=game_->state.get()||
    s.bnw_game_read_final_generation_!=game_->generation||
    s.bnw_game_read_final_initialized_!=game_->final_initialized||
    final_member(s.state_->world.game_state)!=game_->final_initialized||
    final_member(s.source_->snapshot.game_state)!=game_->final_initialized)
  throw std::runtime_error("Canonical Game Read member/lifetime retired");
}
std::shared_ptr<BnwLiveGameContext> BnwLiveGameContext::retain_for_actual_host(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host) {
 if(!host)throw std::runtime_error("Game factory has no actual Host");
 // Immutable creator gate BEFORE reading mutable first-issue anchor/session.
 // This also prevents concurrent foreign first issue from touching shared_ptr.
 if(std::this_thread::get_id()!=host->saved_database_thread_)
  throw std::runtime_error("Game factory foreign immutable Host creator before anchor borrow");
 if(host->bnw_live_game_context_){
  auto p=host->bnw_live_game_context_;p->require_current();
  auto actual=p->host_.lock();
  if(actual.get()!=host.get()||actual.owner_before(host)||host.owner_before(actual))
   throw std::runtime_error("Game factory Host control changed");
  return p;
 }
 auto p=std::shared_ptr<BnwLiveGameContext>(new BnwLiveGameContext(host));
 host->bnw_live_game_context_=p;return p;
}
std::unique_ptr<BnwLiveGameContext::GameReference> BnwLiveGameContext::get_game() {
 require_current();return std::unique_ptr<GameReference>(new GameReference(shared_from_this()));
}
BnwLiveGameContext::GameReference::GameReference(std::shared_ptr<BnwLiveGameContext> context)
 :context_(std::move(context)),host_(context_->host_.lock()),
  lock_(context_->game_core_lock_),creator_(std::this_thread::get_id()) {
 context_->require_current();if(!host_)throw std::runtime_error("GetGame lost Host");
}
BnwLiveGameContext::GameReference::~GameReference() {
 // Even a destroyed wrapper releases retained Host/context only on its creator.
 if(std::this_thread::get_id()!=creator_)std::terminate();
 if(reference_count_){
  // RAII exception cleanup performs the same real release; foreign destruction
  // must never unlock a mutex owned by another thread or dereference the world.
  if(std::this_thread::get_id()!=creator_)std::terminate();
  reference_count_=0;if(lock_.owns_lock())lock_.unlock();
 }
}
void BnwLiveGameContext::GameReference::require_actual_binding(const ui::SelectedBnwPreviewHost& host) const {
 if(std::this_thread::get_id()!=creator_)throw std::runtime_error("Game reference foreign creator");
 if(reference_count_!=1||!lock_.owns_lock()||host_.get()!=&host)
  throw std::runtime_error("Game reference not the acquired canonical ref1");
 context_->require_current();
}
bool BnwLiveGameContext::GameReference::is_final_initialized() const {
 require_actual_binding(*host_);
 // Concrete getter over the SAME logical Game member issued by actual Read.
 return context_->game_->final_initialized;
}
std::int32_t BnwLiveGameContext::GameReference::get_active_team() const {
 if(std::this_thread::get_id()!=creator_)throw std::runtime_error("Game active-team foreign creator");
 if(!host_||!context_)throw std::runtime_error("Game active-team actual acquired owners absent");
 require_actual_binding(*host_);
 // SAME qualified logical Game/world, not Scene receipt/team or a DTO lease.
 // The existing reader preserves NO_PLAYER and original Pregame teamType.
 const auto team=detail::bnw_game_active_team_read(*context_->game_->state);
 require_actual_binding(*host_);return team;
}
void BnwLiveGameContext::GameReference::destroy() {
 require_actual_binding(*host_);reference_count_=0;lock_.unlock();
}
void BnwLiveGameContext::GameReference::require_destroyed_for(const ui::SelectedBnwPreviewHost& host) const {
 if(std::this_thread::get_id()!=creator_)throw std::runtime_error("Game Destroy foreign creator");
 if(reference_count_!=0||lock_.owns_lock()||host_.get()!=&host)
  throw std::runtime_error("Game reference did not execute actual Destroy/ref0 release");
 context_->require_current();
}

void BnwLiveGameContext::GameReference::draw_actual_deficit_random100(LiveSessionState& state,
 const std::shared_ptr<const BnwLivePlayerGoldPending>& candidate,std::uint64_t serial,
 const BnwSelectedStockLuaSourceOwner& source,const BnwNativeUnitAllocatorCore& core){
 require_actual_binding(*host_);
 const auto pending=BnwLivePlayerGoldPending::retain_factory_owner(candidate);
 if(deficit_random_||!context_->game_||context_->game_->state.get()!=&state)
  throw std::runtime_error("Game RNG100 actual same acquired world/once receipt absent");
 pending->require_deficit_rng_identity_without_callbacks(*host_,state,serial);
 DeficitRandomReceipt r;r.pending=pending;r.world=&state;r.actor=pending->view().actor;r.turn=state.setup.game_turn;r.serial=serial;
 r.revision_before=state.ui_source_revision;r.before=state.random.gameplay.state();r.map=state.random.map.state();
 if(state.random.gameplay.algorithm()!=RandomAlgorithm::bnw||state.random.map.algorithm()!=RandomAlgorithm::bnw||
  r.before.seed>UINT32_MAX||r.before.calls>=UINT32_MAX||r.before.resets>UINT32_MAX||r.revision_before==UINT64_MAX)
  throw std::runtime_error("Game RNG100 native BNW stream/counter/revision unavailable");
 auto& traces=context_->game_->deficit_random_trace;
 if(traces.size()==traces.max_size()||traces.size()>=UINT64_MAX)
  throw std::runtime_error("Game RNG100 concrete diagnostic trace capacity exhausted");
 traces.reserve(traces.size()+1);r.trace_index=static_cast<std::uint64_t>(traces.size());r.revision_after=r.revision_before+1;
 const auto scope=pending->prepare_actual_deficit_rng_store(*host_,state,serial);
 // These receiver-closed checks follow EVERY Game/Source preparation provider.
 // Source pins the actual Core outside this call; no fresh uncounted lock.
 source.require_live_gold_deficit_random_operands_without_callbacks(*pending,core);
 pending->require_deficit_rng_identity_without_callbacks(*host_,state,serial);
 static_assert(std::is_nothrow_move_constructible_v<DeficitRandomReceipt>);
 static_assert(std::is_nothrow_copy_constructible_v<DeficitRandomDiagnostic>);
 // Actual SAME imported Game generator; no detached SimulationRandom clone.
 // Civis diagnostic trace is concrete. Original GC logging flags and CSV/
 // callstack parity remain unknown, never asserted disabled or delivered.
 r.value=state.random.gameplay.draw(100);r.after=state.random.gameplay.state();r.stored=true;
 DeficitRandomDiagnostic trace;trace.actor=r.actor;trace.turn=r.turn;trace.value=r.value;trace.serial=serial;
 trace.before=r.before;trace.after=r.after;traces.push_back(trace);
 deficit_random_.emplace(std::move(r));
 pending->commit_actual_deficit_rng_store(state,serial,scope.get(),deficit_random_->revision_after,
  deficit_random_->before,deficit_random_->after,deficit_random_->value);
}
const BnwLiveGameContext::GameReference::DeficitRandomReceipt&
BnwLiveGameContext::GameReference::require_actual_deficit_random_return(const BnwLivePlayerGoldPending& pending,
 bool after_destroy)const{
 if(std::this_thread::get_id()!=creator_||!context_||!context_->game_||!host_||!deficit_random_||
  (!after_destroy?(reference_count_!=1||!lock_.owns_lock()):(reference_count_!=0||lock_.owns_lock())))
  throw std::runtime_error("Game RNG100 actual ref1/Destroy-ref0 receipt differs");
 const auto& r=*deficit_random_;const auto actual=r.pending.lock();const auto& state=*context_->game_->state;
 if(!actual||actual.get()!=&pending||r.world!=&state||!r.stored||r.actor!=pending.view().actor||
  r.turn!=state.setup.game_turn||r.revision_before==UINT64_MAX||r.revision_after!=r.revision_before+1||
  state.ui_source_revision!=r.revision_after||state.random.gameplay.state()!=r.after||state.random.map.state()!=r.map)
  throw std::runtime_error("Game RNG100 actual physical stored successor receipt differs");
 pending.require_deficit_rng_identity_without_callbacks(*host_,state,r.serial);
 const auto expected_seed=(r.before.seed*1103515245ULL+12345ULL)&0xffffffffULL;
 const auto expected_value=(((expected_seed>>16)&0xffffULL)*100ULL)/65536ULL;
 if(r.before.calls>=UINT32_MAX||r.after.seed!=expected_seed||r.after.calls!=r.before.calls+1||
  r.after.resets!=r.before.resets||r.value!=expected_value||r.value>=100)
  throw std::runtime_error("Game RNG100 actual native seed/calls/value transition differs");
 const auto& traces=context_->game_->deficit_random_trace;
 if(r.trace_index>=traces.size()||traces.size()!=r.trace_index+1)
  throw std::runtime_error("Game RNG100 genuine diagnostic trace prefix changed");
 const auto& t=traces.at(static_cast<std::size_t>(r.trace_index));
 if(t.actor!=r.actor||t.turn!=r.turn||t.serial!=r.serial||t.range!=100||t.value!=r.value||
  t.before!=r.before||t.after!=r.after||std::string_view(t.location)!="Disband rand")
  throw std::runtime_error("Game RNG100 concrete diagnostic entry differs");
 return r;
}

std::shared_ptr<const BnwLiveGameContext::GameReference::CombatWarnedReceipt>
BnwLiveGameContext::GameReference::set_combat_warned_false(){
 require_actual_binding(*host_);
 if(combat_warned_||!context_->game_||!context_->game_->combat_warned||!context_->game_->combat_warned_original_payload_read_qualified||context_->game_->combat_warned_restore_prepared||
    context_->game_->combat_warned_store_sequence==UINT64_MAX)
  throw std::runtime_error("Game SetCombatWarned actual original Read member/once capacity unavailable");
 auto receipt=std::shared_ptr<CombatWarnedReceipt>(new CombatWarnedReceipt(creator_));
 receipt->self_=receipt;receipt->context_=context_;receipt->host_=host_;receipt->world_=context_->game_->state;
 receipt->reference_=this;receipt->generation_=context_->game_->generation;
 receipt->before_ordinal_=context_->game_->combat_warned_store_sequence;receipt->ordinal_=receipt->before_ordinal_+1;
 receipt->before_=*context_->game_->combat_warned;receipt->original_read_qualified_=context_->game_->combat_warned_original_payload_read_qualified;
 // Publish the closed issued allocation BEFORE the unconditional native store.
 // No providers, allocations or callbacks between these native operations.
 combat_warned_=receipt;
 *context_->game_->combat_warned=false;context_->game_->combat_warned_store_sequence=receipt->ordinal_;receipt->stored_=true;
 return receipt;
}
void BnwLiveGameContext::GameReference::require_combat_warned_return(
 const std::shared_ptr<const CombatWarnedReceipt>& receipt,bool after_destroy)const{
 if(std::this_thread::get_id()!=creator_||!context_||!host_||!context_->game_||!receipt||
    receipt->creator_!=creator_||!combat_warned_||receipt.get()!=combat_warned_.get()||
    receipt.owner_before(combat_warned_)||combat_warned_.owner_before(receipt)||
    (!after_destroy?(reference_count_!=1||!lock_.owns_lock()):(reference_count_!=0||lock_.owns_lock())))
  throw std::runtime_error("Game SetCombatWarned actual closed ref1/Destroy-ref0 receipt differs");
 context_->require_current();
 const auto actual=receipt->self_.lock();const auto source=receipt->context_.lock();
 const auto host=receipt->host_.lock();const auto world=receipt->world_.lock();
 if(!actual||actual.get()!=receipt.get()||actual.owner_before(receipt)||receipt.owner_before(actual)||
    !source||source.get()!=context_.get()||source.owner_before(context_)||context_.owner_before(source)||
    !host||host.get()!=host_.get()||host.owner_before(host_)||host_.owner_before(host)||
    !world||world.get()!=context_->game_->state.get()||world.owner_before(context_->game_->state)||
    context_->game_->state.owner_before(world)||receipt->reference_!=this||receipt->generation_!=context_->game_->generation||
    !receipt->stored_||!receipt->original_read_qualified_||!context_->game_->combat_warned_original_payload_read_qualified||
    (receipt->restored_&&!context_->game_->combat_warned_restore_history_installed)||context_->game_->combat_warned_restore_prepared||
    receipt->before_ordinal_==UINT64_MAX||receipt->ordinal_!=receipt->before_ordinal_+1||
    context_->game_->combat_warned_store_sequence!=receipt->ordinal_||!context_->game_->combat_warned||
    *context_->game_->combat_warned)
  throw std::runtime_error("Game actual SetCombatWarned native false/store sequence/lifetime return changed");
}
std::shared_ptr<const BnwLiveGameContext::GameReference::CombatWarnedReceipt>
BnwLiveGameContext::GameReference::issued_combat_warned_receipt_without_callbacks()const noexcept{
 // Error-history observation only. No currentness provider or world borrow;
 // this retains actual issued/stored facts, never grants retry/completion.
 if(std::this_thread::get_id()!=creator_)return {};return combat_warned_;
}
} // namespace civ5::runtime

namespace civ5::runtime {
BnwLiveGameContext::GameReference::CombatWarnedSnapshot
BnwLiveGameContext::GameReference::snapshot_combat_warned_return(
 const std::shared_ptr<const CombatWarnedReceipt>& receipt,bool after_destroy)const{
 require_combat_warned_return(receipt,after_destroy);
 return {receipt->before_,*context_->game_->combat_warned,receipt->before_ordinal_,receipt->ordinal_,receipt->restored_};
}
std::shared_ptr<const BnwLiveGameContext::GameReference::CombatWarnedReceipt>
BnwLiveGameContext::GameReference::prepare_restore_completed_combat_warned(const BnwUnitCombatWarnedRestorePlan& plan){
 require_actual_binding(*host_);plan.require_current_without_callbacks();
 if(&plan.host_without_callbacks()!=host_.get()||!plan.original_payload_read_qualified()||!context_->game_||combat_warned_||
    context_->game_->combat_warned_store_sequence!=0||context_->game_->combat_warned.value_or(false)||
    context_->game_->combat_warned_restore_prepared||context_->game_->combat_warned_restore_history_installed||prepared_combat_warned_plan_)
  throw std::runtime_error("Game completion restore requires SAME original-payload Read and pristine reset owner");
 // Absent optional member is NOT assumed false. The private Core Plan provides
 // its actual original-payload Read qualification for THIS canonical Host;
 // initialization occurs only in the no-throw install interval below.
 const auto reset=context_->game_->combat_warned;const auto before=plan.before();
 const auto before_ordinal=plan.before_ordinal(),ordinal=plan.ordinal();
 if(before_ordinal==UINT64_MAX||ordinal!=before_ordinal+1)
  throw std::runtime_error("Game completion restore historical store ordinal is not a checked successor");
 auto receipt=std::shared_ptr<CombatWarnedReceipt>(new CombatWarnedReceipt(creator_));
 receipt->self_=receipt;receipt->context_=context_;receipt->host_=host_;receipt->world_=context_->game_->state;
 receipt->reference_=this;receipt->generation_=context_->game_->generation;
 receipt->before_=before;receipt->before_ordinal_=before_ordinal;receipt->ordinal_=ordinal;
 receipt->restored_=true;receipt->original_read_qualified_=true;
 // Allocation and every callbackful binding check precede the final pure Plan
 // guard. No authoritative bool/ordinal/path/Flags mutation is published yet.
 require_actual_binding(*host_);plan.require_current_without_callbacks();
 if(&plan.host_without_callbacks()!=host_.get()||!plan.original_payload_read_qualified()||plan.before()!=before||
    plan.before_ordinal()!=before_ordinal||plan.ordinal()!=ordinal||context_->game_->combat_warned!=reset||
    context_->game_->combat_warned_store_sequence!=0||context_->game_->combat_warned_restore_prepared||
    context_->game_->combat_warned_restore_history_installed||combat_warned_||prepared_combat_warned_plan_)
  throw std::runtime_error("Game completion restore source/reset changed during preparation");
 // Real one-shot reservation and issued allocation survive a late preparation
 // failure. Neither a receipt allocation nor an expired scope earns a store.
 prepared_combat_warned_reset_=reset;prepared_combat_warned_plan_=&plan;
 prepared_combat_warned_mutable_=receipt;combat_warned_=receipt;context_->game_->combat_warned_restore_prepared=true;
 context_->game_->combat_warned_restore_reference=this;
 return receipt;
}
void BnwLiveGameContext::GameReference::install_restore_completed_combat_warned(
 const BnwUnitCombatWarnedRestorePlan& plan,const std::shared_ptr<const CombatWarnedReceipt>& prepared)noexcept{
 // Only the closed Core consumer can call this after its LAST real preflight.
 // Plan address is compared as a synchronous identity, never dereferenced.
 // No provider, parser, allocation, setterfalse, RNG or late Plan guard here.
 if(std::this_thread::get_id()!=creator_||!context_||!context_->game_||!host_||reference_count_!=1||!lock_.owns_lock()||
    prepared_combat_warned_plan_!=&plan||!prepared||!combat_warned_||!prepared_combat_warned_mutable_||
    prepared.get()!=prepared_combat_warned_mutable_.get()||prepared.owner_before(prepared_combat_warned_mutable_)||
    prepared_combat_warned_mutable_.owner_before(prepared)||prepared.get()!=combat_warned_.get()||
    prepared.owner_before(combat_warned_)||combat_warned_.owner_before(prepared)||prepared->creator_!=creator_||
    prepared->reference_!=this||prepared->generation_!=context_->game_->generation||prepared->stored_||!prepared->restored_||
    !prepared->original_read_qualified_||prepared->before_ordinal_==UINT64_MAX||prepared->ordinal_!=prepared->before_ordinal_+1||
    context_->game_->combat_warned!=prepared_combat_warned_reset_||context_->game_->combat_warned_store_sequence!=0||
    !context_->game_->combat_warned_restore_prepared||context_->game_->combat_warned_restore_reference!=this||
    context_->game_->combat_warned_restore_history_installed)
  std::terminate(); // Internal closed-consumer misuse; untrusted bytes were checked before any store.
 const auto actual=prepared->self_.lock();const auto actual_context=prepared->context_.lock();
 const auto actual_host=prepared->host_.lock();const auto actual_world=prepared->world_.lock();
 if(!actual||actual.get()!=prepared.get()||actual.owner_before(prepared)||prepared.owner_before(actual)||
    !actual_context||actual_context.get()!=context_.get()||actual_context.owner_before(context_)||context_.owner_before(actual_context)||
    !actual_host||actual_host.get()!=host_.get()||actual_host.owner_before(host_)||host_.owner_before(actual_host)||
    !actual_world||actual_world.get()!=context_->game_->state.get()||actual_world.owner_before(context_->game_->state)||
    context_->game_->state.owner_before(actual_world))std::terminate();
 // Restoring saved Game history is distinct from executing the original
 // SetCombatWarned call. The receipt explicitly records restored provenance.
 auto& receipt=*prepared_combat_warned_mutable_; // Sole factory retains the actual mutable pointee.
 context_->game_->combat_warned=false;context_->game_->combat_warned_store_sequence=receipt.ordinal_;
 context_->game_->combat_warned_original_payload_read_qualified=true;
 context_->game_->combat_warned_restore_history_installed=true;
 context_->game_->combat_warned_restore_prepared=false;context_->game_->combat_warned_restore_reference=nullptr;
 prepared_combat_warned_plan_=nullptr;receipt.stored_=true;prepared_combat_warned_mutable_.reset();
}
}
