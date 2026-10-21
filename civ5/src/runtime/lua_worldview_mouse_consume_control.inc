// Included inside actual LuaScriptHost::Impl after native scopes/callbacks.
// Lua allocations/checks/pushes occur outside every owning C++ backend scope.
bool worldview_mouse_consume_control_active=false;
static int worldview_mouse_consume_control_body(lua_State* L){
 auto& p=owner(L);const auto slot=graph_slot(L);
 const int op=static_cast<int>(lua_tointeger(L,lua_upvalueindex(2)));
 if(op!=29||lua_gettop(L)!=2||p.worldview_control_entry!=L)
  return luaL_error(L,"ConsumeMouseOver control requires genuine protected lender/arguments");
 if(!lua_checkstack(L,12))return luaL_error(L,"ConsumeMouseOver control stack exhausted");
 graph_require_context(L,slot);const auto id=graph_checked(L,1,slot)->id;child_require_bindings(L);
 if(lua_type(L,2)!=LUA_TBOOLEAN)return luaL_error(L,"SetConsumeMouseOver selected argument must be a boolean command");
 const bool value=lua_toboolean(L,2)!=0;
 p.failed.set_loader_sentinel();p.ready=false;p.control_graphs[slot].touched=true;
 p.source_exception={};p.source_error=nullptr;
 try {
  p.worldview_require_callback_origin();
  if(p.worldview_mouse_consume_control_active)throw ScriptError("ConsumeMouseOver control backend reentered");
  p.worldview_mouse_consume_control_active=true;
  struct Exit{bool& active;~Exit(){active=false;}}exit{p.worldview_mouse_consume_control_active};
  const auto frame=p.worldview_callback_frame();p.worldview_require_callback_origin();
  if(!p.worldview_native_host||slot>=p.control_graphs.size()||!p.control_graphs[slot].graph)
   throw ScriptError("ConsumeMouseOver actual selected graph retired during capture");
  const auto child_slot=p.worldview_native_host->child;
  if(child_slot>=p.child_contexts.size()||!p.child_contexts[child_slot].source)
   throw ScriptError("ConsumeMouseOver actual child source absent");
  const std::weak_ptr<ScriptUiControlGraph> graph_origin=p.control_graphs[slot].graph;
  const std::weak_ptr<const ScriptUiChildSource> source_origin=p.child_contexts[child_slot].source;
  const auto environment=p.child_contexts[child_slot].environment;
  const auto compiled=p.child_contexts[child_slot].execution_source;
  const auto identity=[&]{
   p.worldview_require_callback_origin();
   if(p.worldview_operation!=frame.loan||p.failure_epoch!=frame.epoch||p.execution_uncertainty!=frame.uncertainty||
      !p.worldview_native_host||slot>=p.control_graphs.size()||!p.control_graphs[slot].bound||
      !p.control_graphs[slot].graph||!p.control_graphs[slot].graph->backend_)
    throw ScriptError("ConsumeMouseOver actual graph/source occurrence changed");
   const auto child=p.worldview_native_host->child;
   if(child!=child_slot||child>=p.child_contexts.size()||p.child_contexts[child].retired||
      p.child_contexts[child].name!=p.control_graphs[slot].graph->context()||
      p.child_contexts[child].name!="UnitFlagManager"||p.child_contexts[child].environment!=environment||
      p.child_contexts[child].execution_source!=compiled||!p.worldview_native_child_controls_current(child))
    throw ScriptError("ConsumeMouseOver actual native selected child differs");
   const auto graph=graph_origin.lock();const auto source=source_origin.lock();
   const auto& actual_graph=p.control_graphs[slot].graph;const auto& actual_source=p.child_contexts[child].source;
   if(!graph||!source||graph.get()!=actual_graph.get()||graph.owner_before(actual_graph)||actual_graph.owner_before(graph)||
      source.get()!=actual_source.get()||source.owner_before(actual_source)||actual_source.owner_before(source))
    throw ScriptError("ConsumeMouseOver canonical graph/source rebound");
  };
  identity();p.worldview_operation->require_same_vm(*p.worldview_host_owner);p.worldview_require_callback_origin();identity();
  p.worldview_operation->require_same_source(*p.child_contexts[child_slot].source);p.worldview_require_callback_origin();identity();
  p.control_graphs[slot].graph->set_consume_mouse_over(id,value);
  // Independent Host lifetime FIRST before all saved canonical reuse.
  p.worldview_require_callback_origin();identity();
  p.worldview_callback_fence(frame);p.worldview_require_callback_origin();identity();
 }catch(const std::exception& error){p.source_exception=std::current_exception();p.source_error=error.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="Actual protected layout control failed";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 // Only POD survives these Lua checks/pushes; all scope/weak/callback RAII ended.
 worldview_require_lua_origin(L);graph_require_context(L,slot);
 if(graph_checked(L,1,slot)->id!=id)return luaL_error(L,"ConsumeMouseOver actual control receiver changed");
 child_require_bindings(L);
 return 0;
}
static int worldview_mouse_consume_control_call(lua_State* L){
 auto& p=owner(L);p.source_exception={};p.source_error=nullptr;
 try{p.worldview_require_callback_origin();}
 catch(...){p.source_exception=std::current_exception();p.source_error="ConsumeMouseOver native Host origin retired";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(!lua_checkstack(L,12))return luaL_error(L,"ConsumeMouseOver protected stack exhausted");
 p.worldview_control_validation=L;
 const auto slot=graph_slot(L);const int op=static_cast<int>(lua_tointeger(L,lua_upvalueindex(2)));
 if(op!=29||lua_gettop(L)!=2)return luaL_error(L,"ConsumeMouseOver protected argument profile differs");
 graph_require_context(L,slot);(void)graph_checked(L,1,slot);
 if(!p.control_graphs[slot].graph->backend_)return luaL_error(L,"ConsumeMouseOver canonical backend absent");
 const int args=lua_gettop(L);
 lua_pushvalue(L,lua_upvalueindex(1));lua_pushvalue(L,lua_upvalueindex(2));
 lua_pushcclosure(L,worldview_mouse_consume_control_body,2);lua_insert(L,1);
 p.worldview_control_validation=nullptr;
 int status=0;p.source_exception={};p.source_error=nullptr;
 try {
  {WorldViewControlScope lender(p,L);lender.current();
   status=lua_pcall(L,args,LUA_MULTRET,0);
   if(!status){lender.current();p.ready=lender.prior_ready;
    if(lender.prior_failed)p.failed.set_loader_sentinel();else p.failed=false;
    lender.returned=true;}
  }
 }catch(...){p.source_exception=std::current_exception();p.source_error="ConsumeMouseOver protected lender failed";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(status)return lua_error(L);return lua_gettop(L);
}
