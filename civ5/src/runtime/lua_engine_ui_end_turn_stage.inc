// Included inside LuaScriptHost::Impl after actual child/source/Proto machinery.
using EndTurnQueue=::civ5::ui::BnwEngineUiEndTurnQueue;
std::shared_ptr<EndTurnQueue::VmLifetime> end_turn_lifetime;
std::shared_ptr<const EndTurnQueue::Consumer> end_turn_consumer;
std::weak_ptr<const ScriptUiChildSource> end_turn_expected_source;
std::weak_ptr<const ScriptUiContextSourceOwner> end_turn_expected_sources;
std::string end_turn_context;
std::vector<int> end_turn_functions;
lua_State* end_turn_registered_state=nullptr;
int end_turn_registered_thread=LUA_NOREF;
std::uint64_t end_turn_registration=0;
bool end_turn_load_active=false,end_turn_load_prior_failed=false;
std::uint64_t end_turn_load_epoch=0;
std::size_t end_turn_load_source=graph_no_owner;
EndTurnChildLoaderScope* end_turn_load_owner=nullptr;
bool end_turn_sealed=false,end_turn_registering=false,end_turn_native_active=false;
bool end_turn_load_sentinel()const{return end_turn_load_active&&!end_turn_load_prior_failed&&
 end_turn_load_owner&&end_turn_load_owner==end_turn_child_loader&&end_turn_load_owner->current()&&
 end_turn_load_epoch==failure_epoch&&end_turn_load_source==graph_execution_scope;}
