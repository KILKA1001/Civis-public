// Inside LuaScriptHost::Impl, after Child/Graph/NativeScope/owned-control callbacks.
// All registry references live in Impl even when an allocating Lua API longjmps.
// Native callers must issue a fresh Host scope with its exact selected child.
struct WorldViewContextCallbackEntry {
    std::weak_ptr<Impl> vm_owner;
    std::weak_ptr<EndTurnQueue::VmLifetime> lifetime;
    std::shared_ptr<const ScriptUiChildSource> source;
    std::weak_ptr<ScriptUiControlGraph> graph;
    const LuaScriptHost* host=nullptr;
    lua_State* registered=nullptr;
    std::size_t graph_slot=graph_no_owner,child=graph_no_owner,compiled=graph_no_owner;
    const Proto* root=nullptr;
    const void* closure=nullptr;const void* environment_identity=nullptr;const void* context_identity=nullptr;
    int function=LUA_NOREF,thread=LUA_NOREF,environment=LUA_NOREF,graph_environment=LUA_NOREF,context=LUA_NOREF;
    std::uint64_t epoch=0,uncertainty=0;
    bool shutdown=false,published=false,retired=false,shutdown_returned=false;
};
std::deque<WorldViewContextCallbackEntry> worldview_context_callbacks;
bool worldview_context_registering=false,worldview_context_dispatching=false;

void worldview_context_callback_identity(std::size_t index,bool loaded){
    if(ui_thread!=std::this_thread::get_id())throw ScriptError("WorldView context identity foreign creator");
    worldview_require_callback_origin();
    if(index>=worldview_context_callbacks.size())throw ScriptError("WorldView context callback occurrence absent");
    const auto& e=worldview_context_callbacks[index];
    if(e.retired||e.shutdown_returned||e.epoch!=failure_epoch||e.uncertainty!=execution_uncertainty||
       e.graph_slot>=control_graphs.size()||e.child>=child_contexts.size()||e.compiled>=graph_compiled_sources.size())
        throw ScriptError("WorldView context callback occurrence retired");
    const auto vm=e.vm_owner.lock();const auto life=e.lifetime.lock();const auto graph=e.graph.lock();
    const auto& child=child_contexts[e.child];const auto& binding=control_graphs[e.graph_slot];
    if(!vm||vm.get()!=this||vm.owner_before(worldview_self)||worldview_self.owner_before(vm)||
       !life||!end_turn_lifetime||life.get()!=end_turn_lifetime.get()||
       life.owner_before(end_turn_lifetime)||end_turn_lifetime.owner_before(life)||
       !life->alive||life->failed||life->creator!=ui_thread||life->host!=e.host||life->vm!=state||e.host!=worldview_host_owner||
       !graph||graph.get()!=binding.graph.get()||graph.owner_before(binding.graph)||binding.graph.owner_before(graph)||
       !graph->backend_||!binding.bound||child.retired||!child.source||
       e.source.get()!=child.source.get()||e.source.owner_before(child.source)||child.source.owner_before(e.source)||
       child.environment!=e.environment||binding.environment!=e.graph_environment||child.execution_source!=e.compiled||
       graph_compiled_sources[e.compiled].root!=e.root||graph_compiled_sources[e.compiled].retained==LUA_NOREF||
       graph_compiled_sources[e.compiled].context!=child.name||graph->context()!=child.name||
       child.name!=(e.shutdown?"UnitFlagManager":"WorldView")||
       !worldview_native_child_controls_current(e.child))
        throw ScriptError("WorldView context callback canonical VM/source/graph/child differs");
    if(loaded&&(!e.published||!child.loaded||child.loading||!executed_contexts.contains(child.name)))
        throw ScriptError("WorldView context callback invoked before genuine child return");
}
void worldview_context_callback_owner(std::size_t index,const WorldViewCallbackFrame& frame,bool loaded){
    worldview_callback_fence(frame);worldview_context_callback_identity(index,loaded);
    // Each identity lock dies before the next callbackful source operation.
    worldview_operation->require_same_vm(*worldview_host_owner);
    worldview_callback_fence(frame);worldview_context_callback_identity(index,loaded);
    worldview_operation->require_same_source(*worldview_context_callbacks[index].source);
    worldview_callback_fence(frame);worldview_context_callback_identity(index,loaded);
    worldview_context_callbacks[index].source->require_current();
    worldview_callback_fence(frame);worldview_context_callback_identity(index,loaded);
    child_require_current(worldview_context_callbacks[index].child);
    worldview_callback_fence(frame);worldview_context_callback_identity(index,loaded);
}
static bool worldview_context_registration_refs(lua_State* L,std::size_t index){
    auto& self=owner(L);const auto& e=self.worldview_context_callbacks[index];
    if(!lua_checkstack(L,8))return false;
    const int top=lua_gettop(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,e.thread);
    const bool thread=lua_isthread(L,-1)&&lua_tothread(L,-1)==L;lua_pop(L,1);
    lua_rawgeti(L,LUA_REGISTRYINDEX,e.environment);lua_rawgeti(L,LUA_REGISTRYINDEX,e.graph_environment);
    const bool environment=lua_istable(L,-1)&&lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==e.environment_identity;lua_pop(L,1);
    lua_pushliteral(L,"ContextPtr");lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,e.context);
    const bool context=lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==e.context_identity;lua_pop(L,3);
    lua_rawgeti(L,LUA_REGISTRYINDEX,e.function);
    const bool closure=lua_isfunction(L,-1)&&!lua_iscfunction(L,-1)&&lua_topointer(L,-1)==e.closure&&
        self.graph_compiled_owner(L,-1)==e.compiled;
    lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,e.environment);
    const bool fenv=lua_rawequal(L,-1,-2);lua_settop(L,top);
    return thread&&environment&&context&&closure&&fenv;
}

