#include "civ5/ui/bnw_plot_yield_refresh_ui_adapter.hpp"
#include "civ5/ui/bnw_client_selection_control.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/ui/bnw_plot_yield_refresh_owned_ui_context.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include <atomic>
#include <algorithm>
#include <climits>
#include <functional>
#include <stdexcept>
#include <thread>
namespace civ5::ui {
namespace {void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}}
struct BnwPlotYieldRefreshUiAdapter::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::shared_ptr<const SelectedBnwPreviewHost> host;
 std::shared_ptr<const runtime::LiveSessionState> state;
 std::shared_ptr<const saves::SavedSession> source;
 std::function<bool()> rules_current;
 runtime::SessionCheckpointIdentity identity;
 std::uint64_t generation=0;
 std::atomic<bool> retired{false},poisoned{false};bool busy=false;
 std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage> storage;
 std::shared_ptr<BnwClientSceneFrame> scene;std::shared_ptr<LiveUiHost> ui;
 std::shared_ptr<runtime::BnwPlotYieldRefreshPending> active;
 bool attempted=false;
};
struct BnwPlotYieldRefreshUiAdapter::Call {
 std::shared_ptr<Impl> p;std::shared_ptr<void> context_operation;
 explicit Call(std::shared_ptr<Impl> value):p(std::move(value)){
  need(bool(p),"Plot UI facade absent");
  if(p->thread!=std::this_thread::get_id()){p->poisoned.store(true);throw std::runtime_error("Plot UI foreign operation thread");}
  need(!p->retired.load()&&!p->poisoned.load(),"Plot UI retired/poisoned facade");
  if(p->busy){p->poisoned.store(true);throw std::runtime_error("Plot UI recursive operation");}
  p->busy=true;p->attempted=false;
  try{if(p->storage)context_operation=BnwPlotYieldRefreshOwnedUiContext::retain_adapter_operation(p->storage);}
  catch(...){p->busy=false;throw;}
 }
 ~Call(){p->active.reset();p->attempted=false;p->busy=false;}
};
BnwPlotYieldRefreshUiAdapter::BnwPlotYieldRefreshUiAdapter(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwPlotYieldRefreshUiAdapter::~BnwPlotYieldRefreshUiAdapter(){retire();}
void BnwPlotYieldRefreshUiAdapter::retire()noexcept{if(impl_)impl_->retired.store(true);}
std::shared_ptr<BnwPlotYieldRefreshUiAdapter> BnwPlotYieldRefreshUiAdapter::from_selected_host(
 std::shared_ptr<const SelectedBnwPreviewHost> host){
 auto p=std::make_shared<Impl>();need(bool(host),"Plot UI tail actual Host absent");p->host=std::move(host);
 const auto& session=p->host->session();
 need(session.plot_yield_refresh_thread_==p->thread,"Plot UI tail foreign session thread");
 p->rules_current=p->host->database().immutable_preview_fingerprint_currentness();
 need(p->rules_current&&p->rules_current(),"Plot UI tail immutable actual rules retired");
 need(session.bnw_game_debug_owner_current(),"Plot UI tail genuine imported Game absent");
 p->identity=session.debug_->identity;
 need(p->identity.payload_sha256==p->host->identity().payload_sha256&&
  p->identity.rules_sha256==p->host->identity().rules_sha256&&
  p->host->database().gameplay_fingerprint_sha256()==p->identity.rules_sha256,
  "Plot UI tail SAME Host/source/rules identity differs");
 (void)session.bnw_game_debug_mode();
 p->state=session.state_;p->source=session.source_;p->generation=session.state_generation_;
 auto out=std::shared_ptr<BnwPlotYieldRefreshUiAdapter>(new BnwPlotYieldRefreshUiAdapter(std::move(p)));
 out->require_current();return out;
}
void BnwPlotYieldRefreshUiAdapter::require_current()const{
 const auto p=impl_;
 // Read-only foreign diagnostics preserve their previous refusal contract.
 need(p&&p->thread==std::this_thread::get_id(),"Plot UI tail foreign constructor thread");
 Call call(p);require_current(p);
}
void BnwPlotYieldRefreshUiAdapter::require_current(const std::shared_ptr<Impl>& held){
 const auto& p=*held;
 need(!p.retired.load()&&!p.poisoned.load(),"Plot UI facade retired/poisoned");
 if(p.storage)BnwPlotYieldRefreshOwnedUiContext::require_storage_current(p.storage);
 need(p.thread==std::this_thread::get_id(),"Plot UI tail foreign constructor thread");
 need(p.host&&p.rules_current&&p.rules_current(),"Plot UI tail immutable actual rules retired");
 const auto& session=p.host->session();
 need(session.bnw_game_debug_owner_current()&&session.state_==p.state&&session.source_==p.source&&
  session.state_generation_==p.generation,"Plot UI tail actual source/allocation/generation retired");
 need(session.debug_->identity.payload_sha256==p.identity.payload_sha256&&
  session.debug_->identity.rules_sha256==p.identity.rules_sha256,"Plot UI tail actual import identity changed");
 (void)session.bnw_game_debug_mode();
 need(!p.retired.load()&&!p.poisoned.load(),"Plot UI retired during currentness callback");
 if(p.storage){
  need(p.scene&&p.ui&&p.scene->owned_plot_yield_ui_sources_==p.storage&&
       p.ui->owned_plot_yield_ui_sources_==p.storage&&p.scene->selected_host_==p.host,
       "Plot UI private owned storage/Scene/Host binding lost");
  BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(p.storage);
 }
 for(const auto& e:p.state->plot_gameplay_events)
  need(e.generation==p.generation,"Checkpoint plot notifications require actual generation reconciliation");
 for(const auto& e:p.state->city_dirty_events)
  need(e.generation==p.generation,"Checkpoint UI notifications require actual generation reconciliation");
}
runtime::BnwPlotYieldRefreshProgress BnwPlotYieldRefreshUiAdapter::continue_plot(std::int64_t plot){
 const auto p=impl_;Call call(p);return continue_plot(p,plot);
}
runtime::BnwPlotYieldRefreshProgress BnwPlotYieldRefreshUiAdapter::continue_plot(const std::shared_ptr<Impl>& held,std::int64_t plot){
 require_current(held);const auto& p=*held;const auto& session=p.host->session();
 const auto slot=session.plot_yield_refresh_pending_.find(plot);
 need(slot!=session.plot_yield_refresh_pending_.end()&&slot->second,"Plot UI tail no registered pending cursor");
 const auto pending=slot->second;auto facts=pending->ui_tail_source();held->active=pending;
 need(facts.state.get()==p.state.get()&&facts.generation==p.generation&&facts.plot==plot,
  "Plot UI tail pending is not SAME current session source");
 using Phase=runtime::BnwPlotYieldRefreshPhase;
 need(facts.phase==Phase::city_screen_predicate||facts.phase==Phase::city_screen_dirty||facts.phase==Phase::symbols,
  "Plot UI tail cannot replay completed/non-tail operation");
 try{
  if(facts.phase==Phase::city_screen_predicate){
   need(facts.city.has_value(),"Plot UI tail actual working city absent");
   held->attempted=true;pending->begin_required_effect(); // BEFORE native getters/mandatory selection.
   const auto team=runtime::detail::bnw_player_team_read(*facts.state,facts.city->first);
   const auto active=runtime::detail::bnw_game_active_team_read(*facts.state);
   bool dirty=false;
   if(team==active){
    const auto selection=session.city_selection_.lock();
    need(bool(selection),"Plot UI tail genuine actual UI selection owner unavailable");
    selection->require_current();dirty=selection->is_selected(facts.city->first,facts.city->second);
    selection->require_current();
   }
   require_current(held);pending->complete_city_screen_predicate(dirty);
   if(!dirty)return pending->progress();
   facts=pending->ui_tail_source(); // True branch immediately reaches setDirty.
  }
  if(facts.phase==Phase::city_screen_dirty){
   need(facts.city&&facts.next_yield<6,"Plot UI dirty event actual source identity missing");
   auto& state=*facts.state;
   need(state.city_dirty_next_occurrence<UINT64_MAX&&state.ui_source_revision<UINT64_MAX,
    "Plot UI dirty occurrence/revision overflow");
   auto events=state.city_dirty_events;
   events.push_back({state.city_dirty_next_occurrence,p.generation,facts.city->first,facts.city->second,
    plot,static_cast<std::int8_t>(facts.next_yield),runtime::BnwCityDirtyKind::city_screen,true});
   require_current(held);pending->require_current();held->attempted=true;pending->begin_required_effect();
   state.city_dirty_events.swap(events);++state.city_dirty_next_occurrence;++state.ui_source_revision;
   pending->complete_required_effect();require_current(held);return pending->progress();
  }
  need(facts.phase==Phase::symbols,"Plot UI tail exact source phase differs");
  auto& state=*facts.state;
  need(state.plot_gameplay_next_occurrence<UINT64_MAX&&state.ui_source_revision<UINT64_MAX,
   "Plot gameplay occurrence/revision overflow");
  runtime::BnwPlotGameplayEvent event;
  event.occurrence=state.plot_gameplay_next_occurrence;event.generation=p.generation;
  event.operation=facts.operation;event.target_revision=state.ui_source_revision+1;
  event.map_guid=facts.map_guid;event.plot=plot;event.x=facts.x;event.y=facts.y;event.yields=facts.yields;
  auto events=state.plot_gameplay_events;events.push_back(event);
  require_current(held);pending->require_current();held->attempted=true;pending->begin_required_effect();
  // Actual typed gameplay event publication replaces the native void DLL
  // notification. There is no renderer callback, wait or fabricated ack.
  state.plot_gameplay_events.swap(events);++state.plot_gameplay_next_occurrence;++state.ui_source_revision;
  pending->complete_required_effect();require_current(held);return pending->progress();
 }catch(...){if(held->attempted)pending->fail_required_effect();throw;}
}
std::vector<runtime::BnwPlotGameplayEvent> BnwPlotYieldRefreshUiAdapter::take_plot_events(){
 const auto p=impl_;Call call(p);require_current(p);auto& state=*p->host->session().state_;
 auto out=state.plot_gameplay_events;require_current(p);state.plot_gameplay_events.clear();return out;
}

std::shared_ptr<BnwPlotYieldRefreshUiAdapter> BnwPlotYieldRefreshUiAdapter::from_owned_context(
 std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage> storage,
 std::shared_ptr<BnwClientSceneFrame> scene,std::shared_ptr<LiveUiHost> ui){
 need(storage&&scene&&ui&&scene->selected_host_,"Plot UI private actual Context children absent");
 need(scene->owned_plot_yield_ui_sources_==storage&&ui->owned_plot_yield_ui_sources_==storage,
      "Plot UI private Context source storage differs");
 auto out=from_selected_host(scene->selected_host_);const auto p=out->impl_;
 p->storage=std::move(storage);p->scene=std::move(scene);p->ui=std::move(ui);
 // Context's yield_adapter already owns its real ContextOperation entry.
 require_current(p);return out;
}
BnwPlotYieldRefreshUiAdvance BnwPlotYieldRefreshUiAdapter::advance(
 const std::shared_ptr<const runtime::BnwPlotYieldRefreshPending>& supplied,
 const LiveUiFrame& admitted){
 const auto incoming=admitted; // Retain argument payload BEFORE any borrowed source callback.
 const auto p=impl_;Call call(p);
 need(p->storage&&p->scene&&p->ui,"Plot UI advance requires privately owned Context");
 const auto scene=p->scene;const auto ui=p->ui;
 // Scalar/storage comparisons only. Reject stripped/foreign frames BEFORE
 // currentness, provider, font or any borrowed native source callback.
 auto issued=ui->issued_plot_yield_snapshot_;auto sequence=ui->issued_plot_yield_sequence_;
 need(scene->plot_yield_ui_thread_==p->thread&&ui->plot_yield_ui_thread_==p->thread&&
      issued&&sequence==incoming.sequence&&ui->sequence_==sequence&&incoming.owner==ui.get()&&
      incoming.snapshot.same_payload_storage(*issued),"Plot UI incoming frame not privately issued");
 need(bool(supplied),"Plot UI advance actual pending absent");
 const auto& session=p->host->session();
 const auto found=std::find_if(session.plot_yield_refresh_pending_.begin(),session.plot_yield_refresh_pending_.end(),
  [&](const auto& row){return row.second&&row.second.get()==supplied.get();});
 need(found!=session.plot_yield_refresh_pending_.end(),"Plot UI advance foreign actual pending slot");
 const auto actual=found->second;const auto plot=found->first;
 require_current(p);actual->require_current();
 const auto state=session.read_state_lease();const auto generation=session.state_generation();
 need(state==p->state&&scene->scene_state_==state.get()&&scene->scene_state_generation_==generation&&
      ui->city_selection_control_&&session.city_selection_.lock()==ui->city_selection_control_&&
      ui->selected_owner_lifetime_.lock().get()==ui.get(),"Plot UI advance SAME allocation/canonical UI lost");
 need(!scene->plot_yield_ui_busy_.load()&&!ui->plot_yield_ui_busy_.load()&&
      !scene->plot_yield_ui_poisoned_.load()&&!ui->plot_yield_ui_poisoned_.load(),"Plot UI advance source busy/poisoned");
 scene->plot_yield_ui_busy_.store(true);ui->plot_yield_ui_busy_.store(true);
 struct SourceBusy {std::shared_ptr<BnwClientSceneFrame> scene;std::shared_ptr<LiveUiHost> ui;
  ~SourceBusy(){scene->plot_yield_ui_busy_.store(false);ui->plot_yield_ui_busy_.store(false);}} source_busy{scene,ui};
 auto frame=scene->frame_;auto layout=incoming;
 const auto* stamp=incoming.snapshot.find<BnwClientFrameStamp>();
 need(frame&&stamp&&stamp==frame->ui.find<BnwClientFrameStamp>()&&
      stamp->identity.session==p->host->identity().payload_sha256&&stamp->identity.player==p->host->active_player_id(),
      "Plot UI actual Scene/source publication differs from private bundle");
 const auto initial_progress=actual->progress();
 const std::uint64_t initial_delta=std::uint64_t(initial_progress.plot_writes)+initial_progress.terrain_writes;
 need(stamp->identity.ui_source_revision<=UINT64_MAX-initial_delta,
      "Plot UI initial pending revision overflow");
 auto expected_world_revision=stamp->identity.ui_source_revision+initial_delta;
 need(state->ui_source_revision==expected_world_revision,
      "Plot UI initial stale layout is not this fresh actual prefix");
 const auto guard=[&]{
  require_current(p);actual->require_current();
  need(session.read_state_lease()==state&&session.state_generation()==generation&&
       session.plot_yield_refresh_pending_.at(plot)==actual&&
       state->ui_source_revision==expected_world_revision&&
       !scene->plot_yield_ui_poisoned_.load()&&!ui->plot_yield_ui_poisoned_.load(),"Plot UI advance source changed");
 };
 const auto seal=[&]{
  BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(p->storage);
  ui->require_issued_frame_finality(layout);
  need(scene->frame_==frame,"Plot UI retained raw Scene publication changed");
  scene->owner_.require_plot_tail_publication_identity(frame,p->storage,p->host,ui,state,generation,expected_world_revision);
  actual->require_current();
  need(!p->retired.load()&&!p->poisoned.load()&&session.read_state_lease()==state&&
       session.state_generation()==generation&&session.plot_yield_refresh_pending_.at(plot)==actual&&
       ui->issued_plot_yield_snapshot_==issued&&ui->issued_plot_yield_sequence_==sequence&&ui->sequence_==sequence&&
       ui->city_selection_control_->revision()==frame->stamp.identity.selection_revision,
       "Plot UI advance final owner/publication seal changed");
 };
 BnwPlotYieldRefreshUiAdvance result;
 try{
  for(std::size_t step=0;step<6;++step){
   guard();auto progress=actual->progress();
   using Phase=runtime::BnwPlotYieldRefreshPhase;
   if(progress.phase==Phase::symbols){
    // No native symbols listener/renderer acknowledgement is manufactured.
    // A returned refreshed_ui is only this actual provider publication.
    scene->invalidate();result.refreshed_ui.reset();result.progress=progress;return result;
   }
   need(progress.phase==Phase::city_screen_predicate||progress.phase==Phase::city_screen_dirty,
        "Plot UI advance cannot replay completed/non-tail operation");
   const auto facts=actual->ui_tail_source();
   need(facts.state==state&&facts.generation==generation&&facts.plot==plot&&facts.city&&facts.next_yield<6,
        "Plot UI advance current tail operands absent");
   const auto city=*facts.city;const auto viewport=frame->stamp.viewport;const auto camera=frame->camera;
   const auto revision=ui->plot_city_dirty_revision_;const auto old_sequence=sequence;
   need(revision<UINT64_MAX&&old_sequence<=UINT64_MAX-2&&frame->stamp.identity.revision<=UINT64_MAX-2&&
        frame->stamp.identity.frame<UINT64_MAX,"Plot UI advance publication counters exhausted");
   // Preallocate the real private host queue BEFORE any native tail effect.
   auto queued=ui->plot_city_dirty_queue_;
   queued.push_back({facts.operation,revision+1,old_sequence,0,plot,city.first,city.second,facts.next_yield,false});
   const auto occurrence=state->city_dirty_next_occurrence;
   guard();seal();
   const auto before_progress=progress;
   p->active=actual;progress=continue_plot(p,plot);
   need(progress.plot_writes>=before_progress.plot_writes&&progress.terrain_writes>=before_progress.terrain_writes&&
        state->city_dirty_next_occurrence>=occurrence&&state->city_dirty_next_occurrence-occurrence<=1,
        "Plot UI actual tail counters regressed or foreign event inserted");
   const std::uint64_t delta=std::uint64_t(progress.plot_writes-before_progress.plot_writes)+
        (progress.terrain_writes-before_progress.terrain_writes)+(state->city_dirty_next_occurrence-occurrence);
   need(expected_world_revision<=UINT64_MAX-delta,"Plot UI actual tail revision overflow");
   expected_world_revision+=delta;guard();
   if(state->city_dirty_next_occurrence==occurrence){
    // Predicate false: native prefix can perform subsequent stores. This
    // path has no mandatory CityScreen publication/consumer to acknowledge.
    if(progress.phase==Phase::complete||progress.phase==Phase::symbols){
     scene->invalidate();result.refreshed_ui.reset();result.progress=progress;return result;
    }
    continue;
   }
   need(occurrence<UINT64_MAX&&state->city_dirty_next_occurrence==occurrence+1&&!state->city_dirty_events.empty(),
        "Plot UI advance unexpected factual dirty occurrence");
   const auto event=state->city_dirty_events.back();
   need(event.occurrence==occurrence&&event.generation==generation&&event.owner==city.first&&event.city==city.second&&
        event.plot==plot&&event.yield==static_cast<std::int8_t>(facts.next_yield)&&
        event.kind==runtime::BnwCityDirtyKind::city_screen&&event.value,
        "Plot UI advance factual event differs from actual tail");
   guard();seal();
   need(ui->plot_city_dirty_revision_==revision&&ui->sequence_==old_sequence,
        "Plot UI advance host queue changed before publication");
   ui->plot_city_dirty_queue_.swap(queued);ui->plot_city_dirty_revision_=revision+1;++ui->sequence_;
   scene->invalidate();guard();
   const auto next_frame=scene->begin(viewport,camera);guard();
   need(next_frame&&next_frame!=frame&&scene->frame_==next_frame,"Plot UI consumer did not capture SAME new Scene");
   auto next_ui=ui->begin_frame(viewport.framebuffer_width,viewport.framebuffer_height);guard();
   const auto next_issued=ui->issued_plot_yield_snapshot_;
   need(next_issued&&next_ui.owner==ui.get()&&next_ui.sequence==old_sequence+2&&
        ui->sequence_==next_ui.sequence&&ui->issued_plot_yield_sequence_==next_ui.sequence&&
        next_ui.snapshot.same_payload_storage(*next_issued)&&ui->current(next_ui)&&scene->accepts(*ui,next_ui),
        "Plot UI consumer did not publish SAME actual Host frame");guard();
   const auto* cities=next_ui.snapshot.find<CityScreenFrame>();
   need(cities&&cities->player_id==city.first&&cities->selected_city_id==city.second,
        "Plot UI consumer actual CityScreen payload absent");
   BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(p->storage);
   ui->require_issued_frame_finality(next_ui);scene->require_publication_finality(next_frame);actual->require_current();
   need(!p->retired.load()&&!p->poisoned.load()&&session.read_state_lease()==state&&
        session.state_generation()==generation&&session.plot_yield_refresh_pending_.at(plot)==actual&&
        ui->plot_city_dirty_revision_==revision+1&&ui->sequence_==old_sequence+2&&
        ui->issued_plot_yield_snapshot_==next_issued&&scene->owner_.current_frame()==next_frame&&
        !ui->plot_city_dirty_queue_.empty()&&ui->plot_city_dirty_queue_.back().operation==facts.operation&&
        ui->plot_city_dirty_queue_.back().yield==facts.next_yield&&!ui->plot_city_dirty_queue_.back().consumed,
        "Plot UI consumer final source/queue changed");
   // Only a factual completed SAME Scene/provider consumption record. The
   // native effect already published its state event, which remains queued.
   auto& record=ui->plot_city_dirty_queue_.back();record.new_sequence=next_ui.sequence;record.consumed=true;
   ++result.dirty_deliveries;frame=next_frame;layout=std::move(next_ui);result.refreshed_ui=layout;
   issued=next_issued;sequence=layout.sequence;result.progress=progress;
   if(progress.phase==Phase::complete||progress.phase==Phase::symbols)return result;
  }
  throw std::runtime_error("Plot UI advance six-yield finite bound exceeded");
 }catch(...){
  if(p->attempted&&p->active)p->active->fail_required_effect();
  p->poisoned.store(true);scene->plot_yield_ui_poisoned_.store(true);ui->plot_yield_ui_poisoned_.store(true);throw;
 }
}
}
