#include "civ5/ui/bnw_engine_ui_end_turn_queue.hpp"
#include "civ5/runtime/bnw_player_turn_active_dirty_issuer.hpp"
#include "civ5/ui/bnw_engine_ui_turn_owner.hpp"
#include "civ5/ui/live_ui_host.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/runtime/bnw_owned_gameplay_event_pump_loan.hpp"
#include <limits>
#include <stdexcept>
namespace civ5::ui {
namespace {void need(bool v,const char* m){if(!v)throw std::runtime_error(m);}}
struct BnwEngineUiEndTurnQueue::State {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwEngineUiTurnOwner> engine;
 std::weak_ptr<const LiveUiHost> ui;
 std::weak_ptr<const SelectedBnwPreviewHost> host;
 std::weak_ptr<const runtime::LiveSessionState> world;
 std::shared_ptr<const Consumer> consumer;
 std::unique_ptr<runtime::BnwStockLuaAttemptLease> source;
 std::deque<std::unique_ptr<Event>> events;
 std::atomic<bool> alive{true},failed{false};
 bool active=false,pump=false;std::uint64_t next=0,boundary=0;
};
BnwEngineUiEndTurnQueue::BnwEngineUiEndTurnQueue(std::shared_ptr<State> s):state_(std::move(s)){}
BnwEngineUiEndTurnQueue::~BnwEngineUiEndTurnQueue(){state_->alive=false;state_->failed=true;}
void BnwEngineUiEndTurnQueue::identity(const std::shared_ptr<State>& s){
 need(std::this_thread::get_id()==s->creator,"Event69 foreign thread");
 need(s->alive&&!s->failed,"Event69 retired/failed owner");
 const auto e=s->engine.lock();const auto u=s->ui.lock();const auto h=s->host.lock();const auto w=s->world.lock();
 need(e&&u&&h&&w&&e.use_count()>1&&u.use_count()>1&&h.use_count()>1&&w.use_count()>1,"Event69 outside canonical owner expired");
 // UNJOINED Engine-owner ABI hook: real UI member/self controls, all actual
 // Engine Impl weak lineages and known outside pins around rules callbacks.
 // Queue supplies weak descriptors, never a callback or an admission boolean.
 e->require_end_turn_queue_identity(s->ui,s->host,s->world);
 need(s->alive&&!s->failed&&e.use_count()>1&&u.use_count()>1&&h.use_count()>1&&w.use_count()>1,
  "Event69 currentness callback retired outside canonical owner");
}
runtime::LuaScriptHost& BnwEngineUiEndTurnQueue::checked_vm(const std::shared_ptr<State>& s){
 need(s->consumer!=nullptr,"Event69 selected consumer is unjoined");
 const auto c=s->consumer->lifetime.lock();
 need(c&&c->creator==std::this_thread::get_id()&&c->alive&&!c->failed&&c->host&&
  c->vm==s->consumer->vm,"Event69 actual VM retired/replaced");
 return *c->host;
}
std::shared_ptr<BnwEngineUiEndTurnQueue> BnwEngineUiEndTurnQueue::bind_fresh(
 std::shared_ptr<const BnwEngineUiTurnOwner> e,std::shared_ptr<const LiveUiHost> u,
 std::shared_ptr<const SelectedBnwPreviewHost> h){
 need(e&&u&&h,"Event69 fresh binding incomplete");
 auto s=std::make_shared<State>();s->engine=e;s->ui=u;s->host=h;s->world=h->session().read_state_lease();
 e.reset();u.reset();h.reset(); // supplied binding argument pins do not count as outside owners
 identity(s);return std::shared_ptr<BnwEngineUiEndTurnQueue>(new BnwEngineUiEndTurnQueue(std::move(s)));
}
void BnwEngineUiEndTurnQueue::attach_consumer(BnwEngineUiTurnOwner& e,std::shared_ptr<const Consumer> c,
 const runtime::BnwStockLuaAttemptLease& lease){
 const auto s=state_;need(std::this_thread::get_id()==s->creator,"Event69 foreign entry");
 if(s->active){s->failed=true;throw std::runtime_error("Event69 attach reentry");}
 s->active=true;try{
  identity(s);
  need(s->engine.lock().get()==&e&&!s->consumer&&!s->source&&s->events.empty(),"Event69 duplicate/foreign attach");
  const auto serial=lease.operation_serial();lease.require_operation(serial);
  need(c!=nullptr,"Event69 missing actual selected consumer");
  s->consumer=std::move(c);checked_vm(s).require_end_turn_consumer_owned(*s->consumer);
  lease.require_operation(serial);identity(s);
  s->source=std::make_unique<runtime::BnwStockLuaAttemptLease>(lease);s->active=false;
 }catch(...){s->failed=true;s->active=false;throw;}
}
std::uint64_t BnwEngineUiEndTurnQueue::enqueue_changed_false(BnwEngineUiTurnOwner& e,
 const runtime::BnwStockLuaAttemptLease& lease){
 const auto s=state_;need(std::this_thread::get_id()==s->creator,"Event69 foreign entry");
 if(s->active){s->failed=true;throw std::runtime_error("Event69 enqueue reentry");}
 s->active=true;try{
  identity(s);
  need(s->engine.lock().get()==&e&&s->source,"Event69 foreign/unactivated changed setter");
  lease.require_same_source_as_stored(*s->source);const auto serial=lease.operation_serial();
  checked_vm(s).require_end_turn_consumer_owned(*s->consumer);lease.require_operation(serial);identity(s);
  need(s->next!=std::numeric_limits<std::uint64_t>::max(),"Event69 occurrence exhausted");
  auto event=std::make_unique<Event>(s->next+1,serial);const auto n=event->occurrence;
  s->events.push_back(std::move(event));s->next=n; // no Lua call or acknowledgement
  identity(s);lease.require_operation(serial);s->active=false;return n;
 }catch(...){s->failed=true;s->active=false;throw;}
}
void BnwEngineUiEndTurnQueue::mark_setter_return(BnwEngineUiTurnOwner& e,std::uint64_t n,
 const runtime::BnwStockLuaAttemptLease& lease){
 const auto s=state_;need(std::this_thread::get_id()==s->creator,"Event69 foreign entry");
 if(s->active){s->failed=true;throw std::runtime_error("Event69 setter-return reentry");}
 s->active=true;try{
  identity(s);
  need(s->engine.lock().get()==&e&&s->source&&!s->events.empty(),"Event69 setter return has no actual occurrence");
  lease.require_same_source_as_stored(*s->source);const auto serial=lease.operation_serial();
  auto& event=*s->events.back();need(event.occurrence==n&&event.operation==serial&&!event.setter_returned,"Event69 setter receipt substituted");
  identity(s);lease.require_operation(serial);event.setter_returned=true;s->active=false;
 }catch(...){s->failed=true;s->active=false;throw;}
}
BnwEngineUiEndTurnQueue::PumpLoan::PumpLoan(BnwEngineUiEndTurnQueue& q,const runtime::BnwStockLuaAttemptLease& lease,
 const runtime::BnwOwnedGameplayEventPumpLoan& pump):actual_pump_(pump),state_(q.state_){
 pump.require_for_source(lease);
 need(std::this_thread::get_id()==state_->creator,"Event69 foreign pump entry");if(state_->active){state_->failed=true;throw std::runtime_error("Event69 pump reentry");}
 state_->active=true;state_->pump=true;
 try{
  identity(state_);
  need(state_->source!=nullptr,"Event69 pump is unactivated");lease.require_same_source_as_stored(*state_->source);
  operation_=lease.operation_serial();identity(state_);lease.require_operation(operation_);
  actual_pump_.require_for_source(lease);boundary_=actual_pump_.boundary();
  need(boundary_>state_->boundary,"Event69 actual later pump boundary reused");state_->boundary=boundary_;
 }catch(...){state_->failed=true;state_->active=false;state_->pump=false;throw;}
}
BnwEngineUiEndTurnQueue::PumpLoan::~PumpLoan(){if(!returned_)state_->failed=true;state_->pump=false;state_->active=false;}
void BnwEngineUiEndTurnQueue::drain(PumpLoan& loan,const runtime::BnwStockLuaAttemptLease& lease){
 const auto s=state_;try{
  loan.actual_pump_.require_for_source(lease);
  need(loan.state_.get()==s.get()&&s->active&&s->pump&&loan.boundary_==s->boundary&&!loan.returned_,"Event69 lacks active actual pump loan");
  identity(s);lease.require_same_source_as_stored(*s->source);lease.require_operation(loan.operation_);
  while(!s->events.empty()){
   const auto* event=s->events.front().get();need(event->setter_returned,"Event69 setter has not unwound to return");
   need(event->operation<loan.operation_,"Event69 inline/same-operation drain refused");
   loan.actual_pump_.require_for_source(lease);
   identity(s);checked_vm(s).require_end_turn_consumer_owned(*s->consumer);lease.require_operation(loan.operation_);
   if(event->player_dirty_issuer){
    const auto engine=s->engine.lock();need(bool(engine),"Player Event69 actual Engine retired before dispatch");
    event->player_dirty_issuer->require_before_delivery_without_callbacks(*engine,lease);
    event->player_dirty_issuer->record_delivery_attempt_without_callbacks(*this,*event);
   }
   auto receipt=checked_vm(s).dispatch_end_turn_dirty_owned(*s->consumer,*event);
   if(event->player_dirty_issuer){
    need(receipt&&receipt->event_==event&&receipt->consumer_==s->consumer.get()&&receipt->occurrence_==event->occurrence,
     "Player Event69 lacks actual SAME protected callback Completion");
    event->player_dirty_issuer->record_delivered_without_callbacks(*this,*event,*receipt,loan.operation_);
   }
   identity(s);lease.require_operation(loan.operation_);
   need(receipt&&receipt->event_==event&&receipt->consumer_==s->consumer.get()&&receipt->occurrence_==event->occurrence,
    "Event69 lacks actual same-VM protected callback return");
   receipt.reset();loan.actual_pump_.require_for_source(lease);identity(s);lease.require_operation(loan.operation_);s->events.pop_front();
  }
  identity(s);lease.require_operation(loan.operation_);loan.actual_pump_.require_for_source(lease);loan.returned_=true;
 }catch(...){s->failed=true;throw;}
}
}

