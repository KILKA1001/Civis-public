// Actual714D80/73BE90 lazy FLua prefix, qualified from the original PE.
// Complete finite fresh-session typed/named/builtin registration, real scoped
// coroutine lifetime and selected5D7BD0 field copy. Deferred method invocation
// still acquires its actual backend; registration does not invoke methods.
struct NativeContextFLuaFrame final {
    Impl& host;const NativeContextModdingFrame*const predecessor;
    const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;Job* actual_job=nullptr;
    int table_ref=LUA_NOREF;const void* table=nullptr;std::size_t copied=0;bool returned=false;
    explicit NativeContextFLuaFrame(Impl& p):host(p),predecessor(p.modding_frame),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(p.flua_frame||!predecessor)throw ScriptError("FLua714D80 requires its actual completed Modding predecessor");
        predecessor->first();
        if(predecessor->stores!=1||predecessor->master_ref<0||predecessor->proxy_ref<0||
           !predecessor->master||!predecessor->proxy)
            throw ScriptError("FLua714D80 actual Modding publication did not return");
        p.flua_frame=this;
    }
    ~NativeContextFLuaFrame(){if(host.flua_frame==this)host.flua_frame=nullptr;}
    NativeContextFLuaFrame(const NativeContextFLuaFrame&)=delete;
    NativeContextFLuaFrame& operator=(const NativeContextFLuaFrame&)=delete;
    void first()const {
        if(host.flua_frame!=this||creator!=std::this_thread::get_id()||creator!=host.ui_thread||
           host.modding_frame!=predecessor||!predecessor||epoch!=host.failure_epoch||
           writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty||host.failed.exhausted_)
            throw ScriptError("FLua714D80 actual frame/Source/failure changed");
        predecessor->first();
    }
    void full()const {first();predecessor->full();first();}
};
class NativeFLuaBootstrapOwner final {
    friend struct Impl;
    Impl*const host;lua_State*const master;
    const std::thread::id creator;
    const std::uint64_t epoch,writes,uncertainty;
    bool retired=false;
    enum class Phase {captured,thread_created,environment_retained,table_created,
        global_assigned,metadata_created,metadata_applied,fenv_assigned,exposures_entered,exposures_returned,thread_destroyed,returned};
    Phase phase=Phase::captured;
    lua_State* temporary_thread=nullptr;
    int thread_ref=LUA_NOREF,environment_ref=LUA_NOREF,table_ref=LUA_NOREF,metadata_ref=LUA_NOREF;
    const void* environment=nullptr;const void* table=nullptr;const void* metadata=nullptr;
    int creator_status=-1,exposure_status=-1,destroy_status=-1;
    explicit NativeFLuaBootstrapOwner(Impl& p,const NativeContextFLuaFrame& f):host(&p),master(p.state),
        creator(f.creator),epoch(f.epoch),writes(f.writes),uncertainty(f.uncertainty){}
    NativeFLuaBootstrapOwner(const NativeFLuaBootstrapOwner&)=delete;
    NativeFLuaBootstrapOwner& operator=(const NativeFLuaBootstrapOwner&)=delete;
public:~NativeFLuaBootstrapOwner()=default;
};
static NativeContextFLuaFrame* native_flua_registration_full(lua_State* L){
    auto& p=owner(L);auto*const frame=p.flua_frame;p.source_exception={};p.source_error=nullptr;
    try{if(!frame)throw ScriptError("FLua714D80 has no genuine entered frame");frame->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="FLua registrar SAME Source FULL refused";}
    if(p.source_error)return (native_master_thread_refuse(L,p.source_error),nullptr);
    if(p.flua_frame!=frame||!frame->actual_job||frame->actual_job->owner!=&p||
       frame->actual_job->operation!=Operation::native_context_flua_registration_prefix||!p.calling||
       !p.state||G(L)!=G(p.state)||!frame->predecessor->actual_job)
        return (native_master_thread_refuse(L,"FLua registrar actual Job/predecessor/VM changed"),nullptr);
    native_modding_registration_full(L,*frame->predecessor->actual_job);
    // Reacquire the owning frame after callbackful predecessor validation.
    if(p.flua_frame!=frame)return (native_master_thread_refuse(L,"FLua frame replaced during predecessor FULL"),nullptr);
    p.source_exception={};p.source_error=nullptr;
    try{frame->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="FLua registrar FIRST after callbacks refused";}
    if(p.source_error)return (native_master_thread_refuse(L,p.source_error),nullptr);
    const auto saved=native_master_thread_callback_first(L);native_master_thread_callback_post(L,saved);
    return frame;
}
static NativeFLuaBootstrapOwner* native_flua_owner_current(lua_State* L){
    auto& p=owner(L);auto*const actual=p.native_flua_bootstrap.get();
    (void)native_flua_registration_full(L);
    if(!actual||p.native_flua_bootstrap.get()!=actual||actual->retired||actual->host!=&p||
       actual->master!=p.state||actual->creator!=std::this_thread::get_id()||actual->creator!=p.ui_thread||
       actual->epoch!=p.failure_epoch||actual->writes!=p.failed.writes_||
       actual->uncertainty!=p.execution_uncertainty)
        return (native_master_thread_refuse(L,"FLua real bootstrap storage/VM retired"),nullptr);
    const int top=lua_gettop(L);
    if(actual->thread_ref>=0){
        lua_rawgeti(L,LUA_REGISTRYINDEX,actual->thread_ref);
        if(lua_tothread(L,-1)!=actual->temporary_thread)
            return (native_master_thread_refuse(L,"FLua actual scoped coroutine retention changed"),nullptr);
        lua_pop(L,1);
    }
    if(actual->environment_ref>=0){
        lua_rawgeti(L,LUA_REGISTRYINDEX,actual->environment_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=actual->environment)
            return (native_master_thread_refuse(L,"FLua actual Threads map environment retention changed"),nullptr);
        lua_pop(L,1);
    }
    if(actual->table_ref>=0){
        lua_rawgeti(L,LUA_REGISTRYINDEX,actual->table_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=actual->table)
            return (native_master_thread_refuse(L,"FLua actual cache retention changed"),nullptr);
        lua_pop(L,1);
    }
    if(actual->phase>=NativeFLuaBootstrapOwner::Phase::global_assigned){
        // Check master globals even while the offered L is the temporary thread.
        lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_master_thread_bootstrap->globals_ref);
        lua_getfield(L,-1,"FLua");
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=actual->table)
            return (native_master_thread_refuse(L,"FLua original master cache publication changed"),nullptr);
        lua_pop(L,2);
    }
    if(actual->metadata_ref>=0){
        lua_rawgeti(L,LUA_REGISTRYINDEX,actual->metadata_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=actual->metadata)
            return (native_master_thread_refuse(L,"FLua actual metadata retention changed"),nullptr);
        lua_pop(L,1);
    }
    if(actual->phase>=NativeFLuaBootstrapOwner::Phase::metadata_applied){
        lua_rawgeti(L,LUA_REGISTRYINDEX,actual->table_ref);
        if(!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=actual->metadata)
            return (native_master_thread_refuse(L,"FLua original cache metatable changed"),nullptr);
        lua_getfield(L,-1,"__index");
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=actual->environment)
            return (native_master_thread_refuse(L,"FLua original __index environment alias changed"),nullptr);
        lua_pop(L,3);
    }
    if(actual->phase>=NativeFLuaBootstrapOwner::Phase::fenv_assigned&&actual->phase<NativeFLuaBootstrapOwner::Phase::thread_destroyed){
        lua_rawgeti(L,LUA_REGISTRYINDEX,actual->thread_ref);lua_getfenv(L,-1);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=actual->table)
            return (native_master_thread_refuse(L,"FLua real scoped coroutine fenv publication changed"),nullptr);
        lua_pop(L,2);
    }
    if(lua_gettop(L)!=top)return (native_master_thread_refuse(L,"FLua physical validation changed its actual stack"),nullptr);
    return actual;
}
static int native_flua_create_scoped_thread_5a0af0(lua_State* L){
    auto& p=owner(L);auto*const actual=native_flua_owner_current(L);
    if(L!=p.state||lua_touserdata(L,1)!=actual||actual->phase!=NativeFLuaBootstrapOwner::Phase::captured||
       actual->temporary_thread||actual->thread_ref!=LUA_NOREF)
        return native_master_thread_refuse(L,"FLua4D9FC0 actual scoped-thread constructor replay/substitution");
    // Original5A0AF0 executes the real _CreateThread with0 arguments/1 result.
    // Its closed native57A350 body creates and publishes both real maps.
    lua_getfield(L,LUA_GLOBALSINDEX,"_CreateThread");(void)native_flua_owner_current(L);
    const auto*const called=native_master_protected_call_8f7ae0_owned(L,0,1);
    native_master_protected_call_return_current_owned(L,called);
    if(called->actual_status!=0)
        return luaL_error(L,"FLua actual _CreateThread protected call failed; no coroutine loan issued");
    (void)native_flua_owner_current(L);
    actual->temporary_thread=lua_tothread(L,-1);
    if(!actual->temporary_thread)return luaL_error(L,"FLua actual _CreateThread did not return a coroutine");
    actual->phase=NativeFLuaBootstrapOwner::Phase::thread_created;
    lua_pushvalue(L,-1);actual->thread_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    if(actual->thread_ref<0)return luaL_error(L,"FLua real scoped coroutine retention failed");
    (void)native_flua_owner_current(L);return 0;
}
static int native_flua_exposures_76b700(lua_State* L){
    auto& p=owner(L);auto*const actual=native_flua_owner_current(L);
    if(L!=actual->temporary_thread||lua_touserdata(L,1)!=actual||
       actual->phase!=NativeFLuaBootstrapOwner::Phase::fenv_assigned)
        return native_master_thread_refuse(L,"FLua76B700 actual registration coroutine/fenv changed");
    actual->phase=NativeFLuaBootstrapOwner::Phase::exposures_entered;
    auto*const frame=native_flua_registration_full(L);
    native_flua_exposure_bootstrap_owned(L,*frame->actual_job);
    actual->phase=NativeFLuaBootstrapOwner::Phase::exposures_returned;
    (void)native_flua_owner_current(L);return 0;
}

