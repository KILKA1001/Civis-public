// Original519700 HIT: preserve the real master list, skip refresh, create a
// NEW include closure for this actual scoped Threads environment. Inside Impl,
// after the existing master-list/include helpers and Logic's scoped guard.
// Logic owns thread/profile/GC/Job lifetime and the protected outer boundary.
static void native_gamecore_cached_include_current_owned(lua_State* L,int environment,
                                                         std::string_view associated_context){
    auto& p=owner(L);native_gamecore_scoped_profile_current_owned(L);
    p.source_exception={};p.source_error=nullptr;
    const NativeContextIncludeTableControl* observed_control=nullptr;
    const BnwContextLuaIncludeCollectionLoan* observed_collection=nullptr;
    try{
        const auto control=p.profile_include_table_control;
        const auto collection=p.profile_include_collection;
        if(L!=p.state||!p.native_gamecore_bootstrap||p.native_gamecore_bootstrap->host!=&p||
           p.native_gamecore_bootstrap->master!=L||!p.native_gamecore_bootstrap->actual_job||
           associated_context!=p.native_gamecore_bootstrap->actual_job->context||
           !p.native_gamecore_bootstrap->profile_gc_stopped||!p.worldview_native_host||
           !p.worldview_native_host->loan||!control||!collection||!p.profile_include_master_alive||
           p.profile_include_refreshing||!p.include_configuration_alive||p.include_configuration_ref<0||
           !p.include_configuration_pointer||!p.include_bootstrap_proto||p.sources.strict_map_includes||
           control->creator!=std::this_thread::get_id()||control->host.expired()||
           control->host.owner_before(p.weak_from_this())||p.weak_from_this().owner_before(control->host)||
           control->vm!=p.state||control->epoch!=p.failure_epoch||control->writes!=p.failed.writes_||
           control->uncertainty!=p.execution_uncertainty||control->collection.get()!=collection.get()||
           control->collection.owner_before(collection)||collection.owner_before(control->collection)||
           control->table_ref!=p.profile_include_table_ref||control->refresh_ref!=p.profile_include_refresh_ref||
           control->table!=p.profile_include_table_pointer||control->refresh!=p.profile_include_refresh_pointer||
           control->table_ref<0||control->refresh_ref<0||!control->table||!control->refresh||
           p.profile_include_names.size()>=static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw ScriptError("GameCore cached include lacks SAME actual master/list/factory/scoped owner");
        const auto*const op=p.worldview_native_host->loan;
        observed_control=control.get();observed_collection=collection.get();
        collection->require_current_without_callbacks(*op);
        collection->require_current_owned(*op);p.worldview_require_callback_origin();
        if(p.profile_include_table_control.get()!=control.get()||p.profile_include_collection.get()!=collection.get()||
           !p.worldview_native_host||p.worldview_native_host->loan!=op)
            throw ScriptError("GameCore cached include Source/control retired at POST");
        collection->require_current_without_callbacks(*op);
    }catch(...){p.source_exception=std::current_exception();p.source_error="GameCore cached include genuine Source/list current refused";}
    // All strong C++ borrowers ended before any allocating Lua API/longjmp.
    if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
    native_gamecore_scoped_profile_current_owned(L);
    if(p.profile_include_table_control.get()!=observed_control||p.profile_include_collection.get()!=observed_collection||
       observed_control->table_ref!=p.profile_include_table_ref||observed_control->refresh_ref!=p.profile_include_refresh_ref||
       observed_control->table!=p.profile_include_table_pointer||observed_control->refresh!=p.profile_include_refresh_pointer)
        return (void)luaL_error(L,"GameCore cached include control/keepers replaced after scoped Source POST");
    const int top=lua_gettop(L);
    if(environment<0)environment=top+environment+1;
    if(environment<=0||environment>top||!lua_istable(L,environment)||
       lua_topointer(L,environment)!=p.native_gamecore_bootstrap->environment)
        return (void)luaL_error(L,"GameCore cached include actual offered Threads environment changed");
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_gamecore_bootstrap->environment_ref);
    bool current=lua_istable(L,-1)&&lua_rawequal(L,-1,environment);lua_settop(L,top);
    if(!current)return (void)luaL_error(L,"GameCore cached include actual environment keeper changed");
    lua_pushliteral(L,"RefreshIncludeFileList");lua_rawget(L,LUA_GLOBALSINDEX);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.profile_include_refresh_ref);
    current=lua_iscfunction(L,-1)&&lua_tocfunction(L,-1)==native_master_refresh_include_table&&
        lua_topointer(L,-1)==p.profile_include_refresh_pointer&&lua_rawequal(L,-1,-2)&&!lua_getupvalue(L,-1,1);
    lua_settop(L,top);if(!current)return (void)luaL_error(L,"GameCore cached include genuine existing Refresh publisher changed");
    lua_pushliteral(L,"IncludeFileList");lua_rawget(L,LUA_GLOBALSINDEX);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.profile_include_table_ref);const int list=lua_gettop(L);
    current=lua_istable(L,list)&&lua_topointer(L,list)==p.profile_include_table_pointer&&lua_rawequal(L,-1,-2)&&
        !lua_getmetatable(L,list)&&lua_objlen(L,list)==p.profile_include_names.size();
    if(!current){lua_settop(L,top);return (void)luaL_error(L,"GameCore cached include SAME actual ordered table changed");}
    for(std::size_t i=0;i<p.profile_include_names.size();++i){
        lua_rawgeti(L,list,static_cast<int>(i+1));std::size_t n=0;
        const char*const name=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        current=name&&n==p.profile_include_names[i].size()&&!std::memcmp(name,p.profile_include_names[i].data(),n);
        lua_settop(L,list);if(!current){lua_settop(L,top);return (void)luaL_error(L,"GameCore cached include ordered name changed");}
    }
    lua_settop(L,top);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.include_configuration_ref);
    current=lua_isfunction(L,-1)&&!lua_iscfunction(L,-1)&&lua_topointer(L,-1)==p.include_configuration_pointer&&
        graph_contains_proto(p.include_bootstrap_proto,static_cast<const Closure*>(lua_topointer(L,-1))->l.p);
    lua_settop(L,top);if(!current)return (void)luaL_error(L,"GameCore cached include actual retained factory changed");
}
static bool native_gamecore_include_capture_physical_owned(lua_State* L,int function,int environment,
                                                          std::string_view associated_context){
    auto& p=owner(L);
    if(!native_include_list_upvalue_without_callbacks(L,function,p.profile_include_table_pointer))return false;
    const auto*const closure=static_cast<const Closure*>(lua_topointer(L,function));
    bool found_environment=false,found_context=false;
    for(unsigned i=0;i<closure->l.nupvalues;++i){
        const auto*const name=closure->l.p->upvalues[i];
        if(!name)continue;
        const bool is_environment=name->tsv.len==3&&!std::memcmp(getstr(name),"env",3);
        const bool is_context=name->tsv.len==7&&!std::memcmp(getstr(name),"context",7);
        if(!is_environment&&!is_context)continue;
        const auto*const upvalue=closure->l.upvals[i];
        if(!upvalue||upvalue->v!=&upvalue->u.value)return false;
        if(is_environment){
            if(found_environment||!ttistable(upvalue->v)||hvalue(upvalue->v)!=lua_topointer(L,environment))return false;
            found_environment=true;
        }else{
            if(found_context||!ttisstring(upvalue->v)||tsvalue(upvalue->v)->len!=associated_context.size()||
               (!associated_context.empty()&&std::memcmp(svalue(upvalue->v),associated_context.data(),associated_context.size())))return false;
            found_context=true;
        }
    }
    return found_environment&&found_context;
}
static void native_gamecore_profile_cached_include_owned(lua_State* L,int environment,
                                                        std::string_view associated_context){
    const int top=lua_gettop(L);if(environment<0)environment=top+environment+1;
    if(!lua_checkstack(L,16))return (void)luaL_error(L,"GameCore cached include protected stack unavailable");
    auto& p=owner(L);native_gamecore_cached_include_current_owned(L,environment,associated_context);
    // Original519710/18/22 HIT observes the retained Refresh and skips582AC0.
    lua_getglobal(L,"RefreshIncludeFileList");
    native_gamecore_cached_include_current_owned(L,environment,associated_context);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_master_refresh_include_table||
       lua_topointer(L,-1)!=p.profile_include_refresh_pointer||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"GameCore IncludeFileList HIT refresh identity changed");
    lua_pop(L,1); // original519758 lua_settop(-2): no refresh call
    lua_getglobal(L,"IncludeFileList");
    native_gamecore_cached_include_current_owned(L,environment,associated_context);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.profile_include_table_pointer)
        return (void)luaL_error(L,"GameCore IncludeFileList HIT table changed");
    const int actual_list=lua_gettop(L);
    // The real existing factory creates a fresh include closure for THIS env;
    // selected_list bypasses its refresh branch. No include body is invoked.
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.include_configuration_ref);
    native_gamecore_cached_include_current_owned(L,environment,associated_context);
    lua_pushvalue(L,environment);
    lua_pushlstring(L,associated_context.data(),associated_context.size());
    lua_pushvalue(L,actual_list);
    native_gamecore_cached_include_current_owned(L,environment,associated_context);
    lua_call(L,3,0);
    native_gamecore_cached_include_current_owned(L,environment,associated_context);
    // Original replaces env.include even if a prior value exists. Check the
    // actual factory output and closed list/env/context capture, never values.
    lua_pushliteral(L,"include");lua_rawget(L,environment);
    if(!native_gamecore_include_capture_physical_owned(L,lua_gettop(L),environment,associated_context))
        return (void)luaL_error(L,"GameCore actual new include closed captures/Proto changed");
    const int actual_function=lua_gettop(L); // retain real output on Lua stack through Source POST
    native_gamecore_cached_include_current_owned(L,environment,associated_context);
    lua_pushliteral(L,"include");lua_rawget(L,environment);
    if(!lua_rawequal(L,actual_function,-1)||
       !native_gamecore_include_capture_physical_owned(L,lua_gettop(L),environment,associated_context))
        return (void)luaL_error(L,"GameCore actual include physical publication changed after Source POST");
    lua_settop(L,top);
}
