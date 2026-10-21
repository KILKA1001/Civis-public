// Ported real include4972D0 factory/list upvalue +6538A0 traceback prefix.
// This private DATA control is not a full61/Controls/preRead setup return.
struct NativeContextIncludeChildControl final {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const void> host;
    const std::shared_ptr<const NativeContextIncludeTableControl> list;
    const std::size_t child;const std::uint64_t serial,epoch,writes,uncertainty;
    const int function_ref,traceback_ref;
    const void*const function;const void*const traceback;
    const Proto*const proto;const UpVal*const list_upvalue;
    NativeContextIncludeChildControl(const std::shared_ptr<Impl>& h,
      std::shared_ptr<const NativeContextIncludeTableControl> l,std::size_t c,
      std::uint64_t s,std::uint64_t e,std::uint64_t w,std::uint64_t u,int fr,int tr,
      const void* f,const void* t,const Proto* p,const UpVal* uv)
      :host(h),list(std::move(l)),child(c),serial(s),epoch(e),writes(w),uncertainty(u),
       function_ref(fr),traceback_ref(tr),function(f),traceback(t),proto(p),list_upvalue(uv){}
};
static const UpVal* native_include_list_upvalue_without_callbacks(lua_State* L,int function,const void* list){
    auto& p=owner(L);
    if(!lua_isfunction(L,function)||lua_iscfunction(L,function))return nullptr;
    const auto* c=static_cast<const Closure*>(lua_topointer(L,function));
    if(!c||!c->l.p||!p.include_bootstrap_proto||!graph_contains_proto(p.include_bootstrap_proto,c->l.p)||
       c->l.nupvalues!=c->l.p->nups||c->l.p->sizeupvalues!=c->l.nupvalues||!c->l.p->upvalues)return nullptr;
    const UpVal* found=nullptr;
    for(unsigned i=0;i<c->l.nupvalues;++i){
        const auto* name=c->l.p->upvalues[i];
        if(name&&name->tsv.len==4&&!std::memcmp(getstr(name),"list",4)){
            const auto* uv=c->l.upvals[i];
            if(found||!uv||uv->v!=&uv->u.value||!ttistable(uv->v)||hvalue(uv->v)!=list)return nullptr;
            found=uv;
        }
    }
    return found;
}
static void native_include_child_physical_query(lua_State* L,int environment,std::size_t slot){
    auto& p=owner(L);
    // Caller has just passed LAST FULL. Nonallocating raw reads/upvalue inspection
    // use already retained live keys/closure and at most the earlier stack peak.
    if(slot>=p.child_contexts.size()||!p.child_contexts[slot].profile_include_child_control)
        return (void)luaL_error(L,"Selected include/traceback actual control absent");
    const auto* c=p.child_contexts[slot].profile_include_child_control.get();
    if(!p.worldview_native_host||p.worldview_native_host->child!=slot||!p.profile_include_master_alive||
       !p.include_configuration_alive||c->creator!=std::this_thread::get_id()||c->host.expired()||
       c->host.owner_before(p.weak_from_this())||p.weak_from_this().owner_before(c->host)||c->child!=slot||
       c->serial!=p.worldview_native_host->loan->operation_serial()||c->epoch!=p.failure_epoch||
       c->writes!=p.failed.writes_||c->uncertainty!=p.execution_uncertainty||
       c->list.get()!=p.profile_include_table_control.get()||
       c->list.owner_before(p.profile_include_table_control)||p.profile_include_table_control.owner_before(c->list)||
       c->function_ref!=p.child_contexts[slot].profile_include_ref||c->traceback_ref!=p.child_contexts[slot].profile_traceback_ref)
        return (void)luaL_error(L,"Selected include/traceback actual private owner changed");
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[slot].environment);
    if(!lua_istable(L,environment)||!lua_rawequal(L,environment,-1))
        return (void)luaL_error(L,"Selected include/traceback actual environment rebound");
    lua_pop(L,1);
    if(lua_getmetatable(L,environment))return (void)luaL_error(L,"Selected include unexpected master fallback");
    lua_pushliteral(L,"include");lua_rawget(L,environment);
    lua_rawgeti(L,LUA_REGISTRYINDEX,c->function_ref);
    if(!lua_isfunction(L,-1)||lua_iscfunction(L,-1)||lua_topointer(L,-1)!=c->function||!lua_rawequal(L,-1,-2)||
       static_cast<const Closure*>(lua_topointer(L,-1))->l.p!=c->proto||
       native_include_list_upvalue_without_callbacks(L,-1,c->list->table)!=c->list_upvalue)
        return (void)luaL_error(L,"Selected include actual factory Proto/list upvalue changed");
    lua_pop(L,2);lua_pushliteral(L,"traceback");lua_rawget(L,environment);
    lua_rawgeti(L,LUA_REGISTRYINDEX,c->traceback_ref);
    if(!lua_isfunction(L,-1)||lua_topointer(L,-1)!=c->traceback||!lua_rawequal(L,-1,-2))
        return (void)luaL_error(L,"Selected actual traceback delivery changed");
    lua_pop(L,2);
}
static void query_native_context_include_traceback_prefix(lua_State* L,Job& job){
    auto& p=owner(L);native_debug_config_full(L,job);native_include_table_full(L);
    query_native_master_include_table(L,job);native_debug_config_full(L,job);
    perform_native_context_state_query(L,job);native_debug_config_full(L,job);native_include_table_full(L);
    native_include_child_physical_query(L,lua_gettop(L),job.worldview_child);
}
static void perform_native_context_include_traceback_prefix(lua_State* L,Job& job){
    auto& p=owner(L);native_debug_config_full(L,job);native_include_table_full(L);
    if(job.worldview_child>=p.child_contexts.size()||p.child_contexts[job.worldview_child].profile_include_child_control||
       p.child_contexts[job.worldview_child].profile_include_ref!=LUA_NOREF||
       p.child_contexts[job.worldview_child].profile_traceback_ref!=LUA_NOREF||
       !p.include_configuration_alive||p.include_configuration_ref==LUA_NOREF||p.sources.strict_map_includes)
        return (void)luaL_error(L,"Selected include true factory missing/replay or map variant unsupported");
    query_native_master_include_table(L,job);native_debug_config_full(L,job);
    perform_native_context_state_query(L,job);native_debug_config_full(L,job);
    const int environment=lua_gettop(L);
    lua_pushliteral(L,"include");native_debug_config_full(L,job);lua_rawget(L,environment);native_debug_config_full(L,job);
    if(!lua_isnil(L,-1))return (void)luaL_error(L,"Unowned existing selected include unsupported");
    lua_pop(L,1);lua_rawgeti(L,LUA_REGISTRYINDEX,p.include_configuration_ref);native_debug_config_full(L,job);
    if(!lua_isfunction(L,-1)||lua_iscfunction(L,-1)||lua_topointer(L,-1)!=p.include_configuration_pointer||
       !graph_contains_proto(p.include_bootstrap_proto,static_cast<const Closure*>(lua_topointer(L,-1))->l.p))
        return (void)luaL_error(L,"Selected include genuine protected factory changed");
    lua_pushvalue(L,environment);native_debug_config_full(L,job);
    lua_pushlstring(L,p.child_contexts[job.worldview_child].name.data(),p.child_contexts[job.worldview_child].name.size());
    native_debug_config_full(L,job);lua_rawgeti(L,LUA_REGISTRYINDEX,p.profile_include_table_ref);native_include_table_full(L);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.profile_include_table_pointer)
        return (void)luaL_error(L,"Selected include SAME actual master table absent");
    lua_call(L,3,0);native_debug_config_full(L,job);native_include_table_full(L);
    query_native_master_include_table(L,job);native_debug_config_full(L,job);
    lua_pushliteral(L,"include");native_debug_config_full(L,job);lua_rawget(L,environment);native_debug_config_full(L,job);
    const auto* list_upvalue=native_include_list_upvalue_without_callbacks(L,-1,p.profile_include_table_pointer);
    if(!list_upvalue)return (void)luaL_error(L,"Selected include real closed list upvalue/Proto absent");
    p.child_contexts[job.worldview_child].profile_include_pointer=lua_topointer(L,-1);
    p.child_contexts[job.worldview_child].profile_include_proto=static_cast<const Closure*>(lua_topointer(L,-1))->l.p;
    p.child_contexts[job.worldview_child].profile_include_list_upvalue=list_upvalue;
    p.child_contexts[job.worldview_child].profile_include_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_debug_config_full(L,job);
    // Original6538A0 occurs only AFTER actual include factory call returns.
    lua_getglobal(L,LUA_DBLIBNAME);native_debug_config_full(L,job);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.profile_debug_master_table)
        return (void)luaL_error(L,"Actual master debug producer changed before traceback");
    lua_getfield(L,-1,"traceback");native_debug_config_full(L,job);
    if(!lua_isfunction(L,-1))return (void)luaL_error(L,"Cannot find debug.traceback method.");
    lua_remove(L,-2);native_debug_config_full(L,job);
    p.child_contexts[job.worldview_child].profile_traceback_pointer=lua_topointer(L,-1);
    lua_pushvalue(L,-1);native_debug_config_full(L,job);
    p.child_contexts[job.worldview_child].profile_traceback_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_debug_config_full(L,job);
    lua_setfield(L,environment,"traceback");native_debug_config_full(L,job);native_include_table_full(L);
    p.source_exception={};p.source_error=nullptr;
    try{
        const auto& child=p.child_contexts[job.worldview_child];
        p.profile_include_child_pending=std::make_shared<const NativeContextIncludeChildControl>(p.shared_from_this(),
          p.profile_include_table_control,job.worldview_child,p.worldview_native_host->loan->operation_serial(),
          p.failure_epoch,p.failed.writes_,p.execution_uncertainty,child.profile_include_ref,child.profile_traceback_ref,
          child.profile_include_pointer,child.profile_traceback_pointer,child.profile_include_proto,child.profile_include_list_upvalue);
        p.profile_include_frame->full();
        p.child_contexts[job.worldview_child].profile_include_child_control=std::move(p.profile_include_child_pending);
    }catch(...){p.source_exception=std::current_exception();p.source_error="Selected include/traceback true control allocation refused";}
    if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
    query_native_context_include_traceback_prefix(L,job);
    // Genuine partial result only: normal GC restart4EC639/full61 unissued.
}
// Late create consumes actual function, never a caller ready bit/flat source ID.
static void native_context_include_delivery_full(lua_State* L,std::size_t slot){
    native_context_print_delivery_full(L,slot);auto& p=owner(L);
    p.source_exception={};p.source_error=nullptr;
    try{
        if(!p.profile_include_collection)throw ScriptError("Selected actual include collector retired");
        const auto& op=*p.worldview_native_host->loan;
        p.profile_include_collection->require_current_without_callbacks(op);
        p.profile_include_collection->require_current_owned(op);p.worldview_require_callback_origin();
        if(!p.worldview_native_host||p.worldview_native_host->child!=slot)
            throw ScriptError("Selected include actual late Source child changed");
        p.profile_include_collection->require_current_without_callbacks(op);
    }catch(...){p.source_exception=std::current_exception();p.source_error="Selected include genuine collector/source POST refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}
static void push_native_context_include_delivery(lua_State* L,std::size_t slot,int environment){
    native_context_include_delivery_full(L,slot);auto& p=owner(L);
    native_include_child_physical_query(L,environment,slot);
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[slot].profile_include_ref);
    native_context_include_delivery_full(L,slot);
    native_include_child_physical_query(L,environment,slot);
}
