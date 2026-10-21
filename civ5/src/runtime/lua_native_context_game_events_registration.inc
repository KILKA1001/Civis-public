// Original7AD340/7ACBD0 and complete690B60/754B30/55F2E0.
// GameEvent lists are SAME values consumed by the retained runtime dispatcher.
struct NativeContextGameEventsFrame final {
    Impl& host;const NativeContextPreGameFrame*const predecessor;
    const std::shared_ptr<BnwCivisApplicationStateOwner> captured_app_owner;
    const BnwCivisApplicationStateOwner::CapturedState captured_state;
    const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;
    const std::size_t child;const int environment_ref,thread_ref;
    const void*const actual_environment;lua_State*const selected_thread;
    Job* actual_job=nullptr;int table_ref=LUA_NOREF;const void*table=nullptr;
    int protected_status=-1;bool callback_returned=false,returned=false;
    NativeContextGameEventsFrame(Impl&p,std::shared_ptr<BnwCivisApplicationStateOwner> app,
        const BnwCivisApplicationStateOwner::CapturedState& captured)
      :host(p),predecessor(p.pregame_frame),captured_app_owner(std::move(app)),captured_state(captured),
       epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty),
       child(predecessor?predecessor->child:0),environment_ref(predecessor?predecessor->environment_ref:LUA_NOREF),
       thread_ref(predecessor?predecessor->thread_ref:LUA_NOREF),
       actual_environment(predecessor?predecessor->actual_environment:nullptr),
       selected_thread(predecessor?predecessor->selected_thread:nullptr){
        if(p.game_events_frame||!predecessor||!captured_app_owner)
            throw ScriptError("GameEvents requires actual APP capture and complete PreGame");
        predecessor->first();
        if(!predecessor->returned||!predecessor->callback_returned||predecessor->fields_created!=104||
           predecessor->protected_status||!predecessor->actual_job||predecessor->table_ref<0||!predecessor->table)
            throw ScriptError("GameEvents actual PreGame normal return absent");
        const auto current_app=p.worldview_native_host->loan->context_application_state_owned();
        if(!current_app->game_context_gate_owned(captured_state))
            throw ScriptError("GameEvents actual current APP aliases do not take original branch");
        p.game_events_frame=this;
    }
    ~NativeContextGameEventsFrame(){if(host.game_events_frame==this)host.game_events_frame=nullptr;}
    void first()const{
        if(host.game_events_frame!=this||creator!=std::this_thread::get_id()||creator!=host.ui_thread||
           host.pregame_frame!=predecessor||epoch!=host.failure_epoch||writes!=host.failed.writes_||
           uncertainty!=host.execution_uncertainty||host.failed.exhausted_)
            throw ScriptError("GameEvents actual APP/Source/frame changed");
        predecessor->first();
    }
    void full()const{first();predecessor->full();first();}
};
struct NativeGameEventsStorage final {
    Impl*host=nullptr;lua_State*master=nullptr;global_State*global=nullptr;
    int factory_ref=LUA_NOREF,groups_ref=LUA_NOREF,index_ref=LUA_NOREF;
    const void*factory=nullptr;const void*groups=nullptr;const void*index=nullptr;
};
static void native_context_game_events_registration_full(lua_State*L,Job&job){
    auto&p=owner(L);auto*const f=p.game_events_frame;p.source_exception={};p.source_error=nullptr;
    try{if(!f)throw ScriptError("GameEvents registrar frame absent");f->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="GameEvents actual Source FULL refused";}
    if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
    if(p.game_events_frame!=f||!f->actual_job||f->actual_job!=&job||job.owner!=&p||
       job.operation!=Operation::native_context_game_events_registration||!f->predecessor->actual_job)
        return (void)native_master_thread_refuse(L,"GameEvents actual Job/PreGame changed");
    native_context_pregame_registration_full(L,*f->predecessor->actual_job);
    if(p.game_events_frame!=f)return (void)native_master_thread_refuse(L,"GameEvents frame changed after PreGame FULL");
    p.source_exception={};p.source_error=nullptr;
    try{f->first();}catch(...){p.source_exception=std::current_exception();p.source_error="GameEvents FIRST after callbacks refused";}
    if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
    const auto saved=native_master_thread_callback_first(L);native_master_thread_callback_post(L,saved);
}
static NativeGameEventsStorage* native_game_events_storage_current(lua_State*L){
    auto&p=owner(L);const auto saved=native_master_thread_callback_first(L);
    auto*const s=p.native_game_events_storage.get();
    if(!s||s->host!=&p||s->master!=p.state||s->global!=G(L)||s->global!=G(p.state)||
       s->factory_ref<0||s->groups_ref<0||s->index_ref<0)
        return (native_master_thread_refuse(L,"GameEvents genuine VM factory owner absent"),nullptr);
    const int top=lua_gettop(L);lua_rawgeti(L,LUA_REGISTRYINDEX,s->factory_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=s->factory)
        return (native_master_thread_refuse(L,"GameEvents factory real keeper changed"),nullptr);
    lua_getfield(L,-1,"__index");lua_rawgeti(L,LUA_REGISTRYINDEX,s->index_ref);
    if(!lua_rawequal(L,-1,-2)||lua_topointer(L,-1)!=s->index)
        return (native_master_thread_refuse(L,"GameEvents genuine factory closure changed"),nullptr);
    lua_rawgeti(L,LUA_REGISTRYINDEX,s->groups_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=s->groups)
        return (native_master_thread_refuse(L,"GameEvents actual one-upvalue group cache changed"),nullptr);
    lua_settop(L,top);native_master_thread_callback_post(L,saved);return s;
}
static void native_game_events_function(lua_State*L,const char*name){
    auto&p=owner(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.functions);lua_getfield(L,-1,name);lua_remove(L,-2);
    if(!lua_isfunction(L,-1))native_master_thread_refuse(L,"GameEvents canonical retained runtime bridge absent");
}
static void native_game_events_group_name(lua_State*L,int group){
    auto*s=native_game_events_storage_current(L);const int top=lua_gettop(L);
    if(group<0)group=top+group+1;
    lua_rawgeti(L,LUA_REGISTRYINDEX,s->groups_ref);const int cache=lua_gettop(L);lua_pushnil(L);
    while(lua_next(L,cache)){
        if(lua_rawequal(L,group,-1)){lua_pop(L,1);lua_remove(L,cache);return;}
        lua_pop(L,1);
    }
    lua_settop(L,top);native_master_thread_refuse(L,"GameEvents handler has no actual named group");
}
static lua_Integer native_game_events_journal(lua_State*L,int phase,int op,lua_Integer action,std::size_t removed){
    native_game_events_function(L,"native_game_event_journal");
    lua_pushvalue(L,lua_upvalueindex(1));lua_pushinteger(L,phase);lua_pushinteger(L,op);
    lua_pushinteger(L,action);lua_pushnumber(L,static_cast<lua_Number>(removed));lua_call(L,5,1);
    const lua_Integer value=lua_tointeger(L,-1);lua_pop(L,1);return value;
}
static int native_game_events_add_6a0bf0(lua_State*L){
    const auto saved=native_master_thread_callback_first(L);const int args=lua_gettop(L);
    const lua_Integer action=native_game_events_journal(L,0,3,0,0);native_master_thread_callback_post(L,saved);
    if(args)lua_pushvalue(L,args);else lua_pushnil(L);
    lua_pushvalue(L,-1);lua_rawseti(L,lua_upvalueindex(1),static_cast<int>(lua_objlen(L,lua_upvalueindex(1))+1));
    (void)native_game_events_journal(L,1,3,action,0);native_master_thread_callback_post(L,saved);return 1;
}
static int native_game_events_count_4e1630(lua_State*L){
    const auto saved=native_master_thread_callback_first(L);
    lua_pushnumber(L,static_cast<lua_Number>(lua_objlen(L,lua_upvalueindex(1))));
    native_master_thread_callback_post(L,saved);return 1;
}
static int native_game_events_remove_4240f0(lua_State*L){
    const auto saved=native_master_thread_callback_first(L);const int args=lua_gettop(L);
    const lua_Integer action=native_game_events_journal(L,0,4,0,0);native_master_thread_callback_post(L,saved);
    if(args)lua_pushvalue(L,args);else lua_pushnil(L);const int value=lua_gettop(L);
    const int n=static_cast<int>(lua_objlen(L,lua_upvalueindex(1)));int i=0;std::size_t removed=0;
    lua_pushnil(L);
    do{
        if(lua_rawequal(L,-1,value)){
            if(i>0&&!lua_isnil(L,-1))++removed;
            for(int j=i;j<=n;++j){lua_rawgeti(L,lua_upvalueindex(1),j+1);lua_rawseti(L,lua_upvalueindex(1),j);}
        }
        lua_pop(L,1);++i;lua_rawgeti(L,lua_upvalueindex(1),i);
    }while(!lua_isnil(L,-1));
    lua_pop(L,2);(void)native_game_events_journal(L,1,4,action,removed);
    native_master_thread_callback_post(L,saved);return 0;
}
static int native_game_events_remove_all_761770(lua_State*L){
    const auto saved=native_master_thread_callback_first(L);const lua_Integer action=native_game_events_journal(L,0,5,0,0);native_master_thread_callback_post(L,saved);
    const int n=static_cast<int>(lua_objlen(L,lua_upvalueindex(1)));std::size_t removed=0;
    for(int i=1;i<=n;++i){lua_rawgeti(L,lua_upvalueindex(1),i);if(!lua_isnil(L,-1))++removed;lua_pop(L,1);
        lua_pushnil(L);lua_rawseti(L,lua_upvalueindex(1),i);}
    (void)native_game_events_journal(L,1,5,action,removed);native_master_thread_callback_post(L,saved);return 0;
}
template<int Mode>static int native_game_events_dispatch(lua_State*L){
    const auto saved=native_master_thread_callback_first(L);const int args=lua_gettop(L);
    static constexpr const char* modes[]={"Call","Accumulate","AccumulateINT","TestAny","TestAll"};
    native_game_events_function(L,"dispatch_owned_nested");lua_insert(L,1);
    lua_pushstring(L,modes[Mode]);lua_insert(L,2);
    lua_pushvalue(L,lua_upvalueindex(1));native_game_events_group_name(L,-1);lua_remove(L,-2);lua_insert(L,3);
    lua_call(L,args+2,2);lua_pop(L,1);native_master_thread_callback_post(L,saved);return 1;
}
static int native_game_events_constructor_55f2e0(lua_State*L){
    auto&p=owner(L);const auto saved=native_master_thread_callback_first(L);
    auto*const s=native_game_events_storage_current(L);
    if(lua_topointer(L,lua_upvalueindex(1))==nullptr)return native_master_thread_refuse(L,"GameEvents constructor actual group absent");
    lua_createtable(L,0,0);native_master_thread_callback_post(L,saved);const int event=lua_gettop(L);
    lua_createtable(L,0,0);native_master_thread_callback_post(L,saved);const int list=lua_gettop(L);
    lua_pushvalue(L,list);lua_rawseti(L,lua_upvalueindex(1),static_cast<int>(lua_objlen(L,lua_upvalueindex(1))+1));
    // Functional adapter joins the actual named weak group VALUE to the same
    // existing engine emit dispatcher; it creates no second event/list copy.
    native_game_events_function(L,"native_game_event_join");
    if(!p.worldview_native_host||p.worldview_native_host->child>=p.child_contexts.size())
        return native_master_thread_refuse(L,"GameEvents construction actual selected context absent");
    const auto& context=p.child_contexts[p.worldview_native_host->child].name;
    lua_pushlstring(L,context.data(),context.size());lua_pushvalue(L,lua_upvalueindex(1));
    native_game_events_group_name(L,-1);lua_remove(L,-2);
    lua_pushvalue(L,lua_upvalueindex(1));lua_pushvalue(L,list);lua_call(L,4,0);native_master_thread_callback_post(L,saved);
    struct Entry{const char*name;lua_CFunction body;bool group;};
    static constexpr Entry methods[]={
      {"Add",native_game_events_add_6a0bf0,false},{"Count",native_game_events_count_4e1630,false},
      {"Remove",native_game_events_remove_4240f0,false},{"RemoveAll",native_game_events_remove_all_761770,false},
      {"Call",native_game_events_dispatch<0>,true},{"Accumulate",native_game_events_dispatch<1>,true},
      {"AccumulateINT",native_game_events_dispatch<2>,true},{"TestAny",native_game_events_dispatch<3>,true},
      {"TestAll",native_game_events_dispatch<4>,true}};
    for(const auto& entry:methods){lua_pushstring(L,entry.name);lua_pushvalue(L,entry.group?lua_upvalueindex(1):list);
        lua_pushcclosure(L,entry.body,1);lua_rawset(L,event);native_master_thread_callback_post(L,saved);}
    lua_pop(L,1);lua_createtable(L,3,0);native_master_thread_callback_post(L,saved);lua_pushliteral(L,"__newindex");lua_pushnil(L);lua_rawset(L,-3);
    lua_pushliteral(L,"__metatable");lua_createtable(L,0,0);native_master_thread_callback_post(L,saved);lua_rawset(L,-3);lua_setmetatable(L,event);
    if(p.native_game_events_storage.get()!=s)return native_master_thread_refuse(L,"GameEvents storage replaced during constructor");
    native_master_thread_callback_post(L,saved);return 1;
}
static int native_game_events_index_754b30(lua_State*L){
    const auto saved=native_master_thread_callback_first(L);auto*s=native_game_events_storage_current(L);
    if(lua_topointer(L,lua_upvalueindex(1))!=s->groups)return native_master_thread_refuse(L,"GameEvents index actual one cache upvalue changed");
    lua_pushvalue(L,2);lua_rawget(L,lua_upvalueindex(1));
    if(lua_isnil(L,-1)){
        lua_pop(L,1);lua_createtable(L,0,0);native_master_thread_callback_post(L,saved);lua_createtable(L,1,0);native_master_thread_callback_post(L,saved);
        lua_pushliteral(L,"__mode");lua_pushliteral(L,"v");lua_rawset(L,-3);lua_setmetatable(L,-2);
        lua_pushvalue(L,2);lua_pushvalue(L,-2);lua_rawset(L,lua_upvalueindex(1));
    }
    lua_pushcclosure(L,native_game_events_constructor_55f2e0,1);native_master_thread_callback_post(L,saved);lua_call(L,0,1);native_master_thread_callback_post(L,saved);
    lua_pushvalue(L,2);lua_pushvalue(L,-2);lua_rawset(L,1);
    native_master_thread_callback_post(L,saved);return 1;
}
static void native_game_events_factory_690b60(lua_State*L,Job&job){
    auto&p=owner(L);native_context_game_events_registration_full(L,job);
    lua_getfield(L,LUA_GLOBALSINDEX,"GameEventFactory");
    if(lua_isnil(L,-1)){
        lua_pop(L,1);
        if(p.native_game_events_storage)return (void)native_master_thread_refuse(L,"GameEvents partial factory replay refused");
        p.source_exception={};p.source_error=nullptr;
        try{p.native_game_events_storage=std::make_unique<NativeGameEventsStorage>();}
        catch(...){p.source_exception=std::current_exception();p.source_error="GameEvents real factory storage allocation refused";}
        if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
        auto*s=p.native_game_events_storage.get();
        s->host=&p;s->master=p.state;s->global=G(L);
        lua_createtable(L,0,0);s->factory=lua_topointer(L,-1);lua_pushvalue(L,-1);s->factory_ref=luaL_ref(L,LUA_REGISTRYINDEX);
        lua_pushvalue(L,-1);lua_setfield(L,LUA_GLOBALSINDEX,"GameEventFactory");
        lua_createtable(L,0,0);lua_pushvalue(L,-2);lua_setmetatable(L,-2);lua_setfield(L,LUA_GLOBALSINDEX,"GameEvents");
        lua_pushliteral(L,"__index");lua_createtable(L,0,0);s->groups=lua_topointer(L,-1);
        lua_pushvalue(L,-1);s->groups_ref=luaL_ref(L,LUA_REGISTRYINDEX);
        lua_pushcclosure(L,native_game_events_index_754b30,1);s->index=lua_topointer(L,-1);
        lua_pushvalue(L,-1);s->index_ref=luaL_ref(L,LUA_REGISTRYINDEX);lua_rawset(L,-3);
    }else{
        auto*s=native_game_events_storage_current(L);
        if(lua_topointer(L,-1)!=s->factory)return (void)native_master_thread_refuse(L,"GameEvents cache HIT is not actual factory");
    }
    luaL_checktype(L,-1,LUA_TTABLE);native_context_game_events_registration_full(L,job);
}
static int native_game_events_selected_callback_7acbd0(lua_State*L){
    auto&p=owner(L);auto*const f=p.game_events_frame;
    if(!f||L!=p.state||lua_touserdata(L,1)!=f->selected_thread)
        return native_master_thread_refuse(L,"GameEvents real master callback selected userdata changed");
    native_game_events_factory_690b60(L,*f->actual_job);
    native_master_find_thread_environment_534b50_owned(L,f->selected_thread);
    const int environment=lua_gettop(L);
    if(lua_topointer(L,environment)!=f->actual_environment)return native_master_thread_refuse(L,"GameEvents534B50 actual selected environment changed");
    lua_createtable(L,0,0);const int table=lua_gettop(L);const void*const value=lua_topointer(L,table);
    lua_pushvalue(L,-4);lua_setmetatable(L,table);
    lua_pushvalue(L,table);f->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);f->table=value;
    lua_setfield(L,environment,"GameEvents");
    native_context_game_events_registration_full(L,*f->actual_job);f->callback_returned=true;return 0;
}
static void perform_native_context_game_events_registration(lua_State*L,Job&job){
    auto&p=owner(L);auto*const f=p.game_events_frame;
    if(!f||f->actual_job||f->returned||L!=p.state||job.owner!=&p||job.operation!=Operation::native_context_game_events_registration)
        return (void)native_master_thread_refuse(L,"GameEvents7AD340 actual master/frame replay refused");
    f->actual_job=&job;native_context_game_events_registration_full(L,job);
    auto*const boundary=p.native_master_protected_boundary;native_master_protected_boundary_current(L,boundary);
    if(boundary->held_locks==UINT64_MAX)return (void)native_master_thread_refuse(L,"GameEvents recursive original lock exhausted");
    ::EnterCriticalSection(&boundary->critical->actual);++boundary->held_locks;
    f->protected_status=lua_cpcall(p.state,native_game_events_selected_callback_7acbd0,f->selected_thread);
    ::LeaveCriticalSection(&boundary->critical->actual);--boundary->held_locks;
    if(f->protected_status)return (void)lua_error(L);
    native_context_game_events_registration_full(L,job);
    if(!f->callback_returned||f->table_ref<0||!f->table)return (void)native_master_thread_refuse(L,"GameEvents callback did not return its real proxy");
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->environment_ref);lua_getfield(L,-1,"GameEvents");
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->table_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->table||!lua_rawequal(L,-1,-2)||!lua_getmetatable(L,-1)||
       lua_topointer(L,-1)!=p.native_game_events_storage->factory)
        return (void)native_master_thread_refuse(L,"GameEvents selected actual proxy/metatable differs");
    lua_pop(L,4);f->returned=true;
    job.context_game_events_reference=f->table_ref;job.context_game_events_table=f->table;
    job.context_game_events_factory_reference=p.native_game_events_storage->factory_ref;
    job.context_game_events_factory=p.native_game_events_storage->factory;
}