void end_turn_current(bool require_loaded){
 if(std::this_thread::get_id()!=ui_thread||!end_turn_lifetime||!end_turn_lifetime->alive||
  end_turn_lifetime->failed||(failed&&(require_loaded||!end_turn_load_sentinel()))||!context_sources)throw ScriptError("Event69 selected VM/source is unjoined or failed");
 const auto epoch=failure_epoch;
 const auto sources=end_turn_expected_sources.lock();const auto source=end_turn_expected_source.lock();
 if(!sources||sources.get()!=context_sources.get()||sources.owner_before(context_sources)||context_sources.owner_before(sources)||!source)
  throw ScriptError("Event69 canonical selected source controls expired");
 std::size_t slot=0;for(;slot<child_contexts.size();++slot)if(child_contexts[slot].name==end_turn_context)break;
 if(slot==child_contexts.size())throw ScriptError("Event69 actual selected child has not loaded");
 auto& child=child_contexts[slot];
 if(!require_loaded&&failed&&(!end_turn_load_owner||end_turn_load_owner->child!=slot))throw ScriptError("Event69 sentinel belongs to another actual child attempt");
 if(child.source.get()!=source.get()||child.source.owner_before(source)||source.owner_before(child.source)||
  child.retired||child.environment==LUA_NOREF||(require_loaded&&(!child.loaded||child.loading)))
  throw ScriptError("Event69 actual selected child/source changed");
 const auto post=[&]{if(epoch!=failure_epoch||(failed&&(require_loaded||!end_turn_load_sentinel()))||!end_turn_lifetime->alive||end_turn_lifetime->failed)
  throw ScriptError("Event69 source callback poisoned VM");};
 sources->require_current(child.name,source.get());post();
 source->require_current();post();
 const auto context=source->context();post();
 const auto* asset=&source->asset();post();
 if(context!=child.name||asset!=child.asset)throw ScriptError("Event69 child asset/context rebound");
 if(child.loaded){source->require_captured_bytes(child.bytes);post();}
}
void end_turn_require_consumer(const EndTurnQueue::Consumer& c){
 if(std::this_thread::get_id()!=ui_thread)throw ScriptError("Event69 consumer foreign thread");
 if(end_turn_native_active){record_failure();throw ScriptError("Event69 consumer native reentry");}
 end_turn_native_active=true;
 try{
  const auto life=c.lifetime.lock();
  if(!life||life.get()!=end_turn_lifetime.get()||!life->alive||life->failed||life->vm!=state||
   !end_turn_consumer||end_turn_consumer.get()!=&c||c.vm!=state||c.epoch!=failure_epoch||
   c.registration!=end_turn_registration||c.registered_state!=end_turn_registered_state||end_turn_registered_thread==LUA_NOREF||!end_turn_sealed||end_turn_registering||end_turn_functions.size()!=1)
   throw ScriptError("Event69 selected actual VM/registration control changed");
  end_turn_current(true);
  if(failed||c.epoch!=failure_epoch||!life->alive||life->failed)throw ScriptError("Event69 consumer source postflight failed");
  end_turn_native_active=false;
 }catch(...){end_turn_native_active=false;record_failure();if(end_turn_lifetime)end_turn_lifetime->failed=true;throw;}
}
static int end_turn_event_method(lua_State* L){
 auto& self=owner(L);const auto op=static_cast<int>(lua_tointeger(L,lua_upvalueindex(1)));
 if((self.end_turn_sealed&&op!=3)||self.end_turn_registering){self.record_failure();return luaL_error(L,"Event69 dynamic list mutation/registration reentry is outside selected static profile");}
 self.end_turn_registering=true;
 const auto epoch=self.failure_epoch;self.source_exception={};self.source_error=nullptr;
 try{self.end_turn_current(false);}catch(...){self.source_exception=std::current_exception();self.source_error="Event69 registration source refused";}
 if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}
 lua_Debug caller{};
 if(!lua_getstack(L,1,&caller)||!lua_getinfo(L,"f",&caller)){self.record_failure();return luaL_error(L,"Event69 lacks actual compiled caller");}
 const auto parent=self.graph_compiled_owner(L,-1);lua_pop(L,1);
 if(parent==graph_no_owner||self.graph_execution_scope!=parent||self.graph_compiled_sources[parent].context!=self.end_turn_context){
  self.record_failure();return luaL_error(L,"Event69 registration is not the SAME selected compiled source");}
 const int count=lua_gettop(L);
 if(op==0||op==1){
  if(count<1||!lua_isfunction(L,count)||lua_iscfunction(L,count)||self.graph_compiled_owner(L,count)!=parent){
   self.record_failure();return luaL_error(L,"Event69 callback is not owned by actual selected Proto");}
  std::size_t slot=0;for(;slot<self.child_contexts.size();++slot)if(self.child_contexts[slot].name==self.end_turn_context)break;
  lua_getfenv(L,count);lua_rawgeti(L,LUA_REGISTRYINDEX,self.child_contexts[slot].environment);
  const bool actual_env=lua_rawequal(L,-1,-2);lua_pop(L,2);
  if(!actual_env||self.child_contexts[slot].execution_source!=parent){self.record_failure();return luaL_error(L,"Event69 callback differs from actual retained child load");}
  if(op==0){
   if(self.end_turn_registration==UINT64_MAX){self.record_failure();return luaL_error(L,"Event69 registration occurrence exhausted");}
   if(self.end_turn_registered_state&&self.end_turn_registered_state!=L){self.record_failure();return luaL_error(L,"Event69 Add changed actual registered Lua state");}
   if(!self.end_turn_registered_state){lua_pushthread(L);self.end_turn_registered_thread=luaL_ref(L,LUA_REGISTRYINDEX);self.end_turn_registered_state=L;}
   lua_pushvalue(L,count);const int retained=luaL_ref(L,LUA_REGISTRYINDEX);
   try{self.end_turn_functions.push_back(retained);}catch(...){luaL_unref(L,LUA_REGISTRYINDEX,retained);self.record_failure();return luaL_error(L,"Event69 registration allocation failed");}
   ++self.end_turn_registration;
  }else{
   for(auto it=self.end_turn_functions.begin();it!=self.end_turn_functions.end();){
    lua_rawgeti(L,LUA_REGISTRYINDEX,*it);const bool match=lua_rawequal(L,-1,count);lua_pop(L,1);
    if(match){luaL_unref(L,LUA_REGISTRYINDEX,*it);it=self.end_turn_functions.erase(it);}else ++it;
   }
  }
 }else if(op==2){for(int ref:self.end_turn_functions)luaL_unref(L,LUA_REGISTRYINDEX,ref);self.end_turn_functions.clear();}
 else if(op!=3){self.record_failure();return luaL_error(L,"Unsupported Event69 operation");}
 self.source_exception={};self.source_error=nullptr;
 try{self.end_turn_current(false);if(epoch!=self.failure_epoch)throw ScriptError("Event69 registration epoch changed");}
 catch(...){self.source_exception=std::current_exception();self.source_error="Event69 registration postflight failed";}
 if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}
 self.end_turn_registering=false;
 if(op==0){lua_pushvalue(L,count);return 1;}if(op==3){lua_pushinteger(L,static_cast<lua_Integer>(self.end_turn_functions.size()));return 1;}return 0;
}
static int end_turn_event(lua_State* L){
 auto& self=owner(L);std::size_t n=0;const char* context=luaL_checklstring(L,1,&n);
 if(!self.end_turn_lifetime||self.end_turn_context!=std::string_view(context,n))return luaL_error(L,"Event69 selected child activation is unjoined");
 lua_newtable(L);constexpr const char* names[]={"Add","Remove","RemoveAll","Count"};
 for(int op=0;op<4;++op){lua_pushinteger(L,op);lua_pushcclosure(L,end_turn_event_method,1);lua_setfield(L,-2,names[op]);}
 return 1;
}
std::size_t end_turn_check_closure(lua_State* L){
 lua_rawgeti(L,LUA_REGISTRYINDEX,end_turn_functions.front());
 const auto compiled=graph_compiled_owner(L,-1);
 if(!end_turn_consumer||end_turn_consumer->child>=child_contexts.size()||compiled==graph_no_owner||
  compiled!=child_contexts[end_turn_consumer->child].execution_source||graph_compiled_sources[compiled].context!=end_turn_context)
  luaL_error(L,"Event69 registered actual child Proto changed");
 lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,child_contexts[end_turn_consumer->child].environment);
 const bool same=lua_rawequal(L,-1,-2);lua_pop(L,2);
 if(!same)luaL_error(L,"Event69 registered closure environment rebound");
 return compiled; // actual closure remains on stack for protected invocation
}
static int end_turn_registered_dispatch(lua_State* registered){
 auto& self=owner(registered);
 if(registered!=self.end_turn_registered_state||self.end_turn_functions.size()!=1)
  return luaL_error(registered,"Event69 actual registered state/list changed");
 if(!lua_checkstack(registered,6))return luaL_error(registered,"Event69 registered callback stack unavailable");
 self.graph_execution_scope=self.end_turn_check_closure(registered);
 lua_call(registered,0,0); // same registered state; whole context protected by caller
 return 0;
}
void perform_end_turn_dirty(lua_State* L){
 end_turn_current(true);
 if(!end_turn_sealed||end_turn_registering||!end_turn_consumer||end_turn_functions.size()!=1||
  !end_turn_registered_state||end_turn_registered_thread==LUA_NOREF)
  throw ScriptError("Event69 actual static selected consumer is unsealed");
 const auto epoch=failure_epoch;const int ref=end_turn_functions.front();
 if(!lua_checkstack(L,4))luaL_error(L,"Event69 protected callback stack unavailable");
 lua_rawgeti(L,LUA_REGISTRYINDEX,end_turn_registered_thread);
 const bool actual=lua_isthread(L,-1)&&lua_tothread(L,-1)==end_turn_registered_state;lua_pop(L,1);
 if(!actual||lua_status(end_turn_registered_state))luaL_error(L,"Event69 actual registered coroutine is unavailable");
 const int top=lua_gettop(end_turn_registered_state);
 const auto previous=graph_execution_scope;
 const auto prior_hook=lua_gethook(end_turn_registered_state);
 const int prior_mask=lua_gethookmask(end_turn_registered_state),prior_count=lua_gethookcount(end_turn_registered_state);
 if(budget.instructions)lua_sethook(end_turn_registered_state,hook,LUA_MASKCOUNT,1000);
 const int status=lua_cpcall(end_turn_registered_state,end_turn_registered_dispatch,nullptr);
 lua_sethook(end_turn_registered_state,prior_hook,prior_mask,prior_count);graph_execution_scope=previous;
 if(status){
  if(end_turn_registered_state!=L){lua_xmove(end_turn_registered_state,L,1);lua_settop(end_turn_registered_state,top);}
  lua_error(L);
 }
 lua_settop(end_turn_registered_state,top);
 child_require_bindings(L);end_turn_current(true);
 end_turn_check_closure(L);lua_pop(L,1);
 if(epoch!=failure_epoch||end_turn_functions.size()!=1||end_turn_functions.front()!=ref)
  throw ScriptError("Event69 callback changed owner/list/epoch");
}

