// Native SP client scalar reads for actual initial ActionInfoPanel.
// Owned native reset fields, never a default boolean/processing observation.
// Reuses the unchanged genuine Source3 protected lender and Source4 lineage.
static int worldview_sp_client_scalar_body(lua_State* L){
 auto& p=owner(L);auto* const lender=p.worldview_game_active_read_scope;
 if(!lender||lender->entry!=L){p.record_failure();return luaL_error(L,"WorldView SP client scalar read has no actual protected lender");}
 p.source_exception={};p.source_error=nullptr;
 try{lender->current();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView SP client scalar read native origin refused";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 lua_Debug frame{};std::size_t compiled=graph_no_owner;int closure=0;
 for(int level=1;lua_getstack(L,level,&frame);++level){
  if(!lua_getinfo(L,"f",&frame))break;
  if(lua_iscfunction(L,-1)){lua_pop(L,1);continue;}
  compiled=p.graph_compiled_owner(L,-1);closure=lua_gettop(L);break;
 }
 const auto child=lender->child;
 if(compiled==graph_no_owner||compiled>=p.graph_compiled_sources.size()||child>=p.child_contexts.size()||
  p.child_contexts[child].name!="ActionInfoPanel"||p.graph_compiled_sources[compiled].context!=p.child_contexts[child].name){
  p.record_failure();return luaL_error(L,"WorldView SP client scalar read caller is not actual ActionInfoPanel Proto");
 }
 lua_getfenv(L,closure);lua_rawgeti(L,LUA_REGISTRYINDEX,lender->environment);
 const bool same_environment=lua_rawequal(L,-1,-2);lua_pop(L,2);
 lua_rawgeti(L,LUA_REGISTRYINDEX,lender->execution_thread);
 const bool same_thread=lua_isthread(L,-1)&&lua_tothread(L,-1)==L;lua_pop(L,2);
 if(!same_environment||!same_thread){p.record_failure();return luaL_error(L,"WorldView SP client scalar read actual environment/thread changed");}
 bool result=false;p.source_exception={};p.source_error=nullptr;
 try{
  lender->current();p.map.calculated_yield->require_current();lender->current();
  const auto selector=lua_tointeger(L,1);
  if(selector==0)result=p.map.calculated_yield->local_client_has_sent_turn_complete();
  else if(selector==1)result=p.map.calculated_yield->local_client_has_sent_turn_all_complete();
  else if(selector==2)result=p.map.calculated_yield->local_client_is_processing_messages();
  else throw ScriptError("WorldView SP client scalar selector unavailable");
  lender->current();p.map.calculated_yield->require_current();lender->current();
  lender->result=result?1:0;lender->query_returned=true;
 }catch(const std::exception& error){p.source_exception=std::current_exception();p.source_error=error.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView actual SP client scalar field query failed";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 lua_pushboolean(L,result);return 1;
}
static int worldview_sp_client_scalar(lua_State* L){
 auto& p=owner(L);p.source_exception={};p.source_error=nullptr;
 try{p.worldview_require_callback_origin();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView SP client scalar read actual native origin refused";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(!lua_checkstack(L,8)){p.record_failure();return luaL_error(L,"WorldView SP client scalar read protected stack unavailable");}
 const int args=lua_gettop(L)+1;lua_pushinteger(L,lua_tointeger(L,lua_upvalueindex(1)));lua_insert(L,1);lua_pushcfunction(L,worldview_sp_client_scalar_body);lua_insert(L,1);
 int status=0;
 try{
  {WorldViewGameActiveReadScope lender(p,L);lender.provisional();
   status=lua_pcall(L,args,1,0);
   if(!status){
    lender.current();
    if(!lender.query_returned||lua_gettop(L)!=1||lua_type(L,1)!=LUA_TBOOLEAN||lua_toboolean(L,1)!=(lender.result!=0))
     throw ScriptError("WorldView SP client scalar publication differs from genuine bool field read");
    p.ready=lender.prior_ready;if(lender.prior_failed)p.failed.set_loader_sentinel();else p.failed=false;
    lender.provisional_active=false;lender.pure();lender.returned=true;
   }
  }
 }catch(const std::exception& error){p.source_exception=std::current_exception();p.source_error=error.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView Player protected native field lender refused";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(status)return lua_error(L);return 1;
}
// The public Network callback remains zero-upvalue. Only a real closure
// with the qualified op0/op1 selector enters the unchanged protected SP lender.
static int worldview_sp_network_scalar_call_owned(lua_State* L,int operation){
 auto&p=owner(L);native_db_query_current(L);
 const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
 lua_State*const vm=p.state;global_State*const global=G(L);
 p.source_exception={};p.source_error=nullptr;
 try{p.worldview_require_callback_origin();
  if(operation<0||operation>1||!vm||p.state!=vm||G(vm)!=global||std::this_thread::get_id()!=p.ui_thread||
   !p.worldview_native_host||p.worldview_native_host->child>=p.child_contexts.size()||p.failed.exhausted_)
   throw ScriptError("Network SP actual Host/VM/selector/source retired");
 }catch(...){p.source_exception=std::current_exception();p.source_error="Network SP genuine origin FIRST refused";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 const int arguments=lua_gettop(L);
 lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[p.worldview_native_host->child].environment);
 if(!native_network_selected_namespace_current_owned(L,lua_gettop(L))){
  p.record_failure();return luaL_error(L,"Network SP caller no longer has its genuine published namespace");
 }
 lua_settop(L,arguments);
 lua_pushinteger(L,operation);lua_pushcclosure(L,worldview_sp_client_scalar,1);lua_insert(L,1);
 native_db_query_current(L);worldview_require_lua_origin(L);
 if(p.state!=vm||G(L)!=global||G(vm)!=global||epoch!=p.failure_epoch||writes!=p.failed.writes_||
  uncertainty!=p.execution_uncertainty||p.failed.exhausted_){
  p.record_failure();return luaL_error(L,"Network SP closure allocation retired actual source/VM");
 }
 lua_call(L,arguments,LUA_MULTRET);native_db_query_current(L);worldview_require_lua_origin(L);
 if(p.state!=vm||G(L)!=global||G(vm)!=global||epoch!=p.failure_epoch||writes!=p.failed.writes_||
  uncertainty!=p.execution_uncertainty||p.failed.exhausted_||lua_gettop(L)!=1||lua_type(L,1)!=LUA_TBOOLEAN){
  p.record_failure();return luaL_error(L,"Network SP actual field-read publication/source changed at POST");
 }
 lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[p.worldview_native_host->child].environment);
 if(!native_network_selected_namespace_current_owned(L,lua_gettop(L))){
  p.record_failure();return luaL_error(L,"Network SP actual namespace retired at POST");
 }
 lua_settop(L,1);return 1;
}
// Public Game endpoint remains the SDK zero-upvalue function. The
// actual internal op2 closure enters the existing protected SP scalar lender;
// ActionInfoPanel Proto/fenv/thread and real client guards remain unchanged.
static int worldview_sp_game_processing_messages_call_owned(lua_State*L){
 auto&p=owner(L);native_gamecore_invocation_current_owned(L);native_db_query_current(L);
 const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
 lua_State*const vm=p.state;global_State*const global=G(L);
 const int arguments=lua_gettop(L);
 if(!p.worldview_native_host||p.worldview_native_host->child>=p.child_contexts.size()){
  p.record_failure();return luaL_error(L,"Game SP processing actual selected child absent");
 }
 const auto child=p.worldview_native_host->child;
 lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[child].environment);
 if(!native_gamecore_game_processing_endpoint_current_owned(L,lua_gettop(L))){
  p.record_failure();return luaL_error(L,"Game SP processing no longer has its genuine SDK namespace/endpoint");
 }
 lua_settop(L,arguments);lua_pushinteger(L,2);lua_pushcclosure(L,worldview_sp_client_scalar,1);lua_insert(L,1);
 native_gamecore_invocation_current_owned(L);native_db_query_current(L);worldview_require_lua_origin(L);
 if(p.state!=vm||G(L)!=global||G(vm)!=global||epoch!=p.failure_epoch||writes!=p.failed.writes_||
  uncertainty!=p.execution_uncertainty||p.failed.exhausted_||!p.worldview_native_host||p.worldview_native_host->child!=child){
  p.record_failure();return luaL_error(L,"Game SP processing closure allocation retired actual Source/VM");
 }
 lua_call(L,arguments,LUA_MULTRET);native_gamecore_invocation_current_owned(L);native_db_query_current(L);worldview_require_lua_origin(L);
 if(p.state!=vm||G(L)!=global||G(vm)!=global||epoch!=p.failure_epoch||writes!=p.failed.writes_||
  uncertainty!=p.execution_uncertainty||p.failed.exhausted_||!p.worldview_native_host||p.worldview_native_host->child!=child||lua_gettop(L)!=1||lua_type(L,1)!=LUA_TBOOLEAN){
  p.record_failure();return luaL_error(L,"Game SP processing actual field publication/Source changed at POST");
 }
 lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[child].environment);
 if(!native_gamecore_game_processing_endpoint_current_owned(L,lua_gettop(L))){
  p.record_failure();return luaL_error(L,"Game SP processing genuine SDK namespace retired at POST");
 }
 lua_settop(L,1);return 1;
}
static void worldview_install_sp_client_scalars(lua_State* L,int environment,std::string_view context){
 auto& p=owner(L);if(!p.worldview_native_host||context!="ActionInfoPanel")return;
 p.source_exception={};p.source_error=nullptr;
 try{
  p.worldview_require_callback_origin();const auto* const native=p.worldview_native_host;
  if(!p.calling||!p.map.calculated_yield||!p.end_turn_child_loader||p.end_turn_child_loader->entry!=L||!p.end_turn_child_loader->current()||
   !native||native->child>=p.child_contexts.size()||!p.worldview_native_child_controls_current(native->child)||
   p.end_turn_child_loader->child!=native->child||p.child_contexts[native->child].name!=context)
   throw ScriptError("WorldView SP client scalar installation lacks genuine native ActionInfoPanel child");
 }catch(...){p.source_exception=std::current_exception();p.source_error="WorldView SP client scalar installation source origin refused";}
 if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
 // Network is the real original79-field master/readonly proxy publication.
 // Never replace it with a two-entry table or accept matching bool values.
 if(!native_network_selected_namespace_current_owned(L,environment)){
  p.record_failure();return (void)luaL_error(L,"WorldView SP client installation requires its genuine original Network publisher");
 }
 lua_pushliteral(L,"Game");lua_rawget(L,environment);
 if(!lua_istable(L,-1)){p.record_failure();return (void)luaL_error(L,"WorldView SP client installation actual Game namespace absent");}
 lua_pushliteral(L,"IsProcessingMessages");lua_rawget(L,-2);
 if(!lua_isnil(L,-1)){
  if(!native_gamecore_game_processing_endpoint_current_owned(L,environment)){p.record_failure();return (void)luaL_error(L,"WorldView SP client installation refuses foreign existing Game API");}
  lua_pop(L,2); // preserve SAME completed SDK Game endpoint242 nup0
 }else{
  lua_pop(L,1);lua_pushliteral(L,"IsProcessingMessages");lua_pushinteger(L,2);lua_pushcclosure(L,worldview_sp_client_scalar,1);lua_rawset(L,-3);lua_pop(L,1);
 }
 p.source_exception={};p.source_error=nullptr;
 try{p.worldview_require_callback_origin();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView SP client scalar source changed at publication";}
 if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
}
