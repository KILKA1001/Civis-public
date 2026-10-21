#include "civ5/graphics/bnw_unit_should_dim_transport.hpp"
// Actual common owner must supply this closed interface before integration.
// This provider cannot manufacture a native-unwind pump from a source serial.
#include "civ5/runtime/bnw_owned_gameplay_event_pump_loan.hpp"
#include <algorithm>
#include <chrono>
#include <climits>
#include <deque>
#include <limits>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
namespace civ5::graphics {
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class A,class B>bool same(const A& a,const B& b){
 return a&&b&&a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
}
template<class A,class B>bool owner_same(const A& a,const B& b){
 return !a.owner_before(b)&&!b.owner_before(a);
}
std::uint32_t packed(std::int32_t owner,std::int32_t id){
 need(owner>=0&&owner<=255&&id>=-1&&id<=0xffffff,"ShouldDim unsupported packed24/8 domain");
 return (static_cast<std::uint32_t>(id)&0xffffffu)|(static_cast<std::uint32_t>(owner)<<24);
}
void add_pin(std::size_t& count,std::size_t amount=1){
 need(amount<std::numeric_limits<std::size_t>::max()-count,"ShouldDim internal owner pins exhausted");
 count+=amount;
}
}
struct BnwUnitShouldDimSourceLoan::State {
 const std::thread::id creator=std::this_thread::get_id();
 const runtime::BnwStockLuaAttemptLease attempt;
 const runtime::BnwStockLuaAttemptLease::ScopeIdentity scope;
 // Only the closed RendererLoan can install its Core54 composite lender.
 // An empty lender means a fresh genuine receiver/registration operation.
 const std::function<void()> composite;
 bool alive=true,failed=false,checking=false;
 State(const runtime::BnwStockLuaAttemptLease& a,std::function<void()> c)
 :attempt(a),scope(a.pin_active_scope_without_callbacks()),composite(std::move(c)){}
};
BnwUnitShouldDimSourceLoan::BnwUnitShouldDimSourceLoan(const runtime::BnwStockLuaAttemptLease& a)
 :BnwUnitShouldDimSourceLoan(a,{}){}
BnwUnitShouldDimSourceLoan::BnwUnitShouldDimSourceLoan(const runtime::BnwStockLuaAttemptLease& a,std::function<void()> c)
 :state_(std::make_shared<State>(a,std::move(c))){
 const auto p=state_;need(p->scope.control&&p->scope.source_owner,"ShouldDim Source scope missing");
 // Producer preflight captures actual scope BEFORE the store. Its composite
 // dispatch lender is invoked only by the later genuine post-store enqueue.
 pure_state(p);if(!p->composite)full_state(p);
}
BnwUnitShouldDimSourceLoan::~BnwUnitShouldDimSourceLoan(){
 const auto p=state_;if(p->creator!=std::this_thread::get_id())std::terminate();p->alive=false;
}
void BnwUnitShouldDimSourceLoan::pure_state(const std::shared_ptr<State>& state){
 const auto& p=*state;
 need(p.creator==std::this_thread::get_id()&&p.alive&&!p.failed,"ShouldDim Source retired/foreign creator");
 p.attempt.require_scope_identity_without_callbacks(p.scope);
 const auto pins=p.scope.control->transport_source_pins.load();
 need(pins<=static_cast<std::uint64_t>(std::numeric_limits<long>::max())&&
  p.scope.source_owner.use_count()>static_cast<long>(pins),"ShouldDim Source outside owner lost");
}
void BnwUnitShouldDimSourceLoan::full_state(const std::shared_ptr<State>& p){
 pure_state(p);
 if(p->checking){fail_state(p);throw std::runtime_error("ShouldDim Source validator reentry");}
 p->checking=true;std::optional<std::function<void()>> copy;
 try{
  if(p->composite){copy.emplace(p->composite);pure_state(p);(*copy)();pure_state(p);
   copy.reset();pure_state(p);}
  else{p->attempt.require_operation(p->scope.serial);pure_state(p);}
  p->checking=false;
 }catch(...){fail_state(p);copy.reset();p->checking=false;throw;}
}
void BnwUnitShouldDimSourceLoan::fail_state(const std::shared_ptr<State>& p)noexcept{
 p->failed=true;p->attempt.invalidate();
}
void BnwUnitShouldDimSourceLoan::pure()const{pure_state(state_);}
void BnwUnitShouldDimSourceLoan::full()const{const auto p=state_;full_state(p);}
void BnwUnitShouldDimSourceLoan::fail()const noexcept{fail_state(state_);}
BnwUnitShouldDimPayloadLoan::BnwUnitShouldDimPayloadLoan(const BnwUnitShouldDimSourceLoan& s,
 std::function<void()> current,std::function<std::int32_t()> owner,
 std::function<std::int32_t()> id,std::int32_t moves)
 :source_(s),dispatch_current_(std::move(current)),owner_(std::move(owner)),id_(std::move(id)),stored_moves_(moves){}
