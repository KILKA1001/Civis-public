// Private actual WorldView Game getter lender. The legacy getter remains
// unchanged outside this native operation. All allocating Lua work occurs
// inside pcall while the pre-sentinel C++ lender lives outside that boundary.
struct WorldViewGameActiveReadScope;
WorldViewGameActiveReadScope* worldview_game_active_read_scope=nullptr;
struct WorldViewGameActiveReadScope final {
 Impl& p;lua_State* const entry;
 WorldViewNativeHostScope* const native;
 const BnwWorldViewSourceOperationLoan* const loan;
 const std::uint64_t epoch,writes,uncertainty,serial;
 const std::size_t child;
 const std::weak_ptr<const ScriptUiChildSource> source;
 const ScriptUiChildSource* const source_get;
 const std::weak_ptr<const BnwLuaCalculatedPlotYieldOwner> game;
 const BnwLuaCalculatedPlotYieldOwner* const game_get;
 WorldViewLoadedChildCallScope* const loaded_call;
 const bool loaded_calling;
 EndTurnChildLoaderScope* const loader;
 const std::size_t loader_attempt;
 lua_State* const loader_entry;
 const std::size_t graph_scope,execution_source;
 const int environment,execution_thread;
 const bool prior_failed,prior_ready;
 bool provisional_active=false,query_returned=false,returned=false;
 std::int32_t result=-1;
 WorldViewGameActiveReadScope(Impl& owner,lua_State* L):p(owner),entry(L),native(p.worldview_native_host),loan(p.worldview_operation),
  epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty),serial(native?native->serial:0),
  child(native?native->child:graph_no_owner),
  source(child<p.child_contexts.size()?p.child_contexts[child].source:std::shared_ptr<const ScriptUiChildSource>{}),
  source_get(child<p.child_contexts.size()?p.child_contexts[child].source.get():nullptr),game(p.map.calculated_yield),game_get(p.map.calculated_yield.get()),
  loaded_call(p.worldview_loaded_child_call),loaded_calling(p.calling),
  loader(p.end_turn_child_loader),loader_attempt(loader?loader->attempt:graph_no_owner),loader_entry(loader?loader->entry:nullptr),graph_scope(p.graph_execution_scope),
  execution_source(child<p.child_contexts.size()?p.child_contexts[child].execution_source:graph_no_owner),
  environment(child<p.child_contexts.size()?p.child_contexts[child].environment:LUA_NOREF),
  execution_thread(child<p.child_contexts.size()?p.child_contexts[child].execution_thread:LUA_NOREF),
  prior_failed(p.failed),prior_ready(p.ready){
  p.worldview_require_callback_origin();
  if(loaded_call)loaded_call->current();
  if(!entry||p.worldview_game_active_read_scope||!native||!loan||child>=p.child_contexts.size()||
   !p.worldview_native_child_controls_current(child)||!game_get||p.child_contexts[child].retired||
   (loaded_call?(loaded_call->child!=child||loaded_call->registered!=entry||loaded_call->compiled!=graph_scope||
    !p.child_contexts[child].loaded||p.child_contexts[child].loading||loader):
    (!p.calling||!loader||!loader->current()||loader->child!=child||!p.child_contexts[child].loading||p.child_contexts[child].loaded))||
   graph_scope==graph_no_owner||graph_scope>=p.graph_compiled_sources.size()||execution_source==graph_no_owner||
   execution_source>=p.graph_compiled_sources.size()||environment==LUA_NOREF||execution_thread==LUA_NOREF||
   p.graph_compiled_sources[graph_scope].context!=p.child_contexts[child].name||
   p.graph_compiled_sources[execution_source].context!=p.child_contexts[child].name)
   throw ScriptError("WorldView Game getter lacks actual native pre-sentinel child/source lender");
  p.worldview_game_active_read_scope=this;
 }
 void pure()const{
  if(!native||native!=p.worldview_native_host)throw ScriptError("WorldView Game getter native scope retired");
  native->pure();
  if(p.worldview_game_active_read_scope!=this||native!=p.worldview_native_host||loan!=p.worldview_operation||
   (loaded_call?p.calling!=loaded_calling:!p.calling)||p.failure_epoch!=epoch||p.failed.writes_!=writes||p.failed.exhausted_||p.execution_uncertainty!=uncertainty||
   static_cast<bool>(p.failed)!=(provisional_active?true:prior_failed)||p.ready!=(provisional_active?false:prior_ready)||
   !native||native->serial!=serial||native->child!=child||child>=p.child_contexts.size()||p.child_contexts[child].retired||
   source.expired()||source_get!=p.child_contexts[child].source.get()||
   source.owner_before(p.child_contexts[child].source)||p.child_contexts[child].source.owner_before(source)||
   game.expired()||game_get!=p.map.calculated_yield.get()||game.owner_before(p.map.calculated_yield)||p.map.calculated_yield.owner_before(game)||
   (loaded_call?(p.worldview_loaded_child_call!=loaded_call||loader!=p.end_turn_child_loader||loader||
    !p.child_contexts[child].loaded||p.child_contexts[child].loading):
    (loader!=p.end_turn_child_loader||!loader->current()||loader->attempt!=loader_attempt||loader->entry!=loader_entry||
    loader->child!=child||!p.child_contexts[child].loading||p.child_contexts[child].loaded))||
   graph_scope!=p.graph_execution_scope||execution_source!=p.child_contexts[child].execution_source||
   environment!=p.child_contexts[child].environment||execution_thread!=p.child_contexts[child].execution_thread||
   !p.worldview_native_child_controls_current(child))
   throw ScriptError("WorldView Game getter actual lender/failure/source changed");
  if(loaded_call)loaded_call->pure();
  native->pure();
 }
 void current()const{pure();if(loaded_call){loaded_call->current();pure();}p.worldview_require_callback_origin();pure();}
 void provisional(){current();p.failed.set_loader_sentinel();p.ready=false;provisional_active=true;pure();}
 void accept_publication(lua_State* L){
  current();
  if(L!=entry||!query_returned||lua_gettop(L)!=1||lua_type(L,1)!=LUA_TNUMBER||lua_tonumber(L,1)!=static_cast<lua_Number>(result))
   throw ScriptError("WorldView Game getter publication differs from actual native query");
  // Only this unchanged, genuinely returned pcall restores its own boolean
  // marker. No real writer counter, epoch, exhaustion or uncertainty changes.
  p.ready=prior_ready;
  if(prior_failed)p.failed.set_loader_sentinel();else p.failed=false;
  provisional_active=false;
  pure();returned=true;
 }
 ~WorldViewGameActiveReadScope(){
  if(p.worldview_game_active_read_scope!=this||!returned)p.record_failure();
  p.worldview_game_active_read_scope=nullptr;
 }
 WorldViewGameActiveReadScope(const WorldViewGameActiveReadScope&)=delete;
 WorldViewGameActiveReadScope& operator=(const WorldViewGameActiveReadScope&)=delete;
};
static int worldview_game_active_read_body(lua_State* L){
 auto& p=owner(L);auto* const lender=p.worldview_game_active_read_scope;
 if(!lender||lender->entry!=L){p.record_failure();return luaL_error(L,"WorldView Game getter has no actual protected lender");}
 // First actual Lua caller must be the retained selected child/helper Proto,
 // on its actual execution coroutine and exact child environment table.
 if(lua_type(L,lua_upvalueindex(1))!=LUA_TNUMBER){p.record_failure();return luaL_error(L,"WorldView Game selector type differs");}
 const auto op=lua_tointeger(L,lua_upvalueindex(1));
 if(op!=0&&op!=1){p.record_failure();return luaL_error(L,"WorldView Game selector domain differs");}
 lua_Debug frame{};std::size_t compiled=graph_no_owner;int closure=0;
 for(int level=1;lua_getstack(L,level,&frame);++level){
  if(!lua_getinfo(L,"f",&frame))break;
  if(lua_iscfunction(L,-1)){lua_pop(L,1);continue;}
  compiled=p.graph_compiled_owner(L,-1);closure=lua_gettop(L);break;
 }
 const auto child=lender->child;
 if(compiled==graph_no_owner||compiled>=p.graph_compiled_sources.size()||child>=p.child_contexts.size()||p.child_contexts[child].environment==LUA_NOREF||
  p.child_contexts[child].execution_thread==LUA_NOREF||p.graph_compiled_sources[compiled].context!=p.child_contexts[child].name){
  p.record_failure();return luaL_error(L,"WorldView Game getter caller is not actual retained selected Proto");}
 if(lender->loaded_call&&(compiled!=lender->loaded_call->compiled||
   p.graph_compiled_sources[compiled].root!=lender->loaded_call->root)){
  p.record_failure();return luaL_error(L,"WorldView loaded Game caller differs from issued original callback Proto");}
 lua_getfenv(L,closure);lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[child].environment);
 const bool same_environment=lua_rawequal(L,-1,-2);lua_pop(L,2);
 lua_rawgeti(L,LUA_REGISTRYINDEX,lender->loaded_call?lender->loaded_call->thread:p.child_contexts[child].execution_thread);
 const bool same_thread=lua_isthread(L,-1)&&lua_tothread(L,-1)==L;lua_pop(L,2); // retained thread and caller closure
 if(!same_environment||!same_thread){p.record_failure();return luaL_error(L,"WorldView Game getter actual caller environment/thread changed");}
 p.source_exception={};p.source_error=nullptr;std::int32_t result=-1;
 try{
  lender->current();
  p.map.calculated_yield->require_current();lender->current();
  result=op==0?p.map.calculated_yield->active_player():p.map.calculated_yield->active_team();
  lender->current();p.map.calculated_yield->require_current();lender->current();
  lender->result=result;lender->query_returned=true;
 }catch(const std::exception& error){p.source_exception=std::current_exception();p.source_error=error.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView actual Game getter native query failed";}
 // Every native temporary has unwound before Lua publication/longjmp. OOM
 // cannot earn a returned pcall: the outside C++ lender records real failure.
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 lua_pushinteger(L,result);return 1;
}
static int worldview_game_active_read(lua_State* L){
 auto& p=owner(L);p.source_exception={};p.source_error=nullptr;
 try{p.worldview_require_callback_origin();}
 catch(const std::exception& error){p.source_exception=std::current_exception();p.source_error=error.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView Game getter native origin refused";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(!lua_checkstack(L,6)){p.record_failure();return luaL_error(L,"WorldView Game getter protected stack unavailable");}
 const int args=lua_gettop(L);
 lua_pushvalue(L,lua_upvalueindex(1));lua_pushcclosure(L,worldview_game_active_read_body,1);lua_insert(L,1);
 int status=0;
 try{
  {WorldViewGameActiveReadScope lender(p,L);lender.provisional();
   status=lua_pcall(L,args,1,0);
   if(!status)lender.accept_publication(L);
  }
 }catch(const std::exception& error){p.source_exception=std::current_exception();p.source_error=error.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView protected Game getter lender refused";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(status)return lua_error(L);
 return 1;
}
