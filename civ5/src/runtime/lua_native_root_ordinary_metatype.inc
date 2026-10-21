// Inside the actual LuaScriptHost::Impl, after ordinary Controls export.
// Original contracts: real615D70; complete517C20 prefix/ordered548; 47D5F0.
// The separate collected-library4E35C0/current method catalog is unissued.
// An actual static CData creator is not a completed method library. The three
// required517C closures and the separate method-library return remain closed
// producers. No error closure, AuthoredControl table, or guessed NULL is installed.
class NativeOrdinary517CFunction final {
 friend struct Impl;
 enum class Function {fn505b40,fn7801b0,fn775dc0};
 const Impl*const host;
 const std::thread::id creator=std::this_thread::get_id();
 mutable std::atomic<bool> alive{true};
 const Function original;
 const std::string field;
 const lua_CFunction body;
 NativeOrdinary517CFunction(const Impl* p,Function f,std::string key,lua_CFunction code)
  :host(p),original(f),field(std::move(key)),body(code){
  if(!host||field.empty()||!body)throw ScriptError("native517C real closure/field owner absent");
 }
};

class NativeOrdinaryTypeDescriptor final {
 friend struct Impl;
 const Impl*const host;
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true};
 const std::shared_ptr<const NativeOrdinaryCDataCreator> native_creator;
 const std::uint32_t original_descriptor,original_writer;
 // These are the three actual writes in every qualified descriptor constructor:
 // [544]=0,[548]=NULL,[54C]=NULL. They are constructor data, never a claim that
 // the later nativeMethodDefs initializer/collector has executed.
 const std::uint32_t constructor_count544=0;
 const lua_CFunction constructor_callback548=nullptr,constructor_callback54c=nullptr;
 NativeOrdinaryTypeDescriptor(const Impl* p,std::shared_ptr<const NativeOrdinaryCDataCreator> type,
  std::uint32_t descriptor,std::uint32_t writer)
  :host(p),native_creator(std::move(type)),original_descriptor(descriptor),original_writer(writer){
  if(!host||!native_creator||native_creator->host!=host||!original_descriptor||!original_writer)
   throw ScriptError("native descriptor actual constructor/name setter lineage absent");
 }
};

class NativeOrdinaryDescriptorPrefixReturn final {
 friend struct Impl;
 enum class Parent {fn6a6920,fn61d770};
 const Impl*const host;
 const std::thread::id creator=std::this_thread::get_id();
 mutable std::atomic<bool> alive{true};
 const std::shared_ptr<const NativeOrdinaryTypeDescriptor> descriptor;
 const Parent parent548;
 const std::uint32_t actual548,actual54c;
 NativeOrdinaryDescriptorPrefixReturn(const Impl* p,std::shared_ptr<const NativeOrdinaryTypeDescriptor> d,
  Parent parent,std::uint32_t callback548,std::uint32_t callback54c)
  :host(p),descriptor(std::move(d)),parent548(parent),actual548(callback548),actual54c(callback54c){
  if(!host||!descriptor||descriptor->host!=host||!actual548||!actual54c)
   throw ScriptError("native descriptor actual nonNULL548/54C initializer return absent");
 }
};

// The two DISTINCT original nested constructors initialize real historical
// zeros and append their own deferred callback. Graph8 proves later49/10 count
// writers, therefore those initial values do not issue current name/count/548.
// Only the published catalog owner can produce this closed current return.
class NativeOrdinaryNestedDescriptorReturn final {
 friend struct Impl;
 enum class Kind {descriptor_cb7c68,descriptor_cb35d8};
 const Impl*const host;
 const std::thread::id creator=std::this_thread::get_id();
 mutable std::atomic<bool> alive{true};
 const Kind kind;
 const std::uint32_t actual_descriptor,actual_ctor;
 const std::string actual_name0;
 NativeOrdinaryNestedDescriptorReturn(const Impl* p,Kind k,std::uint32_t address,
  std::uint32_t constructor,std::string name)
  :host(p),kind(k),actual_descriptor(address),actual_ctor(constructor),actual_name0(std::move(name)){
  if(!host||actual_name0.empty())throw ScriptError("native nested descriptor real constructor/name return absent");
 }
};

class NativeOrdinaryPointerFormatReturn final {
 friend struct Impl;
 const Impl*const host;
 const std::uint32_t literal_address=0x9dfdc0;
 const std::string format;
 // The actual CRT/operand ABI belongs to the qualified formatter producer.
 // A format label alone cannot issue a variadic call or truncate a64-bit pointer.
 int(*const write_actual_pointer)(char*,std::size_t,void*);
 NativeOrdinaryPointerFormatReturn(const Impl* p,std::string actual_literal,
  int(*formatter)(char*,std::size_t,void*))
  :host(p),format(std::move(actual_literal)),write_actual_pointer(formatter){
  if(!host||format.empty()||!write_actual_pointer)
   throw ScriptError("native505B40 genuine9DFDC0/CRT formatter owner absent");
 }
};

class NativeOrdinary548FieldReturn final {
 friend struct Impl;
 const Impl*const host;
 const std::uint32_t literal_address=0x9e5f0c;
 const std::string field;
 NativeOrdinary548FieldReturn(const Impl* p,std::string actual_literal)
  :host(p),field(std::move(actual_literal)){
  if(!host||field.empty())throw ScriptError("native517C real548 field literal9E5F0C absent");
 }
};

