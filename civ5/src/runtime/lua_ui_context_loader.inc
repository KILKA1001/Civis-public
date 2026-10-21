// Impl-owned state survives Lua OOM/error longjmp. Lua closures carry only a
// numeric slot and the exact receiver; no stack-local C++ owner crosses LuaAPI.
#include "lua_context_loader_temporary_types.inc"
struct ChildContext {
    std::unique_ptr<ContextLoaderTemporaries> loader_temporaries;
    std::string name,chunk;
    std::shared_ptr<const ScriptUiChildSource> source;
    const assets::AssetHandle *asset=nullptr;
    assets::Bytes bytes;
    int environment=LUA_NOREF,receiver=LUA_NOREF;
    int execution_thread=LUA_NOREF; // actual load thread retained for named-context queries
    std::size_t execution_source=graph_no_owner; // Host-retained compiled load source, not a name proof
    int update_function=LUA_NOREF;
    bool loading=false,loaded=false,retired=false;
    bool state_prepared=false; // earned only by actual protected coroutine constructor
#include "lua_native_context_setup_state_name_fields.inc"
    bool state_registered=false,state_gc_returned=false; // real manager append/GC effects only
};
std::shared_ptr<ScriptUiContextSourceOwner> context_sources;
std::deque<ChildContext> child_contexts;
std::size_t child_load_depth=0;
std::deque<bool> child_attempt_succeeded;
int child_binding_key=LUA_NOREF;
std::shared_ptr<ScriptUiUpdateSourceOwner> update_sources;
std::unique_ptr<const ScriptUiUpdateFrame> update_frame;
std::optional<std::uint64_t> update_last_sequence;
std::uint64_t update_sequence=0,update_epoch=0;
double update_delta=0;
bool update_registering=false;
void update_require_context_current(std::size_t slot) {
    child_require_current(slot);
    for(auto &entry:control_graphs)if(entry.graph->context()==child_contexts[slot].name) {
        entry.touched=true;entry.graph->require_current();
    }
    if(child_contexts[slot].name==ui_context.context_id&&!ui_context.context_id.empty())require_ui_callback_owner();
}
void update_require_current(bool frame) {
    if(!update_sources||!context_sources)throw ScriptError("Update owner/source channel unavailable");
    const auto epoch=failure_epoch;
    update_sources->require_current(*context_sources);
    if(frame) {
        if(update_registering)throw ScriptError("Update registration had a terminal failure");
        if(!update_frame||&update_frame->issuer()!=update_sources.get())throw ScriptError("Foreign update frame issuer");
        update_frame->require_current(*context_sources);
        if(update_frame->sequence()!=update_sequence||update_frame->delta_seconds()!=update_delta)
            throw ScriptError("Update frame observation rebound");
    }
    if(epoch!=failure_epoch||(frame&&update_epoch!=failure_epoch))throw ScriptError("Update owner caught host reentry");
}
static int set_context_update(lua_State *L) {
    auto &self=owner(L);const bool ready=self.ready,failed=self.failed;const auto epoch=self.failure_epoch;
    self.ready=false;self.failed=true;
    if(self.update_registering){self.record_failure();return luaL_error(L,"Update registration is reentrant or failed");}
    self.update_registering=true;
    const auto slot=static_cast<std::size_t>(lua_tointeger(L,lua_upvalueindex(1)));
    if(lua_gettop(L)!=2||!lua_rawequal(L,1,lua_upvalueindex(2)))return luaL_error(L,"SetUpdate receiver/arguments differ");
    if(!lua_isnil(L,2)&&!lua_isfunction(L,2))return luaL_error(L,"SetUpdate expects a function or generic nil unregister");
    child_require_bindings(L);
    lua_Debug caller{};
    if(!lua_getstack(L,1,&caller)||!lua_getinfo(L,"f",&caller))return luaL_error(L,"SetUpdate has no Lua caller");
    const auto compiled=self.graph_compiled_owner(L,-1);lua_pop(L,1);
    if(slot>=self.child_contexts.size()||compiled==graph_no_owner||self.graph_execution_scope==graph_no_owner||
       self.graph_compiled_sources[compiled].context!=self.child_contexts[slot].name||
       self.graph_compiled_sources[self.graph_execution_scope].context!=self.child_contexts[slot].name)
        return luaL_error(L,"SetUpdate has no same compiled context scope");
    if(!lua_isnil(L,2)) {
        const auto callback=self.graph_compiled_owner(L,2);
        if(callback==graph_no_owner||self.graph_compiled_sources[callback].context!=self.child_contexts[slot].name)
            return luaL_error(L,"SetUpdate callback is not owned by this compiled context");
        lua_getfenv(L,2);lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[slot].environment);
        const bool environment=lua_rawequal(L,-1,-2);lua_pop(L,2);
        if(!environment)return luaL_error(L,"SetUpdate callback environment is not the retained context");
    }
    self.source_exception={};self.source_error=nullptr;
    try {self.update_require_context_current(slot);self.update_require_current(false);graph_require_epoch(L,epoch);}
    catch(const std::exception &error){self.source_exception=std::current_exception();self.source_error=error.what();}
    catch(...){self.source_exception=std::current_exception();self.source_error="Update registration owner failed";}
    if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}
    const int previous=self.child_contexts[slot].update_function;
    int retained=LUA_NOREF;
    if(!lua_isnil(L,2)){lua_pushvalue(L,2);retained=luaL_ref(L,LUA_REGISTRYINDEX);}
    self.child_contexts[slot].update_function=retained;
    if(previous!=LUA_NOREF)luaL_unref(L,LUA_REGISTRYINDEX,previous);
    self.source_exception={};self.source_error=nullptr;
    try {self.update_require_context_current(slot);self.update_require_current(false);graph_require_epoch(L,epoch);}
    catch(const std::exception &error){self.source_exception=std::current_exception();self.source_error=error.what();}
    catch(...){self.source_exception=std::current_exception();self.source_error="Update registration postflight failed";}
    if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}
    child_require_bindings(L);self.update_registering=false;self.ready=ready;self.failed=failed;return 0;
}
static int release_retired_updates(lua_State *L) {
    auto &self=owner(L);
    for(auto &entry:self.child_contexts)if(entry.retired&&entry.update_function!=LUA_NOREF) {
        const int previous=entry.update_function;entry.update_function=LUA_NOREF;
        luaL_unref(L,LUA_REGISTRYINDEX,previous);
    }
    return 0;
}
// Private actual load lender. All allocating/error Lua APIs run INSIDE the
// wrapper's protected pcall, so Lua longjmp cannot bypass this RAII retirement.
struct EndTurnChildLoaderScope final {
 Impl& owner;lua_State* const entry;EndTurnChildLoaderScope* const previous;
 const std::thread::id creator=std::this_thread::get_id();
 const bool prior_failed,prior_ready;
 const bool healthy_origin;
 const std::uint64_t epoch,writes;
 const std::size_t attempt;
 std::size_t child=graph_no_owner;bool returned=false;
 EndTurnChildLoaderScope(Impl& p,lua_State* L):owner(p),entry(L),previous(p.end_turn_child_loader),
  prior_failed(p.failed),prior_ready(p.ready),healthy_origin(previous?previous->current():!p.failed),epoch(p.failure_epoch),writes(p.failed.writes_),attempt(p.child_attempt_succeeded.size()){
  if(creator!=owner.ui_thread)throw ScriptError("Event69 child lender foreign thread");
  owner.end_turn_child_loader=this;
 }
 ~EndTurnChildLoaderScope(){
  if(owner.end_turn_child_loader!=this||!returned)owner.record_failure();
  owner.end_turn_child_loader=previous;
 }
 EndTurnChildLoaderScope(const EndTurnChildLoaderScope&)=delete;
 EndTurnChildLoaderScope& operator=(const EndTurnChildLoaderScope&)=delete;
 bool current()const noexcept{return creator==std::this_thread::get_id()&&owner.end_turn_child_loader==this&&
  healthy_origin&&entry&&owner.state&&epoch==owner.failure_epoch&&!owner.failed.exhausted_&&writes==owner.failed.writes_&&
  attempt<owner.child_attempt_succeeded.size()&&!owner.child_attempt_succeeded[attempt]&&child<owner.child_contexts.size()&&
  owner.child_contexts[child].loading&&!owner.child_contexts[child].retired;}
};
EndTurnChildLoaderScope* end_turn_child_loader=nullptr;
static constexpr std::size_t child_source_limit=8*1024*1024; // host policy
void child_require_current(std::size_t slot) {
    if(!context_sources||slot>=child_contexts.size())throw ScriptError("Child context owner unavailable");
    auto &entry=child_contexts[slot];
    if(entry.retired)throw ScriptError("Child context retired");
    const auto epoch=failure_epoch;
    configured_loading_lifetime();
    context_sources->require_current(entry.name,entry.source.get());configured_loading_lifetime();
    if(entry.source) {
        entry.source->require_current();configured_loading_lifetime();
        const auto context=entry.source->context();configured_loading_lifetime();
        const auto* asset=&entry.source->asset();configured_loading_lifetime();
        if(context!=entry.name||asset!=entry.asset)
            throw ScriptError("Child source identity rebound");
        if(entry.loaded){entry.source->require_captured_bytes(entry.bytes);configured_loading_lifetime();}
    }
    if(epoch!=failure_epoch)throw ScriptError("Child owner caught a reentrant host failure");
}
void child_require_all() {
    const auto epoch=failure_epoch;
    if(update_registering)throw ScriptError("Update registration had a terminal failure");
    for(bool succeeded:child_attempt_succeeded)if(!succeeded)throw ScriptError("Child load had a terminal failure");
    for(std::size_t i=0;i<child_contexts.size();++i)child_require_current(i);
    if(epoch!=failure_epoch)throw ScriptError("Child owner callback failed");
}
static void child_require_bindings(lua_State *L) {
    auto &self=owner(L);
    if(!self.context_sources)return;
    // Called only inside a protected Lua boundary. The registry-rooted key
    // avoids string allocation/GC while reading bindings; no provider runs.
    // No owning C++ local crosses stack growth or error Lua APIs.
    if(!lua_checkstack(L,3)){self.record_failure();luaL_error(L,"Child binding check stack unavailable");return;}
    for(std::size_t slot=0;slot<self.child_contexts.size();++slot) {
        const auto &entry=self.child_contexts[slot];
        if(entry.environment==LUA_NOREF)continue; // not yet installed
        lua_rawgeti(L,LUA_REGISTRYINDEX,entry.environment);
        if(!lua_istable(L,-1)){self.record_failure();luaL_error(L,"Retained child environment retired");return;}
        if(entry.receiver==LUA_NOREF&&entry.state_prepared&&!entry.loaded){
            if(!self.worldview_native_child_controls_current(slot)||entry.execution_thread==LUA_NOREF){
                self.record_failure();luaL_error(L,"Prepared Context518 has no actual native binding scope");return;}
            lua_pop(L,1);continue; // original pre-binding phase, never a loaded/callback receiver
        }
        if(self.child_binding_key==LUA_NOREF){self.record_failure();luaL_error(L,"Retained child binding key unavailable");return;}
        lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_binding_key);lua_rawget(L,-2);
        lua_rawgeti(L,LUA_REGISTRYINDEX,entry.receiver);
        const bool same=lua_istable(L,-1)&&lua_rawequal(L,-1,-2);
        lua_pop(L,3);
        if(!same){self.record_failure();luaL_error(L,"Retained root/child ContextPtr rebound");return;}
        if(entry.update_function!=LUA_NOREF) {
            lua_rawgeti(L,LUA_REGISTRYINDEX,entry.update_function);
            if(!lua_isfunction(L,-1)){self.record_failure();luaL_error(L,"Retained update closure retired");return;}
            lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,entry.environment);
            const bool same_environment=lua_rawequal(L,-1,-2);lua_pop(L,3);
            if(!same_environment){self.record_failure();luaL_error(L,"Retained update closure environment rebound");return;}
        }
    }
}
#include "lua_context_loader_temporary_refs.inc"
static void install_child_context(lua_State *L,int environment,std::string_view context) {
    auto &self=owner(L);
    if(!self.context_sources)return;
    std::size_t slot=0;
    for(;slot<self.child_contexts.size();++slot)if(self.child_contexts[slot].name==context)break;
    if(slot==self.child_contexts.size()) {
        if(slot>=4096)throw ScriptError("Child context host policy budget exhausted");
        self.child_contexts.emplace_back();self.child_contexts.back().name=context;
    }
    auto &entry=self.child_contexts[slot];self.child_require_current(slot);
    if(self.child_binding_key==LUA_NOREF) {
        lua_pushliteral(L,"ContextPtr");self.child_binding_key=luaL_ref(L,LUA_REGISTRYINDEX);
    }
    if(entry.environment!=LUA_NOREF) {
        lua_rawgeti(L,LUA_REGISTRYINDEX,entry.environment);
        const bool same=lua_rawequal(L,environment,-1);lua_pop(L,1);
        if(!same)return (void)luaL_error(L,"Child environment rebound");
        if(entry.receiver!=LUA_NOREF){
            lua_getfield(L,environment,"ContextPtr");
            lua_rawgeti(L,LUA_REGISTRYINDEX,entry.receiver);
            const bool receiver=lua_rawequal(L,-1,-2);lua_pop(L,2);
            if(!receiver)return (void)luaL_error(L,"Child ContextPtr rebound");
            retain_context_loader_temporary_refs(L,slot,environment);
            return;
        }
        if(!entry.state_prepared||!self.worldview_native_child_controls_current(slot))
            return (void)luaL_error(L,"Partial child environment has no genuine Context518 constructor/load scope");
    }
    // Preserve the real layout or graph receiver already installed above.
    // Context-only sources get an actual retained context object, no Controls.
    lua_getfield(L,environment,"ContextPtr");
    if(lua_isnil(L,-1)) {lua_pop(L,1);lua_newtable(L);lua_pushvalue(L,-1);lua_setfield(L,environment,"ContextPtr");}
    if(!lua_istable(L,-1))return (void)luaL_error(L,"Unsupported ContextPtr representation");
    if(entry.environment==LUA_NOREF){lua_pushvalue(L,environment);entry.environment=luaL_ref(L,LUA_REGISTRYINDEX);}
    lua_pushvalue(L,-1);entry.receiver=luaL_ref(L,LUA_REGISTRYINDEX);
    lua_pushinteger(L,static_cast<lua_Integer>(slot));lua_pushvalue(L,-2);
    lua_pushcclosure(L,load_new_context,2);lua_setfield(L,-2,"LoadNewContext");lua_pop(L,1);
    if(self.update_sources) {
        lua_rawgeti(L,LUA_REGISTRYINDEX,entry.receiver);
        lua_pushinteger(L,static_cast<lua_Integer>(slot));lua_pushvalue(L,-2);
        lua_pushcclosure(L,set_context_update,2);lua_setfield(L,-2,"SetUpdate");lua_pop(L,1);
    }
    retain_context_loader_temporary_refs(L,slot,environment);
}
bool child_prepare(std::size_t parent,const char *request,std::size_t length,std::size_t &child) {
    source_exception={};source_error=nullptr;
    try {
        std::unique_ptr<EndTurnChildPrepareScope> provider;
        if(end_turn_lifetime&&end_turn_child_loader)provider=std::make_unique<EndTurnChildPrepareScope>(*this,parent,request,length);
        const auto post=[&]{configured_loading_lifetime();if(provider)provider->require_current();};
        post();child_require_current(parent);post();
        if(std::string_view(request,length).empty()||std::string_view(request,length).find('\0')!=std::string_view::npos)
            throw ScriptError("Invalid child source request");
        if(child_contexts.size()>=4096||child_load_depth>=64)throw ScriptError("Child loader host policy budget exhausted");
        child=child_contexts.size();child_contexts.emplace_back();
        auto &entry=child_contexts[child];auto &parent_entry=child_contexts[parent];
        entry.loading=true;
        entry.source=context_sources->resolve_child(parent_entry.name,parent_entry.source.get(),{request,length});post();
        if(!entry.source)throw ScriptError("Owned child source unresolved");
        entry.source->require_current();post();entry.name=entry.source->context();post();entry.asset=&entry.source->asset();post();
        if(entry.name.empty()||entry.name.find('\0')!=std::string::npos||executed_contexts.contains(entry.name))
            throw ScriptError("Child owner must issue a fresh context identity");
        for(std::size_t i=0;i<child;++i)if(child_contexts[i].name==entry.name)throw ScriptError("Child context identity reused");
        child_require_current(child);child_require_current(parent);
        entry.bytes=entry.asset->read_bytes(child_source_limit);post();
        entry.source->require_captured_bytes(entry.bytes);post();
        entry.chunk="@"+entry.asset->candidate().original_name;
        if(entry.chunk.find('\0')!=std::string::npos)throw ScriptError("NUL in owned child chunk name");
        child_require_current(child);child_require_current(parent);
        executed_contexts.emplace(entry.name);
        return true;
    }catch(const std::exception &error){source_exception=std::current_exception();source_error=error.what();}
    catch(...){source_exception=std::current_exception();source_error="Child source provider failed";}
    return false;
}
static int run_child_context(lua_State *L) {
    auto &self=owner(L);
    const auto slot=static_cast<std::size_t>(lua_tointeger(L,lua_upvalueindex(1)));
    // Catch C++ provider failures before entering lua_error, preserving owners.
    self.source_exception={};self.source_error=nullptr;
    try {self.child_require_current(slot);}
    catch(const std::exception &error){self.source_exception=std::current_exception();self.source_error=error.what();}
    catch(...){self.source_exception=std::current_exception();self.source_error="Child owner failed";}
    if(self.source_error)return luaL_error(L,"%s",self.source_error);
    auto &entry=self.child_contexts[slot];
    const char *bytes=entry.bytes.empty()?"":reinterpret_cast<const char*>(entry.bytes.data());std::size_t length=entry.bytes.size();
    if(length>=3&&std::memcmp(bytes,"\xef\xbb\xbf",3)==0){bytes+=3;length-=3;}
    if(length&&bytes[0]=='\x1b')return luaL_error(L,"Child source bytecode refused");
    // execute_context catches C++ exceptions in its outer caller below.
    std::size_t executed=0;
    try {
        executed=self.execution_begin(ScriptLuaAction::context_execute,entry.name,entry.chunk);
        self.execution_source(executed,{bytes,length});
        execute_context(L,entry.name,{bytes,length},entry.chunk);
        self.execution_finish(executed,ScriptLuaActionOutcome::succeeded);
    }
    catch(const std::exception &error){self.source_exception=std::current_exception();self.source_error=error.what();}
    catch(...){self.source_exception=std::current_exception();self.source_error="Child context initialization failed";}
    if(self.source_error)return luaL_error(L,"%s",self.source_error);
    return 0;
}
#include "lua_generic_child_include_lender.inc"
static int load_new_context_body(lua_State *L) {
    auto &self=owner(L);const bool ready=self.ready,failed=self.failed;const auto epoch=self.failure_epoch;
    self.ready=false;
    if(self.end_turn_child_loader&&self.end_turn_child_loader->entry==L&&
       self.end_turn_child_loader->attempt==self.child_attempt_succeeded.size())self.failed.set_loader_sentinel();
    else if(self.generic_child_loader&&self.generic_child_loader->entry==L&&self.generic_child_loader->healthy_origin&&
            self.generic_child_loader->attempt==self.child_attempt_succeeded.size()&&self.generic_child_loader->epoch==self.failure_epoch&&
            self.generic_child_loader->uncertainty==self.execution_uncertainty&&self.generic_child_loader->writes==self.failed.writes_)
        self.failed.set_loader_sentinel();
    else self.failed=true;
    if(self.child_attempt_succeeded.size()>=4096){self.record_failure();return luaL_error(L,"Child attempt host policy budget exhausted");}
    const auto attempt=self.child_attempt_succeeded.size();
    self.source_exception={};self.source_error=nullptr;
    try {self.child_attempt_succeeded.push_back(false);}
    catch(...){self.source_exception=std::current_exception();self.source_error="Cannot retain child load attempt";}
    if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}
    const auto parent=static_cast<std::size_t>(lua_tointeger(L,lua_upvalueindex(1)));
    std::size_t action=0;
    try {action=self.execution_begin(ScriptLuaAction::child_load,
        parent<self.child_contexts.size()?std::string_view(self.child_contexts[parent].name):std::string_view{},"LoadNewContext");}
    catch(const std::exception &error){self.source_exception=std::current_exception();self.source_error=error.what();}
    catch(...){self.source_exception=std::current_exception();self.source_error="Cannot retain child execution action";}
    if(self.source_error)return luaL_error(L,"%s",self.source_error);
    if(lua_gettop(L)!=2||!lua_rawequal(L,1,lua_upvalueindex(2)))return luaL_error(L,"LoadNewContext receiver/arguments differ");
    std::size_t length=0;const char *request=luaL_checklstring(L,2,&length);
    try {self.execution_records[action].name.assign(request,length);}
    catch(...){self.source_exception=std::current_exception();self.source_error="Cannot retain child request";}
    if(self.source_error)return luaL_error(L,"%s",self.source_error);
    lua_Debug caller{};
    const int caller_level=(self.end_turn_child_loader&&self.end_turn_child_loader->entry==L)||
        (self.generic_child_loader&&self.generic_child_loader->entry==L)?2:1;
    if(!lua_getstack(L,caller_level,&caller)||!lua_getinfo(L,"f",&caller))return luaL_error(L,"Child load has no Lua caller");
    const auto compiled=self.graph_compiled_owner(L,-1);lua_pop(L,1);
    if(parent>=self.child_contexts.size()||compiled==graph_no_owner||self.graph_execution_scope==graph_no_owner||
       self.graph_compiled_sources[compiled].context!=self.child_contexts[parent].name||
       self.graph_compiled_sources[self.graph_execution_scope].context!=self.child_contexts[parent].name)
        return luaL_error(L,"Child load has no same compiled parent scope");
    lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[parent].environment);
    lua_getfield(L,-1,"ContextPtr");
    const bool receiver=lua_rawequal(L,-1,1);lua_pop(L,2);
    if(!receiver)return luaL_error(L,"Parent ContextPtr rebound");
    std::size_t child=0;
    if(!self.child_prepare(parent,request,length,child)) {
        self.execution_finish(action,ScriptLuaActionOutcome::error);return luaL_error(L,"%s",self.source_error);
    }
    if(epoch!=self.failure_epoch)return luaL_error(L,"Child provider caught host reentry");
    if(self.end_turn_child_loader&&self.end_turn_child_loader->entry==L&&self.end_turn_child_loader->attempt==attempt)
        self.end_turn_child_loader->child=child;
    if(self.generic_child_loader&&self.generic_child_loader->entry==L&&self.generic_child_loader->attempt==attempt)
        self.generic_child_loader->child=child;
    const auto previous_scope=self.graph_execution_scope;
    lua_pushinteger(L,static_cast<lua_Integer>(child));lua_pushcclosure(L,run_child_context,1);
    ++self.child_load_depth;const int status=lua_pcall(L,0,0,0);--self.child_load_depth;
    self.graph_execution_scope=previous_scope;
    if(status){self.execution_finish(action,ScriptLuaActionOutcome::error);return lua_error(L);}
    self.source_exception={};self.source_error=nullptr;
    try {self.child_require_current(child);self.child_contexts[child].source->require_captured_bytes(self.child_contexts[child].bytes);
         self.child_require_current(parent);if(epoch!=self.failure_epoch)throw ScriptError("Child invocation failed");}
    catch(const std::exception &error){self.source_exception=std::current_exception();self.source_error=error.what();}
    catch(...){self.source_exception=std::current_exception();self.source_error="Child currentness failed";}
    if(self.source_error)return luaL_error(L,"%s",self.source_error);
    // A caught nested error must not be hidden by this outer successful chunk.
    for(std::size_t i=attempt+1;i<self.child_attempt_succeeded.size();++i)
        if(!self.child_attempt_succeeded[i])return luaL_error(L,"Nested child load failed");
    self.child_contexts[child].loading=false;self.child_contexts[child].loaded=true;
    lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[child].environment);lua_getfield(L,-1,"ContextPtr");
    lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[child].receiver);
    const bool retained=lua_rawequal(L,-1,-2);lua_pop(L,3);
    if(!retained)return luaL_error(L,"Child ContextPtr rebound");
    lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[child].receiver);
    self.execution_finish(action,ScriptLuaActionOutcome::succeeded);
    self.child_attempt_succeeded[attempt]=true;
    self.ready=ready;
    if(failed&&self.end_turn_child_loader&&self.end_turn_child_loader->entry==L&&
       self.end_turn_child_loader->healthy_origin&&self.end_turn_child_loader->epoch==self.failure_epoch&&
       self.end_turn_child_loader->writes==self.failed.writes_&&!self.failed.exhausted_)self.failed.set_loader_sentinel();
    else if(failed&&self.generic_child_loader&&self.generic_child_loader->entry==L&&self.generic_child_loader->healthy_origin&&
            self.generic_child_loader->epoch==self.failure_epoch&&self.generic_child_loader->uncertainty==self.execution_uncertainty&&
            self.generic_child_loader->writes==self.failed.writes_&&!self.failed.exhausted_)self.failed.set_loader_sentinel();
    else self.failed=failed;
    return 1;
}

static int load_new_context(lua_State* L){
 auto& self=owner(L);
 if(!self.end_turn_lifetime)return load_generic_child_context(L); // genuine generic operation lender, no selected grant
 const int count=lua_gettop(L);
 // Prepare the protected closure before the RAII loan; no sentinel exists yet.
 lua_pushvalue(L,lua_upvalueindex(1));lua_pushvalue(L,lua_upvalueindex(2));
 lua_pushcclosure(L,load_new_context_body,2);lua_insert(L,1);
 int status=0;
 try{
  {EndTurnChildLoaderScope lender(self,L);
   status=lua_pcall(L,count,1,0);lender.returned=(status==0);
  } // retired before lua_error; the protected body's longjmp never crosses RAII
 }catch(...){self.record_failure();return luaL_error(L,"Event69 actual child load lender refused");}
 if(status)return lua_error(L);
 return 1;
}
