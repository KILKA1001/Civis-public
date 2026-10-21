#include "civ5/graphics/bnw_unit_setup_graphical_transport.hpp"
#include "civ5/runtime/bnw_pending_created_unit_read.hpp"
#include "civ5/graphics/bnw_initial_unit_setup_graphical_renderer_loan.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include <list>
#include <limits>
#include <optional>
#include <stdexcept>
#include <thread>
namespace civ5::graphics {
namespace {
void setup_need(bool v,const char*t){if(!v)throw std::runtime_error(t);}
template<class A,class B>bool setup_control(const A&a,const B&b){return !a.owner_before(b)&&!b.owner_before(a);}
}
struct BnwUnitSetupGraphicsState::State {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<BnwUnitSetupGraphicsState> self;
 std::weak_ptr<const ui::BnwOwnedCameraGuiScreen> gui;
 std::weak_ptr<const BnwOwnedCameraRendererWindowLease> window;
 std::optional<bool> initialized;
 std::uint64_t epoch=0;
 std::size_t pins=0;
 bool alive=true,failed=false,busy=false;
};
BnwUnitSetupGraphicsState::BnwUnitSetupGraphicsState(std::shared_ptr<State>s):state_(std::move(s)){}
BnwUnitSetupGraphicsState::~BnwUnitSetupGraphicsState(){if(state_->creator!=std::this_thread::get_id())std::terminate();retire();}
void BnwUnitSetupGraphicsState::retire()noexcept{
 if(state_->creator!=std::this_thread::get_id())std::terminate();
 state_->alive=false;state_->initialized.reset();
}
void BnwUnitSetupGraphicsState::pure()const{
 const auto&s=*state_;setup_need(s.creator==std::this_thread::get_id(),"Graphics-state foreign creator");
 const auto self=s.self.lock();
 setup_need(s.alive&&!s.failed&&self&&self.get()==this&&setup_control(self,s.self)&&
  self.use_count()>static_cast<long>(s.pins+1),"Graphics-state canonical outside owner retired");
}
std::shared_ptr<BnwUnitSetupGraphicsState>BnwUnitSetupGraphicsState::capture_actual_renderer_origin(
 const BnwOwnedCameraRendererWindowLease::Loan&window,const ui::BnwOwnedCameraGuiScreen&gui){
 auto data=std::make_shared<State>();
 // Both retainers are genuine factory seams. Compare supplied GUI before callback.
 {
  const auto canonical_gui=gui.retain_owner();
  setup_need(canonical_gui.get()==&gui,"Graphics-state GUI substituted");
  const auto canonical_window=window.retain_owner();
  data->gui=canonical_gui;data->window=canonical_window;
 } // Pure identity locks die before callbackful full(), and cannot mask loss.
 auto result=std::shared_ptr<BnwUnitSetupGraphicsState>(new BnwUnitSetupGraphicsState(data));data->self=result;
 // Construction does not publish false. The actual lifecycle owner must call
 // publish_before_actual_renderer_initialize at its genuine false assignment.
 result->full();return result;
}
void BnwUnitSetupGraphicsState::full()const{
 pure();const auto s=state_;
 auto gui=s->gui.lock();auto window=s->window.lock();
 setup_need(gui&&window&&gui->thread_==s->creator&&window->thread_==s->creator,"Graphics-state GUI/window retired");
 setup_need(gui->pins_!=std::numeric_limits<std::size_t>::max()&&
  window->self_pins_!=std::numeric_limits<std::size_t>::max(),"Graphics-state origin pins exhausted");
 ++gui->pins_;++window->self_pins_;
 struct End {std::shared_ptr<const ui::BnwOwnedCameraGuiScreen>g;std::shared_ptr<const BnwOwnedCameraRendererWindowLease>w;
  ~End(){--g->pins_;--w->self_pins_;}}end{gui,window};
 // End's strong owners are the SAME pin, moved below rather than duplicated.
 gui.reset();window.reset();
 const auto&g=end.g;const auto&w=end.w;
 setup_need(g.use_count()>static_cast<long>(g->pins_)&&w.use_count()>static_cast<long>(w->self_pins_),
  "Graphics-state origin outside owners lost");
 g->require_origin_lifetime_without_frame_callbacks();w->require_origin_lifetime_without_native_callbacks();
 pure();w->require_current();pure();g->require_current();pure();g->require_same_window(*w);pure();
 g->require_origin_lifetime_without_frame_callbacks();w->require_origin_lifetime_without_native_callbacks();
}
void BnwUnitSetupGraphicsState::publish_before_actual_renderer_initialize(){
 pure();const auto s=state_;if(s->busy){s->failed=true;throw std::runtime_error("Graphics lifecycle reentry");}
 s->busy=true;struct End{std::shared_ptr<State>s;~End(){s->busy=false;}}end{s};
 try{setup_need(!s->initialized&&s->epoch==0,"Graphics false publication replay");full();
  s->initialized=false;++s->epoch;full();}catch(...){s->failed=true;throw;}
}
void BnwUnitSetupGraphicsState::publish_after_actual_renderer_initialize(){
 pure();const auto s=state_;if(s->busy){s->failed=true;throw std::runtime_error("Graphics lifecycle reentry");}
 s->busy=true;struct End{std::shared_ptr<State>s;~End(){s->busy=false;}}end{s};
 try{setup_need(s->initialized.has_value()&&!*s->initialized&&s->epoch==1,"Graphics true publication lacks actual false phase");
  full();s->initialized=true;++s->epoch;full();}catch(...){s->failed=true;throw;}
}
bool BnwUnitSetupGraphicsState::read_is_graphics_initialized()const{
 pure();setup_need(state_->initialized.has_value(),"IsGraphicsInitialized actual publisher UNJOINED");return *state_->initialized;
}
std::uint64_t BnwUnitSetupGraphicsState::revision()const{pure();return state_->epoch;}
void BnwUnitSetupGraphicsState::require_same_gui(const ui::BnwOwnedCameraGuiScreen&g)const{
 pure();const auto actual=state_->gui.lock();
 setup_need(actual&&actual.get()==&g&&setup_control(actual,g.self_),"Graphics-state different actual GUI");
}
BnwUnitSetupGraphicsState::BorrowedOwner::BorrowedOwner(std::shared_ptr<BnwUnitSetupGraphicsState>o):owner_(std::move(o)){
 setup_need(bool(owner_),"Graphics-state absent");owner_->pure();
 setup_need(owner_->state_->pins!=std::numeric_limits<std::size_t>::max(),"Graphics-state pin overflow");++owner_->state_->pins;
}
BnwUnitSetupGraphicsState::BorrowedOwner::~BorrowedOwner(){
 if(owner_->state_->creator!=std::this_thread::get_id())std::terminate();--owner_->state_->pins;
}
const BnwUnitSetupGraphicsState&BnwUnitSetupGraphicsState::BorrowedOwner::current()const{owner_->pure();return *owner_;}
struct BnwUnitSetupGraphicalQueue::Batch {
 const std::uint64_t serial;
 const std::vector<BnwUnitSetupGraphicalEvent> events;
 const std::shared_ptr<const runtime::BnwPendingCreatedUnitRead>pending_read;
 Batch(std::uint64_t s,std::vector<BnwUnitSetupGraphicalEvent>e,std::shared_ptr<const runtime::BnwPendingCreatedUnitRead>r)
  :serial(s),events(std::move(e)),pending_read(std::move(r)){}
};
struct BnwUnitSetupGraphicalQueue::State {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<BnwUnitSetupGraphicalQueue>self;
 std::weak_ptr<ui::BnwClientSceneFrame>scene;
 const ui::BnwClientSceneFrame*scene_get=nullptr;
 const CreatedOccurrenceLoan*created_loan=nullptr;
 std::uint64_t created_drain_serial=0;
 std::list<std::shared_ptr<const Batch>>pending;
 std::uint64_t serial=0;std::size_t pins=0;
 bool alive=true,failed=false,busy=false;
};
struct BnwUnitSetupGraphicalQueue::Pin {
 std::shared_ptr<BnwUnitSetupGraphicalQueue>owner;
 explicit Pin(const std::weak_ptr<BnwUnitSetupGraphicalQueue>&w):owner(w.lock()){
  setup_need(bool(owner),"Setup graphical queue absent");owner->pure();
  setup_need(owner->state_->pins!=std::numeric_limits<std::size_t>::max(),"Setup queue pins exhausted");++owner->state_->pins;
 }
 ~Pin(){if(owner->state_->creator!=std::this_thread::get_id())std::terminate();--owner->state_->pins;}
};
BnwUnitSetupGraphicalQueue::BnwUnitSetupGraphicalQueue(std::shared_ptr<State>s):state_(std::move(s)){}
BnwUnitSetupGraphicalQueue::~BnwUnitSetupGraphicalQueue(){if(state_->creator!=std::this_thread::get_id())std::terminate();state_->alive=false;}
void BnwUnitSetupGraphicalQueue::pure()const{
 const auto&s=*state_;setup_need(s.creator==std::this_thread::get_id(),"Setup queue foreign creator");
 const auto actual=s.self.lock();setup_need(s.alive&&!s.failed&&actual&&actual.get()==this&&
  setup_control(actual,s.self)&&actual.use_count()>static_cast<long>(s.pins+1),"Setup queue canonical outside owner lost");
}
std::shared_ptr<BnwUnitSetupGraphicalQueue>BnwUnitSetupGraphicalQueue::bind_actual_scene(const ui::BnwClientSceneFrame&scene){
 setup_need(scene.gui_thread_==std::this_thread::get_id(),"Setup queue Scene foreign creator");
 const auto canonical=scene.gui_self_.lock();
 setup_need(canonical&&canonical.get()==&scene&&setup_control(canonical,scene.gui_self_),"Setup queue Scene canonical origin missing");
 auto data=std::make_shared<State>();data->scene=canonical;data->scene_get=canonical.get();
 auto result=std::shared_ptr<BnwUnitSetupGraphicalQueue>(new BnwUnitSetupGraphicalQueue(data));data->self=result;
 result->pure();return result;
}
void BnwUnitSetupGraphicalQueue::require_same_scene(const ui::BnwClientSceneFrame&scene)const{
 pure();const auto actual=state_->scene.lock();
 setup_need(actual&&actual.get()==&scene&&setup_control(actual,scene.gui_self_),"Setup queue different actual Scene");
}
void BnwUnitSetupGraphicalQueue::require_same_factory(const std::weak_ptr<BnwUnitSetupGraphicalQueue>&q)const{
 pure();setup_need(setup_control(q,state_->self),"Setup queue factory control changed");
}
void BnwUnitSetupGraphicalQueue::quarantine_from_owned_weak(const std::weak_ptr<BnwUnitSetupGraphicalQueue>&q)noexcept{
 const auto actual=q.lock();if(!actual)return;
 if(actual->state_->creator!=std::this_thread::get_id())std::terminate();
 actual->state_->failed=true;
}
BnwUnitSetupGraphicalQueue::Completed::Completed(std::weak_ptr<BnwUnitSetupGraphicalQueue>q,
 std::shared_ptr<const Batch>b,std::uint64_t s):queue_(std::move(q)),batch_(std::move(b)),serial_(s){}
void BnwUnitSetupGraphicalQueue::Completed::require_member(const BnwInitialUnitSetupGraphicalRendererLoan&loan)const{
 Pin pin(queue_);loan.require_dispatch_current();pin.owner->pure();loan.require_queue_identity(*pin.owner);
 setup_need(batch_&&batch_->serial==serial_&&serial_<=pin.owner->state_->serial,"Setup queue return serial changed");
 bool found=false;for(const auto&e:pin.owner->state_->pending)if(e.get()==batch_.get()&&setup_control(e,batch_)){found=true;break;}
 setup_need(found,"Setup queue return no longer an actual member");
 loan.require_dispatch_current();pin.owner->pure();
}
void BnwUnitSetupGraphicalQueue::preflight_from_owned_weak(const std::weak_ptr<BnwUnitSetupGraphicalQueue>&q,
 const BnwInitialUnitSetupGraphicalRendererLoan&loan){
 Pin pin(q);loan.require_dispatch_current();pin.owner->pure();loan.require_queue_identity(*pin.owner);
 setup_need(!pin.owner->state_->busy,"Setup queue active append");loan.require_dispatch_current();pin.owner->pure();
}
std::shared_ptr<const BnwUnitSetupGraphicalQueue::Completed>BnwUnitSetupGraphicalQueue::enqueue_from_owned_weak(
 const std::weak_ptr<BnwUnitSetupGraphicalQueue>&q,const BnwInitialUnitSetupGraphicalRendererLoan&loan,
 std::vector<BnwUnitSetupGraphicalEvent>events,std::shared_ptr<const runtime::BnwPendingCreatedUnitRead>pending_read){
 Pin pin(q);auto s=pin.owner->state_;
 if(s->busy){s->failed=true;throw std::runtime_error("Setup queue append reentry");}
 s->busy=true;struct End{std::shared_ptr<State>s;~End(){s->busy=false;}}end{s};
 try{
  loan.require_dispatch_current();pin.owner->pure();loan.require_queue_identity(*pin.owner);
  setup_need(!events.empty()&&s->serial!=UINT64_MAX,"Setup queue empty/serial exhausted");
  const bool created=events.size()==1&&events.front().kind==BnwUnitSetupGraphicalEventKind::created;
  setup_need(created==bool(pending_read),"Original created enqueue requires exactly its genuine pending read");
  if(created){const auto ids=pending_read->issued_owner_full_id_without_callbacks();
   setup_need(ids.first==events.front().owner&&ids.second==events.front().full_id,"Created enqueue pending full-ID mismatch");}
  auto batch=std::make_shared<const Batch>(s->serial+1,std::move(events),std::move(pending_read));
  auto proof=std::shared_ptr<const Completed>(new Completed(q,batch,batch->serial));
  std::list<std::shared_ptr<const Batch>>staged;staged.push_back(batch);
  loan.require_dispatch_current();pin.owner->pure();
  // All throwing allocations precede actual owned membership; splice does not allocate.
  s->pending.splice(s->pending.end(),staged);s->serial=batch->serial;
  loan.require_dispatch_current();pin.owner->pure();proof->require_member(loan);
  if(batch->pending_read)batch->pending_read->require_issuing_plan_without_callbacks();return proof;
 }catch(...){s->failed=true;throw;}
}
}

#include "bnw_unit_setup_created_pending_lifetime.inc"
#include "bnw_unit_setup_created_occurrence.inc"
#include "bnw_unit_setup_created_pending_read.inc"
#include "bnw_unit_setup_created_pending_reference.inc"
