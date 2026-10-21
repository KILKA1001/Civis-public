// Runs inside the genuine protected Context install, before compiled source.
// Storage is Impl-owned before any allocating Lua call; no RAII owner crosses
// lua_error/longjmp, and these references are distinct from permanent refs.
static void retain_context_loader_temporary_refs(lua_State* L,std::size_t slot,int environment){
 auto& p=owner(L);auto& child=p.child_contexts[slot];
 if(!child.loader_temporaries)return; // generic loader has a separate contract
 auto& temp=*child.loader_temporaries;
 p.worldview_require_callback_origin();
 if(!p.worldview_native_host||p.worldview_native_host->child!=slot||temp.creator!=std::this_thread::get_id()||
    temp.core.expired()||temp.source.get()!=child.source.get()||temp.context!=child.name||
    temp.refs_returned||temp.cleanup_started||temp.environment!=LUA_NOREF||temp.receiver!=LUA_NOREF||
    !child.state_prepared||child.environment==LUA_NOREF||child.receiver==LUA_NOREF)
    return (void)luaL_error(L,"Selected Civis loader real temporary-ref origin/replay mismatch");
 if(L!=p.state)return (void)luaL_error(L,"Loader registry refs require the actual protected Host root callsite");
 temp.registry_state=L;
 lua_rawgeti(L,LUA_REGISTRYINDEX,child.environment);
 if(!lua_istable(L,-1)||!lua_rawequal(L,environment,-1))return (void)luaL_error(L,"Loader environment is not actual Context env");
 temp.actual_environment=lua_topointer(L,-1);temp.environment=luaL_ref(L,LUA_REGISTRYINDEX);
 lua_getfield(L,environment,"ContextPtr");lua_rawgeti(L,LUA_REGISTRYINDEX,child.receiver);
 if(!lua_istable(L,-1)||!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Loader receiver is not actual ContextPtr");
 lua_pop(L,1);temp.actual_receiver=lua_topointer(L,-1);temp.receiver=luaL_ref(L,LUA_REGISTRYINDEX);
 if(temp.environment==LUA_NOREF||temp.receiver==LUA_NOREF||temp.environment==child.environment||
    temp.receiver==child.receiver||temp.receiver==temp.environment)
    return (void)luaL_error(L,"Loader temporary registry refs were not separately allocated");
 p.worldview_require_callback_origin();temp.refs_returned=true;
}
