//Actual Host::Impl after Source16 geometry and Source2 real FLua ref kernels.
static const NativeContextDefinition::Record*native_control_base_color_alpha_record_current_owned(
 lua_State*L,std::size_t ordinal){
 native_control_base_setoffset_record_current_owned(L);auto&p=owner(L);
 struct Recipe {std::uint32_t writer,key,primary,secondary;const char*name;};
 static constexpr Recipe exact[]={
  {0x9be5e0u,0xa3f82cu,0x6b4b50u,0x526620u,"SetColor"},
  {0x9be640u,0xa3f85cu,0x766e70u,0x6d8d40u,"SetAlpha"},
  {0x9be6a0u,0xa3f88cu,0x78f870u,0x4ec6e0u,"GetAlpha"},
 };
 if(ordinal<21||ordinal>23)return (luaL_error(L,"ControlBase colour/alpha requires actual ordinal21..23"),nullptr);
 const auto& proof=exact[ordinal-21];const auto& record=p.native_control_base_constructor->definition->records[ordinal];
 if(!record.recipe||!record.key||record.key!=record.recipe->key||record.recipe->writer!=proof.writer||
  record.recipe->key_address!=proof.key||std::strcmp(record.key,proof.name)||record.primary!=proof.primary||
  record.secondary!=proof.secondary||record.primary!=record.recipe->primary||record.secondary!=record.recipe->secondary||
  record.flag!=0||record.flag!=record.recipe->flag||record.reference!=-1)
  return (luaL_error(L,"ControlBase colour/alpha actual recipe/flag/reference changed"),nullptr);
 return &record;
}

