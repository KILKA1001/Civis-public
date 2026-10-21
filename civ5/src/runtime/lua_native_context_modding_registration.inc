// Native41B400 installs real zero-upvalue consumers without invoking them.
// An issued callback must come from the corresponding genuine backend owner.
// This prefix has no such issuer; the closed binding cannot be constructed.
// Presence checks below never synthesize a Modding result or success.
class NativeModdingBackendBinding final {
    NativeModdingBackendBinding()=delete;
    NativeModdingBackendBinding(const NativeModdingBackendBinding&)=delete;
    NativeModdingBackendBinding& operator=(const NativeModdingBackendBinding&)=delete;
    Impl* actual_host=nullptr;lua_State* actual_vm=nullptr;
    std::uint32_t original_function=0;lua_CFunction actual_callback=nullptr;
    std::weak_ptr<EndTurnQueue::VmLifetime> lifetime;bool retired=false;
    friend struct Impl;
public:~NativeModdingBackendBinding()=default;
};
template<std::uint32_t NativeFunction> static int native_modding_method(lua_State* L){
    if constexpr(NativeFunction==0x6d9910u)return native_modding_activated_entry_points_owned(L);
    auto& p=owner(L);native_db_query_current(L);
    NativeModdingBackendBinding* binding=nullptr;
    for(const auto& issued:p.native_modding_backend_bindings)
        if(issued&&issued->original_function==NativeFunction){if(binding)return luaL_error(L,"Modding actual backend binding duplicated");binding=issued.get();}
    if(!binding){p.record_failure();return luaL_error(L,"Modding native consumer %08X requires its genuine backend; provider unissued",static_cast<unsigned>(NativeFunction));}
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    p.source_exception={};p.source_error=nullptr;
    lua_CFunction callback=nullptr;lua_State* actual_vm=nullptr;const EndTurnQueue::VmLifetime* actual_lifetime=nullptr;
    try{
        p.worldview_require_callback_origin();
        bool owned=false;for(const auto& issued:p.native_modding_backend_bindings)if(issued.get()==binding){owned=true;break;}
        if(!owned)throw ScriptError("Modding binding retired during source FIRST");
        const auto life=binding->lifetime.lock();
        if(binding->retired||binding->original_function!=NativeFunction||binding->actual_host!=&p||binding->actual_vm!=p.state||!binding->actual_callback||
           !life||life.get()!=p.end_turn_lifetime.get()||life.owner_before(p.end_turn_lifetime)||
           p.end_turn_lifetime.owner_before(life)||!life->alive||life->failed||life->vm!=p.state||life->host!=p.worldview_host_owner)
            throw ScriptError("Modding actual backend/facade custody retired");
        callback=binding->actual_callback;actual_vm=binding->actual_vm;actual_lifetime=life.get();
    }catch(...){p.source_exception=std::current_exception();p.source_error="Modding genuine backend FIRST refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    // No owning C++ local or speculative argument/result conversion crosses
    // this real Lua CFunction invocation. A future genuine issuer must supply
    // the complete original method body and its real backend custody.
    const int count=callback(L);native_db_query_current(L);
    p.source_exception={};p.source_error=nullptr;
    try{
        p.worldview_require_callback_origin();
        bool owned=false;for(const auto& issued:p.native_modding_backend_bindings)if(issued.get()==binding){owned=true;break;}
        if(!owned)throw ScriptError("Modding binding retired during callback/source POST");
        const auto life=binding->lifetime.lock();
        if(binding->retired||binding->original_function!=NativeFunction||binding->actual_host!=&p||
           binding->actual_vm!=actual_vm||actual_vm!=p.state||binding->actual_callback!=callback||
           !life||life.get()!=actual_lifetime||life.get()!=p.end_turn_lifetime.get()||
           life.owner_before(p.end_turn_lifetime)||p.end_turn_lifetime.owner_before(life)||!life->alive||life->failed||
           life->vm!=actual_vm||life->host!=p.worldview_host_owner||epoch!=p.failure_epoch||
           writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty)
            throw ScriptError("Modding genuine callback/VM/weak lifetime/source changed");
    }catch(...){p.source_exception=std::current_exception();p.source_error="Modding genuine callback/source POST refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    if(count<0||count>lua_gettop(L)){p.record_failure();return luaL_error(L,"Modding actual callback returned invalid result count");}
    return count;
}
struct NativeModdingEntry {const char* name;std::uint32_t original_function;lua_CFunction function;};
static constexpr NativeModdingEntry native_modding_entries[]={
    {"DiscoverMods",0x761500,native_modding_method<0x761500>}, // 0x41b424
    {"UpdateModdingSystem",0x4f8e30,native_modding_method<0x4f8e30>}, // 0x41b43e
    {"GetModBrowserInstalledListings",0x57e7f0,native_modding_method<0x57e7f0>}, // 0x41b45b
    {"GetModBrowserDownloadingListings",0x70f730,native_modding_method<0x70f730>}, // 0x41b475
    {"GetModsBrowserInstalledListingsState",0x75ae30,native_modding_method<0x75ae30>}, // 0x41b48f
    {"GetDownloadProgress",0x68ad30,native_modding_method<0x68ad30>}, // 0x41b4ac
    {"GetInstalledModDetails",0x628780,native_modding_method<0x628780>}, // 0x41b4c6
    {"HasPendingInstalls",0x777980,native_modding_method<0x777980>}, // 0x41b4e0
    {"InstallMods",0x4e1e60,native_modding_method<0x4e1e60>}, // 0x41b4fd
    {"GetInstallProgress",0x5e7ef0,native_modding_method<0x5e7ef0>}, // 0x41b517
    {"GetActivatedMods",0x60ea10,native_modding_method<0x60ea10>}, // 0x41b531
    {"GetInstalledMods",0x701680,native_modding_method<0x701680>}, // 0x41b54e
    {"GetLatestInstalledModVersion",0x4d1db0,native_modding_method<0x4d1db0>}, // 0x41b568
    {"IsModInstalled",0x766d40,native_modding_method<0x766d40>}, // 0x41b582
    {"GetInstalledFiraxisScenarios",0x71ae90,native_modding_method<0x71ae90>}, // 0x41b59f
    {"GetActivatedModVersion",0x6fbfb0,native_modding_method<0x6fbfb0>}, // 0x41b5b9
    {"GetActivatedModEntryPoints",0x6d9910,native_modding_method<0x6d9910>}, // 0x41b5d3
    {"GetEnabledModsByActivationOrder",0x695ef0,native_modding_method<0x695ef0>}, // 0x41b5f0
    {"GetEvaluatedFilePath",0x73c440,native_modding_method<0x73c440>}, // 0x41b60a
    {"GetModsRequiredToDisableMod",0x798230,native_modding_method<0x798230>}, // 0x41b624
    {"GetModsRequiredToEnableMod",0x636410,native_modding_method<0x636410>}, // 0x41b641
    {"GetModEntryPoints",0x5c1310,native_modding_method<0x5c1310>}, // 0x41b65b
    {"GetModProperty",0x7b1650,native_modding_method<0x7b1650>}, // 0x41b675
    {"GetModProperties",0x4619f0,native_modding_method<0x4619f0>}, // 0x41b692
    {"GetMapFiles",0x4c7450,native_modding_method<0x4c7450>}, // 0x41b6ac
    {"ActivateSpecificMod",0x4cf3f0,native_modding_method<0x4cf3f0>}, // 0x41b6c6
    {"ActivateEnabledMods",0x5604e0,native_modding_method<0x5604e0>}, // 0x41b6e3
    {"DeactivateMods",0x5d28f0,native_modding_method<0x5d28f0>}, // 0x41b6fd
    {"DeactivateDLC",0x5a1a60,native_modding_method<0x5a1a60>}, // 0x41b717
    {"ActivateDLC",0x744a50,native_modding_method<0x744a50>}, // 0x41b734
    {"ActivateAllowedDLC",0x663e80,native_modding_method<0x663e80>}, // 0x41b74e
    {"ActivateModsAndDLCForReplay",0x42f750,native_modding_method<0x42f750>}, // 0x41b768
    {"PerformActions",0x6d2730,native_modding_method<0x6d2730>}, // 0x41b785
    {"PostProcessModActions",0x7ae7b0,native_modding_method<0x7ae7b0>}, // 0x41b79f
    {"CanEnableMod",0x4514a0,native_modding_method<0x4514a0>}, // 0x41b7b9
    {"EnableMod",0x6a0230,native_modding_method<0x6a0230>}, // 0x41b7d6
    {"DisableMod",0x52e1b0,native_modding_method<0x52e1b0>}, // 0x41b7f0
    {"UpdateMod",0x5df1e0,native_modding_method<0x5df1e0>}, // 0x41b80a
    {"CanUnsubscribeMod",0x62c110,native_modding_method<0x62c110>}, // 0x41b827
    {"CanDeleteMod",0x49ee80,native_modding_method<0x49ee80>}, // 0x41b841
    {"UnsubscribeMod",0x64d290,native_modding_method<0x64d290>}, // 0x41b85b
    {"DeleteMod",0x714f70,native_modding_method<0x714f70>}, // 0x41b878
    {"OpenSaveData",0x58eef0,native_modding_method<0x58eef0>}, // 0x41b892
    {"OpenUserData",0x4a26d0,native_modding_method<0x4a26d0>}, // 0x41b8ac
    {"DeleteUserData",0x440650,native_modding_method<0x440650>}, // 0x41b8c9
    {"HasUserData",0x78e5c0,native_modding_method<0x78e5c0>}, // 0x41b8e3
    {"AllActivatedModsContainPropertyValue",0x57d0c0,native_modding_method<0x57d0c0>}, // 0x41b8fd
    {"AnyActivatedModsContainPropertyValue",0x4c7c50,native_modding_method<0x4c7c50>}, // 0x41b91a
    {"AllEnabledModsContainPropertyValue",0x6ff130,native_modding_method<0x6ff130>}, // 0x41b934
    {"AnyEnabledModsContainPropertyValue",0x68ae10,native_modding_method<0x68ae10>}, // 0x41b94e
    {"GetCloudSaveRequirements",0x6d00d0,native_modding_method<0x6d00d0>}, // 0x41b96b
    {"GetReplayRequirements",0x533bc0,native_modding_method<0x533bc0>}, // 0x41b985
    {"GetSavedGameRequirements",0x4f5660,native_modding_method<0x4f5660>}, // 0x41b99f
    {"CanLoadCloudSave",0x4d7420,native_modding_method<0x4d7420>}, // 0x41b9bc
    {"CanLoadReplay",0x5ec500,native_modding_method<0x5ec500>}, // 0x41b9d6
    {"CanLoadSavedGame",0x45f7e0,native_modding_method<0x45f7e0>}, // 0x41b9f0
    {"GetDlcNameDescriptionKeys",0x69f680,native_modding_method<0x69f680>}, // 0x41ba0d
    {"GetModAssociations",0x6ee210,native_modding_method<0x6ee210>}, // 0x41ba27
    {"GetDlcAssociations",0x47c080,native_modding_method<0x47c080>}, // 0x41ba41
    {"GetGameVersionAssociations",0x5b2c50,native_modding_method<0x5b2c50>}, // 0x41ba5e
    {"GetSystemProperty",0x61f9b0,native_modding_method<0x61f9b0>}, // 0x41ba78
    {"SetSystemProperty",0x5d36a0,native_modding_method<0x5d36a0>}, // 0x41ba92
    {"PublishedFileIdsMatch",0x43f720,native_modding_method<0x43f720>}, // 0x41baaf
};
struct NativeContextModdingFrame final {
    Impl& host;const NativeContextContentFrame*const predecessor;
    const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;
    int keys_ref=LUA_NOREF;const void* keys=nullptr;
    int master_ref=LUA_NOREF,proxy_ref=LUA_NOREF;const void* master=nullptr;const void* proxy=nullptr;const void* meta=nullptr;
    std::size_t fields_created=0,stores=0;Job* actual_job=nullptr;
    explicit NativeContextModdingFrame(Impl& p):host(p),predecessor(p.content_frame),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(host.modding_frame||!predecessor)throw ScriptError("Modding registrar requires actual ContentManager predecessor");
        predecessor->first();if(predecessor->stores!=1||predecessor->enums_ref<0||!predecessor->enums||predecessor->master_ref<0||!predecessor->master)throw ScriptError("Modding original predecessor did not execute original master/copies");
        host.modding_frame=this;
    }
    ~NativeContextModdingFrame(){if(host.modding_frame==this)host.modding_frame=nullptr;}
    void first()const {
        if(host.modding_frame!=this||creator!=std::this_thread::get_id()||!predecessor||host.content_frame!=predecessor||
           epoch!=host.failure_epoch||writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty)
            throw ScriptError("Modding registrar actual source/frame changed");
        predecessor->first();
    }
    void full()const {first();predecessor->full();first();}
};
static void native_modding_key(lua_State* L,int i){
    const auto& p=owner(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_modding_registration_keys_ref);lua_rawgeti(L,-1,i);lua_remove(L,-2);
}
static void native_modding_registration_full(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.modding_frame;p.source_exception={};p.source_error=nullptr;
    try{if(!f)throw ScriptError("Modding registrar has no genuine entered frame");f->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Modding registrar SAME source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    if(f->actual_job&&f->actual_job!=&job)return (void)luaL_error(L,"Modding registrar actual entered Job replaced");
    if(!f->predecessor->actual_job)return (void)luaL_error(L,"Modding genuine ContentManager predecessor Job retired");
    native_content_registration_full(L,*f->predecessor->actual_job);
    p.source_exception={};p.source_error=nullptr;
    try{if(p.modding_frame!=f)throw ScriptError("Modding frame replaced after source callbacks");f->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Modding registrar FIRST after callbacks refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    const int top=lua_gettop(L);
    if(f->keys_ref!=LUA_NOREF){
        if(f->keys_ref!=p.native_modding_registration_keys_ref)return (void)luaL_error(L,"Modding registrar keys custody changed");
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->keys||lua_objlen(L,-1)!=64||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Modding registrar retained key table changed");
        for(int i=0;i<64;++i){
            lua_rawgeti(L,-1,i+1);std::size_t n=0;const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
            if(!key||n!=std::strlen(i==0?"Modding":native_modding_entries[i-1].name)||std::memcmp(key,i==0?"Modding":native_modding_entries[i-1].name,n))
                return (void)luaL_error(L,"Modding registrar retained key TString changed");lua_pop(L,1);
        }lua_pop(L,1);
    }
    if(f->master_ref>=0){
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->master)return (void)luaL_error(L,"Modding actual master table changed");
        native_modding_key(L,1);lua_rawget(L,LUA_GLOBALSINDEX);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Modding original master cache changed");lua_pop(L,2);
    }
    if(f->stores){
        if(f->stores!=1||f->proxy_ref<0||!f->proxy||!f->meta||f->master_ref<0)
            return (void)luaL_error(L,"Modding selected store lacks actual return");
        lua_rawgeti(L,LUA_REGISTRYINDEX,p.lua_events_frame->environment);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.lua_events_frame->actual_environment||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Modding actual selected fenv changed");
        native_modding_key(L,1);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->proxy_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->proxy||!lua_rawequal(L,-1,-2)||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=f->meta)
            return (void)luaL_error(L,"Modding selected proxy/ref/metatable changed");
        native_api_prefix_key(L,2);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Modding proxy actual master __index changed");lua_pop(L,2);
        native_api_prefix_key(L,3);lua_rawget(L,-2);
        if(lua_type(L,-1)!=LUA_TBOOLEAN||lua_toboolean(L,-1))return (void)luaL_error(L,"Modding proxy original false __metatable changed");lua_pop(L,1);
        native_api_prefix_key(L,4);lua_rawget(L,-2);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_api_readonly_write||lua_getupvalue(L,-1,1))
            return (void)luaL_error(L,"Modding proxy original write-error consumer changed");
    }
    lua_settop(L,top);
}
static int native_modding_factory(lua_State* L){
    auto& p=owner(L);auto*const f=p.modding_frame;
    if(!f||!f->actual_job)return luaL_error(L,"Native Modding constructor lacks its actual entered Job");
    auto& job=*f->actual_job;
    native_modding_registration_full(L,job);
    if(!f||f->fields_created||f->master_ref!=LUA_NOREF)return luaL_error(L,"Native Modding41B400 constructor replay refused");
    lua_createtable(L,0,0);const int table=lua_gettop(L);const void*const value=lua_topointer(L,table);native_modding_registration_full(L,job);
    for(int i=0;i<63;++i){
        native_modding_key(L,i+2);lua_pushcfunction(L,native_modding_entries[i].function);lua_rawset(L,table);++f->fields_created;native_modding_registration_full(L,job);
    }
    if(f->fields_created!=63||!lua_istable(L,table)||lua_topointer(L,table)!=value||lua_getmetatable(L,table))
        return luaL_error(L,"Native Modding41B400 actual namespace constructor changed");
    for(int i=0;i<63;++i){native_modding_key(L,i+2);lua_rawget(L,table);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_modding_entries[i].function||lua_getupvalue(L,-1,1))return luaL_error(L,"Modding original zero-upvalue consumer changed");lua_pop(L,1);}
    return 1;
}
static void perform_native_context_modding_registration(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.modding_frame;native_modding_registration_full(L,job);
    if(!f||f->stores||f->keys_ref!=LUA_NOREF||f->actual_job||!lua_checkstack(L,32))
        return (void)luaL_error(L,"Modding original registrar replay/key custody/stack refused");
    f->actual_job=&job;
    if(p.native_modding_registration_keys_ref==LUA_NOREF){
        lua_createtable(L,64,0);const int keys=lua_gettop(L);const void*const key_value=lua_topointer(L,keys);native_modding_registration_full(L,job);
        for(int i=0;i<64;++i){lua_pushstring(L,i==0?"Modding":native_modding_entries[i-1].name);lua_rawseti(L,keys,i+1);native_modding_registration_full(L,job);}
        lua_pushvalue(L,keys);const int key_ref=luaL_ref(L,LUA_REGISTRYINDEX);
        if(key_ref<0)return (void)luaL_error(L,"Modding actual retained key allocation failed");
        f->keys_ref=key_ref;f->keys=key_value;p.native_modding_registration_keys_ref=key_ref;p.native_modding_registration_keys_pointer=key_value;
        native_modding_registration_full(L,job);lua_pop(L,1);
    }else{
        f->keys_ref=p.native_modding_registration_keys_ref;f->keys=p.native_modding_registration_keys_pointer;native_modding_registration_full(L,job);
    }
    // Original6DFB60/5605C0 selects actual518 before inspecting
    // GLOBALS.Modding; a cache miss runs41B400 zero args/one result.
    perform_native_context_state_query(L,job);native_modding_registration_full(L,job);const int environment=lua_gettop(L);
    if(lua_topointer(L,environment)!=p.lua_events_frame->actual_environment)
        return (void)luaL_error(L,"Modding registrar actual prepared518 environment changed");
    native_modding_key(L,1);lua_gettable(L,LUA_GLOBALSINDEX);native_modding_registration_full(L,job);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);lua_pushcfunction(L,native_modding_factory);lua_call(L,0,1);native_modding_registration_full(L,job);
        native_modding_key(L,1);lua_pushvalue(L,-2);lua_settable(L,LUA_GLOBALSINDEX);native_modding_registration_full(L,job);
    }
    if(!lua_istable(L,-1))return (void)luaL_error(L,"Modding original namespace master is not a table");
    f->master=lua_topointer(L,-1);lua_pushvalue(L,-1);f->master_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_modding_registration_full(L,job);
    native_api_readonly_proxy(L,job);native_modding_registration_full(L,job);const int proxy=lua_gettop(L);f->proxy=lua_topointer(L,proxy);
    if(!lua_getmetatable(L,proxy))return (void)luaL_error(L,"Modding original49B190 proxy has no metatable");f->meta=lua_topointer(L,-1);lua_pop(L,1);
    native_modding_key(L,1);lua_pushvalue(L,proxy);lua_settable(L,environment);native_modding_registration_full(L,job);
    lua_pushvalue(L,proxy);f->proxy_ref=luaL_ref(L,LUA_REGISTRYINDEX);++f->stores;native_modding_registration_full(L,job);
    job.context_modding_master_reference=f->master_ref;job.context_modding_proxy_reference=f->proxy_ref;
    job.context_modding_table=f->master;job.context_modding_proxy=f->proxy;lua_pop(L,2);
    native_modding_registration_full(L,job);worldview_capture_modding_publication_owned(L,job);native_modding_registration_full(L,job);
}
