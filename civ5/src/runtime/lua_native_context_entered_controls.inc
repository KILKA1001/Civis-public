// Inside actual Host::Impl, after Controls/exporter definitions. No native
// setup/registrar producer is added: only authentic existing table lookup.
static bool native_entered_push_field_without_callbacks(lua_State* L,int table,const char* key,std::size_t length){
 lua_pushnil(L);
 while(lua_next(L,table)){
  if(lua_type(L,-2)==LUA_TSTRING){
   std::size_t size=0;const char*bytes=lua_tolstring(L,-2,&size);
   if(bytes&&size==length&&std::memcmp(bytes,key,length)==0){lua_remove(L,-2);return true;}
  }
  lua_pop(L,1);
 }
 return false;
}
static bool native_entered_controls_physical_without_callbacks(Impl& p,std::size_t slot,
 int environment_ref,int thread_ref,const void* expected_environment,lua_State*expected_thread,
 const void*expected_table,int table_ref,std::string_view state_name,int named_ref=LUA_NOREF){
 // No lua_pushliteral/string/intern/checkGC/Source/metamethod/error calls.
 // Native Controls statement/preparation already earned this stack capacity.
 if(!p.state||slot>=p.child_contexts.size()||!expected_environment||!expected_thread||!expected_table||
  table_ref<0||environment_ref<0||thread_ref<0||p.state->ci->top-p.state->top<12)return false;
 const auto& current=p.child_contexts[slot];
 if(current.retired||!current.state_prepared||current.environment!=environment_ref||current.execution_thread!=thread_ref||
  current.controls_table_ref!=table_ref||current.controls_table_pointer!=expected_table)return false;
 lua_State*L=p.state;const int top=lua_gettop(L);
 lua_rawgeti(L,LUA_REGISTRYINDEX,thread_ref);
 if(lua_type(L,-1)!=LUA_TTHREAD||lua_tothread(L,-1)!=expected_thread||expected_thread==L||
  lua_status(expected_thread)||lua_gettop(expected_thread)){lua_settop(L,top);return false;}
 lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,environment_ref);const int environment=lua_gettop(L);
 if(!lua_istable(L,environment)||lua_topointer(L,environment)!=expected_environment||!lua_rawequal(L,-1,-2)||
  lua_getmetatable(L,environment)){lua_settop(L,top);return false;}
 if(!native_entered_push_field_without_callbacks(L,environment,"StateName",9)||lua_type(L,-1)!=LUA_TSTRING){lua_settop(L,top);return false;}
 std::size_t length=0;const char*actual_name=lua_tolstring(L,-1,&length);
 if(!actual_name||length!=state_name.size()||std::memcmp(actual_name,state_name.data(),length)){lua_settop(L,top);return false;}
 lua_pop(L,1);
 if(!native_entered_push_field_without_callbacks(L,environment,"Controls",8)||!lua_istable(L,-1)||lua_topointer(L,-1)!=expected_table){lua_settop(L,top);return false;}
 lua_rawgeti(L,LUA_REGISTRYINDEX,table_ref);bool good=lua_istable(L,-1)&&lua_topointer(L,-1)==expected_table&&lua_rawequal(L,-1,-2);lua_pop(L,1);
 if(named_ref!=LUA_NOREF){lua_rawgeti(L,LUA_REGISTRYINDEX,named_ref);
  good=good&&lua_istable(L,-1)&&lua_topointer(L,-1)==expected_table&&lua_rawequal(L,-1,-2);lua_pop(L,1);}
 lua_settop(L,top);return good;
}
struct NativeEnteredControlsFrame final {
 Impl& host;const LuaScriptHost& facade;const BnwWorldViewSourceOperationLoan& operation;
 const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation> prepared;
 const std::shared_ptr<const ui::detail::NotificationContextLuaLoadSetupReturn> setup;
 const ui::detail::NotificationContextRootDefaultFieldsReturn::Guard defaults;
 const std::shared_ptr<const ScriptUiChildSource> source;
 const std::size_t slot;const std::uint64_t serial,epoch,writes,uncertainty;
 const int environment,thread,table_ref;const void*const actual_environment;lua_State*const actual_thread;
 const void*const table;const std::string_view state_name;
 NativeEnteredControlsFrame(Impl&p,const LuaScriptHost&f,const BnwWorldViewSourceOperationLoan&op,
  std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation> prep,
  std::shared_ptr<const ui::detail::NotificationContextLuaLoadSetupReturn> completed,
  ui::detail::NotificationContextRootDefaultFieldsReturn::Guard root,std::shared_ptr<const ScriptUiChildSource> descriptor,
  std::size_t child,int env,int thr,int ref,const void*envptr,lua_State*threadptr,const void*tableptr,std::string_view name)
  :host(p),facade(f),operation(op),prepared(std::move(prep)),setup(std::move(completed)),defaults(std::move(root)),
   source(std::move(descriptor)),slot(child),serial(op.operation_serial()),epoch(p.failure_epoch),writes(p.failed.writes_),
   uncertainty(p.execution_uncertainty),environment(env),thread(thr),table_ref(ref),actual_environment(envptr),
   actual_thread(threadptr),table(tableptr),state_name(name){
  if(host.entered_controls_frame)throw ScriptError("entered Root Controls lookup reentered");host.entered_controls_frame=this;
 }
 ~NativeEnteredControlsFrame(){if(host.entered_controls_frame==this)host.entered_controls_frame=nullptr;}
 void first()const {
  defaults.require_current_without_callbacks(operation,prepared); // actual Root FIRST before old op
  facade.require_context_lua_load_setup_current_without_callbacks(operation,prepared,*setup);
  host.worldview_require_callback_origin();
  if(host.entered_controls_frame!=this||!host.worldview_native_host||host.worldview_native_host->loan!=&operation||
   host.worldview_native_host->child!=slot||host.worldview_native_host->child_source.lock()!=source||
   serial!=operation.operation_serial()||epoch!=host.failure_epoch||writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty)
   throw ScriptError("entered Root Controls actual setup/frame/source/VM changed");
 }
 void full()const {
  // RootDefaults FULL is outside NativeScope by contract. It runs around
  // this protected job; within it Source FULL is followed by pure Root FIRST.
  first();
  operation.require_current();first();operation.require_same_vm(facade);first();
  operation.require_same_source(*source);first();
 }
};
static void native_entered_controls_full(lua_State*L){
 auto&p=owner(L);const auto*frame=p.entered_controls_frame;p.source_error=nullptr;p.source_exception={};
 try{if(!frame)throw ScriptError("entered Root Controls frame missing");frame->full();}
 catch(...){p.source_exception=std::current_exception();p.source_error="entered Root Controls FULL refused";}
 if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}
