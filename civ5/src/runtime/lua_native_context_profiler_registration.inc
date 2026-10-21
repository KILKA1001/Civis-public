// Inside Host::Impl, after Path. Fresh PE4D1F50 is182bytes: actual mutable
// GLOBALS.Profiler publication BEFORE five zero-upvalue consumer fields.
// Actual4C5740 calls protected8F8430(callback4D1F50,userdata=nullptr).
template<std::uint32_t NativeFunction> static int native_context_profiler_method(lua_State* L){
    native_db_query_current(L);auto& p=owner(L);p.record_failure();
    return luaL_error(L,"Profiler original consumer %08X requires its genuine canonical engine/timing owner; unissued",static_cast<unsigned>(NativeFunction));
}
struct NativeProfilerEntry {const char* name;std::uint32_t original_function;lua_CFunction function;};
static constexpr NativeProfilerEntry native_profiler_entries[]={
    {"ReportSamples",0x5c2200u,native_context_profiler_method<0x5c2200u>},
    {"ReportFPS",0x4ee550u,native_context_profiler_method<0x4ee550u>},
    {"BeginTiming",0x4ab3d0u,native_context_profiler_method<0x4ab3d0u>},
    {"Assert",0x62e780u,native_context_profiler_method<0x62e780u>},
    {"GetEngineInfoRowString",0x6761e0u,native_context_profiler_method<0x6761e0u>},
};
struct NativeContextProfilerRegistrationFrame final {
    Impl& host;const NativeContextPathRegistrationFrame*const predecessor;
    const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;
    Job* actual_job=nullptr;
    lua_State* execution_thread=nullptr;int selected_thread_ref=LUA_NOREF,selected_environment_ref=LUA_NOREF;
    const void* selected_environment=nullptr;
    int table_ref=LUA_NOREF,keys_ref=LUA_NOREF,status=-1;
    const void* table=nullptr;const void* keys=nullptr;
    std::size_t fields_created=0,stores=0;bool entered=false,returned=false;
    explicit NativeContextProfilerRegistrationFrame(Impl& p):host(p),predecessor(p.path_registration_frame),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(host.profiler_registration_frame||!predecessor)
            throw ScriptError("Profiler4C5740 requires its actual completed Path predecessor");
        predecessor->first();
        if(predecessor->stores!=1||predecessor->master_ref<0||predecessor->proxy_ref<0||
           !predecessor->master||!predecessor->proxy||!predecessor->actual_job)
            throw ScriptError("Profiler4C5740 actual Path publication did not return");
        host.profiler_registration_frame=this;
    }
    ~NativeContextProfilerRegistrationFrame(){host.native_selected_registrar_retire_for_frame_owned(this);if(host.profiler_registration_frame==this)host.profiler_registration_frame=nullptr;}
    NativeContextProfilerRegistrationFrame(const NativeContextProfilerRegistrationFrame&)=delete;
    NativeContextProfilerRegistrationFrame& operator=(const NativeContextProfilerRegistrationFrame&)=delete;
    void first()const {
        if(host.profiler_registration_frame!=this||creator!=std::this_thread::get_id()||creator!=host.ui_thread||
           !predecessor||host.path_registration_frame!=predecessor||epoch!=host.failure_epoch||
           writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty||host.failed.exhausted_)
            throw ScriptError("Profiler registrar actual Source/frame/failure retired");
        predecessor->first();
    }
    void full()const {first();predecessor->full();first();}
};
static void native_profiler_key(lua_State* L,int index){
    const auto*const frame=owner(L).profiler_registration_frame;
    lua_rawgeti(L,LUA_REGISTRYINDEX,frame->keys_ref);lua_rawgeti(L,-1,index);lua_remove(L,-2);
}
static void native_context_profiler_registration_full(lua_State* L,Job& job){
    auto& p=owner(L);auto*const frame=p.profiler_registration_frame;
    p.source_exception={};p.source_error=nullptr;
    try{if(!frame)throw ScriptError("Profiler registrar actual entered frame absent");frame->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Profiler registrar SAME Source FULL refused";}
    if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
    if((frame->actual_job&&frame->actual_job!=&job)||!frame->predecessor->actual_job||
       job.owner!=&p||job.operation!=Operation::native_context_profiler_registration||!p.calling||
       !p.state||G(L)!=G(p.state))return (void)luaL_error(L,"Profiler actual Job/predecessor/VM changed");
    native_context_path_registration_full(L,*frame->predecessor->actual_job);
    p.source_exception={};p.source_error=nullptr;
    try{if(p.profiler_registration_frame!=frame)throw ScriptError("Profiler frame replaced during callbacks");frame->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Profiler registrar FIRST after callback refused";}
    if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
    const int top=lua_gettop(L);
    if(frame->execution_thread){
        if(!frame->actual_job||job.worldview_child>=p.child_contexts.size()||!p.lua_events_frame)
            return (void)luaL_error(L,"Profiler actual selected518 child/frame unavailable");
        const auto& child=p.child_contexts[job.worldview_child];
        if(child.retired||!child.state_prepared||child.execution_thread!=frame->selected_thread_ref||
           child.environment!=frame->selected_environment_ref||frame->selected_thread_ref<0||frame->selected_environment_ref<0||
           frame->execution_thread==p.state||G(frame->execution_thread)!=G(p.state)||
           frame->execution_thread!=p.lua_events_frame->actual_thread||frame->selected_environment!=p.lua_events_frame->actual_environment||
           job.context_state_thread!=frame->execution_thread||job.context_state_environment!=frame->selected_environment||
           (L!=p.state&&L!=frame->execution_thread))
            return (void)luaL_error(L,"Profiler actual selected518 thread/fenv custody changed");
        lua_rawgeti(L,LUA_REGISTRYINDEX,frame->selected_thread_ref);
        if(lua_type(L,-1)!=LUA_TTHREAD||lua_tothread(L,-1)!=frame->execution_thread)
            return (void)luaL_error(L,"Profiler retained selected518 thread changed");
        lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,frame->selected_environment_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=frame->selected_environment||!lua_rawequal(L,-1,-2)||
           (L==frame->execution_thread&&lua_topointer(L,LUA_GLOBALSINDEX)!=frame->selected_environment))
            return (void)luaL_error(L,"Profiler actual selected518 globals/fenv rebound");
        lua_pop(L,3);
    }
    if(frame->keys_ref>=0){
        lua_rawgeti(L,LUA_REGISTRYINDEX,frame->keys_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=frame->keys||lua_objlen(L,-1)!=6||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Profiler retained literal keys changed");
        for(int i=0;i<6;++i){
            lua_rawgeti(L,-1,i+1);std::size_t n=0;
            const char*const value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
            const char*const expected=i?native_profiler_entries[i-1].name:"Profiler";
            if(!value||n!=std::strlen(expected)||std::memcmp(value,expected,n))
                return (void)luaL_error(L,"Profiler retained key TString differs");
            lua_pop(L,1);
        }lua_pop(L,1);
    }
    if(frame->table_ref>=0){
        lua_rawgeti(L,LUA_REGISTRYINDEX,frame->table_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=frame->table||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Profiler actual mutable constructor table changed");
        const int table=lua_gettop(L);
        if(frame->stores){
            if(frame->stores!=1)return (void)luaL_error(L,"Profiler original global store replayed");
            if(!frame->execution_thread||frame->selected_environment_ref<0)
                return (void)luaL_error(L,"Profiler selected publication has no actual thread/fenv");
            lua_rawgeti(L,LUA_REGISTRYINDEX,frame->selected_environment_ref);
            native_profiler_key(L,1);lua_rawget(L,-2);
            if(!lua_rawequal(L,-1,table))return (void)luaL_error(L,"Profiler original actual selected GLOBALS publication changed");
            lua_pop(L,2);
        }
        if(frame->fields_created>5)return (void)luaL_error(L,"Profiler original constructor field count differs");
        for(std::size_t i=0;i<frame->fields_created;++i){
            native_profiler_key(L,static_cast<int>(i)+2);lua_rawget(L,table);
            if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_profiler_entries[i].function||lua_getupvalue(L,-1,1))
                return (void)luaL_error(L,"Profiler original zero-upvalue consumer closure changed");
            lua_pop(L,1);
        }
    }
    lua_settop(L,top);
}
static int native_profiler_factory_4d1f50(lua_State* L){
    auto& p=owner(L);auto*const frame=p.profiler_registration_frame;
    if(!frame||!frame->actual_job)return luaL_error(L,"Profiler4D1F50 actual entered Job absent");
    native_selected_registrar_callback_enter_owned(L);
    auto& job=*frame->actual_job;native_context_profiler_registration_full(L,job);
    if(L!=frame->execution_thread||!frame->entered||frame->stores||frame->table_ref!=LUA_NOREF||frame->fields_created||
       lua_gettop(L)!=1||lua_type(L,1)!=LUA_TLIGHTUSERDATA||lua_touserdata(L,1)!=nullptr)
        return luaL_error(L,"Profiler4D1F50 original protected null-userdata callback replay/ABI differs");
    lua_createtable(L,0,0);const int table=lua_gettop(L);frame->table=lua_topointer(L,table);
    lua_pushvalue(L,table);frame->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    if(frame->table_ref<0)return luaL_error(L,"Profiler actual constructor table retention failed");
    native_context_profiler_registration_full(L,job);
    // Original4D1F5F pushes the SAME table and stores GLOBALS.Profiler before methods.
    native_profiler_key(L,1);lua_pushvalue(L,table);lua_settable(L,LUA_GLOBALSINDEX);++frame->stores;
    native_context_profiler_registration_full(L,job);
    for(int i=0;i<5;++i){
        native_profiler_key(L,i+2);lua_pushcfunction(L,native_profiler_entries[i].function);lua_rawset(L,table);
        ++frame->fields_created;native_context_profiler_registration_full(L,job);
    }
    if(frame->fields_created!=5||frame->stores!=1)
        return luaL_error(L,"Profiler4D1F50 did not create every original consumer field");
    native_context_profiler_registration_full(L,job);native_selected_registrar_callback_return_owned(L);return 0; // Literal PE4D2002 XOR EAX,EAX;4D2004 POP ESI;4D2005 RET.
}
static void perform_native_context_profiler_registration(lua_State* L,Job& job){
    auto& p=owner(L);auto*const frame=p.profiler_registration_frame;
    native_context_profiler_registration_full(L,job);
    if(L!=p.state||frame->actual_job||frame->entered||frame->returned||frame->keys_ref!=LUA_NOREF||!lua_checkstack(L,32))
        return (void)luaL_error(L,"Profiler4C5740 actual registrar replay/stack refused");
    frame->actual_job=&job;
    lua_createtable(L,6,0);const int keys=lua_gettop(L);frame->keys=lua_topointer(L,keys);
    for(int i=0;i<6;++i){lua_pushstring(L,i?native_profiler_entries[i-1].name:"Profiler");lua_rawseti(L,keys,i+1);}
    lua_pushvalue(L,keys);frame->keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    if(frame->keys_ref<0)return (void)luaL_error(L,"Profiler actual retained literal keys failed");
    native_context_profiler_registration_full(L,job);lua_pop(L,1);
    // Original457427 PUSH ESI passes the actual selected518 thread.
    // Borrow it once while idle; callback FULL checks custody without an idle query.
    const int top=lua_gettop(L);perform_native_context_state_query(L,job);
    if(!p.lua_events_frame||!job.context_state_thread||job.worldview_child>=p.child_contexts.size()||
       job.context_state_thread!=p.lua_events_frame->actual_thread||job.context_state_environment!=p.lua_events_frame->actual_environment)
        return (void)luaL_error(L,"Profiler original selected518 borrower changed");
    frame->execution_thread=job.context_state_thread;frame->selected_environment=job.context_state_environment;
    frame->selected_thread_ref=p.child_contexts[job.worldview_child].execution_thread;
    frame->selected_environment_ref=p.child_contexts[job.worldview_child].environment;
    lua_settop(L,top);native_context_profiler_registration_full(L,job);
    if(!lua_checkstack(frame->execution_thread,32))return (void)luaL_error(L,"Profiler selected518 actual stack unavailable");
    const int selected_top=lua_gettop(frame->execution_thread);frame->entered=true;
    native_selected_registrar_begin_owned(L,job,frame,frame->execution_thread,native_profiler_factory_4d1f50);
    native_context_profiler_registration_full(L,job);
    frame->status=lua_cpcall(frame->execution_thread,native_profiler_factory_4d1f50,nullptr);
    native_selected_registrar_after_cpcall_owned(L,frame->status);
    if(frame->status!=0){lua_xmove(frame->execution_thread,L,1);native_selected_registrar_finish_owned(L);return (void)lua_error(L);}
    native_selected_registrar_finish_owned(L);
    native_context_profiler_registration_full(L,job);
    if(lua_gettop(frame->execution_thread)!=selected_top||lua_gettop(L)!=top||frame->stores!=1||frame->fields_created!=5||frame->table_ref<0||!frame->table)
        return (void)luaL_error(L,"Profiler protected constructor did not return original stack/publication");
    frame->returned=true;
    job.context_profiler_reference=frame->table_ref;job.context_profiler_table=frame->table;job.context_profiler_status=frame->status;
    native_context_profiler_registration_full(L,job);
}
