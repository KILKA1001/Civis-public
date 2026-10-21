// Actual5FC250, exact fresh retail image/lazy-initializer/reader semantics.
// CAA844 and CAADD8..CAAE9B are proven zero-filled virtual .data tail.
// This owns a new Civis session registry. It does not inspect or assert the
// contents of an already running original process, or reuse builtinCB6168.
class NativeFLuaTypedRegistryOwner final {
    friend struct Impl;
    using Consumer=void(*)(lua_State*,void*,void*);
    struct Record final {void* word0=nullptr;void* word4=nullptr;Consumer callback=nullptr;};
    Impl*const host;const ScriptSourceProvider*const source;
    const std::thread::id creator;
    const std::uint64_t epoch,writes,uncertainty;
    enum class Phase {fresh_image,vm_bound,retired};Phase phase=Phase::fresh_image;
    lua_State* master=nullptr;global_State* actual_global=nullptr;const Table* lua_registry=nullptr;
    // These are mutable runtime fields, not a constant empty-loop decision.
    // The sole discovered original count writer is the real lazy initializer
    // at5FC263. No external row/callback seed or readiness flag is accepted.
    std::uint32_t guard=0,count=0;
    std::array<Record,16> records{};
    std::uint64_t actual_readers=0,lazy_initializers=0;
    explicit NativeFLuaTypedRegistryOwner(Impl& p):host(&p),source(&p.sources),creator(std::this_thread::get_id()),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){}
    NativeFLuaTypedRegistryOwner(const NativeFLuaTypedRegistryOwner&)=delete;
    NativeFLuaTypedRegistryOwner& operator=(const NativeFLuaTypedRegistryOwner&)=delete;
    void physical_current_without_callbacks()const {
        if(host->native_flua_typed_registry.get()!=this||source!=&host->sources||phase!=Phase::vm_bound||
           creator!=std::this_thread::get_id()||creator!=host->ui_thread||epoch!=host->failure_epoch||
           writes!=host->failed.writes_||uncertainty!=host->execution_uncertainty||host->failed.exhausted_||
           !master||host->state!=master||!actual_global||G(master)!=actual_global||
           !ttistable(&actual_global->l_registry)||hvalue(&actual_global->l_registry)!=lua_registry||
           count>records.size())
            throw ScriptError("Native5FC250 actual fresh-session registry/Source/VM/count retired");
    }
public:~NativeFLuaTypedRegistryOwner()=default;
};
void initialize_native_flua_typed_registry_before_vm_owned(){
    if(native_flua_typed_registry||state||calling||std::this_thread::get_id()!=ui_thread||failed.exhausted_)
        throw ScriptError("Native5FC250 image-state supplier requires the actual first pre-VM session constructor");
    native_flua_typed_registry.reset(new NativeFLuaTypedRegistryOwner(*this));
    // Real fresh data are initialized by this issuer, rather than supplied as
    // a caller count/list. The constructor has no Lua/callback boundary.
}
void bind_native_flua_typed_registry_actual_vm_owned(){
    auto*const actual=native_flua_typed_registry.get();
    if(!actual||actual->host!=this||actual->source!=&sources||actual->phase!=NativeFLuaTypedRegistryOwner::Phase::fresh_image||
       !state||calling||std::this_thread::get_id()!=ui_thread||actual->epoch!=failure_epoch||
       actual->writes!=failed.writes_||actual->uncertainty!=execution_uncertainty||failed.exhausted_||
       !ttistable(&G(state)->l_registry))
        throw ScriptError("Native5FC250 registry requires the actual newly created master VM");
    actual->master=state;actual->actual_global=G(state);actual->lua_registry=hvalue(&G(state)->l_registry);
    actual->phase=NativeFLuaTypedRegistryOwner::Phase::vm_bound;actual->physical_current_without_callbacks();
}
static NativeFLuaTypedRegistryOwner* native_flua_typed_registry_current(lua_State* L){
    auto& p=owner(L);auto*const actual=p.native_flua_typed_registry.get();
    auto*const boundary=p.native_master_protected_boundary;
    // Master initialization has an authentic initialize Job but no entered
    // WorldView Source loan yet. Later original5FC250 readers must have that
    // same real callback-origin Source validation as the surrounding job.
    if(!boundary)return (native_master_thread_refuse(L,"Native5FC250 real protected Job boundary absent"),nullptr);
    if(boundary->actual_job.operation!=Operation::initialize)native_db_query_current(L);
    p.source_error=nullptr;p.source_exception={};
    try{
        if(!actual||p.native_flua_typed_registry.get()!=actual||!p.state||G(L)!=G(p.state))
            throw ScriptError("Native5FC250 registry offered outside the genuine master VM");
        boundary->current_without_lua();actual->physical_current_without_callbacks();
    }catch(...){p.source_exception=std::current_exception();p.source_error="Native5FC250 current registry/Source FIRST refused";}
    if(p.source_error)return (native_master_thread_refuse(L,p.source_error),nullptr);
    // Existing closed initializer produced the actual master Globals, Threads
    // and create/destroy closures. This does not execute a default thread.
    native_master_thread_physical(L,p.native_master_thread_bootstrap.get());
    if(p.native_flua_typed_registry.get()!=actual)
        return (native_master_thread_refuse(L,"Native5FC250 registry storage changed during physical validation"),nullptr);
    return actual;
}
static void native_flua_typed_registry_5fc250_owned(lua_State* L){
    auto& p=owner(L);auto*const actual=native_flua_typed_registry_current(L);
    if(actual->actual_readers==UINT64_MAX)
        return (void)native_master_thread_refuse(L,"Native5FC250 actual reader history exhausted");
    ++actual->actual_readers;
    // Exact5FC253..5FC269: test real bit, set it, then write current count0.
    // Subsequent calls keep the current mutable count and records.
    if((actual->guard&1u)==0){
        actual->guard|=1u;actual->count=0;++actual->lazy_initializers;
    }
    (void)native_flua_typed_registry_current(L);
    std::uint32_t ordinal=0;
    while(ordinal<actual->count){
        if(ordinal>=actual->records.size())
            return (void)native_master_thread_refuse(L,"Native5FC250 current record storage/count mismatch");
        const auto callback=actual->records[ordinal].callback;
        void*const argument2=actual->records[ordinal].word4;
        void*const argument3=actual->records[ordinal].word0;
        if(!callback)return (void)native_master_thread_refuse(L,"Native5FC250 live record lacks its actual registrar callback");
        // Original5FC288..28B invokes the registrar now, in argument order
        // callback(actualL,row.word4,row.word0). It is not a method closure.
        p.source_error=nullptr;p.source_exception={};
        try{callback(L,argument2,argument3);}
        catch(...){p.source_exception=std::current_exception();p.source_error="Native5FC250 actual registrar callback threw";}
        if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
        if(native_flua_typed_registry_current(L)!=actual)
            return (void)native_master_thread_refuse(L,"Native5FC250 registry changed after callback");
        ++ordinal;
        // Actual5FC294 rereads current count AFTER every callback. Do not
        // snapshot it or declare the registry frozen/empty after first use.
    }
    (void)native_flua_typed_registry_current(L);
}
void native_flua_typed_registry_retire()noexcept{
    if(native_flua_typed_registry){native_flua_typed_registry->phase=NativeFLuaTypedRegistryOwner::Phase::retired;
        native_flua_typed_registry->master=nullptr;native_flua_typed_registry->actual_global=nullptr;native_flua_typed_registry->lua_registry=nullptr;}
}
