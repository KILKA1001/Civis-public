// Actual private VM registry consumer used before a fresh HealthInstance mint.
// No Lua function, Source provider, Tree getter or original constructor is run.
struct WorldViewHealthReturnedRegistryJob {Impl*owner;std::size_t receipt;bool returned=false;};
static int worldview_health_return_registry_protected(lua_State*L){
 auto&job=*static_cast<WorldViewHealthReturnedRegistryJob*>(lua_touserdata(L,1));auto&self=*job.owner;
 if(&owner(L)!=&self||job.receipt>=self.worldview_created_instances.size())
  return worldview_created_error(L,"Health registry actual VM/occurrence absent");
 const auto&r=self.worldview_created_instances[job.receipt];
 if(r.retired||!r.original_returned||r.registration>=self.worldview_created_registrations.size()||
  r.instance<=0||r.controls<=0||r.owner<0||r.full_id<0||r.graph_instance>=self.graph_instances.size()||
  !lua_checkstack(L,20))return worldview_created_error(L,"Health registry lacks actual completed constructor return");
 const auto&e=self.worldview_created_registrations[r.registration];
 worldview_created_registry_validate(L,r.registration);
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.master);lua_pushinteger(L,r.owner);lua_rawget(L,-2);
 if(!lua_istable(L,-1))return worldview_created_error(L,"Health actual master owner table retired");
 lua_pushinteger(L,r.full_id);lua_rawget(L,-2);const int flag=lua_gettop(L);
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.instance);
 const bool actual_flag=lua_istable(L,flag)&&lua_rawequal(L,flag,-1)&&lua_topointer(L,flag)==r.instance_identity;
 lua_pop(L,1);if(!actual_flag)return worldview_created_error(L,"Health original full-ID flag membership replaced");
 lua_pushliteral(L,"m_PlayerID");lua_rawget(L,flag);
 const bool owner=lua_type(L,-1)==LUA_TNUMBER&&lua_tonumber(L,-1)==r.owner;lua_pop(L,1);
 lua_pushliteral(L,"m_UnitID");lua_rawget(L,flag);
 const bool unit=lua_type(L,-1)==LUA_TNUMBER&&lua_tonumber(L,-1)==r.full_id;lua_pop(L,1);
 if(!owner||!unit)return worldview_created_error(L,"Health original flag full-ID changed");
 if(!lua_getmetatable(L,flag))return worldview_created_error(L,"Health original flag class retired");
 lua_rawgeti(L,LUA_REGISTRYINDEX,e.klass);const bool klass=lua_rawequal(L,-1,-2);lua_pop(L,2);
 if(!klass)return worldview_created_error(L,"Health original flag actual class replaced");
 lua_pushliteral(L,"m_Instance");lua_rawget(L,flag);const int controls=lua_gettop(L);
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.controls);
 const bool actual_controls=lua_istable(L,controls)&&lua_rawequal(L,controls,-1)&&lua_topointer(L,controls)==r.controls_identity;
 lua_pop(L,1);if(!actual_controls)return worldview_created_error(L,"Health original retained m_Instance replaced");
 const auto&instance=self.graph_instances[r.graph_instance];
 if(instance.slot!=e.graph_slot||instance.table<=0||!r.instance_root||instance.root!=r.instance_root)
  return worldview_created_error(L,"Health canonical GraphInstance root retired");
 lua_rawgeti(L,LUA_REGISTRYINDEX,instance.table);const bool built=lua_rawequal(L,-1,controls);lua_pop(L,1);
 if(!built)return worldview_created_error(L,"Health controls no longer actual canonical built instance");
 const char*names[]={"HealthBarBG","HealthBar","HealthBarButton","NormalButton","NormalSelect","HealthBarSelect"};
 for(std::size_t i=0;i<6;++i){lua_pushstring(L,names[i]);lua_rawget(L,controls);
  const auto*c=worldview_created_graph_control_without_lua_calls(L,lua_gettop(L),e.graph_slot);
  if(c->id!=r.health_ids[i])return worldview_created_error(L,"Health original canonical control occurrence replaced");lua_pop(L,1);}
 lua_pushliteral(L,"m_CargoControls");lua_rawget(L,flag);const int cargo=lua_gettop(L);
 if(r.cargo==LUA_NOREF){if(!lua_isnil(L,cargo)||r.cargo_pull_down||r.cargo_identity||
  r.cargo_graph_instance!=graph_no_owner||r.cargo_instance_root)
  return worldview_created_error(L,"Health original no-Cargo branch changed");}
 else{
  lua_rawgeti(L,LUA_REGISTRYINDEX,r.cargo);
  const bool actual_cargo=lua_istable(L,cargo)&&lua_rawequal(L,-1,cargo)&&lua_topointer(L,cargo)==r.cargo_identity;lua_pop(L,1);
  if(!actual_cargo||!r.cargo_pull_down||r.cargo_graph_instance>=self.graph_instances.size())
   return worldview_created_error(L,"Health actual Cargo branch replaced");
  const auto&built_cargo=self.graph_instances[r.cargo_graph_instance];
  if(built_cargo.slot!=e.graph_slot||built_cargo.table<=0||!r.cargo_instance_root||built_cargo.root!=r.cargo_instance_root)
   return worldview_created_error(L,"Health Cargo canonical GraphInstance root retired");
  lua_rawgeti(L,LUA_REGISTRYINDEX,built_cargo.table);const bool same_cargo=lua_rawequal(L,-1,cargo);lua_pop(L,1);
  if(!same_cargo)return worldview_created_error(L,"Health Cargo actual graph instance table replaced");
  lua_pushliteral(L,"PullDown");lua_rawget(L,cargo);const auto*c=worldview_created_graph_control_without_lua_calls(L,lua_gettop(L),e.graph_slot);
  if(c->id!=*r.cargo_pull_down)return worldview_created_error(L,"Health actual Cargo PullDown replaced");lua_pop(L,1);
 }
 job.returned=true;return 0;
}
void worldview_require_returned_health_registry_owned(std::size_t receipt){
 if(ui_thread!=std::this_thread::get_id())throw ScriptError("Health registry foreign creator before VM access");
 if(calling||failed||worldview_native_host||worldview_loaded_child_call||worldview_created_dispatching||
  !end_turn_lifetime||!end_turn_lifetime->alive.load()||end_turn_lifetime->failed.load()||
  end_turn_lifetime->vm!=state||receipt>=worldview_created_instances.size())
  throw ScriptError("Fresh health registry lender lacks idle authentic live VM");
 const auto&r=worldview_created_instances[receipt];
 if(r.registration>=worldview_created_registrations.size())throw ScriptError("Health registry parent registration absent");
 auto*L=worldview_created_registrations[r.registration].registered;
 const int top=lua_gettop(L);const auto epoch=failure_epoch,writes=failed.writes_,uncertainty=execution_uncertainty;
 const bool prior_ready=ready;WorldViewHealthReturnedRegistryJob job{this,receipt};
 const auto previous=graph_execution_scope;const auto prior_hook=lua_gethook(L);const int mask=lua_gethookmask(L),count=lua_gethookcount(L);
 struct Exit{Impl&p;lua_State*L;int top;std::size_t previous;lua_Hook hook;int mask,count;
  ~Exit(){lua_sethook(L,hook,mask,count);lua_settop(L,top);p.graph_execution_scope=previous;p.calling=false;}}exit{*this,L,top,previous,prior_hook,mask,count};
 calling=true;
 try{
  const int status=lua_cpcall(L,worldview_health_return_registry_protected,&job);
  // No Source.current/origin/serial; FIRST pure VM lifetime/failure POST.
  if(!end_turn_lifetime->alive.load()||end_turn_lifetime->failed.load()||end_turn_lifetime->vm!=state||
   failure_epoch!=epoch||failed.writes_!=writes||failed.exhausted_||execution_uncertainty!=uncertainty||
   ready!=prior_ready||!calling||worldview_native_host||worldview_created_instances[receipt].retired)
   throw ScriptError("Health actual registry/failure/lifetime changed during protected read");
  if(status||!job.returned)throw ScriptError(status?worldview_callback_lua_error(L):"Health actual registry validation did not return");
 }catch(...){record_failure();end_turn_lifetime->failed=true;worldview_retire_created_registrations();throw;}
}
