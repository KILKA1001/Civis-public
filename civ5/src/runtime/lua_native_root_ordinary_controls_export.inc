// Inside the actual Host::Impl. The six actual native subtype creators are
// separate records. AuthoredControl userdata and LuaContext are not suppliers.
using OrdinarySubtype=ui::detail::NotificationContextRootOrdinaryControlsExportLoan::NativeSubtype;
struct NativeOrdinaryCDataCreator final {
 const Impl*const host;
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true};
 const OrdinarySubtype subtype;
 const char*const type_name;
 void(*const push_metatype)(lua_State*);
 NativeOrdinaryCDataCreator(const Impl* h,OrdinarySubtype k,const char* name,void(*callback)(lua_State*))
  :host(h),subtype(k),type_name(name),push_metatype(callback){}
};
void initialize_native_ordinary_cdata_creators_owned(){
 // Executable portable static registration. Exact six native writers and
 // subsequent type-name assignments are freshly physically qualified in38.
 // This does not issue unimplemented native method lists or master tables.
 for(const auto& old:native_ordinary_cdata_creators)if(old)throw ScriptError("ordinary static CData definition replay");
 const char*const names[]={"ImageControl","ColorBoxControl","ButtonControl","TextControl","StackControl","GridControl"};
 void(*const creators[])(lua_State*)={&Impl::native_ordinary_image_metatype,&Impl::native_ordinary_box_metatype,
  &Impl::native_ordinary_button_metatype,&Impl::native_ordinary_label_metatype,&Impl::native_ordinary_stack_metatype,&Impl::native_ordinary_grid_metatype};
 for(std::size_t i=0;i<6;++i)native_ordinary_cdata_creators[i]=std::make_shared<NativeOrdinaryCDataCreator>(
  this,static_cast<OrdinarySubtype>(i),names[i],creators[i]);
}

struct NativeRootOrdinaryControlsResult final {
 const std::weak_ptr<Impl> host;
 const std::weak_ptr<const ui::detail::NotificationContextRootOrdinaryControlsExportLoan> operand;
 const std::shared_ptr<const NativeOrdinaryCDataCreator> cdata_creator;
 std::shared_ptr<const NativeOrdinaryMetatypeResult> metatype_owner;
 const std::shared_ptr<const ui::detail::NotificationContextLuaControlsNamedReference> parent;
 const std::shared_ptr<const void> concrete;
 const std::weak_ptr<ui::BnwWorldViewControlTree> tree;
 const std::shared_ptr<const ScriptUiChildSource> parent_source;
 const std::shared_ptr<const ui::detail::NotificationLayoutOrdinaryReceiver> receiver;
 const std::thread::id creator=std::this_thread::get_id();
 const BnwWorldViewSourceOperationLoan*const operation;
 const std::size_t slot;
 const std::uint64_t serial,epoch,writes,uncertainty;
 const void*const actual_environment;
 lua_State*const actual_thread;
 const int environment,thread;
 const void*const cdata;
 const std::array<char,32> id;
 const std::string screen;
 std::vector<char> duplicate_diagnostic;
 enum class Stage {captured,keys,table,assigned,returned};
 Stage stage=Stage::captured;
 int keys_ref=LUA_NOREF,table_ref=LUA_NOREF,metatable_ref=LUA_NOREF,types_ref=LUA_NOREF;
 const void*keys=nullptr,*table=nullptr,*metatable=nullptr,*types=nullptr;
 const int named_reference;
 const void*const parent_table;
 NativeRootOrdinaryControlsResult(std::weak_ptr<Impl> h,
  std::weak_ptr<const ui::detail::NotificationContextRootOrdinaryControlsExportLoan> loan,
  std::shared_ptr<const NativeOrdinaryCDataCreator> type,
  std::shared_ptr<const ui::detail::NotificationContextLuaControlsNamedReference> controls,
  std::shared_ptr<const void> physical,std::weak_ptr<ui::BnwWorldViewControlTree> actual_tree,
  std::shared_ptr<const ScriptUiChildSource> parent_src,
  std::shared_ptr<const ui::detail::NotificationLayoutOrdinaryReceiver> actual_receiver,
  const BnwWorldViewSourceOperationLoan*op,std::size_t child_slot,std::uint64_t q,std::uint64_t e,
  std::uint64_t w,std::uint64_t u,const void*env,lua_State*thr,int envref,int threadref,
  std::array<char,32> actual_id,std::string actual_screen,int named,const void*parent_pointer)
  :host(std::move(h)),operand(std::move(loan)),cdata_creator(std::move(type)),parent(std::move(controls)),
   concrete(std::move(physical)),tree(std::move(actual_tree)),parent_source(std::move(parent_src)),
   receiver(std::move(actual_receiver)),operation(op),slot(child_slot),serial(q),epoch(e),writes(w),uncertainty(u),
   actual_environment(env),actual_thread(thr),environment(envref),thread(threadref),cdata(concrete.get()),
   id(actual_id),screen(std::move(actual_screen)),
   duplicate_diagnostic(sizeof("Duplicate control ID tag [] in screen [].")+std::strlen(id.data())+screen.size()),
   named_reference(named),parent_table(parent_pointer){}
};

