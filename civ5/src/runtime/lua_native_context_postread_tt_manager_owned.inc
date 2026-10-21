// Inside Host::Impl. Actual4188E0 Table::SetField ->898350 reflection.
// World17 owns the real Table/Reference wrappers and captures the distinct GUI
// operand OUTER before Lua. Registration does not invoke any TT method.
struct NativePostreadTTManagerReturn final {
    Impl*const host;lua_State*const master;global_State*const global;const Table*const registry_table;
    const int master_globals_ref;const void*const master_globals;
    const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;
    const std::size_t child;const int environment_ref,thread_ref;
    const void*const environment;lua_State*const thread;
    const std::weak_ptr<const ScriptUiChildSource> source;const ScriptUiChildSource*const source_get;
    const std::shared_ptr<const ui::detail::NotificationContextTooltipManagerOperand> operand;
    const void*const cdata;
    enum class Stage {captured,keys,reflected,metatype,published,returned,retired};Stage stage=Stage::captured;
    int keys_ref=LUA_NOREF,table_ref=LUA_NOREF,metatype_ref=LUA_NOREF;
    const void*keys=nullptr,*table=nullptr,*metatype=nullptr;
    NativeFLuaMetatypeReturn*metatype_owner=nullptr;NativeFLuaMethodLibraryReturn*library=nullptr;
    const NativeOrdinaryReflectionResult*reflection_owner=nullptr;
    NativePostreadTTManagerReturn(Impl&p,lua_State*selected,
        std::shared_ptr<const ui::detail::NotificationContextTooltipManagerOperand>actual)
      :host(&p),master(p.state),global(G(p.state)),registry_table(hvalue(&G(p.state)->l_registry)),
       master_globals_ref(p.native_master_thread_bootstrap->globals_ref),master_globals(p.native_master_thread_bootstrap->globals),
       epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty),
       child(p.worldview_native_host->child),environment_ref(p.child_contexts[child].environment),
       thread_ref(p.child_contexts[child].execution_thread),environment(lua_topointer(selected,LUA_GLOBALSINDEX)),
       thread(selected),source(p.child_contexts[child].source),
       source_get(p.child_contexts[child].source.get()),operand(std::move(actual)),cdata(operand->cdata_){}
};
static void native_postread_tt_manager_cpp_current_owned(Impl&p,const NativePostreadTTManagerReturn&r){
    if(r.creator!=std::this_thread::get_id()||r.creator!=p.ui_thread||r.host!=&p||r.master!=p.state||!p.state||
       G(p.state)!=r.global||!ttistable(&r.global->l_registry)||hvalue(&r.global->l_registry)!=r.registry_table||
       !p.native_master_thread_bootstrap||p.native_master_thread_bootstrap->retired||
       p.native_master_thread_bootstrap->vm!=p.state||p.native_master_thread_bootstrap->globals_ref!=r.master_globals_ref||
       p.native_master_thread_bootstrap->globals!=r.master_globals||!ttistable(gt(p.state))||hvalue(gt(p.state))!=r.master_globals||
       r.stage==NativePostreadTTManagerReturn::Stage::retired||r.epoch!=p.failure_epoch||r.writes!=p.failed.writes_||
       r.uncertainty!=p.execution_uncertainty||p.failed.exhausted_||!r.operand||r.cdata!=r.operand->cdata_)
        throw ScriptError("TTManager retained actual Host/VM/manager/failure custody retired");
    p.worldview_require_callback_origin();
    if(!p.worldview_native_host||p.worldview_native_host->child!=r.child||r.child>=p.child_contexts.size())
        throw ScriptError("TTManager invocation has another genuine native child Source");
    const auto&child=p.child_contexts[r.child];
    if(child.retired||!child.state_prepared||child.environment!=r.environment_ref||child.execution_thread!=r.thread_ref||
       child.source.get()!=r.source_get||r.source.expired()||r.source.owner_before(child.source)||child.source.owner_before(r.source)||
       !p.worldview_native_child_controls_current(r.child))
        throw ScriptError("TTManager real selected environment/source roots changed");
    // A fresh actual Source operation; no retired constructor Frame is read.
    r.operand->require_current_owned(*p.worldview_native_host->loan);p.worldview_require_callback_origin();
    if(r.stage==NativePostreadTTManagerReturn::Stage::metatype||r.stage==NativePostreadTTManagerReturn::Stage::published||
       r.stage==NativePostreadTTManagerReturn::Stage::returned){
        bool type_owned=false,library_owned=false,reflection_owned=false;
        for(const auto&item:p.native_flua_metatype_results)if(item.get()==r.metatype_owner){type_owned=true;break;}
        for(const auto&item:p.native_flua_method_libraries)if(item.get()==r.library){library_owned=true;break;}
        for(const auto&item:p.native_ordinary_reflection_results)if(item.get()==r.reflection_owner){reflection_owned=true;break;}
        if(!type_owned||!library_owned||!reflection_owned||!r.metatype_owner||!r.library||!r.reflection_owner||!r.metatype_owner->alive||
           !r.metatype_owner->returned||r.metatype_owner->host!=&p||r.metatype_owner->table!=r.metatype||
           !r.library->alive||!r.library->returned||r.library->host!=&p||r.library->metatype!=r.metatype||
           !r.reflection_owner->alive.load()||r.reflection_owner->host!=&p||
           r.reflection_owner->stage!=NativeOrdinaryReflectionResult::Stage::returned)
            throw ScriptError("TTManager genuine completed creator/library retired during callback");
        const auto*registry=p.native_flua_builtin_registry.get();
        if(!registry)throw ScriptError("TTManager genuine descriptor registry retired during callback");
        registry->physical_current_without_callbacks();
    }
}
static void native_postread_tt_manager_current_owned(lua_State*L,NativePostreadTTManagerReturn*r){
    auto&p=owner(L);p.source_exception={};p.source_error=nullptr;
    bool owned=false;for(const auto&item:p.native_postread_tt_manager_returns)if(item.get()==r){owned=true;break;}
    try{if(!owned||!r)throw ScriptError("TTManager actual retained publication owner absent");
        native_postread_tt_manager_cpp_current_owned(p,*r);}
    catch(...){p.source_exception=std::current_exception();p.source_error="TTManager genuine Source/provider currentness refused";}
    if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
    if(G(L)!=r->global)return (void)native_master_thread_refuse(L,"TTManager publication uses another real VM");
}
static void native_postread_tt_manager_key_owned(lua_State*L,const NativePostreadTTManagerReturn&r,int ordinal){
    static constexpr const char*keys[]={"CData","TTManager","ExposedTypes","ToolTipManager","ExposedMembers","CodeBuddy"};
    if(ordinal<1||ordinal>6||r.keys_ref<0||!r.keys)
        return (void)native_master_thread_refuse(L,"TTManager actual retained key custody absent");
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.keys_ref);
    if(lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r.keys)
        return (void)native_master_thread_refuse(L,"TTManager actual retained key table replaced");
    lua_rawgeti(L,-1,ordinal);std::size_t size=0;
    const char*key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&size):nullptr;
    if(!key||size!=std::strlen(keys[ordinal-1])||std::memcmp(key,keys[ordinal-1],size))
        return (void)native_master_thread_refuse(L,"TTManager original retained key changed");lua_remove(L,-2);
}
// Only the genuine current library's actual primary->secondary association is
// consumed. Master namespace custody is the completed bootstrap, not child fenv.
static bool native_postread_tt_manager_secondary_physical_owned(lua_State*L,
    const NativePostreadTTManagerReturn&r,const NativeFLuaMethodLibraryReturn&library,std::size_t ordinal){
    if(ordinal>=4)return false;
    const auto&row=library.rows[ordinal];auto&p=owner(L);bool owned=false;
    for(const auto&actual:p.native_ordinary_codebuddy_tables)if(actual.get()==row.codebuddy.get()){owned=true;break;}
    if(!owned||!row.codebuddy)return false;
    const auto&b=*row.codebuddy;
    if(b.host!=&p||!b.alive.load()||b.creator!=std::this_thread::get_id()||b.original!=0x8f7d40u||
       b.stage!=NativeOrdinaryCodeBuddyTableReturn::Stage::returned||!b.parent||!b.callback||
       b.parent->host!=&p||!b.parent->alive.load()||b.parent->creator!=std::this_thread::get_id()||
       b.parent->stage!=NativeOrdinaryCodeBuddyGlobalReturn::Stage::returned||b.parent->table_ref<0||
       b.callback!=p.native_ordinary_codebuddy_callback||b.callback->host!=&p||!b.callback->alive.load()||
       b.table_ref<0||b.weak_metadata_ref<0||!b.table||!b.weak_metadata||row.primary_ref<0||row.secondary_ref<0||!row.secondary)
        return false;
    const int top=lua_gettop(L);const auto stop=[&](bool ok){lua_settop(L,top);return ok;};
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.master_globals_ref);
    if(lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r.master_globals)return stop(false);
    native_postread_tt_manager_key_owned(L,r,6);lua_rawget(L,-2);
    lua_rawgeti(L,LUA_REGISTRYINDEX,b.parent->table_ref);
    if(!lua_rawequal(L,-1,-2)||lua_topointer(L,-1)!=b.parent->table)return stop(false);lua_pop(L,1);
    lua_rawgeti(L,-1,1);lua_rawgeti(L,LUA_REGISTRYINDEX,b.table_ref);
    if(!lua_rawequal(L,-1,-2)||lua_topointer(L,-1)!=b.table)return stop(false);lua_pop(L,1);
    const int buddies=lua_gettop(L);
    if(!lua_getmetatable(L,buddies))return stop(false);lua_rawgeti(L,LUA_REGISTRYINDEX,b.weak_metadata_ref);
    if(!lua_rawequal(L,-1,-2)||lua_topointer(L,-1)!=b.weak_metadata)return stop(false);lua_pop(L,1);
    lua_pushliteral(L,"__mode");lua_rawget(L,-2);
    if(lua_type(L,-1)!=LUA_TSTRING||lua_objlen(L,-1)!=1||std::memcmp(lua_tostring(L,-1),"k",1))return stop(false);lua_pop(L,2);
    lua_rawgeti(L,LUA_REGISTRYINDEX,row.primary_ref);lua_rawget(L,buddies);
    lua_rawgeti(L,LUA_REGISTRYINDEX,row.secondary_ref);
    const bool same=lua_iscfunction(L,-1)&&lua_tocfunction(L,-1)==row.secondary&&
        lua_topointer(L,-1)==row.secondary_value&&lua_rawequal(L,-1,-2)&&!lua_getupvalue(L,-1,1);
    return stop(same);
}
// Borrow only the genuine completed constructor+writer+metatype+library HIT.
// The actual615D70 ordinary-consumer guard stays intact; registrar-only
// native_flua_offered_metatype_owned is never called or reopened here.
static void native_postread_tt_manager_push_metatype_owned(lua_State*L,NativePostreadTTManagerReturn&r){
    auto&p=owner(L);native_postread_tt_manager_current_owned(L,&r);
    p.source_exception={};p.source_error=nullptr;
    try{auto*actual=p.native_flua_builtin_registry.get();
        if(!actual)throw ScriptError("TTManager actual static creator registry absent");actual->physical_current_without_callbacks();
        const auto&d=actual->descriptors[18];
        if(!d.constructed||!d.named||d.recipe!=&native_builtin31_recipes[18]||d.recipe->descriptor!=0xca8d30u||
           d.recipe->constructor!=0x68ae60u||d.count544!=4||std::strcmp(d.name0,"ToolTipManager"))
            throw ScriptError("TTManager genuine68AE60 static718B40 creator/name writer did not execute");}
    catch(...){p.source_exception=std::current_exception();p.source_error="TTManager actual initialized creator/descriptor FIRST refused";}
    if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
    // Original718B40's actual615D70 getter can resolve the inherited master
    // cache on selected518. It still enforces its established genuine Source.
    native_ordinary_push_master_types_615d70_owned(L);native_postread_tt_manager_current_owned(L,&r);
    native_postread_tt_manager_key_owned(L,r,4);lua_rawget(L,-2);lua_remove(L,-2);
    if(lua_type(L,-1)!=LUA_TTABLE)
        return (void)native_master_thread_refuse(L,"TTManager actual718B40 metatype MISS requires genuine517C constructor; unissued");
    r.metatype_owner=nullptr;
    for(const auto&actual:p.native_flua_metatype_results)if(actual&&actual->recipe==&native_builtin31_recipes[18]&&
        !actual->is_const&&native_flua_metatype_physical_owned(p,L,*actual,-1)){
        r.metatype_owner=actual.get();break;
    }
    if(!r.metatype_owner)return (void)native_master_thread_refuse(L,"TTManager actual creator cache HIT lacks genuine completed metatype owner");
    r.metatype=lua_topointer(L,-1);const int metatype=lua_gettop(L);
    // The completed reflection keeper was published on the real master. A
    // selected child normally inherits it; child rawget would incorrectly miss.
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.master_globals_ref);
    if(lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r.master_globals)
        return (void)native_master_thread_refuse(L,"TTManager actual master globals keeper changed");
    native_postread_tt_manager_key_owned(L,r,5);lua_rawget(L,-2);lua_remove(L,-2);
    if(lua_type(L,-1)!=LUA_TTABLE)return (void)native_master_thread_refuse(L,"TTManager actual completed master reflection absent");
    const int reflection=lua_gettop(L);r.reflection_owner=nullptr;
    for(const auto&actual:p.native_ordinary_reflection_results)if(actual&&actual->host==&p&&actual->alive.load()&&
        actual->creator==std::this_thread::get_id()&&actual->original==0x5f36d0u&&actual->key_address==0x9e27e4u&&
        actual->stage==NativeOrdinaryReflectionResult::Stage::returned&&actual->table_ref>=0&&actual->key_ref>=0&&
        actual->table==lua_topointer(L,reflection)){
        lua_rawgeti(L,LUA_REGISTRYINDEX,actual->table_ref);
        if(!lua_rawequal(L,-1,reflection))return (void)native_master_thread_refuse(L,"TTManager completed reflection root changed");lua_pop(L,1);
        lua_rawgeti(L,LUA_REGISTRYINDEX,actual->key_ref);std::size_t size=0;
        const char*key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&size):nullptr;
        if(!key||size!=14||std::memcmp(key,"ExposedMembers",14))
            return (void)native_master_thread_refuse(L,"TTManager genuine reflection keeper key changed");lua_pop(L,1);
        r.reflection_owner=actual.get();break;
    }
    if(!r.reflection_owner)return (void)native_master_thread_refuse(L,"TTManager actual master reflection lacks completed keeper");
    native_postread_tt_manager_key_owned(L,r,5);lua_gettable(L,LUA_GLOBALSINDEX);
    native_postread_tt_manager_current_owned(L,&r);
    if(!lua_rawequal(L,-1,reflection))return (void)native_master_thread_refuse(L,"TTManager selected environment resolves another reflection cache");lua_pop(L,1);
    lua_pushvalue(L,metatype);lua_rawget(L,reflection);const int methods=lua_gettop(L);r.library=nullptr;
    if(lua_type(L,methods)!=LUA_TTABLE)return (void)native_master_thread_refuse(L,"TTManager actual ExposedMembers[type] library absent");
    for(const auto&actual:p.native_flua_method_libraries)if(actual&&actual->alive&&actual->returned&&actual->host==&p&&
        actual->creator==std::this_thread::get_id()&&actual->recipe==r.metatype_owner->recipe&&!actual->is_const&&
        actual->metatype==r.metatype&&actual->assigned==4&&actual->table==lua_topointer(L,methods)){
        r.library=actual.get();break; // Historical libraries are not a duplicate-current refusal.
    }
    if(!r.library)return (void)native_master_thread_refuse(L,"TTManager actual current library has no genuine completed return");
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.library->table_ref);
    if(!lua_rawequal(L,-1,methods)||lua_topointer(L,-1)!=r.library->table)
        return (void)native_master_thread_refuse(L,"TTManager authentic current method library root changed");lua_pop(L,1);
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.library->metatype_ref);
    if(!lua_rawequal(L,-1,metatype))return (void)native_master_thread_refuse(L,"TTManager actual current library metatype root changed");lua_pop(L,1);
    for(std::size_t i=0;i<4;++i){const auto&row=r.library->rows[i];const auto&recipe=native_builtin31_method_recipes[147+i];
        if(row.recipe!=&recipe||recipe.descriptor!=0xca8d30u||recipe.ordinal!=i||row.key_ref<0||row.primary_ref<0||!row.primary)
            return (void)native_master_thread_refuse(L,"TTManager actual callable record identity changed");
        lua_rawgeti(L,LUA_REGISTRYINDEX,row.key_ref);std::size_t size=0;
        const char*key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&size):nullptr;
        if(!key||size!=std::strlen(recipe.key)||std::memcmp(key,recipe.key,size))
            return (void)native_master_thread_refuse(L,"TTManager exact Original callable key changed");
        lua_rawget(L,methods);lua_rawgeti(L,LUA_REGISTRYINDEX,row.primary_ref);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=row.primary||lua_topointer(L,-1)!=row.primary_value||
           !lua_rawequal(L,-1,-2)||lua_getupvalue(L,-1,1))
            return (void)native_master_thread_refuse(L,"TTManager genuine zero-upvalue callable publication changed");lua_pop(L,2);
        if(!native_postread_tt_manager_secondary_physical_owned(L,r,*r.library,i))
            return (void)native_master_thread_refuse(L,"TTManager genuine current primary/CodeBuddy secondary association changed");
        native_postread_tt_manager_current_owned(L,&r);
    }
    lua_pop(L,2);native_postread_tt_manager_current_owned(L,&r); // Leave actual metatype only.
}
// World17's private owner getter returns its OUTER acquired actual operand.
// It must not allocate/capture a GUI provider from inside this Lua CFunction.
static void native_postread_tt_manager_registration_owned(lua_State*L,int globals_table,int key_reference){
    auto&p=owner(L);native_postread_load_tail_full_owned(L);native_master_thread_physical(L,p.native_master_thread_bootstrap.get());
    if(globals_table<1||globals_table>lua_gettop(L)||lua_type(L,globals_table)!=LUA_TTABLE||key_reference<0||
       !p.worldview_native_host||p.worldview_native_host->child>=p.child_contexts.size()||
       G(L)!=G(p.state)||!lua_rawequal(L,globals_table,LUA_GLOBALSINDEX))
        return (void)native_master_thread_refuse(L,"TTManager actual4188E0 Table/key/selected518 operands differ");
    if(!lua_checkstack(L,32))return (void)native_master_thread_refuse(L,"TTManager actual reflection stack unavailable");
    const int entry=lua_gettop(L);NativePostreadTTManagerReturn*r=nullptr;
    const auto&child=p.child_contexts[p.worldview_native_host->child];
    if(child.execution_thread<0||child.environment<0)
        return (void)native_master_thread_refuse(L,"TTManager selected518 keeper refs absent");
    lua_rawgeti(L,LUA_REGISTRYINDEX,child.execution_thread);
    if(lua_type(L,-1)!=LUA_TTHREAD||lua_tothread(L,-1)!=L)
        return (void)native_master_thread_refuse(L,"TTManager Table wrapper belongs to another actual selected coroutine");lua_pop(L,1);
    lua_rawgeti(L,LUA_REGISTRYINDEX,child.environment);
    if(!lua_rawequal(L,-1,globals_table))
        return (void)native_master_thread_refuse(L,"TTManager Globals wrapper differs from selected518 environment root");lua_pop(L,1);
    p.source_exception={};p.source_error=nullptr;
    try{const auto&operand=native_postread_load_tail_tt_manager_operand_owned(L);
        if(!operand)throw ScriptError("TTManager actual GUI embedded value operand was not captured OUTER");
        operand->require_current_owned(*p.worldview_native_host->loan);
        auto actual=std::make_unique<NativePostreadTTManagerReturn>(p,L,operand);r=actual.get();
        p.native_postread_tt_manager_returns.push_back(std::move(actual));}
    catch(...){p.source_exception=std::current_exception();p.source_error="TTManager genuine manager publication owner capture refused";}
    if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
    native_postread_load_tail_full_owned(L);native_postread_tt_manager_current_owned(L,r);
    lua_createtable(L,6,0);const char*const keys[]={"CData","TTManager","ExposedTypes","ToolTipManager","ExposedMembers","CodeBuddy"};
    for(int i=0;i<6;++i){lua_pushstring(L,keys[i]);lua_rawseti(L,-2,i+1);native_postread_load_tail_full_owned(L);}
    r->keys=lua_topointer(L,-1);r->keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);r->stage=NativePostreadTTManagerReturn::Stage::keys;
    // PushTable and Reference::Push use the already-earned actual wrappers.
    lua_pushvalue(L,globals_table);lua_rawgeti(L,LUA_REGISTRYINDEX,key_reference);
    std::size_t size=0;const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&size):nullptr;
    if(!key||size!=9||std::memcmp(key,"TTManager",9))
        return (void)native_master_thread_refuse(L,"TTManager actual Reference is nil/invalid or differs from original key");
    // Exact898350: fresh reflected table, genuine OWN embedded CData pointer,
    // initialized actual type creator's completed metatype HIT, not method table.
    lua_createtable(L,0,0);r->table=lua_topointer(L,-1);
    lua_pushlightuserdata(L,const_cast<void*>(r->cdata));native_postread_tt_manager_key_owned(L,*r,1);lua_insert(L,-2);lua_rawset(L,-3);
    lua_pushvalue(L,-1);r->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);r->stage=NativePostreadTTManagerReturn::Stage::reflected;
    native_postread_load_tail_full_owned(L);native_postread_tt_manager_push_metatype_owned(L,*r);
    lua_pushvalue(L,-1);r->metatype_ref=luaL_ref(L,LUA_REGISTRYINDEX);r->stage=NativePostreadTTManagerReturn::Stage::metatype;
    lua_setmetatable(L,-2);native_postread_load_tail_full_owned(L);
    lua_rawset(L,-3);r->stage=NativePostreadTTManagerReturn::Stage::published;
    lua_settop(L,-2); // Original41896A pops pushed Globals; caller owns key dtor.
    if(lua_gettop(L)!=entry)return (void)native_master_thread_refuse(L,"TTManager actual Table::SetField stack did not return");
    native_postread_load_tail_full_owned(L);native_postread_tt_manager_current_owned(L,r);
    native_postread_tt_manager_key_owned(L,*r,2);lua_rawget(L,globals_table);lua_rawgeti(L,LUA_REGISTRYINDEX,r->table_ref);
    if(!lua_rawequal(L,-1,-2)||lua_topointer(L,-1)!=r->table||!lua_getmetatable(L,-1))
        return (void)native_master_thread_refuse(L,"TTManager actual selected reflected publication absent at POST");
    lua_rawgeti(L,LUA_REGISTRYINDEX,r->metatype_ref);
    if(!lua_rawequal(L,-1,-2)||lua_topointer(L,-1)!=r->metatype)
        return (void)native_master_thread_refuse(L,"TTManager actual reflected metatype changed at POST");
    lua_settop(L,entry);native_postread_load_tail_full_owned(L);native_postread_tt_manager_current_owned(L,r);
    r->stage=NativePostreadTTManagerReturn::Stage::returned;
}
// Closed future body seam. Never calls World17's lexical constructor guard.
// Existing four FLua primary endpoints still refuse their unissued backends;
// this publication does not install provider-ready values or a fake registry.
static void native_postread_tt_manager_receiver_current_owned(lua_State*L,int receiver){
    auto&p=owner(L);native_db_query_current(L);const int top=lua_gettop(L);
    if(receiver<0)receiver=top+receiver+1;
    if(receiver<1||receiver>top||lua_type(L,receiver)!=LUA_TTABLE)
        return (void)native_master_thread_refuse(L,"TTManager genuine reflected receiver required");
    NativePostreadTTManagerReturn*r=nullptr;
    for(const auto&item:p.native_postread_tt_manager_returns)if(item&&item->stage==NativePostreadTTManagerReturn::Stage::returned&&
        item->table==lua_topointer(L,receiver)){
        if(r)return (void)native_master_thread_refuse(L,"TTManager receiver publication custody duplicated");r=item.get();
    }
    native_postread_tt_manager_current_owned(L,r);
    lua_rawgeti(L,LUA_REGISTRYINDEX,r->table_ref);
    if(!lua_rawequal(L,-1,receiver)||!lua_getmetatable(L,receiver))
        return (void)native_master_thread_refuse(L,"TTManager actual receiver/root metatype retired");
    lua_rawgeti(L,LUA_REGISTRYINDEX,r->metatype_ref);
    if(!lua_rawequal(L,-1,-2)||lua_topointer(L,-1)!=r->metatype)
        return (void)native_master_thread_refuse(L,"TTManager receiver actual type changed");lua_settop(L,top);
    native_postread_tt_manager_key_owned(L,*r,1);lua_rawget(L,receiver);
    if(lua_type(L,-1)!=LUA_TLIGHTUSERDATA||lua_touserdata(L,-1)!=r->cdata)
        return (void)native_master_thread_refuse(L,"TTManager receiver actual embedded CData changed");lua_settop(L,top);
    lua_rawgeti(L,LUA_REGISTRYINDEX,r->environment_ref);native_postread_tt_manager_key_owned(L,*r,2);lua_rawget(L,-2);
    if(!lua_rawequal(L,-1,receiver)||!lua_rawequal(L,-2,LUA_GLOBALSINDEX))
        return (void)native_master_thread_refuse(L,"TTManager actual selected environment publication changed");lua_settop(L,top);
    native_postread_tt_manager_current_owned(L,r);native_db_query_current(L);
}
void native_postread_tt_manager_close()noexcept{
    for(const auto&r:native_postread_tt_manager_returns)if(r)r->stage=NativePostreadTTManagerReturn::Stage::retired;
    // Private keeper refs remain until the genuine Host lua_close. No public
    // view destructor or normal key-reference dtor erases the live namespace.
}
