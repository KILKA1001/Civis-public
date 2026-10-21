// Private generic child operation lenders. They attest the actual retained
// loader/compiler occurrence only; no selected WorldView/Event69 capability.
struct GenericChildLoaderScope final {
 Impl& owner;lua_State* const entry;GenericChildLoaderScope* const previous;
 const std::thread::id creator=std::this_thread::get_id();
 const bool healthy_origin;
 const std::uint64_t epoch,uncertainty,writes;
 const std::size_t attempt;
 std::size_t child=graph_no_owner;bool returned=false;
 GenericChildLoaderScope(Impl& p,lua_State* L):owner(p),entry(L),previous(p.generic_child_loader),
  healthy_origin(previous?previous->current():!p.failed),epoch(p.failure_epoch),uncertainty(p.execution_uncertainty),
  writes(p.failed.writes_),attempt(p.child_attempt_succeeded.size()){
  if(creator!=owner.ui_thread||p.end_turn_lifetime||p.end_turn_child_loader)
   throw ScriptError("Generic child lender foreign/selected operation");
  owner.generic_child_loader=this;
 }
 ~GenericChildLoaderScope(){
  if(owner.generic_child_loader!=this||!returned)owner.record_failure();
  owner.generic_child_loader=previous;
 }
 GenericChildLoaderScope(const GenericChildLoaderScope&)=delete;
 GenericChildLoaderScope& operator=(const GenericChildLoaderScope&)=delete;
 bool current()const noexcept{return creator==std::this_thread::get_id()&&owner.generic_child_loader==this&&
  healthy_origin&&entry&&owner.state&&owner.calling&&!owner.end_turn_lifetime&&!owner.end_turn_child_loader&&
  epoch==owner.failure_epoch&&uncertainty==owner.execution_uncertainty&&!owner.failed.exhausted_&&writes==owner.failed.writes_&&
  attempt<owner.child_attempt_succeeded.size()&&!owner.child_attempt_succeeded[attempt]&&child<owner.child_contexts.size()&&
  owner.child_contexts[child].loading&&!owner.child_contexts[child].retired;}
};
GenericChildLoaderScope* generic_child_loader=nullptr;
struct GenericChildIncludeCompilerScope final {
 Impl& owner;GenericChildLoaderScope* const loader;
 lua_State* const issuer;const char* const context;const std::size_t context_size;
 const char* const path;const std::size_t path_size;const void* const environment;
 const std::shared_ptr<const ScriptUiChildSource> source;const std::size_t execution_source;
 const std::uint64_t epoch,uncertainty,writes;
 bool succeeded=false;
 GenericChildIncludeCompilerScope(Impl& p,lua_State* L,const char* cn,std::size_t nc,const char* pn,std::size_t np,const void* env)
  :owner(p),loader(p.generic_child_loader),issuer(L),context(cn),context_size(nc),path(pn),path_size(np),environment(env),
   source(loader&&loader->child<p.child_contexts.size()?p.child_contexts[loader->child].source:nullptr),
   execution_source(p.graph_execution_scope),epoch(p.failure_epoch),uncertainty(p.execution_uncertainty),writes(p.failed.writes_){
  if(!loader||!loader->current()||!p.failed||!p.calling||p.generic_child_compiler_scope||p.include_provider_scope||
   !source||p.end_turn_lifetime||p.end_turn_compiler_scope)
   throw ScriptError("Generic include compiler has no healthy actual child loader");
  p.generic_child_compiler_scope=this;
 }
 bool current()const noexcept{return owner.generic_child_compiler_scope==this&&loader==owner.generic_child_loader&&
  loader&&loader->current()&&owner.failed&&owner.calling&&owner.failure_epoch==epoch&&owner.execution_uncertainty==uncertainty&&
  writes==owner.failed.writes_&&!owner.failed.exhausted_&&!owner.end_turn_lifetime&&!owner.end_turn_compiler_scope&&
  source&&source.get()==owner.child_contexts[loader->child].source.get()&&
  !source.owner_before(owner.child_contexts[loader->child].source)&&!owner.child_contexts[loader->child].source.owner_before(source)&&
  execution_source==owner.graph_execution_scope&&execution_source==owner.child_contexts[loader->child].execution_source&&
  issuer&&context&&path&&environment&&&Impl::owner(issuer)==&owner;}
 ~GenericChildIncludeCompilerScope(){
  if(owner.generic_child_compiler_scope!=this||!succeeded)owner.record_failure();
  owner.generic_child_compiler_scope=nullptr;
 }
 GenericChildIncludeCompilerScope(const GenericChildIncludeCompilerScope&)=delete;
 GenericChildIncludeCompilerScope& operator=(const GenericChildIncludeCompilerScope&)=delete;
};
bool generic_child_compiler_proves(lua_State* L,const char* cn,std::size_t nc,const char* pn,std::size_t np,
 const void* env,std::uint64_t epoch,std::uint64_t uncertainty)const noexcept{
 const auto* c=generic_child_compiler_scope;
 return c&&c->current()&&c->issuer==L&&c->context==cn&&c->context_size==nc&&c->path==pn&&c->path_size==np&&
  c->environment==env&&c->epoch==epoch&&c->uncertainty==uncertainty;
}
bool generic_child_qualify_compiler(lua_State* L,const char* context,std::size_t cn,const char* path,std::size_t pn,const void* env){
 const auto* loader=generic_child_loader;
 if(!loader)return false;
 if(!loader->current()||!failed||!calling||!context_sources||generic_child_compiler_scope||include_provider_scope||
  source_error||exhausted||std::string_view(path,pn).find('\0')!=std::string_view::npos)
  throw ScriptError("Generic include arguments/current healthy child loader refused");
 const auto& child=child_contexts[loader->child];
 if(!child.source||child.name!=std::string_view(context,cn)||child.environment==LUA_NOREF||
  child.execution_thread==LUA_NOREF||child.execution_source==graph_no_owner||graph_execution_scope==graph_no_owner||
  graph_execution_scope!=child.execution_source||graph_compiled_sources[graph_execution_scope].context!=child.name)
  throw ScriptError("Generic compiler does not belong to actual retained child source");
 lua_rawgeti(L,LUA_REGISTRYINDEX,child.execution_thread);
 const bool thread=lua_isthread(L,-1)&&lua_tothread(L,-1)==L;lua_pop(L,1);
 lua_rawgeti(L,LUA_REGISTRYINDEX,child.environment);
 const bool same=lua_rawequal(L,3,-1);lua_pop(L,1);
 if(!thread||!same||lua_topointer(L,3)!=env)
  throw ScriptError("Generic compiler thread/environment differs from retained child");
 std::size_t compiled=graph_no_owner;lua_Debug frame{};
 for(int level=1;lua_getstack(L,level,&frame);++level){
  if(!lua_getinfo(L,"f",&frame))break;
  if(lua_iscfunction(L,-1)){lua_pop(L,1);continue;}
  if(!lua_isfunction(L,-1)){lua_pop(L,1);break;}
  const auto* closure=static_cast<const Closure*>(lua_topointer(L,-1));
  if(include_bootstrap_proto&&graph_contains_proto(include_bootstrap_proto,closure->l.p)){lua_pop(L,1);continue;}
  compiled=graph_compiled_owner(L,-1);lua_pop(L,1);break;
 }
 if(compiled==graph_no_owner||graph_compiled_sources[compiled].context!=child.name)
  throw ScriptError("Generic include caller is not retained compiled child/helper");
 return true;
}
static int compile_generic_child_include(lua_State* L){
 auto& self=owner(L);
 if(lua_gettop(L)!=3)return luaL_error(L,"Include compiler argument count differs");
 std::size_t cn=0,pn=0;const char* context=luaL_checklstring(L,1,&cn),*path=luaL_checklstring(L,2,&pn);
 luaL_checktype(L,3,LUA_TTABLE);const void* env=lua_topointer(L,3);
 self.source_exception={};self.source_error=nullptr;bool qualified=false;
 try{qualified=self.generic_child_qualify_compiler(L,context,cn,path,pn,env);}
 catch(...){self.source_exception=std::current_exception();self.source_error="Generic child include compiler provenance refused";}
 if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}
 if(!qualified)return compile_include_source_body(L);
 lua_pushcfunction(L,compile_include_source_body);lua_insert(L,1);
 int status=0;
 try{
  {GenericChildIncludeCompilerScope scope(self,L,context,cn,path,pn,env);
   status=lua_pcall(L,3,LUA_MULTRET,0);
   if(!status&&scope.succeeded&&!scope.current())throw ScriptError("Generic compiler return retired before publication");
  }
 }catch(...){self.record_failure();return luaL_error(L,"Generic child compiler lender refused");}
 if(status)return lua_error(L);
 return lua_gettop(L);
}
static int load_generic_child_context(lua_State* L){
 auto& self=owner(L);const int count=lua_gettop(L);
 lua_pushvalue(L,lua_upvalueindex(1));lua_pushvalue(L,lua_upvalueindex(2));
 lua_pushcclosure(L,load_new_context_body,2);lua_insert(L,1);
 int status=0;
 try{
  {GenericChildLoaderScope lender(self,L);
   status=lua_pcall(L,count,1,0);lender.returned=(status==0);
  }
 }catch(...){self.record_failure();return luaL_error(L,"Generic actual child load lender refused");}
 if(status)return lua_error(L);
 return 1;
}
