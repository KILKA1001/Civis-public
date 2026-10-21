// Inside the actual Host::Impl, before the protected job dispatcher.
// The canonical bootstrap produces the factory once. Its lexical groups VALUE
// is deliberately shared by every selected LuaEvents table.
static const UpVal* lua_events_groups_without_callbacks(lua_State* L,int index){
    if(!lua_isfunction(L,index)||lua_iscfunction(L,index))return nullptr;
    const auto* c=static_cast<const Closure*>(lua_topointer(L,index));
    if(!c||!c->l.p||c->l.nupvalues!=c->l.p->nups||
       c->l.p->sizeupvalues!=c->l.nupvalues||!c->l.p->upvalues)return nullptr;
    const UpVal* found=nullptr;
    for(unsigned i=0;i<c->l.nupvalues;++i){
        const auto* n=c->l.p->upvalues[i];
        if(n&&n->tsv.len==6&&!std::memcmp(getstr(n),"groups",6)){
            const auto* uv=c->l.upvals[i];
            if(found||!uv||uv->v!=&uv->u.value||!ttistable(uv->v))return nullptr;
            found=uv;
        }
    }
    return found;
}
static void retain_native_lua_events_factory(lua_State* L){
    auto& p=owner(L);
    if(p.lua_events_factory_alive||p.lua_events_factory_ref!=LUA_NOREF||
       !lua_isfunction(L,-1)||lua_iscfunction(L,-1))
        return (void)luaL_error(L,"Canonical LuaEvents bootstrap replay/closure refused");
    const int installer=lua_gettop(L);bool found=false;
    for(int i=1;;++i){
        const char* name=lua_getupvalue(L,installer,i);
        if(!name)break;
        if(!std::strcmp(name,"factory")){
            if(found||!lua_istable(L,-1))return (void)luaL_error(L,"Actual LuaEvents factory upvalue refused");
            found=true;const void* factory=lua_topointer(L,-1);
            lua_pushliteral(L,"__index");lua_rawget(L,-2);
            const auto* groups=lua_events_groups_without_callbacks(L,-1);
            if(!groups)return (void)luaL_error(L,"Actual LuaEvents groups upvalue refused");
            const auto* closure=static_cast<const Closure*>(lua_topointer(L,-1));
            p.lua_events_factory_index=closure;p.lua_events_factory_groups=groups;
            p.lua_events_factory_groups_pointer=hvalue(groups->v);
            lua_pop(L,1);lua_pushvalue(L,-1);
            p.lua_events_factory_ref=luaL_ref(L,LUA_REGISTRYINDEX);
            p.lua_events_factory_pointer=factory;
        }
        lua_pop(L,1);
    }
    if(!found||p.lua_events_factory_ref==LUA_NOREF||p.lua_events_factory_ref==LUA_REFNIL)
        return (void)luaL_error(L,"Canonical LuaEvents factory missing");
    // Actual canonical bootstrap allocation owns these interned keys.
    // Later LASTFULL physical checks read only registry-retained TStrings.
    lua_createtable(L,3,0);const int keys=lua_gettop(L);
    p.lua_events_keys_pointer=lua_topointer(L,keys);
    for(int i=0;i<3;++i){lua_pushstring(L,native_lua_events_keys[i]);lua_rawseti(L,keys,i+1);}
    p.lua_events_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    if(p.lua_events_keys_ref<0)return (void)luaL_error(L,"Actual LuaEvents retained key allocation failed");
    p.lua_events_factory_alive=true;
}
static constexpr const char*const native_lua_events_keys[]={"LuaEventFactory","__index","LuaEvents"};
static void native_lua_events_key(lua_State* L,int key){
    auto& p=owner(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.lua_events_keys_ref);lua_rawgeti(L,-1,key);lua_remove(L,-2);
}
static bool native_lua_events_keys_without_callbacks(lua_State* L){
    const auto& p=owner(L);const int top=lua_gettop(L);
    if(p.lua_events_keys_ref<0||!p.lua_events_keys_pointer)return false;
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.lua_events_keys_ref);
    bool valid=lua_istable(L,-1)&&lua_topointer(L,-1)==p.lua_events_keys_pointer;
    if(!valid||lua_getmetatable(L,-1)){lua_settop(L,top);return false;}
    if(lua_objlen(L,-1)!=3){lua_settop(L,top);return false;}
    for(int i=0;i<3;++i){
        lua_rawgeti(L,-1,i+1);std::size_t n=0;
        const char*const value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        valid=valid&&value&&n==std::strlen(native_lua_events_keys[i])&&!std::memcmp(value,native_lua_events_keys[i],n);
        lua_pop(L,1);
    }
    lua_settop(L,top);return valid;
}
static bool native_lua_events_factory_without_callbacks(lua_State* L){
    const auto& p=owner(L);
    if(!p.lua_events_factory_alive||p.lua_events_factory_ref<0||!p.lua_events_factory_pointer||
       !p.lua_events_factory_index||!p.lua_events_factory_groups||!p.lua_events_factory_groups_pointer||
       !native_lua_events_keys_without_callbacks(L))return false;
    const int top=lua_gettop(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.lua_events_factory_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.lua_events_factory_pointer||lua_getmetatable(L,-1)){
        lua_settop(L,top);return false;
    }
    native_lua_events_key(L,2);lua_rawget(L,-2);
    const auto*const groups=lua_events_groups_without_callbacks(L,-1);
    const bool valid=lua_topointer(L,-1)==p.lua_events_factory_index&&groups==p.lua_events_factory_groups&&
        groups&&static_cast<const void*>(hvalue(groups->v))==p.lua_events_factory_groups_pointer;
    lua_settop(L,top);return valid;
}
