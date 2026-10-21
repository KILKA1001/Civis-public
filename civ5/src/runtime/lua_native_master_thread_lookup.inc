// Actual534B50 same-thread lookup, on the genuine master Threads map.
// Leaves the real coroutine key and its real fenv table, in original order.
// This helper alone does not issue4D9FC0/6437F0 scope or7458E0 destruction.
static void native_master_find_thread_environment_534b50_owned(lua_State* L,lua_State* actual_thread){
    auto& p=owner(L);const auto saved=native_master_thread_callback_first(L);
    if(L!=p.state||!actual_thread)
        return (void)native_master_thread_refuse(L,"Native534B50 requires real master VM and actual non-null thread");
    if(!lua_checkstack(L,32))return (void)native_master_thread_refuse(L,"Native534B50 actual Lua stack unavailable");
    native_master_thread_callback_post(L,saved);
    lua_getfield(L,LUA_GLOBALSINDEX,"Threads");native_master_thread_callback_post(L,saved);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=saved.actual->threads)
        return (void)native_master_thread_refuse(L,"Native534B50 actual global Threads table changed");
    const int table=lua_gettop(L);lua_pushnil(L);
    // Original534B78..534BA1 traverses actual Lua entries rather than making
    // a numeric coroutine id or trusting a separately cached fenv pointer.
    for(;;){
        const int next=lua_next(L,table);native_master_thread_callback_post(L,saved);
        if(!next)
            return (void)native_master_thread_refuse(L,"Native534B50 required actual thread is absent from original Threads map");
        if(lua_tothread(L,-2)==actual_thread)break;
        lua_pop(L,1);
    }
    if(lua_type(L,-2)!=LUA_TTHREAD||!lua_istable(L,-1))
        return (void)native_master_thread_refuse(L,"Native534B50 genuine matching thread/fenv operands unavailable");
    // Original returns the retained map value even after FLua replaces the
    // coroutine's globals. It does not require mapValue==gt(actual_thread).
    lua_remove(L,table);native_master_thread_callback_post(L,saved);
}
