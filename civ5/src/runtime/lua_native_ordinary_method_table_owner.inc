// Inside the actual Host::Impl, after owned native metatype helpers.
// Actual8F7840 global CodeBuddy ->8F7D40 slot1 weak-key table ->8F7E20
// secondary CFunction association. Distinct from ExposedTypes/ExposedMembers.
class NativeOrdinaryCodeBuddyCallback final {
 friend struct Impl;
 const Impl*const host;
 const std::thread::id creator=std::this_thread::get_id();
 mutable std::atomic<bool> alive{true};
 const std::uint32_t original=0x8f7c80,key_address=0xa8e454;
 const std::string key="__tostring";
 const lua_CFunction body;
 NativeOrdinaryCodeBuddyCallback(const Impl* p,lua_CFunction actual):host(p),body(actual){
  if(!host||!body)throw ScriptError("nativeCodeBuddy genuine8F7C80 callback body absent");
 }
};

class NativeOrdinaryCodeBuddyGlobalReturn final {
 friend struct Impl;
 const Impl*const host;
 const std::thread::id creator=std::this_thread::get_id();
 mutable std::atomic<bool> alive{true};
 const std::uint32_t original=0x8f7840,get_key_address=0xa8e430,set_key_address=0xa8e43c;
 enum class Stage {captured,table_created,global_assigned,retained,returned};
 Stage stage=Stage::captured;
 int table_ref=LUA_NOREF,key_ref=LUA_NOREF;
 const void*table=nullptr;
 explicit NativeOrdinaryCodeBuddyGlobalReturn(const Impl* p):host(p){
  if(!host)throw ScriptError("native8F7840 actual Host absent");
 }
};

class NativeOrdinaryCodeBuddyTableReturn final {
 friend struct Impl;
 const Impl*const host;
 const std::thread::id creator=std::this_thread::get_id();
 mutable std::atomic<bool> alive{true};
 const std::uint32_t original=0x8f7d40;
 const std::shared_ptr<const NativeOrdinaryCodeBuddyGlobalReturn> parent;
 const std::shared_ptr<const NativeOrdinaryCodeBuddyCallback> callback;
 enum class Stage {captured,table_created,slot1_assigned,weak_metadata_applied,
  function_created,function_metadata_applied,retained,returned};
 Stage stage=Stage::captured;
 int table_ref=LUA_NOREF,weak_metadata_ref=LUA_NOREF,function_ref=LUA_NOREF,function_metadata_ref=LUA_NOREF;
 const void*table=nullptr,*weak_metadata=nullptr,*function=nullptr,*function_metadata=nullptr;
 NativeOrdinaryCodeBuddyTableReturn(const Impl* p,
  std::shared_ptr<const NativeOrdinaryCodeBuddyGlobalReturn> supplied,
  std::shared_ptr<const NativeOrdinaryCodeBuddyCallback> actual)
  :host(p),parent(std::move(supplied)),callback(std::move(actual)){
  if(!host||!parent||parent->host!=host||!callback||callback->host!=host)
   throw ScriptError("native8F7D40 actual global and callback owners absent");
 }
};

static bool native_ordinary_codebuddy_global_physical_without_callbacks(
 Impl& p,lua_State* L,const NativeOrdinaryCodeBuddyGlobalReturn& r,int offered){
 if(!L||!p.state||r.host!=&p||r.creator!=std::this_thread::get_id()||!r.alive.load()||
  r.original!=0x8f7840||r.get_key_address!=0xa8e430||r.set_key_address!=0xa8e43c||
  r.stage!=NativeOrdinaryCodeBuddyGlobalReturn::Stage::returned||
  r.table_ref==LUA_NOREF||r.table_ref==LUA_REFNIL||r.key_ref==LUA_NOREF||r.key_ref==LUA_REFNIL||!r.table)return false;
 const int top=lua_gettop(L);if(offered<0)offered=top+offered+1;
 if(offered<=0||lua_type(L,offered)!=LUA_TTABLE||lua_topointer(L,offered)!=r.table)return false;
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.table_ref);
 bool same=lua_type(L,-1)==LUA_TTABLE&&lua_topointer(L,-1)==r.table&&lua_rawequal(L,-1,offered);
 lua_pushvalue(L,LUA_GLOBALSINDEX);lua_rawgeti(L,LUA_REGISTRYINDEX,r.key_ref);lua_rawget(L,-2);
 same=same&&lua_type(L,-1)==LUA_TTABLE&&lua_topointer(L,-1)==r.table&&lua_rawequal(L,-1,offered);
 lua_settop(L,top);return same;
}

