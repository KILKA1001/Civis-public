// Included INSIDE the SAME Host::Impl after the original Created listener.
// Native dispatcher owns the actual job throughout lua_cpcall. No stack RAII
// exists inside an allocating Lua API/protected callback.
static void unitflag_capture_constructor_managers(lua_State* L,std::size_t registration){
 auto& self=owner(L);auto& e=self.worldview_created_registrations.at(registration);
 if(e.registered!=L||e.new_function<=0||e.constructor_manager_class!=LUA_NOREF)
  worldview_created_error(L,"UnitFlag constructor manager capture replay or foreign state");
 const int top=lua_gettop(L);lua_rawgeti(L,LUA_REGISTRYINDEX,e.new_function);const int original_new=lua_gettop(L);
 const char* names[]={"g_MilitaryManager","g_CivilianManager","g_AirCraftManager"};
 for(std::size_t i=0;i<3;++i){
  const int manager=worldview_created_upvalue(L,original_new,names[i]);
  if(!manager||!lua_istable(L,manager)||!lua_getmetatable(L,manager))
   worldview_created_error(L,"UnitFlag original new manager/class upvalue absent");
  if(i==0){e.constructor_manager_class_identity=lua_topointer(L,-1);
   lua_pushvalue(L,-1);e.constructor_manager_class=luaL_ref(L,LUA_REGISTRYINDEX);
   const char* functions[]={"GetInstance","BuildInstance"};const int lines[]={36,83};
   for(std::size_t k=0;k<2;++k){lua_pushstring(L,functions[k]);lua_rawget(L,-2);
    const auto compiled=self.graph_compiled_owner(L,-1);
    if(!worldview_created_line(L,-1,lines[k])||compiled==graph_no_owner||
       self.graph_compiled_sources[compiled].context!=self.child_contexts[e.child].name)
     worldview_created_error(L,"UnitFlag manager methods are not retained original InstanceManager source");
    e.constructor_manager_method_identities[k]=lua_topointer(L,-1);
    e.constructor_manager_methods[k]=luaL_ref(L,LUA_REGISTRYINDEX);
   }
  }else{lua_rawgeti(L,LUA_REGISTRYINDEX,e.constructor_manager_class);
   const bool same=lua_rawequal(L,-1,-2);lua_pop(L,1);
   if(!same)worldview_created_error(L,"UnitFlag actual managers have different original InstanceManager class");}
  lua_pop(L,1);e.constructor_manager_identities[i]=lua_topointer(L,manager);
  lua_pushvalue(L,manager);e.constructor_managers[i]=luaL_ref(L,LUA_REGISTRYINDEX);
  lua_pop(L,1);
 }
 lua_settop(L,top);
}
struct UnitFlagInitializingButtonJob {
 Impl* owner;std::size_t registration;std::uint64_t node,root;
 std::int32_t actor,full_id;bool returned=false;
};
static int unitflag_initializing_button_protected(lua_State* L){
 auto& job=*static_cast<UnitFlagInitializingButtonJob*>(lua_touserdata(L,1));auto& self=*job.owner;
 const auto* ctor=self.worldview_unitflag_constructor_job;
 if(&owner(L)!=&self||!ctor||ctor->owner!=&self||!ctor->invoked||ctor->returned||
    ctor->index!=job.registration||ctor->player!=job.actor||ctor->full_id!=job.full_id||
    ctor->receipt>=self.worldview_created_instances.size()||job.registration>=self.worldview_created_registrations.size())
  return worldview_created_error(L,"UnitFlag button lacks SAME active genuine original constructor job");
 const auto& e=self.worldview_created_registrations[job.registration];
 const auto& r=self.worldview_created_instances[ctor->receipt];
 if(e.retired||e.registered!=L||r.retired||r.original_returned||r.registration!=job.registration||
    r.owner!=job.actor||r.full_id!=job.full_id||e.child>=self.child_contexts.size()||
    !self.child_contexts[e.child].loaded||self.child_contexts[e.child].loading||
    e.constructor_manager_class<=0||!lua_checkstack(L,32))
  return worldview_created_error(L,"UnitFlag current Initialize realm retired or constructor already returned");
 worldview_created_registry_validate(L,job.registration);
 // The FIRST genuine Lua caller is Initialize149. The next two original
 // frames are its real new73 and original OnUnitCreated954, in that order.
 lua_Debug init{};bool found=false;int next_level=0;
 for(int level=1;level<=256&&lua_getstack(L,level,&init);++level){
  if(!lua_getinfo(L,"f",&init))break;if(lua_iscfunction(L,-1)){lua_pop(L,1);continue;}
  lua_rawgeti(L,LUA_REGISTRYINDEX,e.initialize);
  const bool same=lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==e.initialize_identity;
  lua_pop(L,2);if(!same)return worldview_created_error(L,"UnitFlag first Lua caller is not actual original Initialize");
  found=true;next_level=level+1;break;
 }
 if(!found)return worldview_created_error(L,"UnitFlag original Initialize frame absent");
 const int refs[]={e.new_function,e.function};const void* identities[]={e.new_identity,e.closure};
 for(std::size_t k=0;k<2;++k){bool actual=false;lua_Debug parent{};
  for(;next_level<=256&&lua_getstack(L,next_level,&parent);++next_level){
   if(!lua_getinfo(L,"f",&parent))break;if(lua_iscfunction(L,-1)){lua_pop(L,1);continue;}
   lua_rawgeti(L,LUA_REGISTRYINDEX,refs[k]);actual=lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==identities[k];
   lua_pop(L,2);++next_level;break;}
  if(!actual)return worldview_created_error(L,"UnitFlag original new/listener ancestor order changed");
 }
 // Params are actual locals from the already authenticated retained Proto.
 const char* local=lua_getlocal(L,&init,1);
 if(!local||std::strcmp(local,"o")||!lua_istable(L,-1))
  return worldview_created_error(L,"UnitFlag Initialize actual self table absent");
 const int flag=lua_gettop(L);
 if(!lua_getmetatable(L,flag))return worldview_created_error(L,"UnitFlag constructing self has no actual class");
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.klass);const bool klass=lua_rawequal(L,-1,-2);lua_pop(L,2);
 if(!klass)return worldview_created_error(L,"UnitFlag constructing self actual class substituted");
 const char* idnames[]={"m_PlayerID","m_UnitID"};const std::int32_t ids[]={job.actor,job.full_id};
 for(std::size_t k=0;k<2;++k){lua_pushstring(L,idnames[k]);lua_rawget(L,flag);
  const bool value=lua_type(L,-1)==LUA_TNUMBER&&lua_tonumber(L,-1)==ids[k];lua_pop(L,1);
  if(!value)return worldview_created_error(L,"UnitFlag Initialize actual complete unit identity differs");}
 lua_pushliteral(L,"m_Instance");lua_rawget(L,flag);
 if(!lua_istable(L,-1))return worldview_created_error(L,"UnitFlag constructing m_Instance absent");
 const int controls=lua_gettop(L);std::size_t graph_instance=graph_no_owner;
 for(std::size_t i=0;i<self.graph_instances.size();++i){const auto& actual=self.graph_instances[i];
  if(actual.slot!=e.graph_slot||actual.table<=0)continue;
  lua_rawgeti(L,LUA_REGISTRYINDEX,actual.table);const bool same=lua_rawequal(L,-1,controls);lua_pop(L,1);
  if(same){if(graph_instance!=graph_no_owner)return worldview_created_error(L,"UnitFlag actual GraphInstance duplicated");graph_instance=i;}}
 if(graph_instance==graph_no_owner||self.graph_instances[graph_instance].root!=job.root)
  return worldview_created_error(L,"UnitFlag Initialize did not receive SAME canonical built GraphInstance");
 lua_pushliteral(L,"Anchor");lua_rawget(L,controls);
 const auto* anchor=worldview_created_graph_control_without_lua_calls(L,lua_gettop(L),e.graph_slot);
 if(anchor->id!=job.root)return worldview_created_error(L,"UnitFlag actual instance Anchor replaced");lua_pop(L,1);
 const char* buttons[]={"NormalButton","HealthBarButton"};bool actual_button=false;
 for(const char* name:buttons){lua_pushstring(L,name);lua_rawget(L,controls);
  const auto* button=worldview_created_graph_control_without_lua_calls(L,lua_gettop(L),e.graph_slot);
  if(button->id==job.node)actual_button=true;lua_pop(L,1);}
 if(!actual_button)return worldview_created_error(L,"UnitFlag target not constructing original button member");
 lua_pushliteral(L,"m_InstanceManager");lua_rawget(L,flag);
 if(!lua_istable(L,-1))return worldview_created_error(L,"UnitFlag actual InstanceManager absent");
 const int manager=lua_gettop(L);std::size_t selected=graph_no_owner;
 for(std::size_t i=0;i<3;++i){if(e.constructor_managers[i]<=0)continue;
  lua_rawgeti(L,LUA_REGISTRYINDEX,e.constructor_managers[i]);const bool same=lua_rawequal(L,-1,manager)&&
   lua_topointer(L,manager)==e.constructor_manager_identities[i];lua_pop(L,1);
  if(same){if(selected!=graph_no_owner)return worldview_created_error(L,"UnitFlag original manager alias");selected=i;}}
 if(selected==graph_no_owner||!lua_getmetatable(L,manager))
  return worldview_created_error(L,"UnitFlag manager was not actual new73 upvalue capture");
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.constructor_manager_class);
 const bool manager_class=lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==e.constructor_manager_class_identity;
 lua_pop(L,2);if(!manager_class)return worldview_created_error(L,"UnitFlag actual InstanceManager class changed");
 const char* methods[]={"GetInstance","BuildInstance"};
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.constructor_manager_class);const int actual_class=lua_gettop(L);
 for(std::size_t k=0;k<2;++k){lua_pushstring(L,methods[k]);lua_rawget(L,actual_class);
  lua_rawgeti(L,LUA_REGISTRYINDEX,e.constructor_manager_methods[k]);
  const bool method=lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==e.constructor_manager_method_identities[k];lua_pop(L,2);
  if(!method)return worldview_created_error(L,"UnitFlag original InstanceManager allocation method replaced");}
 lua_pop(L,1);
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.new_function);const int original_new=lua_gettop(L);
 const char* managers[]={"g_MilitaryManager","g_CivilianManager","g_AirCraftManager"};
 const int upvalue=worldview_created_upvalue(L,original_new,managers[selected]);
 if(!upvalue||!lua_rawequal(L,upvalue,manager))return worldview_created_error(L,"UnitFlag original manager upvalue rebound");
 lua_pop(L,2);
 // Original BuildInstance records its manager before GetInstance transfers
 // exactly this table into the actual allocated pool, before Initialize.
 lua_pushliteral(L,"m_InstanceManager");lua_rawget(L,controls);
 const bool retained_manager=lua_rawequal(L,-1,manager);lua_pop(L,1);
 if(!retained_manager)return worldview_created_error(L,"UnitFlag built table belongs to another real manager");
 lua_pushliteral(L,"m_AllocatedInstances");lua_rawget(L,manager);
 if(!lua_istable(L,-1))return worldview_created_error(L,"UnitFlag original allocated instance pool absent");
 const int allocated=lua_gettop(L);std::size_t matches=0;
 lua_pushnil(L);while(lua_next(L,allocated)){if(lua_rawequal(L,-1,controls))++matches;lua_pop(L,1);}
 if(matches!=1)return worldview_created_error(L,"UnitFlag original GetInstance allocation not unique/current");
 lua_pop(L,1);lua_pushliteral(L,"m_AvailableInstances");lua_rawget(L,manager);
 if(!lua_istable(L,-1))return worldview_created_error(L,"UnitFlag original available instance pool absent");
 const int available=lua_gettop(L);lua_pushnil(L);while(lua_next(L,available)){
  const bool same=lua_rawequal(L,-1,controls);lua_pop(L,1);
  if(same)return worldview_created_error(L,"UnitFlag constructing instance still available/released");}
 job.returned=true;return 0;
}
