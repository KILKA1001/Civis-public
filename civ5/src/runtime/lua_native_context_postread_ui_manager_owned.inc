// Inside Host::Impl AFTER the real FLua supplier and World17 postread tail.
// 4A1900 is Table::SetField(KeyReference, GUI*), not a method factory.
// 898310 publishes CData LIGHTUSERDATA and the actual mutable UIManager type.
struct NativePostreadUiManagerReturn final {
    enum class Stage {captured,keys,table,typed,assigned,returned,retired};
    Impl*const host;const std::thread::id creator=std::this_thread::get_id();
    const std::shared_ptr<const ui::detail::NotificationContextUiManagerValueLoan> value;
    const std::shared_ptr<const ScriptUiChildSource> source;
    lua_State*const master;lua_State*const thread;global_State*const global;
    const Table*const registry_table;const std::size_t child;const int environment_ref,thread_ref;
    const void*const environment;const void*const gui;
    const std::uint64_t epoch,writes,uncertainty;
    Stage stage=Stage::captured;int key_ref=LUA_NOREF,cdata_key_ref=LUA_NOREF,table_ref=LUA_NOREF;
    const void*key=nullptr,*table=nullptr;
    NativeFLuaMetatypeReturn*metatype=nullptr;
    NativePostreadUiManagerReturn(Impl&p,const NativePostreadGlobalsControl&g,
        std::shared_ptr<const ui::detail::NotificationContextUiManagerValueLoan>v,const void*actual_gui)
        :host(&p),value(std::move(v)),source(g.source),master(p.state),thread(g.thread),global(G(p.state)),
         registry_table(hvalue(&G(p.state)->l_registry)),child(g.child),environment_ref(g.environment_reference),
         thread_ref(g.thread_reference),environment(g.environment),gui(actual_gui),
         epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){}
};
std::vector<std::shared_ptr<NativePostreadUiManagerReturn>> native_postread_ui_manager_returns;
void native_postread_ui_manager_first_cpp_owned(const NativePostreadUiManagerReturn&r)const{
    if(r.creator!=std::this_thread::get_id()||r.creator!=ui_thread||!worldview_native_host)
        throw ScriptError("UIManager foreign creator/retired native scope before owner pins");
    worldview_native_host->pure();
    if(r.host!=this||!r.value||
        r.stage==NativePostreadUiManagerReturn::Stage::retired||!state||state!=r.master||G(state)!=r.global||
        !ttistable(&r.global->l_registry)||hvalue(&r.global->l_registry)!=r.registry_table||
        failure_epoch!=r.epoch||failed.writes_!=r.writes||execution_uncertainty!=r.uncertainty||failed.exhausted_||
        !worldview_operation||worldview_native_host->loan!=worldview_operation||
        worldview_native_host->child!=r.child||r.child>=child_contexts.size()||
        !worldview_native_child_controls_current(r.child))
        throw ScriptError("UIManager actual Host/VM/selected Source retired FIRST");
    const auto&c=child_contexts[r.child];
    if(c.retired||c.source!=r.source||c.environment!=r.environment_ref||c.execution_thread!=r.thread_ref||
        c.environment<0||c.execution_thread<0||worldview_native_host->child_source.lock()!=r.source)
        throw ScriptError("UIManager actual retained child/environment/thread changed");
    const auto source=r.value->source_.lock();const auto gui=r.value->gui_.lock();
    if(!source||!gui||gui.get()!=r.gui||source.owner_before(worldview_sources)||worldview_sources.owner_before(source)||
        &source->current_operation_loan()!=worldview_operation||!worldview_host_owner)
        throw ScriptError("UIManager actual typed GUI/Source value replaced");
    worldview_operation->require_same_vm(*worldview_host_owner);
    worldview_operation->require_same_source(*r.source);
    r.value->require_current_owned(*worldview_operation);
    worldview_native_host->pure();
}
void native_postread_ui_manager_creator_cpp_owned()const{
    const auto*actual=native_flua_builtin_registry.get();
    if(!actual)throw ScriptError("UIManager actual FLua descriptor constructor is unissued");
    actual->physical_current_without_callbacks();
    const auto*recipe=native_flua_builtin_recipe_owned(0xcb5668u);
    if(!recipe||recipe->constructor!=0x6a11d0u||recipe->original_callback!=0x6451b0u||
        recipe->name_setter!=0x4212c0u||recipe->method_begin!=151||recipe->method_count!=34||
        recipe->const_method_count||recipe->parent548||recipe->parent54c||
        actual->actual_writers!=std::size(native_flua_builtin_writers)||!actual->actual_readers)
        throw ScriptError("UIManager actual constructor/name/row writer stream incomplete");
    const auto&d=actual->descriptors[static_cast<std::size_t>(recipe-native_builtin31_recipes)];
    if(d.recipe!=recipe||!d.constructed||!d.named||d.count544!=34||d.current548||d.current54c||
        !d.name0||std::strcmp(d.name0,"UIManager")||std::strcmp(d.name4.data(),"const UIManager"))
        throw ScriptError("UIManager genuine initialized creator/descriptor storage changed");
    bool queued=false;
    for(std::size_t i=0;i<actual->queue->count&&i<actual->queue->entries.size();++i){
        const auto*entry=actual->queue->entries[i].get();
        if(actual->callback_recipes[i]==recipe&&entry&&entry->actual_constructor==&d&&
            entry->original_callback==recipe->original_callback&&entry->body==native_flua_builtin_queue_body_owned(recipe->descriptor))queued=true;
    }
    if(!queued)throw ScriptError("UIManager initialized creator lacks its actual constructor callback append");
    for(std::size_t i=0;i<34;++i)if(d.records[i]!=&native_builtin31_method_recipes[151+i]||
        d.records[i]->descriptor!=recipe->descriptor||d.records[i]->ordinal!=i||d.records[i]->constructor_reference!=-1)
        throw ScriptError("UIManager actual mutable method writer/record identity changed");
}
static void native_postread_ui_manager_key_owned(lua_State*L,const NativePostreadUiManagerReturn&r,bool cdata){
    const int ref=cdata?r.cdata_key_ref:r.key_ref;
    if(ref<0)return (void)luaL_error(L,"UIManager real retained TString key is unissued");
    lua_rawgeti(L,LUA_REGISTRYINDEX,ref);
    std::size_t n=0;const char*const s=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
    const char*const expected=cdata?"CData":"UIManager";const auto length=cdata?5u:9u;
    if(!s||n!=length||std::memcmp(s,expected,length)||(!cdata&&s!=r.key))
        (void)luaL_error(L,"UIManager real retained TString key changed");
}
static bool native_postread_ui_manager_library_physical_owned(lua_State*L,const NativeFLuaMethodLibraryReturn&r,
    const NativeFLuaMetatypeReturn&type){
    auto&p=owner(L);const int top=lua_gettop(L);
    const auto stop=[&](bool ok){lua_settop(L,top);return ok;};
    if(!r.alive||!r.returned||r.host!=&p||r.creator!=std::this_thread::get_id()||r.is_const||r.recipe!=type.recipe||
        r.metatype!=type.table||r.assigned!=34||r.table_ref<0||r.metatype_ref<0||!r.table)return false;
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.metatype_ref);
    if(!native_flua_metatype_physical_owned(p,L,type,-1))return stop(false);lua_pop(L,1);
    lua_getfield(L,LUA_GLOBALSINDEX,"ExposedMembers");const int reflection=lua_gettop(L);
    bool issued=false;
    for(const auto&keeper:p.native_ordinary_reflection_results)
        if(keeper&&native_ordinary_reflection_physical_without_callbacks(p,L,*keeper,reflection)){issued=true;break;}
    if(!issued)return stop(false);
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.metatype_ref);lua_rawget(L,reflection);
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.table_ref);
    if(lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r.table||!lua_rawequal(L,-1,-2))return stop(false);
    const int methods=lua_gettop(L);
    for(std::size_t i=0;i<34;++i){
        const auto&row=r.rows[i];
        if(row.recipe!=&native_builtin31_method_recipes[151+i]||!row.primary||!row.secondary||
            row.primary_ref<0||row.secondary_ref<0||row.key_ref<0||!row.codebuddy)return stop(false);
        lua_rawgeti(L,LUA_REGISTRYINDEX,row.key_ref);
        std::size_t n=0;const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!key||n!=std::strlen(row.recipe->key)||std::memcmp(key,row.recipe->key,n))return stop(false);
        lua_rawget(L,methods);lua_rawgeti(L,LUA_REGISTRYINDEX,row.primary_ref);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=row.primary||lua_topointer(L,-1)!=row.primary_value||
            !lua_rawequal(L,-1,-2)||lua_getupvalue(L,-1,1))return stop(false);lua_pop(L,2);
        lua_rawgeti(L,LUA_REGISTRYINDEX,row.codebuddy->table_ref);
        if(!native_ordinary_codebuddy_table_physical_without_callbacks(p,L,*row.codebuddy,-1))return stop(false);
        lua_rawgeti(L,LUA_REGISTRYINDEX,row.primary_ref);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,row.secondary_ref);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=row.secondary||lua_topointer(L,-1)!=row.secondary_value||
            !lua_rawequal(L,-1,-2)||lua_getupvalue(L,-1,1))return stop(false);lua_pop(L,3);
    }
    if(r.rows[2].recipe->original_primary!=0x48e920u||r.rows[2].primary!=&Impl::native_flua_exposure_primary_endpoint_owned<0x48e920u>)
        return stop(false);
    return stop(true);
}
static NativeFLuaMethodLibraryReturn*native_postread_ui_manager_current_library_owned(lua_State*L,
    const NativeFLuaMetatypeReturn&type){
    auto&p=owner(L);
    // Repeated genuine FLua registration can replace ExposedMembers[type].
    // Read that actual current return, never freeze an old library/source frame.
    for(const auto&held:p.native_flua_method_libraries)
        if(held&&native_postread_ui_manager_library_physical_owned(L,*held,type))return held.get();
    return (luaL_error(L,"UIManager current reflection method library has no genuine completed FLua producer"),nullptr);
}
static void native_postread_ui_manager_push_metatype_owned(lua_State*L,NativePostreadUiManagerReturn&r){
    native_postread_load_tail_full_owned(L);
    native_ordinary_push_master_types_615d70_owned(L);native_postread_load_tail_full_owned(L);
    lua_getfield(L,-1,"UIManager");native_postread_load_tail_full_owned(L);
    if(lua_type(L,-1)!=LUA_TTABLE)
        return (void)luaL_error(L,"UIManager4631C0 cache MISS requires actual517C20 producer; completed FLua type absent");
    for(const auto&held:owner(L).native_flua_metatype_results)
        if(held&&held->recipe&&held->recipe->descriptor==0xcb5668u&&!held->is_const&&
            native_flua_metatype_physical_owned(owner(L),L,*held,-1)){r.metatype=held.get();break;}
    if(!r.metatype)return (void)luaL_error(L,"UIManager4631C0 cache HIT lacks actual initialized creator return");
    (void)native_postread_ui_manager_current_library_owned(L,*r.metatype);
    lua_remove(L,-2);native_postread_load_tail_full_owned(L); // Original46321D/46322A HIT return.
}
static bool native_postread_ui_manager_physical_owned(lua_State*L,const NativePostreadUiManagerReturn&r,int environment){
    auto&p=owner(L);const int top=lua_gettop(L);if(environment<0)environment=top+environment+1;
    const auto stop=[&](bool ok){lua_settop(L,top);return ok;};
    if(!r.metatype||r.key_ref<0||r.cdata_key_ref<0||r.table_ref<0||!r.table||
        environment<1||lua_type(L,environment)!=LUA_TTABLE||lua_topointer(L,environment)!=r.environment)return false;
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.environment_ref);if(!lua_rawequal(L,-1,environment))return stop(false);lua_pop(L,1);
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.thread_ref);
    if(lua_type(L,-1)!=LUA_TTHREAD||lua_tothread(L,-1)!=r.thread)return stop(false);lua_pop(L,1);
    lua_rawgeti(L,LUA_REGISTRYINDEX,r.table_ref);const int instance=lua_gettop(L);
    if(lua_type(L,instance)!=LUA_TTABLE||lua_topointer(L,instance)!=r.table)return stop(false);
    native_postread_ui_manager_key_owned(L,r,true);lua_rawget(L,instance);
    if(lua_type(L,-1)!=LUA_TLIGHTUSERDATA||lua_touserdata(L,-1)!=r.gui)return stop(false);lua_pop(L,1);
    if(!lua_getmetatable(L,instance)||!native_flua_metatype_physical_owned(p,L,*r.metatype,-1))return stop(false);lua_pop(L,1);
    native_postread_ui_manager_key_owned(L,r,false);lua_rawget(L,environment);
    if(!lua_rawequal(L,-1,instance))return stop(false);lua_pop(L,1);
    (void)native_postread_ui_manager_current_library_owned(L,*r.metatype);
    return stop(true);
}
static void native_postread_ui_manager_registration_owned(lua_State*L,int globals_table,int key_reference){
    auto&p=owner(L);native_postread_load_tail_full_owned(L);
    native_postread_load_tail_lookup_publication_full_owned(L);
    auto&tail=p.postread_load_tail_frame->control;const int before=lua_gettop(L);
    if(globals_table<0)globals_table=before+globals_table+1;
    if(L!=tail.globals->thread||globals_table<1||lua_type(L,globals_table)!=LUA_TTABLE||
        lua_topointer(L,globals_table)!=tail.globals->globals_pointer||key_reference<0||!tail.ui_manager_value)
        return (void)luaL_error(L,"UIManager4A1900 actual table/key/value/predecessor ownership absent");
    p.source_exception={};p.source_error=nullptr;
    try{
        p.postread_load_tail_frame->full();p.native_postread_ui_manager_creator_cpp_owned();
        tail.ui_manager_value->require_current_owned(p.postread_load_tail_frame->operation);
        const auto gui=tail.ui_manager_value->gui_.lock();
        if(!gui)throw ScriptError("UIManager457652 actual typed GUI singleton retired");
        for(const auto&r:p.native_postread_ui_manager_returns)
            if(r&&r->stage!=NativePostreadUiManagerReturn::Stage::retired&&r->environment==tail.globals->environment)
                throw ScriptError("UIManager actual same Globals occurrence registration replay");
        p.native_postread_ui_manager_returns.push_back(std::make_shared<NativePostreadUiManagerReturn>(p,*tail.globals,tail.ui_manager_value,gui.get()));
        p.native_postread_ui_manager_first_cpp_owned(*p.native_postread_ui_manager_returns.back());
    }catch(...){p.source_exception=std::current_exception();p.source_error="UIManager actual GUI/creator capture refused";}
    if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
    auto*r=p.native_postread_ui_manager_returns.back().get();
    lua_rawgeti(L,LUA_REGISTRYINDEX,key_reference);native_postread_load_tail_full_owned(L);
    std::size_t n=0;const char*const name=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
    if(!name||n!=9||std::memcmp(name,"UIManager",9))return (void)luaL_error(L,"UIManager actual519700 key reference changed");
    r->key=name;lua_pushvalue(L,-1);r->key_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_postread_load_tail_full_owned(L);
    lua_pushliteral(L,"CData");r->cdata_key_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_postread_load_tail_full_owned(L);
    r->stage=NativePostreadUiManagerReturn::Stage::keys;
    (void)lua_checkstack(L,1);native_postread_load_tail_full_owned(L);
    lua_createtable(L,0,0);native_postread_load_tail_full_owned(L);r->table=lua_topointer(L,-1);
    lua_pushvalue(L,-1);r->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_postread_load_tail_full_owned(L);
    r->stage=NativePostreadUiManagerReturn::Stage::table;
    lua_pushlightuserdata(L,const_cast<void*>(r->gui));lua_setfield(L,-2,"CData");native_postread_load_tail_full_owned(L);
    native_postread_ui_manager_push_metatype_owned(L,*r);lua_setmetatable(L,-2);native_postread_load_tail_full_owned(L);
    r->stage=NativePostreadUiManagerReturn::Stage::typed;
    lua_rawset(L,globals_table);native_postread_load_tail_full_owned(L);r->stage=NativePostreadUiManagerReturn::Stage::assigned;
    if(lua_gettop(L)!=before||!native_postread_ui_manager_physical_owned(L,*r,globals_table))
        return (void)luaL_error(L,"UIManager actual typed instance/raw Globals publication POST changed");
    native_postread_load_tail_full_owned(L);r->stage=NativePostreadUiManagerReturn::Stage::returned;
}
static NativePostreadUiManagerReturn*native_postread_ui_manager_current_owned(lua_State*L,int environment){
    auto&p=owner(L);native_db_query_current(L);p.source_exception={};p.source_error=nullptr;
    NativePostreadUiManagerReturn*actual=nullptr;
    try{
        p.worldview_require_callback_origin();const auto child=p.worldview_native_host->child;
        for(const auto&r:p.native_postread_ui_manager_returns)if(r&&r->child==child&&
            r->stage==NativePostreadUiManagerReturn::Stage::returned){p.native_postread_ui_manager_first_cpp_owned(*r);actual=r.get();break;}
        if(!actual)throw ScriptError("UIManager actual current child has no issued postread GUI value");
        p.native_postread_ui_manager_creator_cpp_owned();
    }catch(...){p.source_exception=std::current_exception();p.source_error="UIManager genuine consumer Source/GUI FIRST refused";}
    if(p.source_error){p.record_failure();return (luaL_error(L,"%s",p.source_error),nullptr);}
    if(G(L)!=actual->global||!native_postread_ui_manager_physical_owned(L,*actual,environment)){
        p.record_failure();return (luaL_error(L,"UIManager current instance/environment/metatype changed"),nullptr);}
    native_db_query_current(L);return actual;
}
bool native_postread_ui_manager_screen_query_busy=false;
struct NativePostreadUiManagerScreenReadScope final {
    Impl&p;lua_State*const entry;NativePostreadUiManagerReturn*const issued;
    const WorldViewNativeHostScope*const native;const BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t serial,epoch,writes,uncertainty;const std::size_t graph;
    const bool prior_failed,prior_ready;bool provisional=false,returned=false;
    std::optional<std::array<std::int32_t,2>> dimensions;
    NativePostreadUiManagerScreenReadScope(Impl&h,lua_State*L,NativePostreadUiManagerReturn&r)
        :p(h),entry(L),issued(&r),native(p.worldview_native_host),operation(p.worldview_operation),
         serial(operation?operation->operation_serial():0),epoch(p.failure_epoch),writes(p.failed.writes_),
         uncertainty(p.execution_uncertainty),graph(p.graph_execution_scope),prior_failed(p.failed),prior_ready(p.ready){
        if(p.native_postread_ui_manager_screen_scope||p.native_postread_ui_manager_screen_query_busy||!native||!operation)
            throw ScriptError("UIManager actual644/648 getter lender reentered");
        // Fence is established BEFORE the first GUI/Source FULL provider call.
        // Constructor failure cannot run this destructor, so rollback is local.
        p.native_postread_ui_manager_screen_query_busy=true;
        try{
            current_before_publish();dimensions.emplace(r.value->screen_size_owned(*operation));
            current_before_publish();p.native_postread_ui_manager_screen_scope=this;
        }catch(...){p.native_postread_ui_manager_screen_query_busy=false;throw;}
    }
    void current_before_publish()const{
        p.worldview_require_callback_origin();
        if(!p.native_postread_ui_manager_screen_query_busy||p.worldview_native_host!=native||p.worldview_operation!=operation||operation->operation_serial()!=serial||
            p.failure_epoch!=epoch||p.failed.writes_!=writes||p.execution_uncertainty!=uncertainty||p.failed.exhausted_||
            p.graph_execution_scope!=graph||static_cast<bool>(p.failed)!=(provisional?true:prior_failed)||p.ready!=(provisional?false:prior_ready))
            throw ScriptError("UIManager actual getter Source/window/VM/failure occurrence changed");
        p.native_postread_ui_manager_first_cpp_owned(*issued);
    }
    void current()const{if(p.native_postread_ui_manager_screen_scope!=this)throw ScriptError("UIManager getter lender replaced");current_before_publish();}
    void enter(){current();p.failed.set_loader_sentinel();p.ready=false;provisional=true;current();}
    void accept(){
        current();
        if(!dimensions||lua_gettop(entry)!=2||lua_type(entry,1)!=LUA_TNUMBER||lua_type(entry,2)!=LUA_TNUMBER||
            lua_tonumber(entry,1)!=static_cast<lua_Number>((*dimensions)[0])||lua_tonumber(entry,2)!=static_cast<lua_Number>((*dimensions)[1]))
            throw ScriptError("UIManager48E920 protected integer result publication changed");
        p.ready=prior_ready;if(prior_failed)p.failed.set_loader_sentinel();else p.failed=false;provisional=false;current();returned=true;
    }
    ~NativePostreadUiManagerScreenReadScope(){
        if(p.native_postread_ui_manager_screen_scope!=this||!returned)p.record_failure();
        p.native_postread_ui_manager_screen_scope=nullptr;p.native_postread_ui_manager_screen_query_busy=false;
    }
};
NativePostreadUiManagerScreenReadScope*native_postread_ui_manager_screen_scope=nullptr;
static int native_postread_ui_manager_screen_body_owned(lua_State*L){
    auto&p=owner(L);auto*const f=p.native_postread_ui_manager_screen_scope;
    if(!f||f->entry!=L||!f->dimensions){p.record_failure();return luaL_error(L,"UIManager48E920 requires its actual protected lender");}
    const int before=lua_gettop(L);lua_pushvalue(L,LUA_GLOBALSINDEX);
    if(native_postread_ui_manager_current_owned(L,-1)!=f->issued){p.record_failure();return luaL_error(L,"UIManager getter actual instance occurrence changed");}
    lua_settop(L,before);lua_pushinteger(L,static_cast<lua_Integer>((*f->dimensions)[0]));
    lua_pushinteger(L,static_cast<lua_Integer>((*f->dimensions)[1]));return 2;
}
static int native_postread_ui_manager_screen_size_owned(lua_State*L){
    auto&p=owner(L);native_db_query_current(L);const int args=lua_gettop(L);
    if(!lua_checkstack(L,16)){p.record_failure();return luaL_error(L,"UIManager real getter protected stack unavailable");}
    if(p.native_postread_ui_manager_screen_query_busy||p.native_postread_ui_manager_screen_scope){
        p.record_failure();return luaL_error(L,"UIManager actual screen provider query reentered");}
    // Select only a retained receipt here. No GUI/FULL provider call precedes
    // the outer lender's independent busy fence; inner body proves Lua roots.
    NativePostreadUiManagerReturn*issued=nullptr;
    for(const auto&r:p.native_postread_ui_manager_returns)if(r&&r->child==p.worldview_native_host->child&&
        r->stage==NativePostreadUiManagerReturn::Stage::returned){issued=r.get();break;}
    if(!issued){p.record_failure();return luaL_error(L,"UIManager current child has no actual published GUI value");}
    lua_pushcfunction(L,native_postread_ui_manager_screen_body_owned);lua_insert(L,1);
    int status=0;p.source_exception={};p.source_error=nullptr;
    try{{NativePostreadUiManagerScreenReadScope lender(p,L,*issued);lender.enter();status=lua_pcall(L,args,2,0);if(!status)lender.accept();}}
    catch(...){p.source_exception=std::current_exception();p.source_error="UIManager actual Source/GUI/window protected getter refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}if(status)return lua_error(L);return 2;
}
static bool native_postread_ui_manager_preserve_layout_owned(lua_State*L,int environment){
    auto&p=owner(L);if(!p.worldview_native_host)return false;
    (void)native_postread_ui_manager_current_owned(L,environment);return true;
}
static bool native_postread_ui_manager_adopt_screen_owned(lua_State*L,int environment,std::size_t slot){
    auto&p=owner(L);if(!p.worldview_native_host)return false;
    auto*r=native_postread_ui_manager_current_owned(L,environment);
    if(slot>=p.control_graphs.size()||!p.control_graphs[slot].bound||!p.control_graphs[slot].graph||
        !p.control_graphs[slot].graph->backend_||p.control_graphs[slot].graph->context()!=p.child_contexts[r->child].name||
        p.control_graphs[slot].screen_manager!=LUA_NOREF||p.control_graphs[slot].screen_method!=LUA_NOREF)
        return (luaL_error(L,"UIManager actual graph/selected child adoption changed"),false);
    lua_rawgeti(L,LUA_REGISTRYINDEX,r->table_ref);
    p.control_graphs[slot].screen_manager=luaL_ref(L,LUA_REGISTRYINDEX);
    auto*const library=native_postread_ui_manager_current_library_owned(L,*r->metatype);
    lua_rawgeti(L,LUA_REGISTRYINDEX,library->rows[2].primary_ref);
    p.control_graphs[slot].screen_method=luaL_ref(L,LUA_REGISTRYINDEX);
    (void)native_postread_ui_manager_current_owned(L,environment);return true;
}
static int native_postread_ui_manager_cleanup_owned(lua_State*L){
    auto*p=static_cast<Impl*>(lua_touserdata(L,1));if(!p)return 0;
    for(const auto&r:p->native_postread_ui_manager_returns)if(r){
        for(auto*ref:{&r->table_ref,&r->cdata_key_ref,&r->key_ref})
            if(*ref>=0){luaL_unref(L,LUA_REGISTRYINDEX,*ref);*ref=LUA_NOREF;}
    }
    return 0;
}
void native_postread_ui_manager_close_owned()noexcept{
    for(const auto&r:native_postread_ui_manager_returns)if(r)r->stage=NativePostreadUiManagerReturn::Stage::retired;
    if(!state)return;const int top=lua_gettop(state);const auto previous=calling;calling=false;
    (void)lua_cpcall(state,native_postread_ui_manager_cleanup_owned,this);lua_settop(state,top);calling=previous;
}
