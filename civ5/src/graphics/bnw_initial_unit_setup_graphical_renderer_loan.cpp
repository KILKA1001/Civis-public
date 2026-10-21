#include "civ5/graphics/bnw_initial_unit_setup_graphical_renderer_loan.hpp"
#include "civ5/graphics/bnw_unit_setup_graphical_transport.hpp"
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include <limits>
#include <optional>
#include <stdexcept>
#include <thread>
namespace civ5::graphics {
namespace {
void setup_need(bool v,const char*t){if(!v)throw std::runtime_error(t);}
template<class A,class B>bool setup_control(const A&a,const B&b){return !a.owner_before(b)&&!b.owner_before(a);}
}
struct BnwInitialUnitSetupGraphicalRendererLoan::State {
 using Core=runtime::BnwNativeUnitAllocatorCore;
 using Plan=Core::InitialUnitSetupGraphicalPlan;
 struct Identity {
  const Core*core;std::weak_ptr<const Core>core_control;
  std::weak_ptr<const ui::SelectedBnwPreviewHost>host;
  runtime::LiveSession*session;std::weak_ptr<const runtime::LiveSessionState>world;
  const saves::SavedSession*source;
  std::weak_ptr<const runtime::BnwUnitMovePending>pending;
  const void*bank;std::weak_ptr<const void>bank_control;
  const void*node;std::weak_ptr<const void>node_control;
  std::int32_t owner,id,slot,x,y;
  std::uint64_t generation,world_revision,input_revision,plot,operation;
  explicit Identity(const Plan::Binding&b):core(b.core),core_control(b.core_control),host(b.host),
   session(b.session),world(b.state),source(b.source),pending(b.pending),bank(b.bank),bank_control(b.bank_control),
   node(b.node),node_control(b.node_control),owner(b.owner),id(b.full_id),slot(b.slot),x(b.x),y(b.y),
   generation(b.generation),world_revision(b.world_revision),input_revision(b.input_revision),plot(b.plot),operation(b.attempt_serial){}
  void matches(const Plan::Binding&b)const{
   setup_need(core==b.core&&setup_control(core_control,b.core_control)&&setup_control(host,b.host)&&
    session==b.session&&setup_control(world,b.state)&&source==b.source&&setup_control(pending,b.pending)&&
    bank==b.bank&&setup_control(bank_control,b.bank_control)&&node==b.node&&setup_control(node_control,b.node_control)&&
    owner==b.owner&&id==b.full_id&&slot==b.slot&&x==b.x&&y==b.y&&generation==b.generation&&
    world_revision==b.world_revision&&input_revision==b.input_revision&&plot==b.plot&&operation==b.attempt_serial,
    "Setup graphical authentic Plan lineage/operands changed");
  }
 };
 const std::thread::id creator=std::this_thread::get_id();
 std::shared_ptr<ui::BnwClientSceneFrame>scene;
 std::shared_ptr<const ui::BnwOwnedCameraGuiScreen>gui;
 std::shared_ptr<const ui::BnwClientFrame>frame;
 std::unique_ptr<Core::ShouldDimCoreBorrowedOwner>core_borrow;
 const Core*core=nullptr;
 const runtime::BnwUnitMovePending*pending;
 const runtime::BnwStockLuaAttemptLease attempt;
 std::optional<runtime::BnwStockLuaAttemptLease::ScopeIdentity>scope;
 std::unique_ptr<BnwUnitSetupGraphicsState::BorrowedOwner>graphics;
 std::uint64_t graphics_revision=0,serial=0;
 std::weak_ptr<BnwUnitSetupGraphicalQueue>queue;
 std::weak_ptr<const Plan>plan;
 std::optional<Identity>identity;
 std::vector<std::shared_ptr<const BnwUnitSetupGraphicalQueue::Completed>>queued;
 const BnwInitialUnitSetupGraphicalRendererLoan*loan=nullptr;
 bool active=false,failed=false,busy=false,preflighted=false,enqueued=false,returning=false;
 std::optional<bool>genuine_graphics;
 bool counted_scene=false,counted_gui=false;
 State(const ui::BnwClientSceneFrame&supplied,const Core&c,const runtime::BnwUnitMovePending&p,
  const runtime::BnwStockLuaAttemptLease&a):pending(&p),attempt(a){
  setup_need(supplied.gui_thread_==creator,"Setup graphical foreign Scene creator");
  if(supplied.setup_graphical_renderer_action_){supplied.setup_graphical_renderer_failed_=true;
   throw std::runtime_error("Setup graphical Scene reentry");}
  setup_need(!supplied.setup_graphical_renderer_failed_,"Setup graphical Scene quarantined");
  scene=supplied.gui_self_.lock();
  setup_need(scene&&scene.get()==&supplied&&setup_control(scene,supplied.gui_self_)&&
   scene.use_count()>static_cast<long>(scene->gui_publication_pins_.load()+1),"Setup graphical outside Scene absent");
  gui=scene->gui_screen_.lock();frame=scene->frame_;
  setup_need(gui&&frame&&gui->thread_==creator&&setup_control(gui,gui->self_)&&
   gui.use_count()>static_cast<long>(gui->pins_+1),"Setup graphical canonical GUI absent");
  scope.emplace(attempt.pin_active_scope_without_callbacks());
  setup_need(scope->creator==creator&&scope->control&&scope->source_owner&&
   setup_control(scope->control->selected_host,scene->selected_host_),"Setup graphical Source/Host differs");
  core_borrow.reset(new Core::ShouldDimCoreBorrowedOwner(c.retain_factory_owner()));
  core=&core_borrow->current();setup_need(core==&c,"Setup graphical Core not canonical factory");
  graphics.reset(new BnwUnitSetupGraphicsState::BorrowedOwner(scene->setup_graphics_state_.lock()));
  graphics->current().require_same_gui(*gui);graphics_revision=graphics->current().revision();
  queue=scene->setup_graphical_queue_;
 }
 void initialize(){
  setup_need(scene->setup_graphical_renderer_serial_!=UINT64_MAX,"Setup graphical Scene serial exhausted");
  serial=++scene->setup_graphical_renderer_serial_;
  setup_need(gui->pins_!=std::numeric_limits<std::size_t>::max(),"Setup graphical GUI pins exhausted");
  ++gui->pins_;counted_gui=true;
  const auto old=scene->gui_publication_pins_.fetch_add(1);
  if(old==std::numeric_limits<std::size_t>::max()){scene->gui_publication_pins_.fetch_sub(1);
   throw std::runtime_error("Setup graphical Scene pins exhausted");}
  counted_scene=true;
 }
 void pure()const{
  setup_need(creator==std::this_thread::get_id()&&active&&!failed&&loan&&
   scene->setup_graphical_renderer_action_==loan&&scene->setup_graphical_renderer_serial_==serial&&
   !scene->setup_graphical_renderer_failed_,"Setup graphical lender retired/reentered");
  setup_need(scene.use_count()>static_cast<long>(scene->gui_publication_pins_.load())&&
   setup_control(scene,scene->gui_self_)&&gui.use_count()>static_cast<long>(gui->pins_)&&
   setup_control(gui,gui->self_)&&gui->thread_==creator&&&core_borrow->current()==core&&
   setup_control(core_borrow->owner_,core->factory_self_),"Setup graphical outside Scene/GUI/Core lost");
  attempt.require_scope_identity_without_callbacks(*scope);
  const auto pins=scope->control->transport_source_pins.load();
  setup_need(pins<=static_cast<std::uint64_t>(std::numeric_limits<long>::max())&&
   scope->source_owner.use_count()>static_cast<long>(pins),"Setup graphical outside Source lost");
  {const auto current=scene->gui_screen_.lock();
   setup_need(current.get()==gui.get()&&setup_control(current,gui),"Setup graphical GUI association changed");}
  setup_need(scene->frame_.get()==frame.get()&&setup_control(scene->frame_,frame)&&
   scene->scene_state_==scope->world,"Setup graphical original frame/world changed");
  gui->require_origin_lifetime_without_frame_callbacks();
  graphics->current().require_same_gui(*gui);
  setup_need(graphics->current().revision()==graphics_revision&&
   setup_control(graphics->owner_,scene->setup_graphics_state_),"Setup graphical lifecycle epoch changed");
 }
 void matches(const Plan&actual)const{
  pure();setup_need(bool(identity),"Setup graphical Plan not captured");
  const auto b=actual.capture_binding();identity->matches(b);
  setup_need(b.core==core&&setup_control(b.core_control,core_borrow->owner_)&&b.pending.get()==pending&&
   b.host.get()==scene->selected_host_.get()&&setup_control(b.host,scene->selected_host_)&&
   b.session==&scene->session_&&b.state.get()==scope->world&&b.source==identity->source&&
   b.generation==scene->scene_state_generation_&&b.attempt_serial==scope->serial,
   "Setup graphical Plan not SAME Scene/Core/Pending/Source");
 }
 void origin_current()const{
  pure();scene->verify_owned();pure();graphics->current().full();pure();
 }
 void dispatch_current()const{
  pure();auto borrowed=Plan::BorrowedOwner::from_weak(plan);const auto&actual=borrowed->current();
  actual.require_setup_graphical_enqueue_dispatch_without_callbacks();matches(actual);
  scene->verify_owned();pure();actual.require_setup_graphical_enqueue_dispatch_without_callbacks();matches(actual);
  graphics->current().full();pure();actual.require_setup_graphical_enqueue_dispatch_without_callbacks();matches(actual);
 }
 void returned_current()const{
  pure();core_borrow->current().require_initial_setup_graphical_return_without_callbacks(*pending);
  scene->verify_owned();pure();graphics->current().full();pure();
  core_borrow->current().require_initial_setup_graphical_return_without_callbacks(*pending);
 }
 void poison()noexcept{failed=true;scene->setup_graphical_renderer_failed_=true;
  BnwUnitSetupGraphicalQueue::quarantine_from_owned_weak(queue);attempt.invalidate();}
 void close()noexcept{
  if(creator!=std::this_thread::get_id())std::terminate();
  const auto*own=loan;active=false;loan=nullptr;
  if(scene&&scene->setup_graphical_renderer_action_==own)scene->setup_graphical_renderer_action_=nullptr;
 }
 ~State(){if(creator!=std::this_thread::get_id())std::terminate();close();
  if(counted_gui)--gui->pins_;if(counted_scene)scene->gui_publication_pins_.fetch_sub(1);}
};
struct BnwInitialUnitSetupGraphicalEnqueueProof::State {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwInitialUnitSetupGraphicalEnqueueProof>self;
 std::weak_ptr<BnwInitialUnitSetupGraphicalRendererLoan::State>lender;
 std::weak_ptr<const runtime::BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan>plan;
 std::vector<std::shared_ptr<const BnwUnitSetupGraphicalQueue::Completed>>queued;
 bool graphics_initialized;
 explicit State(bool b):graphics_initialized(b){}
};
BnwInitialUnitSetupGraphicalRendererLoan::BnwInitialUnitSetupGraphicalRendererLoan(std::shared_ptr<State>s):state_(std::move(s)){}
BnwInitialUnitSetupGraphicalRendererLoan::~BnwInitialUnitSetupGraphicalRendererLoan(){state_->close();}
BnwInitialUnitSetupGraphicalEnqueueProof::BnwInitialUnitSetupGraphicalEnqueueProof(std::shared_ptr<State>s):state_(std::move(s)){}
std::shared_ptr<const BnwInitialUnitSetupGraphicalEnqueueProof>BnwInitialUnitSetupGraphicalEnqueueProof::retain_factory_owner()const{
 setup_need(state_->creator==std::this_thread::get_id(),"Setup proof foreign creator");
 const auto self=state_->self.lock();setup_need(self&&self.get()==this&&setup_control(self,state_->self),
  "Setup proof canonical factory retired");return self;
}
void BnwInitialUnitSetupGraphicalEnqueueProof::require_for_plan(
 const runtime::BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan&supplied)const{
 setup_need(state_->creator==std::this_thread::get_id(),"Setup proof foreign creator");
 const auto self=state_->self.lock();setup_need(self&&self.get()==this&&setup_control(self,state_->self),"Setup proof self substituted");
 const auto lender=state_->lender.lock();setup_need(bool(lender),"Setup proof lender retired");lender->pure();
 runtime::BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::BorrowedOwner borrowed(supplied);
 const auto&actual=borrowed.current();actual.require_setup_graphical_enqueue_dispatch_without_callbacks();lender->matches(actual);
 setup_need(setup_control(state_->plan,actual.retain_factory_owner())&&lender->enqueued&&
  lender->genuine_graphics.has_value()&&state_->graphics_initialized==*lender->genuine_graphics&&state_->queued==lender->queued,
  "Setup proof not actual graphics-gated return");
 lender->dispatch_current();
 if(state_->graphics_initialized){setup_need(!state_->queued.empty(),"Setup true gate missing queue return");
  for(const auto&receipt:state_->queued)receipt->require_member(*lender->loan);}
 else setup_need(state_->queued.empty(),"Setup false gate fabricated queue work");
 actual.require_setup_graphical_enqueue_dispatch_without_callbacks();lender->matches(actual);
}
void BnwInitialUnitSetupGraphicalEnqueueProof::require_current_for_core(const runtime::BnwNativeUnitAllocatorCore&c)const{
 setup_need(state_->creator==std::this_thread::get_id(),"Setup proof foreign creator");
 const auto lender=state_->lender.lock();setup_need(lender&&lender->core==&c,"Setup proof different/retired Core");
 auto borrowed=runtime::BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::BorrowedOwner::from_weak(state_->plan);
 require_for_plan(borrowed->current());
}
void BnwInitialUnitSetupGraphicalRendererLoan::require_dispatch_current()const{state_->dispatch_current();}
void BnwInitialUnitSetupGraphicalRendererLoan::require_queue_identity(const BnwUnitSetupGraphicalQueue&q)const{
 state_->pure();q.require_same_scene(*state_->scene);
 q.require_same_factory(state_->queue);
}
void BnwInitialUnitSetupGraphicalRendererLoan::preflight_setup_graphical_enqueue(
 const runtime::BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan&supplied)const{
 const auto s=state_;s->pure();if(s->busy){s->poison();throw std::runtime_error("Setup Loan reentry");}
 s->busy=true;struct End{std::shared_ptr<State>s;~End(){s->busy=false;}}end{s};
 try{setup_need(!s->preflighted&&!s->enqueued,"Setup preflight replay");
  State::Plan::BorrowedOwner borrowed(supplied);const auto&actual=borrowed.current();
  actual.require_setup_graphical_enqueue_dispatch_without_callbacks();
  {const auto b=actual.capture_binding();s->identity.emplace(b);}
  s->plan=actual.retain_factory_owner();s->matches(actual);s->dispatch_current();s->preflighted=true;
 }catch(...){s->poison();throw;}
}
std::shared_ptr<const BnwInitialUnitSetupGraphicalEnqueueProof>BnwInitialUnitSetupGraphicalRendererLoan::enqueue_setup_graphical(
 const runtime::BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan&supplied)const{
 const auto s=state_;s->pure();if(s->busy){s->poison();throw std::runtime_error("Setup Loan reentry");}
 s->busy=true;struct End{std::shared_ptr<State>s;~End(){s->busy=false;}}end{s};
 try{
  setup_need(s->preflighted&&!s->enqueued,"Setup enqueue replay/unpreflighted");
  State::Plan::BorrowedOwner borrowed(supplied);const auto&actual=borrowed.current();s->dispatch_current();
  s->genuine_graphics=s->graphics->current().read_is_graphics_initialized();s->dispatch_current();
  if(*s->genuine_graphics){
   actual.prepare_native_readers();s->dispatch_current();
   BnwUnitSetupGraphicalQueue::preflight_from_owned_weak(s->queue,*this);
   s->queued.reserve(7);
   // Every native reader is ordered and bracketed by the actual composite.
   const auto read=[&](auto member){s->dispatch_current();const auto v=(actual.*member)();s->dispatch_current();return v;};
   const bool embarked=read(&State::Plan::read_dispatch_embarked_without_callbacks);
   const auto append=[&](BnwUnitSetupGraphicalEventKind kind,std::int32_t first=0,std::int32_t second=0){
    const auto owner=read(&State::Plan::read_dispatch_owner_without_callbacks);
    const auto id=read(&State::Plan::read_dispatch_full_id_without_callbacks);
    setup_need(owner==s->identity->owner&&id==s->identity->id,"Setup genuine Unit wrapper owner/ID changed");
    std::vector<BnwUnitSetupGraphicalEvent>event;event.reserve(1);event.push_back({kind,owner,id,first,second});
    auto receipt=BnwUnitSetupGraphicalQueue::enqueue_from_owned_weak(s->queue,*this,std::move(event));
    // Capacity for the exact maximum seven calls was reserved before the first
    // queue effect. Record each real append BEFORE the next SDK Unit reader.
    s->queued.push_back(std::move(receipt));s->dispatch_current();
    s->queued.back()->require_member(*this);s->dispatch_current();
   };
   if(embarked)append(BnwUnitSetupGraphicalEventKind::embark,1);
   else{
    // Original incoming Unit.GetPlot/fog then Game.activeTeam/IsInvisible(true).
    const auto owner=read(&State::Plan::read_dispatch_owner_without_callbacks);
    const auto id=read(&State::Plan::read_dispatch_full_id_without_callbacks);
    const auto fog=read(&State::Plan::read_dispatch_original_fog_without_callbacks);
    const auto not_invisible=read(&State::Plan::read_dispatch_original_not_invisible_without_callbacks);
    setup_need(owner==s->identity->owner&&id==s->identity->id,"Created payload SAME pending Unit changed");
    const auto pending_read=actual.capture_created_pending_read_without_callbacks();s->dispatch_current();
    std::vector<BnwUnitSetupGraphicalEvent>created;created.reserve(1);
    created.push_back({BnwUnitSetupGraphicalEventKind::created,owner,id,0,0,fog,not_invisible});
    auto created_receipt=BnwUnitSetupGraphicalQueue::enqueue_from_owned_weak(s->queue,*this,std::move(created),pending_read);
    s->queued.push_back(std::move(created_receipt));s->dispatch_current();
    s->queued.back()->require_member(*this);s->dispatch_current();
    append(BnwUnitSetupGraphicalEventKind::should_dim,read(&State::Plan::read_dispatch_should_dim_without_callbacks)?1:0);
    if(read(&State::Plan::read_dispatch_fortify_turns_without_callbacks)>0)append(BnwUnitSetupGraphicalEventKind::fortify,1);
    const auto damage=read(&State::Plan::read_dispatch_damage_without_callbacks);
    if(damage>0)append(BnwUnitSetupGraphicalEventKind::set_damage,damage,0);
    if(read(&State::Plan::read_dispatch_is_work_without_callbacks)){
     const auto build=read(&State::Plan::read_dispatch_build_type_without_callbacks);
     if(build!=-1)append(BnwUnitSetupGraphicalEventKind::work,build);
    }
    // Actual Core dynamic IsGarrisoned getter, never a saved garrison scalar shortcut.
    if(read(&State::Plan::read_dispatch_is_garrisoned_without_callbacks)){
     append(BnwUnitSetupGraphicalEventKind::visibility,0);
     append(BnwUnitSetupGraphicalEventKind::garrison,1);
    }
   }
   for(const auto&receipt:s->queued){s->dispatch_current();receipt->require_member(*this);s->dispatch_current();}
  }
  // False was read from issued graphics state. No Unit getter, queue preflight
  // or append occurs on this branch; absence/UNISSUED cannot reach a proof.
  auto data=std::make_shared<BnwInitialUnitSetupGraphicalEnqueueProof::State>(*s->genuine_graphics);
  data->lender=s;data->plan=s->plan;data->queued=s->queued;
  auto proof=std::shared_ptr<const BnwInitialUnitSetupGraphicalEnqueueProof>(new BnwInitialUnitSetupGraphicalEnqueueProof(data));
  data->self=proof;s->dispatch_current();s->enqueued=true;proof->require_for_plan(actual);return proof;
 }catch(...){s->poison();throw;}
}
void BnwInitialUnitSetupGraphicalRendererLoan::require_setup_graphical_enqueued_current(
 const runtime::BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan&p,const BnwInitialUnitSetupGraphicalEnqueueProof&proof)const{
 const auto s=state_;s->pure();proof.require_for_plan(p);s->dispatch_current();s->pure();
}
}
namespace civ5::ui {
void BnwClientSceneFrame::with_actual_initial_unit_setup_graphical_renderer(
 const runtime::BnwNativeUnitAllocatorCore&core,const runtime::BnwUnitMovePending&pending,
 const runtime::BnwStockLuaAttemptLease&attempt,
 const std::function<void(const graphics::BnwInitialUnitSetupGraphicalRendererLoan&)>&incoming)const{
 using Loan=graphics::BnwInitialUnitSetupGraphicalRendererLoan;
 auto state=std::make_shared<Loan::State>(*this,core,pending,attempt);Loan loan(state);
 state->loan=&loan;state->active=true;state->scene->setup_graphical_renderer_action_=&loan;
 std::optional<std::function<void(const Loan&)>>target;
 try{
  state->initialize();state->origin_current();
  target.emplace(incoming);state->origin_current();
  if(!*target)throw std::runtime_error("Setup actual Core target absent");
  (*target)(loan);state->returning=true;state->returned_current();
  target.reset();state->returned_current();state->pure();state->close();
 }catch(...){state->poison();target.reset();state->close();throw;}
}
}
