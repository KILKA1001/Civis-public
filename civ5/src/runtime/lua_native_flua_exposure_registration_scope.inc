// Genuine registration-only owner for original477B20 and76B700. This is
// distinct from any entered physical Root control/CData or method invocation.
class NativeFLuaExposureRegistrationOwner final {
    friend struct Impl;
    Impl*const host;const ScriptSourceProvider*const source;
    const std::thread::id creator;
    const std::uint64_t epoch,writes,uncertainty;
    lua_State*const master;global_State*const actual_global;const Table*const lua_registry;
    bool retired=false;
    enum class Stage {idle,typed,named,builtin,returned};Stage stage=Stage::idle;
    Job* actual_job=nullptr;lua_State* registration_thread=nullptr;
    std::uint32_t actual_descriptor=0,actual_callback=0;
    std::size_t actual_ordinal=0;
    // A nested parent metatype changes the active descriptor while keeping
    // the same genuine callback/Job. Slots are owned across a Lua longjmp.
    std::array<std::uint32_t,64> descriptor_stack{};std::size_t descriptor_depth=0;
    explicit NativeFLuaExposureRegistrationOwner(Impl& p):host(&p),source(&p.sources),
        creator(std::this_thread::get_id()),epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty),
        master(p.state),actual_global(G(p.state)),lua_registry(hvalue(&G(p.state)->l_registry)){}
    NativeFLuaExposureRegistrationOwner(const NativeFLuaExposureRegistrationOwner&)=delete;
    NativeFLuaExposureRegistrationOwner& operator=(const NativeFLuaExposureRegistrationOwner&)=delete;
public:~NativeFLuaExposureRegistrationOwner()=default;
};
static bool native_flua_exposure_metadata_current_if_entered(lua_State* L,std::uint32_t descriptor=0){
    auto& p=owner(L);auto*const actual=p.native_flua_exposure_registration.get();
    auto*const boundary=p.native_master_protected_boundary;
    // Absence returns false so established ordinary/postread guards remain
    // in force. A stale/present entered owner is an error, never a fallback.
    if(!actual||actual->stage==NativeFLuaExposureRegistrationOwner::Stage::idle||
       actual->stage==NativeFLuaExposureRegistrationOwner::Stage::returned)return false;
    p.source_error=nullptr;p.source_exception={};
    try{
        if(actual->host!=&p||actual->source!=&p.sources||actual->retired||!boundary||
           actual->actual_job!=&boundary->actual_job||actual->actual_job->owner!=&p||
           (actual->actual_job->operation!=Operation::initialize&&
            actual->actual_job->operation!=Operation::native_context_flua_registration_prefix)||
           actual->creator!=std::this_thread::get_id()||actual->creator!=p.ui_thread||
           actual->epoch!=p.failure_epoch||actual->writes!=p.failed.writes_||
           actual->uncertainty!=p.execution_uncertainty||p.failed.exhausted_||
           !p.state||actual->master!=p.state||G(L)!=actual->actual_global||
           G(p.state)!=actual->actual_global||!ttistable(&actual->actual_global->l_registry)||
           hvalue(&actual->actual_global->l_registry)!=actual->lua_registry||
           actual->registration_thread!=L||actual->descriptor_depth>actual->descriptor_stack.size())
            throw ScriptError("FLua exposure actual registration Job/Source/VM/descriptor stack retired");
        boundary->current_without_lua();
    }catch(...){p.source_exception=std::current_exception();p.source_error="FLua exposure metadata scope FIRST refused";}
    if(p.source_error)return (native_master_thread_refuse(L,p.source_error),false);
    if(boundary->actual_job.operation!=Operation::initialize)native_db_query_current(L);
    if(p.native_flua_exposure_registration.get()!=actual||p.native_master_protected_boundary!=boundary)
        return (native_master_thread_refuse(L,"FLua exposure owning slots changed during Source FULL"),false);
    native_master_thread_physical(L,p.native_master_thread_bootstrap.get());
    if(descriptor&&(actual->stage!=NativeFLuaExposureRegistrationOwner::Stage::builtin||
       actual->actual_descriptor!=descriptor))return false;
    return true;
}
static void native_flua_exposure_registration_full(lua_State* L){
    if(native_flua_exposure_metadata_current_if_entered(L))return;
    // Construction helpers may also be called by real ordinary consumers.
    // Their established physical Source guard remains the only fallback.
    native_ordinary_metatype_full(L);
}
static void native_flua_exposure_begin_owned(lua_State* L,Job& job){
    auto& p=owner(L);auto*const boundary=p.native_master_protected_boundary;
    if(!boundary||&boundary->actual_job!=&job||job.owner!=&p||!p.state||G(L)!=G(p.state)||
       (job.operation!=Operation::initialize&&job.operation!=Operation::native_context_flua_registration_prefix))
        return (void)native_master_thread_refuse(L,"FLua exposure requires its genuine entered protected initializer/FLua Job");
    if(!p.native_flua_exposure_registration){
        p.source_error=nullptr;p.source_exception={};
        try{p.native_flua_exposure_registration.reset(new NativeFLuaExposureRegistrationOwner(p));}
        catch(...){p.source_exception=std::current_exception();p.source_error="FLua exposure genuine registration owner allocation refused";}
        if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
    }
    auto*const actual=p.native_flua_exposure_registration.get();
    if(actual->stage!=NativeFLuaExposureRegistrationOwner::Stage::idle&&
       actual->stage!=NativeFLuaExposureRegistrationOwner::Stage::returned)
        return (void)native_master_thread_refuse(L,"FLua exposure callback registration reentered");
    actual->actual_job=&job;actual->registration_thread=L;actual->stage=NativeFLuaExposureRegistrationOwner::Stage::typed;
    actual->actual_descriptor=0;actual->actual_callback=0;actual->actual_ordinal=0;actual->descriptor_depth=0;
    (void)native_flua_exposure_metadata_current_if_entered(L);
}
static void native_flua_exposure_returned_owned(lua_State* L){
    auto& p=owner(L);if(!native_flua_exposure_metadata_current_if_entered(L))
        return (void)native_master_thread_refuse(L,"FLua exposure return has no genuine entered registrar");
    auto*const actual=p.native_flua_exposure_registration.get();
    if(actual->stage!=NativeFLuaExposureRegistrationOwner::Stage::builtin||actual->descriptor_depth)
        return (void)native_master_thread_refuse(L,"FLua exposure return requires the complete builtin callback/parent stack");
    actual->actual_job=nullptr;actual->registration_thread=nullptr;actual->actual_descriptor=0;
    actual->actual_callback=0;actual->stage=NativeFLuaExposureRegistrationOwner::Stage::returned;
}
void native_flua_exposure_registration_retire()noexcept{
    if(native_flua_exposure_registration){native_flua_exposure_registration->retired=true;
        native_flua_exposure_registration->actual_job=nullptr;native_flua_exposure_registration->registration_thread=nullptr;}
}
