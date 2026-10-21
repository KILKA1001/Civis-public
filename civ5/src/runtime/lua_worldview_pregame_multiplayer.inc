// Source-backed PreGame.IsMultiplayerGame for actual initial ActionInfoPanel.
// Original EXE binding selects CvDllPreGame::isNetworkMultiplayerGame.
// Reuses the unchanged genuine Source3 protected lender and Source4 lineage.
static int worldview_pregame_multiplayer_body(lua_State* L){
 auto& p=owner(L);auto* const lender=p.worldview_game_active_read_scope;
 if(!lender||lender->entry!=L){p.record_failure();return luaL_error(L,"WorldView PreGame multiplayer read has no actual protected lender");}
 p.source_exception={};p.source_error=nullptr;
 try{lender->current();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView PreGame multiplayer read native origin refused";}
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
  p.record_failure();return luaL_error(L,"WorldView PreGame multiplayer read caller is not actual ActionInfoPanel Proto");
 }
 lua_getfenv(L,closure);lua_rawgeti(L,LUA_REGISTRYINDEX,lender->environment);
 const bool same_environment=lua_rawequal(L,-1,-2);lua_pop(L,2);
 lua_rawgeti(L,LUA_REGISTRYINDEX,lender->execution_thread);
 const bool same_thread=lua_isthread(L,-1)&&lua_tothread(L,-1)==L;lua_pop(L,2);
 if(!same_environment||!same_thread){p.record_failure();return luaL_error(L,"WorldView PreGame multiplayer read actual environment/thread changed");}
 bool result=false;p.source_exception={};p.source_error=nullptr;
 try{
  lender->current();p.map.calculated_yield->require_current();lender->current();
  result=p.map.calculated_yield->pregame_is_multiplayer_game();
  lender->current();p.map.calculated_yield->require_current();lender->current();
  lender->result=result?1:0;lender->query_returned=true;
 }catch(const std::exception& error){p.source_exception=std::current_exception();p.source_error=error.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView actual PreGame multiplayer field query failed";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 lua_pushboolean(L,result);return 1;
}
static int worldview_pregame_multiplayer(lua_State* L){
 auto& p=owner(L);p.source_exception={};p.source_error=nullptr;
 try{p.worldview_require_callback_origin();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView PreGame multiplayer read actual native origin refused";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(!lua_checkstack(L,8)){p.record_failure();return luaL_error(L,"WorldView PreGame multiplayer read protected stack unavailable");}
 const int args=lua_gettop(L);lua_pushcfunction(L,worldview_pregame_multiplayer_body);lua_insert(L,1);
 int status=0;
 try{
  {WorldViewGameActiveReadScope lender(p,L);lender.provisional();
   status=lua_pcall(L,args,1,0);
   if(!status){
    lender.current();
    if(!lender.query_returned||lua_gettop(L)!=1||lua_type(L,1)!=LUA_TBOOLEAN||lua_toboolean(L,1)!=(lender.result!=0))
     throw ScriptError("WorldView PreGame multiplayer publication differs from genuine bool field read");
    p.ready=lender.prior_ready;if(lender.prior_failed)p.failed.set_loader_sentinel();else p.failed=false;
    lender.provisional_active=false;lender.pure();lender.returned=true;
   }
  }
 }catch(const std::exception& error){p.source_exception=std::current_exception();p.source_error=error.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView Player protected native field lender refused";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(status)return lua_error(L);return 1;
}
static void worldview_install_pregame_multiplayer(lua_State* L,int environment,std::string_view context){
 auto& p=owner(L);
 if(!p.worldview_native_host||context!="ActionInfoPanel")return;
 p.source_exception={};p.source_error=nullptr;
 try{
  p.worldview_require_callback_origin();const auto* const native=p.worldview_native_host;
  if(!p.calling||!p.map.calculated_yield||!p.end_turn_child_loader||p.end_turn_child_loader->entry!=L||!p.end_turn_child_loader->current()||
   !native||native->child>=p.child_contexts.size()||!p.worldview_native_child_controls_current(native->child)||
   p.end_turn_child_loader->child!=native->child||p.child_contexts[native->child].name!=context)
   throw ScriptError("WorldView PreGame multiplayer installation lacks genuine native ActionInfoPanel child");
 }catch(...){p.source_exception=std::current_exception();p.source_error="WorldView PreGame multiplayer installation source origin refused";}
 if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
 lua_pushliteral(L,"PreGame");lua_rawget(L,environment);
 if(!lua_isnil(L,-1)){p.record_failure();return (void)luaL_error(L,"WorldView PreGame installation refuses a foreign existing namespace");}
 lua_pop(L,1);lua_newtable(L);
 lua_pushliteral(L,"IsMultiplayerGame");lua_rawget(L,-2);
 if(!lua_isnil(L,-1)){p.record_failure();return (void)luaL_error(L,"WorldView PreGame multiplayer installation cannot replace existing API");}
 lua_pop(L,1);lua_pushliteral(L,"IsMultiplayerGame");lua_pushcfunction(L,worldview_pregame_multiplayer);lua_rawset(L,-3);lua_pushliteral(L,"PreGame");lua_pushvalue(L,-2);lua_rawset(L,environment);lua_pop(L,1);
 p.source_exception={};p.source_error=nullptr;
 try{p.worldview_require_callback_origin();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView PreGame multiplayer installation source changed at publication";}
 if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
}
