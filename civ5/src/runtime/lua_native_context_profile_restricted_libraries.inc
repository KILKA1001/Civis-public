// Genuine profile4D94C0 copier +49B190 readonly projection, inside Host::Impl.
// All owning state is in Host/Job; protected Lua callback locals are POD.
static int native_profile_readonly_newindex(lua_State* L){
    return luaL_error(L,"Attempt to modify read-only table");
}
static void native_profile_copy_names(lua_State* L,Job& job,int source,int target,const char*const* names){
    for(std::size_t i=0;names[i];++i){
        lua_getfield(L,source,names[i]);native_debug_config_full(L,job);
        // Real actual sourcevalue including genuine NIL; no guessed default/type.
        lua_setfield(L,target,names[i]);native_debug_config_full(L,job);
    }
}
static void native_profile_make_readonly(lua_State* L,Job& job){
    // [source] -> [source,proxy,meta]; actual source retained by meta.__index.
    lua_createtable(L,0,0);native_debug_config_full(L,job);
    lua_createtable(L,0,0);native_debug_config_full(L,job);
    lua_pushvalue(L,-3);native_debug_config_full(L,job);
    lua_setfield(L,-2,"__index");native_debug_config_full(L,job);
    lua_pushboolean(L,0);native_debug_config_full(L,job);
    lua_setfield(L,-2,"__metatable");native_debug_config_full(L,job);
    lua_pushcfunction(L,native_profile_readonly_newindex);native_debug_config_full(L,job);
    lua_setfield(L,-2,"__newindex");native_debug_config_full(L,job);
    lua_setmetatable(L,-2);native_debug_config_full(L,job);
    lua_remove(L,-2);native_debug_config_full(L,job);
}
static void perform_native_context_profile_restricted_libraries(lua_State* L,Job& job){
    auto& p=owner(L);native_debug_config_full(L,job);
    if(!p.profile_gc_stopped||p.child_contexts[job.worldview_child].profile_libraries_attempted)
        return (void)luaL_error(L,"Profile actual library-copy partial state/replay");
    // Source12 depends on true Source10 master print publisher, not stock baseprint.
    lua_getglobal(L,"print");native_debug_config_full(L,job);
    if(!p.profile_print_sink_alive||!lua_iscfunction(L,-1)||
       lua_tocfunction(L,-1)!=native_master_print||lua_topointer(L,-1)!=p.profile_print_function)
        return (void)luaL_error(L,"Profile genuine master print producer absent/changed");
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.profile_print_function_ref);native_debug_config_full(L,job);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Profile actual print canonical keeper changed");
    lua_pop(L,2);
    perform_native_context_state_query(L,job);native_debug_config_full(L,job);
    const int environment=lua_gettop(L);
    if(!lua_istable(L,environment)||lua_getmetatable(L,environment))
        return (void)luaL_error(L,"Profile cold actual environment metatable unsupported");
    p.child_contexts[job.worldview_child].profile_libraries_attempted=true;
    static const char*const globals[]={"assert","collectgarbage","error","getmetatable","ipairs","loadstring","next","pairs","pcall","print","select","setmetatable","tonumber","tostring","type","unpack","_VERSION","where","xpcall",nullptr};
    native_profile_copy_names(L,job,LUA_GLOBALSINDEX,environment,globals);
    if(job.context_profile_debug==1){
        lua_getfield(L,LUA_GLOBALSINDEX,"debug");native_debug_config_full(L,job);
        lua_setfield(L,environment,"debug");native_debug_config_full(L,job);
    }else if(job.context_profile_debug!=0)
        return (void)luaL_error(L,"Profile required actual0/1 debug operand changed");
    // Native order coroutine -> string -> table -> math -> os. No wholesale envcopy.
    static const char*const entries_coroutine[]={"create","resume","running","status","wrap","yield",nullptr};
    static const char*const entries_string[]={"byte","char","find","format","gmatch","gsub","len","lower","match","rep","reverse","sub","upper",nullptr};
    static const char*const entries_table[]={"concat","count","fill","insert","maxn","remove","sort",nullptr};
    static const char*const entries_math[]={"abs","acos","asin","atan","atan2","ceil","cos","cosh","clamp","deg","exp","floor","fmod","frexp","huge","ldexp","log","log10","max","min","modf","pi","pow","rad","random","randomseed","sin","sinh","sqrt","tan","tanh",nullptr};
    static const char*const entries_os[]={"clock","date","difftime","time","tmpname",nullptr};
    static const char*const library_names[]={"coroutine","string","table","math","os"};
    static const char*const*const library_entries[]={entries_coroutine,entries_string,entries_table,entries_math,entries_os};
    for(std::size_t slot=0;slot<5;++slot){
        lua_getfield(L,LUA_GLOBALSINDEX,library_names[slot]);native_debug_config_full(L,job);
        if(!lua_istable(L,-1))return (void)luaL_error(L,"Profile actual standard master namespace unavailable");
        const int source=lua_gettop(L);
        lua_createtable(L,0,0);native_debug_config_full(L,job);
        const int destination=lua_gettop(L);
        native_profile_copy_names(L,job,source,destination,library_entries[slot]);
        lua_remove(L,source);native_debug_config_full(L,job);
        native_profile_make_readonly(L,job);
        lua_pushvalue(L,-1);native_debug_config_full(L,job);
        p.child_contexts[job.worldview_child].profile_library_ref[slot]=luaL_ref(L,LUA_REGISTRYINDEX);
        native_debug_config_full(L,job);
        lua_setfield(L,environment,library_names[slot]);native_debug_config_full(L,job);
    }
    // Independent actual physical publication query, no bool-only full readiness.
    for(std::size_t slot=0;slot<5;++slot){
        lua_getfield(L,environment,library_names[slot]);native_debug_config_full(L,job);
        lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[job.worldview_child].profile_library_ref[slot]);
        native_debug_config_full(L,job);
        if(!lua_istable(L,-1)||!lua_rawequal(L,-1,-2))
            return (void)luaL_error(L,"Profile actual readonly library publication changed");
        lua_pop(L,2);
    }
    native_debug_config_full(L,job);
    p.child_contexts[job.worldview_child].profile_libraries_copied=true;
    // 519700 include +6538A0 traceback +4EC639 GC restart/RET still UNISSUED.
}
static void query_native_profile_libraries(lua_State* L,Job& job){
    auto& p=owner(L);native_debug_config_full(L,job);
    if(!p.child_contexts[job.worldview_child].profile_libraries_copied)
        return (void)luaL_error(L,"Profile actual library-copy phase absent");
    perform_native_context_state_query(L,job);native_debug_config_full(L,job);
    const int environment=lua_gettop(L);
    static const char*const names[]={"coroutine","string","table","math","os"};
    for(std::size_t slot=0;slot<5;++slot){
        const int reference=p.child_contexts[job.worldview_child].profile_library_ref[slot];
        if(reference==LUA_NOREF||reference==LUA_REFNIL)
            return (void)luaL_error(L,"Profile actual library keeper missing");
        lua_getfield(L,environment,names[slot]);native_debug_config_full(L,job);
        lua_rawgeti(L,LUA_REGISTRYINDEX,reference);native_debug_config_full(L,job);
        if(!lua_istable(L,-1)||!lua_rawequal(L,-1,-2)||!lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Profile actual readonly publication changed");
        lua_getfield(L,-1,"__index");native_debug_config_full(L,job);
        if(!lua_istable(L,-1))return (void)luaL_error(L,"Profile readonly source missing");
        lua_pop(L,1);lua_getfield(L,-1,"__metatable");native_debug_config_full(L,job);
        if(lua_type(L,-1)!=LUA_TBOOLEAN||lua_toboolean(L,-1))
            return (void)luaL_error(L,"Profile readonly metatable lock changed");
        lua_pop(L,1);lua_getfield(L,-1,"__newindex");native_debug_config_full(L,job);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_profile_readonly_newindex)
            return (void)luaL_error(L,"Profile readonly write callback changed");
        lua_pop(L,4);
    }
    lua_getfield(L,environment,"print");native_debug_config_full(L,job);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.profile_print_function_ref);native_debug_config_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_master_print||!lua_rawequal(L,-1,-2))
        return (void)luaL_error(L,"Profile selected child genuine print delivery changed");
    lua_pop(L,2);native_debug_config_full(L,job);
}