// Completed actual5F36D0 table return. Its table starts empty because the real
// helper creates(0,0); this does not claim that the31-member catalog is empty
// or published. Further actual catalog writers remain owned by their caller.
class NativeOrdinaryReflectionResult final {
 friend struct Impl;
 const Impl*const host;
 const std::thread::id creator=std::this_thread::get_id();
 mutable std::atomic<bool> alive{true};
 const std::uint32_t original=0x5f36d0,key_address=0x9e27e4;
 enum class Stage {captured,table_created,global_assigned,retained,returned};
 Stage stage=Stage::captured;
 int table_ref=LUA_NOREF,key_ref=LUA_NOREF;
 const void*table=nullptr;
 explicit NativeOrdinaryReflectionResult(const Impl* p):host(p){
  if(!host)throw ScriptError("native5F36D0 actual Host absent");
 }
};

struct NativeOrdinaryMasterTypesResult final {
 const Impl*const host;
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true};
 enum class Stage {captured,cache_created,global_assigned,metadata_created,slot1_assigned,returned};
 Stage stage=Stage::captured;
 int cache_ref=LUA_NOREF,metadata_ref=LUA_NOREF;
 const void*cache=nullptr,*metadata=nullptr;
 explicit NativeOrdinaryMasterTypesResult(const Impl* p):host(p){}
};

struct NativeOrdinaryMetatypeResult final {
 const Impl*const host;
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true};
 const std::shared_ptr<const NativeOrdinaryTypeDescriptor> descriptor;
 const std::array<std::shared_ptr<const NativeOrdinary517CFunction>,3> functions;
 const std::shared_ptr<NativeOrdinaryMasterTypesResult> master;
 enum class Name {descriptor0,descriptor4};
 const Name name_operand;
 const std::string name;
 enum class Stage {captured,table_created,name_assigned,closures_assigned,metadata_applied,cache_assigned,returned};
 Stage stage=Stage::captured;
 int table_ref=LUA_NOREF;
 const void*table=nullptr;
 NativeOrdinaryMetatypeResult(const Impl* p,std::shared_ptr<const NativeOrdinaryTypeDescriptor> d,
  std::array<std::shared_ptr<const NativeOrdinary517CFunction>,3> f,
  std::shared_ptr<NativeOrdinaryMasterTypesResult> m,Name which,std::string key)
  :host(p),descriptor(std::move(d)),functions(std::move(f)),master(std::move(m)),
   name_operand(which),name(std::move(key)){}
};

static int native_ordinary_pointer_format_sprintf_s_owned(
 char* buffer,std::size_t count,void* actual_pointer){
 // Actual original505B40 calls IAT9C9680: MSVCR90!sprintf_s. Port the
 // proven fixed format/typed void* argument through the Windows x64 CRT,
 // preserving the actual pointer width and native secure-CRT behavior.
 // The caller supplies its real0x20-byte buffer and ignores the return,
 // exactly as the original does. No snprintf/legacy32-bit conversion.
 return ::sprintf_s(buffer,count,": %p",actual_pointer);
}

void initialize_native_ordinary_pointer_formatter_owned(){
 if(native_ordinary_pointer_format)
  throw ScriptError("native505B40 actual CRT pointer formatter bootstrap replay");
 native_ordinary_pointer_format=std::shared_ptr<const NativeOrdinaryPointerFormatReturn>(
  new NativeOrdinaryPointerFormatReturn(this,": %p",&Impl::native_ordinary_pointer_format_sprintf_s_owned));
}

void initialize_native_ordinary_type_descriptors_owned(){
 // Own, executed descriptor constructors and the already executed six genuine
 // type-name setter returns. Method arrays/key+4 buffer writers are separate.
 const std::uint32_t descriptors[]={0xcb4ba0,0xca87d0,0xca9808,0xcb5bc8,0xcaa2e8,0xcb2580};
 const std::uint32_t writers[]={0x79fd50,0x5d11c0,0x609e80,0x5d5010,0x6cc680,0x74b1d0};
 for(std::size_t i=0;i<6;++i){
  if(native_ordinary_type_descriptors[i]||!native_ordinary_cdata_creators[i]||
   native_ordinary_cdata_creators[i]->subtype!=static_cast<OrdinarySubtype>(i))
   throw ScriptError("native ordinary descriptor constructor replay or native writer order changed");
  native_ordinary_type_descriptors[i]=std::shared_ptr<NativeOrdinaryTypeDescriptor>(
   new NativeOrdinaryTypeDescriptor(this,native_ordinary_cdata_creators[i],descriptors[i],writers[i]));
 }
}

void initialize_native_ordinary_descriptor_prefixes_owned(){
 // Six original initializers ALWAYS assign54C then548 after their genuine
 // conditional descriptor ctor. Initial constructor NULL is not current state.
 for(std::size_t i=0;i<6;++i){
  if(native_ordinary_descriptor_prefixes[i]||!native_ordinary_type_descriptors[i])
   throw ScriptError("native descriptor548/54C actual prefix initializer replay/order changed");
  const bool button=i==static_cast<std::size_t>(OrdinarySubtype::button);
  native_ordinary_descriptor_prefixes[i]=std::shared_ptr<const NativeOrdinaryDescriptorPrefixReturn>(
   new NativeOrdinaryDescriptorPrefixReturn(this,native_ordinary_type_descriptors[i],
    button?NativeOrdinaryDescriptorPrefixReturn::Parent::fn61d770:NativeOrdinaryDescriptorPrefixReturn::Parent::fn6a6920,
    button?0x61d770:0x6a6920,button?0x433260:0x4acbf0));
 }
}

