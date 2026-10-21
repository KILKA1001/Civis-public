// Native4A4600 installs real zero-upvalue consumers without invoking them.
// An issued callback must come from the corresponding genuine backend owner.
// Two SP consumers enter the actual protected scalar lender; other binding issuers remain unissued.
// Presence checks below never synthesize a Network result or success.
class NativeNetworkBackendBinding final {
    NativeNetworkBackendBinding()=delete;
    NativeNetworkBackendBinding(const NativeNetworkBackendBinding&)=delete;
    NativeNetworkBackendBinding& operator=(const NativeNetworkBackendBinding&)=delete;
    Impl* actual_host=nullptr;lua_State* actual_vm=nullptr;
    std::uint32_t original_function=0;lua_CFunction actual_callback=nullptr;
    std::weak_ptr<EndTurnQueue::VmLifetime> lifetime;bool retired=false;
    friend struct Impl;
public:~NativeNetworkBackendBinding()=default;
};
template<std::uint32_t NativeFunction> static int native_network_method(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);
    if constexpr(NativeFunction==0x4ac8d0u||NativeFunction==0x5a2360u){
        // Original zero-upvalue publisher identity; the actual one-upvalue SP
        // lender is invoked as a real Lua closure by the protected bridge.
        constexpr int operation=NativeFunction==0x4ac8d0u?0:1;
        return worldview_sp_network_scalar_call_owned(L,operation);
    }
    NativeNetworkBackendBinding* binding=nullptr;
    for(const auto& issued:p.native_network_backend_bindings)
        if(issued&&issued->original_function==NativeFunction){if(binding)return luaL_error(L,"Network actual backend binding duplicated");binding=issued.get();}
    if(!binding){p.record_failure();return luaL_error(L,"Network native consumer %08X requires its genuine backend; provider unissued",static_cast<unsigned>(NativeFunction));}
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    p.source_exception={};p.source_error=nullptr;
    lua_CFunction callback=nullptr;lua_State* actual_vm=nullptr;const EndTurnQueue::VmLifetime* actual_lifetime=nullptr;
    try{
        p.worldview_require_callback_origin();
        bool owned=false;for(const auto& issued:p.native_network_backend_bindings)if(issued.get()==binding){owned=true;break;}
        if(!owned)throw ScriptError("Network binding retired during source FIRST");
        const auto life=binding->lifetime.lock();
        if(binding->retired||binding->actual_host!=&p||binding->actual_vm!=p.state||!binding->actual_callback||
           !life||life.get()!=p.end_turn_lifetime.get()||life.owner_before(p.end_turn_lifetime)||
           p.end_turn_lifetime.owner_before(life)||!life->alive||life->failed||life->vm!=p.state||life->host!=p.worldview_host_owner)
            throw ScriptError("Network actual backend/facade custody retired");
        callback=binding->actual_callback;actual_vm=binding->actual_vm;actual_lifetime=life.get();
    }catch(...){p.source_exception=std::current_exception();p.source_error="Network genuine backend FIRST refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    // No owning C++ local or speculative argument/result conversion crosses
    // this real Lua CFunction invocation. A future genuine issuer must supply
    // the complete original method body and its real backend custody.
    const int count=callback(L);native_db_query_current(L);
    p.source_exception={};p.source_error=nullptr;
    try{
        p.worldview_require_callback_origin();
        bool owned=false;for(const auto& issued:p.native_network_backend_bindings)if(issued.get()==binding){owned=true;break;}
        if(!owned)throw ScriptError("Network binding retired during callback/source POST");
        const auto life=binding->lifetime.lock();
        if(binding->retired||binding->original_function!=NativeFunction||binding->actual_host!=&p||
           binding->actual_vm!=actual_vm||actual_vm!=p.state||binding->actual_callback!=callback||
           !life||life.get()!=actual_lifetime||life.get()!=p.end_turn_lifetime.get()||
           life.owner_before(p.end_turn_lifetime)||p.end_turn_lifetime.owner_before(life)||!life->alive||life->failed||
           life->vm!=actual_vm||life->host!=p.worldview_host_owner||epoch!=p.failure_epoch||
           writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty)
            throw ScriptError("Network genuine callback/VM/weak lifetime/source changed");
    }catch(...){p.source_exception=std::current_exception();p.source_error="Network genuine callback/source POST refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    if(count<0||count>lua_gettop(L)){p.record_failure();return luaL_error(L,"Network actual callback returned invalid result count");}
    return count;
}
// Fresh PE636410 is exactly XOR EAX,EAX;RET. These original three aliases
// are zero-return functions, rather than invented successful no-op policies.
static int native_network_original_636410(lua_State* L){native_db_query_current(L);return 0;}
struct NativeNetworkEntry {const char* name;std::uint32_t original_function;lua_CFunction function;};
static constexpr NativeNetworkEntry native_network_entries[]={
    {"Connect",0x4e37b0,native_network_method<0x4e37b0>}, // 0x4a4624
    {"HttpRequest",0x778680,native_network_method<0x778680>}, // 0x4a463e
    {"SendChangeWar",0x6dc400,native_network_method<0x6dc400>}, // 0x4a465b
    {"SendPledgeMinorProtection",0x55f370,native_network_method<0x55f370>}, // 0x4a4675
    {"SendMinorNoUnitSpawning",0x4289a0,native_network_method<0x4289a0>}, // 0x4a468f
    {"SendLiberateMinor",0x7a05e0,native_network_method<0x7a05e0>}, // 0x4a46ac
    {"SendResearch",0x422a30,native_network_method<0x422a30>}, // 0x4a46c6
    {"SendDoTask",0x598530,native_network_method<0x598530>}, // 0x4a46e0
    {"SendCityBuyPlot",0x5a6b50,native_network_method<0x5a6b50>}, // 0x4a46fd
    {"SendSetCityAIFocus",0x468770,native_network_method<0x468770>}, // 0x4a4717
    {"SendSetCityAvoidGrowth",0x40c8b0,native_network_method<0x40c8b0>}, // 0x4a4731
    {"SendMinorCivEnterTerritory",0x4a8250,native_network_method<0x4a8250>}, // 0x4a474e
    {"SendUpdatePolicies",0x718a60,native_network_method<0x718a60>}, // 0x4a4768
    {"SendDiploVote",0x6a68a0,native_network_method<0x6a68a0>}, // 0x4a4782
    {"SendBarbarianRansom",0x4cd4f0,native_network_method<0x4cd4f0>}, // 0x4a479f
    {"SendGiftUnit",0x73fd70,native_network_method<0x73fd70>}, // 0x4a47b9
    {"SendReturnCivilian",0x535280,native_network_method<0x535280>}, // 0x4a47d3
    {"SendGreatPersonChoice",0x540f60,native_network_method<0x540f60>}, // 0x4a47f0
    {"SendMayaBonusChoice",0x43fce0,native_network_method<0x43fce0>}, // 0x4a480a
    {"SendFaithGreatPersonChoice",0x5a17a0,native_network_method<0x5a17a0>}, // 0x4a4824
    {"SendGoodyChoice",0x655170,native_network_method<0x655170>}, // 0x4a4841
    {"SendArchaeologyChoice",0x4c1390,native_network_method<0x4c1390>}, // 0x4a485b
    {"SendIdeologyChoice",0x6a15e0,native_network_method<0x6a15e0>}, // 0x4a4875
    {"SendExtendedGame",0x607e90,native_network_method<0x607e90>}, // 0x4a4892
    {"SendFoundPantheon",0x727de0,native_network_method<0x727de0>}, // 0x4a48ac
    {"SendFoundReligion",0x5e92f0,native_network_method<0x5e92f0>}, // 0x4a48c6
    {"SendEnhanceReligion",0x71d1c0,native_network_method<0x71d1c0>}, // 0x4a48e3
    {"SendMoveSpy",0x57e450,native_network_method<0x57e450>}, // 0x4a48fd
    {"SendStageCoup",0x78f890,native_network_method<0x78f890>}, // 0x4a4917
    {"SendFaithPurchase",0x744be0,native_network_method<0x744be0>}, // 0x4a4934
    {"SendLeagueVoteEnact",0x4abc00,native_network_method<0x4abc00>}, // 0x4a494e
    {"SendLeagueVoteRepeal",0x5a9820,native_network_method<0x5a9820>}, // 0x4a4968
    {"SendLeagueVoteAbstain",0x6e4ee0,native_network_method<0x6e4ee0>}, // 0x4a4985
    {"SendLeagueProposeEnact",0x6dd980,native_network_method<0x6dd980>}, // 0x4a499f
    {"SendLeagueProposeRepeal",0x78e200,native_network_method<0x78e200>}, // 0x4a49b9
    {"SendLeagueEditName",0x73de90,native_network_method<0x73de90>}, // 0x4a49d6
    {"SendIgnoreWarning",0x4f2010,native_network_method<0x4f2010>}, // 0x4a49f0
    {"SendSetSwappableGreatWork",0x5220d0,native_network_method<0x5220d0>}, // 0x4a4a0a
    {"SendSwapGreatWorks",0x4ee2a0,native_network_method<0x4ee2a0>}, // 0x4a4a27
    {"SendMoveGreatWorks",0x67c950,native_network_method<0x67c950>}, // 0x4a4a41
    {"SendChangeIdeology",0x6c3b20,native_network_method<0x6c3b20>}, // 0x4a4a5b
    {"SendUpdateCityCitizens",0x41ce70,native_network_method<0x41ce70>}, // 0x4a4a78
    {"SendSellBuilding",0x5aed60,native_network_method<0x5aed60>}, // 0x4a4a92
    {"SendRenameCity",0x7561f0,native_network_method<0x7561f0>}, // 0x4a4aac
    {"SendRenameUnit",0x5dc510,native_network_method<0x5dc510>}, // 0x4a4ac9
    {"SendTurnUnready",0x508d60,native_network_method<0x508d60>}, // 0x4a4ae3
    {"HasSentNetTurnComplete",0x4ac8d0,native_network_method<0x4ac8d0>}, // 0x4a4afd
    {"HasSentNetTurnAllComplete",0x5a2360,native_network_method<0x5a2360>}, // 0x4a4b1a
    {"GetSynchRandSeed",0x58c040,native_network_method<0x58c040>}, // 0x4a4b34
    {"GetMapRandSeed",0x436380,native_network_method<0x436380>}, // 0x4a4b4e
    {"ParseURL",0x6e7130,native_network_method<0x6e7130>}, // 0x4a4b6b
    {"URLEncode",0x42ce60,native_network_method<0x42ce60>}, // 0x4a4b85
    {"URLDecode",0x7abfb0,native_network_method<0x7abfb0>}, // 0x4a4b9f
    {"BroadcastGameSettings",0x67abe0,native_network_method<0x67abe0>}, // 0x4a4bbc
    {"BroadcastPlayerInfo",0x713a60,native_network_method<0x713a60>}, // 0x4a4bd6
    {"SendGameOptions",0x6f6fa0,native_network_method<0x6f6fa0>}, // 0x4a4bf0
    {"SendChat",0x487520,native_network_method<0x487520>}, // 0x4a4c0d
    {"HasReconnectCache",0x5adec0,native_network_method<0x5adec0>}, // 0x4a4c27
    {"Reconnect",0x4e0e90,native_network_method<0x4e0e90>}, // 0x4a4c41
    {"IsEveryoneConnected",0x48eea0,native_network_method<0x48eea0>}, // 0x4a4c5e
    {"IsPlayerConnected",0x52a6c0,native_network_method<0x52a6c0>}, // 0x4a4c78
    {"IsPlayerHotJoining",0x4bd230,native_network_method<0x4bd230>}, // 0x4a4c92
    {"IsPlayerKicked",0x5460a0,native_network_method<0x5460a0>}, // 0x4a4caf
    {"IsConnectedToSteam",0x4d5a80,native_network_method<0x4d5a80>}, // 0x4a4cc9
    {"CloseConnection",0x636410,native_network_original_636410}, // 0x4a4ce3
    {"ForceResync",0x636410,native_network_original_636410}, // 0x4a4d00
    {"QueueForceResync",0x636410,native_network_original_636410}, // 0x4a4d1a
    {"GetPingTime",0x6a09c0,native_network_method<0x6a09c0>}, // 0x4a4d34
    {"GetDebugLogLevel",0x465ae0,native_network_method<0x465ae0>}, // 0x4a4d51
    {"SetDebugLogLevel",0x6432c0,native_network_method<0x6432c0>}, // 0x4a4d6b
    {"GetPlayerTurnSliceInfo",0x518930,native_network_method<0x518930>}, // 0x4a4d85
    {"GetLocalTurnSliceInfo",0x467480,native_network_method<0x467480>}, // 0x4a4da2
    {"GetTurnSliceMaxMessageCount",0x73a430,native_network_method<0x73a430>}, // 0x4a4dbc
    {"SetTurnSliceMaxMessageCount",0x5148a0,native_network_method<0x5148a0>}, // 0x4a4dd6
    {"IsDedicatedServer",0x43b570,native_network_method<0x43b570>}, // 0x4a4df3
    {"AutoHost",0x727520,native_network_method<0x727520>}, // 0x4a4e0d
    {"AutoReadyHost",0x4a8d10,native_network_method<0x4a8d10>}, // 0x4a4e27
    {"SetPlayerDesiredSlot",0x714b30,native_network_method<0x714b30>}, // 0x4a4e44
    {"GetPlayerDesiredSlot",0x6c8620,native_network_method<0x6c8620>}, // 0x4a4e5e
};
struct NativeContextNetworkFrame final {
    Impl& host;const NativeContextUiEnumsFrame*const predecessor;
    const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;
    int keys_ref=LUA_NOREF;const void* keys=nullptr;
    int master_ref=LUA_NOREF,proxy_ref=LUA_NOREF;const void* master=nullptr;const void* proxy=nullptr;const void* meta=nullptr;
    std::size_t fields_created=0,stores=0;Job* actual_job=nullptr;
    const void* constructed_master=nullptr;
    explicit NativeContextNetworkFrame(Impl& p):host(p),predecessor(p.ui_enums_frame),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(host.network_frame||!predecessor)throw ScriptError("Network registrar requires actual UIEnums predecessor");
        predecessor->first();if(predecessor->master_ref<0||!predecessor->master)throw ScriptError("Network original predecessor did not execute original master/copies");
        host.network_frame=this;
    }
    ~NativeContextNetworkFrame(){if(host.network_frame==this)host.network_frame=nullptr;}
    void first()const {
        if(host.network_frame!=this||creator!=std::this_thread::get_id()||!predecessor||host.ui_enums_frame!=predecessor||
           epoch!=host.failure_epoch||writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty)
            throw ScriptError("Network registrar actual source/frame changed");
        predecessor->first();
    }
    void full()const {first();predecessor->full();first();}
};
// Retained physical publication data minted only after the actual selected
// Network store returned. Weak Source controls do not prolong the outside owner.
// Registry roots remain under this Host/VM custody until its real lua_close.
struct NativeContextNetworkNamespaceReturn final{
    lua_State*const vm;global_State*const global;
    const std::size_t child;const int environment_ref;const void*const environment;
    const std::weak_ptr<const ScriptUiChildSource> source;const ScriptUiChildSource*const source_get;
    const int network_keys_ref;const void*const network_keys;
    const int readonly_keys_ref;const void*const readonly_keys;
    const int master_globals_ref;const void*const master_globals;
    const int master_ref,proxy_ref;const void*const master;const void*const proxy;const void*const meta;
    NativeContextNetworkNamespaceReturn(const Impl& p,const NativeContextNetworkFrame& f)
        :vm(p.state),global(G(p.state)),child(p.lua_events_frame->child),
         environment_ref(p.lua_events_frame->environment),environment(p.lua_events_frame->actual_environment),
         source(p.child_contexts[child].source),source_get(p.child_contexts[child].source.get()),
         network_keys_ref(f.keys_ref),network_keys(f.keys),readonly_keys_ref(p.api_registration_frame->keys_ref),readonly_keys(p.api_registration_frame->keys),
         master_globals_ref(p.native_master_thread_bootstrap->globals_ref),master_globals(p.native_master_thread_bootstrap->globals),master_ref(f.master_ref),proxy_ref(f.proxy_ref),
         master(f.master),proxy(f.proxy),meta(f.meta){}
    NativeContextNetworkNamespaceReturn(const NativeContextNetworkNamespaceReturn&)=delete;
};
std::vector<std::unique_ptr<const NativeContextNetworkNamespaceReturn>> native_network_namespace_returns;
static void native_network_key(lua_State* L,int i){
    const auto& p=owner(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_network_registration_keys_ref);lua_rawgeti(L,-1,i);lua_remove(L,-2);
}
static void native_network_registration_full(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.network_frame;p.source_exception={};p.source_error=nullptr;
    try{if(!f)throw ScriptError("Network registrar has no genuine entered frame");f->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Network registrar SAME source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    if(f->actual_job&&f->actual_job!=&job)return (void)luaL_error(L,"Network registrar actual entered Job replaced");
    if(!f->predecessor->actual_job)return (void)luaL_error(L,"Network genuine UIEnums predecessor Job retired");
    native_ui_enums_full(L,*f->predecessor->actual_job);
    p.source_exception={};p.source_error=nullptr;
    try{if(p.network_frame!=f)throw ScriptError("Network frame replaced after source callbacks");f->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Network registrar FIRST after callbacks refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    const int top=lua_gettop(L);
    if(f->keys_ref!=LUA_NOREF){
        if(f->keys_ref!=p.native_network_registration_keys_ref)return (void)luaL_error(L,"Network registrar keys custody changed");
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->keys||lua_objlen(L,-1)!=80||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Network registrar retained key table changed");
        for(int i=0;i<80;++i){
            lua_rawgeti(L,-1,i+1);std::size_t n=0;const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
            if(!key||n!=std::strlen(i==0?"Network":native_network_entries[i-1].name)||std::memcmp(key,i==0?"Network":native_network_entries[i-1].name,n))
                return (void)luaL_error(L,"Network registrar retained key TString changed");lua_pop(L,1);
        }lua_pop(L,1);
    }
    if(f->master_ref>=0){
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->master)return (void)luaL_error(L,"Network actual master table changed");
        native_network_key(L,1);lua_rawget(L,LUA_GLOBALSINDEX);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Network original master cache changed");lua_pop(L,2);
    }
    if(f->stores){
        if(f->stores!=1||f->proxy_ref<0||!f->proxy||!f->meta||f->master_ref<0)
            return (void)luaL_error(L,"Network selected store lacks actual return");
        lua_rawgeti(L,LUA_REGISTRYINDEX,p.lua_events_frame->environment);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.lua_events_frame->actual_environment||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Network actual selected fenv changed");
        native_network_key(L,1);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->proxy_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->proxy||!lua_rawequal(L,-1,-2)||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=f->meta)
            return (void)luaL_error(L,"Network selected proxy/ref/metatable changed");
        native_api_prefix_key(L,2);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Network proxy actual master __index changed");lua_pop(L,2);
        native_api_prefix_key(L,3);lua_rawget(L,-2);
        if(lua_type(L,-1)!=LUA_TBOOLEAN||lua_toboolean(L,-1))return (void)luaL_error(L,"Network proxy original false __metatable changed");lua_pop(L,1);
        native_api_prefix_key(L,4);lua_rawget(L,-2);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_api_readonly_write||lua_getupvalue(L,-1,1))
            return (void)luaL_error(L,"Network proxy original write-error consumer changed");
    }
    lua_settop(L,top);
}
static int native_network_factory(lua_State* L){
    auto& p=owner(L);auto*const f=p.network_frame;
    if(!f||!f->actual_job)return luaL_error(L,"Native Network constructor lacks its actual entered Job");
    auto& job=*f->actual_job;
    native_network_registration_full(L,job);
    if(!f||f->fields_created||f->master_ref!=LUA_NOREF)return luaL_error(L,"Native Network4A4600 constructor replay refused");
    lua_createtable(L,0,0);const int table=lua_gettop(L);const void*const value=lua_topointer(L,table);native_network_registration_full(L,job);
    for(int i=0;i<79;++i){
        native_network_key(L,i+2);lua_pushcfunction(L,native_network_entries[i].function);lua_rawset(L,table);++f->fields_created;native_network_registration_full(L,job);
    }
    if(f->fields_created!=79||!lua_istable(L,table)||lua_topointer(L,table)!=value||lua_getmetatable(L,table))
        return luaL_error(L,"Native Network4A4600 actual namespace constructor changed");
    for(int i=0;i<79;++i){native_network_key(L,i+2);lua_rawget(L,table);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_network_entries[i].function||lua_getupvalue(L,-1,1))return luaL_error(L,"Network original zero-upvalue consumer changed");lua_pop(L,1);}
    f->constructed_master=value;
    return 1;
}
static void perform_native_context_network_registration(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.network_frame;native_network_registration_full(L,job);
    if(!f||f->stores||f->keys_ref!=LUA_NOREF||f->actual_job||!lua_checkstack(L,32))
        return (void)luaL_error(L,"Network original registrar replay/key custody/stack refused");
    f->actual_job=&job;
    if(p.native_network_registration_keys_ref==LUA_NOREF){
        lua_createtable(L,80,0);const int keys=lua_gettop(L);const void*const key_value=lua_topointer(L,keys);native_network_registration_full(L,job);
        for(int i=0;i<80;++i){lua_pushstring(L,i==0?"Network":native_network_entries[i-1].name);lua_rawseti(L,keys,i+1);native_network_registration_full(L,job);}
        lua_pushvalue(L,keys);const int key_ref=luaL_ref(L,LUA_REGISTRYINDEX);
        if(key_ref<0)return (void)luaL_error(L,"Network actual retained key allocation failed");
        f->keys_ref=key_ref;f->keys=key_value;p.native_network_registration_keys_ref=key_ref;p.native_network_registration_keys_pointer=key_value;
        native_network_registration_full(L,job);lua_pop(L,1);
    }else{
        f->keys_ref=p.native_network_registration_keys_ref;f->keys=p.native_network_registration_keys_pointer;native_network_registration_full(L,job);
    }
    // Original6DFB60/5605C0 selects actual518 before inspecting
    // GLOBALS.Network; a cache miss runs4A4600 zero args/one result.
    perform_native_context_state_query(L,job);native_network_registration_full(L,job);const int environment=lua_gettop(L);
    if(lua_topointer(L,environment)!=p.lua_events_frame->actual_environment)
        return (void)luaL_error(L,"Network registrar actual prepared518 environment changed");
    native_network_key(L,1);lua_gettable(L,LUA_GLOBALSINDEX);native_network_registration_full(L,job);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);lua_pushcfunction(L,native_network_factory);lua_call(L,0,1);native_network_registration_full(L,job);
        native_network_key(L,1);lua_pushvalue(L,-2);lua_settable(L,LUA_GLOBALSINDEX);native_network_registration_full(L,job);
    }
    if(!lua_istable(L,-1))return (void)luaL_error(L,"Network original namespace master is not a table");
    bool genuine_master=f->fields_created==79&&f->constructed_master==lua_topointer(L,-1);
    if(!genuine_master)for(const auto& prior:p.native_network_namespace_returns)
        if(prior&&prior->vm==p.state&&prior->global==G(L)&&prior->master==lua_topointer(L,-1)){
            lua_rawgeti(L,LUA_REGISTRYINDEX,prior->master_ref);
            genuine_master=lua_istable(L,-1)&&lua_topointer(L,-1)==prior->master&&lua_rawequal(L,-1,-2);
            lua_pop(L,1);if(genuine_master)break;
        }
    if(!genuine_master)return (void)luaL_error(L,"Network master cache lacks its actual own constructor/publication return");
    f->master=lua_topointer(L,-1);lua_pushvalue(L,-1);f->master_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_network_registration_full(L,job);
    native_api_readonly_proxy(L,job);native_network_registration_full(L,job);const int proxy=lua_gettop(L);f->proxy=lua_topointer(L,proxy);
    if(!lua_getmetatable(L,proxy))return (void)luaL_error(L,"Network original49B190 proxy has no metatable");f->meta=lua_topointer(L,-1);lua_pop(L,1);
    native_network_key(L,1);lua_pushvalue(L,proxy);lua_settable(L,environment);native_network_registration_full(L,job);
    lua_pushvalue(L,proxy);f->proxy_ref=luaL_ref(L,LUA_REGISTRYINDEX);++f->stores;native_network_registration_full(L,job);
    p.source_exception={};p.source_error=nullptr;
    try{
        f->first();const auto child=p.lua_events_frame->child;
        if(child>=p.child_contexts.size()||p.child_contexts[child].retired||!p.child_contexts[child].source||
           !p.api_registration_frame||p.api_registration_frame->keys_ref<0||!p.api_registration_frame->keys||
           !p.native_master_thread_bootstrap||p.native_master_thread_bootstrap->globals_ref<0||!p.native_master_thread_bootstrap->globals)
            throw ScriptError("Network selected publication has no actual retained child/source");
        for(const auto& prior:p.native_network_namespace_returns)
            if(prior&&prior->child==child)throw ScriptError("Network same selected child publication repeated");
        p.native_network_namespace_returns.push_back(std::unique_ptr<const NativeContextNetworkNamespaceReturn>(new NativeContextNetworkNamespaceReturn(p,*f)));
        f->first();
    }catch(...){p.source_exception=std::current_exception();p.source_error="Network actual namespace return custody refused";}
    if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
    native_network_registration_full(L,job);
    job.context_network_master_reference=f->master_ref;job.context_network_proxy_reference=f->proxy_ref;
    job.context_network_table=f->master;job.context_network_proxy=f->proxy;lua_pop(L,2);
}

