// Native50FE70 installs real zero-upvalue consumers without invoking them.
// An issued callback must come from the corresponding genuine backend owner.
// This prefix has no such issuer; the closed binding cannot be constructed.
// Presence checks below never synthesize a ContentManager result or success.
class NativeContentBackendBinding final {
    NativeContentBackendBinding()=delete;
    NativeContentBackendBinding(const NativeContentBackendBinding&)=delete;
    NativeContentBackendBinding& operator=(const NativeContentBackendBinding&)=delete;
    Impl* actual_host=nullptr;lua_State* actual_vm=nullptr;
    std::uint32_t original_function=0;lua_CFunction actual_callback=nullptr;
    std::weak_ptr<EndTurnQueue::VmLifetime> lifetime;bool retired=false;
    friend struct Impl;
public:~NativeContentBackendBinding()=default;
};
template<std::uint32_t NativeFunction> static int native_content_method(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);
    NativeContentBackendBinding* binding=nullptr;
    for(const auto& issued:p.native_content_backend_bindings)
        if(issued&&issued->original_function==NativeFunction){if(binding)return luaL_error(L,"ContentManager actual backend binding duplicated");binding=issued.get();}
    if(!binding){p.record_failure();return luaL_error(L,"ContentManager native consumer %08X requires its genuine backend; provider unissued",static_cast<unsigned>(NativeFunction));}
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    p.source_exception={};p.source_error=nullptr;
    lua_CFunction callback=nullptr;lua_State* actual_vm=nullptr;const EndTurnQueue::VmLifetime* actual_lifetime=nullptr;
    try{
        p.worldview_require_callback_origin();
        bool owned=false;for(const auto& issued:p.native_content_backend_bindings)if(issued.get()==binding){owned=true;break;}
        if(!owned)throw ScriptError("ContentManager binding retired during source FIRST");
        const auto life=binding->lifetime.lock();
        if(binding->retired||binding->original_function!=NativeFunction||binding->actual_host!=&p||binding->actual_vm!=p.state||!binding->actual_callback||
           !life||life.get()!=p.end_turn_lifetime.get()||life.owner_before(p.end_turn_lifetime)||
           p.end_turn_lifetime.owner_before(life)||!life->alive||life->failed||life->vm!=p.state||life->host!=p.worldview_host_owner)
            throw ScriptError("ContentManager actual backend/facade custody retired");
        callback=binding->actual_callback;actual_vm=binding->actual_vm;actual_lifetime=life.get();
    }catch(...){p.source_exception=std::current_exception();p.source_error="ContentManager genuine backend FIRST refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    // No owning C++ local or speculative argument/result conversion crosses
    // this real Lua CFunction invocation. A future genuine issuer must supply
    // the complete original method body and its real backend custody.
    const int count=callback(L);native_db_query_current(L);
    p.source_exception={};p.source_error=nullptr;
    try{
        p.worldview_require_callback_origin();
        bool owned=false;for(const auto& issued:p.native_content_backend_bindings)if(issued.get()==binding){owned=true;break;}
        if(!owned)throw ScriptError("ContentManager binding retired during callback/source POST");
        const auto life=binding->lifetime.lock();
        if(binding->retired||binding->original_function!=NativeFunction||binding->actual_host!=&p||
           binding->actual_vm!=actual_vm||actual_vm!=p.state||binding->actual_callback!=callback||
           !life||life.get()!=actual_lifetime||life.get()!=p.end_turn_lifetime.get()||
           life.owner_before(p.end_turn_lifetime)||p.end_turn_lifetime.owner_before(life)||!life->alive||life->failed||
           life->vm!=actual_vm||life->host!=p.worldview_host_owner||epoch!=p.failure_epoch||
           writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty)
            throw ScriptError("ContentManager genuine callback/VM/weak lifetime/source changed");
    }catch(...){p.source_exception=std::current_exception();p.source_error="ContentManager genuine callback/source POST refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    if(count<0||count>lua_gettop(L)){p.record_failure();return luaL_error(L,"ContentManager actual callback returned invalid result count");}
    return count;
}
struct NativeContentEntry {const char* name;std::uint32_t original_function;lua_CFunction function;};
static constexpr NativeContentEntry native_content_entries[]={
    {"IsActive",0x594220,native_content_method<0x594220>}, // 0x50fe94
    {"SetActive",0x5c5db0,native_content_method<0x5c5db0>}, // 0x50feae
    {"IsInstalled",0x5f6150,native_content_method<0x5f6150>}, // 0x50fecb
    {"GetAllPackageIDs",0x6e8ea0,native_content_method<0x6e8ea0>}, // 0x50fee5
    {"GetPackageDescription",0x51fdb0,native_content_method<0x51fdb0>}, // 0x50feff
    {"IsUpgrade",0x6463c0,native_content_method<0x6463c0>}, // 0x50ff1c
};
static constexpr const char*const native_content_retained_keys[]={"ContentManager","IsActive","SetActive","IsInstalled","GetAllPackageIDs","GetPackageDescription","IsUpgrade","ContentManagerEnums","ContentType","GAMEPLAY","UISKIN","COMMON"};
static constexpr lua_Integer native_content_enum_values[]={1,2,0}; // Original73B97B/73B990/73B9A5 immediate writes
struct NativeContextContentFrame final {
    Impl& host;const NativeContextLocaleFrame*const predecessor;
    const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;
    int keys_ref=LUA_NOREF;const void* keys=nullptr;
    int master_ref=LUA_NOREF,proxy_ref=LUA_NOREF;const void* master=nullptr;const void* proxy=nullptr;const void* meta=nullptr;
    struct Copied {int key_ref,value_ref;};std::vector<Copied> enums_copied;
    int enums_ref=LUA_NOREF;const void* enums=nullptr;std::size_t enums_constructed_members=0;
    std::size_t fields_created=0,stores=0;Job* actual_job=nullptr;
    explicit NativeContextContentFrame(Impl& p):host(p),predecessor(p.locale_frame),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(host.content_frame||!predecessor)throw ScriptError("ContentManager registrar requires actual Locale predecessor");
        predecessor->first();if(predecessor->stores!=1||predecessor->master_ref<0||!predecessor->master)throw ScriptError("ContentManager original predecessor did not execute original master/copies");
        host.content_frame=this;
    }
    ~NativeContextContentFrame(){if(host.content_frame==this)host.content_frame=nullptr;}
    void first()const {
        if(host.content_frame!=this||creator!=std::this_thread::get_id()||!predecessor||host.locale_frame!=predecessor||
           epoch!=host.failure_epoch||writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty)
            throw ScriptError("ContentManager registrar actual source/frame changed");
        predecessor->first();
    }
    void full()const {first();predecessor->full();first();}
};
static void native_content_key(lua_State* L,int i){
    const auto& p=owner(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_content_registration_keys_ref);lua_rawgeti(L,-1,i);lua_remove(L,-2);
}
static void native_content_registration_full(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.content_frame;p.source_exception={};p.source_error=nullptr;
    try{if(!f)throw ScriptError("ContentManager registrar has no genuine entered frame");f->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="ContentManager registrar SAME source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    if(f->actual_job&&f->actual_job!=&job)return (void)luaL_error(L,"ContentManager registrar actual entered Job replaced");
    if(!f->predecessor->actual_job)return (void)luaL_error(L,"ContentManager genuine Locale predecessor Job retired");
    native_locale_registration_full(L,*f->predecessor->actual_job);
    p.source_exception={};p.source_error=nullptr;
    try{if(p.content_frame!=f)throw ScriptError("ContentManager frame replaced after source callbacks");f->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="ContentManager registrar FIRST after callbacks refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    const int top=lua_gettop(L);
    if(f->keys_ref!=LUA_NOREF){
        if(f->keys_ref!=p.native_content_registration_keys_ref)return (void)luaL_error(L,"ContentManager registrar keys custody changed");
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->keys||lua_objlen(L,-1)!=12||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"ContentManager registrar retained key table changed");
        for(int i=0;i<12;++i){
            lua_rawgeti(L,-1,i+1);std::size_t n=0;const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
            if(!key||n!=std::strlen(native_content_retained_keys[i])||std::memcmp(key,native_content_retained_keys[i],n))
                return (void)luaL_error(L,"ContentManager registrar retained key TString changed");lua_pop(L,1);
        }lua_pop(L,1);
    }
    if(f->master_ref>=0){
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->master)return (void)luaL_error(L,"ContentManager actual master table changed");
        native_content_key(L,1);lua_rawget(L,LUA_GLOBALSINDEX);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"ContentManager original master cache changed");lua_pop(L,2);
    }
    if(f->stores){
        if(f->stores!=1||f->proxy_ref<0||!f->proxy||!f->meta||f->master_ref<0)
            return (void)luaL_error(L,"ContentManager selected store lacks actual return");
        lua_rawgeti(L,LUA_REGISTRYINDEX,p.lua_events_frame->environment);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.lua_events_frame->actual_environment||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"ContentManager actual selected fenv changed");
        native_content_key(L,1);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->proxy_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->proxy||!lua_rawequal(L,-1,-2)||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=f->meta)
            return (void)luaL_error(L,"ContentManager selected proxy/ref/metatable changed");
        native_api_prefix_key(L,2);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"ContentManager proxy actual master __index changed");lua_pop(L,2);
        native_api_prefix_key(L,3);lua_rawget(L,-2);
        if(lua_type(L,-1)!=LUA_TBOOLEAN||lua_toboolean(L,-1))return (void)luaL_error(L,"ContentManager proxy original false __metatable changed");lua_pop(L,1);
        native_api_prefix_key(L,4);lua_rawget(L,-2);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_api_readonly_write||lua_getupvalue(L,-1,1))
            return (void)luaL_error(L,"ContentManager proxy original write-error consumer changed");
    }
    if(f->enums_ref>=0){
        native_content_key(L,8);lua_rawget(L,LUA_GLOBALSINDEX);lua_rawgeti(L,LUA_REGISTRYINDEX,f->enums_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->enums||!lua_rawequal(L,-1,-2))
            return (void)luaL_error(L,"ContentManagerEnums original master cache/ref changed");lua_pop(L,2);
    }
    for(const auto& copy:f->enums_copied){
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->enums_ref);lua_rawgeti(L,LUA_REGISTRYINDEX,copy.key_ref);lua_rawget(L,-2);
        lua_rawgeti(L,LUA_REGISTRYINDEX,copy.value_ref);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"ContentManagerEnums actual source field changed after copy");lua_pop(L,3);
        lua_rawgeti(L,LUA_REGISTRYINDEX,p.lua_events_frame->environment);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.lua_events_frame->actual_environment||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"ContentManagerEnums actual prepared518 destination changed");
        lua_rawgeti(L,LUA_REGISTRYINDEX,copy.key_ref);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,copy.value_ref);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"ContentManagerEnums actual selected copied field changed");lua_pop(L,3);
    }
    lua_settop(L,top);
}
static int native_content_factory(lua_State* L){
    auto& p=owner(L);auto*const f=p.content_frame;
    if(!f||!f->actual_job)return luaL_error(L,"Native ContentManager constructor lacks its actual entered Job");
    auto& job=*f->actual_job;
    native_content_registration_full(L,job);
    if(!f||f->fields_created||f->master_ref!=LUA_NOREF)return luaL_error(L,"Native ContentManager50FE70 constructor replay refused");
    lua_createtable(L,0,0);const int table=lua_gettop(L);const void*const value=lua_topointer(L,table);native_content_registration_full(L,job);
    for(int i=0;i<6;++i){
        native_content_key(L,i+2);lua_pushcfunction(L,native_content_entries[i].function);lua_rawset(L,table);++f->fields_created;native_content_registration_full(L,job);
    }
    if(f->fields_created!=6||!lua_istable(L,table)||lua_topointer(L,table)!=value||lua_getmetatable(L,table))
        return luaL_error(L,"Native ContentManager50FE70 actual namespace constructor changed");
    for(int i=0;i<6;++i){native_content_key(L,i+2);lua_rawget(L,table);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_content_entries[i].function||lua_getupvalue(L,-1,1))return luaL_error(L,"ContentManager original zero-upvalue consumer changed");lua_pop(L,1);}
    return 1;
}
static void native_content_enums_construct(lua_State* L,Job& job,int library){
    auto& p=owner(L);auto*const f=p.content_frame;native_content_registration_full(L,job);
    if(!f||f->enums_constructed_members||!lua_istable(L,library))return (void)luaL_error(L,"ContentManagerEnums actual73B920 constructor replay/input refused");
    const void*const actual_library=lua_topointer(L,library);
    native_content_key(L,9);lua_gettable(L,library);native_content_registration_full(L,job);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);lua_createtable(L,0,0);native_content_registration_full(L,job);
        native_content_key(L,9);lua_pushvalue(L,-2);lua_settable(L,library);native_content_registration_full(L,job);
    }
    const int group=lua_gettop(L);const void*const actual_group=lua_topointer(L,group);
    if(!lua_istable(L,group))return (void)luaL_error(L,"ContentManagerEnums original ContentType is not a table");
    for(int i=0;i<3;++i){
        lua_pushinteger(L,native_content_enum_values[i]);native_content_registration_full(L,job);
        native_content_key(L,i+10);lua_insert(L,-2);lua_settable(L,group);++f->enums_constructed_members;native_content_registration_full(L,job);
    }
    if(!lua_istable(L,library)||lua_topointer(L,library)!=actual_library||
       !lua_istable(L,group)||lua_topointer(L,group)!=actual_group||f->enums_constructed_members!=3)
        return (void)luaL_error(L,"ContentManagerEnums original library/group/three stores changed");
    for(int i=0;i<3;++i){native_content_key(L,i+10);lua_rawget(L,group);
        if(lua_type(L,-1)!=LUA_TNUMBER||lua_tonumber(L,-1)!=native_content_enum_values[i])
            return (void)luaL_error(L,"ContentManagerEnums original immediate integer changed after FULL");lua_pop(L,1);}
    // Actual73B9B1 invokes lua_checkstack(L,1) before popping ContentType.
    (void)lua_checkstack(L,1);lua_pop(L,1);native_content_registration_full(L,job);
}
static void native_content_enums_register(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.content_frame;native_content_registration_full(L,job);
    if(!f||f->stores!=1||f->enums_ref!=LUA_NOREF||!f->enums_copied.empty())
        return (void)luaL_error(L,"ContentManagerEnums actual717E10 ordered master action replay refused");
    native_content_key(L,8);lua_gettable(L,LUA_GLOBALSINDEX);native_content_registration_full(L,job);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);lua_createtable(L,0,0);native_content_registration_full(L,job);
        native_content_key(L,8);lua_pushvalue(L,-2);lua_settable(L,LUA_GLOBALSINDEX);native_content_registration_full(L,job);
        native_content_enums_construct(L,job,lua_gettop(L));
    }
    if(!lua_istable(L,-1))return (void)luaL_error(L,"ContentManagerEnums actual master is not a table");
    const int library=lua_gettop(L);f->enums=lua_topointer(L,library);lua_pushvalue(L,library);f->enums_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    if(f->enums_ref<0)return (void)luaL_error(L,"ContentManagerEnums actual master retention failed");native_content_registration_full(L,job);
    // Original717E74 captures its actual source index, then same534B50 selects
    // prepared518 and5D7BD0 copies every lua_next key/value into that fenv.
    perform_native_context_state_query(L,job);native_content_registration_full(L,job);const int destination=lua_gettop(L);
    if(lua_topointer(L,destination)!=p.lua_events_frame->actual_environment)
        return (void)luaL_error(L,"ContentManagerEnums original selected518 destination changed");
    lua_pushnil(L);
    while(lua_next(L,library)){
        native_content_registration_full(L,job);
        lua_pushvalue(L,-2);const int key_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_content_registration_full(L,job);
        lua_pushvalue(L,-1);const int value_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_content_registration_full(L,job);
        if(key_ref<0||value_ref<0)return (void)luaL_error(L,"ContentManagerEnums actual observed key/value retention failed");
        lua_pushvalue(L,-2);lua_pushvalue(L,-2);lua_settable(L,destination);native_content_registration_full(L,job);
        p.source_exception={};p.source_error=nullptr;
        try{f->enums_copied.push_back({key_ref,value_ref});}catch(...){p.source_exception=std::current_exception();p.source_error="ContentManagerEnums actual copied-return storage failed";}
        if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
        native_content_registration_full(L,job);lua_pop(L,1);
    }
    native_content_registration_full(L,job);job.context_content_enums_reference=f->enums_ref;
    job.context_content_enums_table=f->enums;job.context_content_enums_copied=f->enums_copied.size();lua_pop(L,2);
}
static void perform_native_context_content_registration(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.content_frame;native_content_registration_full(L,job);
    if(!f||f->stores||f->keys_ref!=LUA_NOREF||f->actual_job||!lua_checkstack(L,32))
        return (void)luaL_error(L,"ContentManager original registrar replay/key custody/stack refused");
    f->actual_job=&job;
    if(p.native_content_registration_keys_ref==LUA_NOREF){
        lua_createtable(L,12,0);const int keys=lua_gettop(L);const void*const key_value=lua_topointer(L,keys);native_content_registration_full(L,job);
        for(int i=0;i<12;++i){lua_pushstring(L,native_content_retained_keys[i]);lua_rawseti(L,keys,i+1);native_content_registration_full(L,job);}
        lua_pushvalue(L,keys);const int key_ref=luaL_ref(L,LUA_REGISTRYINDEX);
        if(key_ref<0)return (void)luaL_error(L,"ContentManager actual retained key allocation failed");
        f->keys_ref=key_ref;f->keys=key_value;p.native_content_registration_keys_ref=key_ref;p.native_content_registration_keys_pointer=key_value;
        native_content_registration_full(L,job);lua_pop(L,1);
    }else{
        f->keys_ref=p.native_content_registration_keys_ref;f->keys=p.native_content_registration_keys_pointer;native_content_registration_full(L,job);
    }
    // Original6DFB60/5605C0 selects actual518 before inspecting
    // GLOBALS.ContentManager; a cache miss runs50FE70 zero args/one result.
    perform_native_context_state_query(L,job);native_content_registration_full(L,job);const int environment=lua_gettop(L);
    if(lua_topointer(L,environment)!=p.lua_events_frame->actual_environment)
        return (void)luaL_error(L,"ContentManager registrar actual prepared518 environment changed");
    native_content_key(L,1);lua_gettable(L,LUA_GLOBALSINDEX);native_content_registration_full(L,job);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);lua_pushcfunction(L,native_content_factory);lua_call(L,0,1);native_content_registration_full(L,job);
        native_content_key(L,1);lua_pushvalue(L,-2);lua_settable(L,LUA_GLOBALSINDEX);native_content_registration_full(L,job);
    }
    if(!lua_istable(L,-1))return (void)luaL_error(L,"ContentManager original namespace master is not a table");
    f->master=lua_topointer(L,-1);lua_pushvalue(L,-1);f->master_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_content_registration_full(L,job);
    native_api_readonly_proxy(L,job);native_content_registration_full(L,job);const int proxy=lua_gettop(L);f->proxy=lua_topointer(L,proxy);
    if(!lua_getmetatable(L,proxy))return (void)luaL_error(L,"ContentManager original49B190 proxy has no metatable");f->meta=lua_topointer(L,-1);lua_pop(L,1);
    native_content_key(L,1);lua_pushvalue(L,proxy);lua_settable(L,environment);native_content_registration_full(L,job);
    lua_pushvalue(L,proxy);f->proxy_ref=luaL_ref(L,LUA_REGISTRYINDEX);++f->stores;native_content_registration_full(L,job);
    job.context_content_master_reference=f->master_ref;job.context_content_proxy_reference=f->proxy_ref;
    job.context_content_table=f->master;job.context_content_proxy=f->proxy;lua_pop(L,2);
    native_content_enums_register(L,job);native_content_registration_full(L,job);
    worldview_capture_content_publications_owned(L,job);native_content_registration_full(L,job);
}
