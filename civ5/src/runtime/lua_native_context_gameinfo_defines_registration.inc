// Complete the remaining two original5C7880 registrations in order.
struct NativeContextDatabaseNamespacesFrame final {
    Impl& host;const NativeContextDbRegistrationFrame*const predecessor;
    const std::thread::id creator=std::this_thread::get_id();const std::uint64_t epoch,writes,uncertainty;
    struct Store {bool constructor_returned=false;int master_ref=LUA_NOREF,proxy_ref=LUA_NOREF;const void* master=nullptr;const void* proxy=nullptr;const void* meta=nullptr;};
    std::array<Store,2> stores{};std::size_t assigned=0;
    int keys_ref=LUA_NOREF;const void* keys=nullptr;Job* actual_job=nullptr;
    explicit NativeContextDatabaseNamespacesFrame(Impl& p):host(p),predecessor(p.db_registration_frame),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(host.database_namespaces_frame||!predecessor)throw ScriptError("GameInfo/GameDefines require their genuine DB predecessor");
        predecessor->first();if(predecessor->stores!=1||predecessor->master_ref<0||predecessor->proxy_ref<0)
            throw ScriptError("GameInfo/GameDefines DB predecessor did not store its actual readonly namespace");
        host.database_namespaces_frame=this;
    }
    ~NativeContextDatabaseNamespacesFrame(){if(host.database_namespaces_frame==this)host.database_namespaces_frame=nullptr;}
    void first()const {
        if(host.database_namespaces_frame!=this||creator!=std::this_thread::get_id()||!predecessor||host.db_registration_frame!=predecessor||
           epoch!=host.failure_epoch||writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty)
            throw ScriptError("GameInfo/GameDefines actual frame/source/failure changed");predecessor->first();
    }
    void full()const {first();predecessor->full();first();}
};
static constexpr const char*const native_database_namespace_keys[]={"GameInfo","GameDefines"};
static void native_database_namespace_key(lua_State* L,int i){
    const auto*const f=owner(L).database_namespaces_frame;
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,i);lua_remove(L,-2);
}
static void native_database_namespaces_full(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.database_namespaces_frame;p.source_exception={};p.source_error=nullptr;
    try{if(!f)throw ScriptError("GameInfo/GameDefines require actual entered frame");f->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="GameInfo/GameDefines source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    // The copied Job is a new real ordered registrar job; the DB predecessor
    // retains its own still-live prior Job and physical return for validation.
    if(!f->predecessor->actual_job)return (void)luaL_error(L,"GameInfo/GameDefines genuine DB predecessor Job retired");
    native_db_registration_full(L,*f->predecessor->actual_job);
    p.source_exception={};p.source_error=nullptr;
    try{if(p.database_namespaces_frame!=f)throw ScriptError("Database namespaces frame replaced after callbacks");f->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="GameInfo/GameDefines FIRST after callbacks refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    if(f->actual_job&&f->actual_job!=&job)return (void)luaL_error(L,"GameInfo/GameDefines actual Job replaced");
    const int top=lua_gettop(L);
    if(f->keys_ref!=LUA_NOREF){
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->keys||lua_objlen(L,-1)!=2||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Database namespaces retained keys changed");
        for(int i=0;i<2;++i){lua_rawgeti(L,-1,i+1);std::size_t n=0;const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
            if(!key||n!=std::strlen(native_database_namespace_keys[i])||std::memcmp(key,native_database_namespace_keys[i],n))
                return (void)luaL_error(L,"Database namespace actual key TString changed");lua_pop(L,1);}
        lua_pop(L,1);
    }
    if(f->assigned>2)return (void)luaL_error(L,"Database original namespace store order exceeded");
    for(std::size_t i=0;i<f->assigned;++i){
        const auto& s=f->stores[i];if(s.master_ref<0||s.proxy_ref<0||!s.master||!s.proxy||!s.meta)
            return (void)luaL_error(L,"Database namespace lacks actual completed store return");
        native_database_namespace_key(L,static_cast<int>(i+1));lua_rawget(L,LUA_GLOBALSINDEX);lua_rawgeti(L,LUA_REGISTRYINDEX,s.master_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=s.master||!lua_rawequal(L,-1,-2))
            return (void)luaL_error(L,"Database namespace actual master/cache/reference changed");lua_pop(L,2);
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->predecessor->predecessor->predecessor->environment);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->predecessor->predecessor->predecessor->actual_environment||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Database namespace actual selected fenv changed");
        native_database_namespace_key(L,static_cast<int>(i+1));lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,s.proxy_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=s.proxy||!lua_rawequal(L,-1,-2)||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=s.meta)
            return (void)luaL_error(L,"Database namespace actual selected readonly proxy changed");
        native_api_prefix_key(L,2);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,s.master_ref);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Database namespace readonly actual master __index changed");lua_pop(L,2);
        native_api_prefix_key(L,3);lua_rawget(L,-2);
        if(lua_type(L,-1)!=LUA_TBOOLEAN||lua_toboolean(L,-1))return (void)luaL_error(L,"Database namespace readonly false __metatable changed");lua_pop(L,1);
        native_api_prefix_key(L,4);lua_rawget(L,-2);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_api_readonly_write||lua_getupvalue(L,-1,1))
            return (void)luaL_error(L,"Database namespace actual write-error callback changed");
        lua_settop(L,top);
    }
    lua_settop(L,top);
}
static int native_gameinfo_factory(lua_State* L){
    auto& p=owner(L);auto*const f=p.database_namespaces_frame;
    if(!f||!f->actual_job||f->assigned!=0)return luaL_error(L,"GameInfo63EE70 constructor lacks its genuine ordered frame");
    auto& job=*f->actual_job;native_database_namespaces_full(L,job);
    if(luaL_loadbuffer(L,lua_native_gameinfo_ctor_bootstrap,sizeof(lua_native_gameinfo_ctor_bootstrap)-1,"=native-GameInfo63EE70-port"))return lua_error(L);
    native_database_namespaces_full(L,job);
    lua_pushcfunction(L,native_gameinfo_prepare);lua_pushcfunction(L,native_gameinfo_object);
    lua_pushlightuserdata(L,&p.gameplay); // actual immutable same-host DB backend
    lua_call(L,3,1);native_database_namespaces_full(L,job);
    if(!lua_isfunction(L,-1))return luaL_error(L,"GameInfo actual port factory did not return its genuine constructor");
    lua_call(L,0,1);native_database_namespaces_full(L,job);
    if(!lua_istable(L,-1))return luaL_error(L,"GameInfo63EE70 actual Database table constructor did not return table");
    native_database_namespaces_full(L,job);f->stores[0].constructor_returned=true;
    return 1;
}
static int native_game_defines_factory(lua_State* L){
    auto& p=owner(L);auto*const f=p.database_namespaces_frame;
    if(!f||!f->actual_job||f->assigned!=1)return luaL_error(L,"GameDefines606C30 constructor lacks its genuine ordered predecessor");
    auto& job=*f->actual_job;native_database_namespaces_full(L,job);
    lua_createtable(L,0,0);const int table=lua_gettop(L);const void*const value=lua_topointer(L,table);native_database_namespaces_full(L,job);
    lua_createtable(L,0,0);const int meta=lua_gettop(L);const void*const meta_value=lua_topointer(L,meta);native_database_namespaces_full(L,job);
    native_api_prefix_key(L,2);
    native_db_push_result(L,"SELECT Value FROM Defines WHERE Name = ? LIMIT 1");native_database_namespaces_full(L,job);
    if(lua_isnil(L,-1))return luaL_error(L,"GameDefines original mandatory Value query failed preparation");
    lua_pushcclosure(L,native_game_defines_index,1);native_database_namespaces_full(L,job);lua_rawset(L,meta);native_database_namespaces_full(L,job);
    if(!lua_istable(L,table)||lua_topointer(L,table)!=value||!lua_istable(L,meta)||lua_topointer(L,meta)!=meta_value)
        return luaL_error(L,"GameDefines original actual table/meta changed before assignment");
    native_api_prefix_key(L,2);lua_rawget(L,meta);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_game_defines_index||!lua_getupvalue(L,-1,1))
        return luaL_error(L,"GameDefines actual prepared __index capture missing");
    native_db_cell(L,-1);lua_pop(L,2);lua_setmetatable(L,table);native_database_namespaces_full(L,job);f->stores[1].constructor_returned=true;return 1;
}
static void native_database_namespace_register(lua_State* L,Job& job,int which,lua_CFunction ctor){
    auto& p=owner(L);auto*const f=p.database_namespaces_frame;native_database_namespaces_full(L,job);
    if(!f||which<0||which>1||f->assigned!=static_cast<std::size_t>(which))
        return (void)luaL_error(L,"Database namespace original registrar order/replay refused");
    perform_native_context_state_query(L,job);native_database_namespaces_full(L,job);const int environment=lua_gettop(L);
    if(lua_topointer(L,environment)!=f->predecessor->predecessor->predecessor->actual_environment)
        return (void)luaL_error(L,"Database namespace original same prepared518 selection changed");
    native_database_namespace_key(L,which+1);lua_gettable(L,LUA_GLOBALSINDEX);native_database_namespaces_full(L,job);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);lua_pushcfunction(L,ctor);lua_call(L,0,1);native_database_namespaces_full(L,job);
        native_database_namespace_key(L,which+1);lua_pushvalue(L,-2);lua_settable(L,LUA_GLOBALSINDEX);native_database_namespaces_full(L,job);
    }
    if(!lua_istable(L,-1))return (void)luaL_error(L,"Database namespace original master is not a table");
    auto& s=f->stores[static_cast<std::size_t>(which)];s.master=lua_topointer(L,-1);lua_pushvalue(L,-1);s.master_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_database_namespaces_full(L,job);native_api_readonly_proxy(L,job);native_database_namespaces_full(L,job);
    const int proxy=lua_gettop(L);s.proxy=lua_topointer(L,proxy);
    if(!lua_getmetatable(L,proxy))return (void)luaL_error(L,"Database original readonly proxy has no actual metatable");s.meta=lua_topointer(L,-1);lua_pop(L,1);
    native_database_namespace_key(L,which+1);lua_pushvalue(L,proxy);lua_settable(L,environment);native_database_namespaces_full(L,job);
    lua_pushvalue(L,proxy);s.proxy_ref=luaL_ref(L,LUA_REGISTRYINDEX);++f->assigned;native_database_namespaces_full(L,job);lua_pop(L,2);
}
static void perform_native_context_database_namespaces(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.database_namespaces_frame;native_database_namespaces_full(L,job);
    if(!f||f->assigned||f->actual_job||f->keys_ref!=LUA_NOREF||!lua_checkstack(L,32))
        return (void)luaL_error(L,"GameInfo/GameDefines actual registrar replay/stack refused");
    f->actual_job=&job;
    lua_createtable(L,2,0);const int keys=lua_gettop(L);const void*const value=lua_topointer(L,keys);native_database_namespaces_full(L,job);
    for(int i=0;i<2;++i){lua_pushstring(L,native_database_namespace_keys[i]);lua_rawseti(L,keys,i+1);native_database_namespaces_full(L,job);}
    lua_pushvalue(L,keys);f->keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);f->keys=value;
    if(f->keys_ref<0)return (void)luaL_error(L,"Database namespace actual retained keys failed");native_database_namespaces_full(L,job);lua_pop(L,1);
    native_database_namespace_register(L,job,0,native_gameinfo_factory);
    native_database_namespace_register(L,job,1,native_game_defines_factory);
    native_database_namespaces_full(L,job);
    for(int i=0;i<2;++i){job.context_database_master_references[i]=f->stores[i].master_ref;job.context_database_proxy_references[i]=f->stores[i].proxy_ref;
        job.context_database_masters[i]=f->stores[i].master;job.context_database_proxies[i]=f->stores[i].proxy;}
    native_database_namespaces_full(L,job);worldview_capture_database_publications_owned(L,job);native_database_namespaces_full(L,job);
}
