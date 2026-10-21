//Inside actual Host::Impl after existing Source17 geometry and Source10 Base/cdata.
//Parent composition extends the EXISTING bool/uint Tree seam; no new borrower.
static const NativeContextDefinition::Record*native_control_base_mouse_record_current_owned(
 lua_State*L,std::size_t ordinal){
 native_control_base_setoffset_record_current_owned(L);auto&p=owner(L);
 struct Recipe {std::uint32_t writer,key,primary,secondary;const char*name;};
 static constexpr Recipe exact[]={
  {0x9bed00u,0xa3fcb0u,0x669850u,0x48a020u,"HasMouseOver"},
  {0x9bedc0u,0xa3fd34u,0x48edf0u,0x613ac0u,"SetConsumeMouseOver"},
  {0x9bee20u,0xa3fd84u,0x624b60u,0x5fd300u,"SetConsumeMouseButton"},
  {0x9bee80u,0xa3fdd8u,0x62af80u,0x770eb0u,"SetConsumeMouseWheel"},
  {0x9beee0u,0xa3fe28u,0x75d480u,0x520f90u,"GetConsumeMouseOver"},
  {0x9bef40u,0xa3fe78u,0x6d8390u,0x5f3680u,"GetConsumeMouseButton"},
  {0x9befa0u,0xa3feccu,0x488050u,0x4c7cb0u,"GetConsumeMouseWheel"},
 };
 if(ordinal!=40u&&(ordinal<42u||ordinal>47u))
  return (luaL_error(L,"ControlBase mouse requires actual ordinal40/42..47"),nullptr);
 const auto& proof=exact[ordinal==40u?0u:ordinal-41u];
 const auto& record=p.native_control_base_constructor->definition->records[ordinal];
 if(!record.recipe||!record.key||record.key!=record.recipe->key||record.recipe->writer!=proof.writer||
  record.recipe->key_address!=proof.key||std::strcmp(record.key,proof.name)||record.primary!=proof.primary||
  record.secondary!=proof.secondary||record.primary!=record.recipe->primary||record.secondary!=record.recipe->secondary||
  record.flag!=0||record.flag!=record.recipe->flag||record.reference!=-1)
  return (luaL_error(L,"ControlBase mouse actual recipe/flag/reference changed"),nullptr);
 return &record; //Only the actual pre-constant stage; no whole49 library claim.
}

static void native_control_base_setconsume_mouse_over_member_4d7210_owned(
 lua_State*L,const void*cdata,bool value){
 //Actual entry4D7210 has BOTH RET4 paths; OR800 or ANDFFFFF7FF at flags40.
 const NativeControlBaseOffsetCommand command=NativeControlBaseBooleanFlag{
  ui::BnwWorldViewControlTree::NativeOrdinaryFlagWrite::consume_mouse_over_4d7210,value};
 native_control_base_offset_command_member_owned(L,cdata,command);
}
static void native_control_base_setconsume_mouse_button_member_437ec0_owned(
 lua_State*L,const void*cdata,bool value){
 //Direct thiscall, low byte of the copied four-byte bool operand; no vslot.
 const NativeControlBaseOffsetCommand command=NativeControlBaseBooleanFlag{
  ui::BnwWorldViewControlTree::NativeOrdinaryFlagWrite::consume_mouse_button_437ec0,value};
 native_control_base_offset_command_member_owned(L,cdata,command);
}
static void native_control_base_setconsume_mouse_wheel_member_6798e0_owned(
 lua_State*L,const void*cdata,bool value){
 const NativeControlBaseOffsetCommand command=NativeControlBaseBooleanFlag{
  ui::BnwWorldViewControlTree::NativeOrdinaryFlagWrite::consume_mouse_wheel_6798e0,value};
 native_control_base_offset_command_member_owned(L,cdata,command);
}
static bool native_control_base_hasmouseover_flag42_669850_owned(lua_State*L,const void*cdata){
 //Read the real owned flags40 mask0x10000 under constructor/current CData
 //custody. Existing Host borrower refuses unsupported input dispatch before
 //this field read; no input supplier is required merely to register closure40.
 return native_control_base_uint_member_owned(L,cdata,
  ui::BnwWorldViewControlTree::NativeOrdinaryUintRead::mouse_over_42_bit0_669850)!=0u;
}
static bool native_control_base_getconsume_mouse_over_flag40_75d480_owned(lua_State*L,const void*cdata){
 return native_control_base_uint_member_owned(L,cdata,
  ui::BnwWorldViewControlTree::NativeOrdinaryUintRead::consume_mouse_over_40_bit11_75d480)!=0u;
}
static bool native_control_base_getconsume_mouse_button_flag40_6d8390_owned(lua_State*L,const void*cdata){
 return native_control_base_uint_member_owned(L,cdata,
  ui::BnwWorldViewControlTree::NativeOrdinaryUintRead::consume_mouse_button_40_bit10_6d8390)!=0u;
}
static bool native_control_base_getconsume_mouse_wheel_flag40_488050_owned(lua_State*L,const void*cdata){
 return native_control_base_uint_member_owned(L,cdata,
  ui::BnwWorldViewControlTree::NativeOrdinaryUintRead::consume_mouse_wheel_40_bit12_488050)!=0u;
}