struct BnwUnitShouldDimQueuedEvent::State {
 enum class Phase {queued,consuming,consumed,uncertain};
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwUnitShouldDimQueuedEvent> self;
 std::weak_ptr<const BnwUnitShouldDimQueue> queue;
 std::weak_ptr<const void> source,control;
 std::weak_ptr<const runtime::LiveSessionState> world;
 const void* vm=nullptr;
 std::uint64_t sequence=0,enqueue_operation=0;
 std::chrono::steady_clock::time_point appended_at;
 std::array<std::optional<std::uint32_t>,28> record;
 std::int32_t owner=-1,full_id=-1;
 bool dim=false,enqueue_returned=false;
 Phase phase=Phase::queued;
};
BnwUnitShouldDimQueuedEvent::BnwUnitShouldDimQueuedEvent(std::shared_ptr<State> s):state_(std::move(s)){}
BnwUnitShouldDimPayloadObservation BnwUnitShouldDimQueuedEvent::observation()const{
 need(state_->creator==std::this_thread::get_id(),"ShouldDim observation foreign creator");
 BnwUnitShouldDimPayloadObservation out;
 std::copy_n(state_->record.begin()+0x44/4,8,out.dwords.begin());return out;
}
BnwUnitShouldDimEnqueueProof::BnwUnitShouldDimEnqueueProof(std::weak_ptr<const BnwUnitShouldDimQueue> q,
 std::shared_ptr<const BnwUnitShouldDimQueuedEvent> e,std::uint64_t serial)
 :queue_(std::move(q)),event_(std::move(e)),enqueue_serial_(serial){}
