// Inside actual Host::Impl AFTER Controls definitions. This is native
// Context664FB0/6768B0, not the generic ContextPtr/flat control model.
struct NativeContextCDataCreator final {
 const Impl*const host;
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true};
 const char*const type_name;
 void(*const push_metatype)(lua_State*);
 explicit NativeContextCDataCreator(const Impl* actual);
};
void initialize_native_context_cdata_creator_owned(){
 if(native_context_cdata_creator)throw ScriptError("Context static type definition replay");
 // Native476380 sets CA823C=43F610; actual static9BAE20->40A100 supplies
 // A35988=LuaContext. Only this executed Host constructor publishes our private
 // equivalent creator control. No method counts/zero548/native pool IDs are
 // claimed from that historical constructor; full registrar output is separate.
 native_context_cdata_creator=std::make_shared<NativeContextCDataCreator>(this);
}
struct NativeChildContextControlsResult final {
 const std::weak_ptr<Impl> host;
 const std::weak_ptr<const ui::detail::NotificationContextChildControlsExportLoan> operand;
 const std::shared_ptr<const NativeContextCDataCreator> cdata_creator;
 const std::shared_ptr<const ui::detail::NotificationContextLuaControlsNamedReference> parent;
 const std::weak_ptr<ui::detail::NotificationLayoutContextOrdinaryState> child;
 const std::weak_ptr<ui::BnwWorldViewControlTree> child_tree;
 const std::shared_ptr<const ScriptUiChildSource> child_source;
 const std::shared_ptr<const ScriptUiChildSource> parent_source;
 const std::shared_ptr<const ui::detail::NotificationLayoutOrdinaryReceiver> receiver;
 const std::thread::id creator=std::this_thread::get_id();
 const BnwWorldViewSourceOperationLoan*const operation;
 const std::size_t slot;
 const std::uint64_t serial,epoch,writes,uncertainty;
 const void*const actual_environment;
 lua_State*const actual_thread;
 const int environment,thread;
 const void*const child_cdata;
 std::array<char,32> id{};
 const std::string screen;
 std::vector<char> duplicate_diagnostic;
 enum class Stage {captured,keys,table,assigned,returned};
 Stage stage=Stage::captured;
 int keys_ref=LUA_NOREF,table_ref=LUA_NOREF,metatable_ref=LUA_NOREF,types_ref=LUA_NOREF;
 const void*keys=nullptr,*table=nullptr,*metatable=nullptr,*types=nullptr;
 int named_reference=LUA_NOREF;const void*parent_table=nullptr;
 NativeChildContextControlsResult(std::weak_ptr<Impl> h,
  std::weak_ptr<const ui::detail::NotificationContextChildControlsExportLoan> loan,
  std::shared_ptr<const NativeContextCDataCreator> type,
  std::shared_ptr<const ui::detail::NotificationContextLuaControlsNamedReference> controls,
  std::weak_ptr<ui::detail::NotificationLayoutContextOrdinaryState> physical,
  std::weak_ptr<ui::BnwWorldViewControlTree> tree,std::shared_ptr<const ScriptUiChildSource> source,
  std::shared_ptr<const ScriptUiChildSource> parent_src,
  std::shared_ptr<const ui::detail::NotificationLayoutOrdinaryReceiver> actual_receiver,
  const BnwWorldViewSourceOperationLoan*op,std::size_t child_slot,std::uint64_t q,std::uint64_t e,
  std::uint64_t w,std::uint64_t u,const void*env,lua_State*thr,int envref,int threadref,const void*cdata,
  std::array<char,32> actual_id,std::string actual_screen,int named,const void*parent_pointer)
  :host(std::move(h)),operand(std::move(loan)),cdata_creator(std::move(type)),parent(std::move(controls)),
   child(std::move(physical)),child_tree(std::move(tree)),child_source(std::move(source)),
   parent_source(std::move(parent_src)),receiver(std::move(actual_receiver)),operation(op),slot(child_slot),
   serial(q),epoch(e),writes(w),uncertainty(u),actual_environment(env),actual_thread(thr),environment(envref),thread(threadref),
   child_cdata(cdata),id(actual_id),screen(std::move(actual_screen)),
   duplicate_diagnostic(sizeof("Duplicate control ID tag [] in screen [].")+std::strlen(id.data())+screen.size()),
   named_reference(named),parent_table(parent_pointer){}
};
struct NativeChildContextControlsFrame final {
 Impl& host;const LuaScriptHost& facade;
 const BnwWorldViewSourceOperationLoan& operation;
 const std::shared_ptr<const ui::detail::NotificationContextChildControlsExportLoan> operand;
 const ui::detail::NotificationContextChildControlsExportLoan::Guard operand_guard;
 const std::shared_ptr<NativeChildContextControlsResult> result;
 const std::shared_ptr<ui::detail::NotificationLayoutContextOrdinaryState> child;
 const std::shared_ptr<ui::BnwWorldViewControlTree> child_tree;
 const std::thread::id creator=std::this_thread::get_id();
 NativeChildContextControlsFrame(Impl& p,const LuaScriptHost& f,const BnwWorldViewSourceOperationLoan& op,
  std::shared_ptr<const ui::detail::NotificationContextChildControlsExportLoan> loan,
  ui::detail::NotificationContextChildControlsExportLoan::Guard g,std::shared_ptr<NativeChildContextControlsResult> r,
  std::shared_ptr<ui::detail::NotificationLayoutContextOrdinaryState> physical,std::shared_ptr<ui::BnwWorldViewControlTree> tree)
  :host(p),facade(f),operation(op),operand(std::move(loan)),operand_guard(std::move(g)),result(std::move(r)),
   child(std::move(physical)),child_tree(std::move(tree)){
  if(host.child_controls_export_frame)throw ScriptError("Context child Controls export reentered");
  host.child_controls_export_frame=this;
 }
 ~NativeChildContextControlsFrame(){if(host.child_controls_export_frame==this)host.child_controls_export_frame=nullptr;}
 void first()const {
  operand_guard.require_current_without_callbacks(operation); // real Root Source FIRST before old raw op
  host.worldview_require_callback_origin();
  if(host.child_controls_export_frame!=this||creator!=std::this_thread::get_id()||!result||!host.worldview_native_host||
   host.worldview_native_host->loan!=&operation||host.worldview_native_host->child!=result->slot||
   result->slot>=host.child_contexts.size()||result->operation!=&operation||result->creator!=creator||
   result->serial!=operation.operation_serial()||result->epoch!=host.failure_epoch||result->writes!=host.failed.writes_||
   result->uncertainty!=host.execution_uncertainty||result->child.lock()!=child||result->child_tree.lock()!=child_tree||
   result->operand.lock()!=operand||host.native_context_cdata_creator!=result->cdata_creator||
   !result->cdata_creator||result->cdata_creator->host!=&host||!result->cdata_creator->alive.load()||
   result->cdata_creator->creator!=creator||std::strcmp(result->cdata_creator->type_name,"LuaContext")||
   result->cdata_creator->push_metatype!=&Impl::native_child_context_push_existing_metatable||
   operand_guard.parent_controls_without_callbacks()!=result->parent||
   operand_guard.child_source_without_callbacks()!=result->child_source||
   operand_guard.child_receiver_without_callbacks()!=result->receiver||
   operand_guard.child_context_without_callbacks()!=child||operand_guard.child_tree_without_callbacks()!=child_tree||
   result->child_cdata!=child.get()||std::strcmp(operand_guard.terminated_id_without_callbacks(),result->id.data()))
   throw ScriptError("Context child Controls SAME frame/VM/creator/physical child changed");
  const auto& current=host.child_contexts[result->slot];
  if(current.retired||!current.state_prepared||current.environment!=result->environment||
   current.execution_thread!=result->thread||current.controls_table_pointer!=result->parent_table||
   current.source!=result->parent_source||host.worldview_native_host->child_source.lock()!=result->parent_source)
   throw ScriptError("Context child Controls actual parent518/table changed");
 }
 void full()const {first();operand_guard.require_source_owned(operation);first();
  operation.require_same_vm(facade);first();}
};
static void native_child_context_export_full(lua_State* L){
 auto& p=owner(L);const auto* frame=p.child_controls_export_frame;p.source_exception={};p.source_error=nullptr;
 try{if(!frame)throw ScriptError("Context child export actual frame absent");frame->full();}
 catch(...){p.source_exception=std::current_exception();p.source_error="Context child export Source FULL refused";}
 if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}
