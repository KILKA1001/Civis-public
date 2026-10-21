// Inside canonical Host::Impl, before master extensions initializer.
// LuaL_Buffer is a C POD; owning staging lives in Host, outside longjmp frame.
static void native_print_source_full(lua_State* L){
    auto& p=owner(L);p.source_exception={};p.source_error=nullptr;
    try{
        p.worldview_require_callback_origin();const auto* scope=p.worldview_native_host;
        if(!p.profile_print_sink_alive||!p.state||G(L)!=G(p.state)||!scope||!scope->loan||
           scope->child>=p.child_contexts.size())
            throw ScriptError("Native print actual VM/selected Source/sink unavailable");
        scope->loan->require_origin_without_callbacks();scope->loan->require_current();
        p.worldview_require_callback_origin();
        if(scope!=p.worldview_native_host||scope->child>=p.child_contexts.size()||
           !p.worldview_native_child_controls_current(scope->child))
            throw ScriptError("Native print actual Source/child publication changed");
        p.child_require_current(scope->child);p.worldview_require_callback_origin();
    }catch(...){p.source_exception=std::current_exception();p.source_error="Native print Source/VM full fence refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}
static void native_print_publish_owned(lua_State* L,const char* text,std::size_t size){
    auto& p=owner(L);native_print_source_full(L);
    p.source_exception={};p.source_error=nullptr;
    try{
        const auto* scope=p.worldview_native_host;const auto child=scope->child;
        const auto full=[&]{
            p.worldview_require_callback_origin();
            if(scope!=p.worldview_native_host||!p.profile_print_sink_alive||G(L)!=G(p.state)||
               child>=p.child_contexts.size()||!p.worldview_native_child_controls_current(child))
                throw ScriptError("Native print SAME retained VM/Source/sink changed");
            scope->loan->require_current();p.worldview_require_callback_origin();
            if(p.profile_print_input)p.profile_print_input->require_current_owned(*scope->loan);
            p.worldview_require_callback_origin();
        };
        if(p.profile_print_publishing)throw ScriptError("Native print sink publication reentered");
        p.profile_print_publishing=true;
        struct End{Impl& p;~End(){p.profile_print_publishing=false;p.profile_print_input.reset();}}end{p};
        p.profile_print_input=scope->loan->context_lua_profile_config_owned();full();
        const auto logging=p.profile_print_input->logging_enabled_value_owned(*scope->loan);full();
        if(logging!=0&&logging!=1)throw ScriptError("Native print logger config unsupported");
        // Actual immutable LuaSTRING bytes retained by this same live Lua stack.
        // Own staging is not published until FULL after every allocation.
        p.profile_print_console_scratch={p.child_contexts[child].name,std::string(text,size)};full();
        if(logging){p.profile_print_log_scratch={p.child_contexts[child].name,std::string(text,size)};full();}
        if(p.messages.size()==p.messages.max_size())throw ScriptError("Native print console queue exhausted");
        p.messages.reserve(p.messages.size()+1);full();
        if(logging){
            if(p.profile_print_log_messages.size()==p.profile_print_log_messages.max_size())
                throw ScriptError("Native print logger queue exhausted");
            p.profile_print_log_messages.reserve(p.profile_print_log_messages.size()+1);full();
        }
        full();
        // Source has passed the last callback; reserved vectors/string moves do
        // not allocate. Real logger then mandatory console publication order.
        if(logging)p.profile_print_log_messages.push_back(std::move(p.profile_print_log_scratch));
        p.messages.push_back(std::move(p.profile_print_console_scratch));
    }catch(...){p.source_exception=std::current_exception();p.source_error="Native print queue/config publication refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}
static int native_master_print(lua_State* L){
    native_print_source_full(L);auto& p=owner(L);
    // Actual caller thread's globals must be the SAME selected child environment.
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[p.worldview_native_host->child].environment);
    native_print_source_full(L);
    if(!lua_rawequal(L,-1,LUA_GLOBALSINDEX))return luaL_error(L,"Native print actual child environment changed");
    lua_pop(L,1);const int count=lua_gettop(L);
    lua_getglobal(L,"tostring");native_print_source_full(L);const int tostring_slot=lua_gettop(L);
    luaL_Buffer buffer;luaL_buffinit(L,&buffer);
    lua_getglobal(L,"StateName");native_print_source_full(L);
    if(lua_isstring(L,-1)){
        luaL_addvalue(&buffer);native_print_source_full(L);
        luaL_addstring(&buffer,": ");native_print_source_full(L);
    }else lua_pop(L,1);
    for(int i=1;i<=count;++i){
        if(i>1){luaL_addstring(&buffer,"\t");native_print_source_full(L);}
        lua_pushvalue(L,tostring_slot);lua_pushvalue(L,i);native_print_source_full(L);
        lua_call(L,1,1);native_print_source_full(L);
        if(lua_type(L,-1)!=LUA_TSTRING)return luaL_error(L,"'tostring' must return a string to 'print'");
        luaL_addvalue(&buffer);native_print_source_full(L);
    }
    luaL_pushresult(&buffer);native_print_source_full(L);
    std::size_t size=0;const char* text=lua_tolstring(L,-1,&size);
    if(!text)return luaL_error(L,"Native print actual built string absent");
    native_print_publish_owned(L,text,size);return 0;
}
static void install_native_master_print(lua_State* L){
    auto& p=owner(L);
    if(p.profile_print_sink_alive||p.profile_print_function_ref!=LUA_NOREF)
        return (void)luaL_error(L,"Native print canonical sink constructor replay");
    lua_pushcfunction(L,native_master_print);p.profile_print_function=lua_topointer(L,-1);
    lua_pushvalue(L,-1);p.profile_print_function_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    lua_setglobal(L,"print");p.profile_print_sink_alive=true;
}