static void worldview_register_owned_context_callback(lua_State* L,std::size_t slot,bool shutdown,int function){
    auto& self=owner(L);
    if(!self.worldview_control_entry||self.worldview_control_entry!=L||
       slot>=self.control_graphs.size()||!self.control_graphs[slot].graph||
       !self.control_graphs[slot].graph->backend_||!self.control_graphs[slot].bound||
       lua_gettop(L)!=2||function!=2||!lua_isfunction(L,function)||lua_iscfunction(L,function)||
       !lua_istable(L,1)||!lua_rawequal(L,1,lua_upvalueindex(3))||
       self.worldview_context_registering||self.worldview_context_dispatching)
        return (void)luaL_error(L,"WorldView context handler receiver/closure/lender refused");
    std::size_t child=0;
    for(;child<self.child_contexts.size();++child)
        if(self.child_contexts[child].name==self.control_graphs[slot].graph->context())break;
    if(child==self.child_contexts.size()||self.child_contexts[child].retired||
       !self.child_contexts[child].source||self.child_contexts[child].name!=(shutdown?"UnitFlagManager":"WorldView")||
       self.child_contexts[child].environment==LUA_NOREF||self.control_graphs[slot].environment==LUA_NOREF)
        return (void)luaL_error(L,"WorldView context handler exact authored child absent");
    lua_Debug caller{};std::size_t compiled=graph_no_owner;bool caller_environment=false;
    for(int level=1;level<=256&&lua_getstack(L,level,&caller);++level){
        if(!lua_getinfo(L,"f",&caller))break;
        if(lua_iscfunction(L,-1)){lua_pop(L,1);continue;}
        compiled=self.graph_compiled_owner(L,-1);
        lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[child].environment);
        caller_environment=lua_rawequal(L,-1,-2);lua_pop(L,3);break;
    }
    if(compiled==graph_no_owner||compiled!=self.graph_execution_scope||!caller_environment||
       self.child_contexts[child].execution_source!=compiled||self.graph_compiled_owner(L,function)!=compiled)
        return (void)luaL_error(L,"WorldView context handler first compiled caller differs");
    lua_getfenv(L,function);lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[child].environment);
    const bool same_environment=lua_rawequal(L,-1,-2);const auto environment=lua_topointer(L,-1);lua_pop(L,2);
    if(!same_environment)return (void)luaL_error(L,"WorldView context handler original environment rebound");
    lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[child].environment);
    lua_rawgeti(L,LUA_REGISTRYINDEX,self.control_graphs[slot].environment);
    const bool same_graph_environment=lua_rawequal(L,-1,-2);lua_pop(L,2);
    if(!same_graph_environment)return (void)luaL_error(L,"WorldView graph/child environment tables differ");
    lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[child].environment);
    lua_pushliteral(L,"ContextPtr");lua_rawget(L,-2);
    const bool same_context=lua_rawequal(L,1,-1);lua_pop(L,2);
    if(!same_context)return (void)luaL_error(L,"WorldView context handler retained ContextPtr differs");
    for(const auto& old:self.worldview_context_callbacks)
        if(old.child==child&&old.shutdown==shutdown)
            return (void)luaL_error(L,"WorldView selected static context handler cannot be replaced/replayed");
    if(self.worldview_context_callbacks.size()>=2)
        return (void)luaL_error(L,"WorldView selected context handler budget exhausted");
    const auto index=self.worldview_context_callbacks.size();self.worldview_context_registering=true;
    self.source_exception={};self.source_error=nullptr;
    try{
        self.worldview_context_callbacks.emplace_back();auto& e=self.worldview_context_callbacks.back();
        e.vm_owner=self.worldview_self;e.lifetime=self.worldview_lifetime;e.host=self.worldview_host_owner;
        e.source=self.child_contexts[child].source;e.graph=self.control_graphs[slot].graph;e.graph_slot=slot;
        e.child=child;e.compiled=compiled;e.root=self.graph_compiled_sources[compiled].root;
        e.environment=self.child_contexts[child].environment;e.graph_environment=self.control_graphs[slot].environment;
        e.environment_identity=environment;
        e.context_identity=lua_topointer(L,1);e.closure=lua_topointer(L,function);e.registered=L;
        e.epoch=self.failure_epoch;e.uncertainty=self.execution_uncertainty;e.shutdown=shutdown;
        const auto frame=self.worldview_callback_frame();self.worldview_context_callback_owner(index,frame,false);
    }catch(...){self.source_exception=std::current_exception();self.source_error="WorldView context handler source preparation refused";}
    if(self.source_error){self.record_failure();return (void)luaL_error(L,"%s",self.source_error);}
    // No owning automatic variables surround any allocating Lua registry API.
    lua_pushvalue(L,function);self.worldview_context_callbacks[index].function=luaL_ref(L,LUA_REGISTRYINDEX);
    lua_pushthread(L);self.worldview_context_callbacks[index].thread=luaL_ref(L,LUA_REGISTRYINDEX);
    lua_pushvalue(L,1);self.worldview_context_callbacks[index].context=luaL_ref(L,LUA_REGISTRYINDEX);
    lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[child].environment);
    lua_pushliteral(L,"ContextPtr");lua_rawget(L,-2);
    const bool context_post=lua_rawequal(L,1,-1);lua_pop(L,2);
    if(!context_post){self.record_failure();return (void)luaL_error(L,"WorldView context handler ContextPtr changed during registration");}
    self.source_exception={};self.source_error=nullptr;
    try{
        const auto frame=self.worldview_callback_frame();self.worldview_context_callback_owner(index,frame,false);
        if(!worldview_context_registration_refs(L,index))throw ScriptError("WorldView context handler real registry postflight changed");
        self.worldview_context_callbacks[index].published=true;self.worldview_context_registering=false;
    }catch(...){self.worldview_context_callbacks[index].retired=true;self.source_exception=std::current_exception();
        self.source_error="WorldView context handler registration postflight refused";}
    if(self.source_error){self.record_failure();return (void)luaL_error(L,"%s",self.source_error);}
}

