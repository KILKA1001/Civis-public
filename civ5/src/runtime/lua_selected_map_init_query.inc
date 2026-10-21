// Included inside Impl. Job owns the output outside both protected Lua frames.
// This data query supplies no selected map, gameplay or UI activation capability.
void map_init_require_current(const Job &job) {
    if(failed||(ui_context_closed&&job.context==ui_layout.context_id)||!context_sources||context_sources.get()!=job.map_source_owner||
       job.map_slot>=child_contexts.size()||failure_epoch!=job.map_epoch)
        throw ScriptError("Map init query owner retired or rebound");
    const auto &entry=child_contexts[job.map_slot];
    if(entry.name!=job.context||entry.loading||entry.environment==LUA_NOREF||
       entry.receiver==LUA_NOREF||entry.execution_thread==LUA_NOREF||(entry.source?!entry.loaded:!executed_contexts.contains(entry.name)))
        throw ScriptError("Map init query context has not loaded");
    if(entry.execution_source>=graph_compiled_sources.size()||graph_compiled_sources[entry.execution_source].context!=entry.name)
        throw ScriptError("Map init query compiled source identity is unavailable");
    update_require_context_current(job.map_slot);
    if(failed||failure_epoch!=job.map_epoch)throw ScriptError("Map init query caught host reentry");
}
static void map_init_require_bindings(lua_State *L,const Job &job) {
    child_require_bindings(L);
    if(!lua_checkstack(L,3))return (void)luaL_error(L,"Map init binding stack unavailable");
    const auto &entry=job.owner->child_contexts[job.map_slot];
    lua_rawgeti(L,LUA_REGISTRYINDEX,entry.execution_thread);
    const bool same_thread=lua_tothread(L,-1)==L;lua_pop(L,1);
    lua_pushvalue(L,LUA_GLOBALSINDEX);lua_rawgeti(L,LUA_REGISTRYINDEX,entry.environment);
    const bool same_environment=lua_rawequal(L,-1,-2);lua_pop(L,2);
    if(!same_thread||!same_environment)return (void)luaL_error(L,"Map init retained execution thread/environment rebound");
}
static int release_retired_map_contexts(lua_State *L) {
    auto &self=owner(L);
    for(auto &entry:self.child_contexts)if(entry.retired&&entry.execution_thread!=LUA_NOREF) {
        luaL_unref(L,LUA_REGISTRYINDEX,entry.execution_thread);entry.execution_thread=LUA_NOREF;
    }
    return release_retired_updates(L);
}
static std::int32_t map_init_integer(lua_State *L,int index) {
    const auto value=source_integer(L,index);
    if(value<std::numeric_limits<std::int32_t>::min()||value>std::numeric_limits<std::int32_t>::max())
        luaL_error(L,"Map init integer is outside the supported native int32 range");
    return static_cast<std::int32_t>(value);
}
static int map_init_query_body(lua_State *L) {
    auto &job=*static_cast<Job*>(lua_touserdata(L,1));auto &self=*job.owner;
    // Only references/scalars survive Lua longjmp; provider exceptions are
    // retained in Job and rethrown by call outside the Lua protection boundary.
    try {
        if(!lua_checkstack(L,12))return luaL_error(L,"Map init protected target stack unavailable");
        self.map_init_require_current(job);map_init_require_bindings(L,job);
        self.graph_execution_scope=self.child_contexts[job.map_slot].execution_source;
        lua_getglobal(L,"GetMapInitData");
        self.map_init_require_current(job);map_init_require_bindings(L,job);
        if(!lua_isfunction(L,-1))return 0;
        self.graph_execution_scope=self.graph_compiled_owner(L,-1);
        lua_pushinteger(L,static_cast<lua_Integer>(job.map_world_id));lua_call(L,1,1);
        self.map_init_require_current(job);map_init_require_bindings(L,job);
        if(!lua_istable(L,-1))return 0;
        const int table=lua_gettop(L);
        for(int field=0;field<6;++field) {
            const char *key=field==0?"Width":field==1?"Height":field==2?"WrapX":
                field==3?"WrapY":field==4?"TopLatitude":"BottomLatitude";
            lua_getfield(L,table,key); // real __index, in the SDK field order
            self.map_init_require_current(job);map_init_require_bindings(L,job);
            if(!lua_isnil(L,-1))switch(field) {
            case 0:job.map_result.fields.width=map_init_integer(L,-1);break;
            case 1:job.map_result.fields.height=map_init_integer(L,-1);break;
            case 2:job.map_result.fields.wrap_x=lua_toboolean(L,-1)!=0;break;
            case 3:job.map_result.fields.wrap_y=lua_toboolean(L,-1)!=0;break;
            case 4:job.map_result.fields.top_latitude=map_init_integer(L,-1);break;
            case 5:job.map_result.fields.bottom_latitude=map_init_integer(L,-1);break;
            }
            lua_pop(L,1);
        }
        self.map_init_require_current(job);map_init_require_bindings(L,job);
        job.map_result.table_result=true;
        return 0;
    }catch(...) {job.exception=std::current_exception();}
    return 0; // call rethrows the retained exception after all Lua C frames exit
}
static int perform_map_init_query(lua_State *L,Job &job) {
    auto &self=*job.owner;self.map_init_require_current(job);
    if(!lua_checkstack(L,4))return luaL_error(L,"Map init query stack unavailable");
    lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[job.map_slot].execution_thread);
    lua_State *thread=lua_tothread(L,-1);
    if(!thread||G(thread)!=G(L)||lua_status(thread)||lua_gettop(thread))return luaL_error(L,"Map init retained thread is not SAME and idle");
    // Allocate the C closure on L's cpcall-protected stack. Only after that
    // possible GC do we inspect the target's two-slot transfer capacity.
    lua_pushcfunction(L,map_init_query_body);lua_pushlightuserdata(L,&job);
    // lua_checkstack can allocate: target growth belongs inside its own pcall.
    if(thread->ci!=thread->base_ci||thread->stack_last-thread->top<2||thread->ci->top-thread->top<2)
        return luaL_error(L,"Map init retained thread has no idle transfer capacity");
    lua_xmove(L,thread,2); // actual lapi.c only transfers existing values
    if(self.budget.instructions)lua_sethook(thread,hook,LUA_MASKCOUNT,1000);
    const int status=lua_pcall(thread,1,0,0);lua_sethook(thread,nullptr,0,0);
    self.graph_execution_scope=graph_no_owner;
    if(status){lua_xmove(thread,L,1);return lua_error(L);}
    lua_pop(L,1);
    if(job.exception)return 0;
    self.map_init_require_current(job);child_require_bindings(L);
    return 0;
}