struct BnwUnitShouldDimCache::State {
 struct Bits{std::uint8_t known=0,value=0;};
 struct Cell{
  std::array<Bits,28> value{};
  std::int32_t last_owner=-1,last_full_id=-1;
  std::uint64_t mutation=0;
  Cell(){
   // Native prepared value: -1 DWORDs at offsets0,4,10(hex), bit0
   // clear at byte18(hex). All unspecified bits retain unknown status.
   for(const auto offset:{0u,4u,16u})for(unsigned i=0;i<4;++i)value[offset+i]={255,255};
   value[24]={1,0};
  }
 };
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwUnitShouldDimCache> self;
 std::weak_ptr<const void> source,control;
 std::weak_ptr<const runtime::LiveSessionState> world;
 const void* vm=nullptr;
 std::map<std::uint32_t,Cell> cells;
 bool alive=true,failed=false;
 std::size_t pins=0;
 std::uint64_t mutations=0,insertions=0;
};
struct BnwUnitShouldDimCache::Pin{
 std::shared_ptr<const BnwUnitShouldDimCache> owner;
 std::shared_ptr<State> state;
 const std::size_t count;
 Pin(const BnwUnitShouldDimCache& c,std::size_t borrowed=0)
 :owner(c.state_->self.lock()),state(c.state_),count(1+borrowed){
  need(owner&&owner.get()==&c,"ShouldDim cache canonical owner expired");add_pin(state->pins,count);
 }
 ~Pin(){state->pins-=count;}
};
BnwUnitShouldDimCache::BnwUnitShouldDimCache(std::shared_ptr<State> s):state_(std::move(s)){}
BnwUnitShouldDimCache::~BnwUnitShouldDimCache(){state_->alive=false;state_->failed=true;}
std::shared_ptr<BnwUnitShouldDimCache> BnwUnitShouldDimCache::bind_fresh(const BnwUnitShouldDimSourceLoan& source){
 const auto a=source.state_;BnwUnitShouldDimSourceLoan::full_state(a);
 auto s=std::make_shared<State>();s->source=a->scope.source_owner;s->control=a->scope.control;
 s->world=a->scope.control->world;s->vm=a->scope.vm;
 auto out=std::shared_ptr<BnwUnitShouldDimCache>(new BnwUnitShouldDimCache(s));s->self=out;
 BnwUnitShouldDimSourceLoan::full_state(a);out->pure(a);return out;
}
void BnwUnitShouldDimCache::pure(const std::shared_ptr<BnwUnitShouldDimSourceLoan::State>& a)const{
 BnwUnitShouldDimSourceLoan::pure_state(a);const auto& s=*state_;
 need(s.creator==std::this_thread::get_id()&&s.alive&&!s.failed,"ShouldDim cache retired");
 const auto owner=s.self.lock();
 need(owner&&owner.get()==this&&s.pins<static_cast<std::size_t>(LONG_MAX)&&
  owner.use_count()>static_cast<long>(s.pins+1)&&owner_same(s.source,a->scope.source_owner)&&
  owner_same(s.control,a->scope.control)&&owner_same(s.world,a->scope.control->world)&&s.vm==a->scope.vm,
  "ShouldDim cache canonical source/world/outside owner lost");
}
void BnwUnitShouldDimCache::apply(const BnwUnitShouldDimQueuedEvent& event,
 const std::shared_ptr<BnwUnitShouldDimSourceLoan::State>& a){
 Pin pin(*this);const auto s=state_;
 try{
  BnwUnitShouldDimSourceLoan::full_state(a);pure(a);const auto& e=*event.state_;
  need(e.phase==BnwUnitShouldDimQueuedEvent::State::Phase::consuming,"ShouldDim cache outside receiver");
  const auto key=packed(e.owner,e.full_id);
  need(s->mutations<UINT64_MAX&&s->insertions<UINT64_MAX,"ShouldDim cache counters exhausted");
  const auto [it,inserted]=s->cells.try_emplace(key);if(inserted)++s->insertions;
  auto& c=it->second;auto& flags=c.value[25];
  flags.value=static_cast<std::uint8_t>((flags.value&~4u)|(e.dim?4u:0u));flags.known|=4u;
  c.last_owner=e.owner;c.last_full_id=e.full_id;c.mutation=++s->mutations;
  BnwUnitShouldDimSourceLoan::full_state(a);pure(a);
 }catch(...){s->failed=true;BnwUnitShouldDimSourceLoan::fail_state(a);throw;}
}
class BnwUnitShouldDimQueue::Subscription final{
 friend class BnwUnitShouldDimQueue;friend class BnwUnitShouldDimFanoutProof;
 enum class Kind{native,lua};const Kind kind;
 std::weak_ptr<const Subscription> self;
 std::weak_ptr<BnwUnitShouldDimCache> cache;
 std::function<void(const BnwUnitShouldDimQueuedEvent&,const BnwUnitShouldDimSourceLoan&)> lua;
 std::uint64_t registration=0;
 bool alive=true;
 mutable std::size_t pins=0;
 explicit Subscription(Kind k):kind(k){}
 public:~Subscription(){alive=false;}
};
struct BnwUnitShouldDimQueue::State{
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwUnitShouldDimQueue> self;
 std::weak_ptr<const void> source,control;
 std::weak_ptr<const runtime::LiveSessionState> world;
 const void* vm=nullptr;
 std::deque<std::shared_ptr<const BnwUnitShouldDimQueuedEvent>> events;
 std::vector<std::weak_ptr<Subscription>> targets;
 std::mutex queue_mutex;
 bool alive=true,failed=false,active=false;
 std::size_t pins=0;
 std::uint64_t next=0,registration=0;
};
struct BnwUnitShouldDimQueue::Pin{
 std::shared_ptr<const BnwUnitShouldDimQueue> owner;
 std::shared_ptr<State> state;
 explicit Pin(const BnwUnitShouldDimQueue& q):owner(q.state_->self.lock()),state(q.state_){
  need(owner&&owner.get()==&q,"ShouldDim queue canonical owner expired");
  if(state->active){state->failed=true;throw std::runtime_error("ShouldDim queue reentry");}
  add_pin(state->pins);state->active=true;
 }
 ~Pin(){state->active=false;--state->pins;}
};
BnwUnitShouldDimQueue::BnwUnitShouldDimQueue(std::shared_ptr<State> s):state_(std::move(s)){}
BnwUnitShouldDimQueue::~BnwUnitShouldDimQueue(){state_->alive=false;state_->failed=true;}
std::shared_ptr<BnwUnitShouldDimQueue> BnwUnitShouldDimQueue::bind_fresh(const BnwUnitShouldDimSourceLoan& source){
 const auto a=source.state_;BnwUnitShouldDimSourceLoan::full_state(a);
 auto s=std::make_shared<State>();s->source=a->scope.source_owner;s->control=a->scope.control;
 s->world=a->scope.control->world;s->vm=a->scope.vm;
 auto out=std::shared_ptr<BnwUnitShouldDimQueue>(new BnwUnitShouldDimQueue(s));s->self=out;
 BnwUnitShouldDimSourceLoan::full_state(a);out->pure(a);return out;
}
void BnwUnitShouldDimQueue::pure(const std::shared_ptr<BnwUnitShouldDimSourceLoan::State>& a)const{
 BnwUnitShouldDimSourceLoan::pure_state(a);const auto& s=*state_;
 need(s.creator==std::this_thread::get_id()&&s.alive&&!s.failed,"ShouldDim queue retired");
 const auto owner=s.self.lock();
 need(owner&&owner.get()==this&&s.pins<static_cast<std::size_t>(LONG_MAX)&&
  owner.use_count()>static_cast<long>(s.pins+1)&&owner_same(s.source,a->scope.source_owner)&&
  owner_same(s.control,a->scope.control)&&owner_same(s.world,a->scope.control->world)&&s.vm==a->scope.vm,
  "ShouldDim queue canonical source/world/outside owner lost");
}
void BnwUnitShouldDimQueue::preflight_from_owned_weak(const std::weak_ptr<BnwUnitShouldDimQueue>& weak,
 const BnwUnitShouldDimSourceLoan& source){
 const auto a=source.state_;BnwUnitShouldDimSourceLoan::pure_state(a);
 const auto queue=weak.lock();need(queue!=nullptr,"ShouldDim actual initialized queue absent");
 add_pin(queue->state_->pins);
 struct Borrow{std::shared_ptr<State>s;~Borrow(){--s->pins;}} borrowed{queue->state_};
 queue->pure(a);
}
std::shared_ptr<const BnwUnitShouldDimEnqueueProof> BnwUnitShouldDimQueue::enqueue_from_owned_weak(
 const std::weak_ptr<BnwUnitShouldDimQueue>& weak,const BnwUnitShouldDimPayloadLoan& payload){
 const auto a=payload.source_.state_;BnwUnitShouldDimSourceLoan::pure_state(a);
 const auto queue=weak.lock();need(queue!=nullptr,"ShouldDim actual queue outside owner expired");
 add_pin(queue->state_->pins);
 struct Borrow{std::shared_ptr<State>s;~Borrow(){--s->pins;}} borrowed{queue->state_};
 queue->pure(a);const auto proof=queue->enqueue(payload);queue->pure(a);
 return proof;
}
void BnwUnitShouldDimEnqueueProof::require_current_for_source(const BnwUnitShouldDimSourceLoan& source)const{
 const auto a=source.state_;BnwUnitShouldDimSourceLoan::pure_state(a);
 const auto queue=queue_.lock();need(queue!=nullptr,"ShouldDim enqueue receipt queue retired");
 add_pin(queue->state_->pins);
 struct Borrow{std::shared_ptr<BnwUnitShouldDimQueue::State>s;~Borrow(){--s->pins;}} borrowed{queue->state_};
 queue->pure(a);const auto& e=*event_->state_;
 need(same(e.self.lock(),event_)&&same(e.queue.lock(),queue)&&e.enqueue_returned&&
  e.phase==BnwUnitShouldDimQueuedEvent::State::Phase::queued&&e.enqueue_operation==enqueue_serial_&&
  enqueue_serial_==a->scope.serial&&owner_same(e.source,a->scope.source_owner)&&
  owner_same(e.control,a->scope.control)&&owner_same(e.world,a->scope.control->world)&&e.vm==a->scope.vm&&
  std::find(queue->state_->events.begin(),queue->state_->events.end(),event_)!=queue->state_->events.end(),
  "ShouldDim enqueue proof is not SAME active returned queue member");
 BnwUnitShouldDimSourceLoan::pure_state(a);
}
std::shared_ptr<BnwUnitShouldDimQueue::Subscription> BnwUnitShouldDimQueue::register_native(
 std::shared_ptr<BnwUnitShouldDimCache> cache,const BnwUnitShouldDimSourceLoan& source){
 const auto a=source.state_;Pin pin(*this);const auto s=state_;
 try{
  BnwUnitShouldDimSourceLoan::full_state(a);pure(a);need(cache!=nullptr,"ShouldDim cache owner absent");
  BnwUnitShouldDimCache::Pin cache_pin(*cache,1);cache_pin.owner->pure(a);
  need(s->registration<UINT64_MAX,"ShouldDim registration exhausted");
  auto t=std::shared_ptr<Subscription>(new Subscription(Subscription::Kind::native));t->self=t;t->cache=cache;
  t->registration=++s->registration;s->targets.emplace_back(t);
  BnwUnitShouldDimSourceLoan::full_state(a);pure(a);cache_pin.owner->pure(a);return t;
 }catch(...){s->failed=true;BnwUnitShouldDimSourceLoan::fail_state(a);throw;}
}
std::shared_ptr<BnwUnitShouldDimQueue::Subscription> BnwUnitShouldDimQueue::register_lua(
 std::function<void(const BnwUnitShouldDimQueuedEvent&,const BnwUnitShouldDimSourceLoan&)> invoke,
 const BnwUnitShouldDimSourceLoan& source){
 const auto a=source.state_;Pin pin(*this);const auto s=state_;
 try{
  BnwUnitShouldDimSourceLoan::full_state(a);pure(a);need(bool(invoke),"ShouldDim actual Lua target absent");
  need(s->registration<UINT64_MAX,"ShouldDim registration exhausted");
  auto t=std::shared_ptr<Subscription>(new Subscription(Subscription::Kind::lua));t->self=t;t->lua=std::move(invoke);
  BnwUnitShouldDimSourceLoan::full_state(a);pure(a);
  t->registration=++s->registration;s->targets.emplace_back(t);
  BnwUnitShouldDimSourceLoan::full_state(a);pure(a);return t;
 }catch(...){s->failed=true;BnwUnitShouldDimSourceLoan::fail_state(a);invoke={};throw;}
}
std::shared_ptr<const BnwUnitShouldDimEnqueueProof> BnwUnitShouldDimQueue::enqueue(const BnwUnitShouldDimPayloadLoan& payload){
 const auto a=payload.source_.state_;Pin pin(*this);const auto s=state_;
 // PayloadLoan is a strict synchronous Core borrow. The authentic Core must
 // retain its lender storage and include every internal Core/Plan pin in guard.
 const auto* payload_ptr=&payload;
 std::optional<std::function<void()>> guard;
 std::optional<std::function<std::int32_t()>> getter;
 const auto full=[&]{BnwUnitShouldDimSourceLoan::full_state(a);pure(a);};
 const auto check=[&]{full();need(bool(payload_ptr->dispatch_current_),"ShouldDim Core issuer unjoined");
  guard.emplace(payload_ptr->dispatch_current_);full();(*guard)();full();guard.reset();full();};
 const auto read=[&](const auto& original){check();getter.emplace(original);check();
  need(bool(*getter),"ShouldDim Unit getter absent");const auto value=(*getter)();
  check();getter.reset();check();return value;};
 try{
  check();const auto first_owner=read(payload_ptr->owner_);const auto first_id=read(payload_ptr->id_);
  const auto id=read(payload_ptr->id_);const auto owner=read(payload_ptr->owner_);
  need(first_owner==owner&&first_id==id,"ShouldDim authentic Unit operands changed");(void)packed(owner,id);
  need(s->next<UINT64_MAX,"ShouldDim queue sequence exhausted");
  auto e=std::make_shared<BnwUnitShouldDimQueuedEvent::State>();
  e->source=a->scope.source_owner;e->control=a->scope.control;e->world=a->scope.control->world;e->vm=a->scope.vm;
  e->owner=owner;e->full_id=id;e->dim=payload_ptr->stored_moves_<=0;
  e->enqueue_operation=a->scope.serial;e->sequence=s->next+1;
  e->record[1]=0x53;e->record[(0x44+4)/4]=static_cast<std::uint32_t>(id);
  e->record[(0x44+8)/4]=static_cast<std::uint32_t>(owner);e->record[(0x44+0x18)/4]=e->dim?1u:0u;
  auto event=std::shared_ptr<const BnwUnitShouldDimQueuedEvent>(new BnwUnitShouldDimQueuedEvent(e));
  e->self=event;e->queue=s->self;
  auto proof=std::shared_ptr<const BnwUnitShouldDimEnqueueProof>(new BnwUnitShouldDimEnqueueProof(s->self,event,a->scope.serial));
  check();{std::lock_guard lock(s->queue_mutex);e->appended_at=std::chrono::steady_clock::now();
   s->events.push_back(event);s->next=e->sequence;}
  check();e->enqueue_returned=true;return proof;
 }catch(...){s->failed=true;BnwUnitShouldDimSourceLoan::fail_state(a);getter.reset();guard.reset();throw;}
}
BnwUnitShouldDimReceiverLoan::BnwUnitShouldDimReceiverLoan(const BnwUnitShouldDimSourceLoan& s,
 const runtime::BnwOwnedGameplayEventPumpLoan& p):source_(s),pump_(p){require_current();}
