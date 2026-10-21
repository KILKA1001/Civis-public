// Inside the existing LuaScriptHost::Impl, after actual ChildContext/native scopes.
// Allocating Lua calls are inside p.call's cpcall. No C++ owning local/RAII
// exists on this protected C callback stack; partial refs belong to Impl.
static void perform_native_context_state_prepare(lua_State* L,Job& job){
    auto& p=owner(L);p.worldview_require_callback_origin();
    if(!p.worldview_native_host||p.worldview_native_host->child!=job.worldview_child||
       job.worldview_child>=p.child_contexts.size())
        return (void)luaL_error(L,"Context518 actual preparation scope/child absent");
    auto& child=p.child_contexts[job.worldview_child];
    if(child.state_prepared||child.loaded||child.loading||child.retired||
       child.environment!=LUA_NOREF||child.execution_thread!=LUA_NOREF||child.receiver!=LUA_NOREF)
        return (void)luaL_error(L,"Context518 coroutine constructor replay/partial state");
    // Native57A350: actual lua_newthread, empty table, setfenv(thread,table).
    // Registry retention ports the original Threads/WeakThreads lifetime to
    // this SAME Host's existing canonical ChildContext; no separate VM owner.
    lua_State* thread=lua_newthread(L);const int thread_index=lua_gettop(L);
    lua_pushvalue(L,thread_index);child.execution_thread=luaL_ref(L,LUA_REGISTRYINDEX);
    lua_newtable(L);lua_pushvalue(L,-1);child.environment=luaL_ref(L,LUA_REGISTRYINDEX);
    if(!lua_setfenv(L,thread_index))return (void)luaL_error(L,"Context518 real coroutine environment rejected");
    if(!thread||thread==p.state||lua_status(thread)||lua_gettop(thread))
        return (void)luaL_error(L,"Context518 creator returned no distinct idle coroutine");
    p.worldview_require_callback_origin();child.state_prepared=true;
}
static void perform_native_context_state_query(lua_State* L,Job& job){
    auto& p=owner(L);p.worldview_require_callback_origin();
    if(!p.worldview_native_host||p.worldview_native_host->child!=job.worldview_child||
       job.worldview_child>=p.child_contexts.size())
        return (void)luaL_error(L,"Context518 retained state query has no actual child scope");
    const auto& child=p.child_contexts[job.worldview_child];
    if(!child.state_prepared||child.retired||child.environment==LUA_NOREF||child.execution_thread==LUA_NOREF)
        return (void)luaL_error(L,"Context518 real retained coroutine/env missing");
    lua_rawgeti(L,LUA_REGISTRYINDEX,child.execution_thread);
    lua_State* thread=lua_tothread(L,-1);
    if(!thread||thread==p.state||lua_status(thread)||lua_gettop(thread))
        return (void)luaL_error(L,"Context518 retained coroutine is not distinct/idle");
    lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,child.environment);
    if(!lua_istable(L,-1)||!lua_rawequal(L,-1,-2))
        return (void)luaL_error(L,"Context518 actual coroutine environment rebound");
    job.context_state_thread=thread;job.context_state_environment=lua_topointer(L,-1);
    p.worldview_require_callback_origin();
}