static void native_ordinary_push_codebuddy_8f7840_owned(lua_State* L){
 auto& p=owner(L);native_flua_exposure_registration_full(L);
 // Original explicitly ignores lua_checkstack's result; it is not a grant.
 (void)lua_checkstack(L,2);native_flua_exposure_registration_full(L);
 lua_getfield(L,LUA_GLOBALSINDEX,"CodeBuddy");native_flua_exposure_registration_full(L);
 const bool table=lua_type(L,-1)==LUA_TTABLE;native_flua_exposure_registration_full(L);
 if(table){
  for(const auto& r:p.native_ordinary_codebuddy_globals)
   if(r&&native_ordinary_codebuddy_global_physical_without_callbacks(p,L,*r,-1))return;
  return (void)luaL_error(L,"native8F7840 HIT lacks genuine completed own CodeBuddy global return");
 }
 lua_settop(L,-2);native_flua_exposure_registration_full(L);
 p.native_ordinary_codebuddy_globals.push_back(std::shared_ptr<NativeOrdinaryCodeBuddyGlobalReturn>(
  new NativeOrdinaryCodeBuddyGlobalReturn(&p)));
 auto* r=p.native_ordinary_codebuddy_globals.back().get();
 lua_createtable(L,0,0);native_flua_exposure_registration_full(L);
 r->table=lua_topointer(L,-1);r->stage=NativeOrdinaryCodeBuddyGlobalReturn::Stage::table_created;
 lua_pushvalue(L,-1);native_flua_exposure_registration_full(L);
 lua_setfield(L,LUA_GLOBALSINDEX,"CodeBuddy");native_flua_exposure_registration_full(L);
 r->stage=NativeOrdinaryCodeBuddyGlobalReturn::Stage::global_assigned;
 // The separately qualified GET/SET addresses hold identical actual NUL
 // bytes. Native missing3B are ADD ESP28 before POP/RET, no hidden Lua effect.
 lua_pushvalue(L,-1);native_flua_exposure_registration_full(L);
 r->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_flua_exposure_registration_full(L);
 lua_pushstring(L,"CodeBuddy");native_flua_exposure_registration_full(L);
 r->key_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_flua_exposure_registration_full(L);
 r->stage=NativeOrdinaryCodeBuddyGlobalReturn::Stage::retained;
 if(!r->table||r->table_ref==LUA_NOREF||r->table_ref==LUA_REFNIL||
  r->key_ref==LUA_NOREF||r->key_ref==LUA_REFNIL)
  return (void)luaL_error(L,"native8F7840 real CodeBuddy returned table/key not retained");
 r->stage=NativeOrdinaryCodeBuddyGlobalReturn::Stage::returned;
 if(!native_ordinary_codebuddy_global_physical_without_callbacks(p,L,*r,-1))
  return (void)luaL_error(L,"native8F7840 actual current global physical POST failed");
 native_flua_exposure_registration_full(L);
}

