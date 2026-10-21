// Inside actual Host::Impl. Job owns C++ state outside protected Lua frame.
static void native_debug_config_full(lua_State* L,Job& job){
    native_setup_state_name_full(L,job);auto& p=owner(L);
    p.source_exception={};p.source_error=nullptr;
    try{
        const auto actual=p.child_contexts[job.worldview_child].setup_state_name_return.lock();
        if(!actual||actual.get()!=job.context_profile_state_name.get()||
           actual.owner_before(job.context_profile_state_name)||job.context_profile_state_name.owner_before(actual)||
           !p.child_contexts[job.worldview_child].profile_debug_attempted||
           !p.profile_debug_master_alive||p.profile_debug_master_ref==LUA_NOREF||
           !p.profile_debug_master_table)
            throw ScriptError("Profile debug/config SAME actual StateName publication changed");
        if(job.context_profile_config){
            job.context_profile_config->require_current_owned(*p.worldview_native_host->loan);
            p.worldview_require_callback_origin();
            job.context_profile_config->require_current_without_callbacks(*p.worldview_native_host->loan);
        }
    }catch(...){p.source_exception=std::current_exception();p.source_error="Profile debug/config lender/config full POST refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}
static void native_profile_capture_config(lua_State* L,Job& job){
    auto& p=owner(L);native_debug_config_full(L,job);
    p.source_exception={};p.source_error=nullptr;
    try{
        const auto* scope=p.worldview_native_host;
        // No path/value/caller flag; real active Source creates selected INI loan.
        job.context_profile_config=scope->loan->context_lua_profile_config_owned();
        p.worldview_require_callback_origin();
        job.context_profile_config->require_current_owned(*scope->loan);
        job.context_profile_debug=job.context_profile_config->debug_library_value_owned(*scope->loan);
        job.context_profile_config->require_current_without_callbacks(*scope->loan);
        p.worldview_require_callback_origin();
    }catch(...){p.source_exception=std::current_exception();p.source_error="Profile required selected INI operand unavailable";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    native_debug_config_full(L,job);
}
static void perform_native_context_debug_config_prefix(lua_State* L,Job& job){
    auto& p=owner(L);native_debug_config_full(L,job);
    if(p.profile_gc_stopped||p.child_contexts[job.worldview_child].profile_debug_ref!=LUA_NOREF)
        return (void)luaL_error(L,"Profile genuine GC/debug partial prefix replay");
    // Actual4EC473->8F97F0->IAT9C9C8C is lua_gc, never lua_createtable.
    lua_gc(L,LUA_GCSTOP,0);p.profile_gc_stopped=true;
    native_debug_config_full(L,job);
    perform_native_context_state_query(L,job);native_debug_config_full(L,job);
    native_profile_capture_config(L,job);native_debug_config_full(L,job);
    if(job.context_profile_debug!=0&&job.context_profile_debug!=1)
        return (void)luaL_error(L,"Profile actual selected debug operand unsupported");
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.profile_debug_master_ref);native_debug_config_full(L,job);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.profile_debug_master_table)
        return (void)luaL_error(L,"Profile genuine master debug retained namespace changed");
    lua_pushvalue(L,-1);native_debug_config_full(L,job);
    p.child_contexts[job.worldview_child].profile_debug_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_debug_config_full(L,job);
    perform_native_context_profile_restricted_libraries(L,job);native_debug_config_full(L,job);
    perform_native_context_master_include_table_prefix(L,job);native_debug_config_full(L,job);
    perform_native_context_include_traceback_prefix(L,job);native_debug_config_full(L,job);
    // Actual include/list/traceback prefixes retained; normal GC restart unissued.
    // Native normal GC restart is4EC639 AFTER include519700/traceback6538A0.
    // This typed partial prefix retains stopped GC; it cannot continue RootRead.
}
static void perform_native_context_debug_config_query(lua_State* L,Job& job){
    auto& p=owner(L);native_debug_config_full(L,job);
    if(!p.profile_gc_stopped)return (void)luaL_error(L,"Profile partial GC-stop state retired");
    perform_native_context_state_query(L,job);native_debug_config_full(L,job);
    const auto ref=p.child_contexts[job.worldview_child].profile_debug_ref;
    if(ref==LUA_NOREF||ref==LUA_REFNIL)return (void)luaL_error(L,"Profile debug child keeper absent");
    lua_rawgeti(L,LUA_REGISTRYINDEX,ref);native_debug_config_full(L,job);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.profile_debug_master_table||
       lua_topointer(L,-1)==job.context_state_environment)
        return (void)luaL_error(L,"Profile actual debug table identity changed");
    job.context_profile_debug_table=lua_topointer(L,-1);
    lua_getglobal(L,LUA_DBLIBNAME);native_debug_config_full(L,job);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=job.context_profile_debug_table)
        return (void)luaL_error(L,"Profile debug global diverged from genuine master publication");
    query_native_profile_libraries(L,job);native_debug_config_full(L,job);
    query_native_master_include_table(L,job);native_debug_config_full(L,job);
    query_native_context_include_traceback_prefix(L,job);native_debug_config_full(L,job);
    native_debug_config_full(L,job);job.context_profile_debug_matched=true;
}
