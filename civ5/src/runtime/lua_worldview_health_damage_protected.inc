void worldview_damage_owner(std::size_t index,const WorldViewCallbackFrame&frame){
 worldview_callback_fence(frame);
 if(index>=worldview_damage_registrations.size())throw ScriptError("Actual original damage registration absent");
 const auto&r=worldview_damage_registrations[index];
 if(r.retired||r.epoch!=failure_epoch||r.uncertainty!=execution_uncertainty||
  r.function<=0||r.thread<=0||r.update_health<=0||!r.closure||!r.health_closure||
  r.created>=worldview_created_registrations.size())
  throw ScriptError("Original damage listener actual references retired");
 const auto&e=worldview_created_registrations[r.created];
 if(r.registered!=e.registered)throw ScriptError("Original damage/created coroutine differs");
 worldview_created_owner(r.created,frame);worldview_callback_fence(frame);
}
struct WorldViewDamageJob {
 Impl*owner;std::size_t registration;
 std::int32_t player,id,damage,previous;
 bool invoked=false,returned=false;
};
static int worldview_damage_protected(lua_State*L){
 auto&job=*static_cast<WorldViewDamageJob*>(lua_touserdata(L,1));auto&self=*job.owner;
 if(&owner(L)!=&self||!self.calling||!self.worldview_damage_dispatching||
  !self.worldview_callback_dispatching||!self.worldview_loaded_child_call||
  self.worldview_loaded_child_call->kind!=WorldViewLoadedChildCallScope::Kind::damage||
  !self.worldview_health_membership_state||!lua_checkstack(L,12))
  return worldview_created_error(L,"Original damage protected native issuer absent");
 worldview_damage_registry_validate(L,job.registration);
 const auto&r=self.worldview_damage_registrations[job.registration];
 const auto&e=self.worldview_created_registrations[r.created];
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.function);
 lua_pushinteger(L,job.player);lua_pushinteger(L,job.id);
 lua_pushinteger(L,job.damage);lua_pushinteger(L,job.previous);
 self.graph_execution_scope=e.compiled;job.invoked=true;
 lua_call(L,4,0);job.returned=true;return 0;
}
