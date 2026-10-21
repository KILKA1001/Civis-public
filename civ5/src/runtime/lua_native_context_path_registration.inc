// Inside Host::Impl. Fresh PE667CB0 is320bytes, eleven methods, hash hint5.
// Registration creates actual zero-upvalue closures. Original method invocations
// have no issued canonical body/backend here and fail honestly at invocation.
template<std::uint32_t NativeFunction> static int native_context_path_method(lua_State* L){
    native_db_query_current(L);auto& p=owner(L);p.record_failure();
    return luaL_error(L,"Path original consumer %08X requires its genuine canonical method body/current owner; unissued",static_cast<unsigned>(NativeFunction));
}
struct NativePathEntry {const char* name;std::uint32_t original_function;lua_CFunction function;};
static constexpr NativePathEntry native_path_entries[]={
    {"GetDirectoryName",0x573f90u,native_context_path_method<0x573f90u>},
    {"GetExtension",0x5644c0u,native_context_path_method<0x5644c0u>},
    {"GetFileName",0x554100u,native_context_path_method<0x554100u>},
    {"GetFileNameWithoutExtension",0x6af830u,native_context_path_method<0x6af830u>},
    {"GetAltDirectorySeparatorChar",0x628b90u,native_context_path_method<0x628b90u>},
    {"GetDirectorySeparatorChar",0x4ab5f0u,native_context_path_method<0x4ab5f0u>},
    {"GetExtensionSeparatorChar",0x666c40u,native_context_path_method<0x666c40u>},
    {"GetPathSeparatorChar",0x4de200u,native_context_path_method<0x4de200u>},
    {"GetVolumeSeparatorChar",0x7a0da0u,native_context_path_method<0x7a0da0u>},
    {"NormalizePath",0x5a6c10u,native_context_path_method<0x5a6c10u>},
    {"UsesExtension",0x72fce0u,native_context_path_method<0x72fce0u>},
};