static int native_control_base_mouse_setter_89e2d0_owned(lua_State*L,std::size_t ordinal){
 native_control_base_mouse_record_current_owned(L,ordinal);
 if(ordinal<42u||ordinal>44u)return luaL_error(L,"Actual mouse setter ordinal42..44 required");
 const int count=lua_gettop(L);native_control_base_mouse_record_current_owned(L,ordinal);
 if(count!=2){(void)lua_gettop(L);native_control_base_mouse_record_current_owned(L,ordinal);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
   "89E33D ControlBase mouse setter receiver-plus-one-argument count; actual normal-return sink required");}
 const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
 native_control_base_mouse_record_current_owned(L,ordinal);
 if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
  "89E322 ControlBase mouse setter NULL receiver; actual normal-return sink required");
 //Actual89E2FA -> TEST/SETNZ. Lua false/nil are false; every other value,
 //including numeric0, is true. There is no boolean-only type check.
 const bool value=lua_toboolean(L,2)!=0;
 native_control_base_mouse_record_current_owned(L,ordinal);
 if(!lua_checkstack(L,32))return luaL_error(L,"Mouse setter requires real receiver borrower stack reserve32");
 native_control_base_mouse_record_current_owned(L,ordinal);
 if(lua_gettop(L)!=2)return luaL_error(L,"Mouse setter actual89E2D0 argument frame changed");
 if(ordinal==42u)native_control_base_setconsume_mouse_over_member_4d7210_owned(L,cdata,value);
 else if(ordinal==43u)native_control_base_setconsume_mouse_button_member_437ec0_owned(L,cdata,value);
 else native_control_base_setconsume_mouse_wheel_member_6798e0_owned(L,cdata,value);
 native_control_base_mouse_record_current_owned(L,ordinal);return 0;
}
static int native_control_base_setconsume_mouse_over_primary_48edf0_owned(lua_State*L){
 return native_control_base_mouse_setter_89e2d0_owned(L,42u);
}
static int native_control_base_setconsume_mouse_button_primary_624b60_owned(lua_State*L){
 return native_control_base_mouse_setter_89e2d0_owned(L,43u);
}
static int native_control_base_setconsume_mouse_wheel_primary_62af80_owned(lua_State*L){
 return native_control_base_mouse_setter_89e2d0_owned(L,44u);
}
static int native_control_base_mouse_getter_owned(lua_State*L,std::size_t ordinal){
 native_control_base_mouse_record_current_owned(L,ordinal);
 if(ordinal!=40u&&(ordinal<45u||ordinal>47u))return luaL_error(L,"Actual mouse getter ordinal40/45..47 required");
 const int count=lua_gettop(L);native_control_base_mouse_record_current_owned(L,ordinal);
 if(count!=1){(void)lua_gettop(L);native_control_base_mouse_record_current_owned(L,ordinal);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
   "Mouse primary count1 diagnostic8F7960; actual normal-return sink required");}
 //Raw669865/75D495/6D83A5/488065 CALL89E050; not const89DF00/89E480.
 const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
 native_control_base_mouse_record_current_owned(L,ordinal);
 if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
  "Mouse primary NULL receiver diagnostic8F7960; actual normal-return sink required");
 if(!lua_checkstack(L,32))return luaL_error(L,"Mouse getter requires real receiver borrower stack reserve32");
 native_control_base_mouse_record_current_owned(L,ordinal);
 if(lua_gettop(L)!=1)return luaL_error(L,"Mouse getter actual receiver frame changed");
 bool value; //Filled only by an actual current physical read; no default false.
 if(ordinal==40u)value=native_control_base_hasmouseover_flag42_669850_owned(L,cdata);
 else if(ordinal==45u)value=native_control_base_getconsume_mouse_over_flag40_75d480_owned(L,cdata);
 else if(ordinal==46u)value=native_control_base_getconsume_mouse_button_flag40_6d8390_owned(L,cdata);
 else value=native_control_base_getconsume_mouse_wheel_flag40_488050_owned(L,cdata);
 //Existing geometry member helper ended ALL C++ borrowers before this API.
 native_control_base_mouse_record_current_owned(L,ordinal);
 lua_pushboolean(L,value?1:0);native_control_base_mouse_record_current_owned(L,ordinal);return 1;
}
static int native_control_base_hasmouseover_primary_669850_owned(lua_State*L){
 return native_control_base_mouse_getter_owned(L,40u);
}
static int native_control_base_getconsume_mouse_over_primary_75d480_owned(lua_State*L){
 return native_control_base_mouse_getter_owned(L,45u);
}
static int native_control_base_getconsume_mouse_button_primary_6d8390_owned(lua_State*L){
 return native_control_base_mouse_getter_owned(L,46u);
}
static int native_control_base_getconsume_mouse_wheel_primary_488050_owned(lua_State*L){
 return native_control_base_mouse_getter_owned(L,47u);
}

