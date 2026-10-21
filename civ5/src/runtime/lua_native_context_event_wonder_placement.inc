// Inside Host::Impl. Actual75E540 twelfth generator7173A0. This is the retail
// WonderTogglePlacement constructor/list lifecycle, not capability UI Events.
// Private watchers are weak VALUES: an abandoned event/list/thread cycle is
// not rooted by the native subscriber channel. No game value is inferred.
enum NativeWonderPlacementKey : int {
    wonder_placement_callback_thread=1,wonder_placement_event_handle,wonder_placement_event,wonder_placement_index,wonder_placement_newindex,
    wonder_placement_gc,wonder_placement_call_meta,wonder_placement_metatable,wonder_placement_dispatch,wonder_placement_add,wonder_placement_call,
    wonder_placement_call_immediate,wonder_placement_count,wonder_placement_remove,wonder_placement_remove_all,wonder_placement_mode,wonder_placement_weak,
    wonder_placement_constructor_name
};
static constexpr const char*const native_wonder_placement_keys[]={
    "callback_thread","event_handle","_Event","__index","__newindex",
    "__gc","__call","__metatable","Dispatch","Add","Call",
    "CallImmediate","Count","Remove","RemoveAll","__mode","v",
    "WonderTogglePlacement"
};
struct NativeWonderPlacementHandle final {
    Impl* host;lua_State* thread;
    const void* list;const void* event;const void* environment;const void* gc_meta;
    // These are port ownership facts, never original pending-message bytes.
    bool constructed,registered;
};
struct NativeWonderPlacementQueuedSignal final {
    // An actual successful Lua Call appends one zero-argument WonderTogglePlacement message.
    // This occurrence only orders private queue entries; it is not game data
    // and cannot grant startup, Init, a frame or a completed setup receipt.
    std::uint64_t occurrence;
};
int native_wonder_placement_keys_ref=LUA_NOREF,native_wonder_placement_weak_ref=LUA_NOREF;
int native_wonder_placement_generator_ref=LUA_NOREF;
const void*native_wonder_placement_keys_table=nullptr;
const void*native_wonder_placement_weak_table=nullptr,*native_wonder_placement_weak_meta=nullptr;
const void*native_wonder_placement_generator_table=nullptr,*native_wonder_placement_constructor_closure=nullptr;
std::uint64_t native_wonder_placement_epoch=0,native_wonder_placement_writes=0,native_wonder_placement_uncertainty=0;
std::uint64_t native_wonder_placement_queue_occurrence=0;
bool native_wonder_placement_retired=false;
std::vector<NativeWonderPlacementHandle*> native_wonder_placement_subscribers;
std::deque<NativeWonderPlacementQueuedSignal> native_wonder_placement_queue;