static void native_ordinary_metatype_full(lua_State* L){
 // Allocator owner is borrowed only during an actual protected Host call. Lock
 // genuine Host/Source/lifetime custody before any raw facade dispatch. Drop
 // those C++ borrowers before luaL_error can longjmp.
 auto& p=owner(L);p.source_exception={};p.source_error=nullptr;
 try{
  const auto self=p.weak_from_this().lock();
  const auto life=p.worldview_lifetime.lock();
  const auto source=p.worldview_sources.lock();
  if(!self||self.get()!=&p||!life||life.get()!=p.end_turn_lifetime.get()||
   !life->alive||life->failed||life->vm!=p.state||life->host!=p.worldview_host_owner||!source)
   throw ScriptError("native ordinary metatype actual Host/Source/lifetime retired");
  // A genuine coroutine shares this exact retained master global_State.
  // Allocator userdata alone does not grant another VM an actual current
  // context. Read both live VM identities only after strong Host/life custody.
  // This permits real threads and never substitutes L==master or a bool.
  if(!L||G(L)!=G(p.state))
   throw ScriptError("native ordinary metatype offered thread belongs to a different Lua VM");
  p.worldview_require_callback_origin();
  const auto& operation=source->current_operation_loan();
  if(&operation!=p.worldview_operation||!p.worldview_native_host||
   p.worldview_native_host->loan!=&operation||p.worldview_native_host->child>=p.child_contexts.size())
   throw ScriptError("native ordinary metatype current protected Source changed");
  const auto child=p.worldview_native_host->child;
  const auto selected=p.child_contexts[child].source;
  if(!selected||p.child_contexts[child].retired||!p.worldview_native_child_controls_current(child))
   throw ScriptError("native ordinary metatype actual selected source/current scope absent");
  operation.require_same_vm(*p.worldview_host_owner);p.worldview_require_callback_origin();
  operation.require_same_source(*selected);p.worldview_require_callback_origin();
  if(child>=p.child_contexts.size()||p.child_contexts[child].source!=selected||p.worldview_native_host->child!=child||
   !p.worldview_native_child_controls_current(child))
   throw ScriptError("native ordinary metatype selected Source changed after FULL");
 }catch(...){p.source_exception=std::current_exception();p.source_error="native ordinary metatype Source FULL refused";}
 if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}

static int native_ordinary_metatable_tostring_47d5f0(lua_State* L){
 // Complete original47D5F0. lua_isstring deliberately accepts a numeric value
 // exactly as Lua5.1 does; no stricter tostring/type-name coercion is invented.
 native_ordinary_metatype_full(L);
 lua_pushstring(L,"Metatable for type: ");native_ordinary_metatype_full(L);
 lua_getfield(L,-2,"CTypeName");native_ordinary_metatype_full(L);
 if(!lua_isstring(L,-1)){
  lua_settop(L,-2);native_ordinary_metatype_full(L);
  lua_pushstring(L,"Unknown");native_ordinary_metatype_full(L);
 }
 lua_concat(L,2);native_ordinary_metatype_full(L);return 1;
}

static int native_ordinary_control_tostring_505b40(lua_State* L){
 auto& p=owner(L);native_ordinary_metatype_full(L);
 const bool has_metatable=lua_getmetatable(L,-1)!=0;native_ordinary_metatype_full(L);
 if(has_metatable){
  lua_getfield(L,-1,"CTypeName");native_ordinary_metatype_full(L);
  // Fresh actualNUL9CFB60 independently proves the same CData key used by
  // both this original505B40 and complete7801B0; no foreign sink label is used.
  lua_getfield(L,-3,"CData");native_ordinary_metatype_full(L);
  void*const cdata=lua_touserdata(L,-1);
  lua_settop(L,-2);native_ordinary_metatype_full(L);
  // Actual raw9DFDC0 is ": %p". Formatting still needs the real CRT/ABI body;
  // raw text alone cannot issue an imported-function or a default observer.
  if(!p.native_ordinary_pointer_format||p.native_ordinary_pointer_format->host!=&p||
   p.native_ordinary_pointer_format->literal_address!=0x9dfdc0||
   p.native_ordinary_pointer_format->format!=": %p"||!p.native_ordinary_pointer_format->write_actual_pointer)
   return luaL_error(L,"required genuine native505B40 CRT formatter owner for actual9DFDC0");
  char buffer[0x20]{};
  (void)p.native_ordinary_pointer_format->write_actual_pointer(buffer,sizeof(buffer),cdata);
  native_ordinary_metatype_full(L);
  lua_pushstring(L,buffer);native_ordinary_metatype_full(L);
  lua_concat(L,2);native_ordinary_metatype_full(L);
  lua_remove(L,-2);native_ordinary_metatype_full(L);return 1;
 }
 // The original missing-metatable branch calls8F7960 BEFORE pushing
 // actual literal Malformed C Reflection. The diagnostic owner is unissued;
 // preserve this actual mandatory effect rather than fabricate the fallback.
 return luaL_error(L,"required genuine native8F7960 missing-metatable diagnostic before505B40 fallback");
}

static int native_ordinary_control_eq_7801b0(lua_State* L){
 // Complete185B original body. Non-table arguments return false. For two
 // tables, BOTH nil CData values compare actual Lua table pointers; otherwise
 // the original compares lua_touserdata outputs, without invented type checks.
 native_ordinary_metatype_full(L);bool equal=false;
 const bool first_table=lua_type(L,1)==LUA_TTABLE;native_ordinary_metatype_full(L);
 if(first_table){
  const bool second_table=lua_type(L,2)==LUA_TTABLE;native_ordinary_metatype_full(L);
  if(second_table){
   lua_getfield(L,1,"CData");native_ordinary_metatype_full(L);
   lua_getfield(L,2,"CData");native_ordinary_metatype_full(L);
   const bool first_nil=lua_type(L,3)==LUA_TNIL;native_ordinary_metatype_full(L);
   if(first_nil){
    const bool second_nil=lua_type(L,4)==LUA_TNIL;native_ordinary_metatype_full(L);
    if(second_nil){const void* second=lua_topointer(L,2);const void* first=lua_topointer(L,1);equal=first==second;}
   }else {void* first=lua_touserdata(L,3);void* second=lua_touserdata(L,4);equal=first==second;}
   lua_settop(L,-3);native_ordinary_metatype_full(L);
  }
 }
 lua_pushboolean(L,equal);native_ordinary_metatype_full(L);return 1;
}

