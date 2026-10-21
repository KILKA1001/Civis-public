// File-scope SAME Host private entry; only actual Bootstrap calls this with
// the genuine FIFO loan. A public event name or payload tuple has no entry.
namespace civ5::runtime {
void LuaScriptHost::dispatch_worldview_unit_created_owned(const BnwWorldViewSourceOperationLoan&loan,
 const graphics::BnwUnitSetupGraphicalQueue::CreatedOccurrenceLoan&occurrence){
 const auto p=impl_;
 if(std::this_thread::get_id()!=p->ui_thread)throw ScriptError("Created native dispatch foreign creator");
 try{
  occurrence.require_origin_without_callbacks();occurrence.require_same_source(loan);
  Impl::WorldViewNativeHostScope native(*p,*this,loan);
  p->worldview_require_callback_origin();loan.require_same_vm(*this);p->worldview_require_callback_origin();
  if(p->worldview_created_registrations.size()!=1||p->worldview_created_dispatching||p->worldview_callback_dispatching||p->worldview_created_pending_occurrence)
   throw ScriptError("Created dispatch lacks unique actual retained registration");
  const auto&e=p->worldview_created_registrations.front();
  native.child=e.child;native.child_source=e.source;
  const auto frame=p->worldview_callback_frame();p->worldview_created_owner(0,frame);
  occurrence.require_origin_without_callbacks();const auto ids=occurrence.actual_owner_full_id();
  const auto visibility=occurrence.actual_original_fog_not_invisible();
  occurrence.require_same_source(loan);p->worldview_created_owner(0,frame);
  if(p->worldview_created_instances.size()>=65536)throw ScriptError("Created retained instance budget exhausted");
  const auto receipt=p->worldview_created_instances.size();p->worldview_created_instances.emplace_back();
  auto&r=p->worldview_created_instances.back();r.registration=0;r.owner=ids.first;r.full_id=ids.second;
  Impl::WorldViewCreatedJob job{p.get(),0,receipt,ids.first,ids.second,visibility.first,visibility.second};
  auto*L=e.registered;const int top=lua_gettop(L);const auto previous=p->graph_execution_scope;
  const auto prior_hook=lua_gethook(L);const int mask=lua_gethookmask(L),count=lua_gethookcount(L);
  struct Restore{Impl&p;lua_State*L;int top;std::size_t previous;lua_Hook hook;int mask,count;
   ~Restore(){lua_sethook(L,hook,mask,count);lua_settop(L,top);p.graph_execution_scope=previous;
    p.worldview_created_pending_occurrence=nullptr;p.worldview_unitflag_constructor_job=nullptr;
    p.calling=false;p.worldview_created_dispatching=false;p.worldview_callback_dispatching=false;}};
  {
   Restore restore{*p,L,top,previous,prior_hook,mask,count};
   occurrence.require_same_source(loan);p->worldview_created_pending_occurrence=&occurrence;
   p->worldview_created_dispatching=true;p->worldview_callback_dispatching=true;p->calling=true;
   Impl::WorldViewLoadedChildCallScope loaded(*p,Impl::WorldViewLoadedChildCallScope::Kind::created,0);
   if(p->budget.instructions)lua_sethook(L,Impl::hook,LUA_MASKCOUNT,1000);
   const int status=lua_cpcall(L,Impl::worldview_created_protected,&job);
   // FIRST native/PURE POST before a next callbackful source operation.
   native.pure();loaded.pure();occurrence.require_origin_without_callbacks();
   if(status||p->exhausted||!job.invoked||!job.returned){
    const auto message=status?p->worldview_callback_lua_error(L):"Original created constructor did not return";
    p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw ScriptError(message);
   }
   p->worldview_created_owner(0,frame);loaded.accept(job.invoked,job.returned);
   occurrence.require_origin_without_callbacks();p->worldview_created_accept(0,receipt,frame);
  }
  p->worldview_require_callback_origin();occurrence.require_same_source(loan);
  // Original callback effects are now retained. A later failure quarantines;
  // neither flags nor a frame receipt substitutes the actual FIFO adoption.
  occurrence.record_original_constructor_returned();p->worldview_require_callback_origin();
  native.returned=true;
 }catch(...){p->worldview_retire_created_registrations();p->record_failure();
  if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