struct NativeRootOrdinaryControlsFrame final {
 Impl& host;const LuaScriptHost& facade;
 const BnwWorldViewSourceOperationLoan& operation;
 const std::shared_ptr<const ui::detail::NotificationContextRootOrdinaryControlsExportLoan> operand;
 const ui::detail::NotificationContextRootOrdinaryControlsExportLoan::Guard operand_guard;
 const std::shared_ptr<NativeRootOrdinaryControlsResult> result;
 const std::shared_ptr<const void> concrete;
 const std::shared_ptr<ui::BnwWorldViewControlTree> tree;
 const std::thread::id creator=std::this_thread::get_id();
 NativeRootOrdinaryControlsFrame(Impl& p,const LuaScriptHost& f,const BnwWorldViewSourceOperationLoan& op,
  std::shared_ptr<const ui::detail::NotificationContextRootOrdinaryControlsExportLoan> loan,
  ui::detail::NotificationContextRootOrdinaryControlsExportLoan::Guard g,std::shared_ptr<NativeRootOrdinaryControlsResult> r,
  std::shared_ptr<const void> physical,std::shared_ptr<ui::BnwWorldViewControlTree> actual_tree)
  :host(p),facade(f),operation(op),operand(std::move(loan)),operand_guard(std::move(g)),result(std::move(r)),
   concrete(std::move(physical)),tree(std::move(actual_tree)){
  if(host.root_ordinary_controls_export_frame)throw ScriptError("ordinary Controls export reentered");
  host.root_ordinary_controls_export_frame=this;
 }
 ~NativeRootOrdinaryControlsFrame(){if(host.root_ordinary_controls_export_frame==this)host.root_ordinary_controls_export_frame=nullptr;}
 void first()const {
  operand_guard.require_current_without_callbacks(operation);
  host.worldview_require_callback_origin();
  if(!result||!result->cdata_creator)throw ScriptError("ordinary Controls actual result/creator absent");
  const auto index=static_cast<std::size_t>(result->cdata_creator->subtype);
  if(host.root_ordinary_controls_export_frame!=this||creator!=std::this_thread::get_id()||!result||
   !host.worldview_native_host||host.worldview_native_host->loan!=&operation||
   host.worldview_native_host->child!=result->slot||result->slot>=host.child_contexts.size()||
   result->operation!=&operation||result->creator!=creator||result->serial!=operation.operation_serial()||
   result->epoch!=host.failure_epoch||result->writes!=host.failed.writes_||result->uncertainty!=host.execution_uncertainty||
   result->concrete!=concrete||result->tree.lock()!=tree||result->operand.lock()!=operand||index>=6||
   host.native_ordinary_cdata_creators[index]!=result->cdata_creator||!result->cdata_creator||
   result->cdata_creator->host!=&host||!result->cdata_creator->alive.load()||result->cdata_creator->creator!=creator||
   operand_guard.subtype_without_callbacks()!=result->cdata_creator->subtype||
   operand_guard.parent_controls_without_callbacks()!=result->parent||
   operand_guard.receiver_without_callbacks()!=result->receiver||
   operand_guard.concrete_cdata_owner_without_callbacks()!=concrete||operand_guard.tree_without_callbacks()!=tree||
   result->cdata!=concrete.get()||std::strcmp(operand_guard.terminated_id_without_callbacks(),result->id.data()))
   throw ScriptError("ordinary Controls SAME native frame/VM/subtype/physical CData changed");
  const auto& current=host.child_contexts[result->slot];
  if(current.retired||!current.state_prepared||current.environment!=result->environment||
   current.execution_thread!=result->thread||current.controls_table_pointer!=result->parent_table||
   current.source!=result->parent_source||host.worldview_native_host->child_source.lock()!=result->parent_source)
   throw ScriptError("ordinary Controls actual parent518/reference changed");
 }
 void full()const {first();operand_guard.require_source_owned(operation);first();operation.require_same_vm(facade);first();}
};
static void native_ordinary_export_full(lua_State* L){
 auto& p=owner(L);const auto* frame=p.root_ordinary_controls_export_frame;p.source_exception={};p.source_error=nullptr;
 try{if(!frame)throw ScriptError("ordinary export actual frame absent");frame->full();}
 catch(...){p.source_exception=std::current_exception();p.source_error="ordinary export Source FULL refused";}
 if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}