static bool native_ordinary_reflection_physical_without_callbacks(
 Impl& p,lua_State* L,const NativeOrdinaryReflectionResult& r,int offered){
 if(!L||!p.state||r.host!=&p||r.creator!=std::this_thread::get_id()||!r.alive.load()||
  r.original!=0x5f36d0||r.key_address!=0x9e27e4||r.stage!=NativeOrdinaryReflectionResult::Stage::returned||
  r.table_ref==LUA_NOREF||r.table_ref==LUA_REFNIL||r.key_ref==LUA_NOREF||r.key_ref==LUA_REFNIL||!r.table)return false;
 const int top=lua_gettop(L);if(offered<0)offered=top+offered+1;
 if(offered<=0||lua_type(L,offered)!=LUA_TTABLE||lua_topointer(L,offered)!=r.table)return false;
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.table_ref);
 bool same=lua_type(L,-1)==LUA_TTABLE&&lua_topointer(L,-1)==r.table&&lua_rawequal(L,-1,offered);
 lua_pushvalue(L,LUA_GLOBALSINDEX);lua_rawgeti(L,LUA_REGISTRYINDEX,r.key_ref);lua_rawget(L,-2);
 same=same&&lua_type(L,-1)==LUA_TTABLE&&lua_topointer(L,-1)==r.table&&lua_rawequal(L,-1,offered);
 lua_settop(L,top);return same;
}

static void native_ordinary_push_reflection_5f36d0_owned(lua_State* L){
 // Complete original84B/29rows. The previously missing3B are actual ADD ESP28,
 // then sharedPOP/RET. Raw NUL9E27E4 is ExposedMembers, independently qualified.
 // Numeric-10002 is the supplied Lua VM globals index, never a source-owner DTO.
 auto& p=owner(L);native_flua_exposure_registration_full(L);
 lua_getfield(L,LUA_GLOBALSINDEX,"ExposedMembers");native_flua_exposure_registration_full(L);
 if(lua_type(L,-1)==LUA_TTABLE){
  for(const auto& r:p.native_ordinary_reflection_results)
   if(r&&native_ordinary_reflection_physical_without_callbacks(p,L,*r,-1))return;
  return (void)luaL_error(L,"native5F36D0 HIT lacks a genuine own completed reflection helper return");
 }
 lua_settop(L,-2);native_flua_exposure_registration_full(L);
 p.native_ordinary_reflection_results.push_back(std::shared_ptr<NativeOrdinaryReflectionResult>(
  new NativeOrdinaryReflectionResult(&p)));
 auto* r=p.native_ordinary_reflection_results.back().get();
 lua_createtable(L,0,0);native_flua_exposure_registration_full(L);
 r->table=lua_topointer(L,-1);r->stage=NativeOrdinaryReflectionResult::Stage::table_created;
 lua_pushvalue(L,-1);native_flua_exposure_registration_full(L);
 lua_setfield(L,LUA_GLOBALSINDEX,"ExposedMembers");native_flua_exposure_registration_full(L);
 r->stage=NativeOrdinaryReflectionResult::Stage::global_assigned;
 // Retain only the real returned table/key. No nontrivial C++ borrower remains
 // across allocation/GC, and no expired creation Source loan is renewed on HIT.
 lua_pushvalue(L,-1);native_flua_exposure_registration_full(L);
 r->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_flua_exposure_registration_full(L);
 lua_pushstring(L,"ExposedMembers");native_flua_exposure_registration_full(L);
 r->key_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_flua_exposure_registration_full(L);
 r->stage=NativeOrdinaryReflectionResult::Stage::retained;
 if(r->table_ref==LUA_NOREF||r->table_ref==LUA_REFNIL||r->key_ref==LUA_NOREF||r->key_ref==LUA_REFNIL||!r->table)
  return (void)luaL_error(L,"native5F36D0 actual returned table/key not retained");
 r->stage=NativeOrdinaryReflectionResult::Stage::returned;
 if(!native_ordinary_reflection_physical_without_callbacks(p,L,*r,-1))
  return (void)luaL_error(L,"native5F36D0 current actual returned reflection table physical POST failed");
 native_flua_exposure_registration_full(L);
}

static const NativeOrdinaryReflectionResult* native_ordinary_current_reflection_return_owned(lua_State* L,int offered){
 // The caller first executes the real helper. A table alone cannot issue this
 // closed return; a fresh genuine Source/FULL scope protects every capture.
 auto& p=owner(L);native_ordinary_metatype_full(L);
 for(const auto& r:p.native_ordinary_reflection_results)
  if(r&&native_ordinary_reflection_physical_without_callbacks(p,L,*r,offered))return r.get();
 luaL_error(L,"native5F36D0 current helper table/scope return absent");return nullptr;
}