struct EndTurnIncludeCompilerScope final {
 Impl& owner;EndTurnChildLoaderScope* const loader;EndTurnIncludeCompilerScope* const previous;
 lua_State* const issuer;const char* const context;const std::size_t context_size;
 const char* const path;const std::size_t path_size;const void* const environment;
 const std::uint64_t epoch,uncertainty,writes;const bool prior_ready,prior_failed;
 bool succeeded=false;
 EndTurnIncludeCompilerScope(Impl& p,lua_State* L,const char* cn,std::size_t nc,const char* pn,std::size_t np,const void* env)
  :owner(p),loader(p.end_turn_child_loader),previous(p.end_turn_compiler_scope),issuer(L),context(cn),context_size(nc),
   path(pn),path_size(np),environment(env),epoch(p.failure_epoch),uncertainty(p.execution_uncertainty),writes(p.failed.writes_),
   prior_ready(p.ready),prior_failed(p.failed){
  if(previous||!loader||!loader->current()||!p.failed||!p.calling||p.include_provider_scope||
   !p.end_turn_lifetime||!p.end_turn_lifetime->alive||p.end_turn_lifetime->failed)
   throw ScriptError("Event69 include compiler has no healthy-origin actual loader");
  p.end_turn_compiler_scope=this;
 }
 bool current()const noexcept{return owner.end_turn_compiler_scope==this&&loader==owner.end_turn_child_loader&&
  loader&&loader->current()&&owner.failed&&owner.calling&&owner.failure_epoch==epoch&&owner.execution_uncertainty==uncertainty&&
  writes==owner.failed.writes_&&!owner.failed.exhausted_&&owner.end_turn_lifetime&&owner.end_turn_lifetime->alive&&!owner.end_turn_lifetime->failed&&
  issuer&&context&&path&&environment&&&Impl::owner(issuer)==&owner;}
 ~EndTurnIncludeCompilerScope(){
  if(owner.end_turn_compiler_scope!=this||!succeeded)owner.record_failure();
  owner.end_turn_compiler_scope=previous;
 }
 EndTurnIncludeCompilerScope(const EndTurnIncludeCompilerScope&)=delete;
 EndTurnIncludeCompilerScope& operator=(const EndTurnIncludeCompilerScope&)=delete;
};
bool end_turn_compiler_proves(lua_State* L,const char* cn,std::size_t nc,const char* pn,std::size_t np,
 const void* env,std::uint64_t epoch,std::uint64_t uncertainty)const noexcept{
 const auto* c=end_turn_compiler_scope;
 return c&&c->current()&&c->issuer==L&&c->context==cn&&c->context_size==nc&&c->path==pn&&c->path_size==np&&
  c->environment==env&&c->epoch==epoch&&c->uncertainty==uncertainty;
}
bool end_turn_qualify_compiler(lua_State* L,const char* context,std::size_t cn,const char* path,std::size_t pn,const void* env){
 if(!end_turn_child_loader)return false;
 // Lua-side argument/receiver/caller provenance first. No C++ strong pin is
 // live while allocating Lua APIs may longjmp.
 if(!end_turn_child_loader->current()||!end_turn_lifetime||!end_turn_lifetime->alive||end_turn_lifetime->failed||
  !failed||!calling||end_turn_compiler_scope||include_provider_scope||source_error||exhausted||
  !selected_loading_context_matches(std::string_view(context,cn))||std::string_view(path,pn).find('\0')!=std::string_view::npos)
  throw ScriptError("Event69 include arguments/current healthy loader refused");
 const auto child=end_turn_child_loader->child;
 lua_rawgeti(L,LUA_REGISTRYINDEX,child_contexts[child].environment);
 const bool same=lua_rawequal(L,3,-1);lua_pop(L,1);
 if(!same||lua_topointer(L,3)!=env||graph_execution_scope==graph_no_owner||
  !selected_loading_context_matches(graph_compiled_sources[graph_execution_scope].context))
  throw ScriptError("Event69 compiler environment/source scope is not actual child");
 std::size_t compiled=graph_no_owner;lua_Debug frame{};
 for(int level=1;lua_getstack(L,level,&frame);++level){
  if(!lua_getinfo(L,"f",&frame))break;
  if(lua_iscfunction(L,-1)){lua_pop(L,1);continue;}
  if(!lua_isfunction(L,-1)){lua_pop(L,1);break;}
  const auto* closure=static_cast<const Closure*>(lua_topointer(L,-1));
  if(include_bootstrap_proto&&graph_contains_proto(include_bootstrap_proto,closure->l.p)){lua_pop(L,1);continue;}
  compiled=graph_compiled_owner(L,-1);lua_pop(L,1);break;
 }
 if(compiled==graph_no_owner||!selected_loading_context_matches(graph_compiled_sources[compiled].context))
  throw ScriptError("Event69 include caller is not actual retained compiled child/helper");
 if(!end_turn_selected_source_controls_current(child))
  throw ScriptError("Selected compiler canonical source controls changed");
 return true;
}

