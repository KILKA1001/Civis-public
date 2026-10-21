// Same Host master IncludeFileList prefix; include4972D0 remains UNISSUED.
// Frame lives outside protected Lua call; no pointer can survive its real DTOR.
struct NativeContextIncludeTableFrame final {
    Impl& p;Job& job;const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;
    NativeContextIncludeTableFrame(Impl& actual,Job& j):p(actual),job(j),
        epoch(actual.failure_epoch),writes(actual.failed.writes_),uncertainty(actual.execution_uncertainty){
        p.worldview_require_callback_origin();
        if(p.profile_include_frame||j.owner!=&p||
           (j.operation!=Operation::native_context_debug_config_prefix&&j.operation!=Operation::native_context_debug_config_query&&
            j.operation!=Operation::native_context_profile_restart&&j.operation!=Operation::native_context_profile_completed_query))
            throw ScriptError("Master include table lexical frame origin/reentry");
        p.profile_include_frame=this;
    }
    void full()const{
        p.worldview_require_callback_origin();
        if(p.profile_include_frame!=this||creator!=std::this_thread::get_id()||
           epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty||
           !p.worldview_native_host||p.worldview_native_host->child!=job.worldview_child||
           !job.context_setup_preparation||!job.context_setup_name_loan||!job.context_profile_state_name)
            throw ScriptError("Master include table actual lexical frame changed");
        const auto& op=*p.worldview_native_host->loan;
        job.context_setup_name_loan->require_current_without_callbacks(op,job.context_setup_preparation);
        op.require_current();p.worldview_require_callback_origin();
        job.context_setup_name_loan->require_current_owned(op,job.context_setup_preparation);
        p.worldview_require_callback_origin();
        if(job.context_profile_config){job.context_profile_config->require_current_owned(op);p.worldview_require_callback_origin();}
        if(p.profile_include_collection){p.profile_include_collection->require_current_owned(op);p.worldview_require_callback_origin();}
        job.context_setup_name_loan->require_current_without_callbacks(op,job.context_setup_preparation);
        if(p.profile_include_collection)p.profile_include_collection->require_current_without_callbacks(op);
        p.worldview_require_callback_origin();
        if(p.profile_include_frame!=this||creator!=std::this_thread::get_id()||epoch!=p.failure_epoch||
           writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty||
           p.worldview_native_host->child!=job.worldview_child||!p.worldview_native_child_controls_current(job.worldview_child))
            throw ScriptError("Master include table final current Source/control changed");
    }
    ~NativeContextIncludeTableFrame()noexcept{
        if(p.profile_include_frame==this)p.profile_include_frame=nullptr;
        else {p.profile_include_frame=nullptr;p.record_failure();}
    }
};
struct NativeContextIncludeTableControl final {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const void> host;
    const std::shared_ptr<const BnwContextLuaIncludeCollectionLoan> collection;
    lua_State*const vm;const void*const table;const void*const refresh;
    const int table_ref,refresh_ref;const std::uint64_t epoch,writes,uncertainty;
    NativeContextIncludeTableControl(const std::shared_ptr<Impl>& h,
      std::shared_ptr<const BnwContextLuaIncludeCollectionLoan> c,lua_State* v,const void* t,
      const void* f,int tr,int fr,std::uint64_t e,std::uint64_t w,std::uint64_t u)
      :host(h),collection(std::move(c)),vm(v),table(t),refresh(f),table_ref(tr),refresh_ref(fr),epoch(e),writes(w),uncertainty(u){}
};
static void native_include_table_full(lua_State* L){
    auto& p=owner(L);p.source_exception={};p.source_error=nullptr;
    try{
        // Source/Host FIRST precedes reading the actual outside-Lua frame pointer.
        p.worldview_require_callback_origin();
        if(!p.profile_include_frame||G(L)!=G(p.state))throw ScriptError("Master include table actual frame/VM absent");
        p.profile_include_frame->full();
    }catch(...){p.source_exception=std::current_exception();p.source_error="Master include table current lender refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}
static int native_master_refresh_include_table(lua_State* L){
    auto& p=owner(L);native_include_table_full(L);
    if(lua_gettop(L)||!p.profile_include_refreshing||!p.profile_include_collection||
       p.profile_include_table_ref!=LUA_NOREF)
        return luaL_error(L,"Master include first table producer phase/replay unsupported");
    lua_getglobal(L,"IncludeFileList");native_include_table_full(L);
    if(!lua_isnil(L,-1))return luaL_error(L,"Unowned existing master IncludeFileList unsupported");
    lua_pop(L,1);lua_createtable(L,0,0);native_include_table_full(L);
    lua_pushvalue(L,-1);native_include_table_full(L);
    lua_setglobal(L,"IncludeFileList");native_include_table_full(L);
    p.profile_include_table_pointer=lua_topointer(L,-1);
    lua_pushvalue(L,-1);native_include_table_full(L);
    p.profile_include_table_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_include_table_full(L);
    // Fresh actual table has native objlen0. The loop is still actual clear1..len,
    // never a caller-computed count or fabricated empty completed collector.
    const auto count=lua_objlen(L,-1);
    if(count>static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return luaL_error(L,"Master include actual table index bounds unsupported");
    for(std::size_t i=1;i<=count;++i){
        lua_pushnil(L);native_include_table_full(L);
        lua_rawseti(L,-2,static_cast<int>(i));native_include_table_full(L);
    }
    for(std::size_t i=0;i<p.profile_include_names.size();++i){
        // Immutable OWN names come solely from same Bank/Source typed collector.
        lua_pushlstring(L,p.profile_include_names[i].data(),p.profile_include_names[i].size());native_include_table_full(L);
        lua_rawseti(L,-2,static_cast<int>(i+1));native_include_table_full(L);
    }
    native_include_table_full(L);return 0;
}
static void query_native_master_include_table(lua_State* L,Job& job){
    auto& p=owner(L);native_debug_config_full(L,job);native_include_table_full(L);
    if(!p.profile_include_table_control||!p.profile_include_master_alive||!p.profile_include_collection)
        return (void)luaL_error(L,"Master include genuine typed table control absent");
    const auto* c=p.profile_include_table_control.get();
    if(c->creator!=std::this_thread::get_id()||c->host.expired()||
       c->host.owner_before(p.weak_from_this())||p.weak_from_this().owner_before(c->host)||c->vm!=p.state||
       c->collection.get()!=p.profile_include_collection.get()||
       c->collection.owner_before(p.profile_include_collection)||p.profile_include_collection.owner_before(c->collection)||
       c->epoch!=p.failure_epoch||c->writes!=p.failed.writes_||c->uncertainty!=p.execution_uncertainty||
       c->table_ref!=p.profile_include_table_ref||c->refresh_ref!=p.profile_include_refresh_ref||
       c->table!=p.profile_include_table_pointer||c->refresh!=p.profile_include_refresh_pointer)
        return (void)luaL_error(L,"Master include genuine typed table owner changed");
    lua_getglobal(L,"RefreshIncludeFileList");native_include_table_full(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.profile_include_refresh_ref);native_include_table_full(L);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_master_refresh_include_table||
       lua_topointer(L,-1)!=p.profile_include_refresh_pointer||!lua_rawequal(L,-1,-2))
        return (void)luaL_error(L,"Master include actual refresh closure changed");
    lua_pop(L,2);lua_getglobal(L,"IncludeFileList");native_include_table_full(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.profile_include_table_ref);native_include_table_full(L);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.profile_include_table_pointer||!lua_rawequal(L,-1,-2))
        return (void)luaL_error(L,"Master include actual list table changed");
    const int list=lua_gettop(L);
    if(lua_objlen(L,list)!=p.profile_include_names.size())
        return (void)luaL_error(L,"Master include actual list length changed");
    for(std::size_t i=0;i<p.profile_include_names.size();++i){
        lua_rawgeti(L,list,static_cast<int>(i+1));native_include_table_full(L);
        std::size_t n=0;const char* v=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!v||n!=p.profile_include_names[i].size()||std::memcmp(v,p.profile_include_names[i].data(),n))
            return (void)luaL_error(L,"Master include actual ordered name changed");
        lua_pop(L,1);
    }
    lua_pop(L,2);native_include_table_full(L);native_debug_config_full(L,job);
    // Final physical DATA POST after the last observer. Known live interned keys
    // are retained by the master globals; GC is stopped for this genuine prefix.
    // Raw reads cannot invoke __index or Lua callbacks; earlier query used the
    // same stack depth. No observer follows this actual publication snapshot.
    lua_pushliteral(L,"RefreshIncludeFileList");lua_rawget(L,LUA_GLOBALSINDEX);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.profile_include_refresh_ref);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_master_refresh_include_table||
       lua_topointer(L,-1)!=p.profile_include_refresh_pointer||!lua_rawequal(L,-1,-2))
        return (void)luaL_error(L,"Master include refresh late physical change");
    lua_pop(L,2);lua_pushliteral(L,"IncludeFileList");lua_rawget(L,LUA_GLOBALSINDEX);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.profile_include_table_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.profile_include_table_pointer||!lua_rawequal(L,-1,-2)||
       lua_objlen(L,-1)!=p.profile_include_names.size())
        return (void)luaL_error(L,"Master include list late physical change");
    const int final_list=lua_gettop(L);
    for(std::size_t i=0;i<p.profile_include_names.size();++i){
        lua_rawgeti(L,final_list,static_cast<int>(i+1));
        std::size_t n=0;const char* v=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!v||n!=p.profile_include_names[i].size()||std::memcmp(v,p.profile_include_names[i].data(),n))
            return (void)luaL_error(L,"Master include ordered name late physical change");
        lua_pop(L,1);
    }
    lua_pop(L,2);
}
static void perform_native_context_master_include_table_prefix(lua_State* L,Job& job){
    auto& p=owner(L);native_debug_config_full(L,job);native_include_table_full(L);
    if(p.profile_include_table_control||p.profile_include_master_alive||p.profile_include_refreshing||
       p.profile_include_refresh_ref!=LUA_NOREF||p.profile_include_table_ref!=LUA_NOREF)
        return (void)luaL_error(L,"Master include existing/reused table branch not yet qualified");
    p.source_exception={};p.source_error=nullptr;
    try{
        const auto& op=*p.worldview_native_host->loan;
        // Sole genuine SourceРІвЂ вЂ™selected Bank acquisition; unknown topology must STOP.
        p.profile_include_collection=op.context_lua_include_collection_owned();p.profile_include_frame->full();
        if(!p.profile_include_collection)throw ScriptError("Master include actual ordered collector missing");
        p.profile_include_names=p.profile_include_collection->ordered_names_owned(op);p.profile_include_frame->full();
        if(p.profile_include_names.size()>=static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw ScriptError("Master include ordered index exhaustion unsupported");
    }catch(...){p.source_exception=std::current_exception();p.source_error="Master include required ordered collector unavailable";}
    if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
    native_debug_config_full(L,job);native_include_table_full(L);
    lua_getglobal(L,"RefreshIncludeFileList");native_include_table_full(L);
    if(!lua_isnil(L,-1))return (void)luaL_error(L,"Unowned master refresh callback unsupported");
    lua_pop(L,1);lua_pushcfunction(L,native_master_refresh_include_table);native_include_table_full(L);
    p.profile_include_refresh_pointer=lua_topointer(L,-1);
    lua_pushvalue(L,-1);native_include_table_full(L);
    p.profile_include_refresh_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_include_table_full(L);
    lua_pushvalue(L,-1);native_include_table_full(L);
    lua_setglobal(L,"RefreshIncludeFileList");native_include_table_full(L);
    p.profile_include_refreshing=true;lua_call(L,0,0);native_include_table_full(L);
    p.profile_include_refreshing=false;
    p.source_exception={};p.source_error=nullptr;
    try{
        p.profile_include_table_pending=std::make_shared<const NativeContextIncludeTableControl>(p.shared_from_this(),
          p.profile_include_collection,p.state,p.profile_include_table_pointer,p.profile_include_refresh_pointer,
          p.profile_include_table_ref,p.profile_include_refresh_ref,p.failure_epoch,p.failed.writes_,p.execution_uncertainty);
        p.profile_include_frame->full();
        p.profile_include_table_control=std::move(p.profile_include_table_pending);p.profile_include_master_alive=true;
    }catch(...){p.source_exception=std::current_exception();p.source_error="Master include genuine table control allocation refused";}
    if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
    query_native_master_include_table(L,job);
    // Actual include4972D0 capture, traceback6538A0 and GC restart remain mandatory.
}
