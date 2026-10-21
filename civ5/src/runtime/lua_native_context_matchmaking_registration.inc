// Original45741B ->5BD5C0 ->6DFB60/5605C0 installs Matchmaking.
// Complete5B3030..5B3356:29 named zero-upvalue lua_CFunction consumers.
// Namespace registration does not execute any lobby/game/Steam callback.
// Actual consumer backends are not issued by this constructor; invocation
// refuses under the existing canonical Host/Source gate until genuinely supplied.
template<std::uint32_t NativeFunction>static int native_matchmaking_method(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);
    p.record_failure();
    return luaL_error(L,"Matchmaking native consumer %08X requires its genuine engine body/backend; invoke provider unissued",
                      static_cast<unsigned>(NativeFunction));
}
struct NativeMatchmakingEntry {const char* name;std::uint32_t original_function;lua_CFunction function;};
static constexpr NativeMatchmakingEntry native_matchmaking_entries[]={
    {"InitInternetLobby",0x5D1430,native_matchmaking_method<0x5D1430>}, // setfield005B3054
    {"InitLanLobby",0x596900,native_matchmaking_method<0x596900>}, // setfield005B306E
    {"RefreshLANGameList",0x47DC80,native_matchmaking_method<0x47DC80>}, // setfield005B308B
    {"RefreshInternetGameList",0x72DDF0,native_matchmaking_method<0x72DDF0>}, // setfield005B30A5
    {"GetMultiplayerGameList",0x4F39F0,native_matchmaking_method<0x4F39F0>}, // setfield005B30BF
    {"GetMultiplayerServerEntry",0x4E6BC0,native_matchmaking_method<0x4E6BC0>}, // setfield005B30DC
    {"GetMultiplayerGameCount",0x793DA0,native_matchmaking_method<0x793DA0>}, // setfield005B30F6
    {"SetMultiplayerGameListType",0x41C270,native_matchmaking_method<0x41C270>}, // setfield005B3110
    {"GetMultiplayerGameListType",0x437D10,native_matchmaking_method<0x437D10>}, // setfield005B312D
    {"IsRefreshingGameList",0x5977C0,native_matchmaking_method<0x5977C0>}, // setfield005B3147
    {"StopRefreshingGameList",0x49ACD0,native_matchmaking_method<0x49ACD0>}, // setfield005B3161
    {"HostLANGame",0x5E9760,native_matchmaking_method<0x5E9760>}, // setfield005B317E
    {"HostInternetGame",0x628530,native_matchmaking_method<0x628530>}, // setfield005B3198
    {"HostHotSeatGame",0x783EC0,native_matchmaking_method<0x783EC0>}, // setfield005B31B2
    {"HostServerGame",0x4B2D60,native_matchmaking_method<0x4B2D60>}, // setfield005B31CF
    {"JoinIPAddress",0x503B80,native_matchmaking_method<0x503B80>}, // setfield005B31E9
    {"JoinMultiplayerGame",0x6B3910,native_matchmaking_method<0x6B3910>}, // setfield005B3203
    {"LeaveMultiplayerGame",0x6D6560,native_matchmaking_method<0x6D6560>}, // setfield005B3220
    {"LaunchMultiplayerGame",0x46CF20,native_matchmaking_method<0x46CF20>}, // setfield005B323A
    {"IsLaunchingGame",0x67AE60,native_matchmaking_method<0x67AE60>}, // setfield005B3254
    {"GetPlayerList",0x434980,native_matchmaking_method<0x434980>}, // setfield005B3271
    {"GetPlayerCount",0x5606C0,native_matchmaking_method<0x5606C0>}, // setfield005B328B
    {"GetCurrentGameName",0x6B4500,native_matchmaking_method<0x6B4500>}, // setfield005B32A5
    {"GetFriendList",0x613C80,native_matchmaking_method<0x613C80>}, // setfield005B32C2
    {"IsHost",0x780D10,native_matchmaking_method<0x780D10>}, // setfield005B32DC
    {"GetHostID",0x58B4A0,native_matchmaking_method<0x58B4A0>}, // setfield005B32F6
    {"GetLocalID",0x762030,native_matchmaking_method<0x762030>}, // setfield005B3313
    {"SetMultiplayerResumeGame",0x5A0560,native_matchmaking_method<0x5A0560>}, // setfield005B332D
    {"KickPlayer",0x4E2BF0,native_matchmaking_method<0x4E2BF0>}, // setfield005B3347
};
struct NativeContextMatchmakingFrame final {
    Impl& host;const NativeContextUiRegistrationFrame*const predecessor;
    const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;
    int keys_ref=LUA_NOREF;const void* keys=nullptr;
    int master_ref=LUA_NOREF,proxy_ref=LUA_NOREF;const void* master=nullptr;const void* proxy=nullptr;const void* meta=nullptr;
    std::size_t fields_created=0,stores=0;Job* actual_job=nullptr;
    explicit NativeContextMatchmakingFrame(Impl& p):host(p),predecessor(p.ui_registration_frame),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(host.matchmaking_frame||!predecessor)throw ScriptError("Matchmaking registrar requires actual UI predecessor");
        predecessor->first();if(predecessor->stores!=1||predecessor->master_ref<0||predecessor->proxy_ref<0||!predecessor->master||!predecessor->proxy)throw ScriptError("Matchmaking original predecessor did not execute original master/copies");
        host.matchmaking_frame=this;
    }
    ~NativeContextMatchmakingFrame(){if(host.matchmaking_frame==this)host.matchmaking_frame=nullptr;}
    void first()const {
        if(host.matchmaking_frame!=this||creator!=std::this_thread::get_id()||!predecessor||host.ui_registration_frame!=predecessor||
           epoch!=host.failure_epoch||writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty)
            throw ScriptError("Matchmaking registrar actual source/frame changed");
        predecessor->first();
    }
    void full()const {first();predecessor->full();first();}
};
static void native_matchmaking_key(lua_State* L,int i){
    const auto& p=owner(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_matchmaking_registration_keys_ref);lua_rawgeti(L,-1,i);lua_remove(L,-2);
}
static void native_matchmaking_registration_full(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.matchmaking_frame;p.source_exception={};p.source_error=nullptr;
    try{if(!f)throw ScriptError("Matchmaking registrar has no genuine entered frame");f->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Matchmaking registrar SAME source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    if(f->actual_job&&f->actual_job!=&job)return (void)luaL_error(L,"Matchmaking registrar actual entered Job replaced");
    if(!f->predecessor->actual_job)return (void)luaL_error(L,"Matchmaking genuine UI predecessor Job retired");
    native_context_ui_registration_full(L,*f->predecessor->actual_job);
    p.source_exception={};p.source_error=nullptr;
    try{if(p.matchmaking_frame!=f)throw ScriptError("Matchmaking frame replaced after source callbacks");f->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Matchmaking registrar FIRST after callbacks refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    const int top=lua_gettop(L);
    if(f->keys_ref!=LUA_NOREF){
        if(f->keys_ref!=p.native_matchmaking_registration_keys_ref)return (void)luaL_error(L,"Matchmaking registrar keys custody changed");
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->keys||lua_objlen(L,-1)!=30||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Matchmaking registrar retained key table changed");
        for(int i=0;i<30;++i){
            lua_rawgeti(L,-1,i+1);std::size_t n=0;const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
            if(!key||n!=std::strlen(i==0?"Matchmaking":native_matchmaking_entries[i-1].name)||std::memcmp(key,i==0?"Matchmaking":native_matchmaking_entries[i-1].name,n))
                return (void)luaL_error(L,"Matchmaking registrar retained key TString changed");lua_pop(L,1);
        }lua_pop(L,1);
    }
    if(f->master_ref>=0){
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->master)return (void)luaL_error(L,"Matchmaking actual master table changed");
        native_matchmaking_key(L,1);lua_rawget(L,LUA_GLOBALSINDEX);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Matchmaking original master cache changed");lua_pop(L,2);
    }
    if(f->stores){
        if(f->stores!=1||f->proxy_ref<0||!f->proxy||!f->meta||f->master_ref<0)
            return (void)luaL_error(L,"Matchmaking selected store lacks actual return");
        lua_rawgeti(L,LUA_REGISTRYINDEX,p.lua_events_frame->environment);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.lua_events_frame->actual_environment||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Matchmaking actual selected fenv changed");
        native_matchmaking_key(L,1);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->proxy_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->proxy||!lua_rawequal(L,-1,-2)||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=f->meta)
            return (void)luaL_error(L,"Matchmaking selected proxy/ref/metatable changed");
        native_api_prefix_key(L,2);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Matchmaking proxy actual master __index changed");lua_pop(L,2);
        native_api_prefix_key(L,3);lua_rawget(L,-2);
        if(lua_type(L,-1)!=LUA_TBOOLEAN||lua_toboolean(L,-1))return (void)luaL_error(L,"Matchmaking proxy original false __metatable changed");lua_pop(L,1);
        native_api_prefix_key(L,4);lua_rawget(L,-2);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_api_readonly_write||lua_getupvalue(L,-1,1))
            return (void)luaL_error(L,"Matchmaking proxy original write-error consumer changed");
    }
    lua_settop(L,top);
}
static int native_matchmaking_factory(lua_State* L){
    auto& p=owner(L);auto*const f=p.matchmaking_frame;
    if(!f||!f->actual_job)return luaL_error(L,"Native Matchmaking constructor lacks its actual entered Job");
    auto& job=*f->actual_job;
    native_matchmaking_registration_full(L,job);
    if(!f||f->fields_created||f->master_ref!=LUA_NOREF)return luaL_error(L,"Native Matchmaking5B3030 constructor replay refused");
    lua_createtable(L,0,0);const int table=lua_gettop(L);const void*const value=lua_topointer(L,table);native_matchmaking_registration_full(L,job);
    for(int i=0;i<29;++i){
        native_matchmaking_key(L,i+2);lua_pushcfunction(L,native_matchmaking_entries[i].function);lua_rawset(L,table);++f->fields_created;native_matchmaking_registration_full(L,job);
    }
    if(f->fields_created!=29||!lua_istable(L,table)||lua_topointer(L,table)!=value||lua_getmetatable(L,table))
        return luaL_error(L,"Native Matchmaking5B3030 actual namespace constructor changed");
    for(int i=0;i<29;++i){native_matchmaking_key(L,i+2);lua_rawget(L,table);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_matchmaking_entries[i].function||lua_getupvalue(L,-1,1))return luaL_error(L,"Matchmaking original zero-upvalue consumer changed");lua_pop(L,1);}
    return 1;
}
static void perform_native_context_matchmaking_registration(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.matchmaking_frame;native_matchmaking_registration_full(L,job);
    if(!f||f->stores||f->keys_ref!=LUA_NOREF||f->actual_job||!lua_checkstack(L,32))
        return (void)luaL_error(L,"Matchmaking original registrar replay/key custody/stack refused");
    f->actual_job=&job;
    if(p.native_matchmaking_registration_keys_ref==LUA_NOREF){
        lua_createtable(L,30,0);const int keys=lua_gettop(L);const void*const key_value=lua_topointer(L,keys);native_matchmaking_registration_full(L,job);
        for(int i=0;i<30;++i){lua_pushstring(L,i==0?"Matchmaking":native_matchmaking_entries[i-1].name);lua_rawseti(L,keys,i+1);native_matchmaking_registration_full(L,job);}
        lua_pushvalue(L,keys);const int key_ref=luaL_ref(L,LUA_REGISTRYINDEX);
        if(key_ref<0)return (void)luaL_error(L,"Matchmaking actual retained key allocation failed");
        f->keys_ref=key_ref;f->keys=key_value;p.native_matchmaking_registration_keys_ref=key_ref;p.native_matchmaking_registration_keys_pointer=key_value;
        native_matchmaking_registration_full(L,job);lua_pop(L,1);
    }else{
        f->keys_ref=p.native_matchmaking_registration_keys_ref;f->keys=p.native_matchmaking_registration_keys_pointer;native_matchmaking_registration_full(L,job);
    }
    // Original6DFB60/5605C0 selects actual518 before inspecting
    // GLOBALS.Matchmaking; a cache miss runs5B3030 zero args/one result.
    perform_native_context_state_query(L,job);native_matchmaking_registration_full(L,job);const int environment=lua_gettop(L);
    if(lua_topointer(L,environment)!=p.lua_events_frame->actual_environment)
        return (void)luaL_error(L,"Matchmaking registrar actual prepared518 environment changed");
    native_matchmaking_key(L,1);lua_gettable(L,LUA_GLOBALSINDEX);native_matchmaking_registration_full(L,job);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);lua_pushcfunction(L,native_matchmaking_factory);lua_call(L,0,1);native_matchmaking_registration_full(L,job);
        native_matchmaking_key(L,1);lua_pushvalue(L,-2);lua_settable(L,LUA_GLOBALSINDEX);native_matchmaking_registration_full(L,job);
    }
    if(!lua_istable(L,-1))return (void)luaL_error(L,"Matchmaking original namespace master is not a table");
    f->master=lua_topointer(L,-1);lua_pushvalue(L,-1);f->master_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_matchmaking_registration_full(L,job);
    native_api_readonly_proxy(L,job);native_matchmaking_registration_full(L,job);const int proxy=lua_gettop(L);f->proxy=lua_topointer(L,proxy);
    if(!lua_getmetatable(L,proxy))return (void)luaL_error(L,"Matchmaking original49B190 proxy has no metatable");f->meta=lua_topointer(L,-1);lua_pop(L,1);
    native_matchmaking_key(L,1);lua_pushvalue(L,proxy);lua_settable(L,environment);native_matchmaking_registration_full(L,job);
    lua_pushvalue(L,proxy);f->proxy_ref=luaL_ref(L,LUA_REGISTRYINDEX);++f->stores;native_matchmaking_registration_full(L,job);
    job.context_matchmaking_master_reference=f->master_ref;job.context_matchmaking_proxy_reference=f->proxy_ref;
    job.context_matchmaking_master_table=f->master;job.context_matchmaking_proxy_table=f->proxy;lua_pop(L,2);
}

// Private operation/registry custody inside the actual Host::Impl.
NativeContextMatchmakingFrame* matchmaking_frame=nullptr;
int native_matchmaking_registration_keys_ref=LUA_NOREF;
const void* native_matchmaking_registration_keys_pointer=nullptr;
