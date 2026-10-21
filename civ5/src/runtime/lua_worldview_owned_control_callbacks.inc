// Included inside LuaScriptHost::Impl after Graph/Child/EndTurn definitions.
// Registry storage and scratch belong to Impl across every allocating Lua API.
// NativeHostScope supplies worldview_operation/host_owner/self/lifetime and the
// PURE worldview_require_callback_origin() healthy-origin/sentinel proof.
struct WorldViewCallbackEntry {
    std::shared_ptr<const ScriptUiChildSource> source;
    std::weak_ptr<ScriptUiControlGraph> graph;
    std::size_t graph_slot=graph_no_owner,child=graph_no_owner,compiled=graph_no_owner;
    const Proto* root=nullptr;const void* closure=nullptr;const void* environment_identity=nullptr;
    int function=LUA_NOREF,thread=LUA_NOREF,environment=LUA_NOREF;
    lua_State* registered=nullptr;
    std::uint64_t epoch=0,uncertainty=0;bool retired=false;
};
std::deque<WorldViewCallbackEntry> worldview_callbacks;
std::shared_ptr<const ScriptUiOwnedControlCallback> worldview_callback_scratch;
bool worldview_callback_dispatching=false;
struct WorldViewCallbackFrame {
    const BnwWorldViewSourceOperationLoan* loan;std::uint64_t serial,epoch,uncertainty;
};
WorldViewCallbackFrame worldview_callback_frame(){
    worldview_require_callback_origin(); // Native scope authenticates pointer FIRST.
    if(!worldview_operation||!worldview_host_owner)throw ScriptError("WorldView callback native operation absent");
    const auto* loan=worldview_operation;const auto serial=loan->operation_serial();
    worldview_require_callback_origin();
    if(worldview_operation!=loan)throw ScriptError("WorldView callback source operation replaced");
    loan->require_operation(serial);
    return {loan,serial,failure_epoch,execution_uncertainty};
}
void worldview_callback_fence(const WorldViewCallbackFrame& f){
    worldview_require_callback_origin(); // Never dereference an expired prior stack address.
    if(worldview_operation!=f.loan||failure_epoch!=f.epoch||execution_uncertainty!=f.uncertainty)
        throw ScriptError("WorldView callback operation/failure origin changed");
    worldview_operation->require_operation(f.serial);
    worldview_require_callback_origin();
    if(worldview_operation!=f.loan||failure_epoch!=f.epoch||execution_uncertainty!=f.uncertainty)
        throw ScriptError("WorldView callback provider changed operation/failure origin");
}
void worldview_callback_owner(std::size_t index,const WorldViewCallbackFrame& f){
    worldview_callback_fence(f);
    if(index>=worldview_callbacks.size())throw ScriptError("WorldView callback registration retired");
    const auto& e=worldview_callbacks[index];
    if(e.retired||e.epoch!=failure_epoch||e.uncertainty!=execution_uncertainty||
       e.graph_slot>=control_graphs.size()||e.child>=child_contexts.size()||e.compiled>=graph_compiled_sources.size())
        throw ScriptError("WorldView callback immutable source occurrence changed");
    const auto identity=[&]{
    if(e.retired||e.epoch!=failure_epoch||e.uncertainty!=execution_uncertainty||
       e.graph_slot>=control_graphs.size()||e.child>=child_contexts.size()||e.compiled>=graph_compiled_sources.size())
        throw ScriptError("WorldView callback source occurrence changed during provider");
    const auto graph=e.graph.lock();const auto& slot=control_graphs[e.graph_slot];const auto& child=child_contexts[e.child];
    if(!graph||graph.get()!=slot.graph.get()||graph.owner_before(slot.graph)||slot.graph.owner_before(graph)||
       !graph->backend_||!slot.bound||child.retired||!child.source||
       e.source.get()!=child.source.get()||e.source.owner_before(child.source)||child.source.owner_before(e.source)||
       child.environment!=e.environment||child.execution_source!=e.compiled||
       graph_compiled_sources[e.compiled].root!=e.root||graph_compiled_sources[e.compiled].retained==LUA_NOREF||
       graph_compiled_sources[e.compiled].context!=child.name||graph->context()!=child.name||
       (child.name!="ActionInfoPanel"&&child.name!="UnitFlagManager"))
        throw ScriptError("WorldView callback SAME canonical graph/child/Proto identity lost");
    };
    identity();worldview_operation->require_same_vm(*worldview_host_owner);worldview_callback_fence(f);identity();
    worldview_operation->require_same_source(*e.source);worldview_callback_fence(f);identity();
    e.source->require_current();worldview_callback_fence(f);identity();
    child_require_current(e.child);worldview_callback_fence(f);identity();
}
struct WorldViewCallbackJob {
    Impl* owner;std::size_t index;bool invoke=false,invoked=false;
    int nargs=0;std::int64_t void1=0,void2=0;
};
static int worldview_callback_protected(lua_State* L){
    auto& job=*static_cast<WorldViewCallbackJob*>(lua_touserdata(L,1));auto& self=*job.owner;
    if(&owner(L)!=&self||job.index>=self.worldview_callbacks.size())return luaL_error(L,"WorldView callback registered VM differs");
    const auto& e=self.worldview_callbacks[job.index];
    if(e.retired||e.registered!=L||e.function==LUA_NOREF||e.thread==LUA_NOREF||e.environment==LUA_NOREF||
       e.compiled>=self.graph_compiled_sources.size()||e.child>=self.child_contexts.size())
        return luaL_error(L,"WorldView callback registered state/refs retired");
    if(!lua_checkstack(L,8))return luaL_error(L,"WorldView callback protected stack unavailable");
    lua_rawgeti(L,LUA_REGISTRYINDEX,e.thread);
    const bool thread=lua_isthread(L,-1)&&lua_tothread(L,-1)==L;lua_pop(L,1);
    if(!thread||lua_status(L))return luaL_error(L,"WorldView callback actual registered coroutine unavailable");
    child_require_bindings(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,e.function);
    if(!lua_isfunction(L,-1)||lua_iscfunction(L,-1)||lua_topointer(L,-1)!=e.closure||
       self.graph_compiled_owner(L,-1)!=e.compiled||self.graph_compiled_sources[e.compiled].root!=e.root)
        return luaL_error(L,"WorldView registered callback closure/Proto changed");
    lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,e.environment);
    const bool environment=lua_rawequal(L,-1,-2)&&lua_topointer(L,-1)==e.environment_identity;lua_pop(L,2);
    if(!environment)return luaL_error(L,"WorldView callback actual child environment rebound");
    if(job.invoke){
        if(!self.child_contexts[e.child].loaded||self.child_contexts[e.child].loading)
            return luaL_error(L,"WorldView callback invoked before actual selected child returned");
        self.graph_execution_scope=e.compiled;job.invoked=true;
        if(job.nargs==2){lua_pushinteger(L,static_cast<lua_Integer>(job.void1));lua_pushinteger(L,static_cast<lua_Integer>(job.void2));}
        else if(job.nargs!=0)return luaL_error(L,"WorldView callback actual argument count differs");
        lua_call(L,job.nargs,0); // SAME protected actual closure with canonical Void operands
    }else lua_pop(L,1);
    return 0;
}
std::string worldview_callback_lua_error(lua_State* L){
    // lua_tolstring on an existing STRING performs no number conversion/allocation.
    if(lua_type(L,-1)==LUA_TSTRING){std::size_t n=0;const char* s=lua_tolstring(L,-1,&n);return std::string(s,n);}
    return "WorldView callback failed with a non-string Lua error";
}
int worldview_callback_run(WorldViewCallbackJob& job){
    auto* L=worldview_callbacks[job.index].registered;
    const int top=lua_gettop(L);const auto previous=graph_execution_scope;
    const auto prior_hook=lua_gethook(L);const int mask=lua_gethookmask(L),count=lua_gethookcount(L);
    if(budget.instructions)lua_sethook(L,hook,LUA_MASKCOUNT,1000);
    const int status=lua_cpcall(L,worldview_callback_protected,&job);
    lua_sethook(L,prior_hook,mask,count);graph_execution_scope=previous;
    if(!status)lua_settop(L,top); // Error remains until copied by caller outside Lua boundary.
    return status;
}
class WorldViewOwnedCallback final:public ScriptUiOwnedControlCallback {
    const std::thread::id creator_=std::this_thread::get_id();
    const std::weak_ptr<Impl> impl_;
    const std::weak_ptr<EndTurnQueue::VmLifetime> lifetime_;
    const std::shared_ptr<const ScriptUiChildSource> source_;
    const LuaScriptHost* const host_;const std::size_t index_;
    std::shared_ptr<Impl> borrow()const {
        if(creator_!=std::this_thread::get_id())throw ScriptError("WorldView callback foreign creator thread");
        const auto life=lifetime_.lock();const auto p=impl_.lock();
        if(!p||!life||!life->alive.load()||life->failed.load()||life->creator!=creator_||
           life->host!=host_||life->vm!=p->state||p->worldview_host_owner!=host_||
           !p->end_turn_lifetime||p->end_turn_lifetime.get()!=life.get()||
           p->end_turn_lifetime.owner_before(lifetime_)||lifetime_.owner_before(p->end_turn_lifetime)||
           p->worldview_lifetime.owner_before(lifetime_)||lifetime_.owner_before(p->worldview_lifetime)||
           p->worldview_self.owner_before(impl_)||impl_.owner_before(p->worldview_self))
            throw ScriptError("WorldView callback actual VM/facade lifetime retired");
        p->worldview_require_callback_origin();return p;
    }
 public:
    WorldViewOwnedCallback(Impl& p,std::size_t n):impl_(p.worldview_self),lifetime_(p.worldview_lifetime),
        source_(p.worldview_callbacks[n].source),host_(p.worldview_host_owner),index_(n){}
    ~WorldViewOwnedCallback()override {
        if(creator_!=std::this_thread::get_id())std::terminate();
        if(const auto p=impl_.lock())if(index_<p->worldview_callbacks.size())p->worldview_callbacks[index_].retired=true;
        // No Lua API, source provider or fabricated renewed operation in destructor.
    }
    const LuaScriptHost& host()const override {const auto p=borrow();return *host_;}
    const ScriptUiChildSource& source()const override {const auto p=borrow();return *source_;}
    void require_current()const override {
        const auto p=borrow();const auto f=p->worldview_callback_frame();p->worldview_callback_owner(index_,f);
        WorldViewCallbackJob job{p.get(),index_};auto* L=p->worldview_callbacks[index_].registered;const int top=lua_gettop(L);
        struct Stack{lua_State* L;int top;~Stack(){lua_settop(L,top);}} stack{L,top};
        const int status=p->worldview_callback_run(job);
        if(status){const auto message=p->worldview_callback_lua_error(L);lua_settop(L,top);p->record_failure();throw ScriptError(message);}
        p->worldview_callback_owner(index_,f);
    }
    ScriptUiEventResult invoke(std::span<const ScriptValue> args)const override {
        const auto p=borrow();const auto f=p->worldview_callback_frame();
        const bool unitflag=source_->context()=="UnitFlagManager";
        if(p->worldview_callback_dispatching||(!unitflag&&!args.empty())||(unitflag&&args.size()!=2))
            throw ScriptError("WorldView callback args/nested dispatch refused");
        std::int64_t void1=0,void2=0;
        if(unitflag){
            const auto* a=std::get_if<std::int64_t>(&args[0]);const auto* b=std::get_if<std::int64_t>(&args[1]);
            if(!a||!b||*a<0||*a>=64||*b<0||*b>INT32_MAX)throw ScriptError("UnitFlag callback actual native identity differs");
            void1=*a;void2=*b;
            host_->require_unitflag_button_unit_owned(*f.loan,static_cast<std::int32_t>(void1),static_cast<std::int32_t>(void2));
            p->worldview_callback_fence(f);
        }
        p->worldview_callback_owner(index_,f);p->worldview_callback_dispatching=true;
        struct Exit{Impl& p;~Exit(){p.worldview_callback_dispatching=false;}} exit{*p};
        ScriptUiEventResult result;WorldViewCallbackJob job{p.get(),index_,true};
        if(unitflag){job.nargs=2;job.void1=void1;job.void2=void2;}
        auto* L=p->worldview_callbacks[index_].registered;const int top=lua_gettop(L);
        struct Stack{lua_State* L;int top;~Stack(){lua_settop(L,top);}} stack{L,top};
        try {
            WorldViewLoadedChildCallScope loaded_call(*p,WorldViewLoadedChildCallScope::Kind::control,index_);
            const int status=p->worldview_callback_run(job);result.listeners=job.invoked?1:0;
            if(status){result.errors.push_back(p->worldview_callback_lua_error(L));lua_settop(L,top);p->record_failure();
                if(const auto life=lifetime_.lock())life->failed.store(true);return result;}
            p->worldview_callback_owner(index_,f);loaded_call.accept(job.invoked,true);return result;
        }catch(...){p->record_failure();if(const auto life=lifetime_.lock())life->failed.store(true);throw;}
    }
};
static void worldview_register_owned_callback(lua_State* L,std::size_t slot,std::uint64_t node,
                                               std::int64_t event,int function){
    auto& self=owner(L);self.worldview_require_callback_origin();
    if(slot>=self.control_graphs.size()||!self.control_graphs[slot].graph||!self.control_graphs[slot].graph->backend_||
       !self.control_graphs[slot].bound||function<1||function>lua_gettop(L)||
       !lua_isfunction(L,function)||lua_iscfunction(L,function)||graph_checked(L,1,slot)->id!=node)
        luaL_error(L,"WorldView callback receiver/owned backend/closure refused");
    std::size_t child=0;
    for(;child<self.child_contexts.size();++child)if(self.child_contexts[child].name==self.control_graphs[slot].graph->context())break;
    if(child==self.child_contexts.size()||!self.child_contexts[child].source||self.child_contexts[child].retired||
       (self.child_contexts[child].name!="ActionInfoPanel"&&self.child_contexts[child].name!="UnitFlagManager"))
        luaL_error(L,"WorldView callback lacks exact selected child/graph environment");
    // Graph and Child independently retain the SAME environment table. Their
    // live luaL_ref registry keys are distinct, not an environment identity.
    const int child_environment=self.child_contexts[child].environment;
    const int graph_environment=self.control_graphs[slot].environment;
    if(child_environment<=0||graph_environment<=0||!lua_checkstack(L,2))
        luaL_error(L,"WorldView callback lacks exact selected child/graph environment");
    lua_rawgeti(L,LUA_REGISTRYINDEX,child_environment);
    lua_rawgeti(L,LUA_REGISTRYINDEX,graph_environment);
    const bool same_environment=lua_istable(L,-2)&&lua_istable(L,-1)&&lua_rawequal(L,-2,-1);
    lua_pop(L,2);
    if(!same_environment)luaL_error(L,"WorldView callback lacks exact selected child/graph environment");
    lua_Debug caller{};std::size_t compiled=graph_no_owner;bool caller_environment=false;
    // Native protected wrappers add C frames. Authenticate the FIRST genuine
    // Lua frame; never search past a foreign Lua caller for a matching ancestor.
    for(int level=1;level<=256&&lua_getstack(L,level,&caller);++level){
        if(!lua_getinfo(L,"f",&caller))break;
        if(lua_iscfunction(L,-1)){lua_pop(L,1);continue;}
        compiled=self.graph_compiled_owner(L,-1);
        lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[child].environment);
        caller_environment=lua_rawequal(L,-1,-2);lua_pop(L,3);break;
    }
    if(compiled==graph_no_owner||compiled!=self.graph_execution_scope||!caller_environment||
       self.child_contexts[child].execution_source!=compiled||self.graph_compiled_owner(L,function)!=compiled)
        luaL_error(L,"WorldView callback differs from actual compiled caller/child scope");
    lua_getfenv(L,function);lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[child].environment);
    const bool same=lua_rawequal(L,-1,-2);const auto environment=lua_topointer(L,-1);lua_pop(L,2);
    if(!same)luaL_error(L,"WorldView callback fenv is not the actual retained selected child");
    if(self.worldview_callbacks.size()>=65536)luaL_error(L,"WorldView callback registration budget exhausted");
    const auto index=self.worldview_callbacks.size();self.source_exception={};self.source_error=nullptr;
    try {
        self.worldview_callbacks.emplace_back();auto& e=self.worldview_callbacks.back();
        e.source=self.child_contexts[child].source;e.graph=self.control_graphs[slot].graph;e.graph_slot=slot;
        e.child=child;e.compiled=compiled;e.root=self.graph_compiled_sources[compiled].root;
        e.environment=self.child_contexts[child].environment;e.environment_identity=environment;
        e.closure=lua_topointer(L,function);e.registered=L;e.epoch=self.failure_epoch;e.uncertainty=self.execution_uncertainty;
        const auto frame=self.worldview_callback_frame();self.worldview_callback_owner(index,frame);
    }catch(const std::exception& error){self.source_exception=std::current_exception();self.source_error=error.what();}
    catch(...){self.source_exception=std::current_exception();self.source_error="WorldView callback producer source origin refused";}
    if(self.source_error)luaL_error(L,"%s",self.source_error);
    // POD-only locals; entry is already in Impl if either registry allocation longjmps.
    lua_pushvalue(L,function);self.worldview_callbacks[index].function=luaL_ref(L,LUA_REGISTRYINDEX);
    lua_pushthread(L);self.worldview_callbacks[index].thread=luaL_ref(L,LUA_REGISTRYINDEX);
    self.source_exception={};self.source_error=nullptr;
    try {
        self.worldview_callback_scratch=std::make_shared<WorldViewOwnedCallback>(self,index);
        self.control_graphs[slot].graph->register_owned_callback(node,event,self.worldview_callback_scratch);
        self.worldview_callback_scratch.reset();
    }catch(const std::exception& error){self.worldview_callback_scratch.reset();self.worldview_callbacks[index].retired=true;
        self.source_exception=std::current_exception();self.source_error=error.what();}
    catch(...){self.worldview_callback_scratch.reset();self.worldview_callbacks[index].retired=true;
        self.source_exception=std::current_exception();self.source_error="WorldView callback graph registration failed";}
    if(self.source_error)luaL_error(L,"%s",self.source_error);
}
static int worldview_release_retired_callbacks(lua_State* L){
    auto& self=owner(L);
    if(self.ui_thread!=std::this_thread::get_id())return luaL_error(L,"WorldView callback cleanup foreign creator thread");
    for(auto& e:self.worldview_callbacks)if(e.retired){
        const int fn=e.function,thread=e.thread;e.function=e.thread=LUA_NOREF;
        if(fn!=LUA_NOREF)luaL_unref(L,LUA_REGISTRYINDEX,fn);
        if(thread!=LUA_NOREF)luaL_unref(L,LUA_REGISTRYINDEX,thread);
    }
    return 0;
}
void worldview_retire_callback_registrations()noexcept {
    if(ui_thread!=std::this_thread::get_id())std::terminate();
    for(auto& e:worldview_callbacks)e.retired=true;
    worldview_callback_scratch.reset();
}