static void native_ordinary_export_keys(lua_State* L){
 auto& p=owner(L);native_ordinary_export_full(L);auto* r=p.root_ordinary_controls_export_frame->result.get();
 if(r->stage!=NativeRootOrdinaryControlsResult::Stage::captured||r->keys_ref!=LUA_NOREF)
  return (void)luaL_error(L,"ordinary control export retained-key stage/replay");
 if(!lua_checkstack(L,32))return (void)luaL_error(L,"ordinary control export stack reserve failed");
 native_ordinary_export_full(L);lua_createtable(L,4,0);native_ordinary_export_full(L);
 const int keys=lua_gettop(L);
 const char*values[]={r->id.data(),"CData","ExposedTypes",r->cdata_creator->type_name};
 for(int i=0;i<4;++i){lua_pushstring(L,values[i]);native_ordinary_export_full(L);
  lua_rawseti(L,keys,i+1);native_ordinary_export_full(L);}
 r->keys=lua_topointer(L,keys);lua_pushvalue(L,keys);native_ordinary_export_full(L);
 r->keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_ordinary_export_full(L);lua_pop(L,1);
 if(!r->keys||r->keys_ref==LUA_NOREF||r->keys_ref==LUA_REFNIL)
  return (void)luaL_error(L,"ordinary control export exact keys not retained");
 r->stage=NativeRootOrdinaryControlsResult::Stage::keys;
}

static void native_ordinary_push_existing_metatype(lua_State* L,OrdinarySubtype offered){
 auto& p=owner(L);native_ordinary_export_full(L);
 if(p.root_ordinary_controls_export_frame->result->cdata_creator->subtype!=offered)
  return (void)luaL_error(L,"ordinary subtype callback mismatch");
 native_ordinary_push_metatype_owned(L,offered);native_ordinary_export_full(L);
}
static void native_ordinary_image_metatype(lua_State* L){native_ordinary_push_existing_metatype(L,OrdinarySubtype::image);}
static void native_ordinary_box_metatype(lua_State* L){native_ordinary_push_existing_metatype(L,OrdinarySubtype::box);}
static void native_ordinary_button_metatype(lua_State* L){native_ordinary_push_existing_metatype(L,OrdinarySubtype::button);}
static void native_ordinary_label_metatype(lua_State* L){native_ordinary_push_existing_metatype(L,OrdinarySubtype::label);}
static void native_ordinary_stack_metatype(lua_State* L){native_ordinary_push_existing_metatype(L,OrdinarySubtype::stack);}
static void native_ordinary_grid_metatype(lua_State* L){native_ordinary_push_existing_metatype(L,OrdinarySubtype::grid);}

