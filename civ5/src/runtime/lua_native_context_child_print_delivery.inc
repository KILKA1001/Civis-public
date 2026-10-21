// Actual selected retained child delivery across legacy event factory setup.
// This keeper check is DATA/function identity, never a full61/setup return.
static void native_context_print_delivery_full(lua_State* L,std::size_t slot){
    native_print_source_full(L);auto& p=owner(L);
    if(!p.worldview_native_host||p.worldview_native_host->child!=slot||slot>=p.child_contexts.size())
        return (void)luaL_error(L,"Selected print actual lexical child role changed");
}
static void native_context_print_delivery_check(lua_State* L,std::size_t slot,int environment){
    auto& p=owner(L);native_context_print_delivery_full(L,slot);
    if(!p.worldview_native_host||p.worldview_native_host->child!=slot||
       slot>=p.child_contexts.size()||!p.child_contexts[slot].state_prepared||
       !p.child_contexts[slot].profile_libraries_copied||p.child_contexts[slot].retired||
       p.profile_print_function_ref==LUA_NOREF||p.profile_print_function_ref==LUA_REFNIL)
        return (void)luaL_error(L,"Selected print actual prepared profile/child keeper absent");
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[slot].environment);native_context_print_delivery_full(L,slot);
    if(!lua_istable(L,environment)||!lua_rawequal(L,environment,-1))
        return (void)luaL_error(L,"Selected print factory actual child environment changed");
    lua_pop(L,1);
    if(lua_getmetatable(L,environment))
        return (void)luaL_error(L,"Selected restricted environment acquired a master/metatable fallback");
    native_context_print_delivery_full(L,slot);
    lua_pushliteral(L,"print");native_context_print_delivery_full(L,slot);
    lua_rawget(L,environment);native_context_print_delivery_full(L,slot);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.profile_print_function_ref);native_context_print_delivery_full(L,slot);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_master_print||
       lua_topointer(L,-1)!=p.profile_print_function||!lua_rawequal(L,-1,-2))
        return (void)luaL_error(L,"Selected print actual canonical function delivery changed");
    lua_pop(L,2);native_context_print_delivery_full(L,slot);
    // Final physical structural POST after the last Source observer/callback.
    // lua_getmetatable is a nonallocating table lookup; existing query stack
    // capacity was already used above. No observer runs after this check.
    if(lua_getmetatable(L,environment))
        return (void)luaL_error(L,"Selected restricted environment late metatable fallback");
}
static void push_native_context_print_delivery(lua_State* L,std::size_t slot,int environment){
    native_context_print_delivery_check(L,slot,environment);auto& p=owner(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.profile_print_function_ref);native_context_print_delivery_full(L,slot);
    native_context_print_delivery_check(L,slot,environment);
}
