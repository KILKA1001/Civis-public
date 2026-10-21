// Inside Impl after the real master protected boundary. Only original selected
// Profiler4D1F50/PreGame751450 producers issue this Host-owned operation.
// No CFunction-local owning RAII crosses a Lua longjmp.
struct NativeSelectedRegistrarProtectedCall final{
    enum class Phase{prepared,callback,callback_returned,returned,error};
    Impl*const host;Job*const job;const void*const actor;
    const ScriptSourceProvider*const source;const std::thread::id creator;
    lua_State*const master;lua_State*const selected;global_State*const global;const Table*const registry_table;
    const NativeMasterProtectedLuaBoundary*const boundary;
    const std::uint64_t epoch,writes,uncertainty;
    const std::size_t child;const int thread_ref,environment_ref;const void*const environment;
    const std::weak_ptr<const ScriptUiChildSource> child_source;const ScriptUiChildSource*const child_get;
    lua_CFunction const callback;const int before_top,before_ci,before_stack_top;
    const Closure* callback_closure=nullptr;
    int callback_ci=-1,callback_func=-1,callback_base=-1,entered_top=-1,return_top=-1,after_top=-1,status=-1;
    std::array<unsigned char,sizeof(TValue)> error_bytes{};
    Phase phase=Phase::prepared;
    NativeSelectedRegistrarProtectedCall(Impl&p,Job&j,const void*f,lua_State*t,lua_CFunction cb)
        :host(&p),job(&j),actor(f),source(&p.sources),creator(std::this_thread::get_id()),
         master(p.state),selected(t),global(G(p.state)),registry_table(hvalue(&G(p.state)->l_registry)),boundary(p.native_master_protected_boundary),
         epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty),
         child(j.worldview_child),thread_ref(p.child_contexts[child].execution_thread),
         environment_ref(p.child_contexts[child].environment),environment(j.context_state_environment),
         child_source(p.child_contexts[child].source),child_get(p.child_contexts[child].source.get()),
         callback(cb),before_top(lua_gettop(t)),before_ci(static_cast<int>(t->ci-t->base_ci)),before_stack_top(static_cast<int>(t->top-t->stack)){}
    void first()const{
        auto&p=*host;
        if(creator!=std::this_thread::get_id()||creator!=p.ui_thread||source!=&p.sources||
           !p.calling||!p.state||p.state!=master||G(master)!=global||!selected||G(selected)!=global||
           !ttistable(&global->l_registry)||hvalue(&global->l_registry)!=registry_table||
           p.native_selected_registrar_call.get()!=this||p.native_master_protected_boundary!=boundary||
           !boundary||&boundary->actual_job!=job||job->owner!=host||
           epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty||p.failed.exhausted_||
           child>=p.child_contexts.size()||job->worldview_child!=child||job->context_state_thread!=selected||
           job->context_state_environment!=environment)
            throw ScriptError("Selected registrar actual Job/VM/Source/failure custody changed");
        const bool profiler=job->operation==Operation::native_context_profiler_registration&&actor==p.profiler_registration_frame&&callback==native_profiler_factory_4d1f50;
        const bool pregame=job->operation==Operation::native_context_pregame_registration&&actor==p.pregame_frame&&callback==native_pregame_register_751450_owned;
        if(!profiler&&!pregame)throw ScriptError("Selected registrar actual producer/frame/callback changed");
        const auto&c=p.child_contexts[child];
        if(c.retired||!c.state_prepared||c.execution_thread!=thread_ref||c.environment!=environment_ref||
           !c.source||c.source.get()!=child_get||child_source.expired()||child_source.owner_before(c.source)||c.source.owner_before(child_source)||
           !p.worldview_native_host||p.worldview_native_host->child!=child||
           child_source.owner_before(p.worldview_native_host->child_source)||p.worldview_native_host->child_source.owner_before(child_source))
            throw ScriptError("Selected registrar SAME selected child/source controls changed");
        boundary->current_without_lua();p.worldview_require_callback_origin();
    }
    bool callback_current()const{
        if(callback_ci<0||selected->ci-selected->base_ci!=callback_ci)return false;
        const auto*ci=selected->ci;
        return ci->func-selected->stack==callback_func&&ci->base-selected->stack==callback_base&&
            ttisfunction(ci->func)&&clvalue(ci->func)==callback_closure&&clvalue(ci->func)->c.isC&&clvalue(ci->func)->c.nupvalues==0&&
            clvalue(ci->func)->c.f==callback&&lua_gettop(selected)>=1&&
            lua_type(selected,1)==LUA_TLIGHTUSERDATA&&lua_touserdata(selected,1)==nullptr;
    }
};
static void native_selected_registrar_first_owned(lua_State* L){
    auto&p=owner(L);p.source_exception={};p.source_error=nullptr;
    try{if(!p.native_selected_registrar_call)throw ScriptError("Selected registrar protected operation absent");p.native_selected_registrar_call->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Selected registrar protected FIRST refused";}
    if(p.source_error)native_master_thread_refuse(L,p.source_error);
}
static void native_selected_registrar_begin_owned(lua_State* L,Job&job,const void*actor,lua_State*selected,lua_CFunction callback){
    auto&p=owner(L);auto*const boundary=p.native_master_protected_boundary;
    if(L!=p.state||!p.calling||job.owner!=&p||p.native_selected_registrar_call||!boundary||&boundary->actual_job!=&job||
       job.worldview_child>=p.child_contexts.size()||!selected||selected==p.state||G(selected)!=G(p.state)||
       selected!=job.context_state_thread||!job.context_state_environment||lua_gettop(selected)||lua_status(selected))
        return (void)native_master_thread_refuse(L,"Selected registrar needs its actual idle selected518 and current protected Job");
    const bool profiler=job.operation==Operation::native_context_profiler_registration&&actor==p.profiler_registration_frame&&callback==native_profiler_factory_4d1f50;
    const bool pregame=job.operation==Operation::native_context_pregame_registration&&actor==p.pregame_frame&&callback==native_pregame_register_751450_owned;
    if(!profiler&&!pregame)return (void)native_master_thread_refuse(L,"Selected registrar closed producer/callback mismatch");
    p.source_exception={};p.source_error=nullptr;
    try{p.native_selected_registrar_call.reset(new NativeSelectedRegistrarProtectedCall(p,job,actor,selected,callback));}
    catch(...){p.source_exception=std::current_exception();p.source_error="Selected registrar actual operation allocation refused";}
    if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
    native_selected_registrar_first_owned(L);
}
static void native_selected_registrar_callback_enter_owned(lua_State* L){
    native_selected_registrar_first_owned(L);auto&p=owner(L);auto*const a=p.native_selected_registrar_call.get();
    if(a->phase!=NativeSelectedRegistrarProtectedCall::Phase::prepared||L!=a->selected||lua_gettop(L)!=1||
       lua_type(L,1)!=LUA_TLIGHTUSERDATA||lua_touserdata(L,1)!=nullptr||!ttisfunction(L->ci->func)||
       !clvalue(L->ci->func)->c.isC||clvalue(L->ci->func)->c.nupvalues||clvalue(L->ci->func)->c.f!=a->callback||
       L->ci-L->base_ci!=a->before_ci+1||L->ci->func-L->stack!=a->before_stack_top||L->ci->base!=L->ci->func+1)
        return (void)native_master_thread_refuse(L,"Selected registrar actual zero-upvalue/nulluserdata C callframe absent");
    a->callback_closure=clvalue(L->ci->func);
    a->callback_ci=static_cast<int>(L->ci-L->base_ci);a->callback_func=static_cast<int>(L->ci->func-L->stack);
    a->callback_base=static_cast<int>(L->ci->base-L->stack);a->entered_top=lua_gettop(L);
    a->phase=NativeSelectedRegistrarProtectedCall::Phase::callback;
}
static void native_selected_registrar_callback_return_owned(lua_State* L){
    native_selected_registrar_first_owned(L);auto*a=owner(L).native_selected_registrar_call.get();
    if(L!=a->selected||a->phase!=NativeSelectedRegistrarProtectedCall::Phase::callback||!a->callback_current())
        return (void)native_master_thread_refuse(L,"Selected registrar actual callback return frame changed");
    a->return_top=lua_gettop(L);a->phase=NativeSelectedRegistrarProtectedCall::Phase::callback_returned;
}
static void native_selected_registrar_after_cpcall_owned(lua_State* L,int status){
    auto&p=owner(L);auto*a=p.native_selected_registrar_call.get();
    if(!a||L!=p.state||a->creator!=std::this_thread::get_id())
        return (void)native_master_thread_refuse(L,"Selected registrar actual protected return has no owner");
    a->status=status;a->after_top=lua_gettop(a->selected);
    if(status==0){
        if(a->phase!=NativeSelectedRegistrarProtectedCall::Phase::callback_returned||a->after_top!=a->before_top)
            return (void)native_master_thread_refuse(L,"Selected registrar actual successful cpcall stack/return differs");
        a->phase=NativeSelectedRegistrarProtectedCall::Phase::returned;
    }else{
        if(a->after_top!=a->before_top+1||a->selected->ci-a->selected->base_ci!=a->before_ci)
            return (void)native_master_thread_refuse(L,"Selected registrar actual failed cpcall error stack differs");
        std::memcpy(a->error_bytes.data(),a->selected->top-1,sizeof(TValue));
        a->phase=NativeSelectedRegistrarProtectedCall::Phase::error;
    }
}
// This is a Source FULL supplier check, never an ordinary state-query permit.
static bool native_selected_registrar_busy_thread_owned(lua_State* L,lua_State*selected,int thread_ref,int environment_ref,const void*environment,std::size_t child){
    auto&p=owner(L);const auto*a=p.native_selected_registrar_call.get();if(!a)return false;
    native_selected_registrar_first_owned(L);a=p.native_selected_registrar_call.get();
    if(L!=p.state||selected!=a->selected||thread_ref!=a->thread_ref||environment_ref!=a->environment_ref||
       environment!=a->environment||child!=a->child||lua_status(selected))return false;
    if(a->phase==NativeSelectedRegistrarProtectedCall::Phase::callback||a->phase==NativeSelectedRegistrarProtectedCall::Phase::callback_returned){
        if(!a->callback_current())return false;
    }else if(a->phase==NativeSelectedRegistrarProtectedCall::Phase::error){
        if(a->status==0||a->after_top!=a->before_top+1||lua_gettop(selected)!=a->after_top||
           selected->ci-selected->base_ci!=a->before_ci||std::memcmp(a->error_bytes.data(),selected->top-1,sizeof(TValue)))return false;
    }else return false;
    const int top=lua_gettop(L);lua_rawgeti(L,LUA_REGISTRYINDEX,thread_ref);
    bool current=lua_type(L,-1)==LUA_TTHREAD&&lua_tothread(L,-1)==selected;
    if(current){lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,environment_ref);
        current=lua_istable(L,-1)&&lua_topointer(L,-1)==environment&&lua_rawequal(L,-1,-2)&&
            ttistable(gt(selected))&&hvalue(gt(selected))==environment;
    }
    lua_settop(L,top);native_selected_registrar_first_owned(L);return current;
}
static void native_selected_registrar_finish_owned(lua_State* L){
    auto&p=owner(L);auto*a=p.native_selected_registrar_call.get();
    if(!a||L!=p.state||a->creator!=std::this_thread::get_id()||a->status<0||
       lua_gettop(a->selected)!=a->before_top||a->selected->ci-a->selected->base_ci!=a->before_ci)
        return (void)native_master_thread_refuse(L,"Selected registrar protected return/errorpop did not finish");
    p.native_selected_registrar_call.reset(); // no LuaAPI or borrowed callback
}
void native_selected_registrar_retire_for_frame_owned(const void*actor)noexcept{
    if(native_selected_registrar_call&&native_selected_registrar_call->actor==actor)native_selected_registrar_call.reset();
}
