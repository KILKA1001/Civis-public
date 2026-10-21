// Inside actual Host::Impl, after ChildContext/state preparation. All owners
// and record storage exist OUTSIDE the allocating coroutine cpcall callback.
// New Lua refs may legitimately reuse retired StateName/Controls integers;
// acquisition history and live referents, not historic numbers, prove ownership.
static_assert(LUA_VERSION_NUM==501&&LUA_GLOBALSINDEX==-10002&&LUA_REGISTRYINDEX==-10000&&LUA_NOREF==-2,
    "postRead Globals requires selected Lua5.1 pseudo-index/reference semantics");
struct NativePostreadGlobalsControl final {
    enum class Phase {cold,globals_initialized,globals_pushed,globals_retained,
        key_constructed,key_pushed,key_retained,prefix_returned,cleanup_started,retired,failed};
    const std::thread::id creator=std::this_thread::get_id();
    const std::shared_ptr<const ui::detail::NotificationContextPostreadGlobalsInputLoan> input;
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation> prepared;
    const std::shared_ptr<const ScriptUiChildSource> source;
    lua_State*const master;
    lua_State*const thread;
    const void*const environment;
    const int environment_reference,thread_reference;
    const std::size_t child;
    const std::uint64_t serial,epoch,writes,uncertainty;
    Phase phase=Phase::cold;
    lua_State* globals_state=nullptr;
    int globals_reference=LUA_NOREF;
    lua_State* key_state=nullptr;
    int key_reference=LUA_NOREF;
    int entry_top=0,returned_top=0,globals_checkstack=0,key_checkstack=0;
    bool dropped=false,vm_retired=false;
    bool key_unref_started=false,key_unref_returned=false;
    bool globals_unref_started=false,globals_unref_returned=false;
    const void* globals_pointer=nullptr;const void* key_pointer=nullptr;
    std::weak_ptr<const ui::detail::NotificationContextPostreadGlobalsReference> canonical;
    NativePostreadGlobalsControl(std::shared_ptr<const ui::detail::NotificationContextPostreadGlobalsInputLoan> i,
        std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation> p,
        std::shared_ptr<const ScriptUiChildSource> s,lua_State*m,lua_State*t,const void*e,int env,int thr,std::size_t slot,
        std::uint64_t op,std::uint64_t failure,std::uint64_t changes,std::uint64_t uncertain)
        :input(std::move(i)),prepared(std::move(p)),source(std::move(s)),master(m),thread(t),environment(e),
         environment_reference(env),thread_reference(thr),
         child(slot),serial(op),epoch(failure),writes(changes),uncertainty(uncertain){}
};
static bool native_postread_globals_retained_thread_without_callbacks(Impl&p,const NativePostreadGlobalsControl&r){
    // Validate a fresh physical registry keeper BEFORE dereferencing a saved
    // coroutine pointer. No API allocation, coercion, callback or Source call.
    if(!p.state||r.vm_retired||r.master!=p.state||r.thread_reference<0||r.child>=p.child_contexts.size()||
        p.child_contexts[r.child].execution_thread!=r.thread_reference||p.state->ci->top-p.state->top<1)return false;
    lua_State*L=p.state;const int top=lua_gettop(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.thread_reference);
    lua_State*const current=lua_type(L,-1)==LUA_TTHREAD?lua_tothread(L,-1):nullptr;
    const bool same=current&&current==r.thread&&current!=L;
    lua_settop(L,top);return same;
}
struct NativePostreadGlobalsFrame final {
    Impl& host;const LuaScriptHost& facade;const BnwWorldViewSourceOperationLoan& operation;
    NativePostreadGlobalsControl& control;
    NativePostreadGlobalsFrame(Impl&p,const LuaScriptHost&f,const BnwWorldViewSourceOperationLoan&op,
        NativePostreadGlobalsControl&r):host(p),facade(f),operation(op),control(r){
        if(host.postread_globals_frame)throw ScriptError("postRead Globals physical frame reentered");
        host.postread_globals_frame=this;
    }
    NativePostreadGlobalsFrame(const NativePostreadGlobalsFrame&)=delete;
    ~NativePostreadGlobalsFrame(){
        if(control.creator!=std::this_thread::get_id())std::terminate();
        if(host.postread_globals_frame==this)host.postread_globals_frame=nullptr;
        else host.record_failure();
    }
    void first()const {
        // Actual Host/Source/facade tombstone FIRST before raw facade dispatch,
        // then genuine Tree postRead input before its saved UI fields.
        host.native_postread_globals_first(control);
        facade.require_context_postread_globals_input_without_callbacks(operation,control.input);
        host.native_postread_globals_first(control);
        if(host.postread_globals_frame!=this||host.worldview_native_host->loan!=&operation)
            throw ScriptError("postRead Globals frame/caller changed");
    }
    void full()const {
        first();facade.require_context_postread_globals_input_owned(operation,control.input);first();
        operation.require_same_vm(facade);first();operation.require_same_source(*control.source);first();
    }
};
void native_postread_globals_first(const NativePostreadGlobalsControl&r)const {
    if(r.creator!=std::this_thread::get_id()||r.creator!=ui_thread||r.vm_retired||r.dropped||
        !state||state!=r.master||failed||!end_turn_lifetime||!end_turn_lifetime->alive||
        end_turn_lifetime->failed||end_turn_lifetime->vm!=state||!worldview_native_host||
        !worldview_native_host->host||worldview_native_host->loan==nullptr||
        worldview_native_host->child!=r.child||r.child>=child_contexts.size()||
        r.epoch!=failure_epoch||r.writes!=failed.writes_||r.uncertainty!=execution_uncertainty)
        throw ScriptError("postRead Globals actual Host/coroutine/source frame retired FIRST");
    // Validate the currently published Source scope before using its saved op.
    const auto sources=worldview_sources.lock();
    if(!sources)throw ScriptError("postRead Globals canonical Source retired FIRST");
    const auto&current=sources->current_operation_loan();
    if(&current!=worldview_native_host->loan||current.operation_serial()!=r.serial)
        throw ScriptError("postRead Globals operation/current scope replaced");
    worldview_native_host->pure();
    const auto&c=child_contexts[r.child];
    if(c.source!=r.source||c.retired||c.loaded||c.loading||!c.state_prepared||
        c.environment!=r.environment_reference||c.execution_thread!=r.thread_reference||
        c.environment<0||c.execution_thread<0||worldview_native_host->child_source.lock()!=r.source)
        throw ScriptError("postRead Globals actual prepared child/source changed");
}
static void native_postread_globals_full(lua_State*L,NativePostreadGlobalsControl*r){
    auto&p=owner(L);p.source_exception={};p.source_error=nullptr;
    try{
        if(!r||!p.postread_globals_frame)throw ScriptError("postRead Globals physical protected frame missing");
        p.native_postread_globals_first(*r);
        p.postread_globals_frame->full();
        p.native_postread_globals_first(*r);
    }catch(...){p.source_exception=std::current_exception();p.source_error="postRead Globals SAME owner FULL refused";}
    // No C++ owner, catch object or RAII spans this Lua longjmp.
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}
static bool native_postread_globals_physical(lua_State*L,const NativePostreadGlobalsControl&r,bool refs){
    if(L!=r.thread||lua_status(L)||!lua_istable(L,LUA_GLOBALSINDEX)||
        lua_topointer(L,LUA_GLOBALSINDEX)!=r.environment)return false;
    auto&p=owner(L);if(r.child>=p.child_contexts.size())return false;
    const int top=lua_gettop(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[r.child].execution_thread);
    const bool thread=lua_tothread(L,-1)==L;lua_pop(L,1);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[r.child].environment);
    const bool env=lua_istable(L,-1)&&lua_rawequal(L,-1,LUA_GLOBALSINDEX);lua_pop(L,1);
    bool actual=thread&&env;
    if(refs){
        lua_rawgeti(L,LUA_REGISTRYINDEX,r.globals_reference);
        actual=actual&&r.globals_state==L&&r.globals_reference>=0&&lua_istable(L,-1)&&
            lua_rawequal(L,-1,LUA_GLOBALSINDEX)&&lua_topointer(L,-1)==r.globals_pointer;
        lua_pop(L,1);lua_rawgeti(L,LUA_REGISTRYINDEX,r.key_reference);
        std::size_t length=0;const char*name=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&length):nullptr;
        actual=actual&&r.key_state==L&&r.key_reference>=0&&name&&length==10&&
            std::memcmp(name,"ContextPtr",10)==0&&name==r.key_pointer;
        lua_pop(L,1);
    }
    return actual&&lua_gettop(L)==top;
}
static int native_postread_globals_coroutine(lua_State*L){
    auto*job=static_cast<Job*>(lua_touserdata(L,1));auto&p=owner(L);
    auto*r=job?job->context_postread_globals:nullptr;
    if(!job||job->owner!=&p||!r||r->thread!=L||lua_gettop(L)!=1)
        return luaL_error(L,"postRead Globals actual coroutine cpcall/Job differs");
    // Extra physical query slots belong to the target owner; they are not an
    // Original ignored checkstack result or a synthetic success marker.
    if(!lua_checkstack(L,16))return luaL_error(L,"postRead Globals diagnostic stack reserve failed");
    native_postread_globals_full(L,r);
    if(!native_postread_globals_physical(L,*r,false))
        return luaL_error(L,"postRead Globals actual thread environment mismatch");
    if(job->operation==Operation::native_context_postread_globals_query){
        if(r->phase!=NativePostreadGlobalsControl::Phase::prefix_returned||
            !native_postread_globals_physical(L,*r,true))
            return luaL_error(L,"postRead Globals retained live local refs changed");
        native_postread_globals_full(L,r);return 0;
    }
    if(job->operation!=Operation::native_context_postread_globals_acquire||
        r->phase!=NativePostreadGlobalsControl::Phase::cold||r->globals_state||r->globals_reference!=LUA_NOREF||
        r->key_state||r->key_reference!=LUA_NOREF)
        return luaL_error(L,"postRead Globals constructor/local refs replay");
    //8F8547/49 store actual518 state and NOREF before the reached Lua calls.
    r->globals_state=L;r->globals_reference=LUA_NOREF;
    r->phase=NativePostreadGlobalsControl::Phase::globals_initialized;
    r->globals_checkstack=lua_checkstack(L,1); //original8F856E result ignored
    if(!r->globals_checkstack)return luaL_error(L,"postRead Globals actual stack allocation failed");
    native_postread_globals_full(L,r);
    lua_pushvalue(L,LUA_GLOBALSINDEX);r->globals_pointer=lua_topointer(L,-1);
    r->phase=NativePostreadGlobalsControl::Phase::globals_pushed;
    // Actual borrowed Globals, NOT root Host globals or old StateName ref.
    r->globals_reference=luaL_ref(L,LUA_REGISTRYINDEX);
    r->phase=NativePostreadGlobalsControl::Phase::globals_retained;
    native_postread_globals_full(L,r);
    const auto&c=p.child_contexts[r->child];
    if(r->globals_reference<0||r->globals_reference==c.environment||r->globals_reference==c.execution_thread||
        r->globals_reference==c.receiver||r->globals_pointer!=r->environment||lua_gettop(L)!=1)
        return luaL_error(L,"postRead Globals real local1C ref did not return distinct live ownership");
    //45751A->8F8490 genuine local10 NULL/NOREF constructor (no Lua calls).
    r->key_state=nullptr;r->key_reference=LUA_NOREF;
    r->phase=NativePostreadGlobalsControl::Phase::key_constructed;
    //457523 sees the actual nonNULL retained518, so the string branch is reached.
    if(!lua_checkstack(L,1))return luaL_error(L,"postRead ContextPtr key stack reserve failed");
    lua_pushstring(L,"ContextPtr"); //exact physical literalA35668
    r->key_pointer=lua_tolstring(L,-1,nullptr);r->phase=NativePostreadGlobalsControl::Phase::key_pushed;
    native_postread_globals_full(L,r);
    //8F8B31 old local10 NULL -> no old unref.8F8B4E/54 reset, then actual
    //8F8B62 thisstate=L and8F8B6F copies caller selector-1 before luaL_ref.
    r->key_state=nullptr;r->key_reference=LUA_NOREF;r->key_state=L;
    r->key_checkstack=lua_checkstack(L,1);
    if(!r->key_checkstack)return luaL_error(L,"postRead ContextPtr assignment stack allocation failed");
    lua_pushvalue(L,-1);r->key_reference=luaL_ref(L,LUA_REGISTRYINDEX);
    r->phase=NativePostreadGlobalsControl::Phase::key_retained;
    //45754C exact lua_settop(L,-2) pops original source key after its copied
    //value was consumed by luaL_ref. Keep cpcall userdata frame intact.
    lua_settop(L,-2);
    if(r->key_reference<0||r->key_reference==r->globals_reference||
        r->key_reference==c.environment||r->key_reference==c.execution_thread||r->key_reference==c.receiver||
        lua_gettop(L)!=1||!native_postread_globals_physical(L,*r,true))
        return luaL_error(L,"postRead ContextPtr name ref/stack effect did not return");
    native_postread_globals_full(L,r);
    r->phase=NativePostreadGlobalsControl::Phase::prefix_returned;return 0;
}
// Housekeeping/exception cleanup on the same still-retained coroutine. This
// proves actual unref then nil stores; it DOES NOT issue original caller-tail
// completion or a renewed Source loan after failure/retirement.
static int native_postread_globals_cleanup_coroutine(lua_State*L){
    auto*r=static_cast<NativePostreadGlobalsControl*>(lua_touserdata(L,1));
    if(!r||L!=r->thread||r->creator!=std::this_thread::get_id()||r->vm_retired)
        return luaL_error(L,"postRead Globals cleanup physical lifetime differs");
    if(!lua_checkstack(L,4))return luaL_error(L,"postRead Globals cleanup stack reserve failed");
    r->phase=NativePostreadGlobalsControl::Phase::cleanup_started;
    if(r->key_state&&r->key_reference!=LUA_NOREF){
        r->key_unref_started=true;luaL_unref(L,LUA_REGISTRYINDEX,r->key_reference);r->key_unref_returned=true;
    }
    r->key_state=nullptr;r->key_reference=LUA_NOREF;
    if(r->globals_state&&r->globals_reference!=LUA_NOREF){
        r->globals_unref_started=true;luaL_unref(L,LUA_REGISTRYINDEX,r->globals_reference);r->globals_unref_returned=true;
    }
    r->globals_state=nullptr;r->globals_reference=LUA_NOREF;return 0;
}
static void perform_native_postread_globals(lua_State*L,Job&job){
    auto&p=owner(L);auto*r=job.context_postread_globals;
    if(L!=p.state||!r||r->master!=L||!r->thread||r->thread==L)
        return (void)luaL_error(L,"postRead Globals requires existing distinct retained518 coroutine");
    native_postread_globals_full(L,r);
    if(!native_postread_globals_retained_thread_without_callbacks(p,*r))
        return (void)luaL_error(L,"postRead Globals physical coroutine keeper changed before pointer borrow");
    if(lua_status(r->thread)||lua_gettop(r->thread)!=0)
        return (void)luaL_error(L,"postRead Globals current coroutine is not idle before protected call");
    r->entry_top=lua_gettop(r->thread);
    const int status=lua_cpcall(r->thread,native_postread_globals_coroutine,&job);
    if(status){
        //Transfer actual error value without string coercion; restore actual
        //child stack before propagating into the existing master boundary.
        if(!lua_checkstack(L,1))return (void)luaL_error(L,"postRead coroutine error transfer reserve failed");
        lua_xmove(r->thread,L,1);lua_settop(r->thread,r->entry_top);
        return (void)lua_error(L);
    }
    r->returned_top=lua_gettop(r->thread);
    if(r->returned_top!=r->entry_top||r->phase!=NativePostreadGlobalsControl::Phase::prefix_returned)
        return (void)luaL_error(L,"postRead Globals protected coroutine did not return unchanged stack");
    native_postread_globals_full(L,r);
}
void native_postread_globals_release_after_abort(NativePostreadGlobalsControl&r)noexcept{
    if(r.creator!=std::this_thread::get_id()||r.creator!=ui_thread)std::terminate();
    if(r.vm_retired||!state||r.master!=state||!r.thread||r.thread==state)return;
    if(!native_postread_globals_retained_thread_without_callbacks(*this,r)){
        r.phase=NativePostreadGlobalsControl::Phase::failed;r.dropped=true;return;
    }
    if((r.key_unref_started&&!r.key_unref_returned)||(r.globals_unref_started&&!r.globals_unref_returned)){
        // A Lua error may follow partial free-list mutation. Do not double
        // unref an uncertain operation; whole actual VM close owns retirement.
        r.phase=NativePostreadGlobalsControl::Phase::failed;r.dropped=true;return;
    }
    const int top=lua_gettop(r.thread);
    //Permanent ChildContext.execution_thread still retains this coroutine.
    const bool previous_calling=calling;calling=true;
    const int status=lua_cpcall(r.thread,native_postread_globals_cleanup_coroutine,&r);
    lua_settop(r.thread,top);
    calling=previous_calling;
    if(status)r.phase=NativePostreadGlobalsControl::Phase::failed;
    else r.phase=NativePostreadGlobalsControl::Phase::retired;
    r.dropped=true; //cleanup never produces normal caller-retirement authority
}
void native_postread_globals_close()noexcept{
    for(auto&r:postread_globals_controls){
        if(!r)continue;
        if(!r->vm_retired&&(r->key_reference!=LUA_NOREF||r->globals_reference!=LUA_NOREF))
            native_postread_globals_release_after_abort(*r);
        r->vm_retired=true;r->dropped=true;
    }
}
