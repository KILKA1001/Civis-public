// Inside Host::Impl AFTER existing debug, readonly libraries, include and
// traceback producers. Existing GC-stopped prefix query remains unchanged.
static constexpr const char*const native_profile_restart_keys[]={
    "debug","coroutine","string","table","math","os","__index","__metatable","__newindex",
    "print","RefreshIncludeFileList","IncludeFileList","include","traceback"};
static void native_profile_restart_keys_physical_query(lua_State* L,Job& job){
    auto& p=owner(L);
    if(job.worldview_child>=p.child_contexts.size())
        return (void)luaL_error(L,"Profile retained key child absent");
    const auto reference=p.child_contexts[job.worldview_child].profile_restart_keys_ref;
    const auto pointer=p.child_contexts[job.worldview_child].profile_restart_keys_pointer;
    if(reference==LUA_NOREF||reference==LUA_REFNIL||!pointer)
        return (void)luaL_error(L,"Profile actual retained key keeper absent");
    lua_rawgeti(L,LUA_REGISTRYINDEX,reference);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=pointer||lua_objlen(L,-1)!=14)
        return (void)luaL_error(L,"Profile actual retained key table/ref replaced");
    const int table=lua_gettop(L);
    for(int i=0;i<14;++i){
        lua_rawgeti(L,table,i+1);std::size_t n=0;
        const char* v=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        const char* original=native_profile_restart_keys[i];
        if(!v||n!=std::strlen(original)||std::memcmp(v,original,n))
            return (void)luaL_error(L,"Profile retained interned key changed");
        lua_pop(L,1);
    }
    lua_pop(L,1);
    // Real registry table retains all fourteen exact interned TString objects.
    // Source24's retained-key interning observation alone did not exclude
    // lua_pushlstring's luaC_checkGC. Successor25 final physical reads use only
    // actual keeper TValue via rawgeti, never any intern/GC-triggering push.
}
static void perform_native_profile_restart_key_keeper(lua_State* L,Job& job){
    auto& p=owner(L);native_debug_config_full(L,job);
    if(!p.profile_gc_stopped||job.operation!=Operation::native_context_profile_restart||
       p.child_contexts[job.worldview_child].profile_restart_keys_ref!=LUA_NOREF||
       p.child_contexts[job.worldview_child].profile_restart_keys_pointer)
        return (void)luaL_error(L,"Profile restart key producer phase/replay");
    if(!lua_checkstack(L,16))return (void)luaL_error(L,"Profile restart key stack reserve failed");
    native_debug_config_full(L,job);
    lua_createtable(L,14,0);native_debug_config_full(L,job);const int table=lua_gettop(L);
    for(int i=0;i<14;++i){
        lua_pushstring(L,native_profile_restart_keys[i]);native_debug_config_full(L,job);
        lua_rawseti(L,table,i+1);native_debug_config_full(L,job);
    }
    const void* pointer=lua_topointer(L,table);
    lua_pushvalue(L,table);native_debug_config_full(L,job);
    const int reference=luaL_ref(L,LUA_REGISTRYINDEX);native_debug_config_full(L,job);
    if(reference==LUA_NOREF||reference==LUA_REFNIL||!pointer)
        return (void)luaL_error(L,"Profile genuine key ref capture failed");
    p.child_contexts[job.worldview_child].profile_restart_keys_ref=reference;
    p.child_contexts[job.worldview_child].profile_restart_keys_pointer=pointer;
    native_debug_config_full(L,job);native_profile_restart_keys_physical_query(L,job);lua_pop(L,1);
}
static void native_profile_completed_include_physical_query(lua_State* L,int environment,std::size_t slot,int keys){
    auto& p=owner(L);
    // Caller has just passed LAST FULL. Nonallocating raw reads/upvalue inspection
    // use already retained live keys/closure and at most the earlier stack peak.
    if(slot>=p.child_contexts.size()||!p.child_contexts[slot].profile_include_child_control)
        return (void)luaL_error(L,"Selected include/traceback actual control absent");
    const auto* c=p.child_contexts[slot].profile_include_child_control.get();
    if(!p.worldview_native_host||p.worldview_native_host->child!=slot||!p.profile_include_master_alive||
       !p.include_configuration_alive||c->creator!=std::this_thread::get_id()||c->host.expired()||
       c->host.owner_before(p.weak_from_this())||p.weak_from_this().owner_before(c->host)||c->child!=slot||
       c->serial!=p.worldview_native_host->loan->operation_serial()||c->epoch!=p.failure_epoch||
       c->writes!=p.failed.writes_||c->uncertainty!=p.execution_uncertainty||
       c->list.get()!=p.profile_include_table_control.get()||
       c->list.owner_before(p.profile_include_table_control)||p.profile_include_table_control.owner_before(c->list)||
       c->function_ref!=p.child_contexts[slot].profile_include_ref||c->traceback_ref!=p.child_contexts[slot].profile_traceback_ref)
        return (void)luaL_error(L,"Selected include/traceback actual private owner changed");
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[slot].environment);
    if(!lua_istable(L,environment)||!lua_rawequal(L,environment,-1))
        return (void)luaL_error(L,"Selected include/traceback actual environment rebound");
    lua_pop(L,1);
    if(lua_getmetatable(L,environment))return (void)luaL_error(L,"Selected include unexpected master fallback");
    lua_rawgeti(L,keys,13);lua_rawget(L,environment);
    lua_rawgeti(L,LUA_REGISTRYINDEX,c->function_ref);
    if(!lua_isfunction(L,-1)||lua_iscfunction(L,-1)||lua_topointer(L,-1)!=c->function||!lua_rawequal(L,-1,-2)||
       static_cast<const Closure*>(lua_topointer(L,-1))->l.p!=c->proto||
       native_include_list_upvalue_without_callbacks(L,-1,c->list->table)!=c->list_upvalue)
        return (void)luaL_error(L,"Selected include actual factory Proto/list upvalue changed");
    lua_pop(L,2);lua_rawgeti(L,keys,14);lua_rawget(L,environment);
    lua_rawgeti(L,LUA_REGISTRYINDEX,c->traceback_ref);
    if(!lua_isfunction(L,-1)||lua_topointer(L,-1)!=c->traceback||!lua_rawequal(L,-1,-2))
        return (void)luaL_error(L,"Selected actual traceback delivery changed");
    lua_pop(L,2);
}
static void native_profile_completed_physical_query(lua_State* L,Job& job){
    auto& p=owner(L);native_profile_restart_keys_physical_query(L,job);
    // Retain the physically verified key TABLE on this actual Lua stack too.
    // Only rawgeti key TValue copies follow: no luaC_checkGC, interning or call.
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[job.worldview_child].profile_restart_keys_ref);
    const int keys=lua_gettop(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[job.worldview_child].environment);
    const int environment=lua_gettop(L);
    if(!lua_istable(L,environment)||lua_topointer(L,environment)!=job.context_state_environment||
       lua_getmetatable(L,environment))
        return (void)luaL_error(L,"Profile completed environment late change");
    lua_rawgeti(L,keys,1);lua_rawget(L,LUA_GLOBALSINDEX);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[job.worldview_child].profile_debug_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.profile_debug_master_table||!lua_rawequal(L,-1,-2))
        return (void)luaL_error(L,"Profile completed debug late change");
    lua_pop(L,2);
    for(std::size_t slot=0;slot<5;++slot){
        lua_rawgeti(L,keys,static_cast<int>(slot+2));lua_rawget(L,environment);
        lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[job.worldview_child].profile_library_ref[slot]);
        if(!lua_istable(L,-1)||!lua_rawequal(L,-1,-2)||!lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Profile completed readonly keeper late change");
        const int meta=lua_gettop(L);
        lua_rawgeti(L,keys,7);lua_rawget(L,meta);
        if(!lua_istable(L,-1))return (void)luaL_error(L,"Profile completed readonly source late change");
        lua_pop(L,1);lua_rawgeti(L,keys,8);lua_rawget(L,meta);
        if(lua_type(L,-1)!=LUA_TBOOLEAN||lua_toboolean(L,-1))
            return (void)luaL_error(L,"Profile completed readonly lock late change");
        lua_pop(L,1);lua_rawgeti(L,keys,9);lua_rawget(L,meta);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_profile_readonly_newindex)
            return (void)luaL_error(L,"Profile completed readonly callback late change");
        lua_pop(L,4);
    }
    lua_rawgeti(L,keys,10);lua_rawget(L,environment);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.profile_print_function_ref);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_master_print||!lua_rawequal(L,-1,-2))
        return (void)luaL_error(L,"Profile completed print late change");
    lua_pop(L,2);
    lua_rawgeti(L,keys,11);lua_rawget(L,LUA_GLOBALSINDEX);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.profile_include_refresh_ref);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_master_refresh_include_table||
       lua_topointer(L,-1)!=p.profile_include_refresh_pointer||!lua_rawequal(L,-1,-2))
        return (void)luaL_error(L,"Profile completed refresh late change");
    lua_pop(L,2);lua_rawgeti(L,keys,12);lua_rawget(L,LUA_GLOBALSINDEX);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.profile_include_table_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.profile_include_table_pointer||!lua_rawequal(L,-1,-2)||
       lua_objlen(L,-1)!=p.profile_include_names.size())
        return (void)luaL_error(L,"Profile completed ordered list late change");
    const int list=lua_gettop(L);
    for(std::size_t i=0;i<p.profile_include_names.size();++i){
        lua_rawgeti(L,list,static_cast<int>(i+1));
        std::size_t n=0;const char* v=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!v||n!=p.profile_include_names[i].size()||std::memcmp(v,p.profile_include_names[i].data(),n))
            return (void)luaL_error(L,"Profile completed ordered name late change");
        lua_pop(L,1);
    }
    lua_pop(L,2);native_profile_completed_include_physical_query(L,environment,job.worldview_child,keys);lua_pop(L,2);
}
static void perform_native_context_profile_completed_query(lua_State* L,Job& job){
    auto& p=owner(L);native_debug_config_full(L,job);
    if((job.operation!=Operation::native_context_profile_restart&&
        job.operation!=Operation::native_context_profile_completed_query)||
       p.profile_gc_stopped||job.worldview_child>=p.child_contexts.size()||
       p.child_contexts[job.worldview_child].profile_debug_ref==LUA_NOREF)
        return (void)luaL_error(L,"Profile completed query requires actual GC restart/child");
    perform_native_context_state_query(L,job);native_debug_config_full(L,job);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[job.worldview_child].profile_debug_ref);
    native_debug_config_full(L,job);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.profile_debug_master_table)
        return (void)luaL_error(L,"Profile completed retained debug producer changed");
    lua_getglobal(L,LUA_DBLIBNAME);native_debug_config_full(L,job);
    if(!lua_istable(L,-1)||!lua_rawequal(L,-1,-2))
        return (void)luaL_error(L,"Profile completed master debug replaced");
    lua_pop(L,2);query_native_profile_libraries(L,job);native_debug_config_full(L,job);
    query_native_master_include_table(L,job);native_debug_config_full(L,job);
    query_native_context_include_traceback_prefix(L,job);native_debug_config_full(L,job);
    // Last callback precedes a fresh physical retained table/closure/Proto
    // check. Existing query already earned this stack depth and live keys.
    native_profile_completed_physical_query(L,job);
    if(p.profile_gc_stopped)return (void)luaL_error(L,"Profile restarted GC phase changed");
    job.context_profile_debug_table=p.profile_debug_master_table;
    job.context_profile_debug_matched=true;
}
static void perform_native_context_profile_restart(lua_State* L,Job& job){
    auto& p=owner(L);native_debug_config_full(L,job);
    if(job.operation!=Operation::native_context_profile_restart||!p.profile_gc_stopped)
        return (void)luaL_error(L,"Profile GC restart foreign phase/replay");
    // This is the real prior prefix; its query still requires GCSTOP.
    perform_native_context_debug_config_query(L,job);native_debug_config_full(L,job);
    perform_native_profile_restart_key_keeper(L,job);native_debug_config_full(L,job);
    native_profile_restart_keys_physical_query(L,job);
    if(!p.profile_gc_stopped)return (void)luaL_error(L,"Profile GCSTOP changed before restart");
    // Original4EC639 ->lua_gc(L,LUA_GCRESTART,0), after519700 and6538A0.
    if(lua_gc(L,LUA_GCRESTART,0)!=0)
        return (void)luaL_error(L,"Profile actual GC restart returned unexpected result");
    p.profile_gc_stopped=false;
    native_debug_config_full(L,job);
    perform_native_context_profile_completed_query(L,job);
}