static void native_child_context_export_keys(lua_State* L){
 auto& p=owner(L);native_child_context_export_full(L);auto* r=p.child_controls_export_frame->result.get();
 if(r->stage!=NativeChildContextControlsResult::Stage::captured||r->keys_ref!=LUA_NOREF)
  return (void)luaL_error(L,"Context child export retained-key stage/replay");
 if(!lua_checkstack(L,32))return (void)luaL_error(L,"Context child export stack reserve failed");
 native_child_context_export_full(L);lua_createtable(L,4,0);native_child_context_export_full(L);
 const int keys=lua_gettop(L);
 const char*values[]={r->id.data(),"CData","ExposedTypes",r->cdata_creator->type_name};
 for(int i=0;i<4;++i){lua_pushstring(L,values[i]);native_child_context_export_full(L);
  lua_rawseti(L,keys,i+1);native_child_context_export_full(L);}
 r->keys=lua_topointer(L,keys);lua_pushvalue(L,keys);native_child_context_export_full(L);
 r->keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_child_context_export_full(L);lua_pop(L,1);
 if(!r->keys||r->keys_ref==LUA_NOREF||r->keys_ref==LUA_REFNIL)
  return (void)luaL_error(L,"Context child export exact keys not retained");
 r->stage=NativeChildContextControlsResult::Stage::keys;
}
static void native_child_context_push_existing_metatable(lua_State* L){
 auto& p=owner(L);native_child_context_export_full(L);auto* r=p.child_controls_export_frame->result.get();
 // Executed equivalent43F610 creator uses authentic Context definition from
 // actual static type ctor (LuaContext) and SAME parent VM. Original615D70 cache-MISS and
 //517C20 generic constructor need their real helpers; no empty fallback.
 lua_getfield(L,LUA_GLOBALSINDEX,"ExposedTypes");native_child_context_export_full(L);
 if(!lua_istable(L,-1))return (void)luaL_error(L,"required original615D70 ExposedTypes cache constructor/type resolver before Context43F610");
 r->types=lua_topointer(L,-1);lua_pushvalue(L,-1);native_child_context_export_full(L);
 r->types_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_child_context_export_full(L);
 lua_getfield(L,-1,r->cdata_creator->type_name);native_child_context_export_full(L);
 if(!lua_istable(L,-1))return (void)luaL_error(L,"required original517C20 Context metatype constructor and registered native definitions after43F610 cache-MISS");
 r->metatable=lua_topointer(L,-1);lua_pushvalue(L,-1);native_child_context_export_full(L);
 r->metatable_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_child_context_export_full(L);
 // Original43F66E removes the ExposedTypes cache and leaves actual type table.
 lua_remove(L,-2);native_child_context_export_full(L);
}
static bool native_child_context_export_physical_without_callbacks(Impl& p,const NativeChildContextControlsResult& r){
 // Earlier protected job established stack highwater32 and retained TString
 // keys. This cut uses no intern/lua_pushstring/checkGC/provider/luaL_error.
 if(!p.state||r.creator!=std::this_thread::get_id()||r.keys_ref==LUA_NOREF||!r.keys||
  r.table_ref==LUA_NOREF||!r.table||r.metatable_ref==LUA_NOREF||!r.metatable||r.types_ref==LUA_NOREF||!r.types||
  (r.stage!=NativeChildContextControlsResult::Stage::assigned&&r.stage!=NativeChildContextControlsResult::Stage::returned))
  return false;
 lua_State* L=p.state;const int top=lua_gettop(L);bool good=true;
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.thread);good=lua_type(L,-1)==LUA_TTHREAD&&lua_tothread(L,-1)==r.actual_thread&&r.actual_thread!=L;
 if(!good){lua_settop(L,top);return false;}
 lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,r.environment);
 good=good&&lua_istable(L,-1)&&lua_topointer(L,-1)==r.actual_environment&&lua_rawequal(L,-1,-2);lua_pop(L,3);
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.keys_ref);const int keys=lua_gettop(L);
 good=good&&lua_istable(L,keys)&&lua_topointer(L,keys)==r.keys&&lua_objlen(L,keys)==4;
 if(!good){lua_settop(L,top);return false;}
 // Private keeper must still contain genuine retained TString keys.
 // Refuse nonstrings before lua_tolstring; this cut never coerces/allocates.
 const char*values[]={r.id.data(),"CData","ExposedTypes",r.cdata_creator->type_name};
 for(int i=0;i<4;++i){lua_rawgeti(L,keys,i+1);
  if(lua_type(L,-1)!=LUA_TSTRING){lua_settop(L,top);return false;}
  std::size_t length=0;const char*actual=lua_tolstring(L,-1,&length);
  const std::size_t expected=std::strlen(values[i]);
  good=actual&&length==expected&&std::memcmp(actual,values[i],expected)==0;lua_pop(L,1);
  if(!good){lua_settop(L,top);return false;}}
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.named_reference);const int parent=lua_gettop(L);
 good=lua_istable(L,parent)&&lua_topointer(L,parent)==r.parent_table;
 if(!good){lua_settop(L,top);return false;}
 lua_rawgeti(L,keys,1);lua_rawget(L,parent);lua_rawgeti(L,LUA_REGISTRYINDEX,r.table_ref);
 good=lua_istable(L,-1)&&lua_topointer(L,-1)==r.table&&lua_rawequal(L,-1,-2);const int table=lua_gettop(L);
 if(!good){lua_settop(L,top);return false;}
 lua_rawgeti(L,keys,2);lua_rawget(L,table);good=lua_type(L,-1)==LUA_TLIGHTUSERDATA&&lua_touserdata(L,-1)==r.child_cdata;lua_pop(L,1);
 if(!lua_getmetatable(L,table)){lua_settop(L,top);return false;}
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.metatable_ref);good=good&&lua_topointer(L,-1)==r.metatable&&lua_rawequal(L,-1,-2);lua_pop(L,2);
 lua_rawgeti(L,keys,3);lua_rawget(L,LUA_GLOBALSINDEX);lua_rawgeti(L,LUA_REGISTRYINDEX,r.types_ref);
 good=good&&lua_istable(L,-1)&&lua_topointer(L,-1)==r.types&&lua_rawequal(L,-1,-2);const int types=lua_gettop(L);
 if(!good){lua_settop(L,top);return false;}
 lua_rawgeti(L,keys,4);lua_rawget(L,types);lua_rawgeti(L,LUA_REGISTRYINDEX,r.metatable_ref);
 good=lua_istable(L,-1)&&lua_topointer(L,-1)==r.metatable&&lua_rawequal(L,-1,-2);lua_pop(L,2);
 // Cache table can be native-script replaced in original Lua; only its SAME
 // actual output is pinned. No generic ContextPtr table is accepted here.
 lua_settop(L,top);return good;
}
static void perform_native_child_context_controls_export(lua_State* L,Job& job){
 auto& p=owner(L);native_child_context_export_full(L);auto* r=p.child_controls_export_frame->result.get();
 if(job.worldview_child!=r->slot||job.operation!=Operation::native_child_context_controls_export)
  return (void)luaL_error(L,"Context child export wrong actual parent job");
 native_child_context_export_keys(L);native_child_context_export_full(L);
 // Actual664FB0 namedRef ID lookup/nil check/temp reference DTOR. Original
 // duplicate diagnostic formats a string and continues with assignment.
 lua_rawgeti(L,LUA_REGISTRYINDEX,r->named_reference);native_child_context_export_full(L);
 if(!lua_istable(L,-1)||lua_topointer(L,-1)!=r->parent_table)
  return (void)luaL_error(L,"Context child export actual parent Controls reference changed");
 lua_getfield(L,-1,r->id.data());native_child_context_export_full(L);
 const bool duplicate=!lua_isnil(L,-1);
 const int lookup=luaL_ref(L,LUA_REGISTRYINDEX);native_child_context_export_full(L);
 luaL_unref(L,LUA_REGISTRYINDEX,lookup);native_child_context_export_full(L);
 if(duplicate){
  const int size=std::snprintf(r->duplicate_diagnostic.data(),r->duplicate_diagnostic.size(),
   "Duplicate control ID tag [%s] in screen [%s].",r->id.data(),r->screen.c_str());
  if(size<0||static_cast<std::size_t>(size)>=r->duplicate_diagnostic.size())
   return (void)luaL_error(L,"Context duplicate native fixed-buffer diagnostic profile unsupported");
  native_child_context_export_full(L);
 }
 // SAME parent Controls is on stack. Actual6768B0 creates a fresh table,
 // CData lightuserdata to genuine child owner, actual registered metatype.
 lua_createtable(L,0,0);native_child_context_export_full(L);r->table=lua_topointer(L,-1);
 lua_pushvalue(L,-1);native_child_context_export_full(L);r->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_child_context_export_full(L);
 lua_pushlightuserdata(L,const_cast<void*>(r->child_cdata));native_child_context_export_full(L);
 lua_setfield(L,-2,"CData");native_child_context_export_full(L);r->stage=NativeChildContextControlsResult::Stage::table;
 r->cdata_creator->push_metatype(L);native_child_context_export_full(L);
 if(!lua_setmetatable(L,-2))return (void)luaL_error(L,"Context actual metatable assignment refused");
 native_child_context_export_full(L);lua_setfield(L,-2,r->id.data());native_child_context_export_full(L);
 lua_pop(L,1);r->stage=NativeChildContextControlsResult::Stage::assigned;
 native_child_context_export_full(L);
 if(!native_child_context_export_physical_without_callbacks(p,*r))
  return (void)luaL_error(L,"Context child Controls actual table/CData/metatype final physical POST failed");
}
