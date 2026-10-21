// Complete original8F7C80 / qualified actualA8E420 / earned protected8F7AE0.
// Inside Host::Impl after actual CodeBuddy8F7840/8F7D40. The protected8F7AE0
// body and private return are owned by Optional, not reimplemented here.

static int native_ordinary_codebuddy_tostring_8f7c80(lua_State* L){
 auto& p=owner(L);native_ordinary_metatype_full(L);
 const auto* callback=native_ordinary_codebuddy_callback_owned(L);
 if(callback->body!=&Impl::native_ordinary_codebuddy_tostring_8f7c80)
  return luaL_error(L,"nativeCodeBuddy8F7C80 callback body differs from actual bootstrap");
 // Complete original186B. Retain the real input on the Lua stack throughout;
 // lookup must be lua_gettable, including its original metamethod boundary.
 native_ordinary_push_codebuddy_table_8f7d40_owned(L);native_ordinary_metatype_full(L);
 lua_pushvalue(L,-2);native_ordinary_metatype_full(L);
 lua_gettable(L,-2);native_ordinary_metatype_full(L);
 const int observed=lua_type(L,-1);native_ordinary_metatype_full(L);
 if(observed==LUA_TFUNCTION){
  lua_remove(L,-2);native_ordinary_metatype_full(L);
  lua_pushvalue(L,-2);native_ordinary_metatype_full(L);
  // The genuine helper, not lua_call or a successful skip, owns the complete
  // status/results/traceback history. On failure preserve its actual stack;
  // original8F7C80 performs no invented error pop before the pointer fallback.
  const auto* returned=p.native_master_protected_call_8f7ae0_owned(L,1,1);
  native_ordinary_metatype_full(L);
  p.native_master_protected_call_return_current_owned(L,returned);
  const bool successful=returned->actual_status==0;
  if(successful){
   const bool string=lua_isstring(L,-1)!=0;native_ordinary_metatype_full(L);
   if(string)return 1;
   lua_settop(L,-2);native_ordinary_metatype_full(L);
  }
 }else if(observed==LUA_TSTRING){
  lua_remove(L,-2);native_ordinary_metatype_full(L);return 1;
 }else{
  lua_settop(L,-3);native_ordinary_metatype_full(L);
 }
 const void*const actual_pointer=lua_topointer(L,-1);native_ordinary_metatype_full(L);
 // Qualified Windows secure CRT / original0x40-byte scratch / ignored return.
 // Actual Lua pointer width is preserved; no x86 narrowing or cached value.
 char buffer[0x40]{};
 (void)::sprintf_s(buffer,sizeof(buffer),"function: %p",const_cast<void*>(actual_pointer));
 native_ordinary_metatype_full(L);
 lua_pushstring(L,buffer);native_ordinary_metatype_full(L);return 1;
}

void initialize_native_ordinary_codebuddy_callback_owned(){
 if(native_ordinary_codebuddy_callback)
  throw ScriptError("nativeCodeBuddy8F7C80 actual callback bootstrap replay");
 // This publishes only this complete actual body/key. The master catalog,
 // default scope and completed Setup are separate mandatory producers.
 native_ordinary_codebuddy_callback=std::shared_ptr<const NativeOrdinaryCodeBuddyCallback>(
  new NativeOrdinaryCodeBuddyCallback(this,&Impl::native_ordinary_codebuddy_tostring_8f7c80));
}