struct NativeControlBaseColorDecodeCall final {
 using Reference=NativeControlBaseSetOffsetDecodeCall::Reference;
 enum class Stage {entered,argument_retained,table_checked,keys_retained,converted,released,member_returned,retired};
 const Impl*const host;const std::thread::id creator=std::this_thread::get_id();
 NativeControlBaseSetOffsetDecodeCall*const borrower;
 bool alive=true;std::array<Reference,4> keys; //x,y,z,w actual retained references.
 std::size_t retained=0,converted=0;std::optional<std::array<float,4>> value;
 Stage stage=Stage::entered;
 NativeControlBaseColorDecodeCall(Impl*p,NativeControlBaseSetOffsetDecodeCall*r):host(p),borrower(r){
  if(!host||!borrower)throw ScriptError("SetColor actual four-key reference owner missing");
 }
};
static void native_control_base_color_decode_current_owned(lua_State*L,const NativeControlBaseColorDecodeCall&r){
 native_control_base_color_alpha_record_current_owned(L,21u);auto&p=owner(L);
 if(r.host!=&p||r.creator!=std::this_thread::get_id()||!r.alive||!r.borrower||
  r.stage==NativeControlBaseColorDecodeCall::Stage::retired||r.retained>4||r.converted>4||
  std::none_of(p.native_control_base_color_decode_calls.begin(),p.native_control_base_color_decode_calls.end(),
   [&](const auto& owned){return owned.get()==&r;})||
  std::none_of(p.native_control_base_setoffset_decode_calls.begin(),p.native_control_base_setoffset_decode_calls.end(),
   [&](const auto& owned){return owned.get()==r.borrower;}))
  return (void)luaL_error(L,"SetColor SAME actual four-key owner/borrower retired");
 //New Source2 custody certifies only Host/L/Source/CData/reference lifetime.
 //No SetOffset argument decode, physical call, value or old return is reused.
 native_control_base_setoffset_decode_full_owned(L,*r.borrower);
 native_control_base_color_alpha_record_current_owned(L,21u);
}
static NativeControlBaseColorDecodeCall*native_control_base_color_begin_decode_owned(lua_State*L,const void*cdata){
 native_control_base_color_alpha_record_current_owned(L,21u);
 auto*const borrower=native_control_base_setoffset_begin_decode_owned(L,cdata);auto&p=owner(L);
 p.source_exception={};p.source_error=nullptr;
 try{p.native_control_base_color_decode_calls.push_back(std::make_unique<NativeControlBaseColorDecodeCall>(&p,borrower));}
 catch(...){p.source_exception=std::current_exception();p.source_error="SetColor four-key custody allocation failed";}
 if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);return nullptr;}
 auto*r=p.native_control_base_color_decode_calls.back().get();native_control_base_color_decode_current_owned(L,*r);return r;
}
//Portable call-local custody cleanup after actual member return. This has
//no Original field/write/callback effect. All Lua refs are already unbound;
//partial calls retain their existing Host owners through Lua longjmp/close.
static void native_control_base_color_success_cleanup_owned(lua_State*L,NativeControlBaseColorDecodeCall&r){
 native_control_base_color_decode_current_owned(L,r);auto&p=owner(L);
 auto*const borrower=r.borrower;
 if(r.stage!=NativeControlBaseColorDecodeCall::Stage::member_returned||!r.value||
  borrower->stage!=NativeControlBaseSetOffsetDecodeCall::Stage::entered||borrower->value||
  lua_gettop(L)!=2||r.retained>4||r.converted>4)
  return (void)luaL_error(L,"SetColor successful custody cleanup requires completed actual call");
 const auto unbound=[](const auto& ref){return !ref.state&&ref.reference==LUA_NOREF;};
 if(!unbound(borrower->argument)||!unbound(borrower->key_x)||!unbound(borrower->key_y)||
  !unbound(borrower->temporary)||std::any_of(r.keys.begin(),r.keys.end(),
   [&](const auto& ref){return !unbound(ref);}))
  return (void)luaL_error(L,"SetColor successful custody cleanup cannot erase a live Lua reference");
 std::size_t color_slot=p.native_control_base_color_decode_calls.size();
 std::size_t borrower_slot=p.native_control_base_setoffset_decode_calls.size();
 for(std::size_t i=0;i<p.native_control_base_color_decode_calls.size();++i)
  if(p.native_control_base_color_decode_calls[i].get()==&r)color_slot=i;
 for(std::size_t i=0;i<p.native_control_base_setoffset_decode_calls.size();++i)
  if(p.native_control_base_setoffset_decode_calls[i].get()==borrower)borrower_slot=i;
 if(color_slot==p.native_control_base_color_decode_calls.size()||
  borrower_slot==p.native_control_base_setoffset_decode_calls.size())
  return (void)luaL_error(L,"SetColor successful custody cleanup lost exact call owners");
 //Every check above can fail/longjmp before mutation. From here: no Lua,
 //Source provider, allocation or retained borrower is invoked. Erase exact
 //pointees, not vector.back(): another pending call may share these vectors.
 r.alive=false;r.stage=NativeControlBaseColorDecodeCall::Stage::retired;
 p.native_control_base_color_decode_calls.erase(p.native_control_base_color_decode_calls.begin()+color_slot);
 borrower->alive=false;borrower->stage=NativeControlBaseSetOffsetDecodeCall::Stage::retired;
 p.native_control_base_setoffset_decode_calls.erase(p.native_control_base_setoffset_decode_calls.begin()+borrower_slot);
 //No access to r/borrower after their deletion. Surviving unique_ptr pointees
 //remain stable when their slots move. Vector capacity stays reusable.
}
static std::array<float,4>native_control_base_color_vector_89e740_owned(lua_State*L,NativeControlBaseColorDecodeCall&r){
 native_control_base_color_decode_current_owned(L,r);auto& b=*r.borrower;
 if(r.stage!=NativeControlBaseColorDecodeCall::Stage::entered||r.value||r.retained||r.converted||
  b.argument.state||b.key_x.state||b.key_y.state||b.temporary.state||b.value||
  b.stage!=NativeControlBaseSetOffsetDecodeCall::Stage::entered){
  luaL_error(L,"SetColor requires fresh unbound actual four-channel decode");std::terminate();
 }
 b.argument.state=L;b.argument.reference=LUA_NOREF;
 (void)lua_checkstack(L,1);native_control_base_color_decode_current_owned(L,r);
 lua_pushvalue(L,2);native_control_base_color_decode_current_owned(L,r);
 b.argument.reference=luaL_ref(L,LUA_REGISTRYINDEX);native_control_base_color_decode_current_owned(L,r);
 r.stage=NativeControlBaseColorDecodeCall::Stage::argument_retained;
 const bool table=native_control_base_setoffset_reference_table_8f88a0_owned(L,b);
 native_control_base_color_decode_current_owned(L,r);r.stage=NativeControlBaseColorDecodeCall::Stage::table_checked;
 if(!table){
  r.value=std::array<float,4>{0.0f,0.0f,0.0f,0.0f}; //Actual89E8FA/FE/903/908 four XORPS+0 stores.
  native_control_base_setoffset_reference_release_8f8820_owned(L,b,b.argument);
  native_control_base_color_decode_current_owned(L,r);r.stage=NativeControlBaseColorDecodeCall::Stage::released;return *r.value;
 }
 const char*const keys[]={"x","y","z","w"}; //ActualA40C94/90/8C/88, not RGBA key names.
 for(std::size_t reverse=4;reverse>0;--reverse){const auto lane=reverse-1;
  (void)lua_checkstack(L,1);native_control_base_color_decode_current_owned(L,r);
  lua_pushstring(L,keys[lane]);native_control_base_color_decode_current_owned(L,r);
  native_control_base_setoffset_reference_from_top_8f8b10_owned(L,b,r.keys[lane]);
  native_control_base_color_decode_current_owned(L,r);
  lua_settop(L,-2);native_control_base_color_decode_current_owned(L,r);++r.retained;
 }
 r.stage=NativeControlBaseColorDecodeCall::Stage::keys_retained;std::array<float,4> values;
 for(std::size_t lane=0;lane<4;++lane){
  //Actual89E87C/889/896/8A3 lazy RAW field lookup+57AC40 converts x,y,z,w.
  values[lane]=native_control_base_setoffset_float_57ac40_owned(L,b,r.keys[lane]);
  native_control_base_color_decode_current_owned(L,r);++r.converted;
 }
 r.value=values;r.stage=NativeControlBaseColorDecodeCall::Stage::converted;
 for(std::size_t lane=0;lane<4;++lane){
  //Actual89E8C3/CC/D5/DE destroys x,y,z,w; temporary unref already returned.
  native_control_base_setoffset_reference_release_8f8820_owned(L,b,r.keys[lane]);
  native_control_base_color_decode_current_owned(L,r);
 }
 native_control_base_setoffset_reference_release_8f8820_owned(L,b,b.argument);
 native_control_base_color_decode_current_owned(L,r);r.stage=NativeControlBaseColorDecodeCall::Stage::released;return *r.value;
}
static int native_control_base_setcolor_primary_6b4b50_owned(lua_State*L){
 native_control_base_color_alpha_record_current_owned(L,21u);
 const int count=lua_gettop(L);native_control_base_color_alpha_record_current_owned(L,21u);
 if(count!=2){(void)lua_gettop(L);native_control_base_color_alpha_record_current_owned(L,21u);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89EAE8 ControlBase::SetColor receiver-plus-one-argument count");}
 const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
 native_control_base_color_alpha_record_current_owned(L,21u);
 if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89EACB ControlBase::SetColor NULL receiver");
 auto*r=native_control_base_color_begin_decode_owned(L,cdata);
 const auto value=native_control_base_color_vector_89e740_owned(L,*r);
 native_control_base_color_decode_current_owned(L,*r);
 if(r->stage!=NativeControlBaseColorDecodeCall::Stage::released||!r->value||lua_gettop(L)!=2||
  r->borrower->argument.state||r->borrower->temporary.state||
  std::any_of(r->keys.begin(),r->keys.end(),[](const auto& key){return key.state!=nullptr;}))
  return luaL_error(L,"SetColor actual89E740 conversion/cleanup did not return");
 if(!lua_checkstack(L,32))return luaL_error(L,"SetColor requires actual receiver borrower Lua stack reserve32");
 native_control_base_color_decode_current_owned(L,*r);
 //Original89EA87..B9 copies four FP32s into a16byte byVALUE member block.
 native_control_base_setcolor_member_6d19f0_owned(L,cdata,value);
 native_control_base_color_decode_current_owned(L,*r);r->stage=NativeControlBaseColorDecodeCall::Stage::member_returned;
 native_control_base_color_success_cleanup_owned(L,*r);return 0;
}
static int native_control_base_setalpha_primary_766e70_owned(lua_State*L){
 native_control_base_color_alpha_record_current_owned(L,22u);
 const int count=lua_gettop(L);native_control_base_color_alpha_record_current_owned(L,22u);
 if(count!=2){(void)lua_gettop(L);native_control_base_color_alpha_record_current_owned(L,22u);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E160 ControlBase::SetAlpha receiver-plus-one-argument count");}
 const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
 native_control_base_color_alpha_record_current_owned(L,22u);
 if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E146 ControlBase::SetAlpha NULL receiver");
 const float value=static_cast<float>(lua_tonumber(L,2));native_control_base_color_alpha_record_current_owned(L,22u);
 if(!lua_checkstack(L,32))return luaL_error(L,"SetAlpha requires actual receiver borrower Lua stack reserve32");
 native_control_base_color_alpha_record_current_owned(L,22u);
 if(lua_gettop(L)!=2)return luaL_error(L,"SetAlpha actual89E100 argument frame changed");
 native_control_base_setalpha_member_601c30_owned(L,cdata,value);
 native_control_base_color_alpha_record_current_owned(L,22u);return 0;
}
static int native_control_base_getalpha_primary_78f870_owned(lua_State*L){
 native_control_base_color_alpha_record_current_owned(L,23u);
 const int count=lua_gettop(L);native_control_base_color_alpha_record_current_owned(L,23u);
 if(count!=1){(void)lua_gettop(L);native_control_base_color_alpha_record_current_owned(L,23u);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E252 ControlBase::GetAlpha receiver-only count");}
 const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
 native_control_base_color_alpha_record_current_owned(L,23u);
 if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E23A ControlBase::GetAlpha NULL receiver");
 if(!lua_checkstack(L,32))return luaL_error(L,"GetAlpha requires actual receiver borrower Lua stack reserve32");
 native_control_base_color_alpha_record_current_owned(L,23u);
 const double value=native_control_base_getalpha_member_6785a0_owned(L,cdata);
 native_control_base_color_alpha_record_current_owned(L,23u);
 lua_pushnumber(L,value);native_control_base_color_alpha_record_current_owned(L,23u);return 1;
}
static int native_control_base_color_alpha_signature_owned(lua_State*L,std::size_t ordinal){
 const auto*const record=native_control_base_color_alpha_record_current_owned(L,ordinal);char buffer[0x100]{};
 if(ordinal==23u)(void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","float","",record->key);
 else (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","",record->key,ordinal==21u?"FGXVector4":"float","");
 native_control_base_color_alpha_record_current_owned(L,ordinal);
 lua_pushstring(L,buffer);native_control_base_color_alpha_record_current_owned(L,ordinal);return 1;
}
static int native_control_base_setcolor_signature_526620_owned(lua_State*L){return native_control_base_color_alpha_signature_owned(L,21u);}
static int native_control_base_setalpha_signature_6d8d40_owned(lua_State*L){return native_control_base_color_alpha_signature_owned(L,22u);}
static int native_control_base_getalpha_signature_4ec6e0_owned(lua_State*L){return native_control_base_color_alpha_signature_owned(L,23u);}
void native_control_base_color_decoder_retire()noexcept{
 for(const auto&r:native_control_base_color_decode_calls)if(r){
  if(r->host!=this||r->creator!=std::this_thread::get_id())std::terminate();
  r->alive=false;r->stage=NativeControlBaseColorDecodeCall::Stage::retired;
  //Partial reference cells are persistent owned state; SAME VM lua_close
  //reclaims them after both colour and original Source2 borrowers retire.
 }
}