static bool native_ordinary_export_physical_without_callbacks(Impl& p,const NativeRootOrdinaryControlsResult& r){
 // Earlier protected job established stack highwater32 and retained TString
 // keys. This cut uses no intern/lua_pushstring/checkGC/provider/luaL_error.
 if(!r.metatype_owner||!native_ordinary_type_physical_without_callbacks(p,*r.metatype_owner,r))return false;
 if(!p.state||r.creator!=std::this_thread::get_id()||r.keys_ref==LUA_NOREF||!r.keys||
  r.table_ref==LUA_NOREF||!r.table||r.metatable_ref==LUA_NOREF||!r.metatable||r.types_ref==LUA_NOREF||!r.types||
  (r.stage!=NativeRootOrdinaryControlsResult::Stage::assigned&&r.stage!=NativeRootOrdinaryControlsResult::Stage::returned))
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
 lua_rawgeti(L,keys,2);lua_rawget(L,table);good=lua_type(L,-1)==LUA_TLIGHTUSERDATA&&lua_touserdata(L,-1)==r.cdata;lua_pop(L,1);
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

static void perform_native_root_ordinary_controls_export(lua_State* L,Job& job){
 auto& p=owner(L);native_ordinary_export_full(L);auto* r=p.root_ordinary_controls_export_frame->result.get();
 if(job.worldview_child!=r->slot||job.operation!=Operation::native_root_ordinary_controls_export)
  return (void)luaL_error(L,"ordinary control export wrong actual parent job");
 native_ordinary_export_keys(L);native_ordinary_export_full(L);
 // Actual664FB0 namedRef ID lookup/nil check/temp reference DTOR. Original
 // duplicate diagnostic formats a string and continues with assignment.
 lua_rawgeti(L,LUA_REGISTRYINDEX,r->named_reference);native_ordinary_export_full(L);
 if(!lua_istable(L,-1)||lua_topointer(L,-1)!=r->parent_table)
  return (void)luaL_error(L,"ordinary control export actual parent Controls reference changed");
 lua_getfield(L,-1,r->id.data());native_ordinary_export_full(L);
 const bool duplicate=!lua_isnil(L,-1);
 const int lookup=luaL_ref(L,LUA_REGISTRYINDEX);native_ordinary_export_full(L);
 luaL_unref(L,LUA_REGISTRYINDEX,lookup);native_ordinary_export_full(L);
 if(duplicate){
  const int size=std::snprintf(r->duplicate_diagnostic.data(),r->duplicate_diagnostic.size(),
   "Duplicate control ID tag [%s] in screen [%s].",r->id.data(),r->screen.c_str());
  if(size<0||static_cast<std::size_t>(size)>=r->duplicate_diagnostic.size())
   return (void)luaL_error(L,"Context duplicate native fixed-buffer diagnostic profile unsupported");
  native_ordinary_export_full(L);
 }
 // SAME parent Controls is on stack. Actual6768B0 creates a fresh table,
 // CData lightuserdata to genuine concrete ordinary state, actual typed metatype.
 lua_createtable(L,0,0);native_ordinary_export_full(L);r->table=lua_topointer(L,-1);
 lua_pushvalue(L,-1);native_ordinary_export_full(L);r->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_ordinary_export_full(L);
 lua_pushlightuserdata(L,const_cast<void*>(r->cdata));native_ordinary_export_full(L);
 lua_setfield(L,-2,"CData");native_ordinary_export_full(L);r->stage=NativeRootOrdinaryControlsResult::Stage::table;
 // Native recovered conditional CALL EAX uses the ACTUALLY executed static creator.
 // Every selected ordinary writer has a nonNULL genuine callback; no missing
 // Civis binding is interpreted as the original NULL branch.
 if(!r->cdata_creator->push_metatype)return (void)luaL_error(L,"ordinary native static callback not issued");
 r->cdata_creator->push_metatype(L);native_ordinary_export_full(L);
 if(!lua_setmetatable(L,-2))return (void)luaL_error(L,"ordinary actual metatable assignment refused");
 native_ordinary_export_full(L);lua_setfield(L,-2,r->id.data());native_ordinary_export_full(L);
 lua_pop(L,1);r->stage=NativeRootOrdinaryControlsResult::Stage::assigned;
 native_ordinary_export_full(L);
 if(!native_ordinary_export_physical_without_callbacks(p,*r))
  return (void)luaL_error(L,"ordinary control Controls actual table/CData/metatype final physical POST failed");
}
