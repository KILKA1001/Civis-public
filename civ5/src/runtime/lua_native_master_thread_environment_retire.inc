// Actual6ACC70 passes its real argument1 to4EB630. Original4BDE30 invokes
// this before _DestroyThread; detaching Threads[thread] alone is incomplete.
// This private callback is not exported as an independently callable API and
// does not issue a default-thread scope or the whole7458E0 destructor.
static int native_master_clear_thread_environment_6acc70(lua_State* L){
    const auto saved=native_master_thread_callback_first(L);
    if(!lua_checkstack(L,32))return native_master_thread_refuse(L,"Native thread fenv clear real stack unavailable");
    native_master_thread_callback_post(L,saved);
    // Original6ACC70 always passes index1;4EB630's true non-table branch
    // returns without mutation. No guessed missing-table success is used by
    // the future scoped caller, which must earn the actual Threads mapping.
    if(lua_type(L,1)!=LUA_TTABLE)return 0;
    const void*const actual=lua_topointer(L,1);
    lua_pushnil(L);
    while(lua_next(L,1)){
        native_master_thread_callback_post(L,saved);
        if(lua_topointer(L,1)!=actual)return native_master_thread_refuse(L,"Native thread fenv clear actual table replaced after next");
        lua_pop(L,1);lua_pushvalue(L,-1);lua_pushnil(L);lua_settable(L,1);
        native_master_thread_callback_post(L,saved);
        if(lua_topointer(L,1)!=actual)return native_master_thread_refuse(L,"Native thread fenv clear actual table replaced after settable");
    }
    native_master_thread_callback_post(L,saved);
    if(lua_topointer(L,1)!=actual)return native_master_thread_refuse(L,"Native thread fenv clear actual final table replaced");
    return 0;
}
