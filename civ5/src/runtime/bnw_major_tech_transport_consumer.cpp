#include "civ5/runtime/bnw_major_tech_transport_consumer.hpp"
#include "civ5/runtime/bnw_barbarian_team_has_tech.hpp"
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include "civ5/ui/live_ui_host.hpp"
#include "civ5/ui/bnw_tech_award_input_dispatch.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include "civ5/ui/bnw_tech_award_modal_snapshot.hpp"
#include "civ5/ui/bnw_tech_award_context_snapshot.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/runtime/live_session.hpp"
#include <array>
#include <atomic>
#include <limits>
#include <thread>
#include <utility>
namespace civ5::runtime {
namespace {
void need(bool ok,const char* message){if(!ok)throw BnwMajorTechTransportUnavailable(message);}
template<class A,class B>bool same(const A& a,const B& b){
 return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
}
}
struct BnwMajorTechClientEndpoint::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 mutable std::atomic<bool> retired{false},busy{false};
 std::weak_ptr<const BnwMajorTechClientEndpoint> self;
 std::weak_ptr<ui::LiveUiHost> ui;
 std::weak_ptr<const ui::SelectedBnwPreviewHost> host;
 std::weak_ptr<ui::BnwClientSelectionControl> selection;
 std::weak_ptr<const LiveSessionState> world;
 std::weak_ptr<const BnwMajorTeamHasTechPending> core;
 std::array<const ui::UiContextDescriptor*,2> contexts{};
 std::uint64_t generation=0;
 std::vector<std::shared_ptr<const BnwMajorTechTransportRequest>> requests;
 std::size_t completed=0;
 struct Guard {
  const Impl& p;
  explicit Guard(const Impl& q):p(q){
   if(q.thread!=std::this_thread::get_id()){q.retired.store(true);throw BnwMajorTechTransportUnavailable("Technology client foreign thread");}
   need(!q.retired.load(),"Technology client retired");
   bool expected=false;
   if(!q.busy.compare_exchange_strong(expected,true)){q.retired.store(true);throw BnwMajorTechTransportUnavailable("Technology client receiver reentry");}
  }
  ~Guard(){p.busy.store(false);}
 };
};
struct BnwMajorTechTransportRequest::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::weak_ptr<const BnwMajorTechClientEndpoint> endpoint;
 std::weak_ptr<const BnwMajorTeamHasTechPending> core;
 BnwStockLuaAttemptLease lease;
 std::uint64_t serial=0,revision=0;
 std::size_t index=0;
 BnwMajorTechUiTransportEvent event;
 std::weak_ptr<const ui::BnwTechAwardInputDispatch> input_dispatch;
 mutable BnwMajorTechRequestPhase phase=BnwMajorTechRequestPhase::reserved;
 explicit Impl(BnwStockLuaAttemptLease actual):lease(std::move(actual)){}
};
struct BnwMajorTechTransportConsumption::Impl {
 std::shared_ptr<BnwMajorTechTransportRequest::Impl> request;
 BnwMajorTechConsumptionPhase phase=BnwMajorTechConsumptionPhase::pending_frame;
};
BnwMajorTechClientEndpoint::BnwMajorTechClientEndpoint(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwMajorTechClientEndpoint::~BnwMajorTechClientEndpoint(){impl_->retired.store(true);}
BnwMajorTechTransportRequest::BnwMajorTechTransportRequest(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwMajorTechTransportRequest::~BnwMajorTechTransportRequest()=default;
BnwMajorTechTransportConsumption::BnwMajorTechTransportConsumption(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwMajorTechTransportConsumption::~BnwMajorTechTransportConsumption()=default;
void BnwMajorTechClientEndpoint::require_current()const{
 const auto& p=*impl_;
 if(p.thread!=std::this_thread::get_id()){p.retired.store(true);throw BnwMajorTechTransportUnavailable("Technology client foreign thread");}
 try{
  need(!p.retired.load(),"Technology client retired");
  const auto issuer=p.ui.lock();const auto host=p.host.lock();
  const auto world=p.world.lock();const auto selection=p.selection.lock();
  need(issuer&&host&&world&&selection,"Technology client physical owner expired");
  issuer->require_major_tech_receiver_current(*this);
  need(same(host->session().read_state_lease(),world)&&host->session().state_generation()==p.generation,
   "Technology client SAME world/generation changed");
  const auto actual=selection->retained_host_for_tech_transport();
  need(same(actual,host),"Technology client canonical Host substituted");
  issuer->require_major_tech_receiver_current(*this);
  need(same(host->session().read_state_lease(),world)&&host->session().state_generation()==p.generation,
   "Technology client world changed after provider checks");
 }catch(...){p.retired.store(true);throw;}
}
void BnwMajorTechClientEndpoint::require_same_host(const ui::SelectedBnwPreviewHost& host)const{
 require_current();need(impl_->host.lock().get()==&host,"Technology client belongs to another Host");require_current();
}
void BnwMajorTechTransportRequest::require_current()const{
 const auto& p=*impl_;
 if(p.thread!=std::this_thread::get_id()){
  if(const auto endpoint=p.endpoint.lock())endpoint->impl_->retired.store(true);
  throw BnwMajorTechTransportUnavailable("Technology request foreign thread");
 }
 const auto endpoint=p.endpoint.lock();
 try{
  need(bool(endpoint),"Technology request client expired");endpoint->require_current();
  need(p.phase!=BnwMajorTechRequestPhase::uncertain,"Technology request uncertain");
  p.lease.require_operation(p.serial);
  const auto core=p.core.lock();const auto host=endpoint->impl_->host.lock();
  need(core&&host,"Technology request canonical source expired");
  need(same(p.lease.world_lease(),host->session().read_state_lease()),"Technology request operation changed world");
  core->require_stored_current(*host);
  need(core->after_revision()==p.revision,"Technology request earned revision changed");
  const auto requests=core->technology_transport_events();
  need(p.index<requests.size()&&requests[p.index]==p.event,"Technology request no longer matches actual queue");
  need(p.index<endpoint->impl_->requests.size()&&
   endpoint->impl_->requests[p.index]->impl_.get()==&p,"Technology request unregistered/substituted");
  p.lease.require_operation(p.serial);endpoint->require_current();
 }catch(...){if(endpoint)endpoint->impl_->retired.store(true);throw;}
}
std::shared_ptr<const ui::SelectedBnwPreviewHost> BnwMajorTechTransportRequest::retain_selected_host_for_context()const{
 need(impl_->thread==std::this_thread::get_id(),"TechAward source issuer foreign thread");
 const auto endpoint=impl_->endpoint.lock();need(bool(endpoint),"TechAward source client expired");
 const auto host=endpoint->impl_->host.lock();need(bool(host),"TechAward source canonical Host expired");
 require_current();
 const auto actual=endpoint->impl_->host.lock();
 need(same(actual,host),"TechAward source canonical Host changed during callbacks");
 require_current();return host;
}
BnwMajorTechUiTransportEvent BnwMajorTechTransportRequest::event()const{
 require_current();auto event=impl_->event;require_current();return event;
}
BnwMajorTechRequestPhase BnwMajorTechTransportRequest::phase()const{require_current();return impl_->phase;}
std::uint64_t BnwMajorTechTransportRequest::operation_serial()const{require_current();return impl_->serial;}
std::shared_ptr<const BnwMajorTechTransportRequest> BnwMajorTechClientEndpoint::prepare_next(
 const std::shared_ptr<const BnwMajorTeamHasTechPending>& core,const BnwStockLuaAttemptLease& lease)const{
 const auto& p=*impl_;Impl::Guard guard(p);
 try{
  require_current();lease.require_active();const auto host=p.host.lock();
  need(core&&host,"Technology transport actual core/Host unavailable");
  core->require_stored_current(*host);
  need(same(lease.world_lease(),host->session().read_state_lease()),"Technology transport source operation belongs to another world");
  if(const auto previous=p.core.lock())need(same(previous,core),"Technology transport attempted another acquisition");
  need(p.completed==p.requests.size(),"Technology transport previous occurrence not completed");
  const auto events=core->technology_transport_events();
  need(p.completed<events.size(),"Technology transport no reached next event");
  need(events[p.completed].occurrence==p.completed+1&&events[p.completed].generation==p.generation,
   "Technology transport native occurrence/generation differs");
  auto actual=std::make_shared<BnwMajorTechTransportRequest::Impl>(lease);
  actual->endpoint=p.self;actual->core=core;actual->serial=lease.operation_serial();
  actual->revision=core->after_revision();actual->index=p.completed;actual->event=events[p.completed];
  auto request=std::shared_ptr<const BnwMajorTechTransportRequest>(new BnwMajorTechTransportRequest(actual));
  // Reserve ledger storage before callbacks; a failed dispatch never reissues.
  impl_->requests.push_back(request);impl_->core=core;
  request->require_current();return request;
 }catch(...){p.retired.store(true);throw;}
}
std::shared_ptr<const BnwMajorTechTransportConsumption> BnwMajorTechClientEndpoint::consume_dirty(
 const std::shared_ptr<const BnwMajorTechTransportRequest>& request)const{
 const auto& p=*impl_;Impl::Guard guard(p);
 try{
  require_current();need(bool(request),"Technology dirty request absent");request->require_current();
  need(request->impl_->endpoint.lock().get()==this&&request->impl_->index==p.completed&&
   request->impl_->phase==BnwMajorTechRequestPhase::reserved&&
   request->impl_->event.kind==BnwMajorTechUiTransportKind::dirty,"Technology dirty request foreign/replayed/kind differs");
  auto receipt=std::make_unique<BnwMajorTechTransportConsumption::Impl>();receipt->request=request->impl_;
  auto result=std::shared_ptr<const BnwMajorTechTransportConsumption>(new BnwMajorTechTransportConsumption(std::move(receipt)));
  const auto ui=p.ui.lock();need(bool(ui),"Technology dirty actual UI expired");
  request->impl_->phase=BnwMajorTechRequestPhase::attempted;
  ui->consume_major_tech_dirty(request->impl_->event,*this);
  request->require_current();++impl_->completed;
  request->impl_->phase=BnwMajorTechRequestPhase::applied;
  result->require_same_request(*request->impl_->core.lock());return result;
 }catch(...){if(request&&request->impl_->thread==std::this_thread::get_id())request->impl_->phase=BnwMajorTechRequestPhase::uncertain;p.retired.store(true);throw;}
}
void BnwMajorTechTransportRequest::begin_actual_popup_dispatch(const BnwStockLuaAttemptLease& lease)const{
 require_current();lease.require_operation(impl_->serial);
 need(same(lease.world_lease(),impl_->lease.world_lease()),"Technology popup switched source world");
 need(impl_->phase==BnwMajorTechRequestPhase::reserved&&impl_->event.kind==BnwMajorTechUiTransportKind::popup_add,
  "Technology popup request replayed/kind differs");
 impl_->phase=BnwMajorTechRequestPhase::attempted;require_current();
}
std::shared_ptr<const BnwMajorTechTransportConsumption>
BnwMajorTechTransportRequest::finish_actual_popup_dispatch(const BnwStockLuaAttemptLease& lease)const{
 try{
  require_current();lease.require_operation(impl_->serial);
  need(same(lease.world_lease(),impl_->lease.world_lease())&&impl_->phase==BnwMajorTechRequestPhase::attempted,
   "Technology popup completion belongs to another/finished attempt");
  const auto endpoint=impl_->endpoint.lock();
  need(endpoint&&endpoint->impl_->completed==impl_->index,"Technology popup completed out of source order");
  auto receipt=std::make_unique<BnwMajorTechTransportConsumption::Impl>();receipt->request=impl_;
  auto result=std::shared_ptr<const BnwMajorTechTransportConsumption>(new BnwMajorTechTransportConsumption(std::move(receipt)));
  require_current();++endpoint->impl_->completed;impl_->phase=BnwMajorTechRequestPhase::applied;
  return result;
 }catch(...){fail_actual_dispatch();throw;}
}
void BnwMajorTechTransportRequest::fail_actual_dispatch()const noexcept{
 if(impl_->thread==std::this_thread::get_id())impl_->phase=BnwMajorTechRequestPhase::uncertain;
 if(const auto endpoint=impl_->endpoint.lock())endpoint->impl_->retired.store(true);
}
BnwMajorTechConsumptionPhase BnwMajorTechTransportConsumption::phase()const{
 // Durable diagnostic after the active Lua operation closes. This does not
 // authorize another dispatch or renew the old attempt lease.
 const auto& p=*impl_->request;
 const auto endpoint=p.endpoint.lock();need(bool(endpoint),"Technology consumed client expired");
 endpoint->require_current();
 need(p.thread==std::this_thread::get_id()&&p.phase==BnwMajorTechRequestPhase::applied,
  "Technology consumed receiver foreign/uncertain/not applied");
 const auto core=p.core.lock();const auto host=endpoint->impl_->host.lock();
 need(core&&host,"Technology consumed source expired");core->require_stored_current(*host);
 const auto events=core->technology_transport_events();
 need(core->after_revision()==p.revision&&p.index<events.size()&&events[p.index]==p.event,
  "Technology consumed request/revision changed");
 endpoint->require_current();return impl_->phase;
}
std::uint64_t BnwMajorTechTransportConsumption::occurrence()const{(void)phase();return impl_->request->event.occurrence;}
BnwMajorTechUiTransportKind BnwMajorTechTransportConsumption::kind()const{(void)phase();return impl_->request->event.kind;}
void BnwMajorTechTransportConsumption::require_same_request(const BnwMajorTeamHasTechPending& core)const{
 (void)phase();need(impl_->request->core.lock().get()==&core,"Technology consumer belongs to another acquisition");
 need(impl_->request->phase==BnwMajorTechRequestPhase::applied,"Technology receiver not actually applied");
}
std::shared_ptr<const BnwMajorTechTransportRequest> BnwMajorTechTransportDispatcher::prepare_next(
 const std::shared_ptr<const BnwMajorTeamHasTechPending>& core,
 const std::shared_ptr<const BnwMajorTechClientEndpoint>& endpoint,const BnwStockLuaAttemptLease& lease){
 need(bool(endpoint),"Technology actual selected client endpoint absent");
 if(endpoint->impl_->thread!=std::this_thread::get_id()){
  endpoint->impl_->retired.store(true);throw BnwMajorTechTransportUnavailable("Technology client foreign issuer thread");
 }
 // Canonical strong pin BEFORE any callbackful currentness checks. A caller
 // no-op control alias is refused without querying its borrowed facade.
 const auto canonical=endpoint->impl_->self.lock();
 need(same(canonical,endpoint),"Technology client no-op/foreign control alias");
 return canonical->prepare_next(core,lease);
}
std::shared_ptr<const BnwMajorTechTransportConsumption> BnwMajorTechTransportDispatcher::consume_dirty(
 const std::shared_ptr<const BnwMajorTechTransportRequest>& request){
 need(bool(request),"Technology dirty request absent");request->require_current();
 const auto endpoint=request->impl_->endpoint.lock();need(bool(endpoint),"Technology dirty client expired");
 return endpoint->consume_dirty(request);
}
}

namespace civ5::ui {
namespace {
void ui_need(bool ok,const char* message){if(!ok)throw runtime::BnwMajorTechTransportUnavailable(message);}
template<class A,class B>bool ui_same(const A& a,const B& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);}
}
std::shared_ptr<const runtime::BnwMajorTechClientEndpoint> LiveUiHost::create_major_tech_endpoint(const LiveUiFrame& frame){
 ui_need(plot_yield_ui_thread_==std::this_thread::get_id(),"Technology UI factory foreign thread");
 const auto ui=selected_owner_lifetime_.lock();const auto selection=city_selection_control_;
 ui_need(ui&&ui.get()==this&&selection&&major_tech_endpoint_.expired(),"Technology UI actual selected owner absent/already issued");
 ui_need(current(frame),"Technology UI factory needs actual current frame");
 const auto host=selection->retained_host_for_tech_transport();
 auto p=std::make_unique<runtime::BnwMajorTechClientEndpoint::Impl>();
 p->ui=ui;p->selection=selection;p->host=host;p->world=host->session().read_state_lease();
 p->generation=host->session().state_generation();
 const std::array<const char*,2> names{"UnitPanel","TechTree"};
 for(std::size_t n=0;n<names.size();++n){p->contexts[n]=screens_.registry().find(names[n]);ui_need(p->contexts[n],"Technology dirty actual context not registered");}
 ui_need(current(frame)&&ui_same(host,selection->retained_host_for_tech_transport()),"Technology UI factory changed during provider callbacks");
 auto result=std::shared_ptr<const runtime::BnwMajorTechClientEndpoint>(new runtime::BnwMajorTechClientEndpoint(std::move(p)));
 result->impl_->self=result;major_tech_endpoint_=result;
 result->require_current();return result;
}
void LiveUiHost::require_major_tech_receiver_current(const runtime::BnwMajorTechClientEndpoint& endpoint)const{
 ui_need(plot_yield_ui_thread_==std::this_thread::get_id(),"Technology UI receiver foreign thread");
 const auto ui=selected_owner_lifetime_.lock();const auto actual=major_tech_endpoint_.lock();
 ui_need(ui&&ui.get()==this&&actual.get()==&endpoint&&city_selection_control_,"Technology UI receiver registration expired/substituted");
 ui_need(ui_same(ui,endpoint.impl_->ui.lock())&&ui_same(city_selection_control_,endpoint.impl_->selection.lock()),
  "Technology UI receiver actual control changed");
 const std::array<const char*,2> names{"UnitPanel","TechTree"};
 for(std::size_t n=0;n<names.size();++n)ui_need(screens_.registry().find(names[n])==endpoint.impl_->contexts[n],"Technology UI handler registry changed");
 city_selection_control_->require_current();
 ui_need(ui_same(actual,endpoint.impl_->self.lock()),"Technology UI endpoint canonical control changed");
}
void LiveUiHost::consume_major_tech_dirty(const runtime::BnwMajorTechUiTransportEvent& event,
 const runtime::BnwMajorTechClientEndpoint& endpoint){
 require_major_tech_receiver_current(endpoint);
 const auto host=city_selection_control_->retained_host_for_tech_transport();
 ui_need(event.kind==runtime::BnwMajorTechUiTransportKind::dirty&&event.dirty&&event.occurrence&&
  event.generation==host->session().state_generation()&&event.turn==host->session().state().setup.game_turn&&
  event.team==city_selection_control_->active_team(),
  "Technology dirty source/active recipient differs");
 ui_need(sequence_<std::numeric_limits<std::uint64_t>::max()&&major_tech_dirty_.revision<std::numeric_limits<std::uint64_t>::max(),
  "Technology UI dirty sequence exhausted");
 auto next=major_tech_dirty_; // Prepare ALL allocation before actual flag store.
 if(event.native_name=="MiscButtons_DIRTY_BIT")next.misc_buttons=true;
 else if(event.native_name=="SelectionButtons_DIRTY_BIT")next.selection_buttons=true;
 else if(event.native_name=="ResearchButtons_DIRTY_BIT")next.research_buttons=true;
 else throw runtime::BnwMajorTechTransportUnavailable("Technology dirty unknown native bit");
 next.deliveries.push_back({event.occurrence,event.generation,host->session().state().ui_source_revision,
  event.team,event.technology,event.player,event.turn,event.native_name});++next.revision;
 require_major_tech_receiver_current(endpoint);
 ++sequence_; // Invalidate real old UiFrame before nonthrowing owned stores.
 major_tech_dirty_=std::move(next);
 require_major_tech_receiver_current(endpoint);
}
void LiveUiHost::publish_major_tech_dirty(LiveUiFrame& frame){
 if(const auto endpoint=major_tech_endpoint_.lock()){
  require_major_tech_receiver_current(*endpoint);
  const auto host=city_selection_control_->retained_host_for_tech_transport();
  const auto* stamp=frame.snapshot.find<BnwClientFrameStamp>();
  ui_need(frame.owner==this&&frame.sequence==sequence_&&stamp&&
   stamp->identity.player==city_selection_control_->active_player()&&
   stamp->identity.team==city_selection_control_->active_team()&&
   stamp->identity.ui_source_revision==host->session().state().ui_source_revision,
   "Technology dirty publication needs actual rebuilt SAME-world frame");
  frame.snapshot=frame.snapshot.with<BnwMajorTechDirtySnapshot>(std::make_shared<const BnwMajorTechDirtySnapshot>(major_tech_dirty_));
  publish_tech_award_lifecycle(frame);
  require_major_tech_receiver_current(*endpoint);
 }
}
}

namespace civ5::runtime {
ui::UiFrameSnapshot BnwMajorTechTransportRequest::retained_ui_snapshot_for_context()const {
 require_current();const auto endpoint=impl_->endpoint.lock();
 need(bool(endpoint),"TechAward actual endpoint expired");
 const auto ui=endpoint->impl_->ui.lock();need(bool(ui),"TechAward actual UI expired");
 auto value=ui->snapshot_for_major_tech_context(*endpoint);require_current();return value;
}
}
namespace civ5::ui {
UiFrameSnapshot LiveUiHost::snapshot_for_major_tech_context(const runtime::BnwMajorTechClientEndpoint& endpoint)const {
 require_major_tech_receiver_current(endpoint);
 const auto keep=selected_owner_lifetime_.lock();
 ui_need(keep&&keep.get()==this,"TechAward actual UI provider owner expired");
 std::optional<UiFrameSnapshot> value;
 {
  const auto provider=provider_; // strong target before provider invocation
  require_major_tech_receiver_current(endpoint);value.emplace(provider());
  require_major_tech_receiver_current(endpoint);
 } // target destructor is also inside the bookends
 require_major_tech_receiver_current(endpoint);return std::move(*value);
}
}

namespace civ5::runtime {
void BnwMajorTechTransportRequest::register_context_input_slot(std::string_view source_id)const {
 require_current();
 need(impl_->phase==BnwMajorTechRequestPhase::attempted&&impl_->event.kind==BnwMajorTechUiTransportKind::popup_add,
      "TechAward native input registration outside actual popup source attempt");
 const auto endpoint=impl_->endpoint.lock();need(bool(endpoint),"TechAward actual endpoint expired");
 const auto ui=endpoint->impl_->ui.lock();need(bool(ui),"TechAward actual UI expired");
 ui->register_tech_award_input_slot(*this,*endpoint,source_id);require_current();
}
}
namespace civ5::ui {
void LiveUiHost::register_tech_award_input_slot(const runtime::BnwMajorTechTransportRequest& request,
 const runtime::BnwMajorTechClientEndpoint& endpoint,std::string_view source_id) {
 require_major_tech_receiver_current(endpoint);request.require_current();
 const std::string id(source_id);const auto event=request.event();
 ui_need(screens_.registry().find("TechAwardPopup")&&
  (id=="ContinueButton"||id=="CloseButton"||id=="ContextPtr.InputHandler"||id=="ContextPtr.ShowHideHandler"),
  "TechAward input source/context not actually registered");
 for(const auto& old:tech_award_inputs_)ui_need(old.occurrence!=event.occurrence||old.native_control!=id,
  "TechAward native source slot registered twice");
 const auto host=city_selection_control_->retained_host_for_tech_transport();
 auto next=tech_award_inputs_;
 next.push_back({event.occurrence,event.generation,host->session().state().ui_source_revision,id});
 require_major_tech_receiver_current(endpoint);request.require_current();
 tech_award_inputs_.swap(next); // actual native routing registration storage
 require_major_tech_receiver_current(endpoint);request.require_current();
 // No measured click rectangle, VM invocation or painted frame is fabricated.
}
}
namespace civ5::runtime {
void BnwMajorTechTransportRequest::apply_context_effect(BnwTechAwardContextEffect effect)const {
 require_context_mutation_current();
 if(const auto input=impl_->input_dispatch.lock()) {
  input->require_active_dispatch();
  need(effect==BnwTechAwardContextEffect::dequeue||effect==BnwTechAwardContextEffect::timer_decrement||effect==BnwTechAwardContextEffect::processed,
       "Queued TechAward input attempted unrelated initialization effect");
 }else need(impl_->phase==BnwMajorTechRequestPhase::attempted,"TechAward context effect outside actual source attempt");
 const auto endpoint=impl_->endpoint.lock();need(bool(endpoint),"TechAward actual endpoint expired");
 const auto ui=endpoint->impl_->ui.lock();need(bool(ui),"TechAward actual UI expired");
 ui->apply_tech_award_context_effect(*this,*endpoint,effect);require_context_mutation_current();
}
}
namespace civ5::ui {
struct LiveUiHost::TechAwardLifecycleStorage {
 enum class Stage {closed,awaiting_increment,awaiting_shown,open,awaiting_decrement,awaiting_processed};
 Stage stage=Stage::closed;
 std::int32_t semaphore=0; // new Civis UI timer, not fabricated saved GameCore state
 std::uint64_t occurrence=0,generation=0,revision=0;
 std::vector<runtime::BnwMajorTechUiTransportEvent> shown_events;
 std::vector<std::pair<std::int32_t,std::int32_t>> processed_events;
};
void LiveUiHost::apply_tech_award_context_effect(const runtime::BnwMajorTechTransportRequest& request,
 const runtime::BnwMajorTechClientEndpoint& endpoint,runtime::BnwTechAwardContextEffect effect) {
 require_major_tech_receiver_current(endpoint);request.require_context_mutation_current();
 const auto event=request.context_effect_event();
 ui_need(event.kind==runtime::BnwMajorTechUiTransportKind::popup_add&&event.native_name=="BUTTONPOPUP_TECH_AWARD",
         "TechAward context effect for another native popup");
 auto prior=tech_award_lifecycle_;
 auto next=prior?std::make_shared<TechAwardLifecycleStorage>(*prior):std::make_shared<TechAwardLifecycleStorage>();
 using Effect=runtime::BnwTechAwardContextEffect;using Stage=TechAwardLifecycleStorage::Stage;
 ui_need(sequence_<UINT64_MAX&&next->revision<UINT64_MAX,"TechAward native UI revision exhausted");
 if(effect==Effect::queue) {
  ui_need(next->stage==Stage::closed&&!screens_.focused_screen(),"TechAward competing modal/priority order requires native queue continuation");
  std::size_t slots=0;for(const auto& slot:tech_award_inputs_)if(slot.occurrence==event.occurrence)++slots;
  ui_need(slots==4&&screens_.registry().find("TechAwardPopup"),"TechAward actual native control registration incomplete");
  next->occurrence=event.occurrence;next->generation=event.generation;next->stage=Stage::awaiting_increment;
 } else {
  ui_need(next->occurrence==event.occurrence,"TechAward native lifecycle occurrence changed");
  switch(effect) {
  case Effect::timer_increment:
   ui_need(next->stage==Stage::awaiting_increment&&next->semaphore<INT32_MAX,"TechAward timer increment order/overflow");
   ++next->semaphore;next->stage=Stage::awaiting_shown;break;
  case Effect::shown:
   ui_need(next->stage==Stage::awaiting_shown,"TechAward PopupShown before real timer increment");
   next->shown_events.push_back(event);next->stage=Stage::open;break;
  case Effect::dequeue:
   ui_need(next->stage==Stage::open&&screens_.focused_screen()==std::optional<std::string>{"TechAwardPopup"},"TechAward close does not own actual modal focus");
   next->stage=Stage::awaiting_decrement;break;
  case Effect::timer_decrement:
   ui_need(next->stage==Stage::awaiting_decrement&&next->semaphore>0,"TechAward timer decrement order/underflow");
   --next->semaphore;next->stage=Stage::awaiting_processed;break;
  case Effect::processed:
   ui_need(next->stage==Stage::awaiting_processed,"TechAward PopupProcessed before real timer decrement");
   next->processed_events.emplace_back(68,0);next->stage=Stage::closed;break;
  default:throw runtime::BnwMajorTechTransportUnavailable("TechAward unknown context effect");
  }
 }
 ++next->revision; // all queue allocation prepared before actual controller stores
 require_major_tech_receiver_current(endpoint);request.require_context_mutation_current();
 if(effect==Effect::queue||effect==Effect::dequeue) {
  const auto snapshot=snapshot_for_major_tech_context(endpoint);
  const UiCommand command=effect==Effect::queue?UiCommand{UiOpenScreen{"TechAwardPopup"}}:UiCommand{UiCloseScreen{"TechAwardPopup"}};
  ++sequence_; // retire old frame before actual native modal stack mutation
  const auto applied=screens_.dispatch(command,snapshot,nullptr);
  ui_need(applied.status==UiCommandStatus::navigation,"TechAward actual modal controller refused");
 } else ++sequence_;
 tech_award_lifecycle_=std::move(next);
 if(const auto input=request.impl_->input_dispatch.lock())input->note_actual_effect_sequence(sequence_);
 require_major_tech_receiver_current(endpoint);request.require_context_mutation_current();
 // This completes owned native state/ordered events. It does NOT claim a
 // source Label layout, VM handler execution or GPU-presented modal frame.
}
void LiveUiHost::publish_tech_award_lifecycle(LiveUiFrame& frame) {
 if(!tech_award_lifecycle_)return;
 const auto endpoint=major_tech_endpoint_.lock();ui_need(bool(endpoint),"TechAward modal actual client expired");
 require_major_tech_receiver_current(*endpoint);
 ui_need(frame.owner==this&&frame.sequence==sequence_,"TechAward modal publication requires actual current issued UI frame");
 auto value=std::make_shared<BnwTechAwardModalSnapshot>();
 using Stage=TechAwardLifecycleStorage::Stage;
 value->occurrence=tech_award_lifecycle_->occurrence;value->revision=tech_award_lifecycle_->revision;
 value->timer_semaphore=tech_award_lifecycle_->semaphore;
 value->modal_queued=tech_award_lifecycle_->stage==Stage::awaiting_increment||
  tech_award_lifecycle_->stage==Stage::awaiting_shown||tech_award_lifecycle_->stage==Stage::open;
 value->shown_received=tech_award_lifecycle_->stage==Stage::open;
 value->source_showhide_pending=tech_award_lifecycle_->stage==Stage::awaiting_increment||
  tech_award_lifecycle_->stage==Stage::awaiting_shown||tech_award_lifecycle_->stage==Stage::awaiting_decrement||
  tech_award_lifecycle_->stage==Stage::awaiting_processed;
 value->gpu_presented=false; // only genuine renderer may prove presentation
 for(const auto& event:tech_award_lifecycle_->shown_events)
  value->shown.push_back({68,event.data1,event.data2,event.data3,event.flags,event.option1,event.option2,
    event.occurrence,event.generation,event.team,event.player,event.turn});
 value->processed=tech_award_lifecycle_->processed_events;
 frame.snapshot=frame.snapshot.with<BnwTechAwardModalSnapshot>(value);
 publish_actual_tech_award_context(frame);
 require_major_tech_receiver_current(*endpoint);
}
}

namespace civ5::runtime {
void BnwMajorTechTransportRequest::require_context_stored_current()const {
 const auto state=impl_; // retain actual stored receiver state before bookends
 if(state->thread!=std::this_thread::get_id()) {
  if(const auto endpoint=state->endpoint.lock())endpoint->impl_->retired.store(true);
  throw BnwMajorTechTransportUnavailable("TechAward stored context foreign thread");
 }
 const auto endpoint=state->endpoint.lock();const auto core=state->core.lock();
 const auto host=endpoint?endpoint->impl_->host.lock():nullptr;
 try {
  need(endpoint&&core&&host&&state->phase==BnwMajorTechRequestPhase::applied&&
       state->event.kind==BnwMajorTechUiTransportKind::popup_add&&state->event.native_name=="BUTTONPOPUP_TECH_AWARD",
       "TechAward actual stock receiver not applied or source expired");
  endpoint->require_current();core->require_stored_current(*host);
  need(same(host->session().read_state_lease(),endpoint->impl_->world.lock())&&
       host->session().state_generation()==endpoint->impl_->generation,
       "TechAward stored SAME world/generation changed");
  const auto events=core->technology_transport_events();
  need(core->after_revision()==state->revision&&state->index<events.size()&&events[state->index]==state->event&&
       state->index<endpoint->impl_->completed&&state->index<endpoint->impl_->requests.size()&&
       endpoint->impl_->requests[state->index]->impl_==state,
       "TechAward stored earned revision/request/source order changed");
  core->require_stored_current(*host);endpoint->require_current();
 }catch(...){if(endpoint)endpoint->impl_->retired.store(true);throw;}
 // No require_operation(oldSerial): it was consumed by genuine receiver.
 // This guard is private and only reads that actual applied record. It cannot
 // dispatch a hook, mutate a control or produce a fresh gameplay operation.
}
std::shared_ptr<const ui::SelectedBnwPreviewHost> BnwMajorTechTransportRequest::retain_stored_host_for_context()const {
 require_context_stored_current();const auto endpoint=impl_->endpoint.lock();
 need(bool(endpoint),"TechAward stored endpoint expired");const auto host=endpoint->impl_->host.lock();
 need(bool(host),"TechAward stored Host expired");require_context_stored_current();return host;
}
void BnwMajorTechTransportRequest::register_actual_context_owner(
 const std::shared_ptr<const ui::BnwTechAwardContextOwner>& supplied)const {
 require_current();need(impl_->phase==BnwMajorTechRequestPhase::attempted,"TechAward context outside actual once operation");
 need(bool(supplied),"TechAward actual context absent");
 const auto canonical=supplied->retain_canonical(supplied);require_current();
 need(canonical.get()!=nullptr,"TechAward actual canonical context absent");
 canonical->require_same_actual_request(*this);require_current();
 const auto endpoint=impl_->endpoint.lock();need(bool(endpoint),"TechAward context actual endpoint expired");
 const auto ui=endpoint->impl_->ui.lock();need(bool(ui),"TechAward actual UI owner expired");
 ui->bind_actual_tech_award_context(*this,*endpoint,canonical);require_current();
}
}
namespace civ5::ui {
void LiveUiHost::bind_actual_tech_award_context(const runtime::BnwMajorTechTransportRequest& request,
 const runtime::BnwMajorTechClientEndpoint& endpoint,const std::shared_ptr<const BnwTechAwardContextOwner>& actual) {
 require_major_tech_receiver_current(endpoint);request.require_current();
 ui_need(tech_award_context_.expired(),"TechAward actual retained context already bound; new occurrence requires real context transition");
 const auto event=request.event();ui_need(event.kind==runtime::BnwMajorTechUiTransportKind::popup_add&&
    event.native_name=="BUTTONPOPUP_TECH_AWARD","TechAward context bound by another engine event");
 const auto canonical=actual->retain_canonical(actual);require_major_tech_receiver_current(endpoint);request.require_current();
 tech_award_context_=canonical;tech_award_context_occurrence_=event.occurrence;tech_award_context_generation_=event.generation;
 require_major_tech_receiver_current(endpoint);request.require_current();
}
void LiveUiHost::publish_actual_tech_award_context(LiveUiFrame& frame) {
 using Stage=TechAwardLifecycleStorage::Stage;
 if(!tech_award_lifecycle_||tech_award_lifecycle_->stage!=Stage::open)return;
 const auto supplied=tech_award_context_.lock();ui_need(bool(supplied),"TechAward queued actual context expired");
 const auto canonical=supplied->retain_queued_canonical(supplied);
 const auto endpoint=major_tech_endpoint_.lock();ui_need(bool(endpoint),"TechAward queued actual endpoint expired");
 require_major_tech_receiver_current(*endpoint);
 ui_need(frame.owner==this&&frame.sequence==sequence_,"TechAward context publication requires actual newly issued UI frame");
 const auto stamp=frame.snapshot.find<BnwClientFrameStamp>();
 ui_need(stamp&&stamp->identity.ui_source_revision==city_selection_control_->retained_host_for_tech_transport()->session().state().ui_source_revision&&stamp->viewport.drawable(),
         "TechAward layout publication lacks SAME actual FrameOwner viewport/source revision");
 auto value=std::make_shared<BnwTechAwardContextSnapshot>();value->publication=*stamp;
 value->occurrence=tech_award_context_occurrence_;value->generation=tech_award_context_generation_;
 value->context=canonical->queued_observation(value->publication.viewport);
 ui_need(value->occurrence==tech_award_lifecycle_->occurrence&&value->generation==tech_award_lifecycle_->generation&&value->context.queued&&value->context.visible,
         "TechAward queued context occurrence/visibility changed");
 canonical->require_queued_current();require_major_tech_receiver_current(*endpoint);
 ui_need(frame.owner==this&&frame.sequence==sequence_&&
         value->publication.identity.ui_source_revision==city_selection_control_->retained_host_for_tech_transport()->session().state().ui_source_revision,
         "TechAward actual frame changed during layout/source publication");
  const auto actual_ui=selected_owner_lifetime_.lock();
  ui_need(actual_ui&&actual_ui.get()==this,"TechAward animation publisher lacks canonical UI owner");
  value->animation=std::shared_ptr<const BnwTechAwardAnimationWitness>(
   new BnwTechAwardAnimationWitness(canonical,actual_ui,value->publication,
    value->occurrence,value->generation,value->context.client_layout));
  canonical->require_queued_current();require_major_tech_receiver_current(*endpoint);
  ui_need(frame.owner==this&&frame.sequence==sequence_,"TechAward animation publisher frame changed");
  value->animation->require_current_without_callbacks();
  frame.snapshot=frame.snapshot.with<BnwTechAwardContextSnapshot>(value);
 canonical->require_queued_current();require_major_tech_receiver_current(*endpoint);
 value->animation->require_current_without_callbacks();
}
}

namespace civ5::runtime {
void BnwMajorTechTransportRequest::require_context_mutation_current()const {
 if(impl_->thread!=std::this_thread::get_id())throw BnwMajorTechTransportUnavailable("TechAward input mutation foreign thread");
 if(const auto input=impl_->input_dispatch.lock()){input->require_active_dispatch();return;}
 require_current();
}
BnwMajorTechUiTransportEvent BnwMajorTechTransportRequest::context_effect_event()const {
 require_context_mutation_current();auto value=impl_->event;require_context_mutation_current();return value;
}
std::shared_ptr<const ui::SelectedBnwPreviewHost> BnwMajorTechTransportRequest::retain_mutation_host_for_context()const {
 require_context_mutation_current();const auto endpoint=impl_->endpoint.lock();
 need(bool(endpoint),"TechAward input endpoint expired");const auto host=endpoint->impl_->host.lock();
 need(bool(host),"TechAward input Host expired");require_context_mutation_current();return host;
}
void BnwMajorTechTransportRequest::attach_context_input(const std::shared_ptr<const ui::BnwTechAwardInputDispatch>& input,const BnwStockLuaAttemptLease& lease)const {
 require_context_stored_current();need(input&&impl_->input_dispatch.expired(),"TechAward input scope already bound");
 lease.require_same_source_as_stored(impl_->lease);input->require_active_dispatch();
 impl_->input_dispatch=input;require_context_mutation_current();
}
void BnwMajorTechTransportRequest::detach_context_input(const ui::BnwTechAwardInputDispatch& input)const {
 require_context_mutation_current();need(impl_->input_dispatch.lock().get()==&input,"TechAward input detach foreign token");
 impl_->input_dispatch.reset();require_context_stored_current();
}
}
namespace civ5::ui {
struct BnwTechAwardInputDispatch::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 mutable std::atomic<bool> retired{false};
 std::weak_ptr<const BnwTechAwardInputDispatch> self;
 std::weak_ptr<LiveUiHost> ui;
 std::weak_ptr<const runtime::BnwMajorTechClientEndpoint> endpoint;
 std::weak_ptr<const BnwTechAwardContextOwner> context;
 std::weak_ptr<const runtime::BnwMajorTechTransportRequest> request;
 std::weak_ptr<const UiFrameSnapshot> issued_snapshot;
 std::optional<runtime::BnwStockLuaAttemptLease> lease;
 BnwClientFrameStamp stamp;
 BnwTechAwardInput input=BnwTechAwardInput::continue_button;
 BnwTechAwardInputPhase phase=BnwTechAwardInputPhase::reserved;
 std::uint64_t occurrence=0,source_occurrence=0,generation=0,source_revision=0;
 std::uint64_t issued_sequence=0,expected_sequence=0,serial=0;
 std::size_t processed_before=0;
};
namespace {void input_need(bool value,const char* why){if(!value)throw runtime::BnwMajorTechTransportUnavailable(why);}}
BnwTechAwardInputDispatch::BnwTechAwardInputDispatch(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwTechAwardInputDispatch::~BnwTechAwardInputDispatch(){impl_->retired.store(true);if(impl_->thread==std::this_thread::get_id()&&impl_->phase==BnwTechAwardInputPhase::executing)fail_actual_dispatch();}
std::shared_ptr<const BnwTechAwardInputDispatch> BnwTechAwardInputDispatch::retain_canonical_identity(const std::shared_ptr<const BnwTechAwardInputDispatch>& supplied)const {
 if(impl_->thread!=std::this_thread::get_id()){impl_->retired.store(true);throw runtime::BnwMajorTechTransportUnavailable("TechAward input foreign token");}
 const auto canonical=impl_->self.lock();input_need(canonical&&canonical.get()==this&&supplied.get()==this&&
  !canonical.owner_before(supplied)&&!supplied.owner_before(canonical),"TechAward input forged control block");
 return canonical;
}
std::shared_ptr<const BnwTechAwardInputDispatch> BnwTechAwardInputDispatch::retain_canonical(const std::shared_ptr<const BnwTechAwardInputDispatch>& supplied)const {
 const auto canonical=retain_canonical_identity(supplied);canonical->require_current();return canonical;
}
void BnwTechAwardInputDispatch::require_current()const {
 const auto& p=*impl_;
 if(p.thread!=std::this_thread::get_id()){p.retired.store(true);throw runtime::BnwMajorTechTransportUnavailable("TechAward input foreign thread");}
 const auto canonical=p.self.lock();const auto ui=p.ui.lock();const auto request=p.request.lock();const auto endpoint=p.endpoint.lock();const auto context=p.context.lock();
 try {
  input_need(canonical&&canonical.get()==this&&!p.retired.load()&&p.phase!=BnwTechAwardInputPhase::uncertain&&ui&&request&&endpoint&&context,"TechAward input owner/source expired or uncertain");
  request->require_context_stored_current();ui->require_major_tech_receiver_current(*endpoint);
  input_need(ui->tech_award_context_.lock().get()==context.get()&&ui->tech_award_context_occurrence_==p.source_occurrence&&
             ui->tech_award_context_generation_==p.generation,"TechAward input actual context occurrence changed");
  if(p.phase==BnwTechAwardInputPhase::reserved){
   const auto issued=p.issued_snapshot.lock();input_need(issued&&ui->issued_plot_yield_snapshot_&&
    issued.get()==ui->issued_plot_yield_snapshot_.get()&&!issued.owner_before(ui->issued_plot_yield_snapshot_)&&!ui->issued_plot_yield_snapshot_.owner_before(issued)&&
    ui->sequence_==p.issued_sequence&&ui->issued_plot_yield_sequence_==p.issued_sequence,"TechAward input frame retired");
   input_need(ui_owners_current(*issued),"TechAward input published owners retired");
   context->require_queued_current();
   input_need(ui->sequence_==p.issued_sequence&&ui->issued_plot_yield_snapshot_.get()==issued.get(),"TechAward input frame changed after owner callbacks");
  } else if(p.phase==BnwTechAwardInputPhase::executing){
   input_need(p.lease.has_value(),"TechAward input new source lease absent");p.lease->require_operation(p.serial);
   input_need(ui->sequence_==p.expected_sequence,"TechAward input competing UI mutation");
  }
  const auto host=request->retain_stored_host_for_context();
  input_need(host->session().state_generation()==p.generation&&host->session().state().ui_source_revision==p.source_revision&&
             ui->city_selection_control_&&ui->city_selection_control_->revision()==p.stamp.identity.selection_revision,
             "TechAward input SAME world/selection revision changed");
  request->require_context_stored_current();ui->require_major_tech_receiver_current(*endpoint);
  input_need(!p.retired.load(),"TechAward input retired during checks");
  if(p.phase==BnwTechAwardInputPhase::executing){p.lease->require_operation(p.serial);input_need(ui->sequence_==p.expected_sequence,"TechAward input UI changed after callbacks");}
 }catch(...){p.retired.store(true);throw;}
}
void BnwTechAwardInputDispatch::require_active_dispatch()const {require_current();input_need(impl_->phase==BnwTechAwardInputPhase::executing,"TechAward input outside real new dispatch");}
BnwTechAwardInputPhase BnwTechAwardInputDispatch::phase()const {if(impl_->thread!=std::this_thread::get_id()){impl_->retired.store(true);throw runtime::BnwMajorTechTransportUnavailable("TechAward input phase foreign thread");}return impl_->retired.load()?BnwTechAwardInputPhase::uncertain:impl_->phase;}
BnwTechAwardInput BnwTechAwardInputDispatch::target()const {require_current();return impl_->input;}
std::uint64_t BnwTechAwardInputDispatch::occurrence()const {require_current();return impl_->occurrence;}
std::int32_t BnwTechAwardInputDispatch::native_key_down()const {require_active_dispatch();return 256;} // PE41DA93 exact KeyEvents.KeyDown PUSH100.
std::int32_t BnwTechAwardInputDispatch::native_escape_key()const {require_active_dispatch();return 27;} // PE41E371.
std::int32_t BnwTechAwardInputDispatch::native_return_key()const {require_active_dispatch();return 13;} // PE41E254.
std::int32_t BnwTechAwardInputDispatch::native_input_key()const {
 require_active_dispatch();if(impl_->input==BnwTechAwardInput::escape)return 27; // PE41E371 actual VK_ESCAPE.
 if(impl_->input==BnwTechAwardInput::return_key)return 13; // PE41E254 actual VK_RETURN.
 throw runtime::BnwMajorTechTransportUnavailable("TechAward button input has no native key operand");
}
void BnwTechAwardInputDispatch::require_same_actual_context(const BnwTechAwardContextOwner& context)const {require_current();input_need(impl_->context.lock().get()==&context,"TechAward input belongs to another retained VM context");}
std::shared_ptr<const BnwTechAwardContextOwner> BnwTechAwardInputDispatch::retain_context_for_dispatch()const {
 require_current();const auto context=impl_->context.lock();input_need(bool(context),"TechAward input context expired");
 const auto canonical=context->retain_queued_canonical(context);require_current();return canonical;
}
void BnwTechAwardInputDispatch::begin_actual_dispatch(const runtime::BnwStockLuaAttemptLease& lease)const {
 require_current();input_need(impl_->phase==BnwTechAwardInputPhase::reserved,"TechAward input replayed");lease.require_active();
 const auto actual=impl_->self.lock();const auto request=impl_->request.lock();const auto context=impl_->context.lock();
 input_need(actual&&request&&context,"TechAward input source expired before attempt");
 const auto serial=lease.operation_serial();impl_->lease.emplace(lease);impl_->serial=serial;
 impl_->phase=BnwTechAwardInputPhase::executing; // no retry after any reached callback
 try {request->attach_context_input(actual,lease);context->begin_queued_input(actual);require_active_dispatch();}
 catch(...){fail_actual_dispatch();throw;}
}
void BnwTechAwardInputDispatch::note_actual_effect_sequence(std::uint64_t sequence)const {
 const auto& p=*impl_;input_need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.phase==BnwTechAwardInputPhase::executing&&p.lease,"TechAward input effect not active");
 p.lease->require_operation(p.serial);const auto ui=p.ui.lock();input_need(ui&&sequence==ui->sequence_&&p.expected_sequence<UINT64_MAX&&sequence==p.expected_sequence+1,"TechAward input native effect sequence changed");
 impl_->expected_sequence=sequence;
}
void BnwTechAwardInputDispatch::finish_actual_dispatch(const runtime::BnwStockLuaAttemptLease& lease)const {
 try {
  require_active_dispatch();lease.require_operation(impl_->serial);const auto ui=impl_->ui.lock();const auto context=impl_->context.lock();const auto request=impl_->request.lock();
  input_need(ui&&context&&request&&ui->tech_award_lifecycle_,"TechAward input completion owners expired");
  using Stage=LiveUiHost::TechAwardLifecycleStorage::Stage;
  const auto state=ui->tech_award_lifecycle_;
  input_need(state->stage==Stage::closed&&state->semaphore==0&&state->processed_events.size()==impl_->processed_before+1&&
    state->processed_events.back()==std::pair<std::int32_t,std::int32_t>{68,0}&&!ui->screens_.focused_screen(),"TechAward input original modal/Processed/timer suffix incomplete");
  context->finish_queued_input(*this);request->detach_context_input(*this);
  impl_->phase=BnwTechAwardInputPhase::pending_frame;
 }catch(...){fail_actual_dispatch();throw;}
}
void BnwTechAwardInputDispatch::fail_actual_dispatch()const noexcept {
 impl_->retired.store(true);
 if(impl_->thread!=std::this_thread::get_id())return;
 impl_->phase=BnwTechAwardInputPhase::uncertain;
 if(const auto context=impl_->context.lock())context->fail_queued_input();
}
std::shared_ptr<const BnwTechAwardInputDispatch> LiveUiHost::prepare_tech_award_input(const LiveUiFrame& frame,BnwTechAwardInput input) {
 input_need(plot_yield_ui_thread_==std::this_thread::get_id(),"TechAward input issuer foreign thread");
 const auto self=selected_owner_lifetime_.lock();input_need(self&&self.get()==this,"TechAward input actual UiHost expired");
 require_issued_frame_finality(frame);input_need(current(frame),"TechAward input current issued frame required");
 switch(input){case BnwTechAwardInput::continue_button:case BnwTechAwardInput::close_button:case BnwTechAwardInput::escape:case BnwTechAwardInput::return_key:break;default:throw runtime::BnwMajorTechTransportUnavailable("TechAward input unknown target");}
 const auto endpoint=major_tech_endpoint_.lock();const auto supplied=tech_award_context_.lock();
 input_need(endpoint&&supplied&&tech_award_lifecycle_&&tech_award_pending_input_.expired(),"TechAward input absent context or already reserved");
 runtime::BnwMajorTechClientEndpoint::Impl::Guard guard(*endpoint->impl_);
 require_major_tech_receiver_current(*endpoint);const auto context=supplied->retain_queued_canonical(supplied);
 const auto request=context->retained_request_for_input();const auto* published=frame.snapshot.find<BnwTechAwardContextSnapshot>();
 const auto* stamp=frame.snapshot.find<BnwClientFrameStamp>();
 using Stage=TechAwardLifecycleStorage::Stage;
 input_need(published&&stamp&&tech_award_lifecycle_->stage==Stage::open&&tech_award_lifecycle_->semaphore==1&&screens_.focused_screen()==std::optional<std::string>{"TechAwardPopup"}&&
  published->occurrence==tech_award_context_occurrence_&&published->generation==tech_award_context_generation_&&published->context.visible&&published->context.queued&&published->context.auto_size_complete,
  "TechAward input requires genuine source modal publication");
 const std::string slot=input==BnwTechAwardInput::continue_button?"ContinueButton":input==BnwTechAwardInput::close_button?"CloseButton":"ContextPtr.InputHandler";
 std::size_t slots=0;for(const auto& value:tech_award_inputs_)if(value.occurrence==published->occurrence&&value.generation==published->generation&&value.native_control==slot)++slots;
 input_need(slots==1&&tech_award_input_occurrence_<UINT64_MAX,"TechAward input original slot absent/overflow");
 auto p=std::make_shared<BnwTechAwardInputDispatch::Impl>();p->ui=self;p->endpoint=endpoint;p->context=context;p->request=request;p->issued_snapshot=issued_plot_yield_snapshot_;
 p->stamp=*stamp;p->input=input;p->occurrence=tech_award_input_occurrence_+1;p->source_occurrence=published->occurrence;p->generation=published->generation;
 p->source_revision=stamp->identity.ui_source_revision;p->issued_sequence=frame.sequence;p->expected_sequence=frame.sequence;p->processed_before=tech_award_lifecycle_->processed_events.size();
 auto token=std::shared_ptr<const BnwTechAwardInputDispatch>(new BnwTechAwardInputDispatch(p));p->self=token;
 require_issued_frame_finality(frame);require_major_tech_receiver_current(*endpoint);context->require_queued_current();
 input_need(tech_award_pending_input_.expired()&&tech_award_input_occurrence_<UINT64_MAX&&p->occurrence==tech_award_input_occurrence_+1,
            "TechAward input issuer reentered or occurrence changed before reservation");
 tech_award_pending_input_=token;++tech_award_input_occurrence_;token->require_current();return token;
}
void LiveUiHost::complete_tech_award_input_after_frame(const std::shared_ptr<const BnwTechAwardInputDispatch>& supplied,const LiveUiFrame& frame) {
 input_need(bool(supplied),"TechAward input completion token missing");const auto token=supplied->retain_canonical_identity(supplied);
 input_need(token->impl_->ui.lock().get()==this,"TechAward input belongs to another UiHost");
 try {
 input_need(plot_yield_ui_thread_==std::this_thread::get_id(),"TechAward input frame completion foreign thread");
 auto& p=*token->impl_;input_need(tech_award_pending_input_.lock().get()==token.get()&&p.phase==BnwTechAwardInputPhase::pending_frame,"TechAward input completion foreign/replayed");
 require_issued_frame_finality(frame);input_need(current(frame)&&frame.sequence>p.expected_sequence,"TechAward close requires actual new final UI frame");
 const auto* stamp=frame.snapshot.find<BnwClientFrameStamp>();const auto* modal=frame.snapshot.find<BnwTechAwardModalSnapshot>();
 input_need(stamp&&modal&&stamp->identity.ui_source_revision==p.source_revision&&modal->occurrence==p.source_occurrence&&modal->timer_semaphore==0&&!modal->modal_queued&&!modal->source_showhide_pending&&
  modal->processed.size()==p.processed_before+1&&!frame.snapshot.find<BnwTechAwardContextSnapshot>()&&!screens_.focused_screen(),"TechAward actual new frame did not consume closed modal");
 token->require_current();require_issued_frame_finality(frame);p.phase=BnwTechAwardInputPhase::completed;tech_award_pending_input_.reset();
 }catch(...){token->fail_actual_dispatch();throw;}
}

void LiveUiHost::retire_tech_award_input_after_failure(const std::shared_ptr<const BnwTechAwardInputDispatch>& supplied) noexcept {
 // No currentness callbacks: the renderer/source may already have failed.
 // Reject forged aliases before touching the genuine context. These weak
 // issuance/host links are immutable for the token's entire lifetime.
 if(!supplied)return;
 const auto token=supplied->impl_->self.lock();
 if(!token||token.get()!=supplied.get()||token.owner_before(supplied)||supplied.owner_before(token))return;
 const auto ui=token->impl_->ui.lock();const auto self=selected_owner_lifetime_.lock();
 if(!ui||!self||ui.get()!=this||self.get()!=this||ui.owner_before(self)||self.owner_before(ui))return;
 token->fail_actual_dispatch(); // foreign release only sets atomic retirement
}
}
