// Inside actual Host::Impl, AFTER the genuine postread tail Control/Frame and
// CodeBuddy8F7E20. The lexical tail authorizes registration only. Late calls
// acquire a fresh protected Source operation and physical current receivers.
struct NativePostreadLookupControlReturn final {
    const Impl*const host;const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<ui::detail::NotificationLayoutContextOrdinaryState> root;
    const NativeContextCDataCreator*const type;
    const void*const contextptr_table;
    const void*function=nullptr;
    enum class Stage {captured,closure,argument,copy,published,copy_retired,argument_retired,returned};
    Stage stage=Stage::captured;
    NativePostreadLookupControlReturn(const Impl*p,
        std::weak_ptr<ui::detail::NotificationLayoutContextOrdinaryState>r,
        const NativeContextCDataCreator*t,const void*table)
        :host(p),root(std::move(r)),type(t),contextptr_table(table){}
};

// Real outer C++ custody remains alive around lua_pcall. No owning local on a
// Lua C callback stack survives an allocating/error API or source callback.
struct NativePostreadLookupCallScope final {
    Impl&host;lua_State*const thread;NativePostreadLookupCallScope*const previous;
    const std::shared_ptr<Impl> held_host;
    const std::shared_ptr<BnwSelectedWorldViewContextOwner> source;
    const std::shared_ptr<ui::BnwEngineUiEndTurnQueue::VmLifetime> life;
    const BnwWorldViewSourceOperationLoan*const operation;
    const void*const callback_scope;
    const std::size_t child;
    const std::uint64_t epoch,writes,uncertainty;
    std::shared_ptr<NativeContextMetatypeResult> expected_context_type;
    std::shared_ptr<ui::detail::NotificationLayoutContextOrdinaryState> context;
    std::shared_ptr<const ui::detail::NotificationLayoutOrdinaryReceiver> receiver;
    std::shared_ptr<const void> concrete;
    std::shared_ptr<NativePostreadRootCDataControl> root_result;
    std::shared_ptr<NativeChildContextControlsResult> child_result;
    std::shared_ptr<NativeRootOrdinaryControlsResult> ordinary_result;
    std::shared_ptr<ui::BnwWorldViewControlTree> tree;
    int metatype_reference=LUA_NOREF;const void*metatype=nullptr;const char*result_type_name=nullptr;
    const void*cdata=nullptr;
    NativePostreadLookupCallScope(Impl&p,lua_State*L)
        :host(p),thread(L),previous(p.postread_lookup_call),held_host(p.weak_from_this().lock()),
         source(p.worldview_sources.lock()),life(p.worldview_lifetime.lock()),operation(p.worldview_operation),
         callback_scope(p.worldview_native_host),child(p.worldview_native_host?p.worldview_native_host->child:SIZE_MAX),
         epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty) {
        if(!held_host||held_host.get()!=&p||!source||!life||!operation||!callback_scope||
            child>=p.child_contexts.size()||!p.state||!L||G(L)!=G(p.state))
            throw ScriptError("LookUpControl fresh actual Host/Source/VM scope absent");
        for(const auto&r:p.native_context_metatype_results){
            if(r&&r->host==&p&&r->alive.load()&&r->creator==std::this_thread::get_id()&&
                r->stage==NativeContextMetatypeResult::Stage::returned&&r->descriptor&&r->descriptor->context&&
                r->descriptor==p.native_context_definitions[0]&&r->table&&r->table_ref>=0){
                if(expected_context_type&&expected_context_type->table!=r->table)
                    throw ScriptError("LookUpControl multiple replaced Context metatype issuers");
                expected_context_type=r;
            }
        }
        if(!expected_context_type)throw ScriptError("LookUpControl actual Context43F610/517C type is not issued");
        p.postread_lookup_call=this;
    }
    ~NativePostreadLookupCallScope(){
        if(host.postread_lookup_call==this)host.postread_lookup_call=previous;else host.record_failure();
    }
    void first()const {
        if(host.postread_lookup_call!=this||held_host.get()!=&host||!life->alive||life->failed||
            life!=host.end_turn_lifetime||life->vm!=host.state||life->host!=host.worldview_host_owner||
            source!=host.worldview_sources.lock()||operation!=host.worldview_operation||
            callback_scope!=host.worldview_native_host||child>=host.child_contexts.size()||
            host.worldview_native_host->child!=child||epoch!=host.failure_epoch||writes!=host.failed.writes_||
            uncertainty!=host.execution_uncertainty||!thread||G(thread)!=G(host.state)||
            !expected_context_type||!expected_context_type->alive.load()||
            expected_context_type->stage!=NativeContextMetatypeResult::Stage::returned||
            expected_context_type->descriptor!=host.native_context_definitions[0])
            throw ScriptError("LookUpControl fresh invocation custody/current failure cohort changed");
        host.worldview_require_callback_origin();
    }
    void full()const {
        first();operation->require_same_vm(*host.worldview_host_owner);first();
        if(context)ui::BnwWorldViewControlTree::require_lookup_context_current_owned(*operation,context);
        first();
        if(root_result){
            if(root_result->closed||root_result->stage!=NativePostreadRootCDataControl::Stage::returned||
                root_result->type!=host.native_context_cdata_creator||!root_result->type->alive.load()||
                !root_result->parent||!root_result->parent->source)
                throw ScriptError("LookUpControl actual root CData/type issuer retired");
            operation->require_same_source(*root_result->parent->source);
        }
        if(child_result){
            if(child_result->stage!=NativeChildContextControlsResult::Stage::returned||
                child_result->host.lock().get()!=&host||child_result->cdata_creator!=host.native_context_cdata_creator||
                !child_result->cdata_creator->alive.load()||!child_result->child_source||!child_result->parent_source)
                throw ScriptError("LookUpControl actual child CData/type issuer retired");
            operation->require_same_source(*child_result->child_source);
            operation->require_same_source(*child_result->parent_source);
        }
        if(ordinary_result){
            const auto&r=*ordinary_result;const auto index=static_cast<std::size_t>(r.cdata_creator->subtype);
            if(r.stage!=NativeRootOrdinaryControlsResult::Stage::returned||r.host.lock().get()!=&host||
                index>=host.native_ordinary_cdata_creators.size()||host.native_ordinary_cdata_creators[index]!=r.cdata_creator||
                !r.cdata_creator->alive.load()||!r.metatype_owner||!r.metatype_owner->alive.load()||
                r.metatype_owner->stage!=NativeOrdinaryMetatypeResult::Stage::returned||
                r.metatype_owner->table!=r.metatable||!r.parent_source)
                throw ScriptError("LookUpControl actual ordinary CData/type issuer retired");
            operation->require_same_source(*r.parent_source);
        }
        first();
        if(receiver){
            const auto physical=ui::BnwWorldViewControlTree::lookup_receiver_physical_owned(*operation,receiver);
            if(physical!=concrete)throw ScriptError("LookUpControl actual returned receiver storage replaced");
        }
        first();
    }
    void resolve(const void*pointer,const char*path) {
        first();
        // Accept genuine registered root/child CData pointers. The pointer is
        // never interpreted as an ID or as a copied authored-control object.
        for(const auto&r:host.postread_root_cdata_controls){
            if(r&&r->cdata==pointer&&!r->closed&&r->stage==NativePostreadRootCDataControl::Stage::returned&&
                r->creator==std::this_thread::get_id()&&r->type==host.native_context_cdata_creator){
                context=r->root.lock();if(context&&context.get()==pointer){root_result=r;break;}
            }
        }
        if(!context)for(const auto&r:host.child_controls_export_results){
            if(r&&r->child_cdata==pointer&&r->host.lock().get()==&host&&
                r->stage==NativeChildContextControlsResult::Stage::returned&&
                r->cdata_creator==host.native_context_cdata_creator){
                context=r->child.lock();if(context&&context.get()==pointer){child_result=r;break;}
            }
        }
        if(!context)throw ScriptError("LookUpControl nonNULL Context pointer has no live actual physical issuer");
        ui::BnwWorldViewControlTree::require_lookup_context_current_owned(*operation,context);full();
        receiver=ui::BnwWorldViewControlTree::lookup_context_relative_owned(*operation,context,path);
        // Null is earned only from the actual NULL/path/map branch above.
        if(!receiver)return;
        concrete=ui::BnwWorldViewControlTree::lookup_receiver_physical_owned(*operation,receiver);
        cdata=concrete.get();root_result.reset();child_result.reset();
        for(const auto&r:host.postread_root_cdata_controls){
            if(r&&r->cdata==cdata&&r->root.lock().get()==cdata&&!r->closed&&
                r->stage==NativePostreadRootCDataControl::Stage::returned&&
                r->type==host.native_context_cdata_creator&&r->metatype_ref>=0){
                root_result=r;metatype_reference=r->metatype_ref;metatype=r->metatype;result_type_name=r->type->type_name;break;
            }
        }
        if(!root_result)for(const auto&r:host.child_controls_export_results){
            if(r&&r->receiver==receiver&&r->child_cdata==cdata&&r->host.lock().get()==&host&&
                r->stage==NativeChildContextControlsResult::Stage::returned&&
                r->cdata_creator==host.native_context_cdata_creator&&r->metatable_ref>=0){
                child_result=r;tree=r->child_tree.lock();metatype_reference=r->metatable_ref;metatype=r->metatable;
                result_type_name=r->cdata_creator->type_name;break;
            }
        }
        if(!root_result&&!child_result)for(const auto&r:host.root_ordinary_controls_export_results){
            if(r&&r->receiver==receiver&&r->concrete==concrete&&r->host.lock().get()==&host&&
                r->stage==NativeRootOrdinaryControlsResult::Stage::returned&&r->metatype_owner&&
                r->metatype_owner->alive.load()&&r->cdata_creator&&r->cdata_creator->alive.load()&&
                r->cdata_creator->host==&host&&r->metatable_ref>=0){
                ordinary_result=r;tree=r->tree.lock();metatype_reference=r->metatable_ref;metatype=r->metatable;
                result_type_name=r->cdata_creator->type_name;break;
            }
        }
        if(metatype_reference<0||!metatype||!result_type_name||(!root_result&&!tree))
            throw ScriptError("LookUpControl actual virtual+80 requires a live registered concrete subtype/type owner");
        full();
    }
};

