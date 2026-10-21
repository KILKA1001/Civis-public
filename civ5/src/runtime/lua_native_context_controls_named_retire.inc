// Inside actual Host::Impl after entered Controls lookup. This is the original
// local8F8820 LuaRef destructor. The existing permanent Controls table/ref and
// prepared parent518 remain alive; only the separately earned named ref goes.
struct NativeControlsNamedRetireFrame final {
 using Controls=ui::detail::NotificationContextLuaControlsNamedReference;
 using Prepared=ui::detail::NotificationContextLuaStatePreparation;
 using Setup=ui::detail::NotificationContextLuaLoadSetupReturn;
 enum class Stage {entered,unref_started,unref_applied};
 struct AppliedUnref final {
  const int original_reference;
  const std::optional<lua_Number> before_free_head;
  const int after_free_head;
  AppliedUnref(int reference,std::optional<lua_Number> head):original_reference(reference),before_free_head(head),after_free_head(reference){}
 };
 Impl& host;const LuaScriptHost& facade;const BnwWorldViewSourceOperationLoan& operation;
 const std::shared_ptr<const Prepared> prepared;
 const std::shared_ptr<const Setup> setup;
 const std::shared_ptr<const Controls> named;
 const Controls::Guard before_guard;
 const ui::detail::NotificationContextRootDefaultFieldsReturn::Guard defaults;
 const std::shared_ptr<const ui::detail::NotificationContextRootDefaultFieldsReturn> default_return;
 const std::shared_ptr<const ScriptUiChildSource> source;
 const std::size_t slot;
 const std::uint64_t serial,epoch,writes,uncertainty;
 const int reference;
 Stage stage=Stage::entered;
 std::optional<lua_Number> before_head;
 std::shared_ptr<const AppliedUnref> applied;
 NativeControlsNamedRetireFrame(Impl&p,const LuaScriptHost&f,const BnwWorldViewSourceOperationLoan&op,
  std::shared_ptr<const Prepared> prep,std::shared_ptr<const Setup> completed,std::shared_ptr<const Controls> r,
  Controls::Guard guard,ui::detail::NotificationContextRootDefaultFieldsReturn::Guard root,
  std::shared_ptr<const ui::detail::NotificationContextRootDefaultFieldsReturn> physical_defaults,
  std::shared_ptr<const ScriptUiChildSource> descriptor,std::size_t child,int ref)
  :host(p),facade(f),operation(op),prepared(std::move(prep)),setup(std::move(completed)),named(std::move(r)),
   before_guard(std::move(guard)),defaults(std::move(root)),default_return(std::move(physical_defaults)),source(std::move(descriptor)),slot(child),
   serial(op.operation_serial()),epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty),reference(ref){
  if(host.controls_named_retire_frame)throw ScriptError("actual Controls local LuaRef destructor reentered");
  host.controls_named_retire_frame=this;
 }
 ~NativeControlsNamedRetireFrame(){if(host.controls_named_retire_frame==this)host.controls_named_retire_frame=nullptr;}
 void first()const {
  defaults.require_current_without_callbacks(operation,prepared); // real entered Root FIRST before raw op
  facade.require_context_lua_load_setup_current_without_callbacks(operation,prepared,*setup);
  host.worldview_require_callback_origin();
  if(host.controls_named_retire_frame!=this||!host.worldview_native_host||host.worldview_native_host->loan!=&operation||
   host.worldview_native_host->child!=slot||host.worldview_native_host->child_source.lock()!=source||reference<0||
   serial!=operation.operation_serial()||epoch!=host.failure_epoch||writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty)
   throw ScriptError("actual Controls local LuaRef destructor frame/source/Host changed");
  if(stage==Stage::entered)before_guard.require_current_without_callbacks(operation,prepared,default_return);
  else if(stage!=Stage::unref_applied||!native_controls_named_unref_physical_without_callbacks(host,reference,before_head))
   throw ScriptError("actual Controls local LuaRef unref has not completed its raw registry mutation");
 }
 void full()const {
  first();operation.require_current();first();operation.require_same_vm(facade);first();operation.require_same_source(*source);first();
 }
};
static bool native_controls_named_unref_physical_without_callbacks(Impl&p,int reference,std::optional<lua_Number> old_head){
 if(!p.state||reference<0||p.state->ci->top-p.state->top<4)return false;
 lua_State*L=p.state;const int top=lua_gettop(L);
 lua_rawgeti(L,LUA_REGISTRYINDEX,0);
 const bool head=lua_type(L,-1)==LUA_TNUMBER&&lua_tonumber(L,-1)==lua_Number(reference);lua_pop(L,1);
 lua_rawgeti(L,LUA_REGISTRYINDEX,reference);
 const bool cell=old_head?(lua_type(L,-1)==LUA_TNUMBER&&lua_tonumber(L,-1)==*old_head):lua_isnil(L,-1);
 lua_settop(L,top);return head&&cell;
}
static void native_controls_named_retire_full(lua_State*L){
 auto&p=owner(L);const auto*frame=p.controls_named_retire_frame;p.source_error=nullptr;p.source_exception={};
 try{if(!frame)throw ScriptError("actual local Controls retirement frame missing");frame->full();}
 catch(...){p.source_exception=std::current_exception();p.source_error="actual local Controls retirement FULL refused";}
 if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}
static void perform_native_controls_named_retire(lua_State*L,Job&job){
 auto&p=owner(L);native_controls_named_retire_full(L);auto*frame=p.controls_named_retire_frame;
 if(job.operation!=Operation::native_context_controls_named_retire||job.worldview_child!=frame->slot||
  frame->stage!=NativeControlsNamedRetireFrame::Stage::entered||frame->applied)
  return (void)luaL_error(L,"actual local Controls destructor wrong job/phase");
 if(!lua_checkstack(L,24))return (void)luaL_error(L,"actual local Controls destructor stack reserve failed");
 native_controls_named_retire_full(L);
 lua_rawgeti(L,LUA_REGISTRYINDEX,0);
 if(lua_isnil(L,-1))frame->before_head.reset();
 else if(lua_type(L,-1)==LUA_TNUMBER)frame->before_head=lua_tonumber(L,-1);
 else return (void)luaL_error(L,"actual Lua registry free head invalid");
 lua_pop(L,1);native_controls_named_retire_full(L);
 //8F8838 forwards real existing nonnegative ref to luaL_unref. A longjmp or
 // allocator failure leaves unref_started and can never produce NilReturn.
 frame->stage=NativeControlsNamedRetireFrame::Stage::unref_started;
 luaL_unref(L,LUA_REGISTRYINDEX,frame->reference);
 frame->stage=NativeControlsNamedRetireFrame::Stage::unref_applied;
 native_controls_named_retire_full(L);
 auto returned=std::make_shared<const NativeControlsNamedRetireFrame::AppliedUnref>(frame->reference,frame->before_head);
 native_controls_named_retire_full(L);frame->applied=returned;native_controls_named_retire_full(L);
}