static void native_network_retained_proxy_key_owned(lua_State* L,const NativeContextNetworkNamespaceReturn& actual,int i){
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual.readonly_keys_ref);lua_rawgeti(L,-1,i);lua_remove(L,-2);
}
// Called by the later genuine selected installer; never admits a caller table
// by shape alone. Return false retains its existing foreign-namespace refusal.
static bool native_network_selected_namespace_physical_owned(lua_State* L,int environment){
    auto& p=owner(L);
    const int top=lua_gettop(L);if(environment<0)environment=top+environment+1;
    if(environment<=0||environment>top||!lua_istable(L,environment))return false;
    const NativeContextNetworkNamespaceReturn* actual=nullptr;
    for(const auto& candidate:p.native_network_namespace_returns)
        if(candidate&&candidate->environment==lua_topointer(L,environment)){
            if(actual)return false;actual=candidate.get();
        }
    if(!actual||!p.state||actual->vm!=p.state||actual->global!=G(L)||G(p.state)!=actual->global||
       actual->child>=p.child_contexts.size()||actual->master_ref<0||actual->proxy_ref<0||actual->readonly_keys_ref<0||!actual->readonly_keys||
       actual->network_keys_ref<0||!actual->network_keys||p.native_network_registration_keys_ref!=actual->network_keys_ref||
       p.native_network_registration_keys_pointer!=actual->network_keys||
       actual->master_globals_ref<0||!actual->master_globals||!ttistable(gt(p.state))||hvalue(gt(p.state))!=actual->master_globals||
       !actual->master||!actual->proxy||!actual->meta||actual->source.expired())return false;
    if(!p.worldview_native_host||p.worldview_native_host->child!=actual->child||
       actual->source.owner_before(p.worldview_native_host->child_source)||p.worldview_native_host->child_source.owner_before(actual->source)||
       actual->source_get!=p.child_contexts[actual->child].source.get())return false;
    try{if(!p.worldview_native_child_controls_current(actual->child))return false;}
    catch(...){return false;}
    const auto& child=p.child_contexts[actual->child];
    if(child.retired||!child.state_prepared||child.environment!=actual->environment_ref||!child.source||child.source.get()!=actual->source_get||
       actual->source.owner_before(child.source)||child.source.owner_before(actual->source))return false;
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->network_keys_ref);
    bool network_keys_current=lua_istable(L,-1)&&lua_topointer(L,-1)==actual->network_keys&&lua_objlen(L,-1)==80&&!lua_getmetatable(L,-1);
    if(network_keys_current)for(int i=0;i<80;++i){lua_rawgeti(L,-1,i+1);std::size_t n=0;
        const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        const char*const expected=i==0?"Network":native_network_entries[i-1].name;
        network_keys_current=key&&n==std::strlen(expected)&&!std::memcmp(key,expected,n);
        lua_pop(L,1);if(!network_keys_current)break;
    }
    lua_settop(L,top);if(!network_keys_current)return false;
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->readonly_keys_ref);
    bool keys_current=lua_istable(L,-1)&&lua_topointer(L,-1)==actual->readonly_keys&&lua_objlen(L,-1)==4&&!lua_getmetatable(L,-1);
    if(keys_current)for(int i=0;i<4;++i){lua_rawgeti(L,-1,i+1);std::size_t n=0;
        const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        keys_current=key&&n==std::strlen(native_api_prefix_keys[i])&&!std::memcmp(key,native_api_prefix_keys[i],n);
        lua_pop(L,1);if(!keys_current)break;
    }
    lua_settop(L,top);if(!keys_current)return false;
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->environment_ref);
    bool current=lua_istable(L,-1)&&lua_rawequal(L,-1,environment)&&!lua_getmetatable(L,-1);
    lua_settop(L,top);if(!current)return false;
    native_network_key(L,1);lua_rawget(L,environment);lua_rawgeti(L,LUA_REGISTRYINDEX,actual->proxy_ref);
    current=lua_istable(L,-1)&&lua_topointer(L,-1)==actual->proxy&&lua_rawequal(L,-1,-2)&&
        lua_getmetatable(L,-1)&&lua_topointer(L,-1)==actual->meta;
    lua_settop(L,top);if(!current)return false;
    // Original49B190 creates an empty readonly proxy. A raw own field can
    // shadow its genuine master despite an unchanged readonly metatable.
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->proxy_ref);lua_pushnil(L);
    current=lua_next(L,-2)==0;lua_settop(L,top);if(!current)return false;
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->master_ref);const int master=lua_gettop(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->master_globals_ref);
    current=lua_istable(L,-1)&&lua_topointer(L,-1)==actual->master_globals;
    if(!current){lua_settop(L,top);return false;}
    native_network_key(L,1);lua_rawget(L,-2);lua_remove(L,-2);
    current=lua_istable(L,master)&&lua_topointer(L,master)==actual->master&&lua_rawequal(L,-1,master)&&!lua_getmetatable(L,master);
    lua_settop(L,master);if(!current){lua_settop(L,top);return false;}
    for(int i=0;i<79;++i){native_network_key(L,i+2);lua_rawget(L,master);
        current=lua_iscfunction(L,-1)&&lua_tocfunction(L,-1)==native_network_entries[i].function&&!lua_getupvalue(L,-1,1);
        lua_settop(L,master);if(!current){lua_settop(L,top);return false;}
    }
    lua_settop(L,top);lua_rawgeti(L,LUA_REGISTRYINDEX,actual->proxy_ref);lua_getmetatable(L,-1);const int meta=lua_gettop(L);
    native_network_retained_proxy_key_owned(L,*actual,2);lua_rawget(L,meta);lua_rawgeti(L,LUA_REGISTRYINDEX,actual->master_ref);
    current=lua_rawequal(L,-1,-2);lua_settop(L,meta);
    native_network_retained_proxy_key_owned(L,*actual,3);lua_rawget(L,meta);
    current=current&&lua_type(L,-1)==LUA_TBOOLEAN&&!lua_toboolean(L,-1);lua_settop(L,meta);
    native_network_retained_proxy_key_owned(L,*actual,4);lua_rawget(L,meta);
    current=current&&lua_iscfunction(L,-1)&&lua_tocfunction(L,-1)==native_api_readonly_write&&!lua_getupvalue(L,-1,1);
    lua_settop(L,top);return current;
}
static bool native_network_selected_namespace_current_owned(lua_State* L,int environment){
    if(!lua_checkstack(L,16))return false;
    native_db_query_current(L);worldview_require_lua_origin(L);
    if(!native_network_selected_namespace_physical_owned(L,environment))return false;
    // Reacquire all record/source/table data after callbackful Source POST.
    // No source borrower or stale record pointer crosses this boundary.
    native_db_query_current(L);worldview_require_lua_origin(L);
    return native_network_selected_namespace_physical_owned(L,environment);
}
