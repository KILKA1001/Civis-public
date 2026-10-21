// Inside Host::Impl, after actual Events frame/physical/FULL bodies.
static void native_ordered_prepare_registration(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->game_view_constructor_assigned||f->ordered_assigned_count||p.native_ordered_retired||
       p.native_ordered_keys_ref!=LUA_NOREF||p.native_ordered_names_ref!=LUA_NOREF||
       p.native_ordered_weak_ref!=LUA_NOREF||p.native_ordered_generator_ref!=LUA_NOREF)
        return (void)luaL_error(L,"ordered Events registration requires completed actual75E540 and unused owner");
    for(const auto& channel:p.native_ordered_channels)
        if(channel.created||channel.assigned||channel.queue_occurrence||!channel.queue.empty()||!channel.subscribers.empty())
            return (void)luaL_error(L,"ordered Events channel has previous unowned contents");
    lua_createtable(L,17,0);const int keys=lua_gettop(L);const void*const key_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,key_pointer);
    for(int i=0;i<17;++i){
        lua_pushstring(L,native_ordered_keys[i]);native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,key_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,key_pointer);
    }
    p.native_ordered_keys_table=key_pointer;lua_pushvalue(L,keys);p.native_ordered_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    lua_createtable(L,static_cast<int>(native_ordered_specs.size()),0);const int names=lua_gettop(L);const void*const name_pointer=lua_topointer(L,names);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,names,name_pointer);
    for(std::size_t i=0;i<native_ordered_specs.size();++i){
        lua_pushstring(L,native_ordered_specs[i].name);native_events_prefix_full(L,job);native_events_stack_table_physical(L,names,name_pointer);
        lua_rawseti(L,names,static_cast<int>(i)+1);native_events_prefix_full(L,job);native_events_stack_table_physical(L,names,name_pointer);
    }
    p.native_ordered_names_table=name_pointer;lua_pushvalue(L,names);p.native_ordered_names_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    native_ordered_key(L,ordered_mode);native_ordered_key(L,ordered_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);lua_setmetatable(L,weak);
    p.native_ordered_weak_table=weak_pointer;p.native_ordered_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_ordered_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_ordered_channel_physical(L);
    p.native_ordered_generator_ref=f->generators_ref;p.native_ordered_generator_table=f->generators;
    p.native_ordered_epoch=f->epoch;p.native_ordered_writes=f->writes;p.native_ordered_uncertainty=f->uncertainty;
    lua_pop(L,3);
}
static void native_ordered_register_group(lua_State* L,Job& job,std::uint32_t registrar,std::size_t begin,std::size_t end){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||begin>=end||end>native_ordered_specs.size()||f->ordered_assigned_count!=begin)
        return (void)luaL_error(L,"ordered Events actual native registrar order/replay refused");
    native_ordered_channel_physical(L);
    for(std::size_t i=begin;i<end;++i){
        if(native_ordered_specs[i].registrar!=registrar||p.native_ordered_channels[i].created||p.native_ordered_channels[i].assigned)
            return (void)luaL_error(L,"ordered Events genuine original registrar/constructor changed");
        native_events_prefix_full(L,job);native_ordered_channel_physical(L);
        if(i)native_ordered_current(L,i-1);else native_game_view_current(L);
        native_ordered_name(L,i);lua_pushcclosure(L,native_ordered_constructors[i],0);
        const void*const constructor=lua_topointer(L,-1);p.native_ordered_channels[i].created=constructor;
        native_events_prefix_full(L,job);native_ordered_channel_physical(L);
        if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||lua_tocfunction(L,-1)!=native_ordered_constructors[i]||lua_getupvalue(L,-1,1))
            return (void)luaL_error(L,"ordered Events genuine native zero-upvalue closure changed after FULL");
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);lua_rawset(L,-3);
        p.native_ordered_channels[i].assigned=constructor;++f->ordered_assigned_count;
        native_events_prefix_full(L,job);native_ordered_current(L,i);lua_pop(L,1);
    }
}