namespace civ5::ui {
bool BnwEngineUiEndTurnQueue::has_pending_owned()const{
 const auto s=state_;need(s->creator==std::this_thread::get_id(),"Event69 idle read foreign creator");
 if(s->active){s->failed=true;throw std::runtime_error("Event69 idle read reentry");}
 s->active=true;
 try{
  identity(s);const bool pending=!s->events.empty();
  need(s->source||(s->events.empty()&&!s->consumer),"Event69 unactivated storage unexpectedly has effects");
  identity(s);s->active=false;return pending;
 }catch(...){s->failed=true;s->active=false;throw;}
}
}

namespace civ5::ui {
void BnwEngineUiEndTurnQueue::publish_player_turn_dirty(BnwEngineUiTurnOwner& engine,
 const std::shared_ptr<const runtime::BnwPlayerTurnActiveDirtyIssuer>& issuer,const runtime::BnwStockLuaAttemptLease& lease){
 const auto s=state_;need(std::this_thread::get_id()==s->creator,"Player Event69 foreign creator");
 need(issuer!=nullptr,"Player Event69 missing closed issuer");
 if(s->active){s->failed=true;throw std::runtime_error("Player Event69 publisher reentry");}
 s->active=true;try{
  identity(s);need(s->engine.lock().get()==&engine&&s->source,"Player Event69 actual Engine/consumer not activated");
  lease.require_same_source_as_stored(*s->source);const auto serial=lease.operation_serial();
  checked_vm(s).require_end_turn_consumer_owned(*s->consumer);lease.require_operation(serial);identity(s);
  need(s->next!=std::numeric_limits<std::uint64_t>::max(),"Player Event69 occurrence exhausted");
  auto event=std::make_unique<Event>(s->next+1,serial,issuer);auto* actual=event.get();
  // All real providers precede the final physical Cursor/source seal.
  identity(s);checked_vm(s).require_end_turn_consumer_owned(*s->consumer);lease.require_operation(serial);identity(s);
  need(s->alive&&!s->failed&&s->active&&actual->occurrence==s->next+1&&!issuer->queued_without_callbacks(),
   "Player Event69 staged allocation/counter changed");
  issuer->require_before_publication_without_callbacks(engine,lease);
  s->events.push_back(std::move(event));s->next=actual->occurrence;
  issuer->record_queued_without_callbacks(*this,*actual); // history before POST callbacks
  identity(s);lease.require_operation(serial);identity(s);
  need(s->alive&&!s->failed&&s->active&&!s->events.empty()&&s->events.back().get()==actual,
   "Player Event69 publisher physical event changed before return");
  issuer->require_before_publication_without_callbacks(engine,lease);
  actual->setter_returned=true;issuer->record_publisher_return_without_callbacks(*this,*actual);s->active=false;
 }catch(...){s->failed=true;s->active=false;throw;}
}
}
