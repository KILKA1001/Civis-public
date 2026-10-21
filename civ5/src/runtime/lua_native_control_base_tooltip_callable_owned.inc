//Inside actual Host::Impl after Source2 reference kernels and source22 geometry.
static const NativeContextDefinition::Record*native_control_base_tooltip_record_current_owned(
 lua_State*L,std::size_t ordinal){
 native_control_base_setoffset_record_current_owned(L);auto&p=owner(L);
 struct Recipe{std::uint32_t writer,key,primary,secondary;int flag;const char*name;};
 static constexpr Recipe exact[]={
  {0x9bea00u,0xa3fa90u,0x5180c0u,0x702570u,0,"SetToolTipCallback"},
  {0x9bea60u,0xa3fad8u,0x453350u,0x75ef70u,1,"IsToolTipEnabled"},
  {0x9beac0u,0xa3fb18u,0x5f8490u,0x5443e0u,0,"EnableToolTip"},
  {0x9beb20u,0xa3fb54u,0x5b2e60u,0x6607a0u,0,"SetToolTipType"},
  {0x9beb80u,0xa3fb98u,0x564ef0u,0x726540u,0,"SetToolTipString"},
  {0x9bebe0u,0xa3fbe8u,0x67e230u,0x5ad3b0u,0,"LocalizeAndSetToolTip"},
  {0x9bec40u,0xa3fc34u,0x74ffa0u,0x4b05f0u,0,"GetToolTipString"},
 };
 if(ordinal<32||ordinal>38)return (luaL_error(L,"Tooltip actual ordinal32..38 required"),nullptr);
 const auto&proof=exact[ordinal-32];const auto&record=p.native_control_base_constructor->definition->records[ordinal];
 if(!record.recipe||!record.key||record.key!=record.recipe->key||record.recipe->writer!=proof.writer||
  record.recipe->key_address!=proof.key||std::strcmp(record.key,proof.name)||record.primary!=proof.primary||
  record.secondary!=proof.secondary||record.primary!=record.recipe->primary||record.secondary!=record.recipe->secondary||
  record.flag!=proof.flag||record.flag!=record.recipe->flag||record.reference!=-1)
  return (luaL_error(L,"Tooltip actual recipe/flag/reference changed"),nullptr);
 return &record;
}
struct NativeControlBaseTooltipCall final {
 using Reference=NativeControlBaseSetOffsetDecodeCall::Reference;
 enum class Stage{entered,function_retained,member_returned,result_pushed,retired};
 const Impl*const host;const std::thread::id creator=std::this_thread::get_id();
 NativeControlBaseSetOffsetDecodeCall*const borrower;const std::size_t ordinal;
 bool alive=true;Reference incoming;std::optional<std::string> text;
 Stage stage=Stage::entered;
 NativeControlBaseTooltipCall(Impl*p,NativeControlBaseSetOffsetDecodeCall*b,std::size_t row):host(p),borrower(b),ordinal(row){
  if(!host||!borrower||(row!=32&&row!=38))throw ScriptError("Tooltip actual call/reference owner absent");
 }
};
static void native_control_base_tooltip_call_current_owned(lua_State*L,const NativeControlBaseTooltipCall&r){
 native_control_base_tooltip_record_current_owned(L,r.ordinal);auto&p=owner(L);
 if(r.host!=&p||r.creator!=std::this_thread::get_id()||!r.alive||!r.borrower||
  r.stage==NativeControlBaseTooltipCall::Stage::retired||
  std::none_of(p.native_control_base_tooltip_calls.begin(),p.native_control_base_tooltip_calls.end(),
   [&](const auto&actual){return actual.get()==&r;})||
  std::none_of(p.native_control_base_setoffset_decode_calls.begin(),p.native_control_base_setoffset_decode_calls.end(),
   [&](const auto&actual){return actual.get()==r.borrower;}))
  return (void)luaL_error(L,"Tooltip SAME Host/call/ref lifetime retired");
 native_control_base_setoffset_decode_full_owned(L,*r.borrower);
 native_control_base_tooltip_record_current_owned(L,r.ordinal);
}
static NativeControlBaseTooltipCall*native_control_base_tooltip_begin_owned(lua_State*L,const void*cdata,std::size_t ordinal){
 auto*const borrower=native_control_base_setoffset_begin_decode_owned(L,cdata);auto&p=owner(L);
 p.source_exception={};p.source_error=nullptr;
 try{p.native_control_base_tooltip_calls.push_back(std::make_unique<NativeControlBaseTooltipCall>(&p,borrower,ordinal));}
 catch(...){p.source_exception=std::current_exception();p.source_error="Tooltip actual retained call custody allocation failed";}
 if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);return nullptr;}
 auto*r=p.native_control_base_tooltip_calls.back().get();native_control_base_tooltip_call_current_owned(L,*r);return r;
}
static void native_control_base_tooltip_read_success_cleanup_owned(lua_State*L,NativeControlBaseTooltipCall&r){
 native_control_base_tooltip_call_current_owned(L,r);auto&p=owner(L);auto*const borrower=r.borrower;
 const auto unbound=[](const auto&ref){return !ref.state&&ref.reference==LUA_NOREF;};
 if(r.ordinal!=38||r.stage!=NativeControlBaseTooltipCall::Stage::result_pushed||lua_gettop(L)!=2||
  borrower->stage!=NativeControlBaseSetOffsetDecodeCall::Stage::entered||borrower->value||
  !unbound(r.incoming)||!unbound(borrower->argument)||!unbound(borrower->key_x)||
  !unbound(borrower->key_y)||!unbound(borrower->temporary))
  return (void)luaL_error(L,"Tooltip success cleanup requires completed read and unbound references");
 std::size_t call_slot=p.native_control_base_tooltip_calls.size(),borrower_slot=p.native_control_base_setoffset_decode_calls.size();
 for(std::size_t i=0;i<p.native_control_base_tooltip_calls.size();++i)
  if(p.native_control_base_tooltip_calls[i].get()==&r)call_slot=i;
 for(std::size_t i=0;i<p.native_control_base_setoffset_decode_calls.size();++i)
  if(p.native_control_base_setoffset_decode_calls[i].get()==borrower)borrower_slot=i;
 if(call_slot==p.native_control_base_tooltip_calls.size()||borrower_slot==p.native_control_base_setoffset_decode_calls.size())
  return (void)luaL_error(L,"Tooltip successful custody cleanup lost exact call owners");
 //No Lua/Source callback/allocation after first erase. Pending/failed calls
 //and their refs keep stable unique_ptr pointees through longjmp/lua_close.
 r.alive=false;r.stage=NativeControlBaseTooltipCall::Stage::retired;
 p.native_control_base_tooltip_calls.erase(p.native_control_base_tooltip_calls.begin()+call_slot);
 borrower->alive=false;borrower->stage=NativeControlBaseSetOffsetDecodeCall::Stage::retired;
 p.native_control_base_setoffset_decode_calls.erase(p.native_control_base_setoffset_decode_calls.begin()+borrower_slot);
}
static void native_control_base_tooltip_member_owned(lua_State*L,const void*cdata,
 ui::BnwWorldViewControlTree::NativeOrdinaryTooltipWrite member,const char*input,int count,bool rooted_argument){
 if(!cdata){
  //4E3D80/73FE50/69ADE0 have no NULL-receiver diagnostic before dereference.
  //The own executable profile requires actual nonNULL CData, not a fake sink.
  luaL_error(L,"Tooltip dynamic CFunction requires actual nonNULL receiver before physical member dereference");std::terminate();
 }
 if(!lua_checkstack(L,32)){luaL_error(L,"Tooltip current physical borrower requires stack reserve32");std::terminate();}
 const NativeControlBaseOffsetCommand command=NativeControlBaseTooltipWrite{member,input,count,rooted_argument};
 native_control_base_offset_command_member_owned(L,cdata,command);
}
static std::size_t native_control_base_tooltip_strnlen4000(const char*text){
 std::size_t length=0;if(text)while(length<0x4000u&&text[length])++length;return length;
}
static int native_control_base_settooltipcallback_primary_5180c0_owned(lua_State*L){
 native_control_base_tooltip_record_current_owned(L,32);
 const int count=lua_gettop(L);native_control_base_tooltip_record_current_owned(L,32);
 if(count!=2){(void)lua_gettop(L);native_control_base_tooltip_record_current_owned(L,32);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E3FD SetToolTipCallback receiver-plus-function count");}
 const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
 native_control_base_tooltip_record_current_owned(L,32);
 if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E3E0 SetToolTipCallback NULL receiver");
 const bool valid=lua_type(L,2)==LUA_TFUNCTION;native_control_base_tooltip_record_current_owned(L,32);
 if(!valid){const int type=lua_type(L,2);native_control_base_tooltip_record_current_owned(L,32);
  (void)lua_typename(L,type);native_control_base_tooltip_record_current_owned(L,32);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"41C8A4 SetToolTipCallback function argument type");}
 auto*r=native_control_base_tooltip_begin_owned(L,cdata,32);r->incoming.state=L;r->incoming.reference=LUA_NOREF;
 (void)lua_checkstack(L,1);native_control_base_tooltip_call_current_owned(L,*r);
 lua_pushvalue(L,2);native_control_base_tooltip_call_current_owned(L,*r);
 r->incoming.reference=luaL_ref(L,LUA_REGISTRYINDEX);native_control_base_tooltip_call_current_owned(L,*r); //8F8500 actual byVALUE8 function.
 r->stage=NativeControlBaseTooltipCall::Stage::function_retained;
 native_control_base_tooltip_member_owned(L,cdata,ui::BnwWorldViewControlTree::NativeOrdinaryTooltipWrite::callback_6dbf40,nullptr,count,false);
 //The reached4C/50 assignment supplier is mandatory; no fabricated return0.
 return luaL_error(L,"SetToolTipCallback actual assignment/destructor return is required");
}
static int native_control_base_istooltipenabled_primary_453350_owned(lua_State*L){
 native_control_base_tooltip_record_current_owned(L,33);
 const int count=lua_gettop(L);native_control_base_tooltip_record_current_owned(L,33);
 if(count!=1){(void)lua_gettop(L);native_control_base_tooltip_record_current_owned(L,33);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"4533AF IsToolTipEnabled receiver-only count");}
 const void*const cdata=native_control_base_const_cdata_89e480_mutable_first_owned(L,1);
 native_control_base_tooltip_record_current_owned(L,33);
 if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"453392 IsToolTipEnabled NULL receiver");
 if(!lua_checkstack(L,32))return luaL_error(L,"IsToolTipEnabled current borrower requires stack reserve32");
 const auto value=native_control_base_uint_member_owned(L,cdata,
  ui::BnwWorldViewControlTree::NativeOrdinaryUintRead::tooltip_enabled_40_bit14_453350);
 native_control_base_tooltip_record_current_owned(L,33);lua_pushboolean(L,value?1:0);
 native_control_base_tooltip_record_current_owned(L,33);return 1;
}
static int native_control_base_enabletooltip_primary_5f8490_owned(lua_State*L){
 native_control_base_tooltip_record_current_owned(L,34);
 const int count=lua_gettop(L);native_control_base_tooltip_record_current_owned(L,34);
 if(count!=2){(void)lua_gettop(L);native_control_base_tooltip_record_current_owned(L,34);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E33D EnableToolTip receiver-plus-argument count");}
 const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
 native_control_base_tooltip_record_current_owned(L,34);
 if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E322 EnableToolTip NULL receiver");
 const bool value=lua_toboolean(L,2)!=0;native_control_base_tooltip_record_current_owned(L,34);
 if(!lua_checkstack(L,32))return luaL_error(L,"EnableToolTip current borrower requires stack reserve32");
 const NativeControlBaseOffsetCommand command=NativeControlBaseBooleanFlag{
  ui::BnwWorldViewControlTree::NativeOrdinaryFlagWrite::tooltip_enable_692040,value};
 native_control_base_offset_command_member_owned(L,cdata,command);
 native_control_base_tooltip_record_current_owned(L,34);return 0;
}
static int native_control_base_settooltiptype_primary_5b2e60_owned(lua_State*L){
 native_control_base_tooltip_record_current_owned(L,35);
 const int count=lua_gettop(L);native_control_base_tooltip_record_current_owned(L,35);
 if(!count)return 0; //Actual4E3D8D zero-argument return skips receiver resolution.
 const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
 native_control_base_tooltip_record_current_owned(L,35);
 const char*text=nullptr;
 if(count>1){text=lua_tolstring(L,2,nullptr);native_control_base_tooltip_record_current_owned(L,35);}
 native_control_base_tooltip_member_owned(L,cdata,ui::BnwWorldViewControlTree::NativeOrdinaryTooltipWrite::type_47cb60_4e3d80,text,count,count>1);
 native_control_base_tooltip_record_current_owned(L,35);return 0; //Extra args genuinely ignored.
}
static int native_control_base_settooltipstring_primary_564ef0_owned(lua_State*L){
 native_control_base_tooltip_record_current_owned(L,36);
 const int count=lua_gettop(L);native_control_base_tooltip_record_current_owned(L,36);
 if(!count)return 0; //Actual73FE5D zero-argument branch.
 const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
 native_control_base_tooltip_record_current_owned(L,36);
 const char*text=nullptr;
 if(count>1){text=lua_tolstring(L,2,nullptr);native_control_base_tooltip_record_current_owned(L,36);}
 const auto member=text&&native_control_base_tooltip_strnlen4000(text)?
  ui::BnwWorldViewControlTree::NativeOrdinaryTooltipWrite::text_5f3b90:
  ui::BnwWorldViewControlTree::NativeOrdinaryTooltipWrite::clear_73fe50_69ade0;
 native_control_base_tooltip_member_owned(L,cdata,member,text,count,count>1);
 native_control_base_tooltip_record_current_owned(L,36);return 0;
}
static int native_control_base_localizeandsettooltip_primary_67e230_owned(lua_State*L){
 native_control_base_tooltip_record_current_owned(L,37);
 //Actual69ADE0 resolves receiver FIRST, even when original gettop would be0.
 const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
 native_control_base_tooltip_record_current_owned(L,37);
 const int count=lua_gettop(L);native_control_base_tooltip_record_current_owned(L,37);
 const char*key=nullptr;
 if(count>1){const int type=lua_type(L,2);native_control_base_tooltip_record_current_owned(L,37);
  if(type!=LUA_TNIL){key=luaL_checklstring(L,2,nullptr);native_control_base_tooltip_record_current_owned(L,37);}}
 const auto member=key&&native_control_base_tooltip_strnlen4000(key)?
  ui::BnwWorldViewControlTree::NativeOrdinaryTooltipWrite::localization_lookup_69ae5a:
  ui::BnwWorldViewControlTree::NativeOrdinaryTooltipWrite::clear_73fe50_69ade0;
 native_control_base_tooltip_member_owned(L,cdata,member,key,count,count>1);
 native_control_base_tooltip_record_current_owned(L,37);return 0;
}
static int native_control_base_gettooltipstring_primary_74ffa0_owned(lua_State*L){
 native_control_base_tooltip_record_current_owned(L,38);
 const int count=lua_gettop(L);native_control_base_tooltip_record_current_owned(L,38);
 if(count!=1){(void)lua_gettop(L);native_control_base_tooltip_record_current_owned(L,38);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"750013 GetToolTipString receiver-only count");}
 const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1); //Actual MUTABLE89E050, not const getter helper.
 native_control_base_tooltip_record_current_owned(L,38);
 if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"74FFF6 GetToolTipString NULL receiver");
 auto*r=native_control_base_tooltip_begin_owned(L,cdata,38);
 if(!lua_checkstack(L,32))return luaL_error(L,"GetToolTipString current borrower requires stack reserve32");
 const NativeControlBaseOffsetCommand command=NativeControlBaseTooltipRead{&r->text};
 native_control_base_offset_command_member_owned(L,cdata,command);
 native_control_base_tooltip_call_current_owned(L,*r);r->stage=NativeControlBaseTooltipCall::Stage::member_returned;
 //Real54NULL becomes Lua nil. Engaged empty native text remains an empty string.
 lua_pushstring(L,r->text?r->text->c_str():nullptr);native_control_base_tooltip_call_current_owned(L,*r);
 r->stage=NativeControlBaseTooltipCall::Stage::result_pushed;
 native_control_base_tooltip_read_success_cleanup_owned(L,*r);return 1;
}
static int native_control_base_tooltip_signature_owned(lua_State*L,std::size_t ordinal){
 const auto*const record=native_control_base_tooltip_record_current_owned(L,ordinal);char buffer[256]{};
 if(ordinal==32)(void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","",record->key,"function","");
 else if(ordinal==33)(void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void ) const","bool","",record->key);
 else if(ordinal==34)(void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","",record->key,"bool","");
 else if(ordinal<=37)(void)::sprintf_s(buffer,sizeof(buffer),"CFunction: %s",record->key); //Actual A40C50 literal.
 else (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","const char","*",record->key);
 native_control_base_tooltip_record_current_owned(L,ordinal);lua_pushstring(L,buffer);
 native_control_base_tooltip_record_current_owned(L,ordinal);return 1;
}
static int native_control_base_settooltipcallback_signature_702570_owned(lua_State*L){return native_control_base_tooltip_signature_owned(L,32);}
static int native_control_base_istooltipenabled_signature_75ef70_owned(lua_State*L){return native_control_base_tooltip_signature_owned(L,33);}
static int native_control_base_enabletooltip_signature_5443e0_owned(lua_State*L){return native_control_base_tooltip_signature_owned(L,34);}
static int native_control_base_settooltiptype_signature_6607a0_owned(lua_State*L){return native_control_base_tooltip_signature_owned(L,35);}
static int native_control_base_settooltipstring_signature_726540_owned(lua_State*L){return native_control_base_tooltip_signature_owned(L,36);}
static int native_control_base_localizeandsettooltip_signature_5ad3b0_owned(lua_State*L){return native_control_base_tooltip_signature_owned(L,37);}
static int native_control_base_gettooltipstring_signature_4b05f0_owned(lua_State*L){return native_control_base_tooltip_signature_owned(L,38);}
void native_control_base_tooltip_retire()noexcept{
 for(const auto&r:native_control_base_tooltip_calls)if(r){
  if(r->host!=this||r->creator!=std::this_thread::get_id())std::terminate();
  r->alive=false;r->stage=NativeControlBaseTooltipCall::Stage::retired;
 }
 //Pending actual function refs and failed getter snapshots remain through
 //lua_close in SAME Host-owned custody. Successful reads are erased promptly.
}
