// Included inside SAME LuaScriptHost::Impl after original owned callbacks.
// Each Lua allocator runs only with POD locals and registry storage already in
// Impl. The outer native caller owns all C++ lenders across lua_cpcall.
static int worldview_created_error(lua_State*L,const char*message){
 auto&p=owner(L);p.record_failure();if(p.end_turn_lifetime)p.end_turn_lifetime->failed=true;
 return luaL_error(L,"%s",message);
}
static GraphControl*worldview_created_graph_control_without_lua_calls(lua_State*L,int index,std::size_t slot){
 if(lua_type(L,index)!=LUA_TUSERDATA||lua_objlen(L,index)!=sizeof(GraphControl)||!lua_getmetatable(L,index)){
  worldview_created_error(L,"Returned actual canonical control userdata absent");return nullptr;}
 lua_pushliteral(L,"__civis_control_tag");lua_rawget(L,-2);lua_pushlightuserdata(L,&owner(L).control_graphs);
 const bool tag=lua_rawequal(L,-1,-2);lua_pop(L,3);
 const auto*value=static_cast<const GraphControl*>(lua_touserdata(L,index));
 if(!tag||!value||value->slot!=slot||slot>=owner(L).control_graphs.size()){
  worldview_created_error(L,"Returned actual canonical control marker/graph differs");return nullptr;}
 return static_cast<GraphControl*>(lua_touserdata(L,index));
}
static int worldview_created_upvalue(lua_State*L,int function,const char*key){
 for(int i=1;;++i){const char*name=lua_getupvalue(L,function,i);if(!name)break;
  if(std::strcmp(name,key)==0)return lua_gettop(L);lua_pop(L,1);}
 return 0;
}
static bool worldview_created_line(lua_State*L,int function,int line){
 if(!lua_isfunction(L,function)||lua_iscfunction(L,function))return false;
 const auto*c=static_cast<const Closure*>(lua_topointer(L,function));return c->l.p&&c->l.p->linedefined==line;
}
void worldview_created_owner(std::size_t index,const WorldViewCallbackFrame&frame){
 worldview_callback_fence(frame);
 if(index>=worldview_created_registrations.size())throw ScriptError("Original created listener occurrence absent");
 const auto&e=worldview_created_registrations[index];
 const auto pure=[&]{
  if(e.retired||e.epoch!=failure_epoch||e.uncertainty!=execution_uncertainty||
   e.child>=child_contexts.size()||e.compiled>=graph_compiled_sources.size()||e.graph_slot>=control_graphs.size())
   throw ScriptError("Original created listener source retired");
  const auto&c=child_contexts[e.child];const auto&g=control_graphs[e.graph_slot];
  const auto graph=e.graph.lock();
  const auto same=[](const auto&a,const auto&b){return !a.owner_before(b)&&!b.owner_before(a);};
  if(!graph||graph.get()!=g.graph.get()||!same(e.graph,g.graph)||!g.bound||!graph->backend_||
   c.retired||c.name!="UnitFlagManager"||!c.source||c.source.get()!=e.source.get()||!same(c.source,e.source)||
   c.environment!=e.environment||c.execution_source!=e.compiled||graph->context()!=c.name||
   graph_compiled_sources[e.compiled].root!=e.root||graph_compiled_sources[e.compiled].retained==LUA_NOREF||
   graph_compiled_sources[e.compiled].context!=c.name||!worldview_native_child_controls_current(e.child))
   throw ScriptError("Original created listener SAME graph/Proto/child changed");
 };
 pure();worldview_operation->require_same_vm(*worldview_host_owner);worldview_callback_fence(frame);pure();
 worldview_operation->require_same_source(*e.source);worldview_callback_fence(frame);pure();
 e.source->require_current();worldview_callback_fence(frame);pure();
 child_require_current(e.child);worldview_callback_fence(frame);pure();
}
static int worldview_created_add(lua_State*L){
 auto&self=owner(L);self.source_exception={};self.source_error=nullptr;
 const auto child=static_cast<std::size_t>(lua_tointeger(L,lua_upvalueindex(1)));
 std::size_t compiled=graph_no_owner,slot=graph_no_owner;bool same_caller=false;
 if(lua_gettop(L)!=1||!worldview_created_line(L,1,954))return worldview_created_error(L,"Original OnUnitCreated closure required");
 try{self.worldview_require_callback_origin();
  if(!self.calling||!self.end_turn_child_loader||!self.end_turn_child_loader->current()||
   self.end_turn_child_loader->child!=child||child>=self.child_contexts.size()||
   self.child_contexts[child].name!="UnitFlagManager"||!self.child_contexts[child].loading||
   self.child_contexts[child].loaded||self.child_contexts[child].retired||
   !self.worldview_created_registrations.empty())throw ScriptError("Original created registration lacks unique actual loader");
  for(std::size_t i=0;i<self.control_graphs.size();++i)if(self.control_graphs[i].graph&&
   self.control_graphs[i].graph->context()=="UnitFlagManager"){if(slot!=graph_no_owner)throw ScriptError("Created graph duplicated");slot=i;}
 }catch(const std::exception&e){self.source_exception=std::current_exception();self.source_error=e.what();}
 catch(...){self.source_exception=std::current_exception();self.source_error="Created registration origin failed";}
 if(self.source_error)return worldview_created_error(L,self.source_error);
 if(slot==graph_no_owner||!lua_checkstack(L,12))return worldview_created_error(L,"Original created graph/stack absent");
 lua_Debug caller{};
 for(int level=1;level<=256&&lua_getstack(L,level,&caller);++level){
  if(!lua_getinfo(L,"f",&caller))break;if(lua_iscfunction(L,-1)){lua_pop(L,1);continue;}
  compiled=self.graph_compiled_owner(L,-1);
  lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[child].environment);
  same_caller=lua_rawequal(L,-1,-2);lua_pop(L,3);break;
 }
 if(!same_caller||compiled==graph_no_owner||compiled!=self.graph_execution_scope||
  self.child_contexts[child].execution_source!=compiled||self.graph_compiled_owner(L,1)!=compiled)
  return worldview_created_error(L,"Created listener first original Lua caller differs");
 lua_getfenv(L,1);lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[child].environment);
 const bool environment=lua_rawequal(L,-1,-2);const void*environment_identity=lua_topointer(L,-1);lua_pop(L,2);
 if(!environment)return worldview_created_error(L,"Created listener original environment differs");
 // Append before the first luaL_ref. An OOM leaves a retained partial entry
 // for retirement; no transient RAII survives an allocating Lua API.
 try{self.worldview_created_registrations.emplace_back();auto&e=self.worldview_created_registrations.back();
  e.source=self.child_contexts[child].source;e.graph=self.control_graphs[slot].graph;e.graph_slot=slot;
  e.child=child;e.compiled=compiled;e.root=self.graph_compiled_sources[compiled].root;
  e.environment=self.child_contexts[child].environment;e.environment_identity=environment_identity;
  e.closure=lua_topointer(L,1);e.registered=L;e.epoch=self.failure_epoch;e.uncertainty=self.execution_uncertainty;
 }catch(const std::exception&e){self.source_exception=std::current_exception();self.source_error=e.what();}
 catch(...){self.source_exception=std::current_exception();self.source_error="Created registry storage allocation failed";}
 if(self.source_error)return worldview_created_error(L,self.source_error);
 auto&e=self.worldview_created_registrations.back();
 lua_pushvalue(L,1);e.function=luaL_ref(L,LUA_REGISTRYINDEX);
 lua_pushthread(L);e.thread=luaL_ref(L,LUA_REGISTRYINDEX);
 const int klass=worldview_created_upvalue(L,1,"g_UnitFlagClass");
 if(!klass||!lua_istable(L,klass))return worldview_created_error(L,"Original actual UnitFlag class upvalue absent");
 e.class_identity=lua_topointer(L,klass);lua_pushvalue(L,klass);e.klass=luaL_ref(L,LUA_REGISTRYINDEX);
 lua_pushliteral(L,"new");lua_rawget(L,klass);
 if(!worldview_created_line(L,-1,73)||self.graph_compiled_owner(L,-1)!=compiled)
  return worldview_created_error(L,"Original UnitFlag.new73 closure absent");
 e.new_identity=lua_topointer(L,-1);lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,e.environment);
 const bool new_environment=lua_rawequal(L,-1,-2);lua_pop(L,2);
 if(!new_environment)return worldview_created_error(L,"Original UnitFlag.new environment differs");
 const int new_function=lua_gettop(L);const int master=worldview_created_upvalue(L,new_function,"g_MasterList");
 if(!master||!lua_istable(L,master))return worldview_created_error(L,"Original actual master list upvalue absent");
 e.master_identity=lua_topointer(L,master);e.master=luaL_ref(L,LUA_REGISTRYINDEX);
 e.new_function=luaL_ref(L,LUA_REGISTRYINDEX);
 lua_pushliteral(L,"Initialize");lua_rawget(L,klass);
 if(!worldview_created_line(L,-1,149)||self.graph_compiled_owner(L,-1)!=compiled)
  return worldview_created_error(L,"Original Initialize149 closure absent");
 e.initialize_identity=lua_topointer(L,-1);lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,e.environment);
 const bool init_environment=lua_rawequal(L,-1,-2);lua_pop(L,2);
 if(!init_environment)return worldview_created_error(L,"Original Initialize environment differs");
 e.initialize=luaL_ref(L,LUA_REGISTRYINDEX);lua_pop(L,1);
 unitflag_capture_constructor_managers(L,self.worldview_created_registrations.size()-1);
 lua_pushvalue(L,1);return 1;
}
static int worldview_created_count(lua_State*L){
 auto&self=owner(L);
 if(self.worldview_created_registrations.size()!=1||self.worldview_created_registrations.front().retired)
  return worldview_created_error(L,"Original created listener not retained");
 lua_pushinteger(L,1);return 1;
}
static int worldview_created_unsupported(lua_State*L){
 owner(L).record_failure();return worldview_created_error(L,"Original created event mutation lacks native owned lifetime");
}
static int worldview_created_event(lua_State*L){
 auto&self=owner(L);
 if(!self.worldview_native_host||lua_type(L,1)!=LUA_TSTRING){lua_pushnil(L);return 1;}
 std::size_t n=0;const char*context=lua_tolstring(L,1,&n);
 if(n!=15||std::memcmp(context,"UnitFlagManager",15)!=0){lua_pushnil(L);return 1;}
 const auto child=self.worldview_native_host->child;
 self.source_exception={};self.source_error=nullptr;
 try{self.worldview_require_callback_origin();
  if(child>=self.child_contexts.size()||self.child_contexts[child].name!="UnitFlagManager"||
   !self.calling||!self.child_contexts[child].loading||self.child_contexts[child].loaded)
   throw ScriptError("Original created event lacks actual selected child loader");
 }catch(const std::exception&e){self.source_exception=std::current_exception();self.source_error=e.what();}
 catch(...){self.source_exception=std::current_exception();self.source_error="Created event factory origin failed";}
 if(self.source_error)return worldview_created_error(L,self.source_error);
 lua_newtable(L);lua_pushinteger(L,static_cast<lua_Integer>(child));lua_pushcclosure(L,worldview_created_add,1);
 lua_setfield(L,-2,"Add");lua_pushcfunction(L,worldview_created_count);lua_setfield(L,-2,"Count");
 lua_pushcfunction(L,worldview_created_unsupported);lua_setfield(L,-2,"Remove");
 lua_pushcfunction(L,worldview_created_unsupported);lua_setfield(L,-2,"RemoveAll");return 1;
}
static void worldview_created_registry_validate(lua_State*L,std::size_t index){
 auto&self=owner(L);const int top=lua_gettop(L);
 if(index>=self.worldview_created_registrations.size())worldview_created_error(L,"Created registry occurrence absent");
 const auto&e=self.worldview_created_registrations[index];
 if(e.retired||e.registered!=L||e.child>=self.child_contexts.size()||e.compiled>=self.graph_compiled_sources.size()||
  e.function<=0||e.thread<=0||e.environment<=0||e.klass<=0||e.new_function<=0||e.initialize<=0||e.master<=0||
  !lua_checkstack(L,16))worldview_created_error(L,"Created durable registry references incomplete");
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.function);const int callback=lua_gettop(L);
 if(lua_topointer(L,callback)!=e.closure||!worldview_created_line(L,callback,954)||
  self.graph_compiled_owner(L,callback)!=e.compiled)worldview_created_error(L,"Created exact original callback replaced");
 lua_getfenv(L,callback);lua_rawgeti(L,LUA_REGISTRYINDEX,e.environment);
 const bool callback_env=lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==e.environment_identity;lua_pop(L,2);
 if(!callback_env)worldview_created_error(L,"Created exact original callback environment changed");
 const int actual_class=worldview_created_upvalue(L,callback,"g_UnitFlagClass");lua_rawgeti(L,LUA_REGISTRYINDEX,e.klass);
 if(!actual_class||!lua_istable(L,actual_class)||!lua_rawequal(L,actual_class,-1)||
  lua_topointer(L,actual_class)!=e.class_identity)worldview_created_error(L,"Created exact actual class changed");
 lua_pop(L,1);lua_pushliteral(L,"new");lua_rawget(L,actual_class);const int actual_new=lua_gettop(L);
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.new_function);
 if(!lua_rawequal(L,actual_new,-1)||lua_topointer(L,actual_new)!=e.new_identity||
  !worldview_created_line(L,actual_new,73)||self.graph_compiled_owner(L,actual_new)!=e.compiled)
  worldview_created_error(L,"Created exact original new/root changed");
 lua_pop(L,1);lua_getfenv(L,actual_new);lua_rawgeti(L,LUA_REGISTRYINDEX,e.environment);
 const bool new_env=lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==e.environment_identity;lua_pop(L,2);
 if(!new_env)worldview_created_error(L,"Created exact original new environment changed");
 const int actual_master=worldview_created_upvalue(L,actual_new,"g_MasterList");lua_rawgeti(L,LUA_REGISTRYINDEX,e.master);
 if(!actual_master||!lua_istable(L,actual_master)||!lua_rawequal(L,actual_master,-1)||
  lua_topointer(L,actual_master)!=e.master_identity)worldview_created_error(L,"Created exact actual master upvalue changed");
 lua_pop(L,2);lua_pushliteral(L,"Initialize");lua_rawget(L,actual_class);const int actual_init=lua_gettop(L);
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.initialize);
 if(!lua_rawequal(L,actual_init,-1)||lua_topointer(L,actual_init)!=e.initialize_identity||
  !worldview_created_line(L,actual_init,149)||self.graph_compiled_owner(L,actual_init)!=e.compiled)
  worldview_created_error(L,"Created exact original Initialize/root changed");
 lua_pop(L,1);lua_getfenv(L,actual_init);lua_rawgeti(L,LUA_REGISTRYINDEX,e.environment);
 const bool init_env=lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==e.environment_identity;lua_pop(L,2);
 if(!init_env)worldview_created_error(L,"Created exact original Initialize environment changed");
 lua_settop(L,top);
}
struct WorldViewCreatedJob {
 Impl*owner;std::size_t index,receipt;
 std::int32_t player,full_id,fog;bool not_invisible,invoked=false,returned=false;
};
static int worldview_created_protected(lua_State*L){
 auto&job=*static_cast<WorldViewCreatedJob*>(lua_touserdata(L,1));auto&self=*job.owner;
 if(&owner(L)!=&self||job.index>=self.worldview_created_registrations.size()||
  job.receipt>=self.worldview_created_instances.size())return worldview_created_error(L,"Created protected same VM/storage lost");
 const auto&e=self.worldview_created_registrations[job.index];
 worldview_created_registry_validate(L,job.index);
 if(e.retired||e.registered!=L||!self.child_contexts[e.child].loaded||self.child_contexts[e.child].loading||
  !lua_checkstack(L,24)||lua_status(L))return worldview_created_error(L,"Original created registered state unavailable");
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.thread);
 const bool thread=lua_isthread(L,-1)&&lua_tothread(L,-1)==L;lua_pop(L,1);
 if(!thread)return worldview_created_error(L,"Created registered coroutine changed");
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.function);
 if(lua_topointer(L,-1)!=e.closure||!worldview_created_line(L,-1,954)||
  self.graph_compiled_owner(L,-1)!=e.compiled)return worldview_created_error(L,"Created original listener/Proto changed");
 lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,e.environment);
 const bool env=lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==e.environment_identity;lua_pop(L,2);
 if(!env)return worldview_created_error(L,"Created original environment changed");
 const int callback=lua_gettop(L);const int klass=worldview_created_upvalue(L,callback,"g_UnitFlagClass");
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.klass);
 const bool actual_class=klass&&lua_istable(L,klass)&&lua_rawequal(L,klass,-1)&&lua_topointer(L,klass)==e.class_identity;
 lua_pop(L,1);if(!actual_class)return worldview_created_error(L,"Created actual original class upvalue replaced");
 lua_pushliteral(L,"new");lua_rawget(L,klass);lua_rawgeti(L,LUA_REGISTRYINDEX,e.new_function);
 const bool actual_new=lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==e.new_identity&&worldview_created_line(L,-1,73);
 lua_pop(L,2);if(!actual_new)return worldview_created_error(L,"Created original constructor replaced");
 lua_pushliteral(L,"Initialize");lua_rawget(L,klass);lua_rawgeti(L,LUA_REGISTRYINDEX,e.initialize);
 const bool actual_init=lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==e.initialize_identity&&worldview_created_line(L,-1,149);
 lua_pop(L,2);if(!actual_init)return worldview_created_error(L,"Created original Initialize replaced");
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.new_function);const int master=worldview_created_upvalue(L,lua_gettop(L),"g_MasterList");
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.master);
 const bool actual_master=master&&lua_istable(L,master)&&lua_rawequal(L,master,-1)&&lua_topointer(L,master)==e.master_identity;
 lua_pop(L,1);if(!actual_master)return worldview_created_error(L,"Created original master list replaced");
 lua_pushinteger(L,job.player);lua_rawget(L,master);
 if(lua_istable(L,-1)){lua_pushinteger(L,job.full_id);lua_rawget(L,-2);
  if(!lua_isnil(L,-1))return worldview_created_error(L,"Original constructor full-ID already published; replay refused");lua_pop(L,1);}
 else if(!lua_isnil(L,-1))return worldview_created_error(L,"Original master owner table malformed");
 lua_settop(L,callback);child_require_bindings(L);self.graph_execution_scope=e.compiled;
 lua_pushinteger(L,job.player);lua_pushinteger(L,job.full_id);
 // These selected original Proto954 operands are unused. They carry no
 // asserted native coordinates, type/color/selection/military evidence.
 for(int i=3;i<=9;++i)lua_pushnil(L);
 lua_pushinteger(L,job.fog);lua_pushnil(L);lua_pushnil(L);lua_pushboolean(L,job.not_invisible);
 if(self.worldview_unitflag_constructor_job)return worldview_created_error(L,"Original constructor scope reentry");
 self.worldview_unitflag_constructor_job=&job;
 job.invoked=true;lua_call(L,13,0);job.returned=true;
 self.worldview_unitflag_constructor_job=nullptr;
 worldview_created_registry_validate(L,job.index);
 // This happens only after the real original listener has normally returned.
 // A partially published g_MasterList inside new cannot pass this point.
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.master);lua_pushinteger(L,job.player);lua_rawget(L,-2);
 if(!lua_istable(L,-1))return worldview_created_error(L,"Original created return has no actual player master table");
 lua_pushinteger(L,job.full_id);lua_rawget(L,-2);
 if(!lua_istable(L,-1))return worldview_created_error(L,"Original created skip/missing flag has no completed health receipt");
 const int flag=lua_gettop(L);auto&r=self.worldview_created_instances[job.receipt];
 lua_pushliteral(L,"m_PlayerID");lua_rawget(L,flag);
 const bool player=lua_type(L,-1)==LUA_TNUMBER&&lua_tonumber(L,-1)==job.player;lua_pop(L,1);
 lua_pushliteral(L,"m_UnitID");lua_rawget(L,flag);
 const bool unit=lua_type(L,-1)==LUA_TNUMBER&&lua_tonumber(L,-1)==job.full_id;lua_pop(L,1);
 if(!player||!unit)return worldview_created_error(L,"Returned actual flag full-ID differs");
 if(!lua_getmetatable(L,flag))return worldview_created_error(L,"Original returned flag class absent");
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.klass);const bool meta=lua_rawequal(L,-1,-2);lua_pop(L,2);
 if(!meta)return worldview_created_error(L,"Returned flag actual original class differs");
 r.instance_identity=lua_topointer(L,flag);lua_pushvalue(L,flag);r.instance=luaL_ref(L,LUA_REGISTRYINDEX);
 lua_pushliteral(L,"m_Instance");lua_rawget(L,flag);
 if(!lua_istable(L,-1))return worldview_created_error(L,"Returned flag actual controls table missing");
 const int controls=lua_gettop(L);r.controls_identity=lua_topointer(L,controls);
 lua_pushvalue(L,controls);r.controls=luaL_ref(L,LUA_REGISTRYINDEX);
 for(std::size_t i=0;i<self.graph_instances.size();++i){const auto&g=self.graph_instances[i];
  if(g.slot!=e.graph_slot||g.table==LUA_NOREF)continue;lua_rawgeti(L,LUA_REGISTRYINDEX,g.table);
  const bool actual=lua_rawequal(L,-1,controls);lua_pop(L,1);
  if(actual){if(r.graph_instance!=graph_no_owner)return worldview_created_error(L,"Returned canonical instance duplicated");r.graph_instance=i;}}
 if(r.graph_instance==graph_no_owner)return worldview_created_error(L,"Returned actual instance not built by canonical graph");
 r.instance_root=self.graph_instances[r.graph_instance].root;
 if(!r.instance_root)return worldview_created_error(L,"Returned canonical health instance root absent");
 const char*names[]={"HealthBarBG","HealthBar","HealthBarButton","NormalButton","NormalSelect","HealthBarSelect"};
 for(std::size_t i=0;i<6;++i){lua_pushstring(L,names[i]);lua_rawget(L,controls);
  const auto*c=worldview_created_graph_control_without_lua_calls(L,lua_gettop(L),e.graph_slot);r.health_ids[i]=c->id;
  for(std::size_t k=0;k<i;++k)if(r.health_ids[k]==r.health_ids[i])return worldview_created_error(L,"Returned health control alias");lua_pop(L,1);}
 lua_pushliteral(L,"m_CargoControls");lua_rawget(L,flag);
 if(!lua_isnil(L,-1)){if(!lua_istable(L,-1))return worldview_created_error(L,"Returned actual Cargo controls malformed");
  const int actual_cargo=lua_gettop(L);
  for(std::size_t i=0;i<self.graph_instances.size();++i){const auto&g=self.graph_instances[i];
   if(g.slot!=e.graph_slot||g.table==LUA_NOREF)continue;lua_rawgeti(L,LUA_REGISTRYINDEX,g.table);
   const bool same=lua_rawequal(L,-1,actual_cargo);lua_pop(L,1);
   if(same){if(r.cargo_graph_instance!=graph_no_owner)return worldview_created_error(L,"Returned Cargo canonical instance duplicated");r.cargo_graph_instance=i;}}
  if(r.cargo_graph_instance==graph_no_owner)return worldview_created_error(L,"Returned Cargo not built by canonical graph");
  r.cargo_instance_root=self.graph_instances[r.cargo_graph_instance].root;
  if(!r.cargo_instance_root)return worldview_created_error(L,"Returned Cargo actual built root absent");
  r.cargo_identity=lua_topointer(L,-1);lua_pushvalue(L,-1);r.cargo=luaL_ref(L,LUA_REGISTRYINDEX);
  lua_pushliteral(L,"PullDown");lua_rawget(L,-2);r.cargo_pull_down=worldview_created_graph_control_without_lua_calls(L,lua_gettop(L),e.graph_slot)->id;lua_pop(L,1);}
 lua_pop(L,1);return 0;
}
void worldview_created_accept(std::size_t index,std::size_t receipt,const WorldViewCallbackFrame&frame){
 worldview_created_owner(index,frame);
 if(receipt>=worldview_created_instances.size())throw ScriptError("Created return receipt storage lost");
 auto&r=worldview_created_instances[receipt];const auto&e=worldview_created_registrations[index];
 if(r.retired||r.original_returned||r.registration!=index||r.owner<0||r.full_id<0||
  r.instance<=0||r.controls<=0||!r.instance_identity||!r.controls_identity||r.graph_instance>=graph_instances.size()||
  graph_instances[r.graph_instance].slot!=e.graph_slot||graph_instances[r.graph_instance].table<=0||
  !r.instance_root||graph_instances[r.graph_instance].root!=r.instance_root)
  throw ScriptError("Created original return lacks retained actual full-ID instance");
 // A retained returned record is DATA until a fresh Stock/member Source
 // borrower proves the live full-ID unit and canonical controls again.
 r.original_returned=true;
}
void worldview_retire_created_registrations()noexcept{
 if(ui_thread!=std::this_thread::get_id())std::terminate();
 for(auto&e:worldview_created_registrations)e.retired=true;
 for(auto&r:worldview_created_instances)r.retired=true;
}
static int worldview_release_retired_created(lua_State*L){
 auto&self=owner(L);if(self.ui_thread!=std::this_thread::get_id())return worldview_created_error(L,"Created registry cleanup foreign creator");
 for(auto&e:self.worldview_created_registrations)if(e.retired){
  int*refs[]={&e.function,&e.thread,&e.klass,&e.new_function,&e.initialize,&e.master,
   &e.constructor_managers[0],&e.constructor_managers[1],&e.constructor_managers[2],&e.constructor_manager_class,
   &e.constructor_manager_methods[0],&e.constructor_manager_methods[1]};
  for(int*p:refs){const int ref=*p;*p=LUA_NOREF;if(ref!=LUA_NOREF)luaL_unref(L,LUA_REGISTRYINDEX,ref);}}
 for(auto&r:self.worldview_created_instances)if(r.retired){int*refs[]={&r.instance,&r.controls,&r.cargo};
  for(int*p:refs){const int ref=*p;*p=LUA_NOREF;if(ref!=LUA_NOREF)luaL_unref(L,LUA_REGISTRYINDEX,ref);}}
 return 0;
}

#include "lua_worldview_unitflag_inflight_constructor.inc"
