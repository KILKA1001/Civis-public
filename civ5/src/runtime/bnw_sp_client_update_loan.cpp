#include "civ5/runtime/bnw_sp_client_update_loan.hpp"
#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include "civ5/runtime/bnw_local_client_scalar_owner.hpp"
#include "civ5/ui/bnw_scroll_animation_update_loan.hpp"
#include <atomic>
#include <cmath>
#include <exception>
#include <stdexcept>
#include <thread>
namespace civ5::runtime {
namespace {void client_need(bool value,const char* why){if(!value)throw std::runtime_error(why);}}
struct BnwSpClientUpdateLoan::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 const int unwind=std::uncaught_exceptions();
 const BnwWorldViewSourceOperationLoan& source;
 const ui::BnwScrollAnimationUpdateLoan& timing;
 const BnwStockLuaAttemptLease lease;
 const BnwStockLuaAttemptLease::ScopeIdentity scope;
 const std::weak_ptr<BnwLocalClientScalarOwner> client;
 const BnwLocalClientScalarOwner* client_get=nullptr;
 std::uint64_t sequence=0;
 float seconds=0;
 mutable std::atomic<bool> failed{false};
 bool entered=false,consumed=false,alive=true;
 bool control_callback_entered=false,control_callback_returned=false;
 bool proxy_ui_entered=false,proxy_ui_returned=false;
 bool client155_callback_entered=false,client155_callback_returned=false;
 bool game_timer_entered=false,game_timer_returned=false;
 bool postcontext_prefix_entered=false,postcontext_prefix_returned=false;
 Impl(const BnwWorldViewSourceOperationLoan& s,const ui::BnwScrollAnimationUpdateLoan& t,
      const BnwStockLuaAttemptLease& l,std::weak_ptr<BnwLocalClientScalarOwner> c)
  :source(s),timing(t),lease(l),scope(l.pin_active_scope_without_callbacks()),client(std::move(c)){}
 void creator_current()const{
  if(creator!=std::this_thread::get_id()){
   failed.store(true);lease.invalidate();throw std::runtime_error("SP frame loan foreign creator");
  }
  client_need(!failed.load()&&alive&&!consumed,"SP frame loan retired or already returned");
 }
};
BnwSpClientUpdateLoan::BnwSpClientUpdateLoan(const BnwWorldViewSourceOperationLoan& source,
 const ui::BnwScrollAnimationUpdateLoan& timing,const BnwStockLuaAttemptLease& lease,
 std::weak_ptr<BnwLocalClientScalarOwner> client)
 :impl_(std::make_unique<Impl>(source,timing,lease,std::move(client))){
 auto& p=*impl_;
 try{
  p.creator_current();source.require_origin_without_callbacks();
  p.lease.require_scope_identity_without_callbacks(p.scope);
  const auto actual_source=source.sp_client_source_owned();
  actual_source.require_scope_identity_without_callbacks(p.scope);
  timing.require_current_without_callbacks();
  source.require_grid_scene_origin_without_callbacks(timing.actual_scene_without_callbacks());
  p.sequence=timing.tick_sequence_without_callbacks();p.seconds=timing.delta_seconds_without_callbacks();
  client_need(p.sequence&&std::isfinite(p.seconds)&&p.seconds>=0,"SP frame actual platform sample absent");
  const auto actual=p.client.lock();client_need(actual!=nullptr,"SP frame canonical client retired");
  p.client_get=actual.get();actual->require_frame_current_without_callbacks(p.scope);
  actual->require_frame_game_origin_owned(p.scope);
  p.lease.require_scope_identity_without_callbacks(p.scope);source.require_origin_without_callbacks();
  if(actual->frame_update_entered_){
   actual->poisoned_.store(true);throw std::runtime_error("SP frame client update reentered");
  }
  require_current_without_callbacks();
 }catch(...){p.failed.store(true);p.lease.invalidate();throw;}
}
BnwSpClientUpdateLoan::~BnwSpClientUpdateLoan(){
 auto& p=*impl_;if(p.creator!=std::this_thread::get_id())std::terminate();
 if(std::uncaught_exceptions()>p.unwind||p.entered||!p.consumed){p.failed.store(true);p.lease.invalidate();}
 p.alive=false;
}
void BnwSpClientUpdateLoan::require_current_without_callbacks()const{
 const auto& p=*impl_;p.creator_current();
 try{
  p.lease.require_scope_identity_without_callbacks(p.scope);
  p.source.require_origin_without_callbacks();p.timing.require_current_without_callbacks();
  p.source.require_grid_scene_origin_without_callbacks(p.timing.actual_scene_without_callbacks());
  client_need(p.timing.tick_sequence_without_callbacks()==p.sequence&&
   p.timing.delta_seconds_without_callbacks()==p.seconds,"SP frame actual timing sample substituted");
  const auto actual=p.client.lock();client_need(actual&&actual.get()==p.client_get,
   "SP frame SAME canonical client allocation absent");
  actual->require_frame_current_without_callbacks(p.scope);
  p.lease.require_scope_identity_without_callbacks(p.scope);p.source.require_origin_without_callbacks();
 }catch(...){p.failed.store(true);p.lease.invalidate();throw;}
}
float BnwSpClientUpdateLoan::delta_seconds_without_callbacks()const{
 require_current_without_callbacks();return impl_->seconds;
}
std::uint64_t BnwSpClientUpdateLoan::tick_sequence_without_callbacks()const{
 require_current_without_callbacks();return impl_->sequence;
}
void BnwSpClientUpdateLoan::execute_owned(){
 auto& p=*impl_;require_current_without_callbacks();
 client_need(!p.entered&&!p.consumed,"SP frame consumer cannot replay");p.entered=true;
 try{
  const auto actual=p.client.lock();client_need(actual&&actual.get()==p.client_get,"SP frame client expired before Update");
  actual->update_tick_owned(*this);require_current_without_callbacks();
  p.entered=false;p.consumed=true;
 }catch(...){p.failed.store(true);p.lease.invalidate();throw;}
}
void BnwLocalClientScalarOwner::update_tick_owned(const BnwSpClientUpdateLoan& loan){
 const auto& p=*loan.impl_;loan.require_current_without_callbacks();
 require_frame_current_without_callbacks(p.scope);
 try{
  if(frame_update_entered_){poisoned_.store(true);throw std::runtime_error("SP frame SAME client reentered Update");}
  need(p.sequence>last_frame_sequence_,"SP frame actual loop sample already attempted or replaced");
  // This is borrow accounting, not the original native frame counter or an ACK.
  frame_update_entered_=true;last_frame_sequence_=p.sequence;
  // Actual53FC65 first mandatory native store. uint32 overflow preserves INC bits.
  before_write();frame_counter_+=std::uint32_t{1};
  loan.require_current_without_callbacks();
  need(imported_pregame_!=nullptr,"SP Update requires genuine imported Game/client boot");
  imported_pregame_->update_actual_client_frame(p.scope,loan,p.seconds);
  loan.require_current_without_callbacks();
  // Strict current production prerequisite gateway:6AA190/components/load
  // state/150 must actually return before the53C990 timer call. Its missing
  // provider refuses here; this method never infers a completed prefix.
  require_actual_frame_postcontext_prefix_owned(loan);
  loan.require_current_without_callbacks();
  execute_frame_game_timer_owned(loan);
  loan.require_current_without_callbacks();require_frame_game_origin_owned(p.scope);
  need(p.entered&&!p.consumed&&p.postcontext_prefix_returned&&p.game_timer_returned&&
   p.client_get==this&&frame_update_entered_,
   "SP normal53FCCF requires genuine post-context/timer return and SAME outer borrow");
  // Only normal original53C990->53FCCF return retires this private borrow.
  // No native store/counter or receipt is granted, and native histories are
  // never reset. A required suffix failure leaves the entered loan poisoned.
  frame_update_entered_=false;
  loan.require_current_without_callbacks();require_frame_game_origin_owned(p.scope);loan.require_current_without_callbacks();
  frame_update_entered_=false; // Loan's execute_owned issues return only AFTER all suffixes.
 }catch(...){poisoned_.store(true);source_.invalidate();throw;}
}
} // namespace civ5::runtime