void BnwUnitShouldDimReceiverLoan::require_current()const{
 const auto a=source_.state_;const auto* pump=&pump_;BnwUnitShouldDimSourceLoan::pure_state(a);
 pump->require_for_source(a->attempt);BnwUnitShouldDimSourceLoan::pure_state(a);
}
struct BnwUnitShouldDimFanoutProof::State{
 struct NativeMutation{
  std::weak_ptr<const BnwUnitShouldDimCache> cache;
  std::uint32_t key=0;
  std::uint64_t mutation=0;
 };
 std::weak_ptr<const BnwUnitShouldDimFanoutProof> self;
 std::weak_ptr<const BnwUnitShouldDimQueue> queue;
 std::vector<std::weak_ptr<const BnwUnitShouldDimQueue::Subscription>> targets;
 std::vector<NativeMutation> native;
 bool completed=false;
};
BnwUnitShouldDimFanoutProof::BnwUnitShouldDimFanoutProof(std::shared_ptr<State> s,
 std::shared_ptr<const BnwUnitShouldDimQueuedEvent> e,std::uint64_t b,std::uint64_t op,
 std::vector<std::uint64_t> order)
 :state_(std::move(s)),event_(std::move(e)),pump_boundary_(b),receiver_operation_(op),actual_registrations_(std::move(order)){}
