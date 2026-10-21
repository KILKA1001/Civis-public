// Inside actual Host Impl. Canonical manager registration ports4A31F0's
// append of the SAME actual coroutine. Never schedules a synthetic callback.
std::deque<lua_State*> native_context_active_threads;
static int native_context_gc_step500(lua_State* child){
    (void)lua_gc(child,LUA_GCSTEP,500);return 0;
}
static void perform_native_context_register_gc(lua_State* L,Job& job){
    auto& p=owner(L);p.worldview_require_callback_origin();
    if(!p.worldview_native_host||p.worldview_native_host->child!=job.worldview_child||
       job.worldview_child>=p.child_contexts.size())
        return (void)luaL_error(L,"Context4A31F0 real child scope absent");
    auto& child=p.child_contexts[job.worldview_child];
    if(!child.state_prepared||!child.loaded||child.loading||child.retired||
       child.state_registered||child.state_gc_returned||child.execution_source==graph_no_owner)
        return (void)luaL_error(L,"Context4A31F0 requires original compiled load return and cannot replay");
    lua_rawgeti(L,LUA_REGISTRYINDEX,child.execution_thread);lua_State* thread=lua_tothread(L,-1);
    if(!thread||thread==p.state||lua_status(thread)||lua_gettop(thread))
        return (void)luaL_error(L,"Context4A31F0 genuine idle coroutine unavailable");
    for(const auto* registered:p.native_context_active_threads)if(registered==thread)
        return (void)luaL_error(L,"Context4A31F0 actual coroutine already registered");
    p.native_context_active_threads.push_back(thread);child.state_registered=true;
    // Import9C9C8C proves original457747 is lua_gc, not lua_sethook:
    // ESI comes from original45739A Context518->4: SAME child coroutine,
    // not EBX root-manager state. __gc may error/reenter on that child, so
    // its allocating C closure/GC body has its own protected child boundary.
    const int gc_status=lua_cpcall(thread,native_context_gc_step500,nullptr);
    if(gc_status){lua_xmove(thread,L,1);return (void)lua_error(L);}
    p.worldview_require_callback_origin();child.state_gc_returned=true;
}
static void perform_native_context_initial_state_query(lua_State* L,Job& job){
    auto& p=owner(L);perform_native_context_state_query(L,job);
    const auto& child=p.child_contexts[job.worldview_child];
    if(!child.loaded||child.loading||!child.state_registered||!child.state_gc_returned||
       child.execution_source==graph_no_owner||child.execution_source>=p.graph_compiled_sources.size())
        return (void)luaL_error(L,"Context original compiled/registration/GC return absent");
    const auto& compiled=p.graph_compiled_sources[child.execution_source];
    if(compiled.context!=child.name)
        return (void)luaL_error(L,"Context initial compiled source belongs to another child");
    std::size_t found=0;for(const auto* thread:p.native_context_active_threads)
        if(thread==job.context_state_thread)++found;
    if(found!=1)return (void)luaL_error(L,"Context canonical manager registration missing/duplicated");
    p.child_require_current(job.worldview_child);p.worldview_require_callback_origin();
    child.source->require_captured_bytes(child.bytes);p.worldview_require_callback_origin();
}
