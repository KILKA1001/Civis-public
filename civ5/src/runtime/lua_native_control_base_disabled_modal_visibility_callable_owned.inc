//Source21 sequential actual rows27..31, using existing Source18 physical borrower.
static const NativeContextDefinition::Record*native_control_base_flags_visibility_record_current_owned(
 lua_State*L,std::size_t ordinal){
 native_control_base_setoffset_record_current_owned(L);auto&p=owner(L);
 struct Recipe{std::uint32_t writer,key,primary,secondary;int flag;const char*name;};
 static constexpr Recipe exact[]={
  {0x9be820u,0xa3f974u,0x5acf00u,0x559fe0u,0,"SetDisabled"},
  {0x9be880u,0xa3f9a4u,0x6d2930u,0x5dc890u,1,"IsDisabled"},
  {0x9be8e0u,0xa3f9d4u,0x58e600u,0x5ad210u,0,"SetModal"},
  {0x9be940u,0xa3fa00u,0x70c030u,0x64a700u,1,"IsModal"},
  {0x9be9a0u,0xa3fa44u,0x6ad460u,0x62ce50u,0,"CalculateVisibilityBox"},
 };
 if(ordinal<27||ordinal>31)return (luaL_error(L,"ControlBase source21 ordinal differs"),nullptr);
 const auto& proof=exact[ordinal-27];const auto& record=p.native_control_base_constructor->definition->records[ordinal];
 if(!record.recipe||!record.key||record.key!=record.recipe->key||record.recipe->writer!=proof.writer||
  record.recipe->key_address!=proof.key||std::strcmp(record.key,proof.name)||record.primary!=proof.primary||
  record.secondary!=proof.secondary||record.primary!=record.recipe->primary||record.secondary!=record.recipe->secondary||
  record.flag!=proof.flag||record.flag!=record.recipe->flag||record.reference!=-1)
  return (luaL_error(L,"ControlBase source21 actual recipe/flag/reference changed"),nullptr);
 return &record;
}
static int native_control_base_disabled_modal_bool_89e2d0_owned(lua_State*L,std::size_t ordinal){
 native_control_base_flags_visibility_record_current_owned(L,ordinal);
 if(ordinal!=27&&ordinal!=29)return luaL_error(L,"Actual disabled/modal setter ordinal required");
 const int count=lua_gettop(L);native_control_base_flags_visibility_record_current_owned(L,ordinal);
 if(count!=2){(void)lua_gettop(L);native_control_base_flags_visibility_record_current_owned(L,ordinal);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E33D Disabled/Modal receiver-plus-one-argument count");}
 const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
 native_control_base_flags_visibility_record_current_owned(L,ordinal);
 if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E322 Disabled/Modal NULL receiver");
 const bool value=lua_toboolean(L,2)!=0;native_control_base_flags_visibility_record_current_owned(L,ordinal);
 if(!lua_checkstack(L,32))return luaL_error(L,"Disabled/Modal actual receiver borrower needs reserve32");
 native_control_base_flags_visibility_record_current_owned(L,ordinal);
 if(lua_gettop(L)!=2)return luaL_error(L,"Disabled/Modal actual bool argument frame changed");
 if(ordinal==27){const NativeControlBaseOffsetCommand command=NativeControlBaseDisabledWrite{value};
  native_control_base_offset_command_member_owned(L,cdata,command);}
 else{const NativeControlBaseOffsetCommand command=NativeControlBaseBooleanFlag{
  ui::BnwWorldViewControlTree::NativeOrdinaryFlagWrite::modal_7a5fc0,value};
  native_control_base_offset_command_member_owned(L,cdata,command);}
 native_control_base_flags_visibility_record_current_owned(L,ordinal);return 0;
}
static int native_control_base_setdisabled_primary_5acf00_owned(lua_State*L){
 return native_control_base_disabled_modal_bool_89e2d0_owned(L,27);
}
static int native_control_base_setmodal_primary_58e600_owned(lua_State*L){
 return native_control_base_disabled_modal_bool_89e2d0_owned(L,29);
}
static int native_control_base_disabled_modal_read_owned(lua_State*L,std::size_t ordinal){
 native_control_base_flags_visibility_record_current_owned(L,ordinal);
 if(ordinal!=28&&ordinal!=30)return luaL_error(L,"Actual disabled/modal const getter ordinal required");
 const int count=lua_gettop(L);native_control_base_flags_visibility_record_current_owned(L,ordinal);
 if(count!=1){(void)lua_gettop(L);native_control_base_flags_visibility_record_current_owned(L,ordinal);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,ordinal==28?"6D298F IsDisabled receiver-only count":"70C08F IsModal receiver-only count");}
 //Both actual primary CALL89E480, unlike mouse getters. Source18 owns its
 //mutable-first89DF00 and strict required distinct CA8D24 const fallback.
 const void*const cdata=native_control_base_const_cdata_89e480_mutable_first_owned(L,1);
 native_control_base_flags_visibility_record_current_owned(L,ordinal);
 if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,
  ordinal==28?"6D2972 IsDisabled NULL receiver":"70C072 IsModal NULL receiver");
 if(!lua_checkstack(L,32))return luaL_error(L,"Disabled/Modal getter current borrower needs reserve32");
 native_control_base_flags_visibility_record_current_owned(L,ordinal);
 if(lua_gettop(L)!=1)return luaL_error(L,"Disabled/Modal actual getter frame changed");
 const auto observed=native_control_base_uint_member_owned(L,cdata,ordinal==28?
  ui::BnwWorldViewControlTree::NativeOrdinaryUintRead::disabled_40_bit17_6d2930:
  ui::BnwWorldViewControlTree::NativeOrdinaryUintRead::modal_40_bit19_70c030);
 native_control_base_flags_visibility_record_current_owned(L,ordinal);
 lua_pushboolean(L,observed?1:0);native_control_base_flags_visibility_record_current_owned(L,ordinal);return 1;
}
static int native_control_base_isdisabled_primary_6d2930_owned(lua_State*L){return native_control_base_disabled_modal_read_owned(L,28);}
static int native_control_base_ismodal_primary_70c030_owned(lua_State*L){return native_control_base_disabled_modal_read_owned(L,30);}
static int native_control_base_calculatevisibilitybox_primary_6ad460_owned(lua_State*L){
 native_control_base_flags_visibility_record_current_owned(L,31);
 const int count=lua_gettop(L);native_control_base_flags_visibility_record_current_owned(L,31);
 if(count!=1){(void)lua_gettop(L);native_control_base_flags_visibility_record_current_owned(L,31);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"6AD4B0 CalculateVisibilityBox receiver-only count");}
 const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
 native_control_base_flags_visibility_record_current_owned(L,31);
 if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"6AD493 CalculateVisibilityBox NULL receiver");
 if(!lua_checkstack(L,32))return luaL_error(L,"CalculateVisibilityBox current borrower needs reserve32");
 native_control_base_flags_visibility_record_current_owned(L,31);
 const NativeControlBaseOffsetCommand command=NativeControlBaseVisibilityCalculate{};
 native_control_base_offset_command_member_owned(L,cdata,command);
 native_control_base_flags_visibility_record_current_owned(L,31);return 0;
}
static int native_control_base_flags_visibility_signature_owned(lua_State*L,std::size_t ordinal){
 const auto*const record=native_control_base_flags_visibility_record_current_owned(L,ordinal);char buffer[256]{};
 if(ordinal==27||ordinal==29)
  (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","",record->key,"bool","");
 else if(ordinal==28||ordinal==30)
  (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void ) const","bool","",record->key);
 else (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","void","",record->key);
 native_control_base_flags_visibility_record_current_owned(L,ordinal);lua_pushstring(L,buffer);
 native_control_base_flags_visibility_record_current_owned(L,ordinal);return 1;
}
static int native_control_base_setdisabled_signature_559fe0_owned(lua_State*L){return native_control_base_flags_visibility_signature_owned(L,27);}
static int native_control_base_isdisabled_signature_5dc890_owned(lua_State*L){return native_control_base_flags_visibility_signature_owned(L,28);}
static int native_control_base_setmodal_signature_5ad210_owned(lua_State*L){return native_control_base_flags_visibility_signature_owned(L,29);}
static int native_control_base_ismodal_signature_64a700_owned(lua_State*L){return native_control_base_flags_visibility_signature_owned(L,30);}
static int native_control_base_calculatevisibilitybox_signature_62ce50_owned(lua_State*L){return native_control_base_flags_visibility_signature_owned(L,31);}
