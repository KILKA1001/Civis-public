// Native546FA0 installs real zero-upvalue consumers without invoking them.
// An issued callback must come from the corresponding genuine backend owner.
// This prefix has no such issuer; the closed binding cannot be constructed.
// Presence checks below never synthesize a Steam result or success.
class NativeSteamBackendBinding final {
    NativeSteamBackendBinding()=delete;
    NativeSteamBackendBinding(const NativeSteamBackendBinding&)=delete;
    NativeSteamBackendBinding& operator=(const NativeSteamBackendBinding&)=delete;
    Impl* actual_host=nullptr;lua_State* actual_vm=nullptr;
    std::uint32_t original_function=0;lua_CFunction actual_callback=nullptr;
    std::weak_ptr<EndTurnQueue::VmLifetime> lifetime;bool retired=false;
    friend struct Impl;
public:~NativeSteamBackendBinding()=default;
};
template<std::uint32_t NativeFunction> static int native_steam_method(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);
    NativeSteamBackendBinding* binding=nullptr;
    for(const auto& issued:p.native_steam_backend_bindings)
        if(issued&&issued->original_function==NativeFunction){if(binding)return luaL_error(L,"Steam actual backend binding duplicated");binding=issued.get();}
    if(!binding){p.record_failure();return luaL_error(L,"Steam native consumer %08X requires its genuine backend; provider unissued",static_cast<unsigned>(NativeFunction));}
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    p.source_exception={};p.source_error=nullptr;
    lua_CFunction callback=nullptr;lua_State* actual_vm=nullptr;const EndTurnQueue::VmLifetime* actual_lifetime=nullptr;
    try{
        p.worldview_require_callback_origin();
        bool owned=false;for(const auto& issued:p.native_steam_backend_bindings)if(issued.get()==binding){owned=true;break;}
        if(!owned)throw ScriptError("Steam binding retired during source FIRST");
        const auto life=binding->lifetime.lock();
        if(binding->retired||binding->original_function!=NativeFunction||binding->actual_host!=&p||binding->actual_vm!=p.state||!binding->actual_callback||
           !life||life.get()!=p.end_turn_lifetime.get()||life.owner_before(p.end_turn_lifetime)||
           p.end_turn_lifetime.owner_before(life)||!life->alive||life->failed||life->vm!=p.state||life->host!=p.worldview_host_owner)
            throw ScriptError("Steam actual backend/facade custody retired");
        callback=binding->actual_callback;actual_vm=binding->actual_vm;actual_lifetime=life.get();
    }catch(...){p.source_exception=std::current_exception();p.source_error="Steam genuine backend FIRST refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    // No owning C++ local or speculative argument/result conversion crosses
    // this real Lua CFunction invocation. A future genuine issuer must supply
    // the complete original method body and its real backend custody.
    const int count=callback(L);native_db_query_current(L);
    p.source_exception={};p.source_error=nullptr;
    try{
        p.worldview_require_callback_origin();
        bool owned=false;for(const auto& issued:p.native_steam_backend_bindings)if(issued.get()==binding){owned=true;break;}
        if(!owned)throw ScriptError("Steam binding retired during callback/source POST");
        const auto life=binding->lifetime.lock();
        if(binding->retired||binding->original_function!=NativeFunction||binding->actual_host!=&p||
           binding->actual_vm!=actual_vm||actual_vm!=p.state||binding->actual_callback!=callback||
           !life||life.get()!=actual_lifetime||life.get()!=p.end_turn_lifetime.get()||
           life.owner_before(p.end_turn_lifetime)||p.end_turn_lifetime.owner_before(life)||!life->alive||life->failed||
           life->vm!=actual_vm||life->host!=p.worldview_host_owner||epoch!=p.failure_epoch||
           writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty)
            throw ScriptError("Steam genuine callback/VM/weak lifetime/source changed");
    }catch(...){p.source_exception=std::current_exception();p.source_error="Steam genuine callback/source POST refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    if(count<0||count>lua_gettop(L)){p.record_failure();return luaL_error(L,"Steam actual callback returned invalid result count");}
    return count;
}
struct NativeSteamEntry {const char* name;std::uint32_t original_function;lua_CFunction function;};
static constexpr NativeSteamEntry native_steam_entries[]={
    {"ActivateGameOverlay",0x4c4ae0,native_steam_method<0x4c4ae0>}, // 0x546fc4
    {"ActivateGameOverlayToWebPage",0x6ea210,native_steam_method<0x6ea210>}, // 0x546fde
    {"ActivateGameOverlayToStore",0x749370,native_steam_method<0x749370>}, // 0x546ffb
    {"ActivateInviteOverlay",0x5a4900,native_steam_method<0x5a4900>}, // 0x547015
    {"SetOverlayNotificationPosition",0x493220,native_steam_method<0x493220>}, // 0x54702f
    {"IsOverlayEnabled",0x53aaa0,native_steam_method<0x53aaa0>}, // 0x54704c
    {"GetMySteamID",0x6a4680,native_steam_method<0x6a4680>}, // 0x547066
    {"GetPlayerSteamID",0x773870,native_steam_method<0x773870>}, // 0x547080
    {"GetCloudSaveFileName",0x692e60,native_steam_method<0x692e60>}, // 0x54709d
    {"GetMaxCloudSaves",0x531c00,native_steam_method<0x531c00>}, // 0x5470b7
    {"GetCloudSaves",0x566f40,native_steam_method<0x566f40>}, // 0x5470d1
    {"SaveGameToCloud",0x55de60,native_steam_method<0x55de60>}, // 0x5470ee
    {"CopyLastAutoSaveToSteamCloud",0x691070,native_steam_method<0x691070>}, // 0x547108
};
struct NativeContextSteamFrame final {
    Impl& host;const NativeContextNetworkFrame*const predecessor;
    const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;
    int keys_ref=LUA_NOREF;const void* keys=nullptr;
    int master_ref=LUA_NOREF,proxy_ref=LUA_NOREF;const void* master=nullptr;const void* proxy=nullptr;const void* meta=nullptr;
    std::size_t fields_created=0,stores=0;Job* actual_job=nullptr;
    explicit NativeContextSteamFrame(Impl& p):host(p),predecessor(p.network_frame),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(host.steam_frame||!predecessor)throw ScriptError("Steam registrar requires actual Network predecessor");
        predecessor->first();if(predecessor->stores!=1||predecessor->master_ref<0||!predecessor->master)throw ScriptError("Steam original predecessor did not execute original master/copies");
        host.steam_frame=this;
    }
    ~NativeContextSteamFrame(){if(host.steam_frame==this)host.steam_frame=nullptr;}
    void first()const {
        if(host.steam_frame!=this||creator!=std::this_thread::get_id()||!predecessor||host.network_frame!=predecessor||
           epoch!=host.failure_epoch||writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty)
            throw ScriptError("Steam registrar actual source/frame changed");
        predecessor->first();
    }
    void full()const {first();predecessor->full();first();}
};
static void native_steam_key(lua_State* L,int i){
    const auto& p=owner(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_steam_registration_keys_ref);lua_rawgeti(L,-1,i);lua_remove(L,-2);
}
static void native_steam_registration_full(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.steam_frame;p.source_exception={};p.source_error=nullptr;
    try{if(!f)throw ScriptError("Steam registrar has no genuine entered frame");f->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Steam registrar SAME source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    if(f->actual_job&&f->actual_job!=&job)return (void)luaL_error(L,"Steam registrar actual entered Job replaced");
    if(!f->predecessor->actual_job)return (void)luaL_error(L,"Steam genuine Network predecessor Job retired");
    native_network_registration_full(L,*f->predecessor->actual_job);
    p.source_exception={};p.source_error=nullptr;
    try{if(p.steam_frame!=f)throw ScriptError("Steam frame replaced after source callbacks");f->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Steam registrar FIRST after callbacks refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    const int top=lua_gettop(L);
    if(f->keys_ref!=LUA_NOREF){
        if(f->keys_ref!=p.native_steam_registration_keys_ref)return (void)luaL_error(L,"Steam registrar keys custody changed");
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->keys||lua_objlen(L,-1)!=14||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Steam registrar retained key table changed");
        for(int i=0;i<14;++i){
            lua_rawgeti(L,-1,i+1);std::size_t n=0;const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
            if(!key||n!=std::strlen(i==0?"Steam":native_steam_entries[i-1].name)||std::memcmp(key,i==0?"Steam":native_steam_entries[i-1].name,n))
                return (void)luaL_error(L,"Steam registrar retained key TString changed");lua_pop(L,1);
        }lua_pop(L,1);
    }
    if(f->master_ref>=0){
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->master)return (void)luaL_error(L,"Steam actual master table changed");
        native_steam_key(L,1);lua_rawget(L,LUA_GLOBALSINDEX);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Steam original master cache changed");lua_pop(L,2);
    }
    if(f->stores){
        if(f->stores!=1||f->proxy_ref<0||!f->proxy||!f->meta||f->master_ref<0)
            return (void)luaL_error(L,"Steam selected store lacks actual return");
        lua_rawgeti(L,LUA_REGISTRYINDEX,p.lua_events_frame->environment);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.lua_events_frame->actual_environment||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Steam actual selected fenv changed");
        native_steam_key(L,1);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->proxy_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->proxy||!lua_rawequal(L,-1,-2)||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=f->meta)
            return (void)luaL_error(L,"Steam selected proxy/ref/metatable changed");
        native_api_prefix_key(L,2);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Steam proxy actual master __index changed");lua_pop(L,2);
        native_api_prefix_key(L,3);lua_rawget(L,-2);
        if(lua_type(L,-1)!=LUA_TBOOLEAN||lua_toboolean(L,-1))return (void)luaL_error(L,"Steam proxy original false __metatable changed");lua_pop(L,1);
        native_api_prefix_key(L,4);lua_rawget(L,-2);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_api_readonly_write||lua_getupvalue(L,-1,1))
            return (void)luaL_error(L,"Steam proxy original write-error consumer changed");
    }
    lua_settop(L,top);
}
static int native_steam_factory(lua_State* L){
    auto& p=owner(L);auto*const f=p.steam_frame;
    if(!f||!f->actual_job)return luaL_error(L,"Native Steam constructor lacks its actual entered Job");
    auto& job=*f->actual_job;
    native_steam_registration_full(L,job);
    if(!f||f->fields_created||f->master_ref!=LUA_NOREF)return luaL_error(L,"Native Steam546FA0 constructor replay refused");
    lua_createtable(L,0,5);const int table=lua_gettop(L);const void*const value=lua_topointer(L,table);native_steam_registration_full(L,job);
    for(int i=0;i<13;++i){
        native_steam_key(L,i+2);lua_pushcfunction(L,native_steam_entries[i].function);lua_rawset(L,table);++f->fields_created;native_steam_registration_full(L,job);
    }
    if(f->fields_created!=13||!lua_istable(L,table)||lua_topointer(L,table)!=value||lua_getmetatable(L,table))
        return luaL_error(L,"Native Steam546FA0 actual namespace constructor changed");
    for(int i=0;i<13;++i){native_steam_key(L,i+2);lua_rawget(L,table);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_steam_entries[i].function||lua_getupvalue(L,-1,1))return luaL_error(L,"Steam original zero-upvalue consumer changed");lua_pop(L,1);}
    return 1;
}
static void perform_native_context_steam_registration(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.steam_frame;native_steam_registration_full(L,job);
    if(!f||f->stores||f->keys_ref!=LUA_NOREF||f->actual_job||!lua_checkstack(L,32))
        return (void)luaL_error(L,"Steam original registrar replay/key custody/stack refused");
    f->actual_job=&job;
    if(p.native_steam_registration_keys_ref==LUA_NOREF){
        lua_createtable(L,14,0);const int keys=lua_gettop(L);const void*const key_value=lua_topointer(L,keys);native_steam_registration_full(L,job);
        for(int i=0;i<14;++i){lua_pushstring(L,i==0?"Steam":native_steam_entries[i-1].name);lua_rawseti(L,keys,i+1);native_steam_registration_full(L,job);}
        lua_pushvalue(L,keys);const int key_ref=luaL_ref(L,LUA_REGISTRYINDEX);
        if(key_ref<0)return (void)luaL_error(L,"Steam actual retained key allocation failed");
        f->keys_ref=key_ref;f->keys=key_value;p.native_steam_registration_keys_ref=key_ref;p.native_steam_registration_keys_pointer=key_value;
        native_steam_registration_full(L,job);lua_pop(L,1);
    }else{
        f->keys_ref=p.native_steam_registration_keys_ref;f->keys=p.native_steam_registration_keys_pointer;native_steam_registration_full(L,job);
    }
    // Original6DFB60/5605C0 selects actual518 before inspecting
    // GLOBALS.Steam; a cache miss runs546FA0 zero args/one result.
    perform_native_context_state_query(L,job);native_steam_registration_full(L,job);const int environment=lua_gettop(L);
    if(lua_topointer(L,environment)!=p.lua_events_frame->actual_environment)
        return (void)luaL_error(L,"Steam registrar actual prepared518 environment changed");
    native_steam_key(L,1);lua_gettable(L,LUA_GLOBALSINDEX);native_steam_registration_full(L,job);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);lua_pushcfunction(L,native_steam_factory);lua_call(L,0,1);native_steam_registration_full(L,job);
        native_steam_key(L,1);lua_pushvalue(L,-2);lua_settable(L,LUA_GLOBALSINDEX);native_steam_registration_full(L,job);
    }
    if(!lua_istable(L,-1))return (void)luaL_error(L,"Steam original namespace master is not a table");
    f->master=lua_topointer(L,-1);lua_pushvalue(L,-1);f->master_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_steam_registration_full(L,job);
    native_api_readonly_proxy(L,job);native_steam_registration_full(L,job);const int proxy=lua_gettop(L);f->proxy=lua_topointer(L,proxy);
    if(!lua_getmetatable(L,proxy))return (void)luaL_error(L,"Steam original49B190 proxy has no metatable");f->meta=lua_topointer(L,-1);lua_pop(L,1);
    native_steam_key(L,1);lua_pushvalue(L,proxy);lua_settable(L,environment);native_steam_registration_full(L,job);
    lua_pushvalue(L,proxy);f->proxy_ref=luaL_ref(L,LUA_REGISTRYINDEX);++f->stores;native_steam_registration_full(L,job);
    job.context_steam_master_reference=f->master_ref;job.context_steam_proxy_reference=f->proxy_ref;
    job.context_steam_table=f->master;job.context_steam_proxy=f->proxy;lua_pop(L,2);
}