void BnwUnitShouldDimFanoutProof::require_for_event(const BnwUnitShouldDimQueuedEvent& event,
 const BnwUnitShouldDimReceiverLoan& loan)const{
 const auto a=loan.source_.state_;loan.require_current();BnwUnitShouldDimSourceLoan::pure_state(a);
 const auto self=state_->self.lock();const auto q=state_->queue.lock();
 need(self&&self.get()==this&&state_->completed&&event_.get()==&event&&same(event.state_->self.lock(),event_)&&
  event.state_->phase==BnwUnitShouldDimQueuedEvent::State::Phase::consumed&&q&&
  receiver_operation_==a->scope.serial&&pump_boundary_==loan.pump_.boundary(),"ShouldDim fanout proof substituted/expired");
 add_pin(q->state_->pins);struct QueueBorrow{std::shared_ptr<BnwUnitShouldDimQueue::State>s;~QueueBorrow(){--s->pins;}} qb{q->state_};
 q->pure(a);need(state_->targets.size()==actual_registrations_.size(),"ShouldDim fanout receipt incomplete");
 for(std::size_t i=0;i<state_->targets.size();++i){const auto t=state_->targets[i].lock();
  need(t&&t->alive&&same(t->self.lock(),t)&&t->registration==actual_registrations_[i],"ShouldDim dispatched target retired");}
 for(const auto& receipt:state_->native){const auto c=receipt.cache.lock();need(c!=nullptr,"ShouldDim native receipt owner retired");
  add_pin(c->state_->pins);struct CacheBorrow{std::shared_ptr<BnwUnitShouldDimCache::State>s;~CacheBorrow(){--s->pins;}} cb{c->state_};
  c->pure(a);const auto it=c->state_->cells.find(receipt.key);
  need(it!=c->state_->cells.end()&&it->second.mutation==receipt.mutation&&it->second.last_owner==event.state_->owner&&
   it->second.last_full_id==event.state_->full_id&&(it->second.value[25].known&4u)&&
   ((it->second.value[25].value&4u)!=0)==event.state_->dim,"ShouldDim native mutation receipt substituted");}
 loan.require_current();q->pure(a);BnwUnitShouldDimSourceLoan::pure_state(a);
}
std::shared_ptr<const BnwUnitShouldDimFanoutProof> BnwUnitShouldDimQueue::consume_one(const BnwUnitShouldDimReceiverLoan& loan){
 const auto a=loan.source_.state_;const auto* source=&loan.source_;const auto* pump=&loan.pump_;
 Pin pin(*this);const auto s=state_;
 std::shared_ptr<const BnwUnitShouldDimQueuedEvent> event;
 std::optional<std::function<void(const BnwUnitShouldDimQueuedEvent&,const BnwUnitShouldDimSourceLoan&)>> target;
 const auto intrinsic=[&]{BnwUnitShouldDimSourceLoan::pure_state(a);pump->require_for_source(a->attempt);pure(a);};
 const auto full=[&]{intrinsic();BnwUnitShouldDimSourceLoan::full_state(a);intrinsic();};
 try{
  full();if(s->events.empty())return {};event=s->events.front();const auto& e=*event->state_;
  need(e.creator==std::this_thread::get_id()&&e.enqueue_returned&&e.phase==BnwUnitShouldDimQueuedEvent::State::Phase::queued&&
   owner_same(e.source,a->scope.source_owner)&&owner_same(e.control,a->scope.control)&&e.vm==a->scope.vm&&
   owner_same(e.world,a->scope.control->world)&&a->scope.serial!=e.enqueue_operation&&
   same(e.self.lock(),event)&&same(e.queue.lock(),s->self.lock()),"ShouldDim wrong later SAME-source queue member");
  bool native=false,lua=false;
  for(const auto& weak:s->targets){const auto t=weak.lock();need(t&&t->alive,"ShouldDim registered target retired");
   native|=t->kind==Subscription::Kind::native;lua|=t->kind==Subscription::Kind::lua;}
  need(native&&lua,"ShouldDim actual native/Lua subscription gates unjoined");
  auto receipts=std::make_shared<BnwUnitShouldDimFanoutProof::State>();receipts->queue=s->self;
  std::vector<std::uint64_t> order;
  event->state_->phase=BnwUnitShouldDimQueuedEvent::State::Phase::consuming;
  for(const auto& weak:s->targets){const auto t=weak.lock();need(t&&t->alive,"ShouldDim registered target disappeared");add_pin(t->pins);
   struct TargetBorrow{std::shared_ptr<Subscription> t;~TargetBorrow(){--t->pins;}} tb{t};
   const auto check=[&]{intrinsic();const auto actual=t->self.lock();
    need(same(actual,t)&&t->alive&&t->pins<static_cast<std::size_t>(LONG_MAX-2)&&
     t.use_count()>static_cast<long>(t->pins+2)&&s->events.front()==event&&
     event->state_->phase==BnwUnitShouldDimQueuedEvent::State::Phase::consuming,"ShouldDim target outside owner/member lost");};
   full();check();
   if(t->kind==Subscription::Kind::native){const auto cache=t->cache.lock();need(cache!=nullptr,"ShouldDim native cache retired");
    add_pin(cache->state_->pins);struct CacheBorrow{std::shared_ptr<BnwUnitShouldDimCache::State>s;~CacheBorrow(){--s->pins;}} cb{cache->state_};
    cache->apply(*event,a);full();check();const auto key=packed(e.owner,e.full_id);const auto& cell=cache->state_->cells.at(key);
    receipts->native.push_back({cache,key,cell.mutation});cache->pure(a);
   }else{target.emplace(t->lua);full();check();need(bool(*target),"ShouldDim Lua target unbound");
    (*target)(*event,*source);full();check();target.reset();full();check();}
   order.push_back(t->registration);receipts->targets.emplace_back(t);
  }
  full();auto proof=std::shared_ptr<const BnwUnitShouldDimFanoutProof>(new BnwUnitShouldDimFanoutProof(
   receipts,event,pump->boundary(),a->scope.serial,std::move(order)));receipts->self=proof;
  {std::lock_guard lock(s->queue_mutex);need(s->events.front()==event,"ShouldDim ordered member replaced");
   event->state_->phase=BnwUnitShouldDimQueuedEvent::State::Phase::consumed;s->events.pop_front();}
  intrinsic();receipts->completed=true;return proof;
 }catch(...){s->failed=true;BnwUnitShouldDimSourceLoan::fail_state(a);
  if(event)event->state_->phase=BnwUnitShouldDimQueuedEvent::State::Phase::uncertain;target.reset();throw;}
}
}