namespace civ5::runtime {
void BnwSpClientUpdateLoan::control_registration_callback_owned(std::int32_t actor)const{
 auto& p=*impl_;require_current_without_callbacks();
 try{
  client_need(p.entered&&!p.control_callback_entered&&!p.control_callback_returned,
   "SP actual proxy24 client callback replay/reentry or outside frame");
  const auto actual=p.client.lock();client_need(actual&&actual.get()==p.client_get,
   "SP original proxy24 canonical GameClient retired");
  p.control_callback_entered=true;
  actual->control_registration_callback_owned(*this,actor);
  require_current_without_callbacks();p.control_callback_returned=true;
 }catch(...){p.failed.store(true);p.lease.invalidate();throw;}
}
void BnwLocalClientScalarOwner::control_registration_callback_owned(
 const BnwSpClientUpdateLoan& loan,std::int32_t actor){
 const auto& p=*loan.impl_;loan.require_current_without_callbacks();
 require_frame_current_without_callbacks(p.scope);
 try{
  need(frame_update_entered_&&p.control_callback_entered&&!p.control_callback_returned&&imported_pregame_,
   "SP original683250 requires SAME entered Update/retained imported client");
  imported_pregame_->begin_actual_control_callback(p.scope,loan,actor);
  loan.require_current_without_callbacks();
  // Original68325B unconditional636860 event146, before local-ID read.
  imported_pregame_->publish_actual_control_client_event(p.scope,loan,146);
  loan.require_current_without_callbacks();
  const auto actual_local=imported_pregame_->actual_control_client_local_id(p.scope,loan);
  loan.require_current_without_callbacks();
  if(actual_local==actor){
   imported_pregame_->publish_actual_control_client_event(p.scope,loan,145);
   loan.require_current_without_callbacks();
  }
  // Native683293..6832AA includes all four equal stores for valid actors. These are
  // source-defined frame-client arrays, not imported CvPlayer/ready DTOs.
  if(actor>=0&&actor<64){const auto index=static_cast<std::size_t>(actor);
   before_write();frame_18c_[index]=0;
   before_write();frame_1cc_[index]=0;
   before_write();frame_20c_[index]=-1;
   before_write();frame_30c_[index]=-1;
  }
  loan.require_current_without_callbacks();
  imported_pregame_->complete_actual_control_arrays(p.scope,loan);
  loan.require_current_without_callbacks();
  const bool native_c8=imported_pregame_->actual_control_client_c8(p.scope,loan);
  loan.require_current_without_callbacks();
  // Native6832C7 takes the sender branch immediately on true. False instead
  // obtains SAME info twice and calls actual7F3510 isHost before deciding.
  if(!native_c8){
   const bool actual_host=imported_pregame_->actual_control_client_is_host(p.scope,loan);
   loan.require_current_without_callbacks();
   if(!actual_host)throw std::runtime_error("SP client mandatory STOP before original59FD50 nonhost UI/application effects");
  }
  const auto sender_local=imported_pregame_->actual_control_client_sender_local_id(p.scope,loan);
  loan.require_current_without_callbacks();
  if(actor!=sender_local){
   throw std::runtime_error("SP client mandatory original683250 remote3F3 bundle serializer/sender unjoined");
  }
  // Original53FAF0(actor,true) and actual inner+4C submission are mandatory.
  // This client registration message155 never issues GameCore EndTurn.
  imported_pregame_->submit_actual_client155(p.scope,loan);
  loan.require_current_without_callbacks();
  imported_pregame_->complete_actual_control_local_ui_return(p.scope,loan);
  loan.require_current_without_callbacks();
  // Only SAME local683250 callback returns here. The receiver still requires
  // original494940/8B7E30 optional name/UI component and Control destruction.
  // A queued155 is not processed and no frame/EndTurn completion is issued.
 }catch(...){poisoned_.store(true);source_.invalidate();throw;}
}
}