static bool native_ordinary_codebuddy_table_physical_without_callbacks(
 Impl& p,lua_State* L,const NativeOrdinaryCodeBuddyTableReturn& r,int offered){
 if(!L||!p.state||r.host!=&p||r.creator!=std::this_thread::get_id()||!r.alive.load()||
  r.original!=0x8f7d40||r.stage!=NativeOrdinaryCodeBuddyTableReturn::Stage::returned||
  !r.parent||r.parent->host!=&p||!r.callback||r.callback!=p.native_ordinary_codebuddy_callback||
  r.callback->host!=&p||!r.callback->alive.load()||r.callback->creator!=std::this_thread::get_id()||
  r.callback->original!=0x8f7c80||r.callback->key_address!=0xa8e454||!r.callback->body||
  r.table_ref==LUA_NOREF||r.table_ref==LUA_REFNIL||r.weak_metadata_ref==LUA_NOREF||r.weak_metadata_ref==LUA_REFNIL||
  r.function_ref==LUA_NOREF||r.function_ref==LUA_REFNIL||r.function_metadata_ref==LUA_NOREF||
  r.function_metadata_ref==LUA_REFNIL||!r.table||!r.weak_metadata||!r.function||!r.function_metadata)return false;
 const int top=lua_gettop(L);if(offered<0)offered=top+offered+1;
 if(offered<=0||lua_type(L,offered)!=LUA_TTABLE||lua_topointer(L,offered)!=r.table)return false;
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.parent->table_ref);
 bool same=native_ordinary_codebuddy_global_physical_without_callbacks(p,L,*r.parent,-1);
 lua_rawgeti(L,-1,1);lua_rawgeti(L,LUA_REGISTRYINDEX,r.table_ref);
 same=same&&lua_type(L,-1)==LUA_TTABLE&&lua_topointer(L,-1)==r.table&&lua_rawequal(L,-1,-2)&&lua_rawequal(L,-1,offered);
 if(same){
  if(lua_getmetatable(L,-1)==0)same=false;
  else{
   lua_rawgeti(L,LUA_REGISTRYINDEX,r.weak_metadata_ref);
   same=lua_topointer(L,-1)==r.weak_metadata&&lua_rawequal(L,-1,-2);
   lua_pushliteral(L,"__mode");lua_rawget(L,-2);
   const std::size_t length=lua_type(L,-1)==LUA_TSTRING?lua_objlen(L,-1):0;
   same=same&&length==1&&std::memcmp(lua_tostring(L,-1),"k",1)==0;
  }
 }
 lua_settop(L,top);
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.function_ref);
 same=same&&lua_type(L,-1)==LUA_TFUNCTION&&lua_iscfunction(L,-1)&&lua_topointer(L,-1)==r.function;
 if(same){
  if(lua_getmetatable(L,-1)==0)same=false;
  else{
   const void*const current_metadata=lua_topointer(L,-1);
   const NativeOrdinaryCodeBuddyTableReturn* current_owner=nullptr;
   for(const auto& owned:p.native_ordinary_codebuddy_tables)
    if(owned&&owned->host==&p&&owned->alive.load()&&owned->creator==std::this_thread::get_id()&&
       owned->stage==NativeOrdinaryCodeBuddyTableReturn::Stage::returned&&owned->function_metadata==current_metadata)
       current_owner=owned.get();
   same=current_owner&&current_owner->function_metadata_ref>=0&&current_owner->function_ref>=0&&
        current_owner->callback==p.native_ordinary_codebuddy_callback;
   if(same){
    lua_rawgeti(L,LUA_REGISTRYINDEX,current_owner->function_metadata_ref);
    same=lua_rawequal(L,-1,-2);lua_pop(L,1);
    lua_pushliteral(L,"__tostring");lua_rawget(L,-2);
    same=same&&lua_type(L,-1)==LUA_TFUNCTION&&lua_topointer(L,-1)==current_owner->function&&
         lua_tocfunction(L,-1)==r.callback->body;
   }
  }
 }
 lua_settop(L,top);return same;
}

static const NativeOrdinaryCodeBuddyCallback* native_ordinary_codebuddy_callback_owned(lua_State* L){
 auto& p=owner(L);native_flua_exposure_registration_full(L);
 if(!p.native_ordinary_codebuddy_callback||p.native_ordinary_codebuddy_callback->host!=&p||
  !p.native_ordinary_codebuddy_callback->alive.load()||
  p.native_ordinary_codebuddy_callback->creator!=std::this_thread::get_id()||
  p.native_ordinary_codebuddy_callback->original!=0x8f7c80||
  p.native_ordinary_codebuddy_callback->key_address!=0xa8e454||
  p.native_ordinary_codebuddy_callback->key!="__tostring"||!p.native_ordinary_codebuddy_callback->body){
  luaL_error(L,"required genuine nativeCodeBuddy8F7C80 callback body before8F7D40 closure0");return nullptr;
 }
 return p.native_ordinary_codebuddy_callback.get();
}