struct NativeContextPathRegistrationFrame final {
    Impl& host;const NativeContextMatchmakingFrame*const predecessor;
    const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;
    int keys_ref=LUA_NOREF;const void* keys=nullptr;
    int master_ref=LUA_NOREF,proxy_ref=LUA_NOREF;const void* master=nullptr;const void* proxy=nullptr;const void* meta=nullptr;
    std::size_t fields_created=0,stores=0;Job* actual_job=nullptr;
    explicit NativeContextPathRegistrationFrame(Impl& p):host(p),predecessor(p.matchmaking_frame),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(host.path_registration_frame||!predecessor)throw ScriptError("Path registrar requires actual Matchmaking predecessor");
        predecessor->first();if(predecessor->stores!=1||predecessor->master_ref<0||predecessor->proxy_ref<0||
           !predecessor->master||!predecessor->proxy||!predecessor->actual_job)
            throw ScriptError("Path original Matchmaking predecessor did not execute original master/copies");
        host.path_registration_frame=this;
    }
    ~NativeContextPathRegistrationFrame(){if(host.path_registration_frame==this)host.path_registration_frame=nullptr;}
    void first()const {
        if(host.path_registration_frame!=this||creator!=std::this_thread::get_id()||creator!=host.ui_thread||!predecessor||host.matchmaking_frame!=predecessor||
           epoch!=host.failure_epoch||writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty||host.failed.exhausted_)
            throw ScriptError("Path registrar actual source/frame changed");
        predecessor->first();
    }
    void full()const {first();predecessor->full();first();}
};
static void native_path_key(lua_State* L,int i){
    const auto& p=owner(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_path_registration_keys_ref);lua_rawgeti(L,-1,i);lua_remove(L,-2);
}
static int native_path_master_validation(lua_State* L){
    auto*const job=static_cast<Job*>(lua_touserdata(L,1));
    if(!job||L!=owner(L).state)return luaL_error(L,"Path master validator actual Job/VM changed");
    native_context_path_registration_full(L,*job);return 0;
}
static void native_context_path_registration_full(lua_State* L,Job& job){
    auto& p=owner(L);
    if(L!=p.state){
        if(!p.state||G(L)!=G(p.state)||!lua_checkstack(p.state,32))
            return (void)luaL_error(L,"Path actual master validation VM/stack unavailable");
        // The selected callback has another gt. Protect the genuine master
        // predecessor/cache check on its own VM, then propagate an actual error.
        const int master_top=lua_gettop(p.state);
        const int status=lua_cpcall(p.state,native_path_master_validation,&job);
        if(status!=0){lua_xmove(p.state,L,1);return (void)lua_error(L);}
        if(lua_gettop(p.state)!=master_top)return (void)luaL_error(L,"Path master validation stack changed");
        return;
    }
    auto*const f=p.path_registration_frame;p.source_exception={};p.source_error=nullptr;
    try{if(!f)throw ScriptError("Path registrar has no genuine entered frame");f->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Path registrar SAME source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    if(f->actual_job&&f->actual_job!=&job)return (void)luaL_error(L,"Path registrar actual entered Job replaced");
    if(job.owner!=&p||job.operation!=Operation::native_context_path_registration||!p.calling||!p.state||G(L)!=G(p.state))
        return (void)luaL_error(L,"Path registrar actual protected Job/VM differs");
    if(!f->predecessor->actual_job)return (void)luaL_error(L,"Path genuine Matchmaking predecessor Job retired");
    native_matchmaking_registration_full(L,*f->predecessor->actual_job);
    p.source_exception={};p.source_error=nullptr;
    try{if(p.path_registration_frame!=f)throw ScriptError("Path frame replaced after source callbacks");f->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Path registrar FIRST after callbacks refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    const int top=lua_gettop(L);
    if(f->keys_ref!=LUA_NOREF){
        if(f->keys_ref!=p.native_path_registration_keys_ref)return (void)luaL_error(L,"Path registrar keys custody changed");
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->keys||lua_objlen(L,-1)!=12||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Path registrar retained key table changed");
        for(int i=0;i<12;++i){
            lua_rawgeti(L,-1,i+1);std::size_t n=0;const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
            if(!key||n!=std::strlen(i==0?"Path":native_path_entries[i-1].name)||std::memcmp(key,i==0?"Path":native_path_entries[i-1].name,n))
                return (void)luaL_error(L,"Path registrar retained key TString changed");lua_pop(L,1);
        }lua_pop(L,1);
    }
    if(f->master_ref>=0){
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->master)return (void)luaL_error(L,"Path actual master table changed");
        const int master=lua_gettop(L);
        if(lua_getmetatable(L,master))return (void)luaL_error(L,"Path actual constructor master gained a metatable");
        for(int i=0;i<11;++i){native_path_key(L,i+2);lua_rawget(L,master);
            if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_path_entries[i].function||lua_getupvalue(L,-1,1))
                return (void)luaL_error(L,"Path cached original zero-upvalue method changed");lua_pop(L,1);}
        native_path_key(L,1);lua_rawget(L,LUA_GLOBALSINDEX);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Path original master cache changed");lua_pop(L,2);
    }
    if(f->stores){
        if(f->stores!=1||f->proxy_ref<0||!f->proxy||!f->meta||f->master_ref<0)
            return (void)luaL_error(L,"Path selected store lacks actual return");
        lua_rawgeti(L,LUA_REGISTRYINDEX,p.lua_events_frame->environment);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.lua_events_frame->actual_environment||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Path actual selected fenv changed");
        native_path_key(L,1);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->proxy_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->proxy||!lua_rawequal(L,-1,-2)||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=f->meta)
            return (void)luaL_error(L,"Path selected proxy/ref/metatable changed");
        native_api_prefix_key(L,2);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Path proxy actual master __index changed");lua_pop(L,2);
        native_api_prefix_key(L,3);lua_rawget(L,-2);
        if(lua_type(L,-1)!=LUA_TBOOLEAN||lua_toboolean(L,-1))return (void)luaL_error(L,"Path proxy original false __metatable changed");lua_pop(L,1);
        native_api_prefix_key(L,4);lua_rawget(L,-2);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_api_readonly_write||lua_getupvalue(L,-1,1))
            return (void)luaL_error(L,"Path proxy original write-error consumer changed");
    }
    lua_settop(L,top);
}
static int native_path_factory(lua_State* L){
    auto& p=owner(L);auto*const f=p.path_registration_frame;
    if(!f||!f->actual_job)return luaL_error(L,"Native Path667CB0 constructor lacks its actual entered Job");
    auto& job=*f->actual_job;
    native_context_path_registration_full(L,job);
    if(!f||f->fields_created||f->master_ref!=LUA_NOREF)return luaL_error(L,"Native Path667CB0 constructor replay refused");
    lua_createtable(L,0,5);const int table=lua_gettop(L);const void*const value=lua_topointer(L,table);native_context_path_registration_full(L,job);
    for(int i=0;i<11;++i){
        native_path_key(L,i+2);lua_pushcfunction(L,native_path_entries[i].function);lua_rawset(L,table);++f->fields_created;native_context_path_registration_full(L,job);
    }
    if(f->fields_created!=11||!lua_istable(L,table)||lua_topointer(L,table)!=value||lua_getmetatable(L,table))
        return luaL_error(L,"Native Path667CB0 actual namespace constructor changed");
    for(int i=0;i<11;++i){native_path_key(L,i+2);lua_rawget(L,table);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_path_entries[i].function||lua_getupvalue(L,-1,1))return luaL_error(L,"Path original zero-upvalue consumer changed");lua_pop(L,1);}
    return 1;
}
static void perform_native_context_path_registration(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.path_registration_frame;native_context_path_registration_full(L,job);
    if(!f||f->stores||f->keys_ref!=LUA_NOREF||f->actual_job||!lua_checkstack(L,32))
        return (void)luaL_error(L,"Path4F73C0 original registrar replay/key custody/stack refused");
    f->actual_job=&job;
    if(p.native_path_registration_keys_ref==LUA_NOREF){
        lua_createtable(L,12,0);const int keys=lua_gettop(L);const void*const key_value=lua_topointer(L,keys);native_context_path_registration_full(L,job);
        for(int i=0;i<12;++i){lua_pushstring(L,i==0?"Path":native_path_entries[i-1].name);lua_rawseti(L,keys,i+1);native_context_path_registration_full(L,job);}
        lua_pushvalue(L,keys);const int key_ref=luaL_ref(L,LUA_REGISTRYINDEX);
        if(key_ref<0)return (void)luaL_error(L,"Path actual retained key allocation failed");
        f->keys_ref=key_ref;f->keys=key_value;p.native_path_registration_keys_ref=key_ref;p.native_path_registration_keys_pointer=key_value;
        native_context_path_registration_full(L,job);lua_pop(L,1);
    }else{
        f->keys_ref=p.native_path_registration_keys_ref;f->keys=p.native_path_registration_keys_pointer;native_context_path_registration_full(L,job);
    }
    // Original6DFB60/5605C0 selects actual518 before inspecting
    // GLOBALS.Path; a cache miss runs667CB0 zero args/one result.
    perform_native_context_state_query(L,job);native_context_path_registration_full(L,job);const int environment=lua_gettop(L);
    if(lua_topointer(L,environment)!=p.lua_events_frame->actual_environment)
        return (void)luaL_error(L,"Path registrar actual prepared518 environment changed");
    native_path_key(L,1);lua_gettable(L,LUA_GLOBALSINDEX);native_context_path_registration_full(L,job);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);lua_pushcfunction(L,native_path_factory);lua_call(L,0,1);native_context_path_registration_full(L,job);
        native_path_key(L,1);lua_pushvalue(L,-2);lua_settable(L,LUA_GLOBALSINDEX);native_context_path_registration_full(L,job);
    }
    if(!lua_istable(L,-1))return (void)luaL_error(L,"Path original namespace master is not a table");
    f->master=lua_topointer(L,-1);lua_pushvalue(L,-1);f->master_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_context_path_registration_full(L,job);
    native_api_readonly_proxy(L,job);native_context_path_registration_full(L,job);const int proxy=lua_gettop(L);f->proxy=lua_topointer(L,proxy);
    if(!lua_getmetatable(L,proxy))return (void)luaL_error(L,"Path original49B190 proxy has no metatable");f->meta=lua_topointer(L,-1);lua_pop(L,1);
    native_path_key(L,1);lua_pushvalue(L,proxy);lua_settable(L,environment);native_context_path_registration_full(L,job);
    lua_pushvalue(L,proxy);f->proxy_ref=luaL_ref(L,LUA_REGISTRYINDEX);++f->stores;native_context_path_registration_full(L,job);
    job.context_path_master_reference=f->master_ref;job.context_path_proxy_reference=f->proxy_ref;
    job.context_path_table=f->master;job.context_path_proxy=f->proxy;lua_pop(L,2);
}