bool end_turn_selected_source_controls_current(std::size_t child)const{
 if(worldview_native_host)return worldview_native_child_controls_current(child);
 const auto source=end_turn_expected_source.lock();const auto sources=end_turn_expected_sources.lock();
 return source&&sources&&child<child_contexts.size()&&sources.get()==context_sources.get()&&
  !sources.owner_before(context_sources)&&!context_sources.owner_before(sources)&&source.get()==child_contexts[child].source.get()&&
  !source.owner_before(child_contexts[child].source)&&!child_contexts[child].source.owner_before(source);
}
struct EndTurnSourceListScope final {
 Impl& owner;EndTurnChildLoaderScope* const loader;lua_State* const issuer;
 const std::uint64_t epoch,uncertainty,writes;
 const char* const context;const std::size_t context_size;
 EndTurnSourceListScope(Impl& p,lua_State* L,const char* cn,std::size_t nc):owner(p),loader(p.end_turn_child_loader),issuer(L),
  epoch(p.failure_epoch),uncertainty(p.execution_uncertainty),writes(p.failed.writes_),context(cn),context_size(nc){
  if(!loader||!loader->current()||!p.failed||!p.calling||p.end_turn_source_list||p.include_provider_scope||
   !p.selected_loading_context_matches(std::string_view(cn,nc))||!p.selected_loading_context_matches(p.child_contexts[loader->child].name)||
   !p.end_turn_lifetime||!p.end_turn_lifetime->alive||p.end_turn_lifetime->failed||!p.end_turn_selected_source_controls_current(loader->child))
   throw ScriptError("Event69 selected source list has no actual healthy child loader");
  p.end_turn_source_list=this;
 }
 void require_current()const{
  if(owner.end_turn_source_list!=this||owner.end_turn_child_loader!=loader||!loader->current()||
   !owner.failed||!owner.calling||owner.failure_epoch!=epoch||owner.execution_uncertainty!=uncertainty||
   owner.failed.writes_!=writes||owner.failed.exhausted_||!owner.end_turn_lifetime||!owner.end_turn_lifetime->alive||owner.end_turn_lifetime->failed||
   &Impl::owner(issuer)!=&owner||!owner.end_turn_selected_source_controls_current(loader->child))throw ScriptError("Event69 selected source-list lender retired");
 }
 ~EndTurnSourceListScope(){if(owner.end_turn_source_list==this)owner.end_turn_source_list=nullptr;else owner.record_failure();}
 EndTurnSourceListScope(const EndTurnSourceListScope&)=delete;
 EndTurnSourceListScope& operator=(const EndTurnSourceListScope&)=delete;
};

