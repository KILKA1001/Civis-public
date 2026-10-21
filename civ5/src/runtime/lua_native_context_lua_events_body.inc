// Inside actual Host::Impl, after the lexical LuaEvents frame definition.
static void native_lua_events_full(lua_State* L,Job& job){
    auto& p=owner(L);const auto* frame=p.lua_events_frame;
    p.source_exception={};p.source_error=nullptr;
    try{if(!frame)throw ScriptError("LuaEvents actual load frame absent");frame->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="LuaEvents SAME load/source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    native_events_prefix_full(L,job);
    native_context_include_delivery_full(L,frame->child);
    p.source_exception={};p.source_error=nullptr;
    try{if(p.lua_events_frame!=frame)throw ScriptError("LuaEvents actual frame replaced");frame->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="LuaEvents FIRST after callbacks refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}
static void require_native_lua_events_predecessors(lua_State* L,Job& job){
    native_lua_events_full(L,job);auto& p=owner(L);const auto*const frame=p.lua_events_frame;
    const auto*const events=p.events_prefix_frame;
    if(!frame||frame->events!=events||!events||events->child!=job.worldview_child||
       events->published_events_reference<0||!events->published_events_table||
       job.context_events_reference!=events->published_events_reference||job.context_events_table!=events->published_events_table||
       events->master_factory_assigned!=events->factory||events->ordered_assigned_count!=232||
       !events->lua_event_constructor_assigned||!events->factory_index_assigned)
        return (void)luaL_error(L,"LuaEvents4573B5 lacks actual completed Events predecessor/data");
    native_events_prefix_physical(L,*events);
}
static void perform_native_context_lua_events_registration(lua_State* L,Job& job){
    auto& p=owner(L);native_lua_events_full(L,job);
    require_native_lua_events_predecessors(L,job);
    if(!lua_checkstack(L,20))return (void)luaL_error(L,"LuaEvents registration stack unavailable");
    native_lua_events_full(L,job);
    if(!p.lua_events_frame||p.lua_events_frame->child!=job.worldview_child||
       job.worldview_child>=p.child_contexts.size()||
       p.child_contexts[job.worldview_child].lua_events_reference!=LUA_NOREF)
        return (void)luaL_error(L,"LuaEvents actual selected receiver/replay refused");
    perform_native_context_state_query(L,job);native_lua_events_full(L,job);
    const int environment=lua_gettop(L);
    if(lua_topointer(L,environment)!=p.lua_events_frame->actual_environment||
       job.context_state_thread!=p.lua_events_frame->actual_thread)
        return (void)luaL_error(L,"LuaEvents same actual518 environment refused");
    native_lua_events_key(L,3);native_lua_events_full(L,job);
    lua_rawget(L,environment);native_lua_events_full(L,job);
    if(!lua_isnil(L,-1))return (void)luaL_error(L,"Unowned selected LuaEvents value unsupported");
    lua_pop(L,1);
    // Native700540 uses GLOBALS before534B50 selects the target environment.
    // The real port bootstrap's retained factory is the exact common owner.
    native_lua_events_key(L,1);native_lua_events_full(L,job);
    lua_rawget(L,LUA_GLOBALSINDEX);native_lua_events_full(L,job);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);native_lua_events_key(L,1);native_lua_events_full(L,job);
        lua_rawgeti(L,LUA_REGISTRYINDEX,p.lua_events_factory_ref);native_lua_events_full(L,job);
        if(!native_lua_events_factory_without_callbacks(L))
            return (void)luaL_error(L,"Canonical LuaEvents factory retired/rebound");
        lua_rawset(L,LUA_GLOBALSINDEX);native_lua_events_full(L,job);
    }else{
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.lua_events_factory_pointer||
           !native_lua_events_factory_without_callbacks(L))
            return (void)luaL_error(L,"Unowned master LuaEventFactory unsupported");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);native_lua_events_full(L,job);
    const int table=lua_gettop(L);const void* value=lua_topointer(L,table);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.lua_events_factory_ref);native_lua_events_full(L,job);
    if(!native_lua_events_factory_without_callbacks(L))
        return (void)luaL_error(L,"LuaEvents same factory after allocation refused");
    lua_setmetatable(L,table);native_lua_events_full(L,job);
    lua_pushvalue(L,table);native_lua_events_full(L,job);
    const int reference=luaL_ref(L,LUA_REGISTRYINDEX);native_lua_events_full(L,job);
    if(reference==LUA_NOREF||reference==LUA_REFNIL)
        return (void)luaL_error(L,"LuaEvents actual table registry reference refused");
    native_lua_events_key(L,3);native_lua_events_full(L,job);
    lua_pushvalue(L,table);native_lua_events_full(L,job);
    lua_rawset(L,environment);native_lua_events_full(L,job);
    // LAST FULL precedes only retained-key nonallocating checks and publication.
    native_lua_events_full(L,job);
    if(!native_lua_events_factory_without_callbacks(L))
        return (void)luaL_error(L,"LuaEvents final canonical factory refused");
    native_lua_events_key(L,1);lua_rawget(L,LUA_GLOBALSINDEX);
    if(lua_topointer(L,-1)!=p.lua_events_factory_pointer)
        return (void)luaL_error(L,"LuaEvents final master factory changed");
    lua_pop(L,1);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.lua_events_frame->thread);
    if(lua_tothread(L,-1)!=p.lua_events_frame->actual_thread||
       lua_tothread(L,-1)==L||lua_status(p.lua_events_frame->actual_thread)||
       lua_gettop(p.lua_events_frame->actual_thread))
        return (void)luaL_error(L,"LuaEvents final actual518 coroutine changed");
    lua_getfenv(L,-1);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.lua_events_frame->environment);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.lua_events_frame->actual_environment||
       !lua_rawequal(L,-1,-2)||!lua_rawequal(L,-1,environment))
        return (void)luaL_error(L,"LuaEvents final actual518 fenv changed");
    lua_pop(L,3);
    if(lua_getmetatable(L,environment))
        return (void)luaL_error(L,"LuaEvents selected master fallback unsupported");
    native_lua_events_key(L,3);lua_rawget(L,environment);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=value||!lua_getmetatable(L,-1)||
       lua_topointer(L,-1)!=p.lua_events_factory_pointer)
        return (void)luaL_error(L,"LuaEvents physical selected table/metatable changed");
    lua_pop(L,2);lua_rawgeti(L,LUA_REGISTRYINDEX,reference);
    if(lua_topointer(L,-1)!=value)return (void)luaL_error(L,"LuaEvents retained table changed");
    lua_pop(L,1);
    p.child_contexts[job.worldview_child].lua_events_reference=reference;
    p.child_contexts[job.worldview_child].lua_events_pointer=value;
    job.context_lua_events_reference=reference;job.context_lua_events_table=value;
    // Actual registrar DATA only; no fullSetup/RootRead or late-create grant.
}
