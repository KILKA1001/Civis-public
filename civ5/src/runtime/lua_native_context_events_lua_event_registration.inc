// Actual fourth573AC0 registrar42AA50, after both complete native groups.
static void native_events_register_lua_event_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||f->ordered_assigned_count!=232||f->lua_event_constructor_created||f->lua_event_constructor_assigned||
       p.native_lua_event_retired||p.native_lua_event_keys_ref!=LUA_NOREF||
       p.native_lua_event_group_ref!=LUA_NOREF||p.native_lua_event_lists_ref!=LUA_NOREF||p.native_lua_event_constructor_closure)
        return (void)luaL_error(L,"LuaEvent actual42AA50 order/replay refused");
    native_ordered_current(L,231);
    lua_createtable(L,11,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<11;++i){
        lua_pushstring(L,native_lua_event_keys[i]);native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_lua_event_keys_table=keys_pointer;lua_pushvalue(L,keys);p.native_lua_event_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    lua_createtable(L,0,0);const int group=lua_gettop(L);const void*const group_pointer=lua_topointer(L,group);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,group,group_pointer);
    lua_createtable(L,1,0);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    native_lua_event_key(L,lua_event_mode);native_lua_event_key(L,lua_event_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);lua_setmetatable(L,group);
    p.native_lua_event_group=group_pointer;p.native_lua_event_group_meta=meta_pointer;
    lua_pushvalue(L,group);p.native_lua_event_group_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    // Weak, pointer-keyed port evidence does not change native group # or GC.
    lua_createtable(L,0,0);const int lists=lua_gettop(L);const void*const lists_pointer=lua_topointer(L,lists);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,lists,lists_pointer);
    lua_createtable(L,0,1);const int lists_meta=lua_gettop(L);const void*const lists_meta_pointer=lua_topointer(L,lists_meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,lists_meta,lists_meta_pointer);
    native_lua_event_key(L,lua_event_mode);native_lua_event_key(L,lua_event_weak);lua_rawset(L,lists_meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,lists_meta,lists_meta_pointer);lua_setmetatable(L,lists);
    p.native_lua_event_lists=lists_pointer;p.native_lua_event_lists_meta=lists_meta_pointer;
    lua_pushvalue(L,lists);p.native_lua_event_lists_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_lua_event_physical(L);
    native_lua_event_key(L,lua_event_name);lua_pushvalue(L,group);lua_pushcclosure(L,native_lua_event_constructor,1);
    const void*const constructor=lua_topointer(L,-1);f->lua_event_constructor_created=constructor;
    native_events_prefix_full(L,job);native_lua_event_physical(L);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||lua_tocfunction(L,-1)!=native_lua_event_constructor||
       !lua_getupvalue(L,-1,1)||!lua_istable(L,-1)||lua_topointer(L,-1)!=group_pointer)
        return (void)luaL_error(L,"LuaEvent genuine4CAF40 one weak-group capture changed after FULL");
    lua_pop(L,1);if(lua_getupvalue(L,-1,2))return (void)luaL_error(L,"LuaEvent constructor acquired extra native upvalues");
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);lua_rawset(L,-3);
    f->lua_event_constructor_assigned=constructor;p.native_lua_event_constructor_closure=constructor;
    native_events_prefix_full(L,job);native_lua_event_current(L);lua_pop(L,4);
}