void configured_loading_lifetime()const{
 if(end_turn_lifetime&&(!end_turn_lifetime->alive||end_turn_lifetime->failed))throw ScriptError("Configured Lua loading facade retired during provider");
}
struct EndTurnChildPrepareScope final {
 Impl& owner;EndTurnChildLoaderScope* const loader;const std::size_t parent;
 const int parent_environment,parent_receiver;
 const std::uint64_t epoch,uncertainty,writes;const std::size_t compiled;
 const char* const request;const std::size_t request_size;
 std::weak_ptr<const ScriptUiContextSourceOwner> sources;
 EndTurnChildPrepareScope(Impl& p,std::size_t slot,const char* name,std::size_t size):owner(p),loader(p.end_turn_child_loader),parent(slot),
  parent_environment(slot<p.child_contexts.size()?p.child_contexts[slot].environment:LUA_NOREF),
  parent_receiver(slot<p.child_contexts.size()?p.child_contexts[slot].receiver:LUA_NOREF),
  epoch(p.failure_epoch),uncertainty(p.execution_uncertainty),writes(p.failed.writes_),compiled(p.graph_execution_scope),request(name),request_size(size),sources(p.context_sources){
  if(!loader||!loader->healthy_origin||p.end_turn_child_prepare||!p.failed||!p.calling||
   loader->epoch!=epoch||loader->writes!=writes||p.failed.exhausted_||loader->attempt>=p.child_attempt_succeeded.size()||
   p.child_attempt_succeeded[loader->attempt]||slot>=p.child_contexts.size()||compiled==graph_no_owner||
   p.graph_compiled_sources[compiled].context!=p.child_contexts[slot].name||parent_environment==LUA_NOREF||parent_receiver==LUA_NOREF||
   !name||!size)throw ScriptError("Event69 child prepare has no actual healthy parent/caller/source request");
  const auto expected=p.end_turn_expected_sources.lock();
  if(!expected||expected.get()!=p.context_sources.get()||expected.owner_before(p.context_sources)||p.context_sources.owner_before(expected))throw ScriptError("Event69 child prepare selected source owner control changed");
  p.configured_loading_lifetime();p.end_turn_child_prepare=this;
 }
 void require_current()const{
  owner.configured_loading_lifetime();const auto actual=sources.lock();const auto expected=owner.end_turn_expected_sources.lock();
  if(owner.end_turn_child_prepare!=this||owner.end_turn_child_loader!=loader||!loader->healthy_origin||
   !owner.failed||!owner.calling||owner.failure_epoch!=epoch||owner.execution_uncertainty!=uncertainty||owner.failed.writes_!=writes||owner.failed.exhausted_||
   owner.graph_execution_scope!=compiled||parent>=owner.child_contexts.size()||owner.child_contexts[parent].retired||
   owner.child_contexts[parent].environment!=parent_environment||owner.child_contexts[parent].receiver!=parent_receiver||
   !actual||!expected||actual.get()!=expected.get()||actual.owner_before(expected)||expected.owner_before(actual)||actual.get()!=owner.context_sources.get()||actual.owner_before(owner.context_sources)||owner.context_sources.owner_before(actual))
   throw ScriptError("Event69 parent child-prepare provider lender retired");
 }
 ~EndTurnChildPrepareScope(){if(owner.end_turn_child_prepare==this)owner.end_turn_child_prepare=nullptr;else owner.record_failure();}
 EndTurnChildPrepareScope(const EndTurnChildPrepareScope&)=delete;
 EndTurnChildPrepareScope& operator=(const EndTurnChildPrepareScope&)=delete;
};
