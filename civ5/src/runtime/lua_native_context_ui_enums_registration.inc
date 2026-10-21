// Original protected4F52F0: cached UIEnumsLibrary, actual41CEC0 ctor on MISS,
// then534B50 same prepared518 selection and5D7BD0 lua_next/settable copy.
struct NativeContextUiEnumsFrame final {
    Impl& host;const NativeContextDatabaseNamespacesFrame*const predecessor;
    const std::thread::id creator=std::this_thread::get_id();const std::uint64_t epoch,writes,uncertainty;
    struct Copied {int key_ref,value_ref;};std::vector<Copied> copied;
    int keys_ref=LUA_NOREF;const void* keys=nullptr;
    int master_ref=LUA_NOREF;const void* master=nullptr;
    std::size_t constructed_groups=0,constructed_members=0;Job* actual_job=nullptr;
    explicit NativeContextUiEnumsFrame(Impl& p):host(p),predecessor(p.database_namespaces_frame),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(host.ui_enums_frame||!predecessor)throw ScriptError("UIEnums registrar requires its genuine completed Database predecessor");
        predecessor->first();if(predecessor->assigned!=2)throw ScriptError("UIEnums predecessor did not execute all real Database namespace stores");host.ui_enums_frame=this;
    }
    ~NativeContextUiEnumsFrame(){if(host.ui_enums_frame==this)host.ui_enums_frame=nullptr;}
    void first()const {
        if(host.ui_enums_frame!=this||creator!=std::this_thread::get_id()||!predecessor||host.database_namespaces_frame!=predecessor||
           epoch!=host.failure_epoch||writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty)
            throw ScriptError("UIEnums actual frame/source/failure changed");predecessor->first();
    }
    void full()const {first();predecessor->full();first();}
};
static void native_ui_enums_key(lua_State* L,int i){
    const auto*const f=owner(L).ui_enums_frame;lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,i);lua_remove(L,-2);
}
static void native_ui_enums_full(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.ui_enums_frame;p.source_exception={};p.source_error=nullptr;
    try{if(!f)throw ScriptError("UIEnums has no genuine entered registrar frame");f->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="UIEnums SAME source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    if(!f->predecessor->actual_job)return (void)luaL_error(L,"UIEnums actual Database predecessor Job retired");
    native_database_namespaces_full(L,*f->predecessor->actual_job);
    p.source_exception={};p.source_error=nullptr;
    try{if(p.ui_enums_frame!=f)throw ScriptError("UIEnums frame replaced after callbacks");f->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="UIEnums FIRST after callbacks refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    if(f->actual_job&&f->actual_job!=&job)return (void)luaL_error(L,"UIEnums actual entered Job changed");
    const int top=lua_gettop(L);
    if(f->keys_ref!=LUA_NOREF){
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->keys||lua_objlen(L,-1)!=408||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"UIEnums retained authentic keys table changed");
        for(int i=0;i<408;++i){
            const char*const expected=i==0?"UIEnumsLibrary":i<=20?native_ui_enum_groups[i-1].name:native_ui_enum_entries[i-21].name;
            lua_rawgeti(L,-1,i+1);std::size_t n=0;const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
            if(!key||n!=std::strlen(expected)||std::memcmp(key,expected,n))return (void)luaL_error(L,"UIEnums authentic retained key TString changed");lua_pop(L,1);
        }lua_pop(L,1);
    }
    if(f->master_ref>=0){
        native_ui_enums_key(L,1);lua_rawget(L,LUA_GLOBALSINDEX);lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->master||!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"UIEnums actual master cache/ref changed");lua_pop(L,2);
    }
    for(const auto& copy:f->copied){
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);lua_rawgeti(L,LUA_REGISTRYINDEX,copy.key_ref);lua_rawget(L,-2);
        lua_rawgeti(L,LUA_REGISTRYINDEX,copy.value_ref);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"UIEnums actual source field changed after copy");lua_pop(L,3);
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->predecessor->predecessor->predecessor->predecessor->environment);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->predecessor->predecessor->predecessor->predecessor->actual_environment||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"UIEnums actual prepared518 destination changed");
        lua_rawgeti(L,LUA_REGISTRYINDEX,copy.key_ref);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,copy.value_ref);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"UIEnums actual selected copied field changed");lua_pop(L,3);
    }
    lua_settop(L,top);
}
static void native_ui_enums_construct(lua_State* L,Job& job,int library){
    auto& p=owner(L);auto*const f=p.ui_enums_frame;native_ui_enums_full(L,job);
    if(!f||f->constructed_groups||f->constructed_members||!lua_istable(L,library))
        return (void)luaL_error(L,"UIEnums original41CEC0 constructor replay/input refused");
    const void*const library_value=lua_topointer(L,library);
    for(std::size_t g=0;g<std::size(native_ui_enum_groups);++g){
        const auto& group=native_ui_enum_groups[g];
        native_ui_enums_key(L,static_cast<int>(g+2));lua_gettable(L,library);native_ui_enums_full(L,job);
        if(lua_isnil(L,-1)){
            lua_pop(L,1);lua_createtable(L,0,0);native_ui_enums_full(L,job);
            native_ui_enums_key(L,static_cast<int>(g+2));lua_pushvalue(L,-2);lua_settable(L,library);native_ui_enums_full(L,job);
        }
        const int table=lua_gettop(L);const void*const table_value=lua_topointer(L,table);
        if(!lua_istable(L,table))return (void)luaL_error(L,"UIEnums original group is not a table");
        for(std::size_t i=0;i<group.count;++i){
            const auto index=group.begin+i;const auto& item=native_ui_enum_entries[index];
            lua_pushinteger(L,item.value);native_ui_enums_full(L,job);
            native_ui_enums_key(L,static_cast<int>(index+22));lua_insert(L,-2);lua_settable(L,table);
            ++f->constructed_members;native_ui_enums_full(L,job);
        }
        if(lua_topointer(L,library)!=library_value||!lua_istable(L,table)||lua_topointer(L,table)!=table_value)
            return (void)luaL_error(L,"UIEnums original group/library changed after field allocation");
        for(std::size_t i=0;i<group.count;++i){const auto index=group.begin+i;
            native_ui_enums_key(L,static_cast<int>(index+22));lua_rawget(L,table);
            if(lua_type(L,-1)!=LUA_TNUMBER||lua_tonumber(L,-1)!=static_cast<lua_Number>(native_ui_enum_entries[index].value))
                return (void)luaL_error(L,"UIEnums authentic immediate field changed after FULL");lua_pop(L,1);}
        // Actual41CEC0 calls lua_checkstack(L,1) before its final group pop.
        if(g+1==std::size(native_ui_enum_groups))(void)lua_checkstack(L,1);
        lua_pop(L,1);++f->constructed_groups;native_ui_enums_full(L,job);
    }
    if(f->constructed_groups!=20||f->constructed_members!=387)
        return (void)luaL_error(L,"UIEnums original41CEC0 mandatory immediate registrations incomplete");
}
static void perform_native_context_ui_enums_registration(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.ui_enums_frame;native_ui_enums_full(L,job);
    if(!f||f->actual_job||f->keys_ref!=LUA_NOREF||f->master_ref!=LUA_NOREF||!lua_checkstack(L,32))
        return (void)luaL_error(L,"UIEnums original4F52F0 registrar replay/stack refused");f->actual_job=&job;
    lua_createtable(L,408,0);const int keys=lua_gettop(L);const void*const key_value=lua_topointer(L,keys);native_ui_enums_full(L,job);
    lua_pushliteral(L,"UIEnumsLibrary");lua_rawseti(L,keys,1);native_ui_enums_full(L,job);
    for(std::size_t i=0;i<std::size(native_ui_enum_groups);++i){lua_pushstring(L,native_ui_enum_groups[i].name);lua_rawseti(L,keys,static_cast<int>(i+2));native_ui_enums_full(L,job);}
    for(std::size_t i=0;i<std::size(native_ui_enum_entries);++i){lua_pushstring(L,native_ui_enum_entries[i].name);lua_rawseti(L,keys,static_cast<int>(i+22));native_ui_enums_full(L,job);}
    lua_pushvalue(L,keys);f->keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);f->keys=key_value;
    if(f->keys_ref<0)return (void)luaL_error(L,"UIEnums actual retained key allocation failed");native_ui_enums_full(L,job);lua_pop(L,1);
    native_ui_enums_key(L,1);lua_gettable(L,LUA_GLOBALSINDEX);native_ui_enums_full(L,job);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);lua_createtable(L,0,0);native_ui_enums_full(L,job);
        native_ui_enums_key(L,1);lua_pushvalue(L,-2);lua_settable(L,LUA_GLOBALSINDEX);native_ui_enums_full(L,job);
        native_ui_enums_construct(L,job,lua_gettop(L));
    }
    if(!lua_istable(L,-1))return (void)luaL_error(L,"UIEnums original master library is not a table");
    const int library=lua_gettop(L);f->master=lua_topointer(L,library);lua_pushvalue(L,library);f->master_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_ui_enums_full(L,job);
    perform_native_context_state_query(L,job);native_ui_enums_full(L,job);const int destination=lua_gettop(L);
    if(lua_topointer(L,destination)!=f->predecessor->predecessor->predecessor->predecessor->actual_environment)
        return (void)luaL_error(L,"UIEnums original same534B50 prepared518 selection changed");
    lua_pushnil(L);
    while(lua_next(L,library)){
        native_ui_enums_full(L,job);
        // Original5D7BD0 duplicates the key/value and stores to the selected
        // destination. Registry refs retain the actual observed data for POST.
        lua_pushvalue(L,-2);const int key_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_ui_enums_full(L,job);
        lua_pushvalue(L,-1);const int value_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_ui_enums_full(L,job);
        if(key_ref<0||value_ref<0)return (void)luaL_error(L,"UIEnums actual observed field retention failed");
        lua_pushvalue(L,-2);lua_pushvalue(L,-2);lua_settable(L,destination);native_ui_enums_full(L,job);
        p.source_exception={};p.source_error=nullptr;
        try{f->copied.push_back({key_ref,value_ref});}catch(...){p.source_exception=std::current_exception();p.source_error="UIEnums actual field-return storage failed";}
        if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
        native_ui_enums_full(L,job);lua_pop(L,1);
    }
    native_ui_enums_full(L,job);job.context_ui_enums_reference=f->master_ref;job.context_ui_enums_table=f->master;
    job.context_ui_enums_copied=f->copied.size();lua_pop(L,2);
    native_ui_enums_full(L,job);worldview_capture_ui_enum_publications_owned(L,job);native_ui_enums_full(L,job);
}
