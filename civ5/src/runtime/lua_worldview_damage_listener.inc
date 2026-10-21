// Dedicated original UnitFlag damage registration; no generic Events witness.
static int worldview_damage_add(lua_State*L){
 auto&self=owner(L);self.source_exception={};self.source_error=nullptr;
 const auto child=static_cast<std::size_t>(lua_tointeger(L,lua_upvalueindex(1)));
 std::size_t compiled=graph_no_owner;bool same_caller=false;
 if(lua_gettop(L)!=1||!worldview_created_line(L,1,1291))
  return worldview_created_error(L,"Exact original OnUnitSetDamage1291 closure required");
 try{self.worldview_require_callback_origin();
  if(!self.calling||!self.end_turn_child_loader||!self.end_turn_child_loader->current()||
   self.end_turn_child_loader->child!=child||child>=self.child_contexts.size()||
   self.child_contexts[child].name!="UnitFlagManager"||!self.child_contexts[child].loading||
   self.child_contexts[child].loaded||self.child_contexts[child].retired||
   self.worldview_created_registrations.size()!=1||self.worldview_created_registrations.front().retired||
   !self.worldview_damage_registrations.empty())
   throw ScriptError("Damage registration lacks actual unique original child/created registry");
 }catch(const std::exception&e){self.source_exception=std::current_exception();self.source_error=e.what();}
 catch(...){self.source_exception=std::current_exception();self.source_error="Damage loader source failed";}
 if(self.source_error)return worldview_created_error(L,self.source_error);
 if(!lua_checkstack(L,18))return worldview_created_error(L,"Damage registrar stack absent");
 lua_Debug caller{};
 for(int level=1;level<=256&&lua_getstack(L,level,&caller);++level){
  if(!lua_getinfo(L,"f",&caller))break;if(lua_iscfunction(L,-1)){lua_pop(L,1);continue;}
  compiled=self.graph_compiled_owner(L,-1);lua_getfenv(L,-1);
  lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[child].environment);
  same_caller=lua_rawequal(L,-1,-2);lua_pop(L,3);break;
 }
 const auto&e=self.worldview_created_registrations.front();
 if(!same_caller||compiled==graph_no_owner||compiled!=self.graph_execution_scope||
  compiled!=e.compiled||child!=e.child||self.graph_compiled_owner(L,1)!=compiled)
  return worldview_created_error(L,"Damage listener first original compiled caller differs");
 lua_getfenv(L,1);lua_rawgeti(L,LUA_REGISTRYINDEX,e.environment);
 const bool env=lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==e.environment_identity;lua_pop(L,2);
 if(!env)return worldview_created_error(L,"Damage original environment differs");
 const int master=worldview_created_upvalue(L,1,"g_MasterList");
 if(!master||!lua_istable(L,master))return worldview_created_error(L,"Damage original master upvalue absent");
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.master);
 const bool same_master=lua_rawequal(L,-1,master)&&lua_topointer(L,master)==e.master_identity;lua_pop(L,2);
 if(!same_master)return worldview_created_error(L,"Damage master differs from original constructor registry");
 try{self.worldview_damage_registrations.emplace_back();auto&r=self.worldview_damage_registrations.back();
  r.created=0;r.closure=lua_topointer(L,1);r.registered=L;
  r.epoch=self.failure_epoch;r.uncertainty=self.execution_uncertainty;
 }catch(const std::exception&x){self.source_exception=std::current_exception();self.source_error=x.what();}
 catch(...){self.source_exception=std::current_exception();self.source_error="Damage retained registration allocation failed";}
 if(self.source_error)return worldview_created_error(L,self.source_error);
 auto&r=self.worldview_damage_registrations.back();
 lua_pushvalue(L,1);r.function=luaL_ref(L,LUA_REGISTRYINDEX);
 lua_pushthread(L);r.thread=luaL_ref(L,LUA_REGISTRYINDEX);
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.klass);lua_pushliteral(L,"UpdateHealth");lua_rawget(L,-2);
 if(!worldview_created_line(L,-1,380)||self.graph_compiled_owner(L,-1)!=compiled)
  return worldview_created_error(L,"Exact original class.UpdateHealth380 absent");
 r.health_closure=lua_topointer(L,-1);lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,e.environment);
 const bool health_env=lua_rawequal(L,-1,-2);lua_pop(L,2);
 if(!health_env)return worldview_created_error(L,"Original UpdateHealth environment differs");
 r.update_health=luaL_ref(L,LUA_REGISTRYINDEX);lua_pop(L,1);lua_pushvalue(L,1);return 1;
}
static int worldview_damage_count(lua_State*L){
 auto&self=owner(L);
 if(self.worldview_damage_registrations.size()!=1||self.worldview_damage_registrations.front().retired)
  return worldview_created_error(L,"Actual original damage registration absent");
 lua_pushinteger(L,1);return 1;
}
static int worldview_damage_event(lua_State*L){
 auto&self=owner(L);
 if(!self.worldview_native_host||lua_type(L,1)!=LUA_TSTRING){lua_pushnil(L);return 1;}
 std::size_t n=0;const char*context=lua_tolstring(L,1,&n);
 if(n!=15||std::memcmp(context,"UnitFlagManager",15)!=0){lua_pushnil(L);return 1;}
 const auto child=self.worldview_native_host->child;
 self.source_exception={};self.source_error=nullptr;
 try{self.worldview_require_callback_origin();
  if(child>=self.child_contexts.size()||self.child_contexts[child].name!="UnitFlagManager"||
   !self.calling||!self.child_contexts[child].loading||self.child_contexts[child].loaded)
   throw ScriptError("Damage event factory lacks actual UnitFlag loader");
 }catch(const std::exception&e){self.source_exception=std::current_exception();self.source_error=e.what();}
 catch(...){self.source_exception=std::current_exception();self.source_error="Damage event factory source failed";}
 if(self.source_error)return worldview_created_error(L,self.source_error);
 lua_newtable(L);lua_pushinteger(L,static_cast<lua_Integer>(child));lua_pushcclosure(L,worldview_damage_add,1);
 lua_setfield(L,-2,"Add");lua_pushcfunction(L,worldview_damage_count);lua_setfield(L,-2,"Count");
 lua_pushcfunction(L,worldview_created_unsupported);lua_setfield(L,-2,"Remove");
 lua_pushcfunction(L,worldview_created_unsupported);lua_setfield(L,-2,"RemoveAll");return 1;
}
static void worldview_damage_registry_validate(lua_State*L,std::size_t index){
 auto&self=owner(L);const int top=lua_gettop(L);
 if(index>=self.worldview_damage_registrations.size())worldview_created_error(L,"Damage listener occurrence absent");
 const auto&r=self.worldview_damage_registrations[index];
 if(r.retired||r.registered!=L||r.function<=0||r.thread<=0||r.update_health<=0||
  r.epoch!=self.failure_epoch||r.uncertainty!=self.execution_uncertainty||
  r.created>=self.worldview_created_registrations.size()||!lua_checkstack(L,20))
  worldview_created_error(L,"Damage retained original registration retired");
 worldview_created_registry_validate(L,r.created);const auto&e=self.worldview_created_registrations[r.created];
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.function);const int callback=lua_gettop(L);
 if(lua_topointer(L,callback)!=r.closure||!worldview_created_line(L,callback,1291)||
  self.graph_compiled_owner(L,callback)!=e.compiled)worldview_created_error(L,"Original damage Proto replaced");
 lua_getfenv(L,callback);lua_rawgeti(L,LUA_REGISTRYINDEX,e.environment);
 const bool env=lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==e.environment_identity;lua_pop(L,2);
 if(!env)worldview_created_error(L,"Damage original environment replaced");
 const int master=worldview_created_upvalue(L,callback,"g_MasterList");
 if(!master||!lua_istable(L,master))worldview_created_error(L,"Damage master upvalue retired");
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.master);
 const bool same=lua_rawequal(L,-1,master)&&lua_topointer(L,master)==e.master_identity;lua_pop(L,2);
 if(!same)worldview_created_error(L,"Damage original shared master replaced");
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.klass);lua_pushliteral(L,"UpdateHealth");lua_rawget(L,-2);
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.update_health);
 const bool health=lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==r.health_closure&&
  worldview_created_line(L,-1,380)&&self.graph_compiled_owner(L,-1)==e.compiled;
 if(!health)worldview_created_error(L,"Original UpdateHealth member replaced");
 lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,e.environment);
 const bool health_env=lua_rawequal(L,-1,-2);lua_pop(L,2);
 if(!health_env)worldview_created_error(L,"Original UpdateHealth environment replaced");
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.thread);
 if(!lua_isthread(L,-1)||lua_tothread(L,-1)!=L)worldview_created_error(L,"Damage original coroutine replaced");
 lua_settop(L,top);
}
void worldview_retire_damage_registrations()noexcept{
 if(ui_thread!=std::this_thread::get_id())std::terminate();
 for(auto&r:worldview_damage_registrations)r.retired=true;
}
static int worldview_release_retired_damage(lua_State*L){
 auto&self=owner(L);if(self.ui_thread!=std::this_thread::get_id())return worldview_created_error(L,"Damage cleanup foreign creator");
 for(auto&r:self.worldview_damage_registrations)if(r.retired){
  int*refs[]={&r.function,&r.thread,&r.update_health};
  for(int*p:refs){const int ref=*p;*p=LUA_NOREF;if(ref!=LUA_NOREF)luaL_unref(L,LUA_REGISTRYINDEX,ref);}}
 return 0;
}