namespace civ5::runtime {
void BnwSpClientUpdateLoan::control_proxy_optional_ui_owned()const{
 auto& p=*impl_;require_current_without_callbacks();
 try{
  client_need(p.entered&&p.control_callback_returned&&!p.proxy_ui_entered&&!p.proxy_ui_returned,
   "SP original494940 optional UI suffix outside actual returned683250 or replay");
  const auto actual=p.client.lock();client_need(actual&&actual.get()==p.client_get,"SP optional UI canonical client absent");
  p.proxy_ui_entered=true;actual->control_proxy_optional_ui_owned(*this);
  require_current_without_callbacks();p.proxy_ui_returned=true;
 }catch(...){p.failed.store(true);p.lease.invalidate();throw;}
}
void BnwLocalClientScalarOwner::control_proxy_optional_ui_owned(const BnwSpClientUpdateLoan& loan){
 const auto& p=*loan.impl_;loan.require_current_without_callbacks();require_frame_current_without_callbacks(p.scope);
 try{
  need(p.control_callback_returned&&p.proxy_ui_entered&&!p.proxy_ui_returned&&imported_pregame_,
   "SP actual494940 requires SAME completed local callback before optional component read");
  imported_pregame_->require_actual_control_proxy_ui(p.scope,loan);loan.require_current_without_callbacks();
  need(native_client_profile_==NativeClientProfile::selected_civis_launcher&&legacy_components_constructed_,
   "SP native legacy component profile/init absent");
  //494961 reads SAME actual component08, then8B7E30 tests it FIRST.
  // The genuine constructor-null slot returns at8B7E40->8B7F66 without names,
  // allocations orB8 callback. No current real Renderer/UI readiness is read.
  if(legacy_component08_)throw std::runtime_error("SP nonnull legacy component08 requires original8B7E30 names/B8 owner");
  loan.require_current_without_callbacks();
 }catch(...){poisoned_.store(true);source_.invalidate();throw;}
}
}

