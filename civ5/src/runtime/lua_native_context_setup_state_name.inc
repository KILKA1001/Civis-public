// Inside actual Host::Impl, after genuine prepared-state helpers. All owning
// arguments live in Job/Host outside the Lua longjmp frame. No local C++ RAII.
static void native_setup_state_name_full(lua_State* L,Job& job){
    auto& p=owner(L);p.source_exception={};p.source_error=nullptr;
    try{
        p.worldview_require_callback_origin();
        const auto* scope=p.worldview_native_host;
        if(!scope||!job.context_setup_name_loan||!job.context_setup_preparation)
            throw ScriptError("StateName actual load-name/preparation lender absent");
        job.context_setup_name_loan->require_current_without_callbacks(*scope->loan,job.context_setup_preparation);
        scope->loan->require_current();p.worldview_require_callback_origin();
        if(!p.worldview_native_child_controls_current(job.worldview_child))
            throw ScriptError("StateName SAME native child/source control changed");
        p.child_require_current(job.worldview_child);p.worldview_require_callback_origin();
        job.context_setup_name_loan->require_current_owned(*scope->loan,job.context_setup_preparation);
        p.worldview_require_callback_origin();
        job.context_setup_name_loan->require_current_without_callbacks(*scope->loan,job.context_setup_preparation);
        if(scope->child!=job.worldview_child||job.worldview_child>=p.child_contexts.size())
            throw ScriptError("StateName SAME actual child scope changed");
        const auto& child=p.child_contexts[job.worldview_child];
        if(!child.state_prepared||child.retired||child.loading||child.loaded||!child.setup_state_name_attempted)
            throw ScriptError("StateName actual cold setup child changed");
    }catch(...){p.source_exception=std::current_exception();p.source_error="StateName load-name/Host/Source full POST refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}
static void perform_native_context_setup_state_name(lua_State* L,Job& job){
    auto& p=owner(L);native_setup_state_name_full(L,job);
    perform_native_context_state_query(L,job);native_setup_state_name_full(L,job);
    // Query has the genuine coroutine globals/table on top. Retain exactly
    // that actual table, matching8F8540(globals) before key8F8B10.
    lua_pushvalue(L,-1);native_setup_state_name_full(L,job);
    p.child_contexts[job.worldview_child].setup_state_name_table_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_setup_state_name_full(L,job);
    lua_pushstring(L,"StateName");native_setup_state_name_full(L,job);
    p.child_contexts[job.worldview_child].setup_state_name_key_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_setup_state_name_full(L,job);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[job.worldview_child].setup_state_name_table_ref);
    native_setup_state_name_full(L,job);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[job.worldview_child].setup_state_name_key_ref);
    native_setup_state_name_full(L,job);
    if(!lua_istable(L,-2)||lua_type(L,-1)!=LUA_TSTRING)
        return (void)luaL_error(L,"StateName actual table/key refs changed");
    lua_pushlstring(L,job.context_state_name.data(),job.context_state_name.size());
    native_setup_state_name_full(L,job);
    lua_rawset(L,-3);native_setup_state_name_full(L,job);
    lua_pop(L,1);native_setup_state_name_full(L,job);
    // Real8F8820 key THEN table; partial keeper fields are retired, not issued
    // to a caller. Any failing cut poisons actual Host and cannot publish return.
    luaL_unref(L,LUA_REGISTRYINDEX,p.child_contexts[job.worldview_child].setup_state_name_key_ref);
    p.child_contexts[job.worldview_child].setup_state_name_key_ref=LUA_NOREF;
    native_setup_state_name_full(L,job);
    luaL_unref(L,LUA_REGISTRYINDEX,p.child_contexts[job.worldview_child].setup_state_name_table_ref);
    p.child_contexts[job.worldview_child].setup_state_name_table_ref=LUA_NOREF;
    native_setup_state_name_full(L,job);
}
static void perform_native_context_setup_state_name_query(lua_State* L,Job& job){
    auto& p=owner(L);native_setup_state_name_full(L,job);
    perform_native_context_state_query(L,job);native_setup_state_name_full(L,job);
    lua_pushstring(L,"StateName");native_setup_state_name_full(L,job);
    lua_rawget(L,-2);native_setup_state_name_full(L,job);
    if(lua_type(L,-1)!=LUA_TSTRING)return (void)luaL_error(L,"StateName actual global no longer string");
    std::size_t length=0;const char* bytes=lua_tolstring(L,-1,&length);
    // Actual Lua-owned immutable STRING bytes are read immediately; no full
    // callback occurs between acquiring this pointer and its nonalloc compare.
    if(!bytes||length!=job.context_state_name.size()||std::memcmp(bytes,job.context_state_name.data(),length))
        return (void)luaL_error(L,"StateName actual global differs from native basename");
    native_setup_state_name_full(L,job);job.context_state_name_matched=true;
}
