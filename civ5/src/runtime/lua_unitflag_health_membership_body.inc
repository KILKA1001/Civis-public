// File-scope in SAME LuaScriptHost TU after Impl. No Source.current/getter loops.
namespace civ5::runtime {
struct BnwUnitFlagHealthInstanceLoan::OriginalReturnedInstance {
 std::weak_ptr<LuaScriptHost::Impl>vm;
 std::weak_ptr<ui::BnwEngineUiEndTurnQueue::VmLifetime>lifetime;
 std::shared_ptr<BnwUnitFlagHealthFifoLoan::State>fifo;
 std::shared_ptr<const BnwUnitFlagLivingHealthRead>health;
 std::size_t receipt=LuaScriptHost::Impl::graph_no_owner;
 std::uint64_t epoch=0,writes=0,uncertainty=0;
};
struct BnwUnitFlagHealthInstanceLoan::State {
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool>alive{true};mutable std::atomic<bool>failed{false};
 const LuaScriptHost*vm=nullptr;
 std::weak_ptr<LuaScriptHost::Impl>impl;
 std::weak_ptr<ui::BnwEngineUiEndTurnQueue::VmLifetime>lifetime;
 BnwSelectedUnitFlagContextOwner::HealthOrigin context;
 const BnwStockLuaAttemptLease lease;
 const BnwStockLuaAttemptLease::ScopeIdentity stock;
 std::shared_ptr<BnwSelectedWorldViewContextOwner::State>metadata;
 std::weak_ptr<const ui::SelectedBnwPreviewHost>host;
 std::weak_ptr<const LiveSessionState>world;
 std::shared_ptr<BnwUnitFlagHealthFifoLoan::State>fifo;
 std::shared_ptr<const BnwUnitFlagLivingHealthRead>health;
 std::size_t receipt=LuaScriptHost::Impl::graph_no_owner,registration=LuaScriptHost::Impl::graph_no_owner;
 std::uint64_t generation=0,epoch=0,writes=0,uncertainty=0,metadata_serial=0;
 const BnwWorldViewSourceOperationLoan*expected_source=nullptr;
 const void*vm_state=nullptr,*flag_identity=nullptr,*controls_identity=nullptr,*cargo_identity=nullptr;
 int flag_ref=LUA_NOREF,controls_ref=LUA_NOREF,cargo_ref=LUA_NOREF;
 std::array<std::uint64_t,6>ids{};
 std::optional<std::uint64_t>cargo;
 std::uint64_t instance_root=0,cargo_root=0;
 std::size_t graph_instance=LuaScriptHost::Impl::graph_no_owner,cargo_instance=LuaScriptHost::Impl::graph_no_owner;
 std::int32_t owner=-1,id=-1;
 bool ready=false,bound=false;
 State(const BnwStockLuaAttemptLease&a):lease(a),stock(a.pin_active_scope_without_callbacks()){}
 void first()const{
  if(creator!=std::this_thread::get_id()){failed=true;throw ScriptError("UnitFlag membership foreign creator before mutable access");}
  if(!alive.load()||failed.load())throw ScriptError("UnitFlag membership facade expired or quarantined");
 }
 void require_stock_scope_identity_without_callbacks()const{
  first();lease.require_scope_identity_without_callbacks(stock);first();
 }
 void base()const{
  first();lease.require_scope_identity_without_callbacks(stock);first();
  const auto life=lifetime.lock();
  if(!life||life->creator!=creator||!life->alive.load()||life->failed.load()||life->host!=vm||life->vm!=vm_state)
   throw ScriptError("UnitFlag membership SAME VM allocation retired");
  const auto actual=impl.lock();
  if(!actual||actual->ui_thread!=creator||actual->state!=vm_state||actual->end_turn_lifetime.get()!=life.get()||
   actual->end_turn_lifetime.owner_before(lifetime)||lifetime.owner_before(actual->end_turn_lifetime)||
   actual->failure_epoch!=epoch||actual->failed.writes_!=writes||actual->failed.exhausted_||
   actual->execution_uncertainty!=uncertainty||receipt>=actual->worldview_created_instances.size())
   throw ScriptError("UnitFlag membership VM/failure/registry identity changed");
  // Native pure() checks the active saved Source pointer and stamps WITHOUT
  // Source.require_current. It is legal only after metadata authenticates it.
  if(actual->worldview_native_host){
   if(!bound||actual->worldview_operation!=expected_source||actual->worldview_host_owner!=vm)
    throw ScriptError("UnitFlag membership has another actual native scope");
   actual->worldview_native_host->pure();
   if(actual->calling&&!actual->worldview_damage_dispatching)
    throw ScriptError("UnitFlag membership is outside actual original damage callback");
   if(actual->worldview_unit_reference_scope)actual->worldview_unit_reference_scope->pure();
   else if(actual->worldview_control_entry){
    if(!actual->calling||!actual->worldview_damage_dispatching)
     throw ScriptError("UnitFlag health control sentinel lacks original damage callback");
    // The existing protected native ControlScope owns this exact provisional
    // sentinel. No write/epoch relaxation; normal POST restores the prior pair.
    const bool normal=!actual->failed&&actual->ready==ready;
    const bool provisional=actual->failed&&!actual->ready;
    if(!normal&&!provisional)throw ScriptError("UnitFlag native control sentinel changed");
   }else if(actual->failed||actual->ready!=ready)
    throw ScriptError("UnitFlag damage callback failure/ready pair changed");
  }else if(actual->calling||actual->failed||actual->ready!=ready||actual->worldview_damage_dispatching||
   actual->worldview_control_entry||actual->worldview_unit_reference_scope)
   throw ScriptError("UnitFlag membership lacks idle actual VM or genuine native scope");
  BnwSelectedUnitFlagContextOwner::require_health_origin_without_callbacks(context,*vm);first();
  if(actual->worldview_sources.owner_before(context.owner)||context.owner.owner_before(actual->worldview_sources)||
   registration>=actual->worldview_created_registrations.size())
   throw ScriptError("UnitFlag membership actual metadata/registration changed");
  const auto&r=actual->worldview_created_instances[receipt];const auto&e=actual->worldview_created_registrations[registration];
  const auto graph=context.graph.lock();
  if(!graph||e.graph.lock().get()!=graph.get()||e.graph.owner_before(context.graph)||context.graph.owner_before(e.graph)||
   e.retired||r.retired||!r.original_returned||r.registration!=registration||r.owner!=owner||r.full_id!=id||
   r.instance!=flag_ref||r.controls!=controls_ref||r.cargo!=cargo_ref||r.instance_identity!=flag_identity||
   r.controls_identity!=controls_identity||r.cargo_identity!=cargo_identity||r.health_ids!=ids||r.cargo_pull_down!=cargo||
   r.instance_root!=instance_root||r.cargo_instance_root!=cargo_root||r.graph_instance!=graph_instance||
   r.cargo_graph_instance!=cargo_instance||e.child>=actual->child_contexts.size()||
   e.compiled>=actual->graph_compiled_sources.size()||e.epoch!=epoch||e.uncertainty!=uncertainty||
   e.source.get()!=context.source.lock().get()||e.source.owner_before(context.source)||context.source.owner_before(e.source))
   throw ScriptError("UnitFlag membership real original full-ID/control custody changed");
  const auto&child=actual->child_contexts[e.child];
  if(child.retired||!child.loaded||child.loading||child.name!="UnitFlagManager"||child.environment!=e.environment||
   child.execution_source!=e.compiled||actual->graph_compiled_sources[e.compiled].root!=e.root||
   actual->graph_compiled_sources[e.compiled].retained==LUA_NOREF||graph_instance>=actual->graph_instances.size()||
   actual->graph_instances[graph_instance].root!=instance_root||actual->graph_instances[graph_instance].slot!=e.graph_slot||
   actual->graph_instances[graph_instance].table<=0)
   throw ScriptError("UnitFlag membership original child/Proto/built instance retired");
  if(cargo&&(cargo_instance>=actual->graph_instances.size()||
   actual->graph_instances[cargo_instance].root!=cargo_root||actual->graph_instances[cargo_instance].slot!=e.graph_slot||
   actual->graph_instances[cargo_instance].table<=0))
   throw ScriptError("UnitFlag membership genuine Cargo instance retired");
  BnwUnitFlagHealthFifoLoan::require_origin_from_storage_without_callbacks(fifo);first();
  const auto same_health=BnwUnitFlagHealthFifoLoan::health_source_from_storage_without_callbacks(fifo);
  if(!health||same_health.get()!=health.get()||same_health.owner_before(health)||health.owner_before(same_health)||
   BnwUnitFlagHealthFifoLoan::owner_full_id_from_storage_without_callbacks(fifo)!=std::pair{owner,id})
   throw ScriptError("UnitFlag membership actual FIFO/native HP source substituted");
  health->require_same_world_without_callbacks(stock.world,generation);first();
  if(health->owner_full_id_without_callbacks()!=std::pair{owner,id})
   throw ScriptError("UnitFlag membership native full-ID alias changed");
  lease.require_scope_identity_without_callbacks(stock);first();
 }
 void slot()const{
  first();const auto owner=context.owner.lock();
  if(!owner)throw ScriptError("UnitFlag membership canonical metadata owner expired");
  BnwSelectedWorldViewContextOwner::require_health_source_storage_without_callbacks(
   metadata,*owner,world,generation,bound?expected_source:nullptr,bound?metadata_serial:0);first();
 }
};
BnwUnitFlagHealthInstanceLoan::BnwUnitFlagHealthInstanceLoan(const BnwStockLuaAttemptLease&lease,
 const LuaScriptHost&vm,const BnwSelectedUnitFlagContextOwner&context,const OriginalReturnedInstance&w)
 :state_(std::make_shared<State>(lease)){
 const auto s=state_;
 try{
  s->first();s->vm=&vm;s->impl=w.vm;s->lifetime=w.lifetime;s->fifo=w.fifo;s->health=w.health;
  s->receipt=w.receipt;s->epoch=w.epoch;s->writes=w.writes;s->uncertainty=w.uncertainty;
  s->context=context.retain_health_origin_without_callbacks(vm);
  const auto p=w.vm.lock();const auto life=w.lifetime.lock();
  if(!p||p.get()!=vm.impl_.get()||!life||life.get()!=p->end_turn_lifetime.get()||life->host!=&vm||
   life->vm!=p->state||p->ui_thread!=s->creator||p->calling||p->failed||p->worldview_native_host||
   w.receipt>=p->worldview_created_instances.size()||!s->stock.control||!s->stock.world)
   throw ScriptError("UnitFlag membership lacks genuine pre-mint VM/FIFO/original-return witness");
  s->vm_state=p->state;s->ready=p->ready;s->host=s->stock.control->selected_host;s->world=s->stock.control->world;
  const auto host=s->host.lock();
  if(!host)throw ScriptError("UnitFlag membership selected Host facade expired");
  const auto world=host->session().read_state_lease();
  if(!world||world.get()!=s->stock.world||world.owner_before(s->world)||s->world.owner_before(world))
   throw ScriptError("UnitFlag membership selected Host has another actual world");
  s->generation=host->session().state_generation();
  const auto metadata=s->context.owner.lock();
  if(!metadata||!s->generation)throw ScriptError("UnitFlag membership canonical metadata/gen absent");
  s->metadata=BnwSelectedWorldViewContextOwner::retain_health_source_storage_without_callbacks(*metadata,s->world,s->generation);
  const auto&r=p->worldview_created_instances[w.receipt];
  s->registration=r.registration;s->owner=r.owner;s->id=r.full_id;s->flag_ref=r.instance;s->controls_ref=r.controls;
  s->cargo_ref=r.cargo;s->flag_identity=r.instance_identity;s->controls_identity=r.controls_identity;s->cargo_identity=r.cargo_identity;
  s->ids=r.health_ids;s->cargo=r.cargo_pull_down;s->instance_root=r.instance_root;s->cargo_root=r.cargo_instance_root;
  s->graph_instance=r.graph_instance;s->cargo_instance=r.cargo_graph_instance;
  s->slot();s->base();
 }catch(...){s->failed=true;s->alive=false;throw;}
}
BnwUnitFlagHealthInstanceLoan::~BnwUnitFlagHealthInstanceLoan(){
 if(creator_!=std::this_thread::get_id())std::terminate();state_->alive=false;
}
std::shared_ptr<BnwUnitFlagHealthInstanceLoan::State>BnwUnitFlagHealthInstanceLoan::retain_storage_without_callbacks()const{
 require_origin_without_callbacks();return state_;
}
void BnwUnitFlagHealthInstanceLoan::require_origin_from_storage_without_callbacks(const std::shared_ptr<State>&s){
 if(!s)throw ScriptError("UnitFlag membership saved storage absent");s->first();
 try{s->slot();s->base();s->slot();}catch(...){s->failed=true;throw;}
}
void BnwUnitFlagHealthInstanceLoan::require_current_from_storage_owned(const std::shared_ptr<State>&s){
 require_origin_from_storage_without_callbacks(s);
 if(!s->bound||!s->metadata_serial||!s->expected_source){s->failed=true;throw ScriptError("UnitFlag membership Source not genuinely lent");}
}
void BnwUnitFlagHealthInstanceLoan::bind_source_scope_from_storage_without_callbacks(const std::shared_ptr<State>&s,
 const BnwWorldViewSourceOperationLoan&expected,std::uint64_t metadata_serial,std::uint64_t stock_serial){
 if(!s)throw ScriptError("UnitFlag membership binder storage absent");s->first();
 try{
  if(s->bound||!metadata_serial||!stock_serial||stock_serial!=s->stock.serial)
   throw ScriptError("UnitFlag membership repeated/wrong Source/Stock serial bind");
  const auto metadata=s->context.owner.lock();if(!metadata)throw ScriptError("UnitFlag membership metadata owner expired before bind");
  BnwSelectedWorldViewContextOwner::require_health_source_storage_without_callbacks(
   s->metadata,*metadata,s->world,s->generation,&expected,metadata_serial);s->base();
  s->expected_source=&expected;s->metadata_serial=metadata_serial;s->bound=true;
  require_current_from_storage_owned(s);
 }catch(...){s->failed=true;throw;}
}
void BnwUnitFlagHealthInstanceLoan::require_source_origin_from_storage_without_callbacks(const std::shared_ptr<State>&s,
 const LuaScriptHost&vm,const BnwSelectedWorldViewContextOwner&source,
 const std::weak_ptr<const ui::SelectedBnwPreviewHost>&host,const std::weak_ptr<const LiveSessionState>&world,std::uint64_t generation){
 require_origin_from_storage_without_callbacks(s);
 if(s->vm!=&vm||s->context.owner.lock().get()!=&source||s->host.owner_before(host)||host.owner_before(s->host)||
  s->world.owner_before(world)||world.owner_before(s->world)||generation!=s->generation){
  s->failed=true;throw ScriptError("UnitFlag membership different Source/VM/Host/world/generation");}
}
void BnwUnitFlagHealthInstanceLoan::require_control_from_storage_without_callbacks(const std::shared_ptr<State>&s,
 const ui::BnwWorldViewControlTree::Control&control,BnwUnitFlagHealthCommand command){
 require_current_from_storage_owned(s);
 try{
  bool member=false;std::uint64_t root=s->instance_root;
  switch(command){
   case BnwUnitFlagHealthCommand::hide:member=std::find(s->ids.begin(),s->ids.end(),control.id_)!=s->ids.end();break;
   case BnwUnitFlagHealthCommand::fg_color:case BnwUnitFlagHealthCommand::percent:member=control.id_==s->ids[1];break;
   case BnwUnitFlagHealthCommand::offset_x:member=s->cargo&&control.id_==*s->cargo;root=s->cargo_root;break;
   default:break;
  }
  if(!member||!root)throw ScriptError("UnitFlag health command outside actual original instance membership");
  const auto metadata=s->context.owner.lock();const auto tree=s->context.tree.lock();const auto graph=s->context.graph.lock();
  if(!metadata||!tree||!graph)throw ScriptError("UnitFlag health canonical control owners expired");
  const auto actual=BnwWorldViewScriptUiAdapter::unitflag_health_control_owned(graph,control.id_,root,*metadata);
  control.require_same_unitflag_health_control_without_callbacks(actual);
  tree->require_unitflag_health_control_without_callbacks(control,control.id_,root,*metadata);
  require_current_from_storage_owned(s);
 }catch(...){s->failed=true;throw;}
}
void BnwUnitFlagHealthInstanceLoan::require_origin_without_callbacks()const{require_origin_from_storage_without_callbacks(state_);}
void BnwUnitFlagHealthInstanceLoan::require_current()const{require_current_from_storage_owned(state_);}
void BnwUnitFlagHealthInstanceLoan::require_control_without_callbacks(const ui::BnwWorldViewControlTree::Control&c,BnwUnitFlagHealthCommand k)const{
 require_control_from_storage_without_callbacks(state_,c,k);
}
// The actual VM, under Queue-issued FIFO and fresh genuine Stock, is the only
// supplier. The protected read allocates no substitute table/control/instance.
void LuaScriptHost::with_unitflag_health_instance_owned(const BnwUnitFlagHealthFifoLoan&fifo,
 const BnwStockLuaAttemptLease&lease,const BnwSelectedUnitFlagContextOwner&context,
 const std::function<void(const BnwUnitFlagHealthInstanceLoan&)>&target){
 const auto p=impl_;
 if(!p||p->ui_thread!=std::this_thread::get_id())throw ScriptError("UnitFlag health issuer foreign VM creator");
 try{
  if(!target||p->calling||p->failed||p->worldview_native_host)throw ScriptError("UnitFlag health original-return issuer lacks idle VM/target");
  const auto storage=fifo.retain_storage_without_callbacks();const auto health=fifo.health_source_without_callbacks();
  const auto pair=fifo.owner_full_id_without_callbacks();
  std::size_t found=Impl::graph_no_owner;
  for(std::size_t i=0;i<p->worldview_created_instances.size();++i){const auto&r=p->worldview_created_instances[i];
   if(r.retired||!r.original_returned||r.owner!=pair.first||r.full_id!=pair.second)continue;
   if(found!=Impl::graph_no_owner)throw ScriptError("UnitFlag original full-ID returned instance duplicated");found=i;}
  if(found==Impl::graph_no_owner)throw ScriptError("UnitFlag health lacks genuine original constructor return; supplier absence is not stock nil");
  p->worldview_require_returned_health_registry_owned(found);
  fifo.require_origin_without_callbacks();
  if(fifo.owner_full_id_without_callbacks()!=pair)
   throw ScriptError("UnitFlag health native FIFO pair changed after original registry read");
  BnwUnitFlagHealthInstanceLoan::OriginalReturnedInstance witness;
  witness.vm=p;witness.lifetime=p->end_turn_lifetime;witness.fifo=storage;witness.health=health;witness.receipt=found;
  witness.epoch=p->failure_epoch;witness.writes=p->failed.writes_;witness.uncertainty=p->execution_uncertainty;
  {
   const BnwUnitFlagHealthInstanceLoan member(lease,*this,context,witness);
   target(member);
   // Every entered Source must be destroyed before returning to this lender;
   // PRE/POST metadata remains genuine but no previous raw Source is touched.
   member.state_->first();fifo.require_origin_without_callbacks();
   if(p->worldview_native_host||p->calling||p->failed||p->worldview_damage_dispatching)
    throw ScriptError("UnitFlag health callback/Source VM teardown incomplete");
   const auto metadata=member.state_->context.owner.lock();
   if(!metadata)throw ScriptError("UnitFlag health metadata owner retired during callback");
   BnwSelectedWorldViewContextOwner::require_health_source_storage_without_callbacks(
    member.state_->metadata,*metadata,member.state_->world,member.state_->generation,nullptr,0);
   if(member.state_->failed.load())throw ScriptError("UnitFlag health callback caught a quarantined membership failure");
   member.state_->require_stock_scope_identity_without_callbacks();
  }
  fifo.require_origin_without_callbacks();p->worldview_require_returned_health_registry_owned(found);
  fifo.require_origin_without_callbacks();
 }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