static int native_flua_destroy_scoped_thread_4bde30(lua_State*L){
    auto&p=owner(L);auto*const actual=native_flua_owner_current(L);
    if(L!=p.state||lua_touserdata(L,1)!=actual||actual->phase!=NativeFLuaBootstrapOwner::Phase::exposures_returned||
       !actual->temporary_thread||actual->thread_ref<0)return native_master_thread_refuse(L,"FLua actual4BDE30 scope/thread operand changed");
    lua_getfield(L,LUA_GLOBALSINDEX,"_DestroyThread");(void)native_flua_owner_current(L);
    native_master_find_thread_environment_534b50_owned(L,actual->temporary_thread);(void)native_flua_owner_current(L);
    if(lua_topointer(L,-1)!=actual->environment)return native_master_thread_refuse(L,"FLua actual4BDE30 retained Threads map value changed");
    lua_pushcclosure(L,&Impl::native_master_clear_thread_environment_6acc70,0);lua_pushvalue(L,-2);
    lua_call(L,1,0);(void)native_flua_owner_current(L);lua_pop(L,1);
    lua_call(L,1,0);(void)native_flua_owner_current(L);return 0;
}
static void native_flua_normal_scoped_destruction_7458e0_owned(lua_State*L){
    auto&p=owner(L);auto*const actual=native_flua_owner_current(L);
    if(L!=p.state||actual->phase!=NativeFLuaBootstrapOwner::Phase::exposures_returned)
        return (void)native_master_thread_refuse(L,"FLua normal destruction requires the actual completed scoped exposure unit");
    // Actual528C80 mutable reference-count reader. Constructor count0 is real
    // live data; if an actual owned writer later adds a row, execute8F8820.
    auto*bank=native_master_reference_bank_current_owned(L);std::uint32_t ordinal=0;
    while(ordinal<bank->reference_count){
        auto&row=bank->references[ordinal];
        if(row.actual_vm==actual->temporary_thread){
            if(row.actual_vm&&row.actual_reference!=LUA_NOREF){
                if(G(row.actual_vm)!=G(L))return (void)native_master_thread_refuse(L,"FLua actual bank LuaRef belongs to a different VM");
                luaL_unref(row.actual_vm,LUA_REGISTRYINDEX,row.actual_reference);
            }
            row.actual_vm=nullptr;row.actual_reference=LUA_NOREF;
        }
        bank=native_master_reference_bank_current_owned(L);++ordinal;
    }
    auto*ledger=native_master_thread_ledger_current_owned(L);p.source_error=nullptr;p.source_exception={};
    try{ledger->clear_matching_state_callback_free(actual->temporary_thread);}
    catch(...){p.source_exception=std::current_exception();p.source_error="FLua actual7458E0 first matching ledger state clear refused";}
    if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
    auto*const boundary=p.native_master_protected_boundary;native_master_protected_boundary_current(L,boundary);
    if(boundary->held_locks==UINT64_MAX)return (void)native_master_thread_refuse(L,"FLua destroy real lock history exhausted");
    ::EnterCriticalSection(&boundary->critical->actual);++boundary->held_locks;
    actual->destroy_status=lua_cpcall(L,native_flua_destroy_scoped_thread_4bde30,actual);
    if(actual->destroy_status!=0){::LeaveCriticalSection(&boundary->critical->actual);--boundary->held_locks;return (void)lua_error(L);}
    actual->phase=NativeFLuaBootstrapOwner::Phase::thread_destroyed;
    // Release private roots before the original full GC: actual Threads no
    // longer owns this coroutine, so WeakThreads must not gain extra lifetime.
    const int thread_ref=actual->thread_ref;actual->thread_ref=LUA_NOREF;actual->temporary_thread=nullptr;
    luaL_unref(L,LUA_REGISTRYINDEX,thread_ref);
    const int environment_ref=actual->environment_ref;actual->environment_ref=LUA_NOREF;
    luaL_unref(L,LUA_REGISTRYINDEX,environment_ref);
    (void)lua_gc(L,LUA_GCCOLLECT,0);
    ::LeaveCriticalSection(&boundary->critical->actual);--boundary->held_locks;
    (void)native_flua_owner_current(L);bank=native_master_reference_bank_current_owned(L);
    // Qualified7EF7A0 is exactly state==3. Existing real5C2790/7EF4F0
    // socket constructor owns current state1, so actual4E3750 has no packet.
    // A genuinely connected debug writer would require its real transport.
    if(bank->actual_socket_owner->native_socket_state==3)
        return (void)native_master_thread_refuse(L,"FLua actual4E3750 connected debug packet/transport supplier required");
    bank->actual_socket_owner->require_current_without_callbacks();
    actual->phase=NativeFLuaBootstrapOwner::Phase::returned;(void)native_flua_owner_current(L);
}
static void native_flua_selected_field_copy_5d7bd0_owned(lua_State*L,Job&job){
    auto&p=owner(L);auto*const frame=native_flua_registration_full(L);auto*const actual=native_flua_owner_current(L);
    if(actual->phase!=NativeFLuaBootstrapOwner::Phase::returned||frame->returned)
        return (void)native_master_thread_refuse(L,"FLua selected5D7BD0 requires its own completed registrar/cache return");
    lua_getfield(L,LUA_GLOBALSINDEX,"FLua");(void)native_flua_owner_current(L);const int source=lua_gettop(L);
    if(lua_type(L,source)!=LUA_TTABLE||lua_topointer(L,source)!=actual->table)
        return (void)native_master_thread_refuse(L,"FLua73BE90 actual returned cache changed before copy");
    // The existing actual Context518 query is the portable canonical thread/
    // fenv borrower. Its SAME ChildContext roots supply original534B50 operands;
    // they are not fabricated from an environment name or arbitrary LuaTable.
    perform_native_context_state_query(L,job);(void)native_flua_owner_current(L);
    const int destination=lua_gettop(L);const void*const environment=lua_topointer(L,destination);
    if(!job.context_state_thread||job.context_state_environment!=environment||!p.lua_events_frame||
       environment!=p.lua_events_frame->actual_environment||lua_type(L,destination)!=LUA_TTABLE)
        return (void)native_master_thread_refuse(L,"FLua73BE90 selected actual518 thread/fenv borrower changed");
    lua_pushnil(L);
    while(lua_next(L,source)){
        (void)native_flua_owner_current(L);
        lua_pushvalue(L,-2);lua_pushvalue(L,-2);lua_settable(L,destination);lua_pop(L,1);++frame->copied;
        (void)native_flua_owner_current(L);
        if(lua_topointer(L,source)!=actual->table||lua_topointer(L,destination)!=environment)
            return (void)native_master_thread_refuse(L,"FLua original5D7BD0 source/destination changed during field copy");
    }
    frame->table=actual->table;frame->table_ref=actual->table_ref;frame->returned=true;
    job.context_flua_table=actual->table;job.context_flua_reference=actual->table_ref;job.context_flua_copied=frame->copied;
    lua_settop(L,source-1);(void)native_flua_owner_current(L);
}

