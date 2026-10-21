// Actual VM producer of original event return. Source destruction is separate
// and must finish before the private FIFO completion constructor is invoked.
namespace civ5::runtime {
struct LuaScriptHost::UnitDamageEventReturn {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<Impl>vm;
 std::weak_ptr<ui::BnwEngineUiEndTurnQueue::VmLifetime>life;
 std::shared_ptr<BnwUnitFlagHealthFifoLoan::State>fifo;
 std::shared_ptr<BnwSelectedWorldViewContextOwner::State>metadata;
 std::weak_ptr<BnwSelectedWorldViewContextOwner>source;
 std::weak_ptr<const LiveSessionState>world;
 std::uint64_t generation=0,occurrence=0,epoch=0,writes=0,uncertainty=0;
 std::size_t index=0,registration=0,instance=0;
 const LuaScriptHost*host=nullptr;const void*state=nullptr;
 bool ready=false,original_returned=false;mutable bool issued=false;
};
std::shared_ptr<LuaScriptHost::UnitDamageEventReturn>
LuaScriptHost::dispatch_worldview_unit_damage_owned(const BnwWorldViewSourceOperationLoan&loan,
 const BnwUnitFlagHealthInstanceLoan&member,const BnwUnitFlagHealthFifoLoan&fifo){
 const auto p=impl_;
 if(!p||p->ui_thread!=std::this_thread::get_id())throw ScriptError("Damage native dispatch foreign creator before VM read");
 try{
  member.require_current();const auto membership=member.state_;const auto storage=fifo.retain_storage_without_callbacks();
  if(membership->fifo.get()!=storage.get()||membership->fifo.owner_before(storage)||storage.owner_before(membership->fifo)||
   p->worldview_health_membership_state||p->worldview_damage_dispatching||p->worldview_callback_dispatching||
   p->worldview_damage_registrations.size()!=1||p->calling)
   throw ScriptError("Damage dispatch lacks SAME member/FIFO and unique original listener");
  fifo.require_origin_without_callbacks();const auto ids=fifo.owner_full_id_without_callbacks();
  const auto damage=BnwUnitFlagHealthFifoLoan::damage_pair_from_storage_without_callbacks(storage);
  const auto occurrence=BnwUnitFlagHealthFifoLoan::occurrence_from_storage_without_callbacks(storage);
  const auto reference=BnwUnitFlagHealthInstanceLoan::retain_living_unit_reference_from_storage_without_callbacks(membership);
  const auto current_damage=reference->read("GetDamage");
  if(!occurrence||ids!=std::pair{membership->owner,membership->id}||
   !std::holds_alternative<std::int64_t>(current_damage)||damage.first!=std::get<std::int64_t>(current_damage))
   throw ScriptError("Damage dispatch actual FIFO/native payload changed");
  if(p->worldview_damage_returns.size()>=65536)throw ScriptError("Original damage retained return budget exhausted");
  auto returned=std::make_shared<UnitDamageEventReturn>();returned->vm=p;returned->life=p->end_turn_lifetime;
  returned->fifo=storage;returned->metadata=membership->metadata;returned->source=membership->context.owner;
  returned->world=membership->world;returned->generation=membership->generation;returned->occurrence=occurrence;
  returned->host=this;returned->state=p->state;returned->epoch=p->failure_epoch;returned->writes=p->failed.writes_;
  returned->uncertainty=p->execution_uncertainty;returned->ready=p->ready;returned->index=p->worldview_damage_returns.size();
  returned->instance=membership->receipt;p->worldview_damage_returns.push_back(returned);
  Impl::WorldViewNativeHostScope native(*p,*this,loan);
  p->worldview_require_callback_origin();member.require_current();
  const auto&registration=p->worldview_damage_registrations.front();
  if(registration.created!=membership->registration)throw ScriptError("Damage callback shares another original instance parent");
  const auto&created=p->worldview_created_registrations[registration.created];
  native.child=created.child;native.child_source=created.source;
  const auto frame=p->worldview_callback_frame();p->worldview_damage_owner(0,frame);
  Impl::WorldViewDamageJob job{p.get(),0,ids.first,ids.second,damage.first,damage.second};
  auto*L=registration.registered;const int top=lua_gettop(L);const auto previous=p->graph_execution_scope;
  const auto prior_hook=lua_gethook(L);const int mask=lua_gethookmask(L),count=lua_gethookcount(L);
  struct Restore {Impl&p;lua_State*L;int top;std::size_t previous;lua_Hook hook;int mask,count;
   ~Restore(){lua_sethook(L,hook,mask,count);lua_settop(L,top);p.graph_execution_scope=previous;
    p.worldview_health_membership_state.reset();p.calling=false;p.worldview_damage_dispatching=false;p.worldview_callback_dispatching=false;}};
  {
   Restore restore{*p,L,top,previous,prior_hook,mask,count};
   p->worldview_health_membership_state=membership;p->worldview_damage_dispatching=true;
   p->worldview_callback_dispatching=true;p->calling=true;
   Impl::WorldViewLoadedChildCallScope loaded(*p,Impl::WorldViewLoadedChildCallScope::Kind::damage,0);
   if(p->budget.instructions)lua_sethook(L,Impl::hook,LUA_MASKCOUNT,1000);
   const int status=lua_cpcall(L,Impl::worldview_damage_protected,&job);
   // FIRST pure bookends before any callbackful source/full receiver check.
   native.pure();loaded.pure();fifo.require_origin_without_callbacks();member.require_current();
   if(status||p->exhausted||!job.invoked||!job.returned){
    const auto error=status?p->worldview_callback_lua_error(L):"Original OnUnitSetDamage did not return";
    p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw ScriptError(error);
   }
   p->worldview_damage_owner(0,frame);loaded.accept(job.invoked,job.returned);member.require_current();
  }
  native.pure();fifo.require_origin_without_callbacks();p->worldview_require_callback_origin();member.require_current();
  native.returned=true;returned->original_returned=true;return returned;
 }catch(...){p->worldview_retire_damage_registrations();p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
struct BnwUnitFlagHealthFifoLoan::DeliveryReturn::OriginalReturnedEvent {
 const LuaScriptHost*host;
 const std::shared_ptr<LuaScriptHost::UnitDamageEventReturn>actual;
 OriginalReturnedEvent(const LuaScriptHost&vm,std::shared_ptr<LuaScriptHost::UnitDamageEventReturn>r):host(&vm),actual(std::move(r)){}
};
BnwUnitFlagHealthFifoLoan::DeliveryReturn::DeliveryReturn(const BnwUnitFlagHealthFifoLoan&fifo,const OriginalReturnedEvent&w)
 :state_(fifo.retain_storage_without_callbacks()){
 const auto r=w.actual;
 if(!r||r->creator!=std::this_thread::get_id())throw ScriptError("Original damage completion foreign or absent return issuer");
 const auto life=r->life.lock();
 if(!life||life->creator!=r->creator||!life->alive.load()||life->failed.load()||life->host!=w.host||life->vm!=r->state)
  throw ScriptError("Original damage completion VM facade expired before return access");
 occurrence_=w.host->consume_original_unitflag_health_return_without_callbacks(fifo,r);
}
std::uint64_t LuaScriptHost::consume_original_unitflag_health_return_without_callbacks(
 const BnwUnitFlagHealthFifoLoan&fifo,const std::shared_ptr<UnitDamageEventReturn>&r)const{
 if(!r||r->creator!=std::this_thread::get_id())throw ScriptError("Original damage completion foreign creator before mutable return access");
 const auto life=r->life.lock();
 if(!life||!life->alive.load()||life->failed.load()||life->host!=this||life->vm!=r->state)
  throw ScriptError("Original damage completion actual VM facade retired");
 const auto p=r->vm.lock();
 const auto storage=fifo.retain_storage_without_callbacks();
 if(!p||p.get()!=impl_.get()||r->vm.owner_before(impl_)||impl_.owner_before(r->vm)||
  r->host!=this||!r->original_returned||r->issued||p->state!=r->state||p->ready!=r->ready||
  p->end_turn_lifetime.owner_before(r->life)||r->life.owner_before(p->end_turn_lifetime)||
  p->ui_thread!=r->creator||p->end_turn_lifetime.get()!=life.get()||p->failure_epoch!=r->epoch||
  p->failed.writes_!=r->writes||p->failed.exhausted_||p->execution_uncertainty!=r->uncertainty||
  p->calling||p->failed||p->worldview_native_host||p->worldview_operation||p->worldview_health_membership_state||
  p->worldview_damage_dispatching||p->worldview_callback_dispatching||r->index>=p->worldview_damage_returns.size()||
  p->worldview_damage_returns[r->index].get()!=r.get()||
  p->worldview_damage_returns[r->index].owner_before(r)||r.owner_before(p->worldview_damage_returns[r->index])||
  storage.get()!=r->fifo.get()||storage.owner_before(r->fifo)||r->fifo.owner_before(storage))
  throw ScriptError("Original damage completion is not actual canonical returned occurrence after native teardown");
 const auto source=r->source.lock();if(!source)throw ScriptError("Original damage completion metadata facade retired");
 BnwSelectedWorldViewContextOwner::require_health_source_storage_without_callbacks(
  r->metadata,*source,r->world,r->generation,nullptr,0);
 BnwUnitFlagHealthFifoLoan::require_origin_from_storage_without_callbacks(storage);
 const auto occurrence=BnwUnitFlagHealthFifoLoan::occurrence_from_storage_without_callbacks(storage);
 if(!occurrence||occurrence!=r->occurrence)throw ScriptError("Original damage completion changed actual FIFO head");
 // This one native record was stored before protected original dispatch; no
 // caller boolean, event name, listener count or frame was an issuer.
 r->issued=true;return occurrence;
}
BnwUnitFlagHealthFifoLoan::DeliveryReturn LuaScriptHost::complete_unitflag_health_fifo_owned(
 const BnwUnitFlagHealthFifoLoan&fifo,const std::shared_ptr<UnitDamageEventReturn>&returned){
 const auto p=impl_;if(!p||p->ui_thread!=std::this_thread::get_id())throw ScriptError("Damage return issuer foreign creator");
 try{
  if(!returned||returned->vm.lock().get()!=p.get()||returned->host!=this)
   throw ScriptError("Damage return belongs to another actual VM");
  fifo.require_origin_without_callbacks();
  p->worldview_require_returned_health_registry_owned(returned->instance);
  fifo.require_origin_without_callbacks();
  const BnwUnitFlagHealthFifoLoan::DeliveryReturn::OriginalReturnedEvent witness(*this,returned);
  return BnwUnitFlagHealthFifoLoan::DeliveryReturn(fifo,witness);
 }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
