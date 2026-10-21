// Real shared9C5E90/81CE20 ->5BB7E24 recursive Windows critical section,
// real8F7AE0 traceback/pcall/error path and actual default8F79C0/8F74F0 logger.
// This is a protected-call result, not a default-thread or wholeSetup grant.
class NativeMasterLuaCriticalSection final {
    friend struct Impl;
    CRITICAL_SECTION actual{};
    bool default_logger_initialized=false;
    using ErrorSink=void(*)(const char*);
    ErrorSink actual_default_error_sink=nullptr;
    NativeMasterLuaCriticalSection(){::InitializeCriticalSection(&actual);}
    NativeMasterLuaCriticalSection(const NativeMasterLuaCriticalSection&)=delete;
    NativeMasterLuaCriticalSection& operator=(const NativeMasterLuaCriticalSection&)=delete;
public:~NativeMasterLuaCriticalSection(){::DeleteCriticalSection(&actual);}
};
void initialize_native_master_protected_call_before_vm_owned(){
    if(state||calling||native_master_lua_critical_section||std::this_thread::get_id()!=ui_thread||failed.exhausted_)
        throw ScriptError("Native Lua shared critical section requires actual first pre-VM Host constructor");
    // Like original9C5E90+atexit9C8180, there is one actual process-wide
    // recursive CS. Every Host and every borrowed boundary retains its owner.
    // No per-VM mutex or foreign startup metadata replaces that shared object.
    static const std::shared_ptr<NativeMasterLuaCriticalSection> actual_process_owner(new NativeMasterLuaCriticalSection);
    native_master_lua_critical_section=actual_process_owner;
}
class NativeMasterProtectedLuaBoundary final {
    friend struct Impl;
    Impl& host;Job& actual_job;lua_State*const master;
    const std::shared_ptr<NativeMasterLuaCriticalSection> critical;
    const std::thread::id creator;
    const std::uint64_t serial;
    std::uint64_t invocations=0,held_locks=0;
    enum class Phase {prepared,in_cpcall,returned,retired};
    Phase phase=Phase::prepared;
public:
    NativeMasterProtectedLuaBoundary(Impl&p,Job& job):host(p),actual_job(job),master(p.state),critical(p.native_master_lua_critical_section),
        creator(std::this_thread::get_id()),serial(p.native_master_protected_boundary_serial==UINT64_MAX?UINT64_MAX:++p.native_master_protected_boundary_serial){
        if(p.native_master_protected_boundary||p.calling||job.owner!=&p||!master||!critical||serial==UINT64_MAX||
           creator!=p.ui_thread||p.failed.exhausted_)
            throw ScriptError("Native Lua helper boundary requires actual idle Impl::call/Job/master/CS");
        p.native_master_protected_boundary=this;
    }
    NativeMasterProtectedLuaBoundary(const NativeMasterProtectedLuaBoundary&)=delete;
    NativeMasterProtectedLuaBoundary& operator=(const NativeMasterProtectedLuaBoundary&)=delete;
    ~NativeMasterProtectedLuaBoundary(){
        if(creator!=std::this_thread::get_id())std::terminate();
        release_after_abort();phase=Phase::retired;
        if(host.native_master_protected_boundary==this)host.native_master_protected_boundary=nullptr;
    }
    void begin_cpcall(){
        if(host.native_master_protected_boundary!=this||phase!=Phase::prepared||!host.calling||host.state!=master||actual_job.owner!=&host)
            throw ScriptError("Native Lua helper C++ boundary did not enter its real cpcall");
        phase=Phase::in_cpcall;
    }
    void release_after_abort()noexcept{
        // Real held locks survive Lua's longjmp only in this C++ owner outside
        // lua_cpcall. Release remains mandatory even after Source revocation.
        if(held_locks){host.record_failure();while(held_locks){::LeaveCriticalSection(&critical->actual);--held_locks;}}
    }
    void after_cpcall()noexcept{release_after_abort();phase=Phase::returned;}
    void current_without_lua()const{
        if(host.native_master_protected_boundary!=this||phase!=Phase::in_cpcall||!host.calling||host.state!=master||
           actual_job.owner!=&host||serial!=host.native_master_protected_boundary_serial||!critical||
           critical.get()!=host.native_master_lua_critical_section.get()||critical.owner_before(host.native_master_lua_critical_section)||
           host.native_master_lua_critical_section.owner_before(critical)||creator!=std::this_thread::get_id()||creator!=host.ui_thread)
            throw ScriptError("Native Lua protected helper actual C++ boundary/master/CS retired");
    }
};
class NativeMasterProtectedCallReturn final {
    friend struct Impl;
    Impl*const host;lua_State*const actual_vm;
    const NativeMasterProtectedLuaBoundary*const boundary;
    const std::uint64_t boundary_serial,invocation;
    const NativeMasterThreadCallbackSnapshot source;
    const int original_arguments,original_results,entry_top;
    int actual_status=-1; // only the actual lua_pcall return writes this field
    int actual_return_top=0;
    bool retired=false;
    NativeMasterProtectedCallReturn(Impl&p,lua_State* L,NativeMasterProtectedLuaBoundary& f,const NativeMasterThreadCallbackSnapshot& saved,int nargs,int nresults)
        :host(&p),actual_vm(L),boundary(&f),boundary_serial(f.serial),invocation(f.invocations),source(saved),
         original_arguments(nargs),original_results(nresults),entry_top(lua_gettop(L)){}
    NativeMasterProtectedCallReturn(const NativeMasterProtectedCallReturn&)=delete;
    NativeMasterProtectedCallReturn& operator=(const NativeMasterProtectedCallReturn&)=delete;
public:~NativeMasterProtectedCallReturn()=default;
};
static void native_master_protected_boundary_current(lua_State* L,const NativeMasterProtectedLuaBoundary* actual){
    auto& p=owner(L);native_db_query_current(L);
    if(!actual||p.native_master_protected_boundary!=actual||!p.state||G(L)!=G(p.state))
        return (void)native_master_thread_refuse(L,"Native Lua protected call has no actual C++ boundary in this VM");
    p.source_error=nullptr;p.source_exception={};
    try{actual->current_without_lua();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Native Lua protected helper current boundary refused";}
    if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
}
static NativeMasterProtectedCallReturn* native_master_protected_result_prefix_current(lua_State* L,const NativeMasterProtectedCallReturn* offered){
    auto& p=owner(L);NativeMasterProtectedCallReturn* actual=nullptr;
    // Recover real storage FIRST. A stale borrowed boundary/result pointer is
    // never dereferenced before its owning Host slot has matched it.
    for(const auto& item:p.native_master_protected_call_results)if(item.get()==offered){actual=item.get();break;}
    if(!actual||actual->retired||actual->host!=&p||actual->actual_vm!=L||!p.native_master_protected_boundary||
       actual->boundary!=p.native_master_protected_boundary||actual->boundary_serial!=p.native_master_protected_boundary_serial)
        return (native_master_thread_refuse(L,"Native Lua protected result lacks its actual current owner/boundary"),nullptr);
    native_master_protected_boundary_current(L,actual->boundary);
    native_master_thread_callback_post(L,actual->source);
    if(actual->invocation==0||actual->invocation>actual->boundary->invocations)
        return (native_master_thread_refuse(L,"Native Lua protected result has no actual invocation history"),nullptr);
    return actual;
}
static void native_master_protected_call_return_current_owned(lua_State* L,const NativeMasterProtectedCallReturn* result){
    const auto*const actual=native_master_protected_result_prefix_current(L,result);
    if(actual->actual_status<0)
        (void)native_master_thread_refuse(L,"Native Lua protected result has no actual completed pcall return");
}
static void native_master_default_error_sink_8f74f0(const char* message){
    // Original __iob_func()+2 targets stderr. The exact default consumer calls
    // real CRT fprintf_s with the proven "%s\n", rather than discarding output.
    (void)::fprintf_s(stderr,"%s\n",message);
}
static void native_master_log_error_8f79c0_owned(lua_State* L,NativeMasterProtectedCallReturn* result,int status){
    auto& p=owner(L);native_master_protected_result_prefix_current(L,result);
    // POD scratch is safe across Lua allocating/error APIs. Source strings
    // are used only after fresh POST, like the actual native debugger fields.
    lua_Debug info{};char stack_line[1024]{};char message[1024]{};
    for(int level=1;level<20;++level){
        const int has_frame=lua_getstack(L,level,&info);native_master_protected_result_prefix_current(L,result);
        if(has_frame){
            (void)lua_getinfo(L,"Sl",&info);native_master_protected_result_prefix_current(L,result);
            if(info.currentline>0){
                (void)::sprintf_s(stack_line,sizeof(stack_line),"%s:%d\n",info.source?info.source:"",info.currentline);
                native_master_protected_result_prefix_current(L,result);
            }
        }
    }
    const char*const format=status==LUA_ERRRUN?"Runtime Error: %s":status==LUA_ERRMEM?"Memory Allocation Error: %s":
        status==LUA_ERRERR?"Error Handler Error: %s":"%s";
    const char*const error=lua_tolstring(L,-1,nullptr);native_master_protected_result_prefix_current(L,result);
    (void)::sprintf_s(message,sizeof(message),format,error);native_master_protected_result_prefix_current(L,result);
    auto*const critical=p.native_master_lua_critical_section.get();
    if(!critical||critical!=result->boundary->critical.get())
        return (void)native_master_thread_refuse(L,"Native Lua actual default logger/CS owner retired");
    // This is the real lazy original8F7A69..8F7A89 default writer in the
    // newly owned runtime domain. Native custom sink rebinding is not issued.
    if(!critical->default_logger_initialized){
        critical->actual_default_error_sink=native_master_default_error_sink_8f74f0;
        critical->default_logger_initialized=true;
    }
    if(critical->actual_default_error_sink!=native_master_default_error_sink_8f74f0)
        return (void)native_master_thread_refuse(L,"Native Lua actual custom logger requires its genuine provider");
    critical->actual_default_error_sink(message);native_master_protected_result_prefix_current(L,result);
}
static const NativeMasterProtectedCallReturn* native_master_protected_call_8f7ae0_owned(lua_State* L,int nargs,int nresults){
    auto& p=owner(L);const auto*const boundary=p.native_master_protected_boundary;
    native_master_protected_boundary_current(L,boundary);
    const auto saved=native_master_thread_callback_first(L);
    if(nargs<0||nresults<0||nargs>lua_gettop(L)-1)
        return (native_master_thread_refuse(L,"Native Lua protected helper requires actual fixed argument/result call slots"),nullptr);
    NativeMasterProtectedCallReturn* actual=nullptr;p.source_exception={};p.source_error=nullptr;
    // All owning C++ temporaries die before any Lua API. The persistent result
    // is in Host custody; no ownership is stranded by a Lua longjmp.
    try{
        auto*const f=p.native_master_protected_boundary;f->current_without_lua();
        if(f->invocations==UINT64_MAX)throw ScriptError("Native Lua protected helper invocation history exhausted");
        ++f->invocations;
        std::unique_ptr<NativeMasterProtectedCallReturn> retained(new NativeMasterProtectedCallReturn(p,L,*f,saved,nargs,nresults));
        p.native_master_protected_call_results.push_back(std::move(retained));
        actual=p.native_master_protected_call_results.back().get();
    }catch(...){p.source_exception=std::current_exception();p.source_error="Native Lua protected helper actual history allocation refused";}
    if(p.source_error)return (native_master_thread_refuse(L,p.source_error),nullptr);
    native_master_protected_result_prefix_current(L,actual);
    auto*const f=p.native_master_protected_boundary;
    if(f->held_locks==UINT64_MAX)return (native_master_thread_refuse(L,"Native Lua real recursive lock history exhausted"),nullptr);
    ::EnterCriticalSection(&f->critical->actual);++f->held_locks;
    native_master_protected_result_prefix_current(L,actual);
    int error_handler=0;
    if(!lua_checkstack(L,1))return (native_master_thread_refuse(L,"Native Lua original traceback stack unavailable"),nullptr);
    native_master_protected_result_prefix_current(L,actual);
    lua_getfield(L,LUA_GLOBALSINDEX,"quicktraceback");native_master_protected_result_prefix_current(L,actual);
    if(lua_type(L,-1)==LUA_TFUNCTION){
        error_handler=-2-nargs;lua_insert(L,error_handler);native_master_protected_result_prefix_current(L,actual);
    }else{
        lua_pop(L,1);
        if(!lua_checkstack(L,2))return (native_master_thread_refuse(L,"Native Lua original debug traceback stack unavailable"),nullptr);
        native_master_protected_result_prefix_current(L,actual);
        lua_getfield(L,LUA_GLOBALSINDEX,"debug");native_master_protected_result_prefix_current(L,actual);
        if(lua_type(L,-1)==LUA_TTABLE){
            lua_getfield(L,-1,"traceback");native_master_protected_result_prefix_current(L,actual);
            if(lua_type(L,-1)==LUA_TFUNCTION){
                lua_remove(L,-2);lua_pushvalue(L,-1);
                lua_setfield(L,LUA_GLOBALSINDEX,"quicktraceback");native_master_protected_result_prefix_current(L,actual);
                // Actual raw8F7BA5..B8 continues after the cached false
                // no-return setfield and inserts the newly cached handler.
                error_handler=-2-nargs;lua_insert(L,error_handler);native_master_protected_result_prefix_current(L,actual);
            }else lua_pop(L,2);
        }else lua_pop(L,1);
    }
    const int status=lua_pcall(L,nargs,nresults,error_handler);
    native_master_protected_result_prefix_current(L,actual);
    if(status==0){
        if(error_handler){lua_remove(L,-1-nresults);native_master_protected_result_prefix_current(L,actual);}
    }else{
        native_master_log_error_8f79c0_owned(L,actual,status);
        lua_pop(L,error_handler?2:1);native_master_protected_result_prefix_current(L,actual);
    }
    if(!f->held_locks)return (native_master_thread_refuse(L,"Native Lua helper lost its actual recursive lock"),nullptr);
    ::LeaveCriticalSection(&f->critical->actual);--f->held_locks;
    native_master_protected_result_prefix_current(L,actual);
    const int expected_top=actual->entry_top-nargs-1+(status==0?nresults:0);
    if(lua_gettop(L)!=expected_top)
        return (native_master_thread_refuse(L,"Native Lua protected helper actual result stack differs"),nullptr);
    actual->actual_return_top=lua_gettop(L);actual->actual_status=status;
    native_master_protected_call_return_current_owned(L,actual);return actual;
}
void native_master_protected_calls_retire()noexcept{
    for(const auto& result:native_master_protected_call_results)if(result)result->retired=true;
    if(native_master_protected_boundary)native_master_protected_boundary->release_after_abort();
    native_master_protected_boundary=nullptr;
}
