// Included inside LuaScriptHost::Impl, after native WorldView lenders.
// The receiver and compiled caller belong to this actual retained child graph.
bool worldview_screen_query_active=false;
static void worldview_screen_receiver(lua_State* L,std::size_t slot){
 auto& p=owner(L);
 if(slot>=p.control_graphs.size()||!p.control_graphs[slot].bound||
    !p.control_graphs[slot].graph||!p.control_graphs[slot].graph->backend_||
    p.control_graphs[slot].screen_manager==LUA_NOREF||p.control_graphs[slot].screen_method==LUA_NOREF||
    !lua_istable(L,1)||!lua_rawequal(L,1,lua_upvalueindex(3)))
  return (void)luaL_error(L,"WorldView screen query canonical receiver absent");
 lua_rawgeti(L,LUA_REGISTRYINDEX,p.control_graphs[slot].screen_manager);
 const bool retained=lua_rawequal(L,1,-1)&&lua_rawequal(L,lua_upvalueindex(3),-1);
 lua_pop(L,1);
 lua_rawgeti(L,LUA_REGISTRYINDEX,p.control_graphs[slot].environment);
 lua_pushliteral(L,"UIManager");lua_rawget(L,-2);
 const bool environment=lua_rawequal(L,1,-1);lua_pop(L,2);
 lua_pushliteral(L,"GetScreenSizeVal");lua_rawget(L,1);
 lua_rawgeti(L,LUA_REGISTRYINDEX,p.control_graphs[slot].screen_method);
 const bool method=lua_isfunction(L,-1)&&lua_iscfunction(L,-1)&&
  lua_tocfunction(L,-1)==worldview_screen_size_call&&lua_rawequal(L,-1,-2);lua_pop(L,2);
 if(!retained||!environment||!method)return (void)luaL_error(L,"WorldView screen receiver/environment/method rebound");
}
static int worldview_screen_size_body(lua_State* L){
 auto& p=owner(L);const auto slot=graph_slot(L);
 if(lua_gettop(L)!=1||p.worldview_control_entry!=L)
  return luaL_error(L,"WorldView screen query requires protected native lender");
 if(!lua_checkstack(L,12))return luaL_error(L,"WorldView screen query stack exhausted");
 graph_require_context(L,slot);worldview_screen_receiver(L,slot);child_require_bindings(L);
 std::pair<std::int64_t,std::int64_t> result{};
 p.source_exception={};p.source_error=nullptr;
 try{
  p.worldview_require_callback_origin();
  if(p.worldview_screen_query_active){p.record_failure();throw ScriptError("WorldView screen query reentered");}
  p.worldview_screen_query_active=true;
  struct End{bool& active;~End(){active=false;}}end{p.worldview_screen_query_active};
  const auto frame=p.worldview_callback_frame();
  p.worldview_require_callback_origin();
  if(!p.worldview_native_host||slot>=p.control_graphs.size()||!p.control_graphs[slot].graph)
   throw ScriptError("WorldView screen native child/graph retired during scope capture");
  const auto child_slot=p.worldview_native_host->child;
  if(child_slot>=p.child_contexts.size()||!p.child_contexts[child_slot].source)
   throw ScriptError("WorldView screen actual selected source absent");
  const std::weak_ptr<ScriptUiControlGraph> graph_origin=p.control_graphs[slot].graph;
  const std::weak_ptr<const ScriptUiChildSource> source_origin=p.child_contexts[child_slot].source;
  const auto environment=p.child_contexts[child_slot].environment;
  const auto compiled=p.child_contexts[child_slot].execution_source;
  const auto identity=[&]{
   p.worldview_require_callback_origin();
   if(p.worldview_operation!=frame.loan||p.failure_epoch!=frame.epoch||
      p.execution_uncertainty!=frame.uncertainty||!p.worldview_native_host||
      slot>=p.control_graphs.size()||!p.control_graphs[slot].bound||
      !p.control_graphs[slot].graph||!p.control_graphs[slot].graph->backend_)
    throw ScriptError("WorldView screen query source/graph occurrence changed");
   const auto child=p.worldview_native_host->child;
   if(child!=child_slot||child>=p.child_contexts.size()||p.child_contexts[child].retired||
      p.child_contexts[child].name!=p.control_graphs[slot].graph->context()||
      p.child_contexts[child].environment!=environment||
      p.child_contexts[child].execution_source!=compiled||
      !p.worldview_native_child_controls_current(child))
    throw ScriptError("WorldView screen query actual selected child differs");
   const auto graph=graph_origin.lock();const auto source=source_origin.lock();
   const auto& actual_graph=p.control_graphs[slot].graph;const auto& actual_source=p.child_contexts[child].source;
   if(!graph||!source||graph.get()!=actual_graph.get()||graph.owner_before(actual_graph)||actual_graph.owner_before(graph)||
      source.get()!=actual_source.get()||source.owner_before(actual_source)||actual_source.owner_before(source))
    throw ScriptError("WorldView screen query canonical graph/source rebound");
  };
  identity();p.worldview_operation->require_same_vm(*p.worldview_host_owner);
  p.worldview_require_callback_origin();identity();
  const auto child=p.worldview_native_host->child;
  p.worldview_operation->require_same_source(*p.child_contexts[child].source);
  p.worldview_require_callback_origin();identity();
  p.control_graphs[slot].graph->require_current();p.worldview_require_callback_origin();identity();
  result=p.worldview_operation->screen_size_owned();
  // Independent native Host lifetime is the first POST before any saved reuse.
  p.worldview_require_callback_origin();identity();
  p.worldview_callback_fence(frame);p.worldview_require_callback_origin();identity();
  if(result.first<=0||result.second<=0)throw ScriptError("WorldView actual screen extent invalid");
 }catch(const std::exception& e){p.source_exception=std::current_exception();p.source_error=e.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView actual screen query failed";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 // No owning C++ scope or callback target survives these Lua checks/returns.
 worldview_require_lua_origin(L);worldview_screen_receiver(L,slot);child_require_bindings(L);
 lua_pushnumber(L,static_cast<lua_Number>(result.first));
 lua_pushnumber(L,static_cast<lua_Number>(result.second));return 2;
}
static int worldview_screen_size_call(lua_State* L){
 auto& p=owner(L);p.source_exception={};p.source_error=nullptr;
 try{p.worldview_require_callback_origin();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView screen native origin retired";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(lua_gettop(L)!=1)return luaL_error(L,"WorldView screen query argument count differs");
 if(!lua_checkstack(L,12))return luaL_error(L,"WorldView screen protected stack exhausted");
 p.worldview_control_validation=L;const auto slot=graph_slot(L);
 graph_require_context(L,slot);worldview_screen_receiver(L,slot);
 lua_pushvalue(L,lua_upvalueindex(1));lua_pushvalue(L,lua_upvalueindex(2));
 lua_pushvalue(L,lua_upvalueindex(3));lua_pushcclosure(L,worldview_screen_size_body,3);lua_insert(L,1);
 p.worldview_control_validation=nullptr;
 int status=0;p.source_exception={};p.source_error=nullptr;
 try{
  {WorldViewControlScope lender(p,L);lender.current();
   status=lua_pcall(L,1,LUA_MULTRET,0);
   if(!status){lender.current();p.ready=lender.prior_ready;
    if(lender.prior_failed)p.failed.set_loader_sentinel();else p.failed=false;
    lender.returned=true;}
  }
 }catch(...){p.source_exception=std::current_exception();p.source_error="WorldView screen protected lender failed";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(status)return lua_error(L);return lua_gettop(L);
}
static void worldview_install_screen_manager(lua_State* L,int environment,std::size_t slot){
 auto& p=owner(L);
 if(slot>=p.control_graphs.size()||!p.control_graphs[slot].bound||
    !p.control_graphs[slot].graph||!p.control_graphs[slot].graph->backend_||
    p.control_graphs[slot].screen_manager!=LUA_NOREF||p.control_graphs[slot].screen_method!=LUA_NOREF)
  return (void)luaL_error(L,"WorldView screen manager requires one actual graph installation");
 if(!lua_checkstack(L,10))return (void)luaL_error(L,"WorldView screen installation stack exhausted");
 if(native_postread_ui_manager_adopt_screen_owned(L,environment,slot))return;
 // Refuse an existing namespace instead of erasing another installed service.
 lua_getfield(L,environment,"UIManager");const bool vacant=lua_isnil(L,-1);lua_pop(L,1);
 if(!vacant)return (void)luaL_error(L,"WorldView screen manager namespace already installed");
 // A fresh child-owned table. It grants only this sealed current screen query.
 lua_newtable(L);lua_pushvalue(L,-1);
 p.control_graphs[slot].screen_manager=luaL_ref(L,LUA_REGISTRYINDEX);
 lua_pushinteger(L,static_cast<lua_Integer>(slot));lua_pushinteger(L,0);lua_pushvalue(L,-3);
 lua_pushcclosure(L,worldview_screen_size_call,3);lua_pushvalue(L,-1);
 p.control_graphs[slot].screen_method=luaL_ref(L,LUA_REGISTRYINDEX);
 lua_setfield(L,-2,"GetScreenSizeVal");
 lua_setfield(L,environment,"UIManager");
}