namespace civ5::runtime {
void BnwSpClientUpdateLoan::client155_registration_callback_owned()const{
 auto& p=*impl_;require_current_without_callbacks();
 try{
  client_need(p.entered&&p.control_callback_returned&&p.proxy_ui_returned&&
   !p.client155_callback_entered&&!p.client155_callback_returned,
   "SP155 canonical client callback outside actual current frame or reentered");
  const auto actual=p.client.lock();client_need(actual&&actual.get()==p.client_get,
   "SP155 genuine SAME canonical client absent");
  p.client155_callback_entered=true;actual->client155_registration_callback_owned(*this);
  require_current_without_callbacks();p.client155_callback_returned=true;
 }catch(...){p.failed.store(true);p.lease.invalidate();throw;}
}
void BnwLocalClientScalarOwner::client155_registration_callback_owned(const BnwSpClientUpdateLoan& loan){
 const auto& p=*loan.impl_;loan.require_current_without_callbacks();require_frame_current_without_callbacks(p.scope);
 try{
  need(p.client155_callback_entered&&!p.client155_callback_returned&&imported_pregame_,
   "SP155 actual4764E0 lacks typed canonical caller/imported namespace");
  const auto actor=imported_pregame_->begin_actual_client155_callback(p.scope,loan);
  loan.require_current_without_callbacks();
  //4764EF->71AA30 performs unsigned actor<=63 check and an equal byte store.
  // It does not apply decoded byte34 here: original call argument is true.
  if(static_cast<std::uint32_t>(actor)<=63){before_write();frame_1cc_[static_cast<std::size_t>(actor)]=1;}
  imported_pregame_->complete_actual_client155_ready_store(p.scope,loan);loan.require_current_without_callbacks();
  imported_pregame_->publish_actual_client155_event(p.scope,loan,146);loan.require_current_without_callbacks();
  const auto actual_local=imported_pregame_->actual_client155_local_id(p.scope,loan);loan.require_current_without_callbacks();
  if(actor==actual_local){imported_pregame_->publish_actual_client155_event(p.scope,loan,132);loan.require_current_without_callbacks();}
  imported_pregame_->require_actual_client155_core_suffix(p.scope,loan);loan.require_current_without_callbacks();
  //476511 genuine selected App1460 owner; actual62F200 first acquires
  // Pregame, reads+D4 isNetworkMultiplayerGame, Destroy, then branches.
  require_engine_utility_owned(p.lease);loan.require_current_without_callbacks();
  const bool network=imported_pregame_->actual_engine_network_game_owned(p.scope,loan);
  loan.require_current_without_callbacks();require_engine_utility_owned(p.lease);
  if(network)throw std::runtime_error("SP155 actual Utility3AC multiplayer participant/UI effects remain mandatory");
  // Actual non-network branch62F3B2 returnsfalse without further effects;
  //476529 ignores that result. Only now can decoded34/UI suffix execute.
  imported_pregame_->complete_actual_client155_local_return(p.scope,loan);
  loan.require_current_without_callbacks();
 }catch(...){poisoned_.store(true);source_.invalidate();throw;}
}
}