static int native_control_base_mouse_signature_owned(lua_State*L,std::size_t ordinal){
 const auto*const record=native_control_base_mouse_record_current_owned(L,ordinal);
 char buffer[0x100]{};
 if(ordinal>=42u&&ordinal<=44u)
  (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","",record->key,"bool","");
 else (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","bool","",record->key);
 native_control_base_mouse_record_current_owned(L,ordinal);
 lua_pushstring(L,buffer);native_control_base_mouse_record_current_owned(L,ordinal);return 1;
}
static int native_control_base_hasmouseover_signature_48a020_owned(lua_State*L){return native_control_base_mouse_signature_owned(L,40u);}
static int native_control_base_setconsume_mouse_over_signature_613ac0_owned(lua_State*L){return native_control_base_mouse_signature_owned(L,42u);}
static int native_control_base_setconsume_mouse_button_signature_5fd300_owned(lua_State*L){return native_control_base_mouse_signature_owned(L,43u);}
static int native_control_base_setconsume_mouse_wheel_signature_770eb0_owned(lua_State*L){return native_control_base_mouse_signature_owned(L,44u);}
static int native_control_base_getconsume_mouse_over_signature_520f90_owned(lua_State*L){return native_control_base_mouse_signature_owned(L,45u);}
static int native_control_base_getconsume_mouse_button_signature_5f3680_owned(lua_State*L){return native_control_base_mouse_signature_owned(L,46u);}
static int native_control_base_getconsume_mouse_wheel_signature_4c7cb0_owned(lua_State*L){return native_control_base_mouse_signature_owned(L,47u);}