struct WorldViewContextCallbackJob {
    Impl* owner;std::size_t index;std::int64_t message,wparam,lparam;
    bool input=false,invoked=false,consumed=false,returned=false;
};
static int worldview_context_callback_protected(lua_State* L){
    auto& job=*static_cast<WorldViewContextCallbackJob*>(lua_touserdata(L,1));auto& self=*job.owner;
    if(&owner(L)!=&self||job.index>=self.worldview_context_callbacks.size())
        return luaL_error(L,"WorldView context handler actual VM differs");
    const auto& e=self.worldview_context_callbacks[job.index];
    if(e.retired||!e.published||e.shutdown_returned||e.registered!=L||e.shutdown==job.input||
       e.function==LUA_NOREF||e.thread==LUA_NOREF||e.context==LUA_NOREF||e.environment==LUA_NOREF||e.graph_environment==LUA_NOREF||
       e.child>=self.child_contexts.size()||e.compiled>=self.graph_compiled_sources.size())
        return luaL_error(L,"WorldView context handler actual registered state/ref retired");
    if(!lua_checkstack(L,12))return luaL_error(L,"WorldView context handler protected stack exhausted");
    lua_rawgeti(L,LUA_REGISTRYINDEX,e.thread);
    const bool thread=lua_isthread(L,-1)&&lua_tothread(L,-1)==L;lua_pop(L,1);
    if(!thread||lua_status(L))return luaL_error(L,"WorldView context handler registered coroutine unavailable");
    child_require_bindings(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,e.environment);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=e.environment_identity)
        return luaL_error(L,"WorldView context handler environment identity changed");
    lua_rawgeti(L,LUA_REGISTRYINDEX,e.graph_environment);
    const bool graph_environment=lua_rawequal(L,-1,-2);lua_pop(L,1);
    if(!graph_environment)return luaL_error(L,"WorldView context handler graph/child environment tables changed");
    lua_pushliteral(L,"ContextPtr");lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,e.context);
    const bool context=lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==e.context_identity;lua_pop(L,3);
    if(!context)return luaL_error(L,"WorldView context handler canonical receiver rebound");
    lua_rawgeti(L,LUA_REGISTRYINDEX,e.function);
    if(!lua_isfunction(L,-1)||lua_iscfunction(L,-1)||lua_topointer(L,-1)!=e.closure||
       self.graph_compiled_owner(L,-1)!=e.compiled||self.graph_compiled_sources[e.compiled].root!=e.root)
        return luaL_error(L,"WorldView context handler actual closure/Proto changed");
    lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,e.environment);
    const bool environment=lua_rawequal(L,-1,-2);lua_pop(L,2);
    if(!environment)return luaL_error(L,"WorldView context handler closure fenv changed");
    if(!self.child_contexts[e.child].loaded||self.child_contexts[e.child].loading)
        return luaL_error(L,"WorldView context handler child did not finish execution");
    self.graph_execution_scope=e.compiled;job.invoked=true;
    if(job.input){
        lua_pushnumber(L,static_cast<lua_Number>(job.message));
        lua_pushnumber(L,static_cast<lua_Number>(job.wparam));
        lua_pushnumber(L,static_cast<lua_Number>(job.lparam));
        lua_call(L,3,1);
        if(!lua_isnil(L,-1)&&!lua_isboolean(L,-1))return luaL_error(L,"WorldView input handler must return boolean or nil");
        job.consumed=lua_toboolean(L,-1)!=0;lua_pop(L,1);
    }else lua_call(L,0,0); // Original OnShutdown rehomes the selected foreign Anchor.
    job.returned=true;return 0;
}
std::size_t worldview_context_callback_for_child(std::size_t child,bool shutdown){
    if(ui_thread!=std::this_thread::get_id())throw ScriptError("WorldView context dispatch foreign creator");
    worldview_require_callback_origin();
    if(calling||worldview_context_registering||
       worldview_context_dispatching||worldview_callback_dispatching||worldview_control_entry||worldview_control_validation){
        record_failure();throw ScriptError("WorldView context callback native dispatch reentry");
    }
    if(child>=child_contexts.size()||!worldview_native_child_controls_current(child))
        throw ScriptError("WorldView context callback native child issuer differs");
    for(std::size_t i=0;i<worldview_context_callbacks.size();++i)
        if(worldview_context_callbacks[i].child==child&&worldview_context_callbacks[i].shutdown==shutdown)return i;
    throw ScriptError("WorldView context handler actual registration absent");
}
bool worldview_dispatch_owned_context_callback(std::size_t child,bool shutdown,
                                               std::int64_t message,std::int64_t wparam,std::int64_t lparam){
    const auto index=worldview_context_callback_for_child(child,shutdown);
    // Actual native message integers must survive Lua Number conversion exactly.
    constexpr std::int64_t exact=9007199254740991LL;
    if(!shutdown&&(message<-exact||message>exact||wparam<-exact||wparam>exact||lparam<-exact||lparam>exact))
        throw ScriptError("WorldView native input operand exceeds exact Lua numeric range");
    worldview_context_dispatching=true;worldview_callback_dispatching=true;
    struct Dispatch {Impl& p;~Dispatch(){p.worldview_context_dispatching=false;p.worldview_callback_dispatching=false;}} dispatch{*this};
    auto* L=worldview_context_callbacks[index].registered;
    if(!L)throw ScriptError("WorldView context handler registered state absent");
    const int top=lua_gettop(L);const auto previous=graph_execution_scope;
    const auto previous_hook=lua_gethook(L);const int mask=lua_gethookmask(L),count=lua_gethookcount(L);
    const auto previous_remaining=remaining;const bool previous_exhausted=exhausted;
    struct Restore {Impl& p;lua_State* L;int top;std::size_t graph;lua_Hook hook;int mask,count;
        std::uint64_t instructions;bool exhausted;
        ~Restore(){lua_sethook(L,hook,mask,count);lua_settop(L,top);p.graph_execution_scope=graph;
            p.remaining=instructions;p.exhausted=exhausted;p.calling=false;}
    } restore{*this,L,top,previous,previous_hook,mask,count,previous_remaining,previous_exhausted};
    try{
        const auto frame=worldview_callback_frame();
        worldview_context_callback_owner(index,frame,true);
        WorldViewContextCallbackJob job{this,index,message,wparam,lparam,!shutdown};
        calling=true;remaining=budget.instructions;exhausted=false;
        lua_sethook(L,budget.instructions?hook:nullptr,budget.instructions?LUA_MASKCOUNT:0,1000);
        int status=0;
        {WorldViewLoadedChildCallScope loaded_call(*this,WorldViewLoadedChildCallScope::Kind::context,index);
         status=lua_cpcall(L,worldview_context_callback_protected,&job);
         if(!status&&!exhausted)loaded_call.accept(job.invoked,job.returned);
        }
        calling=false;lua_sethook(L,previous_hook,mask,count);graph_execution_scope=previous;
        if(status){const auto error=worldview_callback_lua_error(L);lua_settop(L,top);throw ScriptError(error);}
        if(exhausted||!job.invoked||!job.returned)throw ScriptError("WorldView context handler protected dispatch did not return");
        lua_settop(L,top);worldview_context_callback_owner(index,frame,true);
        if(shutdown){worldview_context_callbacks[index].shutdown_returned=true;worldview_context_callbacks[index].retired=true;}
        return job.consumed;
    }catch(...){record_failure();if(end_turn_lifetime)end_turn_lifetime->failed=true;
        worldview_retire_context_callback_registrations();throw;}
}
bool worldview_dispatch_owned_context_input(std::size_t child,std::int64_t message,std::int64_t wparam,std::int64_t lparam){
    if(ui_thread!=std::this_thread::get_id())throw ScriptError("WorldView input dispatch foreign creator");
    try{return worldview_dispatch_owned_context_callback(child,false,message,wparam,lparam);}
    catch(...){record_failure();if(end_turn_lifetime)end_turn_lifetime->failed=true;
        worldview_retire_context_callback_registrations();throw;}
}
void worldview_dispatch_owned_context_shutdown(std::size_t child){
    if(ui_thread!=std::this_thread::get_id())throw ScriptError("WorldView shutdown dispatch foreign creator");
    try{(void)worldview_dispatch_owned_context_callback(child,true,0,0,0);}
    catch(...){record_failure();if(end_turn_lifetime)end_turn_lifetime->failed=true;
        worldview_retire_context_callback_registrations();throw;}
}
void worldview_retire_context_callback_registrations()noexcept{
    if(ui_thread!=std::this_thread::get_id())std::terminate();
    for(auto& e:worldview_context_callbacks)e.retired=true;
    worldview_context_registering=false;
}
static int worldview_release_retired_context_callbacks(lua_State* L){
    auto& self=owner(L);
    if(self.ui_thread!=std::this_thread::get_id())return luaL_error(L,"WorldView context cleanup foreign creator");
    for(auto& e:self.worldview_context_callbacks)if(e.retired){
        const int function=e.function,thread=e.thread,context=e.context;
        e.function=e.thread=e.context=LUA_NOREF;
        if(function!=LUA_NOREF)luaL_unref(L,LUA_REGISTRYINDEX,function);
        if(thread!=LUA_NOREF)luaL_unref(L,LUA_REGISTRYINDEX,thread);
        if(context!=LUA_NOREF)luaL_unref(L,LUA_REGISTRYINDEX,context);
    }
    return 0;
}
