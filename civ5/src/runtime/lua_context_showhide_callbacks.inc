// Inside the actual Host Impl, after ContextCallbackEntry. The registry owns
// the real +524 function; no XML name or fabricated callback is a supplier.
std::deque<WorldViewContextCallbackEntry> worldview_showhide_callbacks;
bool worldview_showhide_registering=false,worldview_showhide_dispatching=false;
std::set<std::size_t> worldview_initial_showhide_returns;

void worldview_showhide_identity(std::size_t index,bool loaded){
    worldview_require_callback_origin();
    if(index>=worldview_showhide_callbacks.size())throw ScriptError("Context showhide occurrence absent");
    const auto& e=worldview_showhide_callbacks[index];
    const auto vm=e.vm_owner.lock();const auto life=e.lifetime.lock();const auto graph=e.graph.lock();
    if(e.retired||e.epoch!=failure_epoch||e.uncertainty!=execution_uncertainty||!vm||vm.get()!=this||
        vm.owner_before(worldview_self)||worldview_self.owner_before(vm)||!life||!end_turn_lifetime||
        life.get()!=end_turn_lifetime.get()||life.owner_before(end_turn_lifetime)||end_turn_lifetime.owner_before(life)||
        !life->alive||life->failed||life->host!=e.host||life->creator!=ui_thread||e.host!=worldview_host_owner||
        !graph||e.graph_slot>=control_graphs.size()||e.child>=child_contexts.size()||
        e.compiled>=graph_compiled_sources.size()||!worldview_native_child_controls_current(e.child))
        throw ScriptError("Context showhide canonical VM/creator/native scope retired");
    const auto& child=child_contexts[e.child];const auto& binding=control_graphs[e.graph_slot];
    if(graph.get()!=binding.graph.get()||graph.owner_before(binding.graph)||binding.graph.owner_before(graph)||
        !graph->backend_||!binding.bound||child.retired||!child.source||
        e.source.get()!=child.source.get()||e.source.owner_before(child.source)||child.source.owner_before(e.source)||
        child.environment!=e.environment||binding.environment!=e.graph_environment||child.execution_source!=e.compiled||
        graph_compiled_sources[e.compiled].root!=e.root||graph_compiled_sources[e.compiled].retained==LUA_NOREF||
        graph_compiled_sources[e.compiled].context!=child.name||graph->context()!=child.name||
        (loaded&&(!e.published||!child.loaded||child.loading||!executed_contexts.contains(child.name))))
        throw ScriptError("Context showhide SAME graph/source/coroutine occurrence changed");
}
void worldview_showhide_owner(std::size_t index,const WorldViewCallbackFrame& frame,bool loaded){
    worldview_callback_fence(frame);worldview_showhide_identity(index,loaded);
    worldview_operation->require_same_vm(*worldview_host_owner);
    worldview_callback_fence(frame);worldview_showhide_identity(index,loaded);
    worldview_operation->require_same_source(*worldview_showhide_callbacks[index].source);
    worldview_callback_fence(frame);worldview_showhide_identity(index,loaded);
    child_require_current(worldview_showhide_callbacks[index].child);
    worldview_callback_fence(frame);worldview_showhide_identity(index,loaded);
}
static bool worldview_showhide_refs(lua_State* L,std::size_t index){
    auto& p=owner(L);const auto& e=p.worldview_showhide_callbacks[index];
    if(!lua_checkstack(L,8))return false;
    const int top=lua_gettop(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,e.thread);
    const bool thread=lua_isthread(L,-1)&&lua_tothread(L,-1)==L;lua_pop(L,1);
    lua_rawgeti(L,LUA_REGISTRYINDEX,e.environment);lua_rawgeti(L,LUA_REGISTRYINDEX,e.graph_environment);
    const bool env=lua_istable(L,-1)&&lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==e.environment_identity;lua_pop(L,1);
    lua_pushliteral(L,"ContextPtr");lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,e.context);
    const bool context=lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==e.context_identity;lua_pop(L,3);
    lua_rawgeti(L,LUA_REGISTRYINDEX,e.function);
    bool fn=lua_isnil(L,-1)&&e.function==LUA_REFNIL&&e.closure==nullptr;
    if(!fn){
        fn=lua_isfunction(L,-1)&&!lua_iscfunction(L,-1)&&lua_topointer(L,-1)==e.closure&&p.graph_compiled_owner(L,-1)==e.compiled;
        lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,e.environment);fn=fn&&lua_rawequal(L,-1,-2);lua_pop(L,2);
    }
    lua_settop(L,top);return thread&&env&&context&&fn;
}
static void worldview_register_showhide_owned(lua_State* L,std::size_t slot,int function){
    auto& p=owner(L);p.worldview_require_callback_origin();
    if(p.worldview_control_entry!=L||slot>=p.control_graphs.size()||!p.control_graphs[slot].bound||
        !p.control_graphs[slot].graph||!p.control_graphs[slot].graph->backend_||lua_gettop(L)!=2||function!=2||
        !lua_istable(L,1)||!lua_rawequal(L,1,lua_upvalueindex(3))||
        (!lua_isnil(L,2)&&(!lua_isfunction(L,2)||lua_iscfunction(L,2)))||
        p.worldview_showhide_registering||p.worldview_showhide_dispatching)
        return (void)luaL_error(L,"SetShowHideHandler actual compiled receiver/function or nil required; replacement during callback unsupported");
    const auto* native=p.worldview_native_host;const auto* loader=p.end_turn_child_loader;
    if(!native||!loader||!loader->current()||loader->child!=native->child||native->child>=p.child_contexts.size())
        return (void)luaL_error(L,"SetShowHideHandler requires actual initial compiled child-load occurrence");
    const auto child=native->child;const auto& c=p.child_contexts[child];
    if(c.name!=p.control_graphs[slot].graph->context()||c.loaded||!c.loading||c.retired||c.environment==LUA_NOREF||
        c.execution_source==graph_no_owner||c.execution_source!=p.graph_execution_scope)
        return (void)luaL_error(L,"SetShowHideHandler native Context/source occurrence differs");
    lua_Debug caller{};std::size_t compiled=graph_no_owner;bool caller_env=false;
    for(int level=1;level<=256&&lua_getstack(L,level,&caller);++level){
        if(!lua_getinfo(L,"f",&caller))break;
        if(lua_iscfunction(L,-1)){lua_pop(L,1);continue;}
        compiled=p.graph_compiled_owner(L,-1);lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,c.environment);
        caller_env=lua_rawequal(L,-1,-2);lua_pop(L,3);break;
    }
    if(compiled!=c.execution_source||compiled==graph_no_owner||!caller_env||
        (!lua_isnil(L,2)&&p.graph_compiled_owner(L,2)!=compiled))
        return (void)luaL_error(L,"SetShowHideHandler actual caller/handler Proto differs");
    if(!lua_isnil(L,2)){
        lua_getfenv(L,2);lua_rawgeti(L,LUA_REGISTRYINDEX,c.environment);const bool same=lua_rawequal(L,-1,-2);lua_pop(L,2);
        if(!same)return (void)luaL_error(L,"SetShowHideHandler actual closure environment differs");
    }
    lua_rawgeti(L,LUA_REGISTRYINDEX,c.environment);lua_rawgeti(L,LUA_REGISTRYINDEX,p.control_graphs[slot].environment);
    const bool env=lua_rawequal(L,-1,-2);lua_pop(L,1);const auto environment=lua_topointer(L,-1);
    lua_pushliteral(L,"ContextPtr");lua_rawget(L,-2);const bool receiver=lua_rawequal(L,1,-1);lua_pop(L,2);
    if(!env||!receiver)return (void)luaL_error(L,"SetShowHideHandler retained context/graph environment changed");
    for(const auto& e:p.worldview_showhide_callbacks)if(e.child==child)
        return (void)luaL_error(L,"SetShowHideHandler replacement requires a subsequent owned registration lifecycle");
    if(p.worldview_showhide_callbacks.size()>=4096)return (void)luaL_error(L,"SetShowHideHandler context budget exhausted");
    const auto index=p.worldview_showhide_callbacks.size();p.worldview_showhide_registering=true;
    p.source_exception={};p.source_error=nullptr;
    try{
        p.worldview_showhide_callbacks.emplace_back();auto& e=p.worldview_showhide_callbacks.back();
        e.vm_owner=p.worldview_self;e.lifetime=p.worldview_lifetime;e.host=p.worldview_host_owner;e.source=c.source;
        e.graph=p.control_graphs[slot].graph;e.graph_slot=slot;e.child=child;e.compiled=compiled;
        e.root=p.graph_compiled_sources[compiled].root;e.registered=L;e.environment=c.environment;
        e.graph_environment=p.control_graphs[slot].environment;e.environment_identity=environment;
        e.context_identity=lua_topointer(L,1);e.closure=lua_isnil(L,2)?nullptr:lua_topointer(L,2);
        e.epoch=p.failure_epoch;e.uncertainty=p.execution_uncertainty;
        const auto frame=p.worldview_callback_frame();p.worldview_showhide_owner(index,frame,false);
    }catch(...){p.source_exception=std::current_exception();p.source_error="SetShowHideHandler actual owner refused";}
    if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
    lua_pushvalue(L,2);p.worldview_showhide_callbacks[index].function=luaL_ref(L,LUA_REGISTRYINDEX);
    lua_pushthread(L);p.worldview_showhide_callbacks[index].thread=luaL_ref(L,LUA_REGISTRYINDEX);
    lua_pushvalue(L,1);p.worldview_showhide_callbacks[index].context=luaL_ref(L,LUA_REGISTRYINDEX);
    p.source_exception={};p.source_error=nullptr;
    try{
        const auto frame=p.worldview_callback_frame();p.worldview_showhide_owner(index,frame,false);
        if(!worldview_showhide_refs(L,index))throw ScriptError("SetShowHideHandler real refs changed before native flag store");
        BnwWorldViewScriptUiAdapter::register_context_showhide_owned(p.control_graphs[slot].graph,*p.worldview_operation,*p.worldview_host_owner);
        p.worldview_require_callback_origin();p.worldview_showhide_owner(index,frame,false);
        if(!worldview_showhide_refs(L,index))throw ScriptError("SetShowHideHandler real refs changed after native flag store");
        p.worldview_showhide_callbacks[index].published=true;p.worldview_showhide_registering=false;
    }catch(...){p.worldview_showhide_callbacks[index].retired=true;p.source_exception=std::current_exception();p.source_error="SetShowHideHandler actual flag/registry return refused";}
    if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
}
struct ContextInitialShowHideJob {
    Impl* owner=nullptr;std::size_t index=graph_no_owner;bool hidden=false;
    bool function=false,invoked=false,returned=false;int result=LUA_NOREF;
};
ContextInitialShowHideJob context_initial_showhide_job;
static int context_initial_showhide_query(lua_State* L){
    auto& job=*static_cast<ContextInitialShowHideJob*>(lua_touserdata(L,1));auto& p=*job.owner;
    if(&owner(L)!=&p||job.index>=p.worldview_showhide_callbacks.size()||
        p.worldview_showhide_callbacks[job.index].registered!=L||!worldview_showhide_refs(L,job.index))
        return luaL_error(L,"Initial showhide query actual registry objects changed");
    lua_rawgeti(L,LUA_REGISTRYINDEX,p.worldview_showhide_callbacks[job.index].function);
    job.function=lua_isfunction(L,-1);
    if(!job.function&&!lua_isnil(L,-1))return luaL_error(L,"Initial showhide unsupported non-function object");
    lua_pop(L,1);p.worldview_require_callback_origin();return 0;
}
static int context_initial_showhide_protected(lua_State* L){
    auto& job=*static_cast<ContextInitialShowHideJob*>(lua_touserdata(L,1));auto& p=*job.owner;
    if(&owner(L)!=&p||job.index>=p.worldview_showhide_callbacks.size())return luaL_error(L,"Initial showhide actual VM changed");
    const auto& e=p.worldview_showhide_callbacks[job.index];
    if(e.registered!=L||e.retired||!e.published||!worldview_showhide_refs(L,job.index))
        return luaL_error(L,"Initial showhide retained function/context objects changed");
    child_require_bindings(L);lua_rawgeti(L,LUA_REGISTRYINDEX,e.function);
    if(!lua_isfunction(L,-1))return luaL_error(L,"Initial showhide actual IsFunction branch changed");
    p.graph_execution_scope=e.compiled;job.invoked=true;
    lua_pushboolean(L,job.hidden);lua_pushboolean(L,1);lua_call(L,2,1);
    // FIRST after original Lua: a retired native/VM facade is rejected before
    // any result registry API or mutable callback-state borrow.
    p.source_exception={};p.source_error=nullptr;
    try{p.worldview_require_callback_origin();}
    catch(const std::exception& error){p.source_exception=std::current_exception();p.source_error=error.what();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Initial showhide native origin retired on Lua return";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    // Real result temporary is rooted then released before loader cleanup.
    job.result=luaL_ref(L,LUA_REGISTRYINDEX);luaL_unref(L,LUA_REGISTRYINDEX,job.result);job.result=LUA_NOREF;
    p.worldview_require_callback_origin();job.returned=true;return 0;
}
void worldview_dispatch_initial_showhide(std::size_t child,bool hidden){
    worldview_require_callback_origin();
    if(calling||worldview_showhide_registering||worldview_showhide_dispatching||worldview_callback_dispatching||
        worldview_control_entry||worldview_control_validation||child>=child_contexts.size())
        throw ScriptError("Initial showhide actual dispatch reentry or child absent");
    std::size_t index=0;for(;index<worldview_showhide_callbacks.size();++index)if(worldview_showhide_callbacks[index].child==child)break;
    if(index==worldview_showhide_callbacks.size())throw ScriptError("Flag8000000 has no genuine SetShowHideHandler owner");
    if(worldview_initial_showhide_returns.contains(child))throw ScriptError("Initial showhide suffix replay");
    const auto frame=worldview_callback_frame();worldview_showhide_owner(index,frame,true);
    auto* L=worldview_showhide_callbacks[index].registered;
    const int top=lua_gettop(L);const auto previous=graph_execution_scope;
    const auto prior_hook=lua_gethook(L);const int prior_mask=lua_gethookmask(L),prior_count=lua_gethookcount(L);
    const auto prior_remaining=remaining;const bool prior_exhausted=exhausted;
    struct Restore{Impl& p;lua_State* L;int top;std::size_t graph;lua_Hook hook;int mask,count;std::uint64_t budget;bool exhausted;
        ~Restore(){lua_sethook(L,hook,mask,count);lua_settop(L,top);p.graph_execution_scope=graph;
            p.remaining=budget;p.exhausted=exhausted;p.calling=false;p.worldview_callback_dispatching=false;p.worldview_showhide_dispatching=false;}
    }restore{*this,L,top,previous,prior_hook,prior_mask,prior_count,prior_remaining,prior_exhausted};
    context_initial_showhide_job={this,index,hidden};auto& job=context_initial_showhide_job;
    const int query_status=lua_cpcall(L,context_initial_showhide_query,&job);worldview_require_callback_origin();
    if(query_status)throw ScriptError(worldview_callback_lua_error(L));
    worldview_showhide_owner(index,frame,true);
    if(!job.function){
        if(worldview_showhide_callbacks[index].function!=LUA_REFNIL)throw ScriptError("Initial showhide unsupported non-function object");
        worldview_showhide_owner(index,frame,true);worldview_initial_showhide_returns.insert(child);return; // actual nil IsFunction=false
    }
    worldview_callback_dispatching=true;worldview_showhide_dispatching=true;calling=true;
    remaining=budget.instructions;exhausted=false;
    lua_sethook(L,budget.instructions?hook:nullptr,budget.instructions?LUA_MASKCOUNT:0,1000);
    int status=0;
    {WorldViewLoadedChildCallScope loaded(*this,WorldViewLoadedChildCallScope::Kind::showhide,index);
        status=lua_cpcall(L,context_initial_showhide_protected,&job);
        if(!status&&!exhausted)loaded.accept(job.invoked,job.returned);
    }
    worldview_require_callback_origin();calling=false;graph_execution_scope=previous;
    if(status){const auto error=worldview_callback_lua_error(L);throw ScriptError(error);}
    if(exhausted||!job.invoked||!job.returned||job.result!=LUA_NOREF)throw ScriptError("Initial showhide/result cleanup did not return");
    worldview_showhide_owner(index,frame,true);
    worldview_initial_showhide_returns.insert(child);
}
