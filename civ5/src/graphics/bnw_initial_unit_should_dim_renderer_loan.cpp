#include "civ5/graphics/bnw_initial_unit_should_dim_renderer_loan.hpp"
#include "civ5/graphics/bnw_unit_should_dim_transport.hpp"
#include "civ5/runtime/bnw_live_game_context.hpp"
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
// Core55->56->57 concrete Plan and counted Core borrower ABI.
// Missing full Plan/Source inventory remains gated; selection/UI is OPEN.
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include <atomic>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
namespace civ5::graphics {
namespace {
void dim_need(bool v,const char* text){if(!v)throw std::runtime_error(text);}
template<class A,class B>bool dim_control(const A& a,const B& b){return !a.owner_before(b)&&!b.owner_before(a);}
}
struct BnwInitialUnitShouldDimRendererLoan::State final {
 using Core=runtime::BnwNativeUnitAllocatorCore;
 using Plan=Core::InitialUnitShouldDimPlan;
 using Context=runtime::BnwLiveGameContext;
 struct Identity {
  const Core* core=nullptr;
  std::weak_ptr<const Core> core_control;
  std::weak_ptr<const ui::SelectedBnwPreviewHost> host;
  runtime::LiveSession* session=nullptr;
  std::weak_ptr<const runtime::LiveSessionState> world;
  const saves::SavedSession* source=nullptr;
  std::weak_ptr<const runtime::BnwUnitMovePending> pending;
  const void* bank=nullptr;std::weak_ptr<const void> bank_control;
  const void* node=nullptr;std::weak_ptr<const void> node_control;
  std::int32_t owner=-1,id=-1,slot=-1,before=0,requested=0;
  std::uint64_t generation=0,operation=0;
  explicit Identity(const Plan::Binding& b):core(b.core),core_control(b.core_control),host(b.host),session(b.session),
   world(b.state),source(b.source),pending(b.pending),bank(b.bank),bank_control(b.bank_control),node(b.node),
   node_control(b.node_control),owner(b.owner),id(b.full_id),slot(b.slot),before(b.before_moves),requested(b.requested_moves),
   generation(b.generation),operation(b.attempt_serial){}
  void match(const Plan::Binding& b)const{
   dim_need(core==b.core&&dim_control(core_control,b.core_control)&&dim_control(host,b.host)&&session==b.session&&
    dim_control(world,b.state)&&source==b.source&&dim_control(pending,b.pending)&&bank==b.bank&&
    dim_control(bank_control,b.bank_control)&&node==b.node&&dim_control(node_control,b.node_control)&&
    owner==b.owner&&id==b.full_id&&slot==b.slot&&before==b.before_moves&&requested==b.requested_moves&&
    generation==b.generation&&operation==b.attempt_serial,"ShouldDim authentic Plan lineage/operands changed");
  }
 };
 // Independent per-actual-Game-control scope. A recursive mutex alone does
 // not grant nested graphics dispatch, including another Scene for same Host.
 struct GameScope {
  const std::thread::id creator=std::this_thread::get_id();
  std::weak_ptr<Context> context;
  bool alive=true,failed=false,destroyed=false;
  std::size_t pins=0;
 };
 static std::shared_ptr<GameScope> acquire_game_scope(const std::shared_ptr<Context>& context){
  static std::mutex mutex;
  static std::map<std::weak_ptr<Context>,std::shared_ptr<GameScope>,std::owner_less<std::weak_ptr<Context>>> scopes;
  std::lock_guard lock(mutex);
  auto& slot=scopes[context];const auto prior=slot;
  if(prior&&(prior->alive||prior->failed)){prior->failed=true;throw std::runtime_error("ShouldDim canonical Game scope reentry/uncertain");}
  auto scope=std::make_shared<GameScope>();scope->context=context;slot=scope;
  for(auto it=scopes.begin();it!=scopes.end();)if(it->first.expired())it=scopes.erase(it);else ++it;
  return scope;
 }
 const std::thread::id creator=std::this_thread::get_id();
 std::shared_ptr<ui::BnwClientSceneFrame> scene;
 std::shared_ptr<const ui::BnwOwnedCameraGuiScreen> gui;
 std::shared_ptr<const ui::BnwClientFrame> frame;
 std::unique_ptr<Core::ShouldDimCoreBorrowedOwner> core_borrow;
 const Core* core=nullptr;
 const runtime::BnwUnitMovePending* pending=nullptr;
 const runtime::BnwStockLuaAttemptLease attempt;
 std::optional<runtime::BnwStockLuaAttemptLease::ScopeIdentity> scope;
 std::weak_ptr<BnwUnitShouldDimQueue> queue;
 std::weak_ptr<const Plan> plan;
 std::optional<Identity> identity;
 std::shared_ptr<BnwUnitShouldDimSourceLoan> source_loan;
 std::shared_ptr<GameScope> game_scope;
 std::weak_ptr<Context> game_context;
 std::shared_ptr<const BnwUnitShouldDimEnqueueProof> queued;
 const BnwInitialUnitShouldDimRendererLoan* loan=nullptr;
 std::uint64_t serial=0;
 bool active=false,failed=false,busy=false,preflighted=false,enqueued=false,returning=false,game_final=false;
 bool counted_scene=false,counted_gui=false;
 State(const ui::BnwClientSceneFrame& supplied,const Core& c,const runtime::BnwUnitMovePending& p,
   const runtime::BnwStockLuaAttemptLease& a):pending(&p),attempt(a){
  dim_need(supplied.gui_thread_==creator,"ShouldDim foreign Scene creator");
  if(supplied.should_dim_renderer_action_){supplied.should_dim_renderer_failed_=true;throw std::runtime_error("ShouldDim Scene reentry");}
  dim_need(!supplied.should_dim_renderer_failed_,"ShouldDim Scene quarantined");
  scene=supplied.gui_self_.lock();
  dim_need(scene&&scene.get()==&supplied&&dim_control(scene,supplied.gui_self_)&&
   scene.use_count()>static_cast<long>(scene->gui_publication_pins_.load()+1),"ShouldDim Scene outside facade absent");
  gui=scene->gui_screen_.lock();frame=scene->frame_;queue=scene->should_dim_queue_;
  dim_need(gui&&frame&&gui->thread_==creator&&dim_control(gui,gui->self_)&&
   gui.use_count()>static_cast<long>(gui->pins_+1),"ShouldDim canonical GUI origin absent");
  scope.emplace(attempt.pin_active_scope_without_callbacks());
  dim_need(scope->creator==creator&&scope->control&&scope->source_owner&&
   dim_control(scope->control->selected_host,scene->selected_host_),"ShouldDim actual Source/Host differs");
  // The authentic retain method creator-checks before canonical weak access.
  // Move its only returned strong lock into actual Core57's counted owner.
  core_borrow.reset(new Core::ShouldDimCoreBorrowedOwner(c.retain_factory_owner()));
  core=&core_borrow->current();dim_need(core==&c,"ShouldDim Core not canonical factory");
 }
 void initialize(const std::shared_ptr<State>& self){
  dim_need(scene->should_dim_renderer_serial_!=UINT64_MAX,"ShouldDim Scene serial exhausted");
  serial=++scene->should_dim_renderer_serial_;
  dim_need(gui->pins_!=std::numeric_limits<std::size_t>::max(),"ShouldDim GUI pins exhausted");++gui->pins_;counted_gui=true;
  const auto scene_old=scene->gui_publication_pins_.fetch_add(1);
  if(scene_old==std::numeric_limits<std::size_t>::max()){scene->gui_publication_pins_.fetch_sub(1);throw std::runtime_error("ShouldDim Scene pins exhausted");}
  counted_scene=true;
  const std::weak_ptr<State> weak=self;
  source_loan=std::shared_ptr<BnwUnitShouldDimSourceLoan>(new BnwUnitShouldDimSourceLoan(attempt,[weak]{
   const auto actual=weak.lock();dim_need(actual!=nullptr,"ShouldDim composite lender retired");
   if(actual->returning)actual->returned_current();else actual->dispatch_current();
  }),[](BnwUnitShouldDimSourceLoan* p){delete p;});
 }
 void pure()const{
  dim_need(creator==std::this_thread::get_id()&&active&&!failed&&loan&&
   scene->should_dim_renderer_action_==loan&&scene->should_dim_renderer_serial_==serial&&
   !scene->should_dim_renderer_failed_,"ShouldDim independent lender retired/reentered");
  dim_need(scene.use_count()>static_cast<long>(scene->gui_publication_pins_.load())&&dim_control(scene,scene->gui_self_)&&
   gui.use_count()>static_cast<long>(gui->pins_)&&dim_control(gui,gui->self_)&&gui->thread_==creator&&
   &core_borrow->current()==core&&dim_control(core_borrow->owner_,core->factory_self_),
   "ShouldDim counted Scene/GUI/Core outside owner lost");
  attempt.require_scope_identity_without_callbacks(*scope);
  const auto source_pins=scope->control->transport_source_pins.load();
  dim_need(source_pins<=static_cast<std::uint64_t>(std::numeric_limits<long>::max())&&
   scope->source_owner.use_count()>static_cast<long>(source_pins),"ShouldDim counted Source outside owner lost");
  {const auto actual=scene->gui_screen_.lock();dim_need(actual.get()==gui.get()&&dim_control(actual,gui),"ShouldDim GUI association changed");}
  dim_need(scene->frame_.get()==frame.get()&&dim_control(scene->frame_,frame)&&
   scene->scene_state_==scope->world,"ShouldDim original canonical publication/world changed");
  gui->require_origin_lifetime_without_frame_callbacks();
 }
 void matches(const Plan& actual)const{
  pure();dim_need(identity.has_value(),"ShouldDim actual Plan not preflighted");
  const auto b=actual.capture_binding();identity->match(b);
  dim_need(b.core==core&&dim_control(b.core_control,core_borrow->owner_)&&b.pending.get()==pending&&
   b.host.get()==scene->selected_host_.get()&&dim_control(b.host,scene->selected_host_)&&
   b.session==&scene->session_&&b.state.get()==scope->world&&b.generation==scene->scene_state_generation_&&
   b.attempt_serial==scope->serial,"ShouldDim Plan not SAME Scene/Core/Pending/Source");
 }
 void dispatch_current()const{
  pure();auto borrowed=Plan::BorrowedOwner::from_weak(plan);const auto& actual=borrowed->current();
  actual.require_should_dim_enqueue_dispatch_without_callbacks();matches(actual);
  scene->verify_owned();pure();actual.require_should_dim_enqueue_dispatch_without_callbacks();matches(actual);
 }
 void returned_current()const{
  pure();core_borrow->current().require_initial_should_dim_return_without_callbacks(*pending);
  scene->verify_owned();pure();core_borrow->current().require_initial_should_dim_return_without_callbacks(*pending);
 }
 void game_current(const std::shared_ptr<Context>& context)const{
  pure();dim_need(game_scope&&game_scope->creator==creator&&game_scope->alive&&!game_scope->failed&&
   dim_control(game_scope->context,context)&&dim_control(game_context,context)&&
   context.use_count()>static_cast<long>(game_scope->pins)&&
   scene->selected_host_->bnw_live_game_context_.get()==context.get()&&
   dim_control(scene->selected_host_->bnw_live_game_context_,context),"ShouldDim Game scope/factory outside owner lost");
  context->require_current();pure();
 }
 void full()const{
  pure();source_loan->full();pure();
 }
 void poison()noexcept{
  failed=true;scene->should_dim_renderer_failed_=true;if(game_scope)game_scope->failed=true;
  if(source_loan)source_loan->fail();else attempt.invalidate();
 }
 void close()noexcept{
  const auto* own=loan;active=false;loan=nullptr;
  if(scene&&scene->should_dim_renderer_action_==own)scene->should_dim_renderer_action_=nullptr;
  if(game_scope)game_scope->alive=false;
 }
 ~State(){
  if(creator!=std::this_thread::get_id())std::terminate();
  close();source_loan.reset();
  if(counted_gui)--gui->pins_;if(counted_scene)scene->gui_publication_pins_.fetch_sub(1);
 }
};
struct BnwInitialUnitShouldDimEnqueueProof::State final{
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwInitialUnitShouldDimEnqueueProof> self;
 std::weak_ptr<BnwInitialUnitShouldDimRendererLoan::State> lender;
 std::weak_ptr<const runtime::BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan> plan;
 std::shared_ptr<const BnwUnitShouldDimEnqueueProof> queued;
 bool genuine_final=false,destroyed=false;
};
BnwInitialUnitShouldDimRendererLoan::BnwInitialUnitShouldDimRendererLoan(std::shared_ptr<State> s):state_(std::move(s)){}
BnwInitialUnitShouldDimRendererLoan::~BnwInitialUnitShouldDimRendererLoan(){state_->close();}
BnwInitialUnitShouldDimEnqueueProof::BnwInitialUnitShouldDimEnqueueProof(std::shared_ptr<State> s):state_(std::move(s)){}
std::shared_ptr<const BnwInitialUnitShouldDimEnqueueProof> BnwInitialUnitShouldDimEnqueueProof::retain_factory_owner()const{
 dim_need(state_->creator==std::this_thread::get_id(),"ShouldDim proof foreign creator");const auto self=state_->self.lock();
 dim_need(self&&self.get()==this&&dim_control(self,state_->self),"ShouldDim proof canonical factory retired");return self;
}
void BnwInitialUnitShouldDimEnqueueProof::require_for_plan(const runtime::BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan& supplied)const{
 dim_need(state_->creator==std::this_thread::get_id(),"ShouldDim proof foreign creator");
 const auto self=state_->self.lock();dim_need(self&&self.get()==this&&dim_control(self,state_->self),"ShouldDim proof canonical self substituted");
 const auto lender=state_->lender.lock();dim_need(lender!=nullptr,"ShouldDim proof lender retired");lender->pure();
 runtime::BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::BorrowedOwner borrowed(supplied);
 const auto& actual=borrowed.current();actual.require_should_dim_enqueue_dispatch_without_callbacks();lender->matches(actual);
 // The only extra identity lock is a full-expression temporary. It is gone
 // before queue/Game validators; only counted BorrowedOwner remains retained.
 dim_need(dim_control(state_->plan,actual.retain_factory_owner())&&lender->enqueued&&state_->destroyed&&
  state_->genuine_final==lender->game_final&&state_->queued==lender->queued,
  "ShouldDim proof is not actual gated/queued return");
 // The canonical owner checker lock ends before any callback/validator.
 if(state_->genuine_final){dim_need(bool(state_->queued),"ShouldDim true Game gate lacks queue return");
  state_->queued->require_current_for_source(*lender->source_loan);}
 else dim_need(!state_->queued,"ShouldDim false Game gate fabricated queue member");
 dim_need(lender->game_scope&&lender->game_scope->destroyed,"ShouldDim proof lacks actual Game Destroy");
 const auto context=lender->game_context.lock();dim_need(context!=nullptr,"ShouldDim proof Game factory retired");
 ++lender->game_scope->pins;
 struct End{std::shared_ptr<BnwInitialUnitShouldDimRendererLoan::State::GameScope>s;~End(){--s->pins;}} end{lender->game_scope};
 lender->game_current(context);lender->pure();
 borrowed.current().require_should_dim_enqueue_dispatch_without_callbacks();lender->matches(borrowed.current());
}
void BnwInitialUnitShouldDimEnqueueProof::require_current_for_core(const runtime::BnwNativeUnitAllocatorCore& c)const{
 const auto lender=state_->lender.lock();dim_need(lender&&lender->core==&c,"ShouldDim proof different/retired Core");
 auto borrowed=runtime::BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::BorrowedOwner::from_weak(state_->plan);
 require_for_plan(borrowed->current());
}
void BnwInitialUnitShouldDimRendererLoan::preflight_should_dim_enqueue(const runtime::BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan& supplied)const{
 const auto s=state_;s->pure();if(s->busy){s->poison();throw std::runtime_error("ShouldDim Loan method reentry");}
 s->busy=true;struct End{std::shared_ptr<State>s;~End(){s->busy=false;}} end{s};
 try{
  dim_need(!s->preflighted&&!s->enqueued,"ShouldDim preflight replay");
  State::Plan::BorrowedOwner borrowed(supplied);const auto& actual=borrowed.current();actual.require_before_moves_store_without_callbacks();
  {const auto b=actual.capture_binding();s->identity.emplace(b);}
  s->plan=actual.retain_factory_owner();s->matches(actual);
  BnwUnitShouldDimQueue::preflight_from_owned_weak(s->queue,*s->source_loan);
  s->scene->verify_owned();s->pure();actual.require_before_moves_store_without_callbacks();s->matches(actual);
  s->preflighted=true;
 }catch(...){s->poison();throw;}
}
std::shared_ptr<const BnwInitialUnitShouldDimEnqueueProof> BnwInitialUnitShouldDimRendererLoan::enqueue_should_dim(
 const runtime::BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan& supplied)const{
 const auto s=state_;s->pure();if(s->busy){s->poison();throw std::runtime_error("ShouldDim Loan method reentry");}
 s->busy=true;struct End{std::shared_ptr<State>s;~End(){s->busy=false;}} end{s};
 std::shared_ptr<State::Context> context;
 std::unique_ptr<State::Context::GameReference> game;
 bool context_pin=false,game_pin=false;
 try{
  dim_need(s->preflighted&&!s->enqueued,"ShouldDim enqueue replay/not preflighted");
  State::Plan::BorrowedOwner borrowed(supplied);const auto& actual=borrowed.current();actual.require_should_dim_enqueue_dispatch_without_callbacks();s->matches(actual);
  s->full();context=State::Context::retain_for_actual_host(s->scene->selected_host_);s->pure();
  s->game_scope=State::acquire_game_scope(context);s->game_context=context;++s->game_scope->pins;context_pin=true;
  s->full();s->game_current(context);game=context->get_game();++s->game_scope->pins;game_pin=true;
  s->full();s->game_current(context);game->require_actual_binding(*s->scene->selected_host_);
  s->full();s->game_final=game->is_final_initialized();s->full();game->require_actual_binding(*s->scene->selected_host_);s->game_current(context);
  if(s->game_final){
   actual.require_should_dim_enqueue_dispatch_without_callbacks();s->matches(actual);
   const bool dim=actual.read_dispatch_should_dim_without_callbacks();
   dim_need(dim==(s->identity->requested<=0),"ShouldDim Core bool not authentic post-store getMoves<=0");
   const std::weak_ptr<State> weak=s;
   const auto current=[weak]{const auto a=weak.lock();dim_need(a!=nullptr,"ShouldDim closed Core lender retired");a->dispatch_current();};
   const auto owner=[weak]{const auto a=weak.lock();dim_need(a!=nullptr,"ShouldDim closed owner lender retired");
    a->dispatch_current();auto p=State::Plan::BorrowedOwner::from_weak(a->plan);return p->current().read_dispatch_owner_without_callbacks();};
   const auto id=[weak]{const auto a=weak.lock();dim_need(a!=nullptr,"ShouldDim closed ID lender retired");
    a->dispatch_current();auto p=State::Plan::BorrowedOwner::from_weak(a->plan);return p->current().read_dispatch_full_id_without_callbacks();};
   BnwUnitShouldDimPayloadLoan payload(*s->source_loan,current,owner,id,s->identity->requested);
   s->queued=BnwUnitShouldDimQueue::enqueue_from_owned_weak(s->queue,payload);
   s->full();s->queued->require_current_for_source(*s->source_loan);s->full();
  }
  // Actual false gate bypasses every Unit getter and enqueue. Both branches
  // still perform actual Destroy/ref0 release before their sealed return proof.
  s->full();s->game_current(context);game->destroy();s->full();game->require_destroyed_for(*s->scene->selected_host_);s->game_current(context);
  game.reset();--s->game_scope->pins;game_pin=false;s->full();s->game_current(context);s->game_scope->destroyed=true;
  auto data=std::make_shared<BnwInitialUnitShouldDimEnqueueProof::State>();data->lender=s;data->plan=s->plan;
  data->queued=s->queued;data->genuine_final=s->game_final;data->destroyed=true;
  auto proof=std::shared_ptr<const BnwInitialUnitShouldDimEnqueueProof>(new BnwInitialUnitShouldDimEnqueueProof(data));data->self=proof;
  s->full();s->game_current(context);s->enqueued=true;proof->require_for_plan(actual);
  --s->game_scope->pins;context_pin=false;context.reset();s->pure();return proof;
 }catch(...){s->poison();game.reset();if(game_pin)--s->game_scope->pins;
  context.reset();if(context_pin)--s->game_scope->pins;throw;}
}
void BnwInitialUnitShouldDimRendererLoan::require_should_dim_enqueued_current(
 const runtime::BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan& p,const BnwInitialUnitShouldDimEnqueueProof& proof)const{
 const auto s=state_;s->pure();proof.require_for_plan(p);s->full();s->pure();
 if(s->queued)s->queued->require_current_for_source(*s->source_loan);
 dim_need(s->game_scope&&s->game_scope->destroyed&&s->enqueued,"ShouldDim actual Game Destroy not complete");
 const auto context=s->game_context.lock();dim_need(context!=nullptr,"ShouldDim Game factory retired");++s->game_scope->pins;
 struct End{std::shared_ptr<State::GameScope>s;~End(){--s->pins;}} end{s->game_scope};s->game_current(context);s->pure();
}
}
namespace civ5::ui {
void BnwClientSceneFrame::with_actual_initial_unit_should_dim_renderer(
 const runtime::BnwNativeUnitAllocatorCore& core,const runtime::BnwUnitMovePending& pending,
 const runtime::BnwStockLuaAttemptLease& attempt,
 const std::function<void(const graphics::BnwInitialUnitShouldDimRendererLoan&)>& incoming)const{
 using Loan=graphics::BnwInitialUnitShouldDimRendererLoan;
 auto state=std::make_shared<Loan::State>(*this,core,pending,attempt);Loan loan(state);
 state->loan=&loan;state->active=true;state->scene->should_dim_renderer_action_=&loan;
 std::optional<std::function<void(const Loan&)>> target;
 try{
  state->initialize(state);state->pure();target.emplace(incoming);state->pure();
  if(!*target)throw std::runtime_error("ShouldDim actual Core target absent");
  state->scene->verify_owned();state->pure();(*target)(loan);
  // Actual Core56 setMoves/ShouldDim-prefix return changes which genuine composite
  // guard is used. This does not complete selection/UI or later Init.
  // Core's actual heap Plan is not artificially retained past issuing scope.
  state->returning=true;state->full();target.reset();state->full();state->pure();state->close();
 }catch(...){state->poison();target.reset();state->close();throw;}
}
}