static void perform_native_entered_controls_named_lookup(lua_State*L,Job&job){
 auto&p=owner(L);native_entered_controls_full(L);const auto*frame=p.entered_controls_frame;
 if(job.operation!=Operation::native_context_entered_controls_named_lookup||job.worldview_child!=frame->slot)
  return (void)luaL_error(L,"entered Controls wrong actual parent job");
 if(!lua_checkstack(L,24))return (void)luaL_error(L,"entered Controls actual stack reserve failed");
 native_entered_controls_full(L);lua_rawgeti(L,LUA_REGISTRYINDEX,frame->environment);native_entered_controls_full(L);
 if(!lua_istable(L,-1)||lua_topointer(L,-1)!=frame->actual_environment)
  return (void)luaL_error(L,"entered Controls actual parent518 environment changed");
 // Original8F85A0 retains existing Controls value; never creates/reexecutes it.
 lua_getfield(L,-1,"Controls");native_entered_controls_full(L);
 if(!lua_istable(L,-1)||lua_topointer(L,-1)!=frame->table)
  return (void)luaL_error(L,"entered Controls actual field/table changed");
 job.context_controls_named_reference=luaL_ref(L,LUA_REGISTRYINDEX);native_entered_controls_full(L);lua_pop(L,1);
 job.context_controls_table=frame->table;job.context_controls_reference=frame->table_ref;
 job.context_state_environment=frame->actual_environment;job.context_state_thread=frame->actual_thread;
 if(!native_entered_controls_physical_without_callbacks(p,frame->slot,frame->environment,frame->thread,
  frame->actual_environment,frame->actual_thread,frame->table,frame->table_ref,frame->state_name,job.context_controls_named_reference))
  return (void)luaL_error(L,"entered Controls final actual table/518/named ref physical POST changed");
}