static void perform_native_context_flua_registration_prefix(lua_State* L,Job& job){
    auto& p=owner(L);auto*const frame=p.flua_frame;
    if(!frame||frame->actual_job||job.owner!=&p||job.operation!=Operation::native_context_flua_registration_prefix||L!=p.state)
        return (void)native_master_thread_refuse(L,"FLua714D80 actual protected Job/master/frame absent");
    frame->actual_job=&job;(void)native_flua_registration_full(L);
    if(!lua_checkstack(L,48))return (void)native_master_thread_refuse(L,"FLua original bootstrap stack unavailable");
    (void)native_flua_registration_full(L);
    lua_getfield(L,LUA_GLOBALSINDEX,"FLua");(void)native_flua_registration_full(L);
    if(!lua_isnil(L,-1)){
        luaL_checktype(L,-1,LUA_TTABLE);
        auto*const current=native_flua_owner_current(L);
        if(!current||current->phase!=NativeFLuaBootstrapOwner::Phase::returned||lua_topointer(L,-1)!=current->table)
            return (void)luaL_error(L,"FLua73BE90 cache HIT lacks its genuine completed exposure/lifetime return");
        lua_pop(L,1);native_flua_selected_field_copy_5d7bd0_owned(L,job);return;
    }
    lua_pop(L,1);
    if(p.native_flua_bootstrap)return (void)luaL_error(L,"FLua7769D0 cache miss cannot replay an already retained partial bootstrap");
    // Persistent Host custody is acquired before Lua APIs. Every subsequent
    // local is POD/borrowed; Lua longjmp strands no owning C++ stack object.
    p.source_error=nullptr;p.source_exception={};
    try{p.native_flua_bootstrap.reset(new NativeFLuaBootstrapOwner(p,*frame));}
    catch(...){p.source_exception=std::current_exception();p.source_error="FLua actual bootstrap storage allocation refused";}
    if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
    auto*const actual=native_flua_owner_current(L);
    auto*const boundary=p.native_master_protected_boundary;
    native_master_protected_boundary_current(L,boundary);
    if(boundary->held_locks==UINT64_MAX)return (void)native_master_thread_refuse(L,"FLua real recursive lock history exhausted");
    ::EnterCriticalSection(&boundary->critical->actual);++boundary->held_locks;
    // Actual6437F0 protects5A0AF0 on this same master; real lock custody is
    // outside the Lua jump boundary and is also released by Impl::call abort.
    actual->creator_status=lua_cpcall(L,native_flua_create_scoped_thread_5a0af0,actual);
    ::LeaveCriticalSection(&boundary->critical->actual);--boundary->held_locks;
    if(actual->creator_status!=0)return (void)lua_error(L);
    (void)native_flua_owner_current(L);
    native_master_find_thread_environment_534b50_owned(L,actual->temporary_thread);
    (void)native_flua_owner_current(L);
    const int environment=lua_gettop(L),thread=environment-1;
    if(lua_tothread(L,thread)!=actual->temporary_thread||!lua_istable(L,environment))
        return (void)luaL_error(L,"FLua7769D0 actual scoped thread/Threads environment operands absent");
    actual->environment=lua_topointer(L,environment);
    lua_pushvalue(L,environment);actual->environment_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    if(actual->environment_ref<0)return (void)luaL_error(L,"FLua actual Threads environment retention failed");
    actual->phase=NativeFLuaBootstrapOwner::Phase::environment_retained;(void)native_flua_owner_current(L);
    // Actual7769F3..776A1A publishes the real cache before its metatable.
    lua_createtable(L,0,0);actual->table=lua_topointer(L,-1);
    lua_pushvalue(L,-1);actual->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    if(actual->table_ref<0)return (void)luaL_error(L,"FLua actual cache retention failed");
    actual->phase=NativeFLuaBootstrapOwner::Phase::table_created;(void)native_flua_owner_current(L);
    lua_pushvalue(L,-1);lua_setfield(L,LUA_GLOBALSINDEX,"FLua");
    actual->phase=NativeFLuaBootstrapOwner::Phase::global_assigned;(void)native_flua_owner_current(L);
    lua_createtable(L,0,0);actual->metadata=lua_topointer(L,-1);
    lua_pushvalue(L,-1);actual->metadata_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    if(actual->metadata_ref<0)return (void)luaL_error(L,"FLua actual metadata retention failed");
    actual->phase=NativeFLuaBootstrapOwner::Phase::metadata_created;(void)native_flua_owner_current(L);
    lua_pushvalue(L,environment);lua_setfield(L,-2,"__index");lua_setmetatable(L,-2);
    actual->phase=NativeFLuaBootstrapOwner::Phase::metadata_applied;(void)native_flua_owner_current(L);
    // Actual776A45 uses the coroutine key, not its original map value.
    if(!lua_setfenv(L,thread))return (void)luaL_error(L,"FLua actual scoped coroutine fenv replacement failed");
    actual->phase=NativeFLuaBootstrapOwner::Phase::fenv_assigned;(void)native_flua_owner_current(L);
    native_master_protected_boundary_current(L,boundary);
    if(boundary->held_locks==UINT64_MAX)return (void)native_master_thread_refuse(L,"FLua real exposure lock history exhausted");
    ::EnterCriticalSection(&boundary->critical->actual);++boundary->held_locks;
    actual->exposure_status=lua_cpcall(actual->temporary_thread,native_flua_exposures_76b700,actual);
    ::LeaveCriticalSection(&boundary->critical->actual);--boundary->held_locks;
    // Failed actual Lua APIs propagate their protected failure. They cannot be
    // misreported as a complete exposure cache or selected copy.
    if(actual->exposure_status!=0){lua_xmove(actual->temporary_thread,L,1);return (void)lua_error(L);}
    native_flua_normal_scoped_destruction_7458e0_owned(L);
    native_flua_selected_field_copy_5d7bd0_owned(L,job);
}
void native_flua_prefix_retire()noexcept{
    flua_frame=nullptr;
    if(native_flua_bootstrap)native_flua_bootstrap->retired=true;
    // Completed scopes released thread roots in7458E0. Partial failure roots
    // remain under actual VM/Host custody until lua_close, without a forged
    // destructor completion or Lua APIs in this noexcept retirement.
}
