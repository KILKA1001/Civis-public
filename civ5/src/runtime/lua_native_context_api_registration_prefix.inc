// Inside Host::Impl, after the actual Events and LuaEvents bodies. Each
// original API registrar executes on the same prepared518 selected fenv.
struct NativeContextApiRegistrationFrame final {
    Impl& host;const NativeContextLuaEventsFrame*const predecessor;
    const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;
    int keys_ref=LUA_NOREF;const void* keys=nullptr;
    int art_info_ref=LUA_NOREF;int art_info_type=LUA_TNONE;
    std::size_t assigned_count=0; // successful real field stores, never caller flags
    explicit NativeContextApiRegistrationFrame(Impl& p):host(p),predecessor(p.lua_events_frame),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(host.api_registration_frame||!predecessor)throw ScriptError("API registrars require their actual entered LuaEvents predecessor");
        predecessor->first();const auto& child=host.child_contexts[predecessor->child];
        if(child.lua_events_reference<0||!child.lua_events_pointer)
            throw ScriptError("API registrars require genuine selected LuaEvents registration return");
        host.api_registration_frame=this;
    }
    ~NativeContextApiRegistrationFrame(){if(host.api_registration_frame==this)host.api_registration_frame=nullptr;}
    void first()const {
        if(host.api_registration_frame!=this||creator!=std::this_thread::get_id()||!predecessor||
           host.lua_events_frame!=predecessor||epoch!=host.failure_epoch||writes!=host.failed.writes_||
           uncertainty!=host.execution_uncertainty)
            throw ScriptError("API registrar actual predecessor/failure/lexical frame changed");
        predecessor->first();
    }
    void full()const {first();predecessor->full();first();}
};
static constexpr const char*const native_api_prefix_keys[]={"ArtInfo","__index","__metatable","__newindex"};
static void native_api_prefix_key(lua_State* L,int index){
    const auto*const f=owner(L).api_registration_frame;
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,index);lua_remove(L,-2);
}
static void native_api_prefix_physical(lua_State* L,const NativeContextApiRegistrationFrame& f){
    const int top=lua_gettop(L);
    if(f.keys_ref!=LUA_NOREF){
        lua_rawgeti(L,LUA_REGISTRYINDEX,f.keys_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f.keys||lua_objlen(L,-1)!=4||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"API registrar actual retained keys changed");
        for(int i=0;i<4;++i){
            lua_rawgeti(L,-1,i+1);std::size_t n=0;
            const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
            if(!key||n!=std::strlen(native_api_prefix_keys[i])||std::memcmp(key,native_api_prefix_keys[i],n))
                return (void)luaL_error(L,"API registrar actual key TString changed");
            lua_pop(L,1);
        }lua_pop(L,1);
    }
    if(f.assigned_count){
        if(f.assigned_count!=1||f.art_info_type==LUA_TNONE||f.art_info_ref==LUA_NOREF)
            return (void)luaL_error(L,"ArtInfo original68AA80 field store lacks its real return");
        lua_rawgeti(L,LUA_REGISTRYINDEX,f.predecessor->environment);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f.predecessor->actual_environment||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"ArtInfo original selected fenv changed");
        native_api_prefix_key(L,1);lua_rawget(L,-2);
        if(lua_type(L,-1)!=f.art_info_type)return (void)luaL_error(L,"ArtInfo selected original TValue type changed");
        native_api_prefix_key(L,1);lua_rawget(L,LUA_GLOBALSINDEX);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"ArtInfo master-to-selected original VALUE changed");
        if(f.art_info_ref==LUA_REFNIL){
            if(!lua_isnil(L,-1))return (void)luaL_error(L,"ArtInfo actual original nil value changed");
        }else{
            lua_rawgeti(L,LUA_REGISTRYINDEX,f.art_info_ref);
            if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"ArtInfo retained actual master value changed");
        }
    }
    lua_settop(L,top);
}
static void native_api_prefix_full(lua_State* L,Job& job){
    auto& p=owner(L);const auto*const f=p.api_registration_frame;
    p.source_exception={};p.source_error=nullptr;
    try{if(!f)throw ScriptError("API registration has no actual entered frame");f->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="API registrar SAME load/source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    native_lua_events_full(L,job);
    p.source_exception={};p.source_error=nullptr;
    try{if(p.api_registration_frame!=f)throw ScriptError("API registration frame replaced after Source callbacks");f->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="API registrar FIRST after callbacks refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    const int top=lua_gettop(L);const auto& child=p.child_contexts[f->predecessor->child];
    if(child.lua_events_reference<0||!child.lua_events_pointer||child.lua_events_reference!=job.context_lua_events_reference||
       child.lua_events_pointer!=job.context_lua_events_table||!native_lua_events_factory_without_callbacks(L))
        return (void)luaL_error(L,"API registrar genuine selected LuaEvents predecessor changed");
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->predecessor->environment);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->predecessor->actual_environment||lua_getmetatable(L,-1))
        return (void)luaL_error(L,"API registrar actual prepared518 fenv changed");
    native_lua_events_key(L,3);lua_rawget(L,-2);
    lua_rawgeti(L,LUA_REGISTRYINDEX,child.lua_events_reference);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=child.lua_events_pointer||!lua_rawequal(L,-1,-2)||
       !lua_getmetatable(L,-1)||lua_topointer(L,-1)!=p.lua_events_factory_pointer)
        return (void)luaL_error(L,"API registrar selected LuaEvents field/metatable/ref changed");
    lua_settop(L,top);native_api_prefix_physical(L,*f);
}
static int native_api_readonly_write(lua_State* L){
    return luaL_error(L,"Attempt to modify read-only table"); // actual5C26D0/A678F4
}
static void native_api_readonly_proxy(lua_State* L,Job& job){
    native_api_prefix_full(L,job);
    if(!lua_istable(L,-1))return (void)luaL_error(L,"Native49B190 requires its actual namespace table");
    const int actual=lua_gettop(L);const void*const master=lua_topointer(L,actual);
    lua_createtable(L,0,0);const int proxy=lua_gettop(L);const void*const proxy_value=lua_topointer(L,proxy);
    native_api_prefix_full(L,job);
    lua_createtable(L,0,0);const int meta=lua_gettop(L);const void*const meta_value=lua_topointer(L,meta);
    native_api_prefix_full(L,job);
    native_api_prefix_key(L,2);lua_pushvalue(L,actual);lua_rawset(L,meta);native_api_prefix_full(L,job);
    native_api_prefix_key(L,3);lua_pushboolean(L,0);lua_rawset(L,meta);native_api_prefix_full(L,job);
    native_api_prefix_key(L,4);lua_pushcfunction(L,native_api_readonly_write);lua_rawset(L,meta);native_api_prefix_full(L,job);
    // All actual objects remain stack-rooted, and every allocating original
    // operation was followed by FULL before final physical reads/assignment.
    if(!lua_istable(L,actual)||lua_topointer(L,actual)!=master||!lua_istable(L,proxy)||lua_topointer(L,proxy)!=proxy_value||
       !lua_istable(L,meta)||lua_topointer(L,meta)!=meta_value||lua_getmetatable(L,proxy)||lua_getmetatable(L,meta))
        return (void)luaL_error(L,"Native49B190 actual namespace/proxy/metatable changed after FULL");
    native_api_prefix_key(L,2);lua_rawget(L,meta);
    if(!lua_rawequal(L,-1,actual))return (void)luaL_error(L,"Native49B190 actual __index namespace changed");lua_pop(L,1);
    native_api_prefix_key(L,3);lua_rawget(L,meta);
    if(lua_type(L,-1)!=LUA_TBOOLEAN||lua_toboolean(L,-1))return (void)luaL_error(L,"Native49B190 __metatable=false changed");lua_pop(L,1);
    native_api_prefix_key(L,4);lua_rawget(L,meta);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_api_readonly_write||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Native49B190 actual zero-upvalue write error changed");lua_pop(L,1);
    lua_setmetatable(L,proxy);lua_remove(L,actual); // original8F9796 lua_remove
}
static void perform_native_context_api_registration_prefix(lua_State* L,Job& job){
    auto& p=owner(L);native_api_prefix_full(L,job);auto*const f=p.api_registration_frame;
    if(!f||f->assigned_count||f->keys_ref!=LUA_NOREF||f->art_info_ref!=LUA_NOREF||!lua_checkstack(L,24))
        return (void)luaL_error(L,"API original620DF0 first registrar replay/stack refused");
    native_api_prefix_full(L,job);
    lua_createtable(L,4,0);const int keys=lua_gettop(L);const void*const keys_value=lua_topointer(L,keys);
    native_api_prefix_full(L,job);
    for(int i=0;i<4;++i){lua_pushstring(L,native_api_prefix_keys[i]);lua_rawseti(L,keys,i+1);native_api_prefix_full(L,job);}
    if(!lua_istable(L,keys)||lua_topointer(L,keys)!=keys_value||lua_getmetatable(L,keys))
        return (void)luaL_error(L,"API actual key allocation changed before retention");
    lua_pushvalue(L,keys);const int keys_reference=luaL_ref(L,LUA_REGISTRYINDEX);
    if(keys_reference<0)return (void)luaL_error(L,"API actual retained key allocation failed");
    f->keys_ref=keys_reference;f->keys=keys_value;native_api_prefix_full(L,job);lua_pop(L,1);
    // Original68AA80 looks up master.ArtInfo, calls lua_type (discarded),
    // selects actual518 via534B50 and copies the exact VALUE. It deliberately
    // has no nil/type branch or constructor. No ArtInfo data is invented.
    native_api_prefix_key(L,1);lua_gettable(L,LUA_GLOBALSINDEX);native_api_prefix_full(L,job);
    const int art=lua_gettop(L);const int actual_type=lua_type(L,art);
    perform_native_context_state_query(L,job);native_api_prefix_full(L,job);
    const int environment=lua_gettop(L);
    if(lua_topointer(L,environment)!=f->predecessor->actual_environment||job.context_state_thread!=f->predecessor->actual_thread)
        return (void)luaL_error(L,"ArtInfo68AA80 genuine same selected518 changed");
    native_api_prefix_key(L,1);lua_pushvalue(L,art);lua_settable(L,environment);native_api_prefix_full(L,job);
    lua_pushvalue(L,art);const int reference=luaL_ref(L,LUA_REGISTRYINDEX);native_api_prefix_full(L,job);
    if(reference==LUA_NOREF||((reference==LUA_REFNIL)!=(actual_type==LUA_TNIL)))
        return (void)luaL_error(L,"ArtInfo actual copied value retention disagrees with original type");
    f->art_info_type=actual_type;f->art_info_ref=reference;++f->assigned_count;
    native_api_prefix_full(L,job);native_api_prefix_physical(L,*f);
    job.context_api_art_info_reference=reference;job.context_api_art_info_type=actual_type;lua_pop(L,2);
    // Only original620DF0 returned. Genuine5C7880 DB/GameInfo/GameDefines
    // and every following API registrar are still mandatory before Controls.
}