static void native_ordinary_push_codebuddy_table_8f7d40_owned(lua_State* L){
 auto& p=owner(L);native_ordinary_push_codebuddy_8f7840_owned(L);native_flua_exposure_registration_full(L);
 const NativeOrdinaryCodeBuddyGlobalReturn* parent=nullptr;
 for(const auto& r:p.native_ordinary_codebuddy_globals)
  if(r&&native_ordinary_codebuddy_global_physical_without_callbacks(p,L,*r,-1)){parent=r.get();break;}
 if(!parent)return (void)luaL_error(L,"native8F7D40 actual8F7840 parent return absent");
 lua_rawgeti(L,-1,1);native_flua_exposure_registration_full(L);
 const bool table=lua_type(L,-1)==LUA_TTABLE;native_flua_exposure_registration_full(L);
 if(table){
  const NativeOrdinaryCodeBuddyTableReturn* actual=nullptr;
  for(const auto& r:p.native_ordinary_codebuddy_tables)
   if(r&&r->parent.get()==parent&&native_ordinary_codebuddy_table_physical_without_callbacks(p,L,*r,-1)){
    actual=r.get();break;
   }
  if(!actual)return (void)luaL_error(L,"native8F7D40 slot1 HIT lacks genuine completed current table return");
  lua_remove(L,-2);native_flua_exposure_registration_full(L);return;
 }
 lua_settop(L,-2);native_flua_exposure_registration_full(L);
 (void)lua_checkstack(L,5);native_flua_exposure_registration_full(L);
 // An unissued actual callback remains a mandatory STOP. Never install an
 // error-only CFunction, a NULL or another metatype's closure as8F7C80.
 const auto* callback=native_ordinary_codebuddy_callback_owned(L);
 {
  std::shared_ptr<const NativeOrdinaryCodeBuddyGlobalReturn> parent_owner;
  for(const auto& r:p.native_ordinary_codebuddy_globals)if(r.get()==parent){parent_owner=r;break;}
  p.native_ordinary_codebuddy_tables.push_back(std::shared_ptr<NativeOrdinaryCodeBuddyTableReturn>(
   new NativeOrdinaryCodeBuddyTableReturn(&p,parent_owner,p.native_ordinary_codebuddy_callback)));
 } // No shared borrower survives allocating Lua calls or longjmp.
 auto* r=p.native_ordinary_codebuddy_tables.back().get();
 lua_createtable(L,0,0);native_flua_exposure_registration_full(L);
 r->table=lua_topointer(L,-1);r->stage=NativeOrdinaryCodeBuddyTableReturn::Stage::table_created;
 lua_pushvalue(L,-1);native_flua_exposure_registration_full(L);
 lua_rawseti(L,-3,1);native_flua_exposure_registration_full(L);
 r->stage=NativeOrdinaryCodeBuddyTableReturn::Stage::slot1_assigned;
 lua_createtable(L,0,0);native_flua_exposure_registration_full(L);
 r->weak_metadata=lua_topointer(L,-1);
 lua_pushstring(L,"k");native_flua_exposure_registration_full(L);
 lua_setfield(L,-2,"__mode");native_flua_exposure_registration_full(L);
 lua_pushvalue(L,-1);native_flua_exposure_registration_full(L);
 r->weak_metadata_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_flua_exposure_registration_full(L);
 lua_setmetatable(L,-2);native_flua_exposure_registration_full(L);
 r->stage=NativeOrdinaryCodeBuddyTableReturn::Stage::weak_metadata_applied;
 callback=native_ordinary_codebuddy_callback_owned(L);
 lua_pushcclosure(L,callback->body,0);native_flua_exposure_registration_full(L);
 r->function=lua_topointer(L,-1);r->stage=NativeOrdinaryCodeBuddyTableReturn::Stage::function_created;
 lua_pushvalue(L,-1);native_flua_exposure_registration_full(L);
 r->function_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_flua_exposure_registration_full(L);
 lua_createtable(L,0,0);native_flua_exposure_registration_full(L);
 r->function_metadata=lua_topointer(L,-1);
 lua_pushvalue(L,-2);native_flua_exposure_registration_full(L);
 callback=native_ordinary_codebuddy_callback_owned(L);
 lua_setfield(L,-2,callback->key.c_str());native_flua_exposure_registration_full(L);
 lua_pushvalue(L,-1);native_flua_exposure_registration_full(L);
 r->function_metadata_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_flua_exposure_registration_full(L);
 lua_setmetatable(L,-2);native_flua_exposure_registration_full(L);
 r->stage=NativeOrdinaryCodeBuddyTableReturn::Stage::function_metadata_applied;
 lua_settop(L,-2);native_flua_exposure_registration_full(L);
 lua_remove(L,-2);native_flua_exposure_registration_full(L);
 // The real gap ends ADD ESP10 and the saved tail removes the parent at-2.
 // The sole returned table is the genuine slot1 table, not a hand-built DTO.
 if(lua_topointer(L,-1)!=r->table)return (void)luaL_error(L,"native8F7D40 actual returned slot1 table changed");
 lua_pushvalue(L,-1);native_flua_exposure_registration_full(L);
 r->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_flua_exposure_registration_full(L);
 r->stage=NativeOrdinaryCodeBuddyTableReturn::Stage::retained;
 if(r->table_ref==LUA_NOREF||r->table_ref==LUA_REFNIL||r->weak_metadata_ref==LUA_NOREF||
  r->weak_metadata_ref==LUA_REFNIL||r->function_ref==LUA_NOREF||r->function_ref==LUA_REFNIL||
  r->function_metadata_ref==LUA_NOREF||r->function_metadata_ref==LUA_REFNIL)
  return (void)luaL_error(L,"native8F7D40 actual returned table/metatables/function not retained");
 r->stage=NativeOrdinaryCodeBuddyTableReturn::Stage::returned;
 if(!native_ordinary_codebuddy_table_physical_without_callbacks(p,L,*r,-1))
  return (void)luaL_error(L,"native8F7D40 actual returned weak table/function metadata physical POST failed");
 native_flua_exposure_registration_full(L);
}

void native_ordinary_codebuddy_retire()noexcept{
 if(native_ordinary_codebuddy_callback)native_ordinary_codebuddy_callback->alive.store(false);
 for(const auto& r:native_ordinary_codebuddy_tables)if(r)r->alive.store(false);
 for(const auto& r:native_ordinary_codebuddy_globals)if(r)r->alive.store(false);
}
