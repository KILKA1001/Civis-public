// Included at file scope after existing owned setup transport implementation.
namespace civ5::graphics {
struct BnwUnitSetupGraphicalQueue::CreatedOccurrenceLoan::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 std::unique_ptr<Pin>pin;
 const runtime::BnwWorldViewSourceOperationLoan*const source;
 const std::uint64_t source_serial,append_serial,drain_serial;
 const std::shared_ptr<const Batch>batch;
 const runtime::LiveSessionState*const world;
 const std::uint64_t generation;
 const CreatedOccurrenceLoan*loan=nullptr;
 std::shared_ptr<PendingReadLifetime>reference_life;
 mutable bool checking=false,returned=false;
 Impl(std::unique_ptr<Pin>p,const runtime::BnwWorldViewSourceOperationLoan&s,
  std::shared_ptr<const Batch>b,std::uint64_t drain):pin(std::move(p)),source(&s),
  source_serial(s.operation_serial()),append_serial(pin->owner->state_->serial),drain_serial(drain),batch(std::move(b)),
  world([&]{const auto scene=pin->owner->state_->scene.lock();setup_need(bool(scene),"Created Scene lost");return scene->scene_state_;}()),
  generation([&]{const auto scene=pin->owner->state_->scene.lock();setup_need(bool(scene),"Created Scene generation lost");return scene->scene_state_generation_;}()){}
 void poison()const noexcept{pin->owner->state_->failed=true;}
 void pure()const {
  setup_need(creator==std::this_thread::get_id(),"Created occurrence foreign creator");
  pin->owner->pure();const auto&s=*pin->owner->state_;
  const auto scene=s.scene.lock();
  setup_need(scene&&scene.get()==s.scene_get&&setup_control(scene,s.scene)&&
   scene->gui_thread_==creator&&setup_control(scene,scene->gui_self_)&&
   scene.use_count()>static_cast<long>(scene->gui_publication_pins_.load()+1)&&
   scene->setup_graphical_storage_.get()==pin->owner.get()&&setup_control(scene->setup_graphical_storage_,s.self)&&
   scene->scene_state_==world&&scene->scene_state_generation_==generation&&
   &scene->session_.state()==world&&scene->session_.state_generation()==generation&&
   s.busy&&s.created_loan==loan&&loan&&s.created_drain_serial==drain_serial&&
   s.serial==append_serial&&!s.pending.empty()&&s.pending.front().get()==batch.get()&&
   setup_control(s.pending.front(),batch)&&batch&&batch->serial<=append_serial&&batch->events.size()==1&&
   batch->events.front().kind==BnwUnitSetupGraphicalEventKind::created,
   "Created occurrence SAME canonical Scene/world/FIFO/member changed");
 }
};
BnwUnitSetupGraphicalQueue::CreatedOccurrenceLoan::CreatedOccurrenceLoan(std::unique_ptr<Impl>s):impl_(std::move(s)){
 impl_->reference_life=std::make_shared<PendingReadLifetime>(impl_->pin->owner->state_->self,this,impl_->drain_serial,impl_->append_serial);
}
BnwUnitSetupGraphicalQueue::CreatedOccurrenceLoan::~CreatedOccurrenceLoan(){
 if(impl_->creator!=std::this_thread::get_id())std::terminate();
 if(impl_->reference_life)impl_->reference_life->alive=false;
 if(!impl_->returned||impl_->pin->owner->state_->created_loan!=this)impl_->poison();
 if(impl_->pin->owner->state_->created_loan==this)impl_->pin->owner->state_->created_loan=nullptr;
}
void BnwUnitSetupGraphicalQueue::require_render_scene_source(
 const runtime::BnwWorldViewSourceOperationLoan&source,const std::weak_ptr<ui::BnwClientSceneFrame>&scene){
 source.require_origin_without_callbacks();source.require_render_operation_owned();
 source.require_grid_scene_origin_without_callbacks(scene);source.require_origin_without_callbacks();
}
void BnwUnitSetupGraphicalQueue::CreatedOccurrenceLoan::require_origin_without_callbacks()const{
 try{impl_->pure();BnwUnitSetupGraphicalQueue::require_render_scene_source(*impl_->source,impl_->pin->owner->state_->scene);impl_->pure();}
 catch(...){impl_->poison();throw;}
}
void BnwUnitSetupGraphicalQueue::CreatedOccurrenceLoan::require_current()const{
 require_origin_without_callbacks();if(impl_->checking){impl_->poison();throw std::runtime_error("Created occurrence callback reentry");}
 impl_->checking=true;struct End{Impl&s;~End(){s.checking=false;}}end{*impl_};
 try{impl_->source->require_operation(impl_->source_serial);require_origin_without_callbacks();}
 catch(...){impl_->poison();throw;}
}
void BnwUnitSetupGraphicalQueue::CreatedOccurrenceLoan::require_same_source(const runtime::BnwWorldViewSourceOperationLoan&s)const{
 require_origin_without_callbacks();setup_need(&s==impl_->source,"Created occurrence substituted Source");
 require_current();
}
std::pair<std::int32_t,std::int32_t>BnwUnitSetupGraphicalQueue::CreatedOccurrenceLoan::actual_owner_full_id()const{
 require_origin_without_callbacks();const auto&e=impl_->batch->events.front();
 setup_need(e.owner>=0&&e.owner<64&&e.full_id>=0,"Original created full-ID payload malformed");return {e.owner,e.full_id};
}
std::pair<std::int32_t,bool>BnwUnitSetupGraphicalQueue::CreatedOccurrenceLoan::actual_original_fog_not_invisible()const{
 require_origin_without_callbacks();const auto&e=impl_->batch->events.front();
 setup_need(e.original_fog.has_value()&&e.original_not_invisible.has_value(),
  "Mandatory genuine original created fog10/notInvisible13 supplier absent");
 setup_need(*e.original_fog>=0&&*e.original_fog<=2,"Original created native fog byte invalid");
 return {*e.original_fog,*e.original_not_invisible};
}
void BnwUnitSetupGraphicalQueue::CreatedOccurrenceLoan::record_original_constructor_returned()const{
 require_current();if(impl_->returned){impl_->poison();throw std::runtime_error("Original created constructor return replay");}
 impl_->returned=true;require_origin_without_callbacks();
}
bool BnwUnitSetupGraphicalQueue::with_first_created_owned(const std::weak_ptr<BnwUnitSetupGraphicalQueue>&weak,
 const runtime::BnwWorldViewSourceOperationLoan&source,const std::function<void(const CreatedOccurrenceLoan&)>&incoming){
 Pin pin(weak);const auto state=pin.owner->state_;
 // Pure immutable creator/factory checks precede every callbackful Source use.
 require_render_scene_source(source,state->scene);pin.owner->pure();
 if(state->busy||state->created_loan){state->failed=true;throw std::runtime_error("Created FIFO consumer reentry");}
 state->busy=true;struct End{std::shared_ptr<State>s;~End(){s->busy=false;}}end{state};
 std::optional<std::function<void(const CreatedOccurrenceLoan&)>>target;
 try{
  source.require_current();pin.owner->pure();require_render_scene_source(source,state->scene);
  if(state->pending.empty()){source.require_current();pin.owner->pure();return false;}
  const auto batch=state->pending.front();
  setup_need(batch&&batch->events.size()==1&&batch->events.front().kind==BnwUnitSetupGraphicalEventKind::created,
   "Mandatory original setup FIFO predecessor is not supported created event");
  setup_need(state->created_drain_serial!=UINT64_MAX,"Created FIFO drain serial exhausted");
  ++state->created_drain_serial;
  auto data=std::make_unique<CreatedOccurrenceLoan::Impl>(std::make_unique<Pin>(weak),source,batch,state->created_drain_serial);
  CreatedOccurrenceLoan loan(std::move(data));loan.impl_->loan=&loan;state->created_loan=&loan;
  loan.require_current();target.emplace(incoming);loan.require_current();setup_need(bool(*target),"Original created consumer absent");
  (*target)(loan);loan.require_origin_without_callbacks(); // FIRST POST before any next Source callback
  loan.require_current();target.reset();loan.require_origin_without_callbacks();loan.require_current();
  setup_need(loan.impl_->returned,"Created FIFO lacks actual original constructor/call return");
  // Constructor effects may already exist. No rollback/requeue after failure.
  state->pending.pop_front();
  source.require_current();pin.owner->pure();require_render_scene_source(source,state->scene);
  return true;
 }catch(...){state->failed=true;target.reset();throw;}
}
}