static NativePostreadLookupCallScope&native_postread_lookup_call_full_owned(lua_State*L){
    native_ordinary_metatype_full(L);auto&p=owner(L);p.source_exception={};p.source_error=nullptr;
    try{
        if(!p.postread_lookup_call||p.postread_lookup_call->thread!=L)
            throw ScriptError("LookUpControl real invocation C++ custody absent");
        p.postread_lookup_call->full();
    }
    catch(...){p.source_exception=std::current_exception();p.source_error="LookUpControl fresh Source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    return *p.postread_lookup_call;
}

static void native_postread_lookup_expected_context_metatype_owned(lua_State*L){
    auto&r=native_postread_lookup_call_full_owned(L);
    // Genuine existing615D70 cache-HIT and actual already-issued43F610 type.
    native_ordinary_push_master_types_615d70_owned(L);native_postread_lookup_call_full_owned(L);
    lua_getfield(L,-1,"LuaContext");native_postread_lookup_call_full_owned(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.expected_context_type->table_ref);
    if(lua_type(L,-2)!=LUA_TTABLE||lua_topointer(L,-2)!=r.expected_context_type->table||!lua_rawequal(L,-1,-2))
        return (void)luaL_error(L,"LookUpControl actual Context cache/type allocation replaced");
    lua_pop(L,1);lua_remove(L,-2);native_postread_lookup_call_full_owned(L);
}
static bool native_postread_lookup_context_cast_8987d60_owned(lua_State*L,int offered){
    native_postread_lookup_expected_context_metatype_owned(L);
    const bool has=lua_getmetatable(L,offered)!=0;native_postread_lookup_call_full_owned(L);
    if(!has){lua_settop(L,-2);return false;}
    bool equal=lua_equal(L,-1,-2)!=0;native_postread_lookup_call_full_owned(L);
    while(!equal){
        lua_getfield(L,-1,"BaseType");native_postread_lookup_call_full_owned(L);
        if(lua_type(L,-1)!=LUA_TTABLE){lua_settop(L,-4);return false;}
        lua_remove(L,-2);equal=lua_equal(L,-1,-2)!=0;native_postread_lookup_call_full_owned(L);
    }
    lua_settop(L,-3);native_postread_lookup_call_full_owned(L);return true;
}
[[noreturn]]static void native_postread_lookup_diagnostic_8f7960_required_owned(lua_State*L,const char*site){
    native_postread_lookup_call_full_owned(L);
    luaL_error(L,"LookUpControl requires actual returning8F7960/73B280 diagnostic sink at %s",site);
    std::terminate();
}
static const void*native_postread_lookup_context_pointer_898030_owned(lua_State*L){
    const int type=lua_type(L,1);native_postread_lookup_call_full_owned(L);
    if(type==LUA_TTABLE){
        if(native_postread_lookup_context_cast_8987d60_owned(L,1)){
            lua_pushvalue(L,1);lua_getfield(L,-1,"CData");native_postread_lookup_call_full_owned(L);
            const void*const result=lua_touserdata(L,-1);lua_settop(L,-3);return result;
        }
        // Original expected-name8987E00 and offered-name8987F20 precede
        // returning8F7960. Preserve their real Lua effects before the frontier.
        native_postread_lookup_expected_context_metatype_owned(L);
        lua_getfield(L,-1,"CTypeName");(void)lua_tolstring(L,-1,nullptr);lua_settop(L,-3);
        if(lua_getmetatable(L,1)){lua_getfield(L,-1,"CTypeName");(void)lua_tolstring(L,-1,nullptr);lua_settop(L,-3);}
        native_postread_lookup_diagnostic_8f7960_required_owned(L,"898030 failed Context reflection cast");
    }
    const int repeated=lua_type(L,1);native_postread_lookup_call_full_owned(L);
    if(repeated==LUA_TLIGHTUSERDATA)return lua_touserdata(L,1);
    native_postread_lookup_diagnostic_8f7960_required_owned(L,"898030 incorrect pointer argument type");
}
static int native_postread_lookup_control_6c44d0_inner_body_owned(lua_State*L){
    auto&r=native_postread_lookup_call_full_owned(L);const void*const pointer=native_postread_lookup_context_pointer_898030_owned(L);
    if(!pointer){lua_pushnil(L);native_postread_lookup_call_full_owned(L);return 1;}
    const char*const path=lua_tolstring(L,2,nullptr);native_postread_lookup_call_full_owned(L);
    auto&p=owner(L);p.source_exception={};p.source_error=nullptr;
    try{r.resolve(pointer,path);}catch(...){p.source_exception=std::current_exception();p.source_error="LookUpControl actual relative receiver lookup refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    native_postread_lookup_call_full_owned(L);
    if(!r.receiver){lua_pushnil(L);native_postread_lookup_call_full_owned(L);return 1;}
    // Actual Context virtual+80 is496400 (A368D0+80); its fresh CData/type
    // prefix is shared with6768B0. Return a NEW genuine concrete wrapper.
    lua_createtable(L,0,0);native_postread_lookup_call_full_owned(L);
    lua_pushlightuserdata(L,const_cast<void*>(r.cdata));lua_setfield(L,-2,"CData");native_postread_lookup_call_full_owned(L);
    // Preserve the real creator's615D70/getfield/remove cache-HIT effects.
    native_ordinary_push_master_types_615d70_owned(L);native_postread_lookup_call_full_owned(L);
    lua_getfield(L,-1,r.result_type_name);native_postread_lookup_call_full_owned(L);
    if(lua_type(L,-1)!=LUA_TTABLE)
        return luaL_error(L,"LookUpControl concrete creator cache-MISS requires its actual517C constructor lifecycle");
    lua_remove(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,r.metatype_reference);
    if(lua_type(L,-2)!=LUA_TTABLE||lua_topointer(L,-2)!=r.metatype||!lua_rawequal(L,-1,-2))
        return luaL_error(L,"LookUpControl actual concrete metatype keeper changed");
    lua_pop(L,1);
    if(!lua_setmetatable(L,-2))return luaL_error(L,"LookUpControl actual concrete metatype assignment failed");
    native_postread_lookup_call_full_owned(L);return 1;
}
static int native_postread_lookup_control_6c44d0_inner_owned(lua_State*L){
    auto&p=owner(L);p.source_exception={};p.source_error=nullptr;
    try{return native_postread_lookup_control_6c44d0_inner_body_owned(L);}
    catch(...){p.source_exception=std::current_exception();p.source_error="LookUpControl protected body C++ supplier failed";}
    p.record_failure();return luaL_error(L,"%s",p.source_error);
}
static int native_postread_lookup_control_6c44d0_owned(lua_State*L){
    native_ordinary_metatype_full(L);auto&p=owner(L);const int arguments=lua_gettop(L);
    if(!lua_checkstack(L,32))return luaL_error(L,"LookUpControl protected stack reserve failed");
    // Setup Lua call operands before acquiring C++ custody. The inner frame
    // receives every original argument; no stricter arity rule is introduced.
    lua_pushcfunction(L,&Impl::native_postread_lookup_control_6c44d0_inner_owned);lua_insert(L,1);
    int status=LUA_ERRRUN;p.source_exception={};p.source_error=nullptr;
    try{
        NativePostreadLookupCallScope scope(p,L);
        status=lua_pcall(L,arguments,1,0); // Scope is OUTSIDE the protected C frame.
    }catch(...){p.source_exception=std::current_exception();p.source_error="LookUpControl invocation custody acquisition failed";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    if(status!=0)return lua_error(L); // All C++ custody has already unwound.
    native_ordinary_metatype_full(L);return 1;
}

static int native_postread_lookup_primary_898390_owned(lua_State*L){
    native_ordinary_metatype_full(L);
    const auto function=reinterpret_cast<lua_CFunction>(lua_touserdata(L,lua_upvalueindex(1)));
    const char*const name=lua_tolstring(L,lua_upvalueindex(2),nullptr);
    if(function!=&Impl::native_postread_lookup_control_6c44d0_owned||!name||std::strcmp(name,"LookUpControl"))
        return luaL_error(L,"LookUpControl actual two-upvalue primary identity changed");
    return function(L);
}
static int native_postread_lookup_secondary_8983c0_owned(lua_State*L){
    native_ordinary_metatype_full(L);
    if(!lua_iscfunction(L,-1)){lua_pushstring(L,"Misused ToString Function!");return 1;}
    if(lua_getupvalue(L,-1,1)){(void)lua_touserdata(L,-1);lua_settop(L,-2);}
    const char*name="UnknownFunction";
    if(lua_getupvalue(L,-1,2)){name=lua_tolstring(L,-1,nullptr);lua_settop(L,-2);}
    if(!name)return luaL_error(L,"LookUpControl signature requires original nonNULL formatter operand");
    char buffer[256];const int written=std::snprintf(buffer,sizeof(buffer),"CFunction: %s",name);
    if(written<0||static_cast<std::size_t>(written)>=sizeof(buffer))
        return luaL_error(L,"LookUpControl original256-byte signature formatter input unsupported");
    lua_pushstring(L,buffer);native_ordinary_metatype_full(L);return 1;
}

static void native_postread_lookup_control_return_full_owned(lua_State*L,const NativePostreadLookupControlReturn&r){
    auto&control=native_postread_load_tail_current_owned(L);auto&p=owner(L);
    const bool retained=std::any_of(p.postread_lookup_returns.begin(),p.postread_lookup_returns.end(),
        [&](const auto&value){return value.get()==&r;});
    if(!retained||r.host!=&p||r.creator!=std::this_thread::get_id()||
        r.stage!=NativePostreadLookupControlReturn::Stage::returned||control.lookup_return!=&r||
        r.root.lock()!=p.postread_load_tail_frame->held_root||r.type!=control.cdata->type.get()||
        !r.function||r.contextptr_table!=control.cdata->table||control.lookup_argument_reference!=LUA_NOREF||
        control.lookup_copy_reference!=LUA_NOREF||!control.lookup_argument_unref_started||!control.lookup_copy_unref_started||
        !control.lookup_argument_unref_returned||!control.lookup_copy_unref_returned)
        return (void)luaL_error(L,"LookUpControl actual published return/normal ref DTOR changed");
    const int top=lua_gettop(L);lua_rawgeti(L,LUA_REGISTRYINDEX,control.cdata->table_ref);
    if(lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r.contextptr_table)
        return (void)luaL_error(L,"LookUpControl actual ContextPtr table keeper changed");
    lua_getfield(L,-1,"LookUpControl");
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=&Impl::native_postread_lookup_primary_898390_owned||
        lua_topointer(L,-1)!=r.function)return (void)luaL_error(L,"LookUpControl actual field/closure identity changed");
    if(!lua_getupvalue(L,-1,1)||lua_type(L,-1)!=LUA_TLIGHTUSERDATA||
        lua_touserdata(L,-1)!=reinterpret_cast<void*>(&Impl::native_postread_lookup_control_6c44d0_owned))
        return (void)luaL_error(L,"LookUpControl actual function-pointer upvalue changed");
    lua_pop(L,1);if(!lua_getupvalue(L,-1,2)||lua_type(L,-1)!=LUA_TSTRING||
        std::strcmp(lua_tostring(L,-1),"LookUpControl"))
        return (void)luaL_error(L,"LookUpControl actual name upvalue changed");
    lua_pop(L,1);const int primary=lua_gettop(L);
    native_ordinary_push_codebuddy_table_8f7d40_owned(L);lua_pushvalue(L,primary);lua_gettable(L,-2);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=&Impl::native_postread_lookup_secondary_8983c0_owned)
        return (void)luaL_error(L,"LookUpControl genuine CodeBuddy association changed");
    lua_settop(L,top);native_postread_load_tail_full_owned(L);
}

static void native_postread_lookup_control_registration_owned(lua_State*L,int contextptr_table,
    int*argument_ref,int*copy_ref){
    auto&control=native_postread_load_tail_current_owned(L);auto&p=owner(L);
    if(contextptr_table<=0||lua_type(L,contextptr_table)!=LUA_TTABLE||
        lua_topointer(L,contextptr_table)!=control.cdata->table||
        argument_ref!=&control.lookup_argument_reference||copy_ref!=&control.lookup_copy_reference||
        *argument_ref!=LUA_NOREF||*copy_ref!=LUA_NOREF||control.lookup_return||
        control.lookup_argument_unref_started||control.lookup_copy_unref_started)
        return (void)luaL_error(L,"LookUpControl actual ContextPtr/byvalue slots absent/replayed");
    NativePostreadLookupControlReturn*r=nullptr;p.source_exception={};p.source_error=nullptr;
    try{
        p.postread_lookup_returns.push_back(std::make_unique<NativePostreadLookupControlReturn>(&p,
            p.postread_load_tail_frame->held_root,control.cdata->type.get(),control.cdata->table));
        r=p.postread_lookup_returns.back().get();
    }catch(...){p.source_exception=std::current_exception();p.source_error="LookUpControl publication custody allocation failed";}
    if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
    native_postread_load_tail_full_owned(L);const int top=lua_gettop(L);
    lua_pushlightuserdata(L,reinterpret_cast<void*>(&Impl::native_postread_lookup_control_6c44d0_owned));
    lua_pushstring(L,"LookUpControl");lua_pushcclosure(L,&Impl::native_postread_lookup_primary_898390_owned,2);
    native_postread_load_tail_full_owned(L);r->function=lua_topointer(L,-1);r->stage=NativePostreadLookupControlReturn::Stage::closure;
    native_flua_associate_codebuddy_8f7e20_owned(L,&Impl::native_postread_lookup_secondary_8983c0_owned);
    native_postread_load_tail_full_owned(L);
    // Exact8F8540 duplicates selector-1 to a real byvalue argument reference.
    (void)lua_checkstack(L,1);lua_pushvalue(L,-1);*argument_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    r->stage=NativePostreadLookupControlReturn::Stage::argument;native_postread_load_tail_full_owned(L);
    lua_settop(L,-2);native_postread_load_tail_full_owned(L); // Actual4575F4 pops offered closure.
    // Actual4EE626/8F88F0 pushes the genuine copied38 ContextPtr table first.
    (void)lua_checkstack(L,1);lua_pushvalue(L,contextptr_table);native_postread_load_tail_full_owned(L);
    // Exact8F84A0 copy: consume real argument ref, duplicate its referent.
    (void)lua_checkstack(L,1);lua_rawgeti(L,LUA_REGISTRYINDEX,*argument_ref);
    *copy_ref=luaL_ref(L,LUA_REGISTRYINDEX);r->stage=NativePostreadLookupControlReturn::Stage::copy;
    native_postread_load_tail_full_owned(L);
    (void)lua_checkstack(L,1);lua_rawgeti(L,LUA_REGISTRYINDEX,*copy_ref);
    lua_setfield(L,-2,"LookUpControl");r->stage=NativePostreadLookupControlReturn::Stage::published;
    native_postread_load_tail_full_owned(L);lua_settop(L,-2);native_postread_load_tail_full_owned(L);
    // RET0xC destroys internal copy before the eight-byte byvalue argument.
    control.lookup_copy_unref_started=true;luaL_unref(L,LUA_REGISTRYINDEX,*copy_ref);
    *copy_ref=LUA_NOREF;control.lookup_copy_unref_returned=true;r->stage=NativePostreadLookupControlReturn::Stage::copy_retired;
    native_postread_load_tail_full_owned(L);
    control.lookup_argument_unref_started=true;luaL_unref(L,LUA_REGISTRYINDEX,*argument_ref);
    *argument_ref=LUA_NOREF;control.lookup_argument_unref_returned=true;r->stage=NativePostreadLookupControlReturn::Stage::argument_retired;
    native_postread_load_tail_full_owned(L);
    if(lua_gettop(L)!=top)return (void)luaL_error(L,"LookUpControl original caller net stack changed");
    r->stage=NativePostreadLookupControlReturn::Stage::returned;control.lookup_return=r;
    native_postread_lookup_control_return_full_owned(L,*r);
}