static int native_ordinary_control_index_775dc0(lua_State* L){
 // Complete209B original local body. Mandatory first5F36D0 is not skipped.
 // Source5 proves rawget8F97D8 and INSERT8F97D2, never replace. A script-written
 // BaseType is read by originalgetfield; no invented recursion limit is added.
 native_ordinary_push_reflection_5f36d0_owned(L);native_ordinary_metatype_full(L);
 const bool has_metatable=lua_getmetatable(L,-3)!=0;native_ordinary_metatype_full(L);
 if(has_metatable){
  for(;;){
   lua_pushvalue(L,-1);native_ordinary_metatype_full(L);
   lua_rawget(L,-3);native_ordinary_metatype_full(L);
   const bool methods_table=lua_type(L,-1)==LUA_TTABLE;native_ordinary_metatype_full(L);
   if(methods_table){
    lua_pushvalue(L,-4);native_ordinary_metatype_full(L);
    lua_rawget(L,-2);native_ordinary_metatype_full(L);
    const bool found=lua_type(L,-1)!=LUA_TNIL;native_ordinary_metatype_full(L);
    if(found){
     lua_insert(L,-4);native_ordinary_metatype_full(L);
     lua_settop(L,-4);native_ordinary_metatype_full(L);return 1;
    }
    lua_settop(L,-3);native_ordinary_metatype_full(L);
   }else {lua_settop(L,-2);native_ordinary_metatype_full(L);}
   lua_getfield(L,-1,"BaseType");native_ordinary_metatype_full(L);
   const bool base_table=lua_type(L,-1)==LUA_TTABLE;native_ordinary_metatype_full(L);
   if(base_table){lua_remove(L,-2);native_ordinary_metatype_full(L);continue;}
   lua_settop(L,-4);native_ordinary_metatype_full(L);break;
  }
 }else {lua_settop(L,-2);native_ordinary_metatype_full(L);}
 lua_pushnil(L);native_ordinary_metatype_full(L);return 1;
}

void initialize_native_ordinary_517c_functions_owned(){
 // Actually install precisely the three qualified native closure bodies and
 // keys. The original5F36D0 dependency now has its own complete body; only
 // unissued diagnostic/format providers still stop their actual branches. This is not method-library
 // completion, an emitted placeholder callback, or a fullSetup return.
 if(native_ordinary_548_field)throw ScriptError("native517C BaseType field writer replay");
 const char* keys[]={"__tostring","__eq","__index"};
 const lua_CFunction bodies[]={&Impl::native_ordinary_control_tostring_505b40,
  &Impl::native_ordinary_control_eq_7801b0,&Impl::native_ordinary_control_index_775dc0};
 for(std::size_t i=0;i<3;++i){
  if(native_ordinary_517c_functions[i])throw ScriptError("native517C closure writer replay");
  native_ordinary_517c_functions[i]=std::shared_ptr<const NativeOrdinary517CFunction>(
   new NativeOrdinary517CFunction(this,static_cast<NativeOrdinary517CFunction::Function>(i),keys[i],bodies[i]));
 }
 native_ordinary_548_field=std::shared_ptr<const NativeOrdinary548FieldReturn>(
  new NativeOrdinary548FieldReturn(this,"BaseType"));
}

static bool native_ordinary_master_physical_without_callbacks(
 Impl& p,lua_State* L,const NativeOrdinaryMasterTypesResult& r,int offered){
 if(!L||!p.state||r.host!=&p||r.creator!=std::this_thread::get_id()||!r.alive.load()||
  r.stage!=NativeOrdinaryMasterTypesResult::Stage::returned||r.cache_ref==LUA_NOREF||
  r.metadata_ref==LUA_NOREF||!r.cache||!r.metadata)return false;
 const int top=lua_gettop(L);
 if(offered<0)offered=top+offered+1;
 if(offered<=0||lua_type(L,offered)!=LUA_TTABLE||lua_topointer(L,offered)!=r.cache)return false;
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.cache_ref);
 bool same=lua_type(L,-1)==LUA_TTABLE&&lua_topointer(L,-1)==r.cache&&lua_rawequal(L,-1,offered);
 lua_rawgeti(L,-1,1);lua_rawgeti(L,LUA_REGISTRYINDEX,r.metadata_ref);
 same=same&&lua_type(L,-1)==LUA_TTABLE&&lua_topointer(L,-1)==r.metadata&&lua_rawequal(L,-1,-2);
 lua_settop(L,top);return same;
}

static void native_ordinary_push_master_types_615d70_owned(lua_State* L){
 auto& p=owner(L);native_flua_exposure_registration_full(L);
 lua_getfield(L,LUA_GLOBALSINDEX,"ExposedTypes");native_flua_exposure_registration_full(L);
 if(lua_type(L,-1)==LUA_TTABLE){
  // A Lua table alone is not the native master constructor return. This branch
  // accepts only the retained output of a genuinely completed own615D70 body.
  for(const auto& keeper:p.native_ordinary_master_types_results){
   if(keeper&&keeper->cache==lua_topointer(L,-1)&&
    native_ordinary_master_physical_without_callbacks(p,L,*keeper,-1))return;
  }
  return (void)luaL_error(L,"native615D70 cache HIT lacks a genuine retained master constructor return");
 }
 lua_settop(L,-2);native_flua_exposure_registration_full(L);
 p.native_ordinary_master_types_results.push_back(std::make_shared<NativeOrdinaryMasterTypesResult>(&p));
 auto* r=p.native_ordinary_master_types_results.back().get();
 lua_createtable(L,0,0);native_flua_exposure_registration_full(L);r->cache=lua_topointer(L,-1);
 r->stage=NativeOrdinaryMasterTypesResult::Stage::cache_created;
 lua_pushvalue(L,-1);native_flua_exposure_registration_full(L);
 r->cache_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_flua_exposure_registration_full(L);
 lua_pushvalue(L,-1);native_flua_exposure_registration_full(L);
 lua_setfield(L,LUA_GLOBALSINDEX,"ExposedTypes");native_flua_exposure_registration_full(L);
 r->stage=NativeOrdinaryMasterTypesResult::Stage::global_assigned;
 lua_createtable(L,0,0);native_flua_exposure_registration_full(L);r->metadata=lua_topointer(L,-1);
 r->stage=NativeOrdinaryMasterTypesResult::Stage::metadata_created;
 lua_pushcclosure(L,&Impl::native_ordinary_metatable_tostring_47d5f0,0);native_flua_exposure_registration_full(L);
 lua_setfield(L,-2,"__tostring");native_flua_exposure_registration_full(L);
 lua_pushvalue(L,-1);native_flua_exposure_registration_full(L);
 r->metadata_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_flua_exposure_registration_full(L);
 // FreshSource3 exact615DEB→8F9724 JMP/IAT/name is lua_rawseti. This is slot1
 // assignment; wrong8F9740 is not an instruction start and is never reused.
 lua_rawseti(L,-2,1);native_flua_exposure_registration_full(L);
 r->stage=NativeOrdinaryMasterTypesResult::Stage::slot1_assigned;
 if(!r->cache||!r->metadata||r->cache_ref==LUA_NOREF||r->cache_ref==LUA_REFNIL||
  r->metadata_ref==LUA_NOREF||r->metadata_ref==LUA_REFNIL)
  return (void)luaL_error(L,"native615D70 actual master cache/slot1 metadata not retained");
 r->stage=NativeOrdinaryMasterTypesResult::Stage::returned;
 if(!native_ordinary_master_physical_without_callbacks(p,L,*r,-1))
  return (void)luaL_error(L,"native615D70 actual slot1 metadata final physical POST failed");
 native_flua_exposure_registration_full(L);
}

