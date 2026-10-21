// Inside the SAME Host::Impl, after the real Source6/7 Globals/CData owners.
// All reference slots and C++ borrowers live outside the allocating cpcall.
struct NativePostreadLoadTailControl final {
    struct Reference final {
        lua_State* state=nullptr;
        int reference=LUA_NOREF,checkstack=0;
        bool unref_started=false,unref_returned=false;
    };
    enum class Stage {captured,lookup,copy24,temp_retired,copy38,copy24_retired,
        key_retiring,key_retired,prefix_returned,lookup_registered,ui_registered,
        tt_registered,registrations_returned,failed,retired};
    const std::thread::id creator=std::this_thread::get_id();
    const std::shared_ptr<const ui::detail::NotificationContextPostreadRootCDataReturn> root_return;
    const std::shared_ptr<NativePostreadRootCDataControl> cdata;
    const std::shared_ptr<NativePostreadGlobalsControl> globals;
    const std::shared_ptr<const ui::detail::NotificationContextUiManagerValueLoan> ui_manager_value;
    const std::shared_ptr<const ui::detail::NotificationContextTooltipManagerOperand> tt_manager_operand;
    Stage stage=Stage::captured;
    Reference lookup_temporary2c,copied24,contextptr38;
    Reference ui_manager_key10,tt_manager_key10;
    // Original4575EC BYVALUE stack argument and 4EE63B inner copy. Actual
    // 4EE620 destroys inner copy4EE66B and argument4EE675 before RET0xC.
    int lookup_argument_reference=LUA_NOREF,lookup_copy_reference=LUA_NOREF;
    bool lookup_argument_unref_started=false,lookup_argument_unref_returned=false;
    bool lookup_copy_unref_started=false,lookup_copy_unref_returned=false;
    NativePostreadLookupControlReturn* lookup_return=nullptr;
    int entry_top=0,returned_top=0,protected_status=-1;
    bool closed=false;
    explicit NativePostreadLoadTailControl(
        std::shared_ptr<const ui::detail::NotificationContextPostreadRootCDataReturn> view,
        std::shared_ptr<NativePostreadRootCDataControl> actual,
        std::shared_ptr<const ui::detail::NotificationContextUiManagerValueLoan> ui_value,
        std::shared_ptr<const ui::detail::NotificationContextTooltipManagerOperand> tt_operand)
        :root_return(std::move(view)),cdata(std::move(actual)),globals(cdata->parent),ui_manager_value(std::move(ui_value)),
         tt_manager_operand(std::move(tt_operand)){}
};
struct NativePostreadLoadTailFrame final {
    Impl& host;const LuaScriptHost& facade;const BnwWorldViewSourceOperationLoan& operation;
    NativePostreadLoadTailControl& control;
    const std::shared_ptr<ui::detail::NotificationLayoutContextOrdinaryState> held_root;
    const std::shared_ptr<const ui::detail::NotificationContextUiManagerValueLoan> held_ui_manager;
    const std::shared_ptr<const ui::detail::NotificationContextTooltipManagerOperand> held_tt_manager;
    const std::shared_ptr<const ui::detail::NotificationContextDirectLuaTailInput> direct_input;
    Job* actual_job=nullptr;
    NativePostreadLoadTailFrame(Impl&p,const LuaScriptHost&f,const BnwWorldViewSourceOperationLoan&op,
        NativePostreadLoadTailControl&r,std::shared_ptr<ui::detail::NotificationLayoutContextOrdinaryState> root,
        std::shared_ptr<const ui::detail::NotificationContextDirectLuaTailInput> input={})
        :host(p),facade(f),operation(op),control(r),held_root(std::move(root)),held_ui_manager(r.ui_manager_value),
         held_tt_manager(r.tt_manager_operand),direct_input(std::move(input)){
        if(host.postread_load_tail_frame)throw ScriptError("postRead load tail frame reentered");
        host.postread_load_tail_frame=this;
    }
    NativePostreadLoadTailFrame(const NativePostreadLoadTailFrame&)=delete;
    ~NativePostreadLoadTailFrame(){
        if(control.creator!=std::this_thread::get_id())std::terminate();
        if(host.postread_load_tail_frame==this)host.postread_load_tail_frame=nullptr;
        else host.record_failure();
    }
    void first()const {
        if(direct_input){
            // Genuine post5761 input authenticates immutable constructor/read
            // custody and ONLY real writer journals. Loading/loaded + owned
            // loader sentinel are legal here; prefix predicates stay intact.
            direct_input->require_current_without_callbacks(operation);
            if(control.globals->creator!=std::this_thread::get_id()||control.globals->creator!=host.ui_thread||
                control.globals->vm_retired||control.globals->dropped||!host.state||
                control.globals->master!=host.state||!host.end_turn_lifetime||
                !host.end_turn_lifetime->alive||host.end_turn_lifetime->failed||
                host.end_turn_lifetime->host!=&facade||host.end_turn_lifetime->vm!=host.state||
                control.globals->epoch!=host.failure_epoch||control.globals->writes!=host.failed.writes_||
                host.failed.exhausted_||control.globals->uncertainty!=host.execution_uncertainty||
                !host.worldview_native_host||host.worldview_native_host->loan!=&operation||
                host.worldview_native_host->child!=control.globals->child||
                control.globals->child>=host.child_contexts.size())
                throw ScriptError("direct postLua tail actual VM/failure/current child owner retired FIRST");
            host.worldview_native_host->pure();
            const auto&child=host.child_contexts[control.globals->child];
            if(child.retired||!child.state_prepared||child.source!=control.globals->source||
                child.environment!=control.globals->environment_reference||
                child.execution_thread!=control.globals->thread_reference||
                host.worldview_native_host->child_source.lock()!=control.globals->source||
                !native_postread_globals_retained_thread_without_callbacks(host,*control.globals))
                throw ScriptError("direct postLua tail SAME actual source/environment/518 registry owner changed");
        }else{
            host.native_postread_globals_first(*control.globals);
            facade.require_context_postread_globals_input_without_callbacks(operation,control.globals->input);
        }
        const auto canonical=control.cdata->canonical.lock();
        if(host.postread_load_tail_frame!=this||control.closed||control.creator!=std::this_thread::get_id()||
            !held_root||!held_ui_manager||held_ui_manager!=control.ui_manager_value||
            !held_tt_manager||held_tt_manager!=control.tt_manager_operand||
            control.cdata->root.lock()!=held_root||held_root.get()!=control.cdata->cdata||
            canonical!=control.root_return||canonical.owner_before(control.root_return)||
            control.root_return.owner_before(canonical)||control.cdata->closed||
            control.cdata->stage!=NativePostreadRootCDataControl::Stage::returned||
            control.cdata->parent!=control.globals||!control.cdata->type||
            control.cdata->type!=host.native_context_cdata_creator||control.cdata->type->host!=&host||
            !control.cdata->type->alive.load()||control.cdata->type->creator!=control.creator||
            std::strcmp(control.cdata->type->type_name,"LuaContext")||
            control.cdata->type->push_metatype!=&Impl::native_child_context_push_existing_metatable||
            (control.globals->phase!=NativePostreadGlobalsControl::Phase::prefix_returned&&
             !(direct_input&&control.globals->phase==NativePostreadGlobalsControl::Phase::retired&&
               control.contextptr38.state==nullptr&&control.contextptr38.reference==LUA_NOREF&&
               control.contextptr38.unref_started&&control.contextptr38.unref_returned&&
               control.globals->globals_state==nullptr&&control.globals->globals_reference==LUA_NOREF&&
               control.globals->globals_unref_started&&control.globals->globals_unref_returned)))
            throw ScriptError("postRead tail SAME returned current Context/Globals owner changed");
        if(actual_job&&(actual_job->owner!=&host||actual_job->context_postread_load_tail!=&control||
            actual_job->worldview_child!=control.globals->child))
            throw ScriptError("postRead tail actual active protected Job operand replaced");
    }
    void full()const {
        first();
        if(direct_input){
            direct_input->require_current_owned(operation);
            direct_input->require_root_cdata_return_owned(operation,*control.root_return);
        }else facade.require_context_postread_globals_input_owned(operation,control.globals->input);
        first();
    }
};
static void native_postread_load_tail_full_owned(lua_State*L){
    auto&p=owner(L);p.source_exception={};p.source_error=nullptr;
    try{
        if(!p.postread_load_tail_frame)throw ScriptError("postRead tail actual protected owner missing");
        p.postread_load_tail_frame->full();
    }catch(...){p.source_exception=std::current_exception();p.source_error="postRead tail SAME current Source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}
static NativePostreadLoadTailControl& native_postread_load_tail_current_owned(lua_State*L){
    native_postread_load_tail_full_owned(L);
    auto&p=owner(L);auto&r=p.postread_load_tail_frame->control;
    if(L!=r.globals->thread||!p.worldview_native_host||p.worldview_native_host->child!=r.globals->child)
        luaL_error(L,"postRead tail active operand is not the real retained518 coroutine");
    return r;
}
static const std::shared_ptr<const ui::detail::NotificationContextTooltipManagerOperand>&
native_postread_load_tail_tt_manager_operand_owned(lua_State*L){
    auto&r=native_postread_load_tail_current_owned(L);auto&p=owner(L);
    if(!p.calling||!p.postread_load_tail_frame->actual_job||!r.tt_manager_operand)
        luaL_error(L,"postRead TT value requires SAME actual active tail Job");
    return r.tt_manager_operand;
}
static void native_postread_load_tail_lookup_publication_full_owned(lua_State*L){
    auto&r=native_postread_load_tail_current_owned(L);auto&p=owner(L);
    if(!p.calling||!p.postread_load_tail_frame->actual_job||!r.lookup_return||
        r.lookup_argument_reference!=LUA_NOREF||r.lookup_copy_reference!=LUA_NOREF||
        !r.lookup_argument_unref_started||!r.lookup_argument_unref_returned||
        !r.lookup_copy_unref_started||!r.lookup_copy_unref_returned)
        return (void)luaL_error(L,"postRead successor requires real LookUpControl normal publication/temporary DTORs");
    native_postread_lookup_control_return_full_owned(L,*r.lookup_return);
    native_postread_load_tail_full_owned(L);
}
static void native_postread_load_tail_copy_8f84a0_owned(lua_State*L,
    NativePostreadLoadTailControl::Reference& destination,const NativePostreadLoadTailControl::Reference& source){
    native_postread_load_tail_full_owned(L);
    if(destination.state||destination.reference!=LUA_NOREF)
        return (void)luaL_error(L,"postRead8F84A0 destination is not its fresh original local");
    destination.state=source.state;destination.reference=LUA_NOREF;
    if(source.state&&source.reference!=LUA_NOREF){
        if(source.state!=L)return (void)luaL_error(L,"postRead8F84A0 source belongs to another lua_State");
        destination.checkstack=lua_checkstack(L,1);native_postread_load_tail_full_owned(L);
        if(!destination.checkstack)return (void)luaL_error(L,"postRead8F84A0 physical stack allocation failed");
        lua_rawgeti(L,LUA_REGISTRYINDEX,source.reference);native_postread_load_tail_full_owned(L);
        destination.reference=luaL_ref(L,LUA_REGISTRYINDEX);native_postread_load_tail_full_owned(L);
    }
}
static void native_postread_load_tail_unref_8f8820_owned(lua_State*L,
    NativePostreadLoadTailControl::Reference& value){
    native_postread_load_tail_full_owned(L);
    if(value.state&&value.reference!=LUA_NOREF){
        if(value.state!=L||value.unref_started)
            return (void)luaL_error(L,"postRead8F8820 foreign/uncertain original local destruction");
        value.unref_started=true;
        luaL_unref(L,LUA_REGISTRYINDEX,value.reference);value.unref_returned=true;
        native_postread_load_tail_full_owned(L);
    }
    value.state=nullptr;value.reference=LUA_NOREF;
}
static void native_postread_load_tail_named_key_8f8b10_owned(lua_State*L,
    NativePostreadLoadTailControl::Reference&key,const char*name){
    native_postread_load_tail_full_owned(L);
    if(key.state||key.reference!=LUA_NOREF||key.unref_started)
        return (void)luaL_error(L,"postRead original named local10 occurrence replay");
    (void)lua_checkstack(L,1);native_postread_load_tail_full_owned(L);
    lua_pushstring(L,name);native_postread_load_tail_full_owned(L);
    key.state=L;key.checkstack=lua_checkstack(L,1);native_postread_load_tail_full_owned(L);
    if(!key.checkstack)return (void)luaL_error(L,"postRead named key physical stack allocation failed");
    lua_pushvalue(L,-1);key.reference=luaL_ref(L,LUA_REGISTRYINDEX);native_postread_load_tail_full_owned(L);
    lua_settop(L,-2);native_postread_load_tail_full_owned(L);
}
static bool native_postread_load_tail_contextptr_physical_owned(lua_State*L,const NativePostreadLoadTailControl&r){
    if(!r.globals||L!=r.globals->thread||L->ci->top-L->top<12||
        !native_postread_globals_physical(L,*r.globals,false)||
        !native_postread_root_cdata_physical(L,*r.cdata))return false;
    const int top=lua_gettop(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.globals->globals_reference);
    bool same=r.globals->globals_state==L&&r.globals->globals_reference>=0&&
        lua_istable(L,-1)&&lua_rawequal(L,-1,LUA_GLOBALSINDEX)&&
        lua_topointer(L,-1)==r.globals->globals_pointer;lua_pop(L,1);
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.contextptr38.reference);
    same=same&&r.contextptr38.state==L&&r.contextptr38.reference>=0&&lua_istable(L,-1)&&
        lua_topointer(L,-1)==r.cdata->table;
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.cdata->table_ref);same=same&&lua_rawequal(L,-1,-2);lua_pop(L,2);
    lua_settop(L,top);
    return same&&r.lookup_temporary2c.state==nullptr&&r.lookup_temporary2c.reference==LUA_NOREF&&
        r.lookup_temporary2c.unref_started&&r.lookup_temporary2c.unref_returned&&
        r.copied24.state==nullptr&&r.copied24.reference==LUA_NOREF&&
        r.copied24.unref_started&&r.copied24.unref_returned&&
        r.globals->key_state==nullptr&&r.globals->key_reference==LUA_NOREF&&
        r.globals->key_unref_started&&r.globals->key_unref_returned;
}
static int native_postread_contextptr_coroutine_body_owned(lua_State*L){
    auto*job=static_cast<Job*>(lua_touserdata(L,1));auto&p=owner(L);
    auto*r=job?job->context_postread_load_tail:nullptr;
    if(!job||job->owner!=&p||!r||L!=r->globals->thread||lua_gettop(L)!=1||
        !p.postread_load_tail_frame||&p.postread_load_tail_frame->control!=r||
        p.postread_load_tail_frame->actual_job!=job)
        return luaL_error(L,"postRead ContextPtr actual cpcall/frame/Job differs");
    if(!lua_checkstack(L,20))return luaL_error(L,"postRead ContextPtr owner query stack reserve failed");
    native_postread_load_tail_full_owned(L);
    if(job->operation==Operation::native_context_postread_contextptr_query){
        if((r->stage!=NativePostreadLoadTailControl::Stage::prefix_returned&&
            r->stage!=NativePostreadLoadTailControl::Stage::registrations_returned)||
            !native_postread_load_tail_contextptr_physical_owned(L,*r))
            return luaL_error(L,"postRead ContextPtr actual copied38/key10-DTOR return changed");
        if(r->stage==NativePostreadLoadTailControl::Stage::registrations_returned)
            native_postread_load_tail_lookup_publication_full_owned(L);
        native_postread_load_tail_full_owned(L);return 0;
    }
    if(job->operation!=Operation::native_context_postread_contextptr_acquire||
        r->stage!=NativePostreadLoadTailControl::Stage::captured||
        !native_postread_globals_physical(L,*r->globals,true)||
        !native_postread_root_cdata_physical(L,*r->cdata))
        return luaL_error(L,"postRead ContextPtr8F8C60 requires actual returned live Globals/key/CData refs");
    //8F8C6D/73 initializes the output NULL/NOREF. Existing genuine Table/Key
    //owners prove the reached nonNULL SAME-state/table/non-nil-key branches.
    r->lookup_temporary2c.state=nullptr;r->lookup_temporary2c.reference=LUA_NOREF;
    if(r->globals->globals_state!=r->globals->key_state)
        return luaL_error(L,"postRead8F8C60 actual key belongs to a different lua_State");
    (void)lua_checkstack(L,1);native_postread_load_tail_full_owned(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,r->globals->globals_reference);native_postread_load_tail_full_owned(L);
    if(lua_type(L,-1)!=LUA_TTABLE)return luaL_error(L,"postRead8F88F0 genuine Globals table retired");
    (void)lua_checkstack(L,1);native_postread_load_tail_full_owned(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,r->globals->key_reference);native_postread_load_tail_full_owned(L);
    if(lua_type(L,-1)!=LUA_TSTRING||lua_tolstring(L,-1,nullptr)!=r->globals->key_pointer)
        return luaL_error(L,"postRead8F86D0 genuine ContextPtr key retired");
    lua_rawget(L,-2);native_postread_load_tail_full_owned(L); //RAW; __index must not run.
    if(lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r->cdata->table)
        return luaL_error(L,"postRead8F8C60 did not read the SAME physically assigned root Context table");
    r->lookup_temporary2c.state=L;
    r->lookup_temporary2c.checkstack=lua_checkstack(L,1);native_postread_load_tail_full_owned(L);
    if(!r->lookup_temporary2c.checkstack)return luaL_error(L,"postRead8F8B10 physical stack allocation failed");
    lua_pushvalue(L,-1);r->lookup_temporary2c.reference=luaL_ref(L,LUA_REGISTRYINDEX);
    native_postread_load_tail_full_owned(L);
    lua_settop(L,-2);native_postread_load_tail_full_owned(L); //lookup result
    lua_settop(L,-2);native_postread_load_tail_full_owned(L); //Globals table
    r->stage=NativePostreadLoadTailControl::Stage::lookup;
    native_postread_load_tail_copy_8f84a0_owned(L,r->copied24,r->lookup_temporary2c);
    r->stage=NativePostreadLoadTailControl::Stage::copy24;
    native_postread_load_tail_unref_8f8820_owned(L,r->lookup_temporary2c);
    r->stage=NativePostreadLoadTailControl::Stage::temp_retired;
    native_postread_load_tail_copy_8f84a0_owned(L,r->contextptr38,r->copied24);
    r->stage=NativePostreadLoadTailControl::Stage::copy38;
    native_postread_load_tail_unref_8f8820_owned(L,r->copied24);
    r->stage=NativePostreadLoadTailControl::Stage::copy24_retired;
    //Original4575A4 destroys ContextPtr-name local10, while Globals1C and
    //ContextPtr38 remain live. Reusable integer ref values are not identities.
    r->stage=NativePostreadLoadTailControl::Stage::key_retiring;
    r->globals->key_unref_started=true;
    luaL_unref(L,LUA_REGISTRYINDEX,r->globals->key_reference);r->globals->key_unref_returned=true;
    native_postread_load_tail_full_owned(L);
    r->globals->key_state=nullptr;r->globals->key_reference=LUA_NOREF;
    r->stage=NativePostreadLoadTailControl::Stage::key_retired;
    if(lua_gettop(L)!=1||!native_postread_load_tail_contextptr_physical_owned(L,*r))
        return luaL_error(L,"postRead copied38/ref-copy/temp/local24/key10-DTOR sequence did not return");
    native_postread_load_tail_full_owned(L);
    r->stage=NativePostreadLoadTailControl::Stage::prefix_returned;return 0;
}
static int native_postread_contextptr_coroutine_owned(lua_State*L){
    auto&p=owner(L);p.source_exception={};p.source_error=nullptr;
    try{return native_postread_contextptr_coroutine_body_owned(L);}
    catch(...){p.source_exception=std::current_exception();p.source_error="postRead ContextPtr actual body exception";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}return 0;
}
static void perform_native_postread_contextptr_owned(lua_State*L,Job&job){
    auto&p=owner(L);auto*r=job.context_postread_load_tail;
    if(L!=p.state||!r||!r->globals||r->globals->master!=L)
        return (void)luaL_error(L,"postRead ContextPtr requires its actual Host master");
    native_postread_load_tail_full_owned(L);
    if(!native_postread_globals_retained_thread_without_callbacks(p,*r->globals))
        return (void)luaL_error(L,"postRead ContextPtr physical518 keeper retired before pointer borrow");
    lua_State*thread=r->globals->thread;
    if(lua_status(thread)||lua_gettop(thread)!=0)
        return (void)luaL_error(L,"postRead ContextPtr genuine518 coroutine is not idle");
    r->entry_top=lua_gettop(thread);
    r->protected_status=lua_cpcall(thread,native_postread_contextptr_coroutine_owned,&job);
    if(r->protected_status){
        if(!lua_checkstack(L,1))return (void)luaL_error(L,"postRead ContextPtr error transfer reserve failed");
        lua_xmove(thread,L,1);lua_settop(thread,r->entry_top);return (void)lua_error(L);
    }
    r->returned_top=lua_gettop(thread);
    if(r->returned_top!=r->entry_top||(r->stage!=NativePostreadLoadTailControl::Stage::prefix_returned&&
        r->stage!=NativePostreadLoadTailControl::Stage::registrations_returned))
        return (void)luaL_error(L,"postRead ContextPtr cpcall did not return unchanged physical518 stack");
    native_postread_load_tail_full_owned(L);
}
static int native_postread_registrations_coroutine_body_owned(lua_State*L){
    auto*job=static_cast<Job*>(lua_touserdata(L,1));auto&p=owner(L);
    auto*r=job?job->context_postread_load_tail:nullptr;
    if(!job||job->owner!=&p||!r||L!=r->globals->thread||lua_gettop(L)!=1||
        !p.postread_load_tail_frame||&p.postread_load_tail_frame->control!=r||
        p.postread_load_tail_frame->actual_job!=job||
        job->operation!=Operation::native_context_postread_registrations_acquire||
        r->stage!=NativePostreadLoadTailControl::Stage::prefix_returned)
        return luaL_error(L,"postRead ordered registrars actual Job/frame/predecessor differs");
    if(!lua_checkstack(L,24))return luaL_error(L,"postRead registration physical query stack reserve failed");
    native_postread_load_tail_full_owned(L);
    if(!native_postread_load_tail_contextptr_physical_owned(L,*r))
        return luaL_error(L,"postRead registrars real ContextPtr38/Globals1C predecessor retired");
    lua_rawgeti(L,LUA_REGISTRYINDEX,r->contextptr38.reference);native_postread_load_tail_full_owned(L);
    const int contextptr=lua_gettop(L);
    native_postread_lookup_control_registration_owned(L,contextptr,&r->lookup_argument_reference,&r->lookup_copy_reference);
    native_postread_load_tail_lookup_publication_full_owned(L);
    if(lua_gettop(L)!=contextptr)return luaL_error(L,"LookUpControl4EE620 did not restore actual offered stack");
    lua_settop(L,-2);native_postread_load_tail_full_owned(L);
    r->stage=NativePostreadLoadTailControl::Stage::lookup_registered;
    //Actual457616 creates fresh UIManager-name local10; existing original
    //ContextPtr-name local10 has already undergone its normal4575A4 DTOR.
    native_postread_load_tail_named_key_8f8b10_owned(L,r->ui_manager_key10,"UIManager");
    lua_rawgeti(L,LUA_REGISTRYINDEX,r->globals->globals_reference);native_postread_load_tail_full_owned(L);
    const int globals=lua_gettop(L);
    native_postread_ui_manager_registration_owned(L,globals,r->ui_manager_key10.reference);
    if(lua_gettop(L)!=globals)return luaL_error(L,"UIManager4A1900 did not restore actual offered stack");
    lua_settop(L,-2);native_postread_load_tail_full_owned(L);
    native_postread_load_tail_unref_8f8820_owned(L,r->ui_manager_key10);
    r->stage=NativePostreadLoadTailControl::Stage::ui_registered;
    //Actual45767C fresh TTManager-name local10 +4A?4188E0 real embedded value.
    native_postread_load_tail_named_key_8f8b10_owned(L,r->tt_manager_key10,"TTManager");
    lua_rawgeti(L,LUA_REGISTRYINDEX,r->globals->globals_reference);native_postread_load_tail_full_owned(L);
    native_postread_tt_manager_registration_owned(L,lua_gettop(L),r->tt_manager_key10.reference);
    if(lua_gettop(L)!=globals)return luaL_error(L,"TTManager4188E0 did not restore actual offered stack");
    lua_settop(L,-2);native_postread_load_tail_full_owned(L);
    native_postread_load_tail_unref_8f8820_owned(L,r->tt_manager_key10);
    r->stage=NativePostreadLoadTailControl::Stage::tt_registered;
    if(lua_gettop(L)!=1||!native_postread_load_tail_contextptr_physical_owned(L,*r))
        return luaL_error(L,"postRead ordered LookUpControl/UIManager/TTManager normal returns changed");
    native_postread_load_tail_lookup_publication_full_owned(L);
    r->stage=NativePostreadLoadTailControl::Stage::registrations_returned;return 0;
}
static int native_postread_registrations_coroutine_owned(lua_State*L){
    auto&p=owner(L);p.source_exception={};p.source_error=nullptr;
    try{return native_postread_registrations_coroutine_body_owned(L);}
    catch(...){p.source_exception=std::current_exception();p.source_error="postRead real ordered registrar exception";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}return 0;
}
static void perform_native_postread_registrations_owned(lua_State*L,Job&job){
    auto&p=owner(L);auto*r=job.context_postread_load_tail;
    if(L!=p.state||!r||!r->globals||r->globals->master!=L)
        return (void)luaL_error(L,"postRead registration requires actual Host master");
    native_postread_load_tail_full_owned(L);
    if(!native_postread_globals_retained_thread_without_callbacks(p,*r->globals))
        return (void)luaL_error(L,"postRead registration actual518 keeper retired before pointer borrow");
    lua_State*thread=r->globals->thread;
    if(lua_status(thread)||lua_gettop(thread))return (void)luaL_error(L,"postRead registrar518 not idle");
    r->entry_top=lua_gettop(thread);
    r->protected_status=lua_cpcall(thread,native_postread_registrations_coroutine_owned,&job);
    if(r->protected_status){
        if(!lua_checkstack(L,1))return (void)luaL_error(L,"postRead registrar error transfer reserve failed");
        lua_xmove(thread,L,1);lua_settop(thread,r->entry_top);return (void)lua_error(L);
    }
    r->returned_top=lua_gettop(thread);
    if(r->entry_top||r->returned_top||r->stage!=NativePostreadLoadTailControl::Stage::registrations_returned)
        return (void)luaL_error(L,"postRead original ordered registrar protected return missing");
    native_postread_load_tail_full_owned(L);
}
static int native_postread_load_tail_cleanup_owned(lua_State*L){
    auto*r=static_cast<NativePostreadLoadTailControl*>(lua_touserdata(L,1));
    if(!r||!r->globals||L!=r->globals->thread||r->creator!=std::this_thread::get_id())
        return luaL_error(L,"postRead tail exception cleanup belongs to another actual518");
    if(!lua_checkstack(L,4))return luaL_error(L,"postRead tail exception cleanup stack reserve failed");
    if((r->lookup_copy_unref_started&&!r->lookup_copy_unref_returned)||
        (r->lookup_argument_unref_started&&!r->lookup_argument_unref_returned))
        return luaL_error(L,"postRead tail uncertain LookUpControl temporary unref is left to VM teardown");
    if(r->lookup_copy_reference!=LUA_NOREF){
        r->lookup_copy_unref_started=true;
        luaL_unref(L,LUA_REGISTRYINDEX,r->lookup_copy_reference);
        r->lookup_copy_unref_returned=true;r->lookup_copy_reference=LUA_NOREF;
    }
    if(r->lookup_argument_reference!=LUA_NOREF){
        r->lookup_argument_unref_started=true;
        luaL_unref(L,LUA_REGISTRYINDEX,r->lookup_argument_reference);
        r->lookup_argument_unref_returned=true;r->lookup_argument_reference=LUA_NOREF;
    }
    for(auto*value:{&r->tt_manager_key10,&r->ui_manager_key10,&r->contextptr38,&r->copied24,&r->lookup_temporary2c}){
        if(value->unref_started&&!value->unref_returned)
            return luaL_error(L,"postRead tail uncertain unref is left to real VM teardown");
        if(value->state&&value->reference!=LUA_NOREF){
            value->unref_started=true;luaL_unref(L,LUA_REGISTRYINDEX,value->reference);value->unref_returned=true;
        }
        value->state=nullptr;value->reference=LUA_NOREF;
    }
    return 0;
}
void native_postread_load_tail_release_after_abort_owned(NativePostreadLoadTailControl&r)noexcept{
    if(r.creator!=std::this_thread::get_id()||r.creator!=ui_thread)std::terminate();
    if(r.closed)return;
    if(!r.globals||!native_postread_globals_retained_thread_without_callbacks(*this,*r.globals)){
        r.closed=true;r.stage=NativePostreadLoadTailControl::Stage::failed;return;
    }
    lua_State*L=r.globals->thread;const int top=lua_gettop(L);const bool previous=calling;calling=true;
    const int status=lua_cpcall(L,native_postread_load_tail_cleanup_owned,&r);lua_settop(L,top);calling=previous;
    r.closed=true;r.stage=status?NativePostreadLoadTailControl::Stage::failed:NativePostreadLoadTailControl::Stage::retired;
}
void native_postread_load_tail_close_owned()noexcept{
    for(auto&r:postread_load_tail_controls)if(r&&!r->closed)native_postread_load_tail_release_after_abort_owned(*r);
}