namespace civ5::runtime {
bool BnwLocalClientScalarOwner::frame_engine_turn_boundary_owned(const BnwSpClientUpdateLoan& loan){
 const auto& p=*loan.impl_;loan.require_current_without_callbacks();require_engine_utility_owned(p.lease);
 try{
  need(p.entered&&frame_update_entered_&&imported_pregame_,"SP Utility3F4 outside actual client frame");
  // Native623B00 acquires Game, +14GetGameTurn, reads SAME C88DE4 and
  // compares BEFORE Destroy. Turn provider owns this existing InterfaceLoan.
  InterfaceLoan game(*this,p.lease);const auto turn=game.game_turn();game.current();
  const auto last=imported_pregame_->actual_engine_scheduler_last_turn_owned(p.scope,loan);game.current();
  const bool result=turn==last;
  game.return_owned();loan.require_current_without_callbacks();require_engine_utility_owned(p.lease);return result;
 }catch(...){poisoned_.store(true);source_.invalidate();throw;}
}
bool BnwLocalClientScalarOwner::frame_has_pending_load_vector_owned(const BnwSpClientUpdateLoan& loan,std::int32_t actor){
 const auto& p=*loan.impl_;loan.require_current_without_callbacks();require_frame_current_without_callbacks(p.scope);
 try{
  need(p.entered&&frame_update_entered_&&frame_pending_vectors_constructed_,
   "SP native740630 lacks actual retained client/vector constructor");
  //740630 searches128 first, then118; both are vector<int> stride4.
  // Original accepts its int operand unchanged (no invented actor bounds).
  for(const auto value:frame_pending128_)if(value==actor){loan.require_current_without_callbacks();return true;}
  for(const auto value:frame_pending118_)if(value==actor){loan.require_current_without_callbacks();return true;}
  loan.require_current_without_callbacks();return false;
 }catch(...){poisoned_.store(true);source_.invalidate();throw;}
}
}

namespace civ5::runtime {
void BnwSpClientUpdateLoan::require_imported_cache_owned(const BnwImportedPregameReadCache& cache)const{
 const auto& p=*impl_;require_current_without_callbacks();
 try{
  const auto actual=p.client.lock();
  client_need(p.entered&&actual&&actual.get()==p.client_get&&actual->imported_pregame_&&
   actual->imported_pregame_.get()==&cache,
   "SP outer frame readonly supplier is not SAME canonical client imported Cache");
  actual->require_frame_game_origin_owned(p.scope);require_current_without_callbacks();
 }catch(...){p.failed.store(true);p.lease.invalidate();throw;}
}
}

#include "bnw_frame_game_timer_loop.inc"

namespace civ5::runtime {
void BnwLocalClientScalarOwner::require_frame_native_proxy00_owned(const BnwSpClientUpdateLoan& loan)const{
 const auto& p=*loan.impl_;loan.require_current_without_callbacks();require_frame_game_origin_owned(p.scope);
 try{
  const auto proxy=frame_native_proxy00_.lock();
  need(frame_native_proxy00_published_&&proxy&&imported_pregame_&&proxy.get()==imported_pregame_.get()&&
   !proxy.owner_before(imported_pregame_)&&!imported_pregame_.owner_before(proxy),
   "SP actual client00 SAME retained Boot namespace binding retired/substituted");
  loan.require_imported_cache_owned(*proxy);require_frame_game_origin_owned(p.scope);loan.require_current_without_callbacks();
 }catch(...){poisoned_.store(true);source_.invalidate();throw;}
}
bool BnwLocalClientScalarOwner::frame_loadstate_turn_boundary_owned(const BnwSpClientUpdateLoan& loan){
 const auto& p=*loan.impl_;loan.require_current_without_callbacks();require_engine_utility_owned(p.lease);
 try{
  need(p.entered&&frame_update_entered_,"SP native608A30 outside actual entered frame");
  require_frame_native_proxy00_owned(loan);
  InterfaceLoan game(*this,p.lease);const auto turn=game.game_turn();game.current();
  const auto last=imported_pregame_->actual_frame_loadstate_last_turn_owned(p.scope,loan);game.current();
  const bool equal=turn==last; // native608A60 compares BEFORE GameDestroy.
  game.return_owned();loan.require_current_without_callbacks();require_frame_native_proxy00_owned(loan);return equal;
 }catch(...){poisoned_.store(true);source_.invalidate();throw;}
}
}