static NativeOrdinaryTypeDescriptor* native_ordinary_actual_descriptor_owned(lua_State* L,OrdinarySubtype subtype){
 auto& p=owner(L);native_ordinary_export_full(L);
 const auto index=static_cast<std::size_t>(subtype);
 if(index>=6||!p.native_ordinary_type_descriptors[index]){
  luaL_error(L,"native ordinary descriptor actual static constructor/name setter has not returned");return nullptr;
 }
 auto* d=p.native_ordinary_type_descriptors[index].get();
 const auto* frame=p.root_ordinary_controls_export_frame;
 if(d->host!=&p||d->creator!=std::this_thread::get_id()||!d->alive.load()||
  !d->native_creator||p.native_ordinary_cdata_creators[index]!=d->native_creator||
  !frame||frame->result->cdata_creator!=d->native_creator||
  d->native_creator->subtype!=subtype){
  luaL_error(L,"native ordinary constructor fields/type-name owner changed");return nullptr;
 }
 return d;
}

static const NativeOrdinaryDescriptorPrefixReturn* native_ordinary_actual_prefix_owned(lua_State* L,OrdinarySubtype subtype){
 auto& p=owner(L);auto* descriptor=native_ordinary_actual_descriptor_owned(L,subtype);
 const auto index=static_cast<std::size_t>(subtype);
 if(index>=6||!p.native_ordinary_descriptor_prefixes[index]){
  luaL_error(L,"native descriptor actual nonNULL548/54C static initializer has not returned");return nullptr;
 }
 const auto* r=p.native_ordinary_descriptor_prefixes[index].get();
 const bool button=subtype==OrdinarySubtype::button;
 if(r->host!=&p||r->creator!=std::this_thread::get_id()||!r->alive.load()||r->descriptor.get()!=descriptor||
  r->parent548!=(button?NativeOrdinaryDescriptorPrefixReturn::Parent::fn61d770:NativeOrdinaryDescriptorPrefixReturn::Parent::fn6a6920)||
  r->actual548!=(button?0x61d770u:0x6a6920u)||r->actual54c!=(button?0x433260u:0x4acbf0u)){
  luaL_error(L,"native descriptor actual548/54C owner changed");return nullptr;
 }
 return r;
}

static const NativeOrdinaryNestedDescriptorReturn* native_ordinary_required_nested_descriptor_owned(lua_State* L,bool button){
 auto& p=owner(L);native_ordinary_export_full(L);const std::size_t index=button?1:0;
 if(!p.native_ordinary_nested_descriptors[index]){
  luaL_error(L,"%s",button?"required genuine nested CB35D8 descriptor constructor5E23D0 return before61D770":
   "required genuine nested CB7C68 descriptor constructor5210F0 return before6A6920");return nullptr;
 }
 auto* descriptor=p.native_ordinary_nested_descriptors[index].get();
 if(descriptor->host!=&p||descriptor->creator!=std::this_thread::get_id()||!descriptor->alive.load()||
  descriptor->kind!=(button?NativeOrdinaryNestedDescriptorReturn::Kind::descriptor_cb35d8:
   NativeOrdinaryNestedDescriptorReturn::Kind::descriptor_cb7c68)||
  descriptor->actual_descriptor!=(button?0xcb35d8u:0xcb7c68u)||
  descriptor->actual_ctor!=(button?0x5e23d0u:0x5210f0u)||descriptor->actual_name0.empty()){
  luaL_error(L,"native548 actual distinct nested descriptor constructor/name owner changed");return nullptr;
 }
 return descriptor;
}

static void native_ordinary_push_nested_548_owned(lua_State* L,bool button){
 auto& p=owner(L);auto* descriptor=native_ordinary_required_nested_descriptor_owned(L,button);
 // Actual conditional ctor must have returned before cache lookup. An expired
 // constructor loan is not renewed from an ordinary/type-name Lua table.
 native_ordinary_push_master_types_615d70_owned(L);native_ordinary_export_full(L);
 lua_getfield(L,-1,descriptor->actual_name0.c_str());native_ordinary_export_full(L);
 if(lua_type(L,-1)!=LUA_TTABLE){
  lua_settop(L,-3);native_ordinary_export_full(L);
  descriptor=native_ordinary_required_nested_descriptor_owned(L,button);
  // The corresponding real generic descriptor0 construction is a separate
  // publisher. Its nested548/count/name fields cannot be inferred from the
  // six concrete ordinary types. Preserve the mandatory recursive517C STOP.
  return (void)luaL_error(L,button?"required recursive517C20 for genuine CB35D8 descriptor":
   "required recursive517C20 for genuine CB7C68 descriptor");
 }
 lua_remove(L,-2);native_ordinary_export_full(L);
 // Even a type5 HIT needs a returned nested517C owner. Constructor data alone
 // is not authority for an arbitrary script-supplied nested metatype table.
 return (void)luaL_error(L,"native548 cache HIT requires genuine returned nested517C metatype owner");
}

