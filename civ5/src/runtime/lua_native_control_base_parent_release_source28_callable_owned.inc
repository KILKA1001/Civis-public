//Source28 keeps exact39/41 decoder/signatures and adds real48 receiver-only primary.
static const NativeContextDefinition::Record*native_control_base_parent_release_record_current_owned(
 lua_State*L,std::size_t ordinal){
 native_control_base_setoffset_record_current_owned(L);auto&p=owner(L);
 struct Recipe{std::size_t ordinal;std::uint32_t writer,key,primary,secondary;const char*name;};
 static constexpr Recipe exact[]={
  {39,0x9beca0u,0xa3fc74u,0x661630u,0x4d1410u,"ChangeParent"},
  {41,0x9bed60u,0xa3fcecu,0x43cbf0u,0x436250u,"ReleaseChild"},
  {48,0x9bf000u,0xa3ff18u,0x6fbb50u,0x7a0d50u,"DestroyAllChildren"},
 };
 const Recipe*proof=nullptr;for(const auto&candidate:exact)if(candidate.ordinal==ordinal)proof=&candidate;
 if(!proof)return (luaL_error(L,"Native parent/release actual ordinal required"),nullptr);
 const auto&record=p.native_control_base_constructor->definition->records[ordinal];
 if(!record.recipe||!record.key||record.key!=record.recipe->key||record.recipe->writer!=proof->writer||
  record.recipe->key_address!=proof->key||std::strcmp(record.key,proof->name)||record.primary!=proof->primary||
  record.secondary!=proof->secondary||record.primary!=record.recipe->primary||record.secondary!=record.recipe->secondary||
  record.flag!=0||record.flag!=record.recipe->flag||record.reference!=-1)
  return (luaL_error(L,"Native parent/release actual recipe/flag/reference changed"),nullptr);
 return &record;
}
static int native_control_base_parent_argument_89e410_owned(lua_State*L,std::size_t ordinal){
 native_control_base_parent_release_record_current_owned(L,ordinal);
 const int count=lua_gettop(L);native_control_base_parent_release_record_current_owned(L,ordinal);
 if(count!=2){(void)lua_gettop(L);native_control_base_parent_release_record_current_owned(L,ordinal);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E46E parent/release receiver-plus-pointer count");}
 const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
 native_control_base_parent_release_record_current_owned(L,ordinal);
 if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E454 parent/release NULL THIS receiver");
 //Actual89E43D resolves arg2 only after THIS passed. No arg2 NULL predicate:
 //lightuserdataNULL is allowed; Lua nil still invokes real pointer-type error.
 const void*const argument=native_control_base_setoffset_cdata_89e050_owned(L,2);
 native_control_base_parent_release_record_current_owned(L,ordinal);
 if(!lua_checkstack(L,32))return luaL_error(L,"Native parent/release current borrower requires reserve32");
 using Member=ui::BnwWorldViewControlTree::NativeOrdinaryChildrenMember;
 const NativeControlBaseOffsetCommand command=NativeControlBaseChildrenCommand{
  ordinal==39?Member::change_parent_5335e0:Member::release_child_6ab0d0,argument};
 native_control_base_offset_command_member_owned(L,cdata,command);
 native_control_base_parent_release_record_current_owned(L,ordinal);return 0;
}
static int native_control_base_changeparent_primary_661630_owned(lua_State*L){return native_control_base_parent_argument_89e410_owned(L,39);}
static int native_control_base_releasechild_primary_43cbf0_owned(lua_State*L){return native_control_base_parent_argument_89e410_owned(L,41);}
static int native_control_base_parent_release_signature_owned(lua_State*L,std::size_t ordinal){
 const auto*const record=native_control_base_parent_release_record_current_owned(L,ordinal);char buffer[256]{};
 {
  //Original4D1410/436250 read real C809E0, initialized by SAME Base ctor.
  //Do not insert a hardcoded class name or const descriptor fallback here.
  const auto*const type=owner(L).native_control_base_constructor->mutable_name_c809e0;
  if(!type)return luaL_error(L,"Native parent/release actual mutable typenameC809E0 owner missing");
  (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","",record->key,type,"*");
 }
 native_control_base_parent_release_record_current_owned(L,ordinal);lua_pushstring(L,buffer);
 native_control_base_parent_release_record_current_owned(L,ordinal);return 1;
}
static int native_control_base_changeparent_signature_4d1410_owned(lua_State*L){return native_control_base_parent_release_signature_owned(L,39);}
static int native_control_base_releasechild_signature_436250_owned(lua_State*L){return native_control_base_parent_release_signature_owned(L,41);}

static int native_control_base_destroyallchildren_primary_6fbb50_owned(lua_State*L){
 native_control_base_parent_release_record_current_owned(L,48);
 const int count=lua_gettop(L);native_control_base_parent_release_record_current_owned(L,48);
 if(count!=1){(void)lua_gettop(L);native_control_base_parent_release_record_current_owned(L,48);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"6FBB8F DestroyAllChildren receiver-only count");}
 const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
 native_control_base_parent_release_record_current_owned(L,48);
 if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"6FBB79 DestroyAllChildren NULL THIS receiver");
 if(!lua_checkstack(L,32))return luaL_error(L,"DestroyAllChildren current borrower requires reserve32");
 const NativeControlBaseOffsetCommand command=NativeControlBaseChildrenCommand{
  ui::BnwWorldViewControlTree::NativeOrdinaryChildrenMember::destroy_all_children_781ff0,nullptr};
 native_control_base_offset_command_member_owned(L,cdata,command);
 native_control_base_parent_release_record_current_owned(L,48);return 0;
}
static int native_control_base_destroyallchildren_signature_7a0d50_owned(lua_State*L){
 const auto*const record=native_control_base_parent_release_record_current_owned(L,48);char buffer[256]{};
 //NativeA40670 "%s%s %s( void )": void+empty suffix+actual method key.
 (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","void","",record->key);
 native_control_base_parent_release_record_current_owned(L,48);lua_pushstring(L,buffer);
 native_control_base_parent_release_record_current_owned(L,48);return 1;
}
