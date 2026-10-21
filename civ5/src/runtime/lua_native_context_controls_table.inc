// Inside actual Host::Impl, after genuine prepared-state/include definitions.
// A constructor borrows the actually returned ordered registrar frames;
// later queries consume the existing canonical completed Setup return.
struct NativeContextPreGameFrame;struct NativeContextGameCoreFrame;
struct NativeContextControlsFrame final {
    Impl& host;const LuaScriptHost& facade;
    const BnwWorldViewSourceOperationLoan& operation;
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation> prepared;
    const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan> name;
    const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn> state_name;
    const std::shared_ptr<const ScriptUiChildSource> source;
    const std::weak_ptr<const void> core;
    const std::shared_ptr<const ui::detail::NotificationContextLuaControlsTableReturn> table_return;
    const std::optional<ui::detail::NotificationContextRootDefaultFieldsReturn::Guard> defaults;
    const NativeContextPreGameFrame*const ordered_registrars;
    const NativeContextGameCoreFrame*const gamecore;
    const std::thread::id creator=std::this_thread::get_id();
    const std::size_t child;const std::uint64_t serial,epoch,writes,uncertainty;
    const int environment,thread;const void*const actual_environment;lua_State*const actual_thread;
    NativeContextControlsFrame(Impl& h,const LuaScriptHost& f,const BnwWorldViewSourceOperationLoan& op,
      std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation> p,
      std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan> n,
      std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn> sn,
      std::shared_ptr<const ScriptUiChildSource> src,std::weak_ptr<const void> c,std::size_t slot,
      int env,int thr,const void* envptr,lua_State* thrptr,
      std::shared_ptr<const ui::detail::NotificationContextLuaControlsTableReturn> table={},
      std::optional<ui::detail::NotificationContextRootDefaultFieldsReturn::Guard> d={},
      const NativeContextPreGameFrame*registrars=nullptr,const NativeContextGameCoreFrame*core_frame=nullptr)
      :host(h),facade(f),operation(op),prepared(std::move(p)),name(std::move(n)),state_name(std::move(sn)),
       source(std::move(src)),core(std::move(c)),table_return(std::move(table)),defaults(std::move(d)),
       ordered_registrars(registrars),gamecore(core_frame),
       child(slot),serial(op.operation_serial()),epoch(h.failure_epoch),writes(h.failed.writes_),
       uncertainty(h.execution_uncertainty),environment(env),thread(thr),actual_environment(envptr),actual_thread(thrptr){
        if(host.controls_frame)throw ScriptError("Controls actual load frame is already entered");
        host.controls_frame=this;
    }
    ~NativeContextControlsFrame(){if(host.controls_frame==this)host.controls_frame=nullptr;}
    void first()const {
        host.worldview_require_callback_origin();
        if(host.controls_frame!=this||creator!=std::this_thread::get_id()||!host.worldview_native_host||
           host.worldview_native_host->loan!=&operation||host.worldview_native_host->child!=child||
           child>=host.child_contexts.size()||serial!=operation.operation_serial()||epoch!=host.failure_epoch||
           writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty||core.expired()||
           !prepared||!name||!state_name||!source||!actual_environment||!actual_thread)
            throw ScriptError("Controls SAME load/preparation/Source frame changed");
        const auto& current=host.child_contexts[child];
        if(current.retired||!current.state_prepared||current.environment!=environment||current.execution_thread!=thread||
           current.source.get()!=source.get()||current.source.owner_before(source)||source.owner_before(current.source))
            throw ScriptError("Controls actual child518/source/ref changed");
        const auto actual_name=current.setup_state_name_return.lock();
        if(actual_name.get()!=state_name.get()||actual_name.owner_before(state_name)||state_name.owner_before(actual_name))
            throw ScriptError("Controls actual StateName return changed");
        name->require_current_without_callbacks(operation,prepared);
        if(table_return){
            const auto actual=current.controls_table_return.lock();
            if(actual.get()!=table_return.get()||actual.owner_before(table_return)||table_return.owner_before(actual))
                throw ScriptError("Controls actual table return publication changed");
        }
        if(ordered_registrars){
            if(host.pregame_frame!=ordered_registrars)throw ScriptError("Controls actual ordered registrar frame retired");
            ordered_registrars->first();
            if(!ordered_registrars->returned||!ordered_registrars->callback_returned||ordered_registrars->protected_status||
               ordered_registrars->fields_created!=104||!ordered_registrars->actual_job||ordered_registrars->table_ref<0||
               ordered_registrars->child!=child||ordered_registrars->actual_environment!=actual_environment||
               ordered_registrars->selected_thread!=actual_thread)
                throw ScriptError("Controls real PreGame/order/selected518 normal return absent");
            if(gamecore){
                if(host.gamecore_frame!=gamecore)throw ScriptError("Controls actual GameCore frame retired");
                gamecore->first();
                if(!gamecore->returned||!gamecore->callback_returned||gamecore->protected_status||
                   gamecore->table_ref<0||!gamecore->table||!gamecore->actual_job||
                   gamecore->predecessor!=host.game_events_frame||
                   gamecore->predecessor->predecessor!=ordered_registrars)
                    throw ScriptError("Controls reached GameCore original normal return absent");
            }else if(host.game_events_frame||host.gamecore_frame)
                throw ScriptError("Controls actual taken APP branch requires its live GameCore return");
        }else if(gamecore)throw ScriptError("Controls GameCore lacks actual ordered registrar predecessor");
        if(defaults)defaults->require_current_without_callbacks(operation,prepared);
    }
    void full()const {
        first();operation.require_current();first();operation.require_same_vm(facade);first();
        operation.require_same_source(*source);first();name->require_current_owned(operation,prepared);first();
        if(!host.profile_include_collection)throw ScriptError("Controls genuine include collector missing");
        host.profile_include_collection->require_current_owned(operation);first();
        host.profile_include_collection->require_current_without_callbacks(operation);first();
    }
};
static void native_controls_full(lua_State* L){
    auto& p=owner(L);const auto* frame=p.controls_frame;p.source_exception={};p.source_error=nullptr;
    try{if(!frame)throw ScriptError("Controls actual protected load frame absent");
        frame->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Controls actual Source/load FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    native_context_include_delivery_full(L,frame->child);
    p.source_exception={};p.source_error=nullptr;
    try{if(p.controls_frame!=frame)throw ScriptError("Controls actual protected frame replaced during callback");frame->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Controls load FIRST after include/source callbacks refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}
static void require_native_precontrols_registration_return(lua_State* L){
    auto& p=owner(L);const auto*const frame=p.controls_frame;
    if(!frame)return (void)luaL_error(L,"Controls actual lexical constructor/query frame absent");
    if(frame->ordered_registrars){
        native_context_pregame_registration_full(L,*frame->ordered_registrars->actual_job);
        if(frame->gamecore)native_context_gamecore_registration_full(L,*frame->gamecore->actual_job);
        p.source_exception={};p.source_error=nullptr;
        try{if(p.controls_frame!=frame)throw ScriptError("Controls frame replaced after ordered callbacks");frame->first();}
        catch(...){p.source_exception=std::current_exception();p.source_error="Controls real ordered registrar POST refused";}
        if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
        return;
    }
    p.source_exception={};p.source_error=nullptr;
    try{
        frame->first();const auto setup=p.child_contexts[frame->child].load_setup_completed_return.lock();
        if(!frame->table_return||!setup)throw ScriptError("Controls cold data/profile is not genuine completed Setup");
        frame->facade.require_context_lua_load_setup_current_without_callbacks(frame->operation,frame->prepared,*setup);
        frame->first();
    }catch(...){p.source_exception=std::current_exception();p.source_error="Controls actual canonical ordered Setup return absent";}
    if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
}
static bool controls_value_physical_without_callbacks(Impl& p,std::size_t slot,
    int envref,int threadref,const void* expected_env,lua_State* expected_thread,
    const void* expected_table,int expected_ref,int named_ref=LUA_NOREF){
    // A protected query has already established this stack high-water mark.
    // All values/keys below remain registry/table retained. No allocation,
    // Source callback, error coercion or luaL_error is permitted in this cut.
    if(slot>=p.child_contexts.size()||p.child_contexts[slot].retired||!p.state)return false;
    const auto& child=p.child_contexts[slot];
    if(child.environment!=envref||child.execution_thread!=threadref||
       child.controls_table_pointer!=expected_table||child.controls_table_ref!=expected_ref||
       expected_ref==LUA_NOREF||expected_ref==LUA_REFNIL||!child.profile_include_child_control)return false;
    lua_State* L=p.state;const int top=lua_gettop(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,threadref);
    lua_State* thread=lua_tothread(L,-1);
    bool good=thread&&thread==expected_thread&&thread!=L&&!lua_status(thread)&&!lua_gettop(thread);
    lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,envref);const int environment=lua_gettop(L);
    good=good&&lua_istable(L,environment)&&lua_topointer(L,environment)==expected_env&&lua_rawequal(L,-1,-2);
    if(!good){lua_settop(L,top);return false;}
    if(lua_getmetatable(L,environment)){lua_settop(L,top);return false;}
    lua_pushliteral(L,"print");lua_rawget(L,environment);lua_rawgeti(L,LUA_REGISTRYINDEX,p.profile_print_function_ref);
    good=lua_iscfunction(L,-1)&&lua_tocfunction(L,-1)==native_master_print&&
         lua_topointer(L,-1)==p.profile_print_function&&lua_rawequal(L,-1,-2);lua_pop(L,2);
    const auto* include=child.profile_include_child_control.get();
    lua_pushliteral(L,"include");lua_rawget(L,environment);lua_rawgeti(L,LUA_REGISTRYINDEX,include->function_ref);
    good=good&&lua_isfunction(L,-1)&&!lua_iscfunction(L,-1)&&lua_topointer(L,-1)==include->function&&
         lua_rawequal(L,-1,-2)&&static_cast<const Closure*>(lua_topointer(L,-1))->l.p==include->proto&&
         native_include_list_upvalue_without_callbacks(L,-1,include->list->table)==include->list_upvalue;lua_pop(L,2);
    lua_pushliteral(L,"traceback");lua_rawget(L,environment);lua_rawgeti(L,LUA_REGISTRYINDEX,include->traceback_ref);
    good=good&&lua_isfunction(L,-1)&&lua_topointer(L,-1)==include->traceback&&lua_rawequal(L,-1,-2);lua_pop(L,2);
    lua_pushliteral(L,"Controls");lua_rawget(L,environment);lua_rawgeti(L,LUA_REGISTRYINDEX,expected_ref);
    good=good&&lua_istable(L,-1)&&lua_topointer(L,-1)==expected_table&&lua_rawequal(L,-1,-2);lua_pop(L,2);
    if(named_ref!=LUA_NOREF){
        lua_rawgeti(L,LUA_REGISTRYINDEX,named_ref);
        good=good&&lua_istable(L,-1)&&lua_topointer(L,-1)==expected_table;lua_pop(L,1);
    }
    lua_settop(L,top);return good;
}
static void native_controls_physical_query(lua_State* L,Job& job){
    auto& p=owner(L);native_controls_full(L);perform_native_context_state_query(L,job);native_controls_full(L);
    const auto* frame=p.controls_frame;const int environment=lua_gettop(L);
    if(job.context_state_thread!=frame->actual_thread||job.context_state_environment!=frame->actual_environment)
        return (void)luaL_error(L,"Controls query changed SAME physical518/environment");
    native_context_print_delivery_check(L,frame->child,environment);native_controls_full(L);
    // Final FULL occurs before physical checks. No observer follows these reads.
    native_include_child_physical_query(L,environment,frame->child);
    lua_pushliteral(L,"print");lua_rawget(L,environment);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.profile_print_function_ref);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_master_print||
       lua_topointer(L,-1)!=p.profile_print_function||!lua_rawequal(L,-1,-2))
        return (void)luaL_error(L,"Controls final print identity changed");
    lua_pop(L,2);
    lua_pushliteral(L,"Controls");lua_rawget(L,environment);
    const auto& child=p.child_contexts[frame->child];
    lua_rawgeti(L,LUA_REGISTRYINDEX,child.controls_table_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=child.controls_table_pointer||!lua_rawequal(L,-1,-2))
        return (void)luaL_error(L,"Controls actual created table/field/ref rebound");
    job.context_controls_table=lua_topointer(L,-1);job.context_controls_reference=child.controls_table_ref;
    lua_pop(L,2);
}
static int native_controls_statement_518_callback(lua_State* L){
    auto& p=owner(L);auto*const job=static_cast<Job*>(lua_touserdata(L,1));
    native_controls_full(L);const auto*const frame=p.controls_frame;
    if(!job||job->owner!=&p||job->operation!=Operation::native_context_controls_statement||
       job->worldview_child!=frame->child||L!=frame->actual_thread||lua_gettop(L)!=1||
       !ttistable(gt(L))||hvalue(gt(L))!=frame->actual_environment||!lua_checkstack(L,16))
        return luaL_error(L,"Controls457477 actual selected518 execution/fenv/Job changed");
    // Original8F7EF0 takes its outer recursive lock around compilation, then
    //8F7AE0 takes the nested lock. Both use the already genuine shared owner;
    // its outside-Lua abort cleanup retains every held lock through longjmp.
    auto*const boundary=p.native_master_protected_boundary;
    native_master_protected_boundary_current(L,boundary);
    if(boundary->held_locks==UINT64_MAX)return luaL_error(L,"Controls native recursive lock history exhausted");
    ::EnterCriticalSection(&boundary->critical->actual);++boundary->held_locks;
    native_master_protected_boundary_current(L,boundary);
    // Actual8F7EF0 uses this same518's globals when luaL_loadstring creates
    // the chunk. The already owned8F7AE0 supplies quicktraceback/CS/pcall.
    const int status=luaL_loadstring(L,"Controls = {};");native_controls_full(L);
    if(status)return lua_error(L);
    lua_getfenv(L,-1);const bool same=lua_topointer(L,-1)==frame->actual_environment;lua_pop(L,1);
    if(!same)return luaL_error(L,"Controls fixed chunk is not in the actual selected518 fenv");
    const auto*const returned=native_master_protected_call_8f7ae0_owned(L,0,0);
    native_master_protected_call_return_current_owned(L,returned);native_controls_full(L);
    native_master_protected_boundary_current(L,boundary);
    if(!boundary->held_locks)return luaL_error(L,"Controls original outer recursive lock retired");
    ::LeaveCriticalSection(&boundary->critical->actual);--boundary->held_locks;
    native_master_protected_boundary_current(L,boundary);
    if(!returned||returned->actual_status||lua_gettop(L)!=1)
        return luaL_error(L,"Controls original locked fixed-statement pcall did not return normally");
    lua_pushliteral(L,"Controls");lua_rawget(L,LUA_GLOBALSINDEX);native_controls_full(L);
    if(!lua_istable(L,-1)||lua_getmetatable(L,-1))return luaL_error(L,"Controls actual fixed statement created no plain table");
    lua_pushnil(L);if(lua_next(L,-2))return luaL_error(L,"Controls new table changed during actual constructor callbacks");
    p.child_contexts[frame->child].controls_table_pointer=lua_topointer(L,-1);
    p.child_contexts[frame->child].controls_table_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_controls_full(L);
    return 0;
}
static void perform_native_context_controls_statement(lua_State* L,Job& job){
    native_controls_full(L);require_native_precontrols_registration_return(L);
    auto& p=owner(L);const auto* frame=p.controls_frame;
    if(frame->defaults||frame->table_return||frame->child!=job.worldview_child||
       p.child_contexts[frame->child].controls_table_ref!=LUA_NOREF||
       !p.child_contexts[frame->child].controls_table_return.expired())
        return (void)luaL_error(L,"Controls statement replay or wrong lexical constructor role");
    perform_native_context_state_query(L,job);native_controls_full(L);
    const int environment=lua_gettop(L);
    if(job.context_state_thread!=frame->actual_thread||job.context_state_environment!=frame->actual_environment)
        return (void)luaL_error(L,"Controls statement lacks actual prepared518 environment");
    lua_pushliteral(L,"Controls");native_controls_full(L);lua_rawget(L,environment);native_controls_full(L);
    if(!lua_isnil(L,-1))return (void)luaL_error(L,"Controls constructor existing field unsupported");
    lua_pop(L,1);
    // Original invokes8F7EF0 on selected518. Its allocating child work lives
    // in a protected child cpcall; the real outer C++ frame/CS owner survives.
    const int child_status=lua_cpcall(frame->actual_thread,native_controls_statement_518_callback,&job);
    if(child_status){lua_xmove(frame->actual_thread,L,1);return (void)lua_error(L);}
    native_controls_full(L);
    if(lua_gettop(frame->actual_thread)||lua_status(frame->actual_thread))
        return (void)luaL_error(L,"Controls actual selected518 did not restore its idle stack");
    native_controls_physical_query(L,job);
}
static void perform_native_context_controls_named_lookup(lua_State* L,Job& job){
    native_controls_full(L);require_native_precontrols_registration_return(L);
    auto& p=owner(L);const auto* frame=p.controls_frame;
    if(!frame->table_return||!frame->defaults||frame->child!=job.worldview_child)
        return (void)luaL_error(L,"Controls8F85A0 requires actual table issuer and completed Root5D return");
    native_controls_physical_query(L,job);native_controls_full(L);
    perform_native_context_state_query(L,job);native_controls_full(L);const int environment=lua_gettop(L);
    lua_pushliteral(L,"Controls");native_controls_full(L);lua_rawget(L,environment);native_controls_full(L);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.child_contexts[frame->child].controls_table_pointer)
        return (void)luaL_error(L,"Controls8F85A0 existing genuine field absent/rebound");
    // The actual named reference retains the existing value. It creates no table.
    job.context_controls_named_reference=luaL_ref(L,LUA_REGISTRYINDEX);native_controls_full(L);
    native_controls_physical_query(L,job);
    lua_rawgeti(L,LUA_REGISTRYINDEX,job.context_controls_named_reference);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=job.context_controls_table)
        return (void)luaL_error(L,"Controls8F85A0 retained named reference changed");
    lua_pop(L,1);
}