static const NativeOrdinary517CFunction* native_ordinary_required_517c_function_owned(lua_State* L,std::size_t index){
 auto& p=owner(L);if(!native_flua_exposure_metadata_current_if_entered(L))native_ordinary_export_full(L);
 if(index>=3||!p.native_ordinary_517c_functions[index]){
  const char* errors[]={"required genuine native505B40 closure body and __tostring owner",
   "required genuine native7801B0 closure body and actual9F2CF4 key owner",
   "required genuine native775DC0 closure body and actual9EAF08 key owner"};
  luaL_error(L,"%s",index<3?errors[index]:"native517C callback index differs");return nullptr;
 }
 auto* f=p.native_ordinary_517c_functions[index].get();
 if(f->host!=&p||f->creator!=std::this_thread::get_id()||!f->alive.load()||!f->body||f->field.empty()||
  static_cast<std::size_t>(f->original)!=index){
  luaL_error(L,"native517C actual closure body/key owner changed");return nullptr;
 }
 return f;
}

static bool native_ordinary_type_physical_without_callbacks(
 Impl& p,const NativeOrdinaryMetatypeResult& r,const NativeRootOrdinaryControlsResult& exported){
 if(!p.state||r.host!=&p||r.creator!=std::this_thread::get_id()||!r.alive.load()||
  r.stage!=NativeOrdinaryMetatypeResult::Stage::returned||!r.descriptor||!r.master||
  r.table_ref==LUA_NOREF||r.table_ref==LUA_REFNIL||!r.table||exported.keys_ref==LUA_NOREF||
  exported.cdata_creator!=r.descriptor->native_creator||r.name!=exported.cdata_creator->type_name||
  r.name_operand!=NativeOrdinaryMetatypeResult::Name::descriptor0)return false;
 const auto index=static_cast<std::size_t>(exported.cdata_creator->subtype);
 if(index>=6||p.native_ordinary_type_descriptors[index]!=r.descriptor||
  r.descriptor->host!=&p||!r.descriptor->alive.load())return false;
 for(std::size_t i=0;i<3;++i){const auto& f=r.functions[i];
  if(!f||p.native_ordinary_517c_functions[i]!=f||f->host!=&p||!f->alive.load()||
   f->creator!=std::this_thread::get_id()||static_cast<std::size_t>(f->original)!=i||!f->body)return false;}
 lua_State* L=p.state;const int top=lua_gettop(L);
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.master->cache_ref);
 bool same=native_ordinary_master_physical_without_callbacks(p,L,*r.master,-1);
 if(!same){lua_settop(L,top);return false;}
 const int master=lua_gettop(L);
 lua_rawgeti(L,LUA_REGISTRYINDEX,exported.keys_ref);lua_rawgeti(L,-1,4);lua_rawget(L,master);
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.table_ref);
 same=lua_type(L,-1)==LUA_TTABLE&&lua_topointer(L,-1)==r.table&&lua_rawequal(L,-1,-2);
 if(same){
  if(!lua_getmetatable(L,-1))same=false;
  else {lua_rawgeti(L,LUA_REGISTRYINDEX,r.master->metadata_ref);
   same=lua_topointer(L,-1)==r.master->metadata&&lua_rawequal(L,-1,-2);}
 }
 lua_settop(L,top);return same;
}

