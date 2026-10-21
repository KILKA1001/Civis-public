// Actual5CE8E7..5CE9A9 master-startup suffix, after native master extensions.
// These are the real57A350/563830 closures and their actual Lua upvalues.
// This owner does not issue4D9FC0 scoped-thread destruction or FLua completion.
class NativeMasterThreadBootstrapOwner final {
    friend struct Impl;
    Impl* const host;lua_State* const vm;
    const std::thread::id creator;
    const std::uint64_t epoch,writes;
    Job* initializer;
    bool retired=false;
    int globals_ref=LUA_NOREF,keys_ref=LUA_NOREF,threads_ref=LUA_NOREF,weak_ref=LUA_NOREF,weak_meta_ref=LUA_NOREF;
    int create_ref=LUA_NOREF,destroy_ref=LUA_NOREF;
    const void* globals=nullptr;const void* keys=nullptr;const void* threads=nullptr;const void* weak=nullptr;const void* weak_meta=nullptr;
    const void* create=nullptr;const void* destroy=nullptr;
    NativeMasterThreadBootstrapOwner(Impl& p,lua_State* L,Job& job,std::uint64_t fe,std::uint64_t fw)
        :host(&p),vm(L),creator(std::this_thread::get_id()),epoch(fe),writes(fw),initializer(&job){}
    NativeMasterThreadBootstrapOwner(const NativeMasterThreadBootstrapOwner&)=delete;
    NativeMasterThreadBootstrapOwner& operator=(const NativeMasterThreadBootstrapOwner&)=delete;
public:~NativeMasterThreadBootstrapOwner()=default;
};
static constexpr const char*const native_master_thread_keys[]={"Threads","WeakThreads","__mode","k","_CreateThread","_DestroyThread"};
static int native_master_thread_refuse(lua_State* L,const char* error){
    owner(L).record_failure();return luaL_error(L,"%s",error);
}
static void native_master_thread_key(lua_State* L,const NativeMasterThreadBootstrapOwner& actual,int ordinal){
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual.keys_ref);lua_rawgeti(L,-1,ordinal);lua_remove(L,-2);
}
static void native_master_thread_initial_current(lua_State* L,Job& job){
    auto& p=owner(L);const auto*const actual=p.native_master_thread_bootstrap.get();
    if(!actual||actual->host!=&p||actual->vm!=L||p.state!=L||actual->initializer!=&job||
       job.owner!=&p||job.operation!=Operation::initialize||!p.calling||actual->retired||
       actual->creator!=std::this_thread::get_id()||actual->creator!=p.ui_thread||
       actual->epoch!=p.failure_epoch||actual->writes!=p.failed.writes_||p.failed.exhausted_)
        native_master_thread_refuse(L,"Native master thread bootstrap actual initialize/VM/failure custody changed");
}
static void native_master_thread_physical(lua_State* L,const NativeMasterThreadBootstrapOwner* actual){
    auto& p=owner(L);const int top=lua_gettop(L);
    if(!actual||p.native_master_thread_bootstrap.get()!=actual||actual->host!=&p||actual->vm!=p.state||
       actual->retired||actual->creator!=std::this_thread::get_id()||actual->creator!=p.ui_thread||
       actual->epoch!=p.failure_epoch||actual->writes!=p.failed.writes_||p.failed.exhausted_||
       actual->globals_ref<0||actual->keys_ref<0||actual->threads_ref<0||actual->weak_ref<0||actual->weak_meta_ref<0||
       actual->create_ref<0||actual->destroy_ref<0)
        native_master_thread_refuse(L,"Native master thread bootstrap has no current genuine closed owner");
    //57A350 itself operates on captured upvalues, even if invoked from a
    // coroutine with a different fenv. Its registration custody is the actual
    // retained master globals, not the caller's LUA_GLOBALSINDEX surrogate.
    if(!ttistable(gt(actual->vm))||hvalue(gt(actual->vm))!=actual->globals)
        native_master_thread_refuse(L,"Native master thread original master globals replaced");
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->globals_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=actual->globals)
        native_master_thread_refuse(L,"Native master thread actual master globals retention changed");
    const int globals=lua_gettop(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->keys_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=actual->keys||lua_objlen(L,-1)!=6||lua_getmetatable(L,-1))
        native_master_thread_refuse(L,"Native master thread retained key table changed");
    for(int i=0;i<6;++i){
        lua_rawgeti(L,-1,i+1);std::size_t length=0;
        const char*const value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&length):nullptr;
        if(!value||length!=std::strlen(native_master_thread_keys[i])||std::memcmp(value,native_master_thread_keys[i],length))
            native_master_thread_refuse(L,"Native master thread actual key TString changed");
        lua_pop(L,1);
    }lua_pop(L,1);
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->threads_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=actual->threads||lua_getmetatable(L,-1))
        native_master_thread_refuse(L,"Native Threads actual strong plain table changed");
    native_master_thread_key(L,*actual,1);lua_rawget(L,globals);
    if(!lua_rawequal(L,-1,-2))native_master_thread_refuse(L,"Native Threads original global publication changed");lua_pop(L,2);
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->weak_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=actual->weak||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=actual->weak_meta)
        native_master_thread_refuse(L,"Native WeakThreads actual weak table/metatable changed");
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->weak_meta_ref);
    if(!lua_rawequal(L,-1,-2)||lua_getmetatable(L,-1))native_master_thread_refuse(L,"Native WeakThreads original meta custody changed");lua_pop(L,1);
    native_master_thread_key(L,*actual,3);lua_rawget(L,-2);native_master_thread_key(L,*actual,4);
    if(!lua_rawequal(L,-1,-2))native_master_thread_refuse(L,"Native WeakThreads original weak-key mode changed");lua_pop(L,2);
    // The original meta contains only __mode. Extra __newindex/__index/GC
    // behavior would change57A350/563830 settable semantics and is refused.
    lua_pushnil(L);int fields=0;
    while(lua_next(L,-2)){
        native_master_thread_key(L,*actual,3);
        if(!lua_rawequal(L,-1,-3))native_master_thread_refuse(L,"Native WeakThreads original meta acquired a foreign field");
        lua_pop(L,2);++fields;
    }
    if(fields!=1)native_master_thread_refuse(L,"Native WeakThreads original mode field absent");lua_pop(L,1);
    native_master_thread_key(L,*actual,2);lua_rawget(L,globals);
    if(!lua_rawequal(L,-1,-2))native_master_thread_refuse(L,"Native WeakThreads original global publication changed");lua_pop(L,2);
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->create_ref);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_master_create_thread_57a350||lua_topointer(L,-1)!=actual->create)
        native_master_thread_refuse(L,"Native _CreateThread real closure changed");
    const int create=lua_gettop(L);
    if(!lua_getupvalue(L,create,1)||lua_topointer(L,-1)!=actual->threads)native_master_thread_refuse(L,"Native _CreateThread first actual Threads upvalue changed");lua_pop(L,1);
    if(!lua_getupvalue(L,create,2)||lua_topointer(L,-1)!=actual->weak)native_master_thread_refuse(L,"Native _CreateThread second actual WeakThreads upvalue changed");lua_pop(L,1);
    if(lua_getupvalue(L,create,3))native_master_thread_refuse(L,"Native _CreateThread original two-upvalue arity changed");
    native_master_thread_key(L,*actual,5);lua_rawget(L,globals);
    if(!lua_rawequal(L,-1,-2))native_master_thread_refuse(L,"Native _CreateThread original global closure publication changed");lua_pop(L,2);
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->destroy_ref);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_master_destroy_thread_563830||lua_topointer(L,-1)!=actual->destroy)
        native_master_thread_refuse(L,"Native _DestroyThread real closure changed");
    const int destroy=lua_gettop(L);
    if(!lua_getupvalue(L,destroy,1)||lua_topointer(L,-1)!=actual->threads)native_master_thread_refuse(L,"Native _DestroyThread first actual Threads upvalue changed");lua_pop(L,1);
    if(lua_getupvalue(L,destroy,2))native_master_thread_refuse(L,"Native _DestroyThread original one-upvalue arity changed");
    native_master_thread_key(L,*actual,6);lua_rawget(L,globals);
    if(!lua_rawequal(L,-1,-2))native_master_thread_refuse(L,"Native _DestroyThread original global closure publication changed");
    lua_settop(L,top);
}
struct NativeMasterThreadCallbackSnapshot final {
    const NativeMasterThreadBootstrapOwner* actual;
    const WorldViewNativeHostScope* scope;
    const BnwWorldViewSourceOperationLoan* operation;
    std::uint64_t epoch,writes,uncertainty;
};
static NativeMasterThreadCallbackSnapshot native_master_thread_callback_first(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);
    const auto*const actual=p.native_master_thread_bootstrap.get();
    if(!actual||actual->initializer)native_master_thread_refuse(L,"Native master thread provider did not return from genuine initialization");
    native_master_thread_physical(L,actual);
    return {actual,p.worldview_native_host,p.worldview_operation,p.failure_epoch,p.failed.writes_,p.execution_uncertainty};
}
static void native_master_thread_callback_post(lua_State* L,const NativeMasterThreadCallbackSnapshot& saved){
    auto& p=owner(L);native_db_query_current(L);
    // Check the owning slot after Source callbacks before the old pointer is
    // dereferenced. No owning C++ local survives a Lua allocating/error API.
    if(p.native_master_thread_bootstrap.get()!=saved.actual||p.worldview_native_host!=saved.scope||
       p.worldview_operation!=saved.operation||p.failure_epoch!=saved.epoch||p.failed.writes_!=saved.writes||
       p.execution_uncertainty!=saved.uncertainty||p.failed.exhausted_)
        native_master_thread_refuse(L,"Native master thread callback/source/VM/failure changed");
    native_master_thread_physical(L,saved.actual);
}
static int native_master_create_thread_57a350(lua_State* L){
    const auto saved=native_master_thread_callback_first(L);
    if(!lua_checkstack(L,32))return native_master_thread_refuse(L,"Native _CreateThread real stack unavailable");
    native_master_thread_callback_post(L,saved);
    if(lua_topointer(L,lua_upvalueindex(1))!=saved.actual->threads||lua_topointer(L,lua_upvalueindex(2))!=saved.actual->weak)
        return native_master_thread_refuse(L,"Native _CreateThread invocation is not its genuine two-upvalue closure");
    // Actual57A35D weak table index first,57A370 strong table index second.
    lua_pushvalue(L,lua_upvalueindex(2));const int weak=lua_gettop(L);
    lua_pushvalue(L,lua_upvalueindex(1));const int threads=lua_gettop(L);
    lua_State*const thread=lua_newthread(L);native_master_thread_callback_post(L,saved);
    if(!thread||lua_tothread(L,-1)!=thread)return native_master_thread_refuse(L,"Native _CreateThread did not create its actual returned coroutine");
    lua_pushvalue(L,-1);lua_pushvalue(L,-1);
    lua_createtable(L,0,0);native_master_thread_callback_post(L,saved);
    lua_pushvalue(L,-1);
    if(!lua_setfenv(L,-3))return native_master_thread_refuse(L,"Native _CreateThread actual thread fenv assignment failed");
    native_master_thread_callback_post(L,saved);
    lua_settable(L,threads);native_master_thread_callback_post(L,saved);
    lua_pushboolean(L,1);lua_settable(L,weak);native_master_thread_callback_post(L,saved);
    if(lua_tothread(L,-1)!=thread)return native_master_thread_refuse(L,"Native _CreateThread actual final coroutine changed");
    return 1;
}
static int native_master_destroy_thread_563830(lua_State* L){
    const auto saved=native_master_thread_callback_first(L);
    if(!lua_checkstack(L,32))return native_master_thread_refuse(L,"Native _DestroyThread real stack unavailable");
    native_master_thread_callback_post(L,saved);
    if(lua_topointer(L,lua_upvalueindex(1))!=saved.actual->threads)
        return native_master_thread_refuse(L,"Native _DestroyThread invocation is not its genuine one-upvalue closure");
    // Original does not invent an arity/type filter: its genuine settable
    // consumes argument1 and reports nil/invalid table keys through Lua.
    lua_pushvalue(L,lua_upvalueindex(1));lua_pushvalue(L,1);lua_pushnil(L);lua_settable(L,-3);
    native_master_thread_callback_post(L,saved);return 0;
}
static void install_native_master_threads(lua_State* L,Job& job){
    auto& p=owner(L);
    if(job.owner!=&p||job.operation!=Operation::initialize||p.state!=L||!p.calling||p.native_master_thread_bootstrap)
        return (void)native_master_thread_refuse(L,"Native master thread bootstrap requires its actual first protected initialize");
    // The allocation is immediately in Host custody before any Lua call;
    // a Lua longjmp cannot strand an owning stack-local C++ object.
    p.native_master_thread_bootstrap.reset(new NativeMasterThreadBootstrapOwner(p,L,job,p.failure_epoch,p.failed.writes_));
    native_master_thread_initial_current(L,job);
    auto*const actual=p.native_master_thread_bootstrap.get();
    if(!lua_checkstack(L,32))return (void)native_master_thread_refuse(L,"Native master thread bootstrap actual stack unavailable");
    native_master_thread_initial_current(L,job);
    if(lua_getmetatable(L,LUA_GLOBALSINDEX))return (void)native_master_thread_refuse(L,"Native master thread original fresh Globals acquired a metatable");
    lua_pushvalue(L,LUA_GLOBALSINDEX);actual->globals=lua_topointer(L,-1);
    actual->globals_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_master_thread_initial_current(L,job);
    lua_createtable(L,6,0);native_master_thread_initial_current(L,job);
    actual->keys=lua_topointer(L,-1);const int keys=lua_gettop(L);
    for(int i=0;i<6;++i){lua_pushstring(L,native_master_thread_keys[i]);lua_rawseti(L,keys,i+1);native_master_thread_initial_current(L,job);}
    actual->keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_master_thread_initial_current(L,job);
    for(int i:{1,2,5,6}){native_master_thread_key(L,*actual,i);lua_rawget(L,LUA_GLOBALSINDEX);
        if(!lua_isnil(L,-1))return (void)native_master_thread_refuse(L,"Native master thread fresh bootstrap would replace a foreign publication");lua_pop(L,1);}
    // Original5CE8EF..5CE907 creates/publishes the actual strong map.
    lua_createtable(L,0,0);native_master_thread_initial_current(L,job);actual->threads=lua_topointer(L,-1);
    lua_pushvalue(L,-1);actual->threads_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_master_thread_initial_current(L,job);
    lua_pushvalue(L,-1);native_master_thread_key(L,*actual,1);lua_insert(L,-2);lua_rawset(L,LUA_GLOBALSINDEX);native_master_thread_initial_current(L,job);
    lua_pushvalue(L,-1);
    // Original5CE919..5CE960 keeps two references to the actual weak map,
    // sets its single __mode="k", then publishes precisely that table.
    lua_createtable(L,0,0);native_master_thread_initial_current(L,job);actual->weak=lua_topointer(L,-1);
    lua_pushvalue(L,-1);actual->weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_master_thread_initial_current(L,job);
    lua_pushvalue(L,-1);lua_createtable(L,0,0);native_master_thread_initial_current(L,job);actual->weak_meta=lua_topointer(L,-1);
    lua_pushvalue(L,-1);actual->weak_meta_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_master_thread_initial_current(L,job);
    native_master_thread_key(L,*actual,3);native_master_thread_key(L,*actual,4);lua_rawset(L,-3);native_master_thread_initial_current(L,job);
    lua_setmetatable(L,-2);native_master_thread_initial_current(L,job);
    native_master_thread_key(L,*actual,2);lua_insert(L,-2);lua_rawset(L,LUA_GLOBALSINDEX);native_master_thread_initial_current(L,job);
    // Actual closures capture the retained strong/weak maps, not a caller id,
    // empty surrogate thread, userdata protocol or inferred startup flag.
    lua_pushcclosure(L,native_master_create_thread_57a350,2);native_master_thread_initial_current(L,job);actual->create=lua_topointer(L,-1);
    lua_pushvalue(L,-1);actual->create_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_master_thread_initial_current(L,job);
    native_master_thread_key(L,*actual,5);lua_insert(L,-2);lua_rawset(L,LUA_GLOBALSINDEX);native_master_thread_initial_current(L,job);
    lua_pushcclosure(L,native_master_destroy_thread_563830,1);native_master_thread_initial_current(L,job);actual->destroy=lua_topointer(L,-1);
    lua_pushvalue(L,-1);actual->destroy_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_master_thread_initial_current(L,job);
    native_master_thread_key(L,*actual,6);lua_insert(L,-2);lua_rawset(L,LUA_GLOBALSINDEX);native_master_thread_initial_current(L,job);
    native_master_thread_physical(L,actual);native_master_thread_initial_current(L,job);
    actual->initializer=nullptr;
}
void native_master_threads_retire()noexcept{
    if(native_master_thread_bootstrap){native_master_thread_bootstrap->retired=true;native_master_thread_bootstrap->initializer=nullptr;}
}
