// Actual547460 lazy factory __index. Its one upvalue is the complete native
// generator table. Unknown keys return zero values as the original does.
static int native_events_lazy_index(lua_State* L){
    native_lua_event_current(L);auto& p=owner(L);
    if(!lua_istable(L,lua_upvalueindex(1))||lua_topointer(L,lua_upvalueindex(1))!=p.native_ordered_generator_table)
        return luaL_error(L,"Events547460 actual native generator capture changed");
    lua_pushvalue(L,2);lua_gettable(L,lua_upvalueindex(1));
    if(lua_type(L,-1)==LUA_TFUNCTION){
        lua_call(L,0,1);native_lua_event_current(L);
        lua_pushvalue(L,2);lua_pushvalue(L,-2);lua_rawset(L,1);native_lua_event_current(L);return 1;
    }return 0;
}
static void native_events_factory_index_physical(lua_State* L,const NativeContextEventsPrefixFrame& f){
    const int top=lua_gettop(L);
    if(!f.factory_index_assigned||f.factory_index_created!=f.factory_index_assigned||!f.lua_event_constructor_assigned||f.ordered_assigned_count!=232)
        return (void)luaL_error(L,"Events actual547460 completion/closure allocation absent");
    lua_rawgeti(L,LUA_REGISTRYINDEX,f.factory_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f.factory||lua_getmetatable(L,-1))
        return (void)luaL_error(L,"Events actual factory table changed after index publication");
    const int factory=lua_gettop(L);int count=0;lua_pushnil(L);
    while(lua_next(L,factory)){
        if(++count!=1||!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=f.factory_index_assigned||lua_tocfunction(L,-1)!=native_events_lazy_index)
            return (void)luaL_error(L,"Events actual factory acquired unowned index field");
        lua_rawgeti(L,LUA_REGISTRYINDEX,f.keys_ref);lua_rawgeti(L,-1,2);lua_remove(L,-2);
        if(!lua_rawequal(L,-1,-3))return (void)luaL_error(L,"Events original factory __index key changed");
        lua_pop(L,1);
        if(!lua_getupvalue(L,-1,1)||!lua_istable(L,-1)||lua_topointer(L,-1)!=f.generators)
            return (void)luaL_error(L,"Events547460 actual complete generator capture changed");
        lua_pop(L,1);if(lua_getupvalue(L,-1,2))return (void)luaL_error(L,"Events lazy index acquired extra upvalues");
        lua_pop(L,1);
    }
    if(count!=1)return (void)luaL_error(L,"Events actual factory lost547460 index");lua_settop(L,top);
}
static void native_events_selected_publication_physical(lua_State* L,const NativeContextEventsPrefixFrame& f){
    const int top=lua_gettop(L);
    if(f.published_events_reference<0||!f.published_events_table||f.master_factory_assigned!=f.factory)
        return (void)luaL_error(L,"Events4637C0 has no actual selected publication");
    lua_rawgeti(L,LUA_REGISTRYINDEX,f.environment);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f.actual_environment)
        return (void)luaL_error(L,"Events publication actual518 environment changed");
    lua_rawgeti(L,LUA_REGISTRYINDEX,f.keys_ref);lua_rawgeti(L,-1,3);lua_remove(L,-2);lua_rawget(L,-2);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f.published_events_table||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=f.factory)
        return (void)luaL_error(L,"Events selected table/real factory metatable changed");
    lua_pop(L,1);lua_rawgeti(L,LUA_REGISTRYINDEX,f.published_events_reference);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Events selected actual retained table changed");
    lua_settop(L,top);
}
static void native_events_publish_factory_and_selected(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||f->ordered_assigned_count!=232||!f->lua_event_constructor_assigned||f->factory_index_created||
       f->factory_index_assigned||f->master_factory_assigned||f->published_events_reference!=LUA_NOREF||f->published_events_table)
        return (void)luaL_error(L,"Events547460/4637C0 actual publication order/replay refused");
    native_lua_event_current(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,2);lua_remove(L,-2);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_pushcclosure(L,native_events_lazy_index,1);
    const void*const closure=lua_topointer(L,-1);f->factory_index_created=closure;
    native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=closure||lua_tocfunction(L,-1)!=native_events_lazy_index||
       !lua_getupvalue(L,-1,1)||!lua_istable(L,-1)||lua_topointer(L,-1)!=f->generators)
        return (void)luaL_error(L,"Events actual547460 closure/generator capture changed after FULL");
    lua_pop(L,1);if(lua_getupvalue(L,-1,2))return (void)luaL_error(L,"Events original547460 upvalue count changed");
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->factory_ref);lua_insert(L,-3);lua_rawset(L,-3);
    f->factory_index_assigned=closure;native_events_prefix_full(L,job);lua_pop(L,1);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);lua_rawgeti(L,LUA_REGISTRYINDEX,f->factory_ref);
    lua_rawseti(L,-2,0xFFFFFF);f->master_factory_assigned=f->factory;native_events_prefix_full(L,job);lua_pop(L,1);
    // Original4637C0 ->534B50 selects this very prepared518 fenv, then
    // creates a fresh Events table whose metatable is the factory itself.
    perform_native_context_state_query(L,job);native_events_prefix_full(L,job);const int environment=lua_gettop(L);
    if(lua_topointer(L,environment)!=f->actual_environment||job.context_state_thread!=f->actual_thread)
        return (void)luaL_error(L,"Events4637C0 actual534B50 selected environment changed");
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,3);lua_remove(L,-2);lua_rawget(L,environment);
    if(!lua_isnil(L,-1))return (void)luaL_error(L,"Events selected original field already has an unowned value");lua_pop(L,1);
    lua_createtable(L,0,0);const int events=lua_gettop(L);const void*const events_pointer=lua_topointer(L,events);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,events,events_pointer);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->factory_ref);lua_setmetatable(L,events);native_events_prefix_full(L,job);
    lua_pushvalue(L,events);const int reference=luaL_ref(L,LUA_REGISTRYINDEX);native_events_prefix_full(L,job);
    if(reference<0)return (void)luaL_error(L,"Events actual selected table registry allocation failed");
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,3);lua_remove(L,-2);lua_pushvalue(L,events);
    lua_settable(L,environment);
    f->published_events_reference=reference;f->published_events_table=events_pointer;
    native_events_prefix_full(L,job);native_events_selected_publication_physical(L,*f);
    // Data from actual execution only. WholeSetup/Controls remain unissued.
    job.context_events_reference=reference;job.context_events_table=events_pointer;lua_pop(L,2);
}
