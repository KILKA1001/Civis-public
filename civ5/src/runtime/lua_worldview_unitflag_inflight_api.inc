namespace civ5::runtime {
void LuaScriptHost::require_unitflag_initializing_button_owned(const BnwWorldViewSourceOperationLoan& loan,
 const std::weak_ptr<const void>& core,std::uint64_t node,std::uint64_t root,
 std::int32_t actor,std::int32_t full_id)const {
 const auto p=impl_;
 if(p->ui_thread!=std::this_thread::get_id())throw ScriptError("UnitFlag Initialize admission foreign creator");
 try{
  const auto first=[&]{
   p->worldview_require_callback_origin();
   if(p->worldview_host_owner!=this||p->worldview_operation!=&loan||!p->calling||
      !p->worldview_created_dispatching||!p->worldview_callback_dispatching||
      !p->worldview_created_pending_occurrence||!p->worldview_unitflag_constructor_job||
      !p->worldview_control_entry||core.expired())
    throw ScriptError("UnitFlag button lacks actual in-flight original created/Initialize control scope");
  };
  first();const auto serial=loan.operation_serial();first();
  const auto* const ctor=p->worldview_unitflag_constructor_job;
  const auto registration=ctor->index;
  if(ctor->owner!=p.get()||!ctor->invoked||ctor->returned||ctor->player!=actor||ctor->full_id!=full_id||
     registration>=p->worldview_created_registrations.size()||actor<0||actor>=64||full_id<0||!node||!root)
   throw ScriptError("UnitFlag active genuine constructor/native full-ID differs");
  const auto& e=p->worldview_created_registrations[registration];
  if(e.retired||e.registered!=p->worldview_control_entry||e.graph_slot>=p->control_graphs.size())
   throw ScriptError("UnitFlag Initialize actual registered coroutine/graph retired");
  const auto occurrence=p->worldview_created_pending_occurrence;
  occurrence->require_origin_without_callbacks();first();
  const auto actual_ids=occurrence->actual_owner_full_id();first();
  if(actual_ids.first!=actor||actual_ids.second!=full_id)throw ScriptError("UnitFlag creator unit is not actual pending FIFO occurrence");
  occurrence->require_same_source(loan);first();
  const auto graph=p->control_graphs[e.graph_slot].graph;
  BnwWorldViewScriptUiAdapter::require_initializing_button_core_owned(graph,core,loan,*this,node,root);first();
  loan.require_same_vm(*this);first();loan.require_same_source(*e.source);first();
  // Genuine native unit provider is independently re-read; a Void tuple or
  // Lua member cannot synthesize the actual complete native unit identity.
  if(!p->gameplay.units)throw ScriptError("UnitFlag Initialize lacks actual retained unit provider");
  p->gameplay.units->require_current();first();
  const auto unit=p->gameplay.units->find(actor,full_id);first();
  if(!unit)throw ScriptError("UnitFlag Initialize full-ID unit absent");
  unit->require_current();first();const auto owner_value=unit->read("GetOwner");first();
  const auto id_value=unit->read("GetID");first();
  const auto* owner=std::get_if<std::int64_t>(&owner_value);const auto* id=std::get_if<std::int64_t>(&id_value);
  if(!owner||!id||*owner!=actor||*id!=full_id)throw ScriptError("UnitFlag Initialize native full-ID slot alias refused");
  auto* const L=e.registered;const int top=lua_gettop(L);
  struct StackReturn{lua_State* state;int top;~StackReturn(){lua_settop(state,top);}} stack_return{L,top};
  Impl::UnitFlagInitializingButtonJob job{p.get(),registration,node,root,actor,full_id};
  const int status=lua_cpcall(L,Impl::unitflag_initializing_button_protected,&job);
  // Stack cleanup follows the protected return, even on genuine Lua error.
  first();const auto message=status?p->worldview_callback_lua_error(L):std::string{};lua_settop(L,top);
  if(status||!job.returned)throw ScriptError(status?message:"UnitFlag genuine Initialize binding query did not return");
  if(p->worldview_unitflag_constructor_job!=ctor||p->worldview_created_pending_occurrence!=occurrence||
     ctor->returned||ctor->player!=actor||ctor->full_id!=full_id)
   throw ScriptError("UnitFlag original construction scope changed during protected binding query");
  BnwWorldViewScriptUiAdapter::require_initializing_button_core_owned(graph,core,loan,*this,node,root);first();
  unit->require_current();first();loan.require_operation(serial);first();
 }catch(...){p->worldview_retire_created_registrations();p->record_failure();
  if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
