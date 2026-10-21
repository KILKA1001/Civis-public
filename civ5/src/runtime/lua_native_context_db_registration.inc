// Actual5C7880 first namespace: protected5605C0 DB/5C1470. The original
// constructor registers six real consumers, without invoking their backends.
struct NativeContextDbRegistrationFrame final {
    Impl& host;const NativeContextApiRegistrationFrame*const predecessor;
    const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;
    int keys_ref=LUA_NOREF;const void* keys=nullptr;
    int master_ref=LUA_NOREF,proxy_ref=LUA_NOREF;const void* master=nullptr;const void* proxy=nullptr;const void* meta=nullptr;
    std::size_t fields_created=0,stores=0;Job* actual_job=nullptr;
    explicit NativeContextDbRegistrationFrame(Impl& p):host(p),predecessor(p.api_registration_frame),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(host.db_registration_frame||!predecessor)throw ScriptError("DB registrar requires actual ArtInfo predecessor");
        predecessor->first();if(predecessor->assigned_count!=1)throw ScriptError("DB original predecessor did not execute ArtInfo store");
        host.db_registration_frame=this;
    }
    ~NativeContextDbRegistrationFrame(){if(host.db_registration_frame==this)host.db_registration_frame=nullptr;}
    void first()const {
        if(host.db_registration_frame!=this||creator!=std::this_thread::get_id()||!predecessor||host.api_registration_frame!=predecessor||
           epoch!=host.failure_epoch||writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty)
            throw ScriptError("DB registrar actual source/frame changed");
        predecessor->first();
    }
    void full()const {first();predecessor->full();first();}
};
static constexpr const char*const native_db_keys[]={"DB","GetMemoryUsage","CollectMemoryUsage","StatementCount","StatementSQL","Query","CreateQuery","__gc"};
static constexpr const char*const native_db_memory_keys[]={"NumRegions","MaxUserAllocatedBytes","CurrentUserAllocatedBytes","CurrentAllocatedBytes","CurrentReservedBytes","CurrentOverheadBytes","CurrentUncommitedBytes"};
static void native_db_key(lua_State* L,int i){
    const auto& p=owner(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_db_registration_keys_ref);lua_rawgeti(L,-1,i);lua_remove(L,-2);
}
static void native_db_registration_full(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.db_registration_frame;p.source_exception={};p.source_error=nullptr;
    try{if(!f)throw ScriptError("DB registrar has no genuine entered frame");f->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="DB registrar SAME source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    if(f->actual_job&&f->actual_job!=&job)return (void)luaL_error(L,"DB registrar actual entered Job replaced");
    native_api_prefix_full(L,job);
    p.source_exception={};p.source_error=nullptr;
    try{if(p.db_registration_frame!=f)throw ScriptError("DB frame replaced after source callbacks");f->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="DB registrar FIRST after callbacks refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    const int top=lua_gettop(L);
    if(f->keys_ref!=LUA_NOREF){
        if(f->keys_ref!=p.native_db_registration_keys_ref)return (void)luaL_error(L,"DB registrar keys custody changed");
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->keys||lua_objlen(L,-1)!=8||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"DB registrar retained key table changed");
        for(int i=0;i<8;++i){
            lua_rawgeti(L,-1,i+1);std::size_t n=0;const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
            if(!key||n!=std::strlen(native_db_keys[i])||std::memcmp(key,native_db_keys[i],n))
                return (void)luaL_error(L,"DB registrar retained key TString changed");lua_pop(L,1);
        }lua_pop(L,1);
    }
    if(f->master_ref>=0){
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->master)return (void)luaL_error(L,"DB actual master table changed");
        native_db_key(L,1);lua_rawget(L,LUA_GLOBALSINDEX);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"DB original master cache changed");lua_pop(L,2);
    }
    if(f->stores){
        if(f->stores!=1||f->proxy_ref<0||!f->proxy||!f->meta||f->master_ref<0)
            return (void)luaL_error(L,"DB selected store lacks actual return");
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->predecessor->predecessor->environment);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->predecessor->predecessor->actual_environment||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"DB actual selected fenv changed");
        native_db_key(L,1);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->proxy_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->proxy||!lua_rawequal(L,-1,-2)||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=f->meta)
            return (void)luaL_error(L,"DB selected proxy/ref/metatable changed");
        native_api_prefix_key(L,2);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"DB proxy actual master __index changed");lua_pop(L,2);
        native_api_prefix_key(L,3);lua_rawget(L,-2);
        if(lua_type(L,-1)!=LUA_TBOOLEAN||lua_toboolean(L,-1))return (void)luaL_error(L,"DB proxy original false __metatable changed");lua_pop(L,1);
        native_api_prefix_key(L,4);lua_rawget(L,-2);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_api_readonly_write||lua_getupvalue(L,-1,1))
            return (void)luaL_error(L,"DB proxy original write-error consumer changed");
    }
    lua_settop(L,top);
}
static int native_db_memory_usage(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    p.query_row={};p.source_exception={};p.source_error=nullptr;
    try{
        if(!p.gameplay.db_memory_usage)throw ScriptError("DB.GetMemoryUsage requires genuine current Database allocator heap report; provider unissued");
        p.query_row=p.gameplay.db_memory_usage();p.worldview_require_callback_origin();
        if(p.query_row->size()!=7)throw ScriptError("DB memory report differs from original seven fields");
        for(std::size_t i=0;i<7;++i){
            auto& item=(*p.query_row)[i];
            if(item.first!=native_db_memory_keys[i]||!std::holds_alternative<std::int64_t>(item.second))
                throw ScriptError("DB memory report original key/integer shape refused");
            const auto actual=std::get<std::int64_t>(item.second);
            if(actual<std::numeric_limits<std::int32_t>::min()||actual>std::numeric_limits<std::uint32_t>::max())
                throw ScriptError("DB memory report exceeds original DWORD field");
            item.second=static_cast<std::int64_t>(static_cast<std::int32_t>(static_cast<std::uint32_t>(actual)));
        }
        if(epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty)
            throw ScriptError("DB memory provider changed actual source during callback");
    }catch(const std::exception& e){p.source_exception=std::current_exception();p.source_error=e.what();}
    catch(...){p.source_exception=std::current_exception();p.source_error="DB genuine memory report failed";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    native_db_query_current(L);push_row(L,*p.query_row);p.query_row={};native_db_query_current(L);
    if(epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty){p.record_failure();return luaL_error(L,"DB memory row publication changed actual source");}
    return 1;
}
static int native_db_collect_memory_usage(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    p.source_exception={};p.source_error=nullptr;
    try{
        if(!p.gameplay.db_collect_memory_usage)throw ScriptError("DB.CollectMemoryUsage requires genuine current Database allocator HeapWalk collector; provider unissued");
        p.gameplay.db_collect_memory_usage();p.worldview_require_callback_origin();
    }catch(const std::exception& e){p.source_exception=std::current_exception();p.source_error=e.what();}
    catch(...){p.source_exception=std::current_exception();p.source_error="DB genuine memory collector failed";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    native_db_query_current(L);
    if(epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty){p.record_failure();return luaL_error(L,"DB memory collection changed actual source");}
    return 0;
}
static int native_db_statement_count(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    p.native_db_count_scratch=0;p.source_exception={};p.source_error=nullptr;
    try{
        if(!p.gameplay.db_statement_count)throw ScriptError("DB.StatementCount requires actual selected rules sqlite3_next_stmt producer; provider unissued");
        p.native_db_count_scratch=p.gameplay.db_statement_count();p.worldview_require_callback_origin();
    }catch(const std::exception& e){p.source_exception=std::current_exception();p.source_error=e.what();}
    catch(...){p.source_exception=std::current_exception();p.source_error="DB actual StatementCount failed";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    native_db_query_current(L);
    if(epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty){p.record_failure();return luaL_error(L,"DB StatementCount changed actual source");}
    lua_pushinteger(L,static_cast<std::int32_t>(static_cast<std::uint32_t>(p.native_db_count_scratch)));return 1;
}
static int native_db_statement_sql(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);(void)luaL_checknumber(L,1);const auto index=source_integer(L,1);
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    p.native_db_sql_scratch={};p.source_exception={};p.source_error=nullptr;
    try{
        if(!p.gameplay.db_statement_sql)throw ScriptError("DB.StatementSQL requires actual selected rules sqlite3_next_stmt/sql producer; provider unissued");
        p.native_db_sql_scratch=p.gameplay.db_statement_sql(index);p.worldview_require_callback_origin();
    }catch(const std::exception& e){p.source_exception=std::current_exception();p.source_error=e.what();}
    catch(...){p.source_exception=std::current_exception();p.source_error="DB actual StatementSQL failed";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    native_db_query_current(L);
    if(epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty){p.record_failure();return luaL_error(L,"DB StatementSQL changed actual source");}
    if(p.native_db_sql_scratch)lua_pushstring(L,p.native_db_sql_scratch->c_str());else lua_pushnil(L);
    p.native_db_sql_scratch={};native_db_query_current(L);
    if(epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty){p.record_failure();return luaL_error(L,"DB SQL publication changed actual source");}
    return 1;
}
static int native_db_factory(lua_State* L){
    auto& p=owner(L);auto*const f=p.db_registration_frame;
    if(!f||!f->actual_job)return luaL_error(L,"Native DB constructor lacks its actual entered Job");
    auto& job=*f->actual_job;
    native_db_registration_full(L,job);
    if(!f||f->fields_created||f->master_ref!=LUA_NOREF)return luaL_error(L,"Native DB5C1470 constructor replay refused");
    static constexpr lua_CFunction functions[]={native_db_memory_usage,native_db_collect_memory_usage,native_db_statement_count,native_db_statement_sql,native_db_query,native_db_create_query};
    lua_createtable(L,0,0);const int table=lua_gettop(L);const void*const value=lua_topointer(L,table);native_db_registration_full(L,job);
    for(int i=0;i<6;++i){
        native_db_key(L,i+2);lua_pushcfunction(L,functions[i]);lua_rawset(L,table);++f->fields_created;native_db_registration_full(L,job);
    }
    if(f->fields_created!=6||!lua_istable(L,table)||lua_topointer(L,table)!=value||lua_getmetatable(L,table))
        return luaL_error(L,"Native DB5C1470 actual namespace constructor changed");
    for(int i=0;i<6;++i){native_db_key(L,i+2);lua_rawget(L,table);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=functions[i]||lua_getupvalue(L,-1,1))return luaL_error(L,"DB original zero-upvalue consumer changed");lua_pop(L,1);}
    return 1;
}
static void perform_native_context_db_registration(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.db_registration_frame;native_db_registration_full(L,job);
    if(!f||f->stores||f->keys_ref!=LUA_NOREF||f->actual_job||!lua_checkstack(L,32))
        return (void)luaL_error(L,"DB original registrar replay/key custody/stack refused");
    f->actual_job=&job;
    if(p.native_db_registration_keys_ref==LUA_NOREF){
        lua_createtable(L,8,0);const int keys=lua_gettop(L);const void*const key_value=lua_topointer(L,keys);native_db_registration_full(L,job);
        for(int i=0;i<8;++i){lua_pushstring(L,native_db_keys[i]);lua_rawseti(L,keys,i+1);native_db_registration_full(L,job);}
        lua_pushvalue(L,keys);const int key_ref=luaL_ref(L,LUA_REGISTRYINDEX);
        if(key_ref<0)return (void)luaL_error(L,"DB actual retained key allocation failed");
        f->keys_ref=key_ref;f->keys=key_value;p.native_db_registration_keys_ref=key_ref;p.native_db_registration_keys_pointer=key_value;
        native_db_registration_full(L,job);lua_pop(L,1);
    }else{
        f->keys_ref=p.native_db_registration_keys_ref;f->keys=p.native_db_registration_keys_pointer;native_db_registration_full(L,job);
    }
    // Original5605C0 selects the actual518 environment before inspecting
    // GLOBALS.DB; a cache miss runs5C1470 zero args/one result exactly once.
    perform_native_context_state_query(L,job);native_db_registration_full(L,job);const int environment=lua_gettop(L);
    if(lua_topointer(L,environment)!=f->predecessor->predecessor->actual_environment)
        return (void)luaL_error(L,"DB registrar actual prepared518 environment changed");
    native_db_key(L,1);lua_gettable(L,LUA_GLOBALSINDEX);native_db_registration_full(L,job);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);lua_pushcfunction(L,native_db_factory);lua_call(L,0,1);native_db_registration_full(L,job);
        native_db_key(L,1);lua_pushvalue(L,-2);lua_settable(L,LUA_GLOBALSINDEX);native_db_registration_full(L,job);
    }
    if(!lua_istable(L,-1))return (void)luaL_error(L,"DB original namespace master is not a table");
    f->master=lua_topointer(L,-1);lua_pushvalue(L,-1);f->master_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_db_registration_full(L,job);
    native_api_readonly_proxy(L,job);native_db_registration_full(L,job);const int proxy=lua_gettop(L);f->proxy=lua_topointer(L,proxy);
    if(!lua_getmetatable(L,proxy))return (void)luaL_error(L,"DB original49B190 proxy has no metatable");f->meta=lua_topointer(L,-1);lua_pop(L,1);
    native_db_key(L,1);lua_pushvalue(L,proxy);lua_settable(L,environment);native_db_registration_full(L,job);
    lua_pushvalue(L,proxy);f->proxy_ref=luaL_ref(L,LUA_REGISTRYINDEX);++f->stores;native_db_registration_full(L,job);
    job.context_db_master_reference=f->master_ref;job.context_db_proxy_reference=f->proxy_ref;
    job.context_db_table=f->master;job.context_db_proxy=f->proxy;lua_pop(L,2);
    native_db_registration_full(L,job);worldview_capture_db_publication_owned(L,job);native_db_registration_full(L,job);
}