static void native_wonder_placement_key(lua_State* L,NativeWonderPlacementKey key){
    auto& p=owner(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_wonder_placement_keys_ref);
    lua_rawgeti(L,-1,static_cast<int>(key));lua_remove(L,-2);
}
static void native_wonder_placement_channel_physical(lua_State* L){
    auto& p=owner(L);const int top=lua_gettop(L);
    if(std::this_thread::get_id()!=p.ui_thread||!p.state||!p.calling||p.native_wonder_placement_retired||
       p.native_wonder_placement_keys_ref==LUA_NOREF||p.native_wonder_placement_keys_ref==LUA_REFNIL||
       p.native_wonder_placement_weak_ref==LUA_NOREF||p.native_wonder_placement_weak_ref==LUA_REFNIL||
       !p.native_wonder_placement_keys_table||!p.native_wonder_placement_weak_table||!p.native_wonder_placement_weak_meta)
        return (void)luaL_error(L,"WonderTogglePlacement actual channel/VM owner is absent or retired");
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_wonder_placement_keys_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.native_wonder_placement_keys_table||
       lua_objlen(L,-1)!=18||lua_getmetatable(L,-1))
        return (void)luaL_error(L,"WonderTogglePlacement actual retained key table changed");
    const int keys=lua_gettop(L);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_wonder_placement_keys[i])||
           std::memcmp(value,native_wonder_placement_keys[i],n))
            return (void)luaL_error(L,"WonderTogglePlacement retained TValue key changed");
        lua_pop(L,1);
    }
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_wonder_placement_weak_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.native_wonder_placement_weak_table||
       !lua_getmetatable(L,-1)||lua_topointer(L,-1)!=p.native_wonder_placement_weak_meta)
        return (void)luaL_error(L,"WonderTogglePlacement actual weak watcher changed");
    native_wonder_placement_key(L,wonder_placement_mode);lua_rawget(L,-2);
    native_wonder_placement_key(L,wonder_placement_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"WonderTogglePlacement watcher lost weak values");
    lua_settop(L,top);
}
static void native_wonder_placement_current(lua_State* L){
    auto& p=owner(L);native_wonder_placement_channel_physical(L);
    if(p.native_wonder_placement_generator_ref==LUA_NOREF||p.native_wonder_placement_generator_ref==LUA_REFNIL||
       !p.native_wonder_placement_generator_table||!p.native_wonder_placement_constructor_closure||
       p.failure_epoch!=p.native_wonder_placement_epoch||p.failed.writes_!=p.native_wonder_placement_writes||
       p.execution_uncertainty!=p.native_wonder_placement_uncertainty||p.failed.exhausted_)
        return (void)luaL_error(L,"WonderTogglePlacement genuine constructor publication/failure owner changed");
    p.source_exception={};p.source_error=nullptr;
    try{
        if(p.worldview_native_host)p.worldview_require_callback_origin();
        else if(p.failed||!p.ready)throw ScriptError("WonderTogglePlacement has no healthy active Host operation");
    }catch(...){p.source_exception=std::current_exception();p.source_error="WonderTogglePlacement active operation refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    const int top=lua_gettop(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_wonder_placement_generator_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.native_wonder_placement_generator_table||lua_getmetatable(L,-1))
        return (void)luaL_error(L,"WonderTogglePlacement actual generator table changed");
    native_wonder_placement_key(L,wonder_placement_constructor_name);lua_rawget(L,-2);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=p.native_wonder_placement_constructor_closure||
       lua_tocfunction(L,-1)!=native_wonder_placement_constructor)
        return (void)luaL_error(L,"WonderTogglePlacement actual7173A0 closure changed");
    lua_settop(L,top);native_wonder_placement_channel_physical(L);
}
void native_wonder_placement_unsubscribe(NativeWonderPlacementHandle* h)noexcept{
    const auto i=std::find(native_wonder_placement_subscribers.begin(),native_wonder_placement_subscribers.end(),h);
    if(i!=native_wonder_placement_subscribers.end())native_wonder_placement_subscribers.erase(i);
    h->registered=false;
}
void native_wonder_placement_retire()noexcept{
    native_wonder_placement_retired=true;
    // Retire before lua_close. Clearing raw registrations never dereferences
    // a userdata pointer (even if a script replaced an original GC callback).
    // A genuine finalizer still sees the live Host and clears its own flag.
    native_wonder_placement_subscribers.clear();native_wonder_placement_queue.clear();
}
static int native_wonder_placement_gc(lua_State* L){
    auto& p=owner(L);
    if(lua_type(L,1)!=LUA_TUSERDATA||lua_objlen(L,1)!=sizeof(NativeWonderPlacementHandle))
        return luaL_error(L,"WonderTogglePlacement __gc requires its actual wide handle");
    auto*const h=static_cast<NativeWonderPlacementHandle*>(lua_touserdata(L,1));
    if(!h||h->host!=&p)return 0; // actual already retired/finalized handle
    if(h->registered)p.native_wonder_placement_unsubscribe(h);
    // Literal45F170 only clears native registration. A manual invocation of
    // the genuine finalizer does not invalidate a still reachable Lua event.
    return 0;
}
static NativeWonderPlacementHandle* native_wonder_placement_list_handle(lua_State* L){
    native_wonder_placement_current(L);const int top=lua_gettop(L);auto& p=owner(L);
    if(!lua_istable(L,lua_upvalueindex(1))||lua_getmetatable(L,lua_upvalueindex(1)))
        return (luaL_error(L,"WonderTogglePlacement actual list upvalue changed"),nullptr);
    const void*const list=lua_topointer(L,lua_upvalueindex(1));
    native_wonder_placement_key(L,wonder_placement_event_handle);lua_rawget(L,lua_upvalueindex(1));
    if(lua_type(L,-1)!=LUA_TUSERDATA||lua_objlen(L,-1)!=sizeof(NativeWonderPlacementHandle))
        return (luaL_error(L,"WonderTogglePlacement actual list handle changed"),nullptr);
    auto*const h=static_cast<NativeWonderPlacementHandle*>(lua_touserdata(L,-1));
    if(!h||h->host!=&p||!h->constructed||h->list!=list||!h->thread||!h->environment||!h->event||
       !lua_getmetatable(L,-1)||lua_topointer(L,-1)!=h->gc_meta)
        return (luaL_error(L,"WonderTogglePlacement list/thread/GC handle owner changed"),nullptr);
    native_wonder_placement_key(L,wonder_placement_gc);lua_rawget(L,-2);
    if(lua_tocfunction(L,-1)!=native_wonder_placement_gc)
        return (luaL_error(L,"WonderTogglePlacement genuine45F170 finalizer changed"),nullptr);
    lua_settop(L,top);
    native_wonder_placement_key(L,wonder_placement_callback_thread);lua_rawget(L,lua_upvalueindex(1));
    if(lua_tothread(L,-1)!=h->thread||&owner(h->thread)!=&p)
        return (luaL_error(L,"WonderTogglePlacement genuine callback thread changed"),nullptr);
    lua_getfenv(L,-1);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=h->environment)
        return (luaL_error(L,"WonderTogglePlacement genuine callback environment changed"),nullptr);
    native_wonder_placement_key(L,wonder_placement_event);lua_rawget(L,-2);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=h->event)
        return (luaL_error(L,"WonderTogglePlacement callback _Event changed"),nullptr);
    lua_settop(L,top);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_wonder_placement_weak_ref);
    lua_pushlightuserdata(L,h);lua_rawget(L,-2);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=list)
        return (luaL_error(L,"WonderTogglePlacement actual weak lifecycle entry changed"),nullptr);
    lua_settop(L,top);return h;
}
static int native_wonder_placement_add(lua_State* L){
    auto*const h=native_wonder_placement_list_handle(L);auto& p=owner(L);
    if(!h->registered){
        p.source_exception={};p.source_error=nullptr;
        try{p.native_wonder_placement_subscribers.push_back(h);}
        catch(...){p.source_exception=std::current_exception();p.source_error="WonderTogglePlacement native subscription allocation failed";}
        if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
        h->registered=true; // actual86D230 insertion succeeded
    }
    // Literal5DB670 stack behavior: last TValue, duplicate and nil allowed.
    // No function-only rule, colon-call rule or invented replacement value.
    lua_pushvalue(L,-1);const auto length=lua_objlen(L,lua_upvalueindex(1));
    if(length>=static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return luaL_error(L,"WonderTogglePlacement list exceeds original rawseti index width");
    lua_rawseti(L,lua_upvalueindex(1),static_cast<int>(length)+1);return 1;
}
static int native_wonder_placement_count(lua_State* L){
    (void)native_wonder_placement_list_handle(L);
    lua_pushinteger(L,static_cast<lua_Integer>(lua_objlen(L,lua_upvalueindex(1))));return 1;
}
static int native_wonder_placement_remove(lua_State* L){
    auto*const h=native_wonder_placement_list_handle(L);auto& p=owner(L);
    const auto length=lua_objlen(L,lua_upvalueindex(1));
    if(length>=static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return luaL_error(L,"WonderTogglePlacement list exceeds original rawseti index width");
    int i=0;lua_pushnil(L);
    do{
        if(lua_rawequal(L,-2,-1)){
            for(int j=i;j<=static_cast<int>(length);++j){
                lua_rawgeti(L,lua_upvalueindex(1),j+1);lua_rawseti(L,lua_upvalueindex(1),j);
            }
        }
        ++i;lua_pop(L,1);lua_rawgeti(L,lua_upvalueindex(1),i);
    }while(!lua_isnil(L,-1));
    (void)lua_objlen(L,lua_upvalueindex(1));
    // Actual50683C tests EBX=INITIAL length, not EAX=fresh objlen. Preserve
    // its adjacent-duplicate behavior and unregister only that native branch.
    if(length==0&&h->registered)p.native_wonder_placement_unsubscribe(h);
    return 0;
}
static int native_wonder_placement_remove_all(lua_State* L){
    (void)native_wonder_placement_list_handle(L);const auto length=lua_objlen(L,lua_upvalueindex(1));
    if(length>=static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return luaL_error(L,"WonderTogglePlacement list exceeds original rawseti index width");
    for(int i=1;i<=static_cast<int>(length);++i){lua_pushnil(L);lua_rawseti(L,lua_upvalueindex(1),i);}
    // Literal761770 deliberately leaves native event_handle registered.
    return 0;
}
static int native_wonder_placement_dispatch_body(lua_State* L){
    auto*const dispatched_handle=native_wonder_placement_list_handle(L);const int args=lua_gettop(L);
    const auto length=lua_objlen(L,lua_upvalueindex(1));
    if(length>=static_cast<std::size_t>(std::numeric_limits<int>::max())||args==std::numeric_limits<int>::max()||!lua_checkstack(L,args+1))
        return luaL_error(L,"WonderTogglePlacement original Dispatch stack/index width unavailable");
    auto& p=owner(L);const auto previous=p.graph_execution_scope;
    for(int i=1;i<=static_cast<int>(length);++i){
        lua_rawgeti(L,lua_upvalueindex(1),i);
        const auto compiled=p.graph_compiled_owner(L,-1);
        for(int j=1;j<=args;++j)lua_pushvalue(L,j);
        // IAT8F984A is lua_call, not lua_pcall. A listener error aborts the
        // remaining list and is handled only by its real outer Lua boundary.
        // The single outer protected port boundary restores graph scope on
        // error. It rethrows that same error; it never continues the list.
        p.graph_execution_scope=compiled;lua_call(L,args,0);p.graph_execution_scope=previous;
        // A listener can catch a native-operation failure and return normally.
        // Restore graph scope first, then earn currentness/handle identity
        // again before the next original list index or successful return.
        if(native_wonder_placement_list_handle(L)!=dispatched_handle)
            return luaL_error(L,"actual event Dispatch handle changed after listener return");
    }
    return 0;
}
static int native_wonder_placement_dispatch(lua_State* L){
    (void)native_wonder_placement_list_handle(L);const int args=lua_gettop(L);auto& p=owner(L);
    if(args==std::numeric_limits<int>::max()||!lua_checkstack(L,2))
        return luaL_error(L,"WonderTogglePlacement protected Dispatch stack unavailable");
    const auto previous=p.graph_execution_scope;
    lua_pushvalue(L,lua_upvalueindex(1));lua_pushcclosure(L,native_wonder_placement_dispatch_body,1);lua_insert(L,1);
    const int status=lua_pcall(L,args,0,0);p.graph_execution_scope=previous;
    if(status)return lua_error(L);return 0;
}
// Actual702600/5958B0 ->532F60 is bare RET, with no Lua data.
// No native padding byte is invented as a signal value.
static int native_wonder_placement_call(lua_State* L){
    (void)native_wonder_placement_list_handle(L);auto& p=owner(L);
    if(p.native_wonder_placement_queue_occurrence==UINT64_MAX)
        return luaL_error(L,"WonderTogglePlacement actual queue occurrence exhausted");
    p.source_exception={};p.source_error=nullptr;
    try{p.native_wonder_placement_queue.push_back({p.native_wonder_placement_queue_occurrence+1});}
    catch(...){p.source_exception=std::current_exception();p.source_error="WonderTogglePlacement actual deferred message allocation failed";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    ++p.native_wonder_placement_queue_occurrence;return 0;
}
static int native_wonder_placement_call_immediate(lua_State* L){
    (void)native_wonder_placement_list_handle(L);
    // Actual5958B0 invokes the current native dispatcher. Its authenticated
    // client owner and subscriber-iteration producer are unissued. A direct
    // call of this event's Dispatch would incorrectly skip the other events'
    // registered callback threads, so no positive substitute is returned.
    return luaL_error(L,"WonderTogglePlacement CallImmediate requires its actual client dispatcher owner; emitter unissued");
}
static int native_wonder_placement_call_meta(lua_State* L){
    native_wonder_placement_current(L);const int count=lua_gettop(L);
    lua_pushvalue(L,lua_upvalueindex(1));lua_insert(L,2);lua_call(L,count-1,0);return 0;
}
static int native_wonder_placement_constructor(lua_State* L){
    native_wonder_placement_current(L);
    if(!lua_checkstack(L,24))return luaL_error(L,"WonderTogglePlacement real7173A0 constructor stack unavailable");
    lua_createtable(L,0,0);const int event=lua_gettop(L);const void*const event_pointer=lua_topointer(L,event);
    // Literal630EC0: list, a NEW callback thread, and5DB050 callback fenv.
    lua_createtable(L,0,1);const int list=lua_gettop(L);const void*const list_pointer=lua_topointer(L,list);
    native_wonder_placement_key(L,wonder_placement_callback_thread);lua_State*const thread=lua_newthread(L);
    lua_createtable(L,0,1);const int environment=lua_gettop(L);const void*const environment_pointer=lua_topointer(L,environment);
    native_wonder_placement_key(L,wonder_placement_event);lua_pushvalue(L,event);lua_rawset(L,environment);
    lua_createtable(L,0,2);const int environment_meta=lua_gettop(L);
    native_wonder_placement_key(L,wonder_placement_index);lua_pushvalue(L,LUA_GLOBALSINDEX);lua_rawset(L,environment_meta);
    native_wonder_placement_key(L,wonder_placement_newindex);lua_pushvalue(L,LUA_GLOBALSINDEX);lua_rawset(L,environment_meta);
    lua_setmetatable(L,environment);lua_setfenv(L,-2);lua_rawset(L,list);
    native_wonder_placement_key(L,wonder_placement_event_handle);
    auto*const h=static_cast<NativeWonderPlacementHandle*>(lua_newuserdata(L,sizeof(NativeWonderPlacementHandle)));
    *h={&owner(L),thread,list_pointer,event_pointer,environment_pointer,nullptr,false,false};
    // Each actual constructor has its own fresh GC metatable, as630EC0.
    lua_createtable(L,0,1);h->gc_meta=lua_topointer(L,-1);
    native_wonder_placement_key(L,wonder_placement_gc);lua_pushcclosure(L,native_wonder_placement_gc,0);lua_rawset(L,-3);
    lua_setmetatable(L,-2);lua_rawset(L,list);
    // Full462AC0 suffix (the old cached decompilation stopped after Add).
    static constexpr NativeWonderPlacementKey fields[]={wonder_placement_add,wonder_placement_call,wonder_placement_call_immediate,wonder_placement_count,wonder_placement_dispatch,wonder_placement_remove,wonder_placement_remove_all};
    static constexpr lua_CFunction methods[]={native_wonder_placement_add,native_wonder_placement_call,native_wonder_placement_call_immediate,
        native_wonder_placement_count,native_wonder_placement_dispatch,native_wonder_placement_remove,native_wonder_placement_remove_all};
    for(int i=0;i<7;++i){native_wonder_placement_key(L,fields[i]);lua_pushvalue(L,list);lua_pushcclosure(L,methods[i],1);lua_rawset(L,event);}
    // Literal621AA0: __newindex nil, fresh locked visible metatable value,
    // and718720 captures the CURRENT actual event.Call closure, not a name.
    lua_createtable(L,0,3);const int event_meta=lua_gettop(L);
    native_wonder_placement_key(L,wonder_placement_newindex);lua_pushnil(L);lua_rawset(L,event_meta);
    native_wonder_placement_key(L,wonder_placement_metatable);lua_createtable(L,0,0);lua_rawset(L,event_meta);
    native_wonder_placement_key(L,wonder_placement_call_meta);native_wonder_placement_key(L,wonder_placement_call);lua_rawget(L,event);
    lua_pushcclosure(L,native_wonder_placement_call_meta,1);lua_rawset(L,event_meta);lua_setmetatable(L,event);
    // Port-only weak watcher is published from the actual completed list.
    auto& p=owner(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_wonder_placement_weak_ref);
    lua_pushlightuserdata(L,h);lua_pushvalue(L,list);lua_rawset(L,-3);lua_pop(L,1);
    h->constructed=true;lua_settop(L,event);native_wonder_placement_current(L);return 1;
}