static void native_ordinary_construct_metatype_517c20_owned(lua_State* L,OrdinarySubtype subtype){
 auto& p=owner(L);auto* descriptor=native_ordinary_actual_descriptor_owned(L,subtype);
 native_ordinary_export_full(L);
 // Execute the actual descriptor0 table/name prefix and all three genuine
 // callback stores. Their own bodies preserve any still unissued mandatory
 // dependencies when called; no error-only callback replaces an original.
 lua_createtable(L,0,0);native_ordinary_export_full(L);
 const void* table=lua_topointer(L,-1);
 lua_pushstring(L,descriptor->native_creator->type_name);native_ordinary_export_full(L);
 lua_setfield(L,-2,"CTypeName");native_ordinary_export_full(L);
 for(std::size_t i=0;i<3;++i){
  auto* fn=native_ordinary_required_517c_function_owned(L,i);
  lua_pushcclosure(L,fn->body,0);native_ordinary_export_full(L);
  fn=native_ordinary_required_517c_function_owned(L,i);
  lua_setfield(L,-2,fn->field.c_str());native_ordinary_export_full(L);
 }
 // All six actual static initializers wrote NONNULL548. Never continue using
 // the earlier constructor NULL. Execute its own distinct real parent callback.
 const auto* prefix=native_ordinary_actual_prefix_owned(L,subtype);
 native_ordinary_push_nested_548_owned(L,prefix->parent548==NativeOrdinaryDescriptorPrefixReturn::Parent::fn61d770);
 native_ordinary_export_full(L);prefix=native_ordinary_actual_prefix_owned(L,subtype);
 if(!p.native_ordinary_548_field||p.native_ordinary_548_field->host!=&p||
  p.native_ordinary_548_field->literal_address!=0x9e5f0c)
  return (void)luaL_error(L,"required actual9E5F0C field key owner after native548 returned metatype");
 lua_setfield(L,-2,p.native_ordinary_548_field->field.c_str());native_ordinary_export_full(L);
 native_ordinary_push_master_types_615d70_owned(L);native_ordinary_export_full(L);
 NativeOrdinaryMasterTypesResult* master=nullptr;
 for(const auto& keeper:p.native_ordinary_master_types_results)
  if(keeper&&keeper->cache==lua_topointer(L,-1)&&native_ordinary_master_physical_without_callbacks(p,L,*keeper,-1)){
   master=keeper.get();break;
  }
 if(!master)return (void)luaL_error(L,"native517C actual615D70 closed master return missing");
 lua_rawgeti(L,-1,1);native_ordinary_export_full(L);
 if(lua_type(L,-1)==LUA_TTABLE){lua_setmetatable(L,-3);native_ordinary_export_full(L);}
 else {lua_settop(L,-2);native_ordinary_export_full(L);}
 lua_pushvalue(L,-2);native_ordinary_export_full(L);
 descriptor=native_ordinary_actual_descriptor_owned(L,subtype);
 lua_setfield(L,-2,descriptor->native_creator->type_name);native_ordinary_export_full(L);
 lua_settop(L,-2);native_ordinary_export_full(L);
 if(lua_topointer(L,-1)!=table)return (void)luaL_error(L,"native517C original returned table changed");
 const auto index=static_cast<std::size_t>(subtype);
 // Persist the body return before retaining APIs can trigger GC. Weak Host
 // backedges are unnecessary here: all raw identities are inspected only with
 // the already retained genuine Host and Source frame; no shared Host cycle.
 NativeOrdinaryMasterTypesResult* actual_master=master;
 {
  std::shared_ptr<NativeOrdinaryMasterTypesResult> master_owner;
  for(const auto& keeper:p.native_ordinary_master_types_results)if(keeper.get()==actual_master){master_owner=keeper;break;}
  p.native_ordinary_metatype_results.push_back(std::make_shared<NativeOrdinaryMetatypeResult>(&p,
   p.native_ordinary_type_descriptors[index],p.native_ordinary_517c_functions,master_owner,
   NativeOrdinaryMetatypeResult::Name::descriptor0,descriptor->native_creator->type_name));
 } // No nontrivial C++ borrower survives across allocating Lua APIs.
 auto* result=p.native_ordinary_metatype_results.back().get();result->table=table;
 result->stage=NativeOrdinaryMetatypeResult::Stage::cache_assigned;
 lua_pushvalue(L,-1);native_ordinary_export_full(L);
 result->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_ordinary_export_full(L);
 if(result->table_ref==LUA_NOREF||result->table_ref==LUA_REFNIL)
  return (void)luaL_error(L,"native517C actual returned metatype not retained");
 result->stage=NativeOrdinaryMetatypeResult::Stage::returned;
 native_ordinary_export_full(L);
}

static void native_ordinary_push_metatype_owned(lua_State* L,OrdinarySubtype subtype){
 auto& p=owner(L);auto* descriptor=native_ordinary_actual_descriptor_owned(L,subtype);
 auto* exported=p.root_ordinary_controls_export_frame->result.get();
 native_ordinary_push_master_types_615d70_owned(L);native_ordinary_export_full(L);
 descriptor=native_ordinary_actual_descriptor_owned(L,subtype);
 lua_getfield(L,-1,descriptor->native_creator->type_name);native_ordinary_export_full(L);
 if(lua_type(L,-1)!=LUA_TTABLE){
  // Actual six callback MISS path pops both lookup and master before517C20.
  lua_settop(L,-3);native_ordinary_export_full(L);
  native_ordinary_construct_metatype_517c20_owned(L,subtype);native_ordinary_export_full(L);
 }else {lua_remove(L,-2);native_ordinary_export_full(L);}
 NativeOrdinaryMetatypeResult* keeper=nullptr;
 for(const auto& result:p.native_ordinary_metatype_results)
  if(result&&result->table==lua_topointer(L,-1)&&
   native_ordinary_type_physical_without_callbacks(p,*result,*exported)){keeper=result.get();break;}
 if(!keeper)return (void)luaL_error(L,"ordinary cache HIT/MISS lacks a genuine retained517C body return");
 for(const auto& result:p.native_ordinary_metatype_results)if(result.get()==keeper){exported->metatype_owner=result;break;}
 exported->types=keeper->master->cache;exported->metatable=keeper->table;
 lua_rawgeti(L,LUA_REGISTRYINDEX,keeper->master->cache_ref);native_ordinary_export_full(L);
 exported->types_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_ordinary_export_full(L);
 lua_pushvalue(L,-1);native_ordinary_export_full(L);
 exported->metatable_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_ordinary_export_full(L);
 if(exported->types_ref==LUA_NOREF||exported->types_ref==LUA_REFNIL||
  exported->metatable_ref==LUA_NOREF||exported->metatable_ref==LUA_REFNIL)
  return (void)luaL_error(L,"ordinary actual master/metatype return references absent");
 native_ordinary_export_full(L);
}

void native_ordinary_metatype_retire()noexcept{
 for(const auto& result:native_ordinary_metatype_results)if(result)result->alive.store(false);
 for(const auto& result:native_ordinary_master_types_results)if(result)result->alive.store(false);
 for(const auto& descriptor:native_ordinary_type_descriptors)if(descriptor)descriptor->alive.store(false);
 for(const auto& prefix:native_ordinary_descriptor_prefixes)if(prefix)prefix->alive.store(false);
 for(const auto& descriptor:native_ordinary_nested_descriptors)if(descriptor)descriptor->alive.store(false);
 for(const auto& fn:native_ordinary_517c_functions)if(fn)fn->alive.store(false);
 for(const auto& result:native_ordinary_reflection_results)if(result)result->alive.store(false);
}
