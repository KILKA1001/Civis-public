// Inside Host::Impl. Original42AA50/4CAF40 dynamic LuaEvent, distinct from
// the named700540 LuaEvents factory. Every constructor creates a fresh list
// in the same weak VALUES group; Call synchronously visits that whole group.
enum NativeLuaEventKey:int {
    lua_event_name=1,lua_event_mode,lua_event_weak,lua_event_add,lua_event_call,
    lua_event_count,lua_event_remove,lua_event_remove_all,lua_event_newindex,
    lua_event_metatable,lua_event_call_meta
};
static constexpr const char*const native_lua_event_keys[]={
    "LuaEvent","__mode","v","Add","Call","Count","Remove","RemoveAll",
    "__newindex","__metatable","__call"
};
int native_lua_event_keys_ref=LUA_NOREF,native_lua_event_group_ref=LUA_NOREF,native_lua_event_lists_ref=LUA_NOREF;
const void* native_lua_event_keys_table=nullptr;const void* native_lua_event_group=nullptr;
const void* native_lua_event_group_meta=nullptr;const void* native_lua_event_lists=nullptr;
const void* native_lua_event_lists_meta=nullptr;const void* native_lua_event_constructor_closure=nullptr;
bool native_lua_event_retired=false;
static void native_lua_event_key(lua_State* L,NativeLuaEventKey key){
    auto& p=owner(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_lua_event_keys_ref);
    lua_rawgeti(L,-1,static_cast<int>(key));lua_remove(L,-2);
}
static void native_lua_event_physical(lua_State* L){
    auto& p=owner(L);const int top=lua_gettop(L);
    if(std::this_thread::get_id()!=p.ui_thread||!p.state||!p.calling||p.native_lua_event_retired||
       p.native_lua_event_keys_ref<0||p.native_lua_event_group_ref<0||p.native_lua_event_lists_ref<0||
       !p.native_lua_event_keys_table||!p.native_lua_event_group||!p.native_lua_event_group_meta||
       !p.native_lua_event_lists||!p.native_lua_event_lists_meta)
        return (void)luaL_error(L,"LuaEvent actual retail group/key owner is absent or retired");
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_lua_event_keys_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.native_lua_event_keys_table||lua_objlen(L,-1)!=11||lua_getmetatable(L,-1))
        return (void)luaL_error(L,"LuaEvent actual native key keeper changed");
    const int keys=lua_gettop(L);
    for(int i=0;i<11;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char*const value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_lua_event_keys[i])||std::memcmp(value,native_lua_event_keys[i],n))
            return (void)luaL_error(L,"LuaEvent retained literal changed");
        lua_pop(L,1);
    }
    const int refs[]={p.native_lua_event_group_ref,p.native_lua_event_lists_ref};
    const void*const values[]={p.native_lua_event_group,p.native_lua_event_lists};
    const void*const metas[]={p.native_lua_event_group_meta,p.native_lua_event_lists_meta};
    for(int i=0;i<2;++i){
        lua_rawgeti(L,LUA_REGISTRYINDEX,refs[i]);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=values[i]||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=metas[i])
            return (void)luaL_error(L,"LuaEvent authentic weak group/list watcher changed");
        native_lua_event_key(L,lua_event_mode);lua_rawget(L,-2);native_lua_event_key(L,lua_event_weak);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"LuaEvent original group lost weak values");
        lua_settop(L,keys);
    }lua_settop(L,top);
}
static void native_lua_event_current(lua_State* L){
    auto& p=owner(L);native_game_view_current(L);native_lua_event_physical(L);
    if(!p.native_lua_event_constructor_closure)
        return (void)luaL_error(L,"LuaEvent has no genuine42AA50 constructor assignment");
    const int top=lua_gettop(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_ordered_generator_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.native_ordered_generator_table||lua_getmetatable(L,-1))
        return (void)luaL_error(L,"LuaEvent actual shared native generator changed");
    native_lua_event_key(L,lua_event_name);lua_rawget(L,-2);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=p.native_lua_event_constructor_closure||
       lua_tocfunction(L,-1)!=native_lua_event_constructor||!lua_getupvalue(L,-1,1)||
       !lua_istable(L,-1)||lua_topointer(L,-1)!=p.native_lua_event_group)
        return (void)luaL_error(L,"LuaEvent real4CAF40 closure/group capture changed");
    lua_pop(L,1);if(lua_getupvalue(L,-1,2))return (void)luaL_error(L,"LuaEvent original one upvalue changed");
    lua_settop(L,top);native_lua_event_physical(L);
}
void native_lua_event_retire()noexcept{native_lua_event_retired=true;}
static const void* native_lua_event_list(lua_State* L){
    native_lua_event_current(L);auto& p=owner(L);const int top=lua_gettop(L);
    if(!lua_istable(L,lua_upvalueindex(1))||lua_getmetatable(L,lua_upvalueindex(1)))
        return (luaL_error(L,"LuaEvent actual list upvalue changed"),nullptr);
    const void*const list=lua_topointer(L,lua_upvalueindex(1));
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_lua_event_lists_ref);lua_pushlightuserdata(L,const_cast<void*>(list));lua_rawget(L,-2);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=list)
        return (luaL_error(L,"LuaEvent list has no genuine4CAF40 creation"),nullptr);
    lua_settop(L,top);return list;
}
static int native_lua_event_add(lua_State* L){
    (void)native_lua_event_list(L);lua_pushvalue(L,-1);const auto count=lua_objlen(L,lua_upvalueindex(1));
    if(count>=static_cast<std::size_t>(std::numeric_limits<int>::max()))return luaL_error(L,"LuaEvent list exceeds original index width");
    lua_rawseti(L,lua_upvalueindex(1),static_cast<int>(count)+1);return 1;
}
static int native_lua_event_count(lua_State* L){
    (void)native_lua_event_list(L);lua_pushinteger(L,static_cast<lua_Integer>(lua_objlen(L,lua_upvalueindex(1))));return 1;
}
static int native_lua_event_remove(lua_State* L){
    (void)native_lua_event_list(L);const auto count=lua_objlen(L,lua_upvalueindex(1));
    if(count>=static_cast<std::size_t>(std::numeric_limits<int>::max()))return luaL_error(L,"LuaEvent list exceeds original index width");
    int i=0;lua_pushnil(L);
    do{
        if(lua_rawequal(L,-2,-1))for(int j=i;j<=static_cast<int>(count);++j){lua_rawgeti(L,lua_upvalueindex(1),j+1);lua_rawseti(L,lua_upvalueindex(1),j);}
        ++i;lua_pop(L,1);lua_rawgeti(L,lua_upvalueindex(1),i);
    }while(!lua_isnil(L,-1));return 0;
}
static int native_lua_event_remove_all(lua_State* L){
    (void)native_lua_event_list(L);const auto count=lua_objlen(L,lua_upvalueindex(1));
    if(count>=static_cast<std::size_t>(std::numeric_limits<int>::max()))return luaL_error(L,"LuaEvent list exceeds original index width");
    for(int i=1;i<=static_cast<int>(count);++i){lua_pushnil(L);lua_rawseti(L,lua_upvalueindex(1),i);}return 0;
}
static void native_lua_event_call_group(lua_State* L){
    native_lua_event_current(L);
    if(!lua_istable(L,lua_upvalueindex(1))||lua_topointer(L,lua_upvalueindex(1))!=owner(L).native_lua_event_group)
        return (void)luaL_error(L,"LuaEvent actual synchronous Call group capture changed");
}
static int native_lua_event_call_body(lua_State* L){
    native_lua_event_call_group(L);const int args=lua_gettop(L);auto& p=owner(L);
    if(args==std::numeric_limits<int>::max()||!lua_checkstack(L,args+3))return luaL_error(L,"LuaEvent original Call stack unavailable");
    const auto previous=p.graph_execution_scope;lua_pushnil(L);
    while(lua_next(L,lua_upvalueindex(1))){
        if(lua_istable(L,-1)){
            const int list=lua_gettop(L);const auto count=lua_objlen(L,list);
            if(count>=static_cast<std::size_t>(std::numeric_limits<int>::max()))return luaL_error(L,"LuaEvent list exceeds original Call index width");
            for(int i=1;i<=static_cast<int>(count);++i){
                lua_rawgeti(L,list,i);const auto compiled=p.graph_compiled_owner(L,-1);
                for(int j=1;j<=args;++j)lua_pushvalue(L,j);
                p.graph_execution_scope=compiled;lua_call(L,args,0);p.graph_execution_scope=previous;
                // Native failure can be caught by the listener's Lua pcall.
                // Re-earn the genuine group/source before another listener.
                native_lua_event_call_group(L);
            }
        }lua_pop(L,1);
    }return 0;
}
static int native_lua_event_call(lua_State* L){
    native_lua_event_call_group(L);const int args=lua_gettop(L);auto& p=owner(L);
    if(args==std::numeric_limits<int>::max()||!lua_checkstack(L,2))return luaL_error(L,"LuaEvent protected Call stack unavailable");
    const auto previous=p.graph_execution_scope;
    lua_pushvalue(L,lua_upvalueindex(1));lua_pushcclosure(L,native_lua_event_call_body,1);lua_insert(L,1);
    const int status=lua_pcall(L,args,0,0);p.graph_execution_scope=previous;if(status)return lua_error(L);return 0;
}
static int native_lua_event_call_meta(lua_State* L){
    native_lua_event_current(L);const int count=lua_gettop(L);
    lua_pushvalue(L,lua_upvalueindex(1));lua_insert(L,2);lua_call(L,count-1,0);return 0;
}
static int native_lua_event_constructor(lua_State* L){
    native_lua_event_current(L);auto& p=owner(L);
    if(!lua_istable(L,lua_upvalueindex(1))||lua_topointer(L,lua_upvalueindex(1))!=p.native_lua_event_group||!lua_checkstack(L,16))
        return luaL_error(L,"LuaEvent actual4CAF40 weak group operand/stack unavailable");
    const auto old_count=lua_objlen(L,lua_upvalueindex(1));
    if(old_count>=static_cast<std::size_t>(std::numeric_limits<int>::max()))return luaL_error(L,"LuaEvent original group exceeds index width");
    lua_createtable(L,0,0);const int event=lua_gettop(L);
    lua_createtable(L,0,0);const int list=lua_gettop(L);const void*const list_pointer=lua_topointer(L,list);
    lua_pushvalue(L,list);lua_rawseti(L,lua_upvalueindex(1),static_cast<int>(old_count)+1);
    // Port-only weak evidence is keyed by the actual list pointer. It does
    // not change sparse weak-array # semantics or root a listener list.
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_lua_event_lists_ref);lua_pushlightuserdata(L,const_cast<void*>(list_pointer));lua_pushvalue(L,list);lua_rawset(L,-3);lua_pop(L,1);
    static constexpr NativeLuaEventKey fields[]={lua_event_add,lua_event_call,lua_event_count,lua_event_remove,lua_event_remove_all};
    static constexpr lua_CFunction methods[]={native_lua_event_add,native_lua_event_call,native_lua_event_count,native_lua_event_remove,native_lua_event_remove_all};
    for(int i=0;i<5;++i){native_lua_event_key(L,fields[i]);lua_pushvalue(L,i==1?lua_upvalueindex(1):list);lua_pushcclosure(L,methods[i],1);lua_rawset(L,event);}
    lua_pop(L,1);lua_createtable(L,0,3);const int meta=lua_gettop(L);
    native_lua_event_key(L,lua_event_newindex);lua_pushnil(L);lua_rawset(L,meta);
    native_lua_event_key(L,lua_event_metatable);lua_createtable(L,0,0);lua_rawset(L,meta);
    native_lua_event_key(L,lua_event_call_meta);native_lua_event_key(L,lua_event_call);lua_rawget(L,event);
    lua_pushcclosure(L,native_lua_event_call_meta,1);lua_rawset(L,meta);lua_setmetatable(L,event);
    native_lua_event_current(L);return 1;
}
