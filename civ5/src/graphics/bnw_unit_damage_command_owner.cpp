#include "civ5/graphics/bnw_unit_damage_command_owner.hpp"
#include "civ5/runtime/bnw_attrition_damage_dll_wrapper_prefix.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include "civ5/runtime/bnw_unitflag_living_health_read.hpp"
#include "civ5/runtime/bnw_owned_gameplay_event_pump_loan.hpp"
#include <atomic>
#include <exception>
#include <optional>
#include <list>
#include <mutex>
#include <stdexcept>
namespace civ5::graphics {
namespace {void demand(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
template<class A,class B>bool same_control(const A&a,const B&b){return !a.owner_before(b)&&!b.owner_before(a);}}
struct BnwUnitDamageCommandOwner::Impl {
 std::mutex lock;std::list<std::shared_ptr<Receipt>>commands;
 // Genuine fresh Civis queue constructor baseline; never imported native-global values.
 std::uint32_t sequence=0;std::uint64_t occurrence=0;bool busy=false,scene_owned=false;
 std::atomic<bool>alive{true},failed{false};std::weak_ptr<HealthStorage>active_health;
};
BnwUnitDamageCommandOwner::BnwUnitDamageCommandOwner(Binding b):impl_(std::make_shared<Impl>()),binding_(std::move(b)){}
BnwUnitDamageCommandOwner::~BnwUnitDamageCommandOwner(){impl_->alive=false;impl_->failed=true;}
void BnwUnitDamageCommandOwner::require_origin_without_callbacks()const{
 demand(creator_==std::this_thread::get_id(),"Damage command immutable creator differs");
 const auto scene=binding_.scene.lock();const auto gui=binding_.gui.lock();
 demand(scene&&gui&&scene->gui_thread_==creator_,"Damage command real Scene/GUI lost or foreign");
 gui->require_origin_lifetime_without_frame_callbacks(); // No old frame/draw currentness requirement.
 const auto host=binding_.host.lock();const auto world=binding_.world.lock();
 demand(host&&world&&binding_.core.lock()&&binding_.source.lock()&&binding_.control.lock()&&
  scene->gui_self_.lock()==scene&&same_control(scene->gui_self_,scene)&&scene->gui_screen_.lock()==gui&&same_control(scene->gui_screen_,gui)&&
  scene->selected_host_==host&&same_control(scene->selected_host_,host)&&scene->scene_state_==world.get()&&
  scene->scene_state_generation_==binding_.generation&&&scene->session_.state()==world.get()&&
  scene->session_.state_generation()==binding_.generation,"Damage command SAME canonical Scene/Host/world/generation/control differs");
 if(impl_->scene_owned)demand(scene->unit_damage_commands_.get()==this&&same_control(scene->unit_damage_commands_,self_),"Damage command canonical Scene queue was replaced");
 else demand(!scene->unit_damage_commands_,"Damage command unbound factory cannot borrow another Scene queue");
 demand(!impl_->failed,"Damage command sticky failed queue");
}
std::shared_ptr<BnwUnitDamageCommandOwner>BnwUnitDamageCommandOwner::bind_owned(const Binding&b){
 const auto scene=b.scene.lock();demand(scene&&scene->gui_thread_==std::this_thread::get_id(),"Damage command binding foreign immutable Scene creator");
 if(const auto current=scene->unit_damage_commands_){
  current->require_origin_without_callbacks();
  demand(same_control(current->binding_.scene,b.scene)&&same_control(current->binding_.gui,b.gui)&&same_control(current->binding_.host,b.host)&&
   same_control(current->binding_.world,b.world)&&same_control(current->binding_.core,b.core)&&same_control(current->binding_.source,b.source)&&
   same_control(current->binding_.control,b.control)&&current->binding_.generation==b.generation,"Damage command cannot rebind owned queue to another source");return current;
 }
 auto queue=std::shared_ptr<BnwUnitDamageCommandOwner>(new BnwUnitDamageCommandOwner(b));queue->self_=queue;
 queue->require_origin_without_callbacks();scene->unit_damage_commands_=queue;queue->impl_->scene_owned=true;return queue;
}
std::shared_ptr<const BnwUnitDamageCommandOwner::Receipt>BnwUnitDamageCommandOwner::append_owned(
 const std::shared_ptr<const runtime::BnwAttritionDamageDllWrapperPrefix>&issuer,const Payload&payload,
 const void*unit,const void*wrapper,std::uint64_t operation,std::uint64_t revision){
 require_origin_without_callbacks();
 if(impl_->busy){impl_->failed=true;throw std::runtime_error("Damage command caught append reentry");}
 impl_->busy=true;
 try{
  demand(issuer&&unit&&wrapper,"Damage command closed Core operands absent");
  issuer->require_command_binding_without_callbacks(*this);
  issuer->require_command_payload_without_callbacks(payload.owner,payload.id,payload.new_damage,payload.previous_damage,payload.max_hit_points,unit,wrapper,operation,revision);
  auto receipt=std::make_shared<Receipt>();receipt->owner=self_;receipt->issuer=issuer;receipt->payload=payload;
  receipt->health_source=issuer->capture_unitflag_health_for_actual_append_without_callbacks();
  demand(bool(receipt->health_source),"Actual Damage append living HP custody missing");
  receipt->native_unit=unit;receipt->wrapper=wrapper;receipt->operation=operation;receipt->revision=revision;receipt->generation=binding_.generation;
  std::list<std::shared_ptr<Receipt>>staged;staged.push_back(receipt);receipt->node=&staged.front();
  {std::lock_guard<std::mutex>guard(impl_->lock);
   require_origin_without_callbacks();issuer->require_command_binding_without_callbacks(*this);
   issuer->require_command_payload_without_callbacks(payload.owner,payload.id,payload.new_damage,payload.previous_damage,payload.max_hit_points,unit,wrapper,operation,revision);
   demand(impl_->occurrence<UINT64_MAX&&!receipt->appended&&payload.header_first==0x80&&payload.header_second==0x48,"Damage command overflow/header/once differs");
   // After this last pure seal: no allocation, callback or throwing operation.
   // Native 32-bit sequence increment wraps; separate Civis occurrence is strict.
   receipt->sequence=impl_->sequence+std::uint32_t{1};receipt->occurrence=impl_->occurrence+1;
   impl_->commands.splice(impl_->commands.end(),staged);impl_->sequence=receipt->sequence;impl_->occurrence=receipt->occurrence;receipt->appended=true;
  } // Actual lock release precedes original utility-call return.
  receipt->lock_released=true;impl_->busy=false;return receipt;
 }catch(...){impl_->busy=false;impl_->failed=true;throw;}
}
void BnwUnitDamageCommandOwner::record_source_adoption_without_callbacks(const Receipt&r,const runtime::BnwAttritionDamageDllWrapperPrefix&issuer)const noexcept{
 if(creator_!=std::this_thread::get_id()||impl_->busy||impl_->failed||r.owner.lock().get()!=this||r.issuer.lock().get()!=&issuer||
  !r.appended||!r.lock_released||r.source_adopted||r.occurrence!=impl_->occurrence||r.sequence!=impl_->sequence)std::terminate();
 r.source_adopted=true; // No callbacks between actual Source/Cursor adoption and this earned command seal.
}
void BnwUnitDamageCommandOwner::require_return_without_callbacks(const Receipt&r)const{
 require_origin_without_callbacks();demand(!impl_->busy&&r.owner.lock().get()==this&&same_control(r.owner,self_)&&r.appended&&r.lock_released&&r.source_adopted&&
  r.occurrence>0&&r.occurrence==impl_->occurrence&&r.sequence==impl_->sequence&&r.generation==binding_.generation,"Damage command real append/lock-release receipt absent");
 bool found=false;for(const auto&entry:impl_->commands)if(entry.get()==&r){demand(r.node==&entry,"Damage command retained list allocation differs");found=true;}
 demand(found,"Damage command actual retained command node absent");
}

struct BnwUnitDamageCommandOwner::HealthStorage final {
 const std::thread::id creator=std::this_thread::get_id();
 const std::shared_ptr<Impl>queue;const Binding binding;
 const std::weak_ptr<BnwUnitDamageCommandOwner>facade;
 const std::shared_ptr<const Receipt>receipt;
 const runtime::BnwStockLuaAttemptLease stock;
 const runtime::BnwStockLuaAttemptLease::ScopeIdentity identity;
 const runtime::BnwOwnedGameplayEventPumpLoan*const pump;
 const runtime::BnwSelectedStockLuaSourceOwner*const source;
 const runtime::BnwSelectedStockLuaBootstrap*const vm;
 bool entered=false,consumed=false,checking=false;
 HealthStorage(std::shared_ptr<Impl>q,Binding b,std::weak_ptr<BnwUnitDamageCommandOwner>f,
  std::shared_ptr<const Receipt>r,const runtime::BnwStockLuaAttemptLease&s,
  runtime::BnwStockLuaAttemptLease::ScopeIdentity i,const runtime::BnwOwnedGameplayEventPumpLoan&p,
  const runtime::BnwSelectedStockLuaSourceOwner&o,const runtime::BnwSelectedStockLuaBootstrap&v)
  :queue(std::move(q)),binding(std::move(b)),facade(std::move(f)),receipt(std::move(r)),stock(s),
   identity(std::move(i)),pump(&p),source(&o),vm(&v){}
};
}
namespace civ5::runtime {
struct BnwUnitFlagHealthFifoLoan::State final {
 const std::thread::id creator=std::this_thread::get_id();std::atomic<bool>alive{true};
 const std::shared_ptr<graphics::BnwUnitDamageCommandOwner::HealthStorage>storage;
 explicit State(std::shared_ptr<graphics::BnwUnitDamageCommandOwner::HealthStorage>s):storage(std::move(s)){}
};
BnwUnitFlagHealthFifoLoan::BnwUnitFlagHealthFifoLoan(std::shared_ptr<State>s):state_(std::move(s)){}
BnwUnitFlagHealthFifoLoan::~BnwUnitFlagHealthFifoLoan(){
 state_->alive=false;
 if(state_->creator!=std::this_thread::get_id()||!state_->storage->consumed)
  graphics::BnwUnitDamageCommandOwner::fail_health_fifo_without_callbacks(state_);
}
std::shared_ptr<BnwUnitFlagHealthFifoLoan::State>BnwUnitFlagHealthFifoLoan::retain_storage_without_callbacks()const{
 const auto s=state_;require_origin_from_storage_without_callbacks(s);return s;
}
void BnwUnitFlagHealthFifoLoan::require_origin_from_storage_without_callbacks(const std::shared_ptr<State>&s){graphics::BnwUnitDamageCommandOwner::require_health_fifo_without_callbacks(s);}
std::uint64_t BnwUnitFlagHealthFifoLoan::occurrence_from_storage_without_callbacks(const std::shared_ptr<State>&s){
 graphics::BnwUnitDamageCommandOwner::require_health_fifo_without_callbacks(s);
 const auto occurrence=s->storage->receipt->occurrence;
 graphics::BnwUnitDamageCommandOwner::require_health_fifo_without_callbacks(s);return occurrence;
}
std::pair<std::int32_t,std::int32_t>BnwUnitFlagHealthFifoLoan::damage_pair_from_storage_without_callbacks(const std::shared_ptr<State>&s){return graphics::BnwUnitDamageCommandOwner::health_damage_pair_from_fifo_without_callbacks(s);}
void BnwUnitFlagHealthFifoLoan::require_current_from_storage(const std::shared_ptr<State>&s){graphics::BnwUnitDamageCommandOwner::require_health_fifo_without_callbacks(s);}
std::shared_ptr<const BnwUnitFlagLivingHealthRead>BnwUnitFlagHealthFifoLoan::health_source_from_storage_without_callbacks(const std::shared_ptr<State>&s){return graphics::BnwUnitDamageCommandOwner::health_source_from_fifo_without_callbacks(s);}
std::pair<std::int32_t,std::int32_t>BnwUnitFlagHealthFifoLoan::owner_full_id_from_storage_without_callbacks(const std::shared_ptr<State>&s){return graphics::BnwUnitDamageCommandOwner::health_full_id_from_fifo_without_callbacks(s);}
void BnwUnitFlagHealthFifoLoan::require_origin_without_callbacks()const{const auto s=state_;require_origin_from_storage_without_callbacks(s);}
void BnwUnitFlagHealthFifoLoan::require_current()const{const auto s=state_;require_current_from_storage(s);}
std::shared_ptr<const BnwUnitFlagLivingHealthRead>BnwUnitFlagHealthFifoLoan::health_source_without_callbacks()const{const auto s=state_;return health_source_from_storage_without_callbacks(s);}
std::pair<std::int32_t,std::int32_t>BnwUnitFlagHealthFifoLoan::owner_full_id_without_callbacks()const{const auto s=state_;return owner_full_id_from_storage_without_callbacks(s);}
BnwUnitFlagHealthFifoLoan::DeliveryReturn::DeliveryReturn(DeliveryReturn&&)noexcept=default;
BnwUnitFlagHealthFifoLoan::DeliveryReturn::~DeliveryReturn()=default;
// DeliveryReturn(actual Loan,OriginalReturnedEvent) remains RootTurn-defined.
// There is no accepting constructor or fabricated protected-event completion here.
}
namespace civ5::graphics {
void BnwUnitDamageCommandOwner::fail_health_fifo_without_callbacks(const std::shared_ptr<runtime::BnwUnitFlagHealthFifoLoan::State>&s)noexcept{
 if(!s)return;const auto q=s->storage;
 q->queue->failed=true;
 if(const auto control=q->stock.control_.lock())control->invalidated=true;
}
void BnwUnitDamageCommandOwner::require_health_fifo_without_callbacks(const std::shared_ptr<runtime::BnwUnitFlagHealthFifoLoan::State>&s){
 try{
  demand(bool(s),"Health FIFO saved occurrence absent");
  // Immutable creator and independent atomics precede all mutable/borrowed state.
  if(s->creator!=std::this_thread::get_id()){fail_health_fifo_without_callbacks(s);throw std::runtime_error("Health FIFO foreign creator");}
  demand(s->alive.load()&&s->storage->queue->alive.load()&&!s->storage->queue->failed.load(),"Health FIFO facade retired or failed");
  const auto q=s->storage;
  demand(q->entered&&!q->consumed&&q->queue->busy&&q->queue->active_health.lock().get()==q.get(),"Health FIFO entered occurrence replaced or consumed");
  {const auto owner=q->facade.lock();demand(owner&&owner->impl_.get()==q->queue.get(),"Health FIFO canonical queue facade lost");
   owner->require_origin_without_callbacks();
   demand(same_control(q->facade,owner->self_),"Health FIFO queue control substituted");}
  demand(!q->queue->commands.empty()&&q->queue->commands.front().get()==q->receipt.get()&&
   q->receipt->node==&q->queue->commands.front()&&q->receipt->owner.lock().get()==q->facade.lock().get()&&
   same_control(q->receipt->owner,q->facade)&&q->receipt->appended&&q->receipt->lock_released&&q->receipt->source_adopted&&
   q->receipt->occurrence>0&&q->receipt->generation==q->binding.generation&&q->receipt->health_source,
   "Health FIFO genuine adopted head/allocation differs");
  q->stock.require_scope_identity_without_callbacks(q->identity);
  {const auto world=q->binding.world.lock();const auto source=q->binding.source.lock();const auto control=q->binding.control.lock();
   demand(world&&source&&source.get()==q->source&&control&&control.get()==q->identity.control.get()&&same_control(control,q->identity.control)&&
    q->identity.creator==q->creator&&q->identity.vm==q->vm&&q->identity.world==world.get()&&q->identity.serial>0,
    "Health FIFO SAME active Stock VM/world/source/control differs");
   q->receipt->health_source->require_same_world_without_callbacks(world.get(),q->binding.generation);}
  demand(q->receipt->health_source->owner_full_id_without_callbacks()==std::make_pair(q->receipt->payload.owner,q->receipt->payload.id),
   "Health FIFO reader owner/full-ID differs from actual append");
 }catch(...){fail_health_fifo_without_callbacks(s);throw;}
}
void BnwUnitDamageCommandOwner::require_health_fifo_current(const std::shared_ptr<runtime::BnwUnitFlagHealthFifoLoan::State>&s){
 require_health_fifo_without_callbacks(s);const auto q=s->storage;
 if(q->checking){fail_health_fifo_without_callbacks(s);throw std::runtime_error("Health FIFO callback validation reentry");}
 q->checking=true;
 try{
  q->pump->require_source_owner_without_callbacks(*q->source);require_health_fifo_without_callbacks(s);
  q->pump->require_for_source(q->stock);require_health_fifo_without_callbacks(s);
  q->checking=false;
 }catch(...){fail_health_fifo_without_callbacks(s);q->checking=false;throw;}
}
std::shared_ptr<const runtime::BnwUnitFlagLivingHealthRead>BnwUnitDamageCommandOwner::health_source_from_fifo_without_callbacks(const std::shared_ptr<runtime::BnwUnitFlagHealthFifoLoan::State>&s){
 require_health_fifo_without_callbacks(s);const auto result=s->storage->receipt->health_source;require_health_fifo_without_callbacks(s);return result;
}
std::pair<std::int32_t,std::int32_t>BnwUnitDamageCommandOwner::health_full_id_from_fifo_without_callbacks(const std::shared_ptr<runtime::BnwUnitFlagHealthFifoLoan::State>&s){
 require_health_fifo_without_callbacks(s);const auto result=s->storage->receipt->health_source->owner_full_id_without_callbacks();require_health_fifo_without_callbacks(s);return result;
}
std::pair<std::int32_t,std::int32_t>BnwUnitDamageCommandOwner::health_damage_pair_from_fifo_without_callbacks(const std::shared_ptr<runtime::BnwUnitFlagHealthFifoLoan::State>&s){
 require_health_fifo_without_callbacks(s);
 const auto&payload=s->storage->receipt->payload;const auto reader=s->storage->receipt->health_source;
 const auto result=std::make_pair(payload.new_damage,payload.previous_damage);
 demand(reader->damage_without_callbacks()==result.first&&
  reader->owner_full_id_without_callbacks()==std::make_pair(payload.owner,payload.id)&&
  reader->max_hit_points_without_callbacks()==payload.max_hit_points,"Health FIFO actual new/previous payload and living HP/full-ID differ");
 require_health_fifo_without_callbacks(s);return result;
}
bool BnwUnitDamageCommandOwner::has_pending_health_owned()const{
 // Immutable creator FIRST. A foreign refusal changes only sticky atomics.
 if(creator_!=std::this_thread::get_id()){impl_->failed=true;throw std::runtime_error("Health pending query foreign immutable creator");}
 const auto queue=impl_;
 try{
  require_origin_without_callbacks();
  demand(queue->alive.load()&&!queue->busy,"Health pending query retired or reentered queue");
  const bool pending=!queue->commands.empty();
  require_origin_without_callbacks();
  demand(queue->alive.load()&&!queue->busy,"Health pending query queue changed before return");
  return pending; // Data only; actual append/adoption/cleanup loan gates remain separate.
 }catch(...){queue->failed=true;throw;}
}
void BnwUnitDamageCommandOwner::with_health_fifo_owned(const runtime::BnwOwnedGameplayEventPumpLoan&pump,
 const runtime::BnwStockLuaAttemptLease&stock,const runtime::BnwSelectedStockLuaSourceOwner&source,
 const runtime::BnwSelectedStockLuaBootstrap&vm,
 const std::function<runtime::BnwUnitFlagHealthFifoLoan::DeliveryReturn(const runtime::BnwUnitFlagHealthFifoLoan&)>&callback){
 const auto queue=impl_;
 if(creator_!=std::this_thread::get_id()){queue->failed=true;if(const auto c=stock.control_.lock())c->invalidated=true;throw std::runtime_error("Health FIFO immutable entry creator differs");}
 const auto q=[&]{try{
  require_origin_without_callbacks();const auto binding=binding_;const auto facade=self_;
  if(queue->busy){queue->failed=true;throw std::runtime_error("Health FIFO entry reentry");}
  demand(!queue->commands.empty()&&callback,"Health FIFO real pending head/callback absent");
  auto identity=stock.pin_active_scope_without_callbacks();
  // Pure prerequisites before publication. No target COPY or Source callback yet.
  demand(identity.vm==&vm&&identity.serial>0&&identity.creator==creator_,"Health FIFO actual Bootstrap/Stock stage differs");
  return std::make_shared<HealthStorage>(queue,binding,facade,queue->commands.front(),stock,std::move(identity),pump,source,vm);
 }catch(...){queue->failed=true;if(const auto c=stock.control_.lock())c->invalidated=true;throw;}}();
 const auto s=std::make_shared<runtime::BnwUnitFlagHealthFifoLoan::State>(q);
 runtime::BnwUnitFlagHealthFifoLoan loan(s);q->entered=true;queue->busy=true;queue->active_health=q;
 std::optional<std::function<runtime::BnwUnitFlagHealthFifoLoan::DeliveryReturn(const runtime::BnwUnitFlagHealthFifoLoan&)>>target;
 std::optional<runtime::BnwUnitFlagHealthFifoLoan::DeliveryReturn>returned;
 try{
  require_health_fifo_current(s);
  target.emplace(callback);require_health_fifo_without_callbacks(s);require_health_fifo_current(s);
  returned.emplace((*target)(loan));require_health_fifo_without_callbacks(s);require_health_fifo_current(s);
  target.reset();require_health_fifo_without_callbacks(s);require_health_fifo_current(s);
  demand(returned->state_.get()==s.get()&&returned->occurrence_==q->receipt->occurrence,
   "Health FIFO genuine original-event return belongs to another occurrence");
  returned.reset();require_health_fifo_without_callbacks(s);require_health_fifo_current(s);
  // Final pure seal after target/result DTOR and full real callback POST.
  require_health_fifo_without_callbacks(s);
  queue->commands.pop_front();q->consumed=true;q->entered=false;queue->active_health.reset();queue->busy=false;
 }catch(...){
  const auto error=std::current_exception();
  try{require_health_fifo_without_callbacks(s);}catch(...){}
  target.reset();
  try{require_health_fifo_without_callbacks(s);}catch(...){}
  returned.reset();
  try{require_health_fifo_without_callbacks(s);}catch(...){}
  fail_health_fifo_without_callbacks(s);
  // Queue storage is saved even if callbacks retired facade/Scene. Foreign FIRST
  // cannot reach this mutable creator-owned cleanup from this synchronous call.
  queue->active_health.reset();queue->busy=false;q->entered=false;
  std::rethrow_exception(error);
 }
}
}
