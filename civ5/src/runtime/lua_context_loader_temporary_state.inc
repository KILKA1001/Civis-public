// Protected query/unref uses SAME actual Context, after compiled Lua+GC.
static void perform_context_loader_temporary_query(lua_State* L,Job& job){
 auto& p=owner(L);perform_native_context_initial_state_query(L,job);
 auto& child=p.child_contexts[job.worldview_child];
 if(!child.loader_temporaries)return (void)luaL_error(L,"Selected Civis loader temporary owner absent");
 auto& temp=*child.loader_temporaries;
 if(temp.creator!=std::this_thread::get_id()||temp.core.expired()||temp.source.get()!=child.source.get()||
    !temp.refs_returned||temp.cleanup_started||temp.cleanup_returned||temp.journal_size||
    temp.environment==LUA_NOREF||temp.receiver==LUA_NOREF||
    temp.registry_state!=L||L!=p.state||temp.actual_environment!=job.context_state_environment)
    return (void)luaL_error(L,"Selected Civis loader temporary lifetime is not current");
 lua_rawgeti(L,LUA_REGISTRYINDEX,temp.environment);lua_rawgeti(L,LUA_REGISTRYINDEX,child.environment);
 if(!lua_istable(L,-1)||!lua_rawequal(L,-1,-2)||lua_topointer(L,-1)!=temp.actual_environment)
    return (void)luaL_error(L,"Selected Civis loader temporary environment was rebound");
 lua_pop(L,2);lua_rawgeti(L,LUA_REGISTRYINDEX,temp.receiver);lua_rawgeti(L,LUA_REGISTRYINDEX,child.receiver);
 if(!lua_istable(L,-1)||!lua_rawequal(L,-1,-2)||lua_topointer(L,-1)!=temp.actual_receiver)
    return (void)luaL_error(L,"Selected Civis loader temporary receiver was rebound");
 lua_pop(L,2);p.worldview_require_callback_origin();
}
static void perform_context_loader_temporary_unref(lua_State* L,Job& job){
 auto& p=owner(L);perform_native_context_initial_state_query(L,job);
 auto& child=p.child_contexts[job.worldview_child];
 if(!child.loader_temporaries)return (void)luaL_error(L,"Loader cleanup has no allocated owner");
 auto& temp=*child.loader_temporaries;
 const bool receiver=job.operation==Operation::native_context_loader_receiver_unref;
 const auto expected=receiver?1u:2u;
 if(temp.creator!=std::this_thread::get_id()||!temp.cleanup_started||temp.cleanup_returned||
    temp.journal_size!=expected||temp.paths[0]||temp.registry_state!=L||L!=p.state||temp.source.get()!=child.source.get())
    return (void)luaL_error(L,"Loader real unref order/owner mismatch");
 int& reference=receiver?temp.receiver:temp.environment;
 const int permanent=receiver?child.receiver:child.environment;
 const void* identity=receiver?temp.actual_receiver:temp.actual_environment;
 if(reference==LUA_NOREF||reference==LUA_REFNIL||reference==permanent)
    return (void)luaL_error(L,"Loader cleanup cannot release permanent or absent registry ref");
 lua_rawgeti(L,LUA_REGISTRYINDEX,reference);lua_rawgeti(L,LUA_REGISTRYINDEX,permanent);
 if(!lua_istable(L,-1)||!lua_rawequal(L,-1,-2)||lua_topointer(L,-1)!=identity)
    return (void)luaL_error(L,"Loader cleanup exact ref object changed");
 lua_pop(L,2);
 luaL_unref(L,LUA_REGISTRYINDEX,reference);reference=LUA_NOREF;
 temp.record(receiver?ContextLoaderTemporaries::Step::receiver_unref:ContextLoaderTemporaries::Step::environment_unref);
 p.worldview_require_callback_origin();
}
