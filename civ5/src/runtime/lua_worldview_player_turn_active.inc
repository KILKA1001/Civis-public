// Source-backed readonly CvLuaPlayer::IsTurnActive for the actual initial
// ActionInfoPanel load. Reuses the genuine Source3 query lender, not DTOs.
static int worldview_player_turn_active_body(lua_State* L){
 auto& p=owner(L);auto* const lender=p.worldview_game_active_read_scope;
 if(!lender||lender->entry!=L){p.record_failure();return luaL_error(L,"WorldView Player turn read has no actual protected lender");}
 p.source_exception={};p.source_error=nullptr;
 try{lender->current();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView Player turn read native origin refused";}
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
  p.record_failure();return luaL_error(L,"WorldView Player turn read caller is not actual ActionInfoPanel Proto");
 }
 lua_getfenv(L,closure);lua_rawgeti(L,LUA_REGISTRYINDEX,lender->environment);
 const bool same_environment=lua_rawequal(L,-1,-2);lua_pop(L,2);
 lua_rawgeti(L,LUA_REGISTRYINDEX,lender->execution_thread);
 const bool same_thread=lua_isthread(L,-1)&&lua_tothread(L,-1)==L;lua_pop(L,2);
 if(!same_environment||!same_thread){p.record_failure();return luaL_error(L,"WorldView Player turn read actual environment/thread changed");}
 // Raw exact receiver identity avoids invoking a caller-supplied __index.
 // Extra arguments retain BasicLuaMethod getter semantics and are ignored.
 if(!lua_istable(L,1)){p.record_failure();return luaL_error(L,"WorldView Player turn read receiver is not a table");}
 lua_pushliteral(L,"__instance");lua_rawget(L,1);
 if(lua_type(L,-1)!=LUA_TUSERDATA||lua_objlen(L,-1)!=sizeof(std::int64_t)){
  p.record_failure();return luaL_error(L,"WorldView Player turn read reference storage differs");
 }
 // Validate the exact retained metatable without a throwing luaL_checkudata
 // path that a stock Lua pcall could catch without recording real failure.
 const bool has_metatable=lua_getmetatable(L,-1)!=0;
 if(!has_metatable){p.record_failure();return luaL_error(L,"WorldView Player turn read reference metatable absent");}
 luaL_getmetatable(L,"Civis.PlayerReference");const bool same_metatable=lua_rawequal(L,-1,-2);lua_pop(L,2);
 if(!same_metatable){p.record_failure();return luaL_error(L,"WorldView Player turn read foreign reference type");}
 const auto player=*static_cast<const std::int64_t*>(lua_touserdata(L,-1));lua_pop(L,1);
 if(player<0||player>=64){p.record_failure();return luaL_error(L,"WorldView Player turn read native ID differs");}
 lua_rawgeti(L,LUA_REGISTRYINDEX,lender->environment);lua_pushliteral(L,"Players");lua_rawget(L,-2);
 if(!lua_istable(L,-1)){p.record_failure();return luaL_error(L,"WorldView Player turn read actual Players publication absent");}
 lua_rawgeti(L,-1,static_cast<int>(player));const bool same_receiver=lua_rawequal(L,1,-1);lua_pop(L,3);
 if(!same_receiver){p.record_failure();return luaL_error(L,"WorldView Player turn read receiver is not same retained Players slot");}
 bool result=false;p.source_exception={};p.source_error=nullptr;
 try{
  lender->current();p.map.calculated_yield->require_current();lender->current();
  result=p.map.calculated_yield->player_is_turn_active(player);
  lender->current();p.map.calculated_yield->require_current();lender->current();
  lender->result=result?1:0;lender->query_returned=true;
 }catch(const std::exception& error){p.source_exception=std::current_exception();p.source_error=error.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView actual Player turn field query failed";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 lua_pushboolean(L,result);return 1;
}
static int worldview_player_turn_active(lua_State* L){
 auto& p=owner(L);p.source_exception={};p.source_error=nullptr;
 try{p.worldview_require_callback_origin();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView Player turn read actual native origin refused";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(!lua_checkstack(L,8)){p.record_failure();return luaL_error(L,"WorldView Player turn read protected stack unavailable");}
 const int args=lua_gettop(L);lua_pushcfunction(L,worldview_player_turn_active_body);lua_insert(L,1);
 int status=0;
 try{
  {WorldViewGameActiveReadScope lender(p,L);lender.provisional();
   status=lua_pcall(L,args,1,0);
   if(!status){
    lender.current();
    if(!lender.query_returned||lua_gettop(L)!=1||lua_type(L,1)!=LUA_TBOOLEAN||lua_toboolean(L,1)!=(lender.result!=0))
     throw ScriptError("WorldView Player turn publication differs from genuine bool field read");
    p.ready=lender.prior_ready;if(lender.prior_failed)p.failed.set_loader_sentinel();else p.failed=false;
    lender.provisional_active=false;lender.pure();lender.returned=true;
   }
  }
 }catch(const std::exception& error){p.source_exception=std::current_exception();p.source_error=error.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView Player protected native field lender refused";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(status)return lua_error(L);return 1;
}
static void worldview_install_player_turn_active(lua_State* L,int environment,std::string_view context){
 auto& p=owner(L);
 if(!p.worldview_native_host||context!="ActionInfoPanel")return;
 p.source_exception={};p.source_error=nullptr;
 try{
  p.worldview_require_callback_origin();const auto* const native=p.worldview_native_host;
  if(!p.calling||!p.map.calculated_yield||!p.end_turn_child_loader||p.end_turn_child_loader->entry!=L||!p.end_turn_child_loader->current()||
   !native||native->child>=p.child_contexts.size()||!p.worldview_native_child_controls_current(native->child)||
   p.end_turn_child_loader->child!=native->child||p.child_contexts[native->child].name!=context)
   throw ScriptError("WorldView Player turn installation lacks genuine native ActionInfoPanel child");
 }catch(...){p.source_exception=std::current_exception();p.source_error="WorldView Player turn installation source origin refused";}
 if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
 lua_pushliteral(L,"Player");lua_rawget(L,environment);
 if(!lua_istable(L,-1)){p.record_failure();return (void)luaL_error(L,"WorldView Player turn installation actual Player table absent");}
 lua_pushliteral(L,"IsTurnActive");lua_rawget(L,-2);
 if(!lua_isnil(L,-1)){p.record_failure();return (void)luaL_error(L,"WorldView Player turn installation cannot replace existing API");}
 lua_pop(L,1);lua_pushliteral(L,"IsTurnActive");lua_pushcfunction(L,worldview_player_turn_active);lua_rawset(L,-3);lua_pop(L,1);
 p.source_exception={};p.source_error=nullptr;
 try{p.worldview_require_callback_origin();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView Player turn installation source changed at publication";}
 if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
}
