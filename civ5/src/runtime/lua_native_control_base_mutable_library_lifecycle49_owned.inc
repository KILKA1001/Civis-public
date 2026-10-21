//Source28 publishes actual48 primary/signature and returns the genuine
//47C500 mutable table after all49 original closure/CodeBuddy/key stores.
//Invoked bit8 child destruction remains strict; publication does not invoke it.
//Qualified47C500/663140/8F7E20 and direct4E35C0 never write original
//record+4. Host registry roots are separate owners; constructor -1 remains
//the actual field value through these registrars, not their retained refs.
struct NativeControlBaseMutableLibraryPrefix final {
 enum class Stage {entered,table_created,table_retained,collecting,next_primary_required,returned,retired};
 struct Row final {
  const void*primary=nullptr,*secondary=nullptr;
  int primary_ref=LUA_NOREF,secondary_ref=LUA_NOREF,key_ref=LUA_NOREF;
  std::shared_ptr<const NativeOrdinaryCodeBuddyTableReturn> codebuddy;
 };
 const Impl*const host;
 const std::thread::id creator=std::this_thread::get_id();
 bool alive=true;
 const std::shared_ptr<const NativeControlBaseConstructor> constructor;
 const std::shared_ptr<const NativeContextDefinition> definition;
 const void*const offered_metatype;
 const void*table=nullptr;
 int metatype_ref=LUA_NOREF,table_ref=LUA_NOREF;
 std::array<Row,49> rows;
 std::size_t assigned=0;
 Stage stage=Stage::entered;
 NativeControlBaseMutableLibraryPrefix(const Impl*p,std::shared_ptr<const NativeControlBaseConstructor>c,
  std::shared_ptr<const NativeContextDefinition>d,const void*actual)
  :host(p),constructor(std::move(c)),definition(std::move(d)),offered_metatype(actual){
  if(!host||!constructor||!definition||!offered_metatype)
   throw ScriptError("ControlBase47C500 offset block lacks its actual caller/storage");
 }
};
//Registration-only data validation: the entered FLua owner owns metadata
//and Lua tables. Every callable consumer retains its physical CData borrower.
static const NativeContextDefinition*native_control_base_mutable_library_source_owned(lua_State*L){
 auto&p=owner(L);
 const bool metadata=native_flua_exposure_metadata_current_if_entered(L,0xcb7c68u);
 if(!metadata)native_postread_root_cdata_full(L); //Established ordinary caller.
 p.source_exception={};p.source_error=nullptr;
 try{
  const auto*c=p.native_control_base_constructor.get();
  if(!c||c->host!=&p||c->queue!=p.native_definition_deferred_queue||
   c->definition!=p.native_context_definitions[1]||!c->canonical||
   c->canonical!=p.native_ordinary_nested_descriptors[0]||!c->canonical->alive.load()||
   c->canonical->host!=&p||c->canonical->creator!=std::this_thread::get_id()||
   c->canonical->kind!=NativeOrdinaryNestedDescriptorReturn::Kind::descriptor_cb7c68||
   c->canonical->actual_descriptor!=0xcb7c68u||c->canonical->actual_ctor!=0x5210f0u)
   throw ScriptError("ControlBase registration lost SAME actual constructor/descriptor");
  c->require_returned(); //Real stores/49 writers/queue/name, no Control field read.
 }catch(...){p.source_exception=std::current_exception();p.source_error="ControlBase registration actual constructor FIRST refused";}
 if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);return nullptr;}
 const auto*d=p.native_control_base_constructor->definition.get();
 if(!d||d->host!=&p||!d->alive.load()||d->creator!=std::this_thread::get_id()||
  d->context||d->descriptor!=0xcb7c68u||d->constructor!=0x5210f0u||
  d->stage!=NativeContextDefinition::Stage::metadata_returned||d->count544!=49u||
  d->name0!="ControlBase"||d->name4!="const ControlBase")
  return (luaL_error(L,"ControlBase registration actual returned definition changed"),nullptr);
 for(std::size_t i=0;i<d->count544;++i){const auto&r=d->records[i];
  if(!r.recipe||!r.key||r.key!=r.recipe->key||r.primary!=r.recipe->primary||
   r.secondary!=r.recipe->secondary||r.flag!=r.recipe->flag||r.reference!=-1)
   return (luaL_error(L,"ControlBase registration constructor-owned method tuple changed"),nullptr);
 }
 if(!metadata)native_control_base_current_definition_read_owned(L,*d);
 else if(!native_flua_exposure_metadata_current_if_entered(L,0xcb7c68u))
  return (luaL_error(L,"ControlBase entered metadata registrar retired during data read"),nullptr);
 return d;
}
static const NativeContextDefinition::Record*native_control_base_lifecycle49_record_owned(lua_State*L,std::size_t ordinal){
 const auto*d=native_control_base_mutable_library_source_owned(L);
 if(ordinal>=d->count544)return (luaL_error(L,"ControlBase registration actual method ordinal exceeded49"),nullptr);
 return &d->records[ordinal]; //Closure creation never calls a method decoder.
}
static lua_CFunction native_control_base_lifecycle49_primary_owned(lua_State*L,std::size_t ordinal){
 native_control_base_lifecycle49_record_owned(L,ordinal);
 //Actual executing x64 CFunctions, never Original addresses cast into code.
 switch(ordinal){
  case 0:return &Impl::native_control_base_setoffset_primary_4e4eb0_owned;
  case 1:return &Impl::native_control_base_setoffset_x_primary_6c44b0_owned;
  case 2:return &Impl::native_control_base_setoffset_y_primary_410610_owned;
  case 3:return &Impl::native_control_base_setoffset_val_primary_688850_owned;
  case 4:return &Impl::native_control_base_getoffset_x_primary_51a6a0_owned;
  case 5:return &Impl::native_control_base_getoffset_y_primary_6bb2e0_owned;
  case 6:return &Impl::native_control_base_getoffset_val_primary_6cba70_owned;
  case 7:return &Impl::native_control_base_setanchor_primary_5ab320_owned;
  case 8:return &Impl::native_control_base_sethide_primary_7a54b0_owned;
  case 9:return &Impl::native_control_base_setnoclip_primary_723910_owned;
  case 10:return &Impl::native_control_base_ishidden_primary_4c4bd0_owned;
  case 11:return &Impl::native_control_base_getnumchildren_primary_47e630_owned;
  case 12:return &Impl::native_control_base_reprocessanchoring_primary_4f7570_owned;
  case 13:return &Impl::native_control_base_getsize_primary_537c60_owned;
  case 14:return &Impl::native_control_base_getsize_x_primary_600630_owned;
  case 15:return &Impl::native_control_base_getsize_y_primary_421820_owned;
  case 16:return &Impl::native_control_base_getsize_val_primary_49f370_owned;
  case 17:return &Impl::native_control_base_setsize_primary_676030_owned;
  case 18:return &Impl::native_control_base_setsize_x_primary_4bad50_owned;
  case 19:return &Impl::native_control_base_setsize_y_primary_57a330_owned;
  case 20:return &Impl::native_control_base_setsize_val_primary_6eee90_owned;
  case 21:return &Impl::native_control_base_setcolor_primary_6b4b50_owned;
  case 22:return &Impl::native_control_base_setalpha_primary_766e70_owned;
  case 23:return &Impl::native_control_base_getalpha_primary_78f870_owned;
  case 24:return &Impl::native_control_base_sortchildren_primary_5bf0b0_owned;
  case 25:return &Impl::native_control_base_branchresetanimation_primary_75eb20_owned;
  case 26:return &Impl::native_control_base_getid_primary_4dd890_owned;
  case 27:return &Impl::native_control_base_setdisabled_primary_5acf00_owned;
  case 28:return &Impl::native_control_base_isdisabled_primary_6d2930_owned;
  case 29:return &Impl::native_control_base_setmodal_primary_58e600_owned;
  case 30:return &Impl::native_control_base_ismodal_primary_70c030_owned;
  case 31:return &Impl::native_control_base_calculatevisibilitybox_primary_6ad460_owned;
  case 32:return &Impl::native_control_base_settooltipcallback_primary_5180c0_owned;
  case 33:return &Impl::native_control_base_istooltipenabled_primary_453350_owned;
  case 34:return &Impl::native_control_base_enabletooltip_primary_5f8490_owned;
  case 35:return &Impl::native_control_base_settooltiptype_primary_5b2e60_owned;
  case 36:return &Impl::native_control_base_settooltipstring_primary_564ef0_owned;
  case 37:return &Impl::native_control_base_localizeandsettooltip_primary_67e230_owned;
  case 38:return &Impl::native_control_base_gettooltipstring_primary_74ffa0_owned;
  case 39:return &Impl::native_control_base_changeparent_primary_661630_owned;
  case 40:return &Impl::native_control_base_hasmouseover_primary_669850_owned;
  case 41:return &Impl::native_control_base_releasechild_primary_43cbf0_owned;
  case 42:return &Impl::native_control_base_setconsume_mouse_over_primary_48edf0_owned;
  case 43:return &Impl::native_control_base_setconsume_mouse_button_primary_624b60_owned;
  case 44:return &Impl::native_control_base_setconsume_mouse_wheel_primary_62af80_owned;
  case 45:return &Impl::native_control_base_getconsume_mouse_over_primary_75d480_owned;
  case 46:return &Impl::native_control_base_getconsume_mouse_button_primary_6d8390_owned;
  case 47:return &Impl::native_control_base_getconsume_mouse_wheel_primary_488050_owned;
  case 48:return &Impl::native_control_base_destroyallchildren_primary_6fbb50_owned;
 }
 return (luaL_error(L,"ControlBase unissued lifecycle49 primary ordinal"),nullptr);
}
static lua_CFunction native_control_base_lifecycle49_secondary_owned(lua_State*L,std::size_t ordinal){
 native_control_base_lifecycle49_record_owned(L,ordinal);
 switch(ordinal){
  case 0:return &Impl::native_control_base_setoffset_signature_43c790_owned;
  case 1:return &Impl::native_control_base_setoffset_x_signature_49d160_owned;
  case 2:return &Impl::native_control_base_setoffset_y_signature_5775b0_owned;
  case 3:return &Impl::native_control_base_setoffset_val_signature_53f760_owned;
  case 4:return &Impl::native_control_base_getoffset_x_signature_581a50_owned;
  case 5:return &Impl::native_control_base_getoffset_y_signature_5b2660_owned;
  case 6:return &Impl::native_control_base_getoffset_val_signature_4161c0_owned;
  case 7:return &Impl::native_control_base_setanchor_signature_461820_owned;
  case 8:return &Impl::native_control_base_sethide_signature_741e00_owned;
  case 9:return &Impl::native_control_base_setnoclip_signature_740110_owned;
  case 10:return &Impl::native_control_base_ishidden_signature_4b0730_owned;
  case 11:return &Impl::native_control_base_getnumchildren_signature_687190_owned;
  case 12:return &Impl::native_control_base_reprocessanchoring_signature_43f790_owned;
  case 13:return &Impl::native_control_base_getsize_signature_782fb0_owned;
  case 14:return &Impl::native_control_base_getsize_x_signature_69fec0_owned;
  case 15:return &Impl::native_control_base_getsize_y_signature_570d40_owned;
  case 16:return &Impl::native_control_base_getsize_val_signature_75b3c0_owned;
  case 17:return &Impl::native_control_base_setsize_signature_5c07b0_owned;
  case 18:return &Impl::native_control_base_setsize_x_signature_4c6990_owned;
  case 19:return &Impl::native_control_base_setsize_y_signature_407840_owned;
  case 20:return &Impl::native_control_base_setsize_val_signature_5c70f0_owned;
  case 21:return &Impl::native_control_base_setcolor_signature_526620_owned;
  case 22:return &Impl::native_control_base_setalpha_signature_6d8d40_owned;
  case 23:return &Impl::native_control_base_getalpha_signature_4ec6e0_owned;
  case 24:return &Impl::native_control_base_sortchildren_signature_6c2210_owned;
  case 25:return &Impl::native_control_base_branchresetanimation_signature_5404b0_owned;
  case 26:return &Impl::native_control_base_getid_signature_685430_owned;
  case 27:return &Impl::native_control_base_setdisabled_signature_559fe0_owned;
  case 28:return &Impl::native_control_base_isdisabled_signature_5dc890_owned;
  case 29:return &Impl::native_control_base_setmodal_signature_5ad210_owned;
  case 30:return &Impl::native_control_base_ismodal_signature_64a700_owned;
  case 31:return &Impl::native_control_base_calculatevisibilitybox_signature_62ce50_owned;
  case 32:return &Impl::native_control_base_settooltipcallback_signature_702570_owned;
  case 33:return &Impl::native_control_base_istooltipenabled_signature_75ef70_owned;
  case 34:return &Impl::native_control_base_enabletooltip_signature_5443e0_owned;
  case 35:return &Impl::native_control_base_settooltiptype_signature_6607a0_owned;
  case 36:return &Impl::native_control_base_settooltipstring_signature_726540_owned;
  case 37:return &Impl::native_control_base_localizeandsettooltip_signature_5ad3b0_owned;
  case 38:return &Impl::native_control_base_gettooltipstring_signature_4b05f0_owned;
  case 39:return &Impl::native_control_base_changeparent_signature_4d1410_owned;
  case 40:return &Impl::native_control_base_hasmouseover_signature_48a020_owned;
  case 41:return &Impl::native_control_base_releasechild_signature_436250_owned;
  case 42:return &Impl::native_control_base_setconsume_mouse_over_signature_613ac0_owned;
  case 43:return &Impl::native_control_base_setconsume_mouse_button_signature_5fd300_owned;
  case 44:return &Impl::native_control_base_setconsume_mouse_wheel_signature_770eb0_owned;
  case 45:return &Impl::native_control_base_getconsume_mouse_over_signature_520f90_owned;
  case 46:return &Impl::native_control_base_getconsume_mouse_button_signature_5f3680_owned;
  case 47:return &Impl::native_control_base_getconsume_mouse_wheel_signature_4c7cb0_owned;
  case 48:return &Impl::native_control_base_destroyallchildren_signature_7a0d50_owned;
 }
 return (luaL_error(L,"ControlBase unissued lifecycle49 secondary ordinal"),nullptr);
}
static void native_control_base_lifecycle49_associate_8f7e20_owned(
 lua_State*L,NativeControlBaseMutableLibraryPrefix&r,std::size_t ordinal){
 auto&p=owner(L);native_control_base_lifecycle49_record_owned(L,ordinal);
 auto&row=r.rows.at(ordinal);
 native_ordinary_push_codebuddy_table_8f7d40_owned(L);native_control_base_mutable_library_source_owned(L);
 for(const auto&actual:p.native_ordinary_codebuddy_tables)
  if(actual&&native_ordinary_codebuddy_table_physical_without_callbacks(p,L,*actual,-1)){
   row.codebuddy=actual;break;
  }
 if(!row.codebuddy)return (void)luaL_error(L,"ControlBase lifecycle49 genuine current8F7D40 return missing");
 (void)lua_checkstack(L,2);native_control_base_mutable_library_source_owned(L);
 lua_pushvalue(L,-2);native_control_base_mutable_library_source_owned(L); //Actual8F7E36 primary key.
 lua_pushcclosure(L,native_control_base_lifecycle49_secondary_owned(L,ordinal),0);
 native_control_base_mutable_library_source_owned(L); //Actual8F7E43 secondary closure0.
 row.secondary=lua_topointer(L,-1);
 lua_pushvalue(L,-1);native_control_base_mutable_library_source_owned(L);
 row.secondary_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_control_base_mutable_library_source_owned(L);
 lua_settable(L,-3);native_control_base_mutable_library_source_owned(L); //Actual8F7E4B.
 lua_settop(L,-2);native_control_base_mutable_library_source_owned(L); //Actual8F7E53 pop slot1 table.
}
static bool native_control_base_lifecycle49_physical_without_callbacks(
 Impl&p,lua_State*L,const NativeControlBaseMutableLibraryPrefix&r,int offered){
 if(!L||!p.state||r.host!=&p||!r.alive||r.creator!=std::this_thread::get_id()||
  r.constructor!=p.native_control_base_constructor||r.definition!=p.native_context_definitions[1]||
  !r.table||!r.offered_metatype||r.metatype_ref<0||r.table_ref<0||!r.assigned||r.assigned>49||
  (r.stage!=NativeControlBaseMutableLibraryPrefix::Stage::collecting&&
   r.stage!=NativeControlBaseMutableLibraryPrefix::Stage::next_primary_required&&
   r.stage!=NativeControlBaseMutableLibraryPrefix::Stage::returned))return false;
 const int top=lua_gettop(L);if(offered<0)offered=top+offered+1;
 const auto stop=[&](bool value){lua_settop(L,top);return value;};
 if(offered<=1||lua_type(L,offered)!=LUA_TTABLE||lua_topointer(L,offered)!=r.table||
  lua_type(L,offered-1)!=LUA_TTABLE||lua_topointer(L,offered-1)!=r.offered_metatype)return false;
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.metatype_ref);
 if(lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r.offered_metatype||!lua_rawequal(L,-1,offered-1))return stop(false);
 lua_settop(L,top);
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.table_ref);const int methods=lua_gettop(L);
 if(lua_type(L,methods)!=LUA_TTABLE||lua_topointer(L,methods)!=r.table||!lua_rawequal(L,methods,offered))return stop(false);
 //No source getter/allocator/metamethod/interning in this physical cut.
 const lua_CFunction primary[]={&Impl::native_control_base_setoffset_primary_4e4eb0_owned,
  &Impl::native_control_base_setoffset_x_primary_6c44b0_owned,
  &Impl::native_control_base_setoffset_y_primary_410610_owned,
  &Impl::native_control_base_setoffset_val_primary_688850_owned,
  &Impl::native_control_base_getoffset_x_primary_51a6a0_owned,
  &Impl::native_control_base_getoffset_y_primary_6bb2e0_owned,
  &Impl::native_control_base_getoffset_val_primary_6cba70_owned,
  &Impl::native_control_base_setanchor_primary_5ab320_owned,
  &Impl::native_control_base_sethide_primary_7a54b0_owned,
  &Impl::native_control_base_setnoclip_primary_723910_owned,
  &Impl::native_control_base_ishidden_primary_4c4bd0_owned,
  &Impl::native_control_base_getnumchildren_primary_47e630_owned,
  &Impl::native_control_base_reprocessanchoring_primary_4f7570_owned,
  &Impl::native_control_base_getsize_primary_537c60_owned,
  &Impl::native_control_base_getsize_x_primary_600630_owned,
  &Impl::native_control_base_getsize_y_primary_421820_owned,
  &Impl::native_control_base_getsize_val_primary_49f370_owned,
  &Impl::native_control_base_setsize_primary_676030_owned,
  &Impl::native_control_base_setsize_x_primary_4bad50_owned,
  &Impl::native_control_base_setsize_y_primary_57a330_owned,
  &Impl::native_control_base_setsize_val_primary_6eee90_owned,
  &Impl::native_control_base_setcolor_primary_6b4b50_owned,
  &Impl::native_control_base_setalpha_primary_766e70_owned,
  &Impl::native_control_base_getalpha_primary_78f870_owned,
  &Impl::native_control_base_sortchildren_primary_5bf0b0_owned,
  &Impl::native_control_base_branchresetanimation_primary_75eb20_owned,
  &Impl::native_control_base_getid_primary_4dd890_owned,
  &Impl::native_control_base_setdisabled_primary_5acf00_owned,
  &Impl::native_control_base_isdisabled_primary_6d2930_owned,
  &Impl::native_control_base_setmodal_primary_58e600_owned,
  &Impl::native_control_base_ismodal_primary_70c030_owned,
  &Impl::native_control_base_calculatevisibilitybox_primary_6ad460_owned,
  &Impl::native_control_base_settooltipcallback_primary_5180c0_owned,
  &Impl::native_control_base_istooltipenabled_primary_453350_owned,
  &Impl::native_control_base_enabletooltip_primary_5f8490_owned,
  &Impl::native_control_base_settooltiptype_primary_5b2e60_owned,
  &Impl::native_control_base_settooltipstring_primary_564ef0_owned,
  &Impl::native_control_base_localizeandsettooltip_primary_67e230_owned,
  &Impl::native_control_base_gettooltipstring_primary_74ffa0_owned,
  &Impl::native_control_base_changeparent_primary_661630_owned,
  &Impl::native_control_base_hasmouseover_primary_669850_owned,
  &Impl::native_control_base_releasechild_primary_43cbf0_owned,
  &Impl::native_control_base_setconsume_mouse_over_primary_48edf0_owned,
  &Impl::native_control_base_setconsume_mouse_button_primary_624b60_owned,
  &Impl::native_control_base_setconsume_mouse_wheel_primary_62af80_owned,
  &Impl::native_control_base_getconsume_mouse_over_primary_75d480_owned,
  &Impl::native_control_base_getconsume_mouse_button_primary_6d8390_owned,
  &Impl::native_control_base_getconsume_mouse_wheel_primary_488050_owned,
  &Impl::native_control_base_destroyallchildren_primary_6fbb50_owned};
 const lua_CFunction secondary[]={&Impl::native_control_base_setoffset_signature_43c790_owned,
  &Impl::native_control_base_setoffset_x_signature_49d160_owned,
  &Impl::native_control_base_setoffset_y_signature_5775b0_owned,
  &Impl::native_control_base_setoffset_val_signature_53f760_owned,
  &Impl::native_control_base_getoffset_x_signature_581a50_owned,
  &Impl::native_control_base_getoffset_y_signature_5b2660_owned,
  &Impl::native_control_base_getoffset_val_signature_4161c0_owned,
  &Impl::native_control_base_setanchor_signature_461820_owned,
  &Impl::native_control_base_sethide_signature_741e00_owned,
  &Impl::native_control_base_setnoclip_signature_740110_owned,
  &Impl::native_control_base_ishidden_signature_4b0730_owned,
  &Impl::native_control_base_getnumchildren_signature_687190_owned,
  &Impl::native_control_base_reprocessanchoring_signature_43f790_owned,
  &Impl::native_control_base_getsize_signature_782fb0_owned,
  &Impl::native_control_base_getsize_x_signature_69fec0_owned,
  &Impl::native_control_base_getsize_y_signature_570d40_owned,
  &Impl::native_control_base_getsize_val_signature_75b3c0_owned,
  &Impl::native_control_base_setsize_signature_5c07b0_owned,
  &Impl::native_control_base_setsize_x_signature_4c6990_owned,
  &Impl::native_control_base_setsize_y_signature_407840_owned,
  &Impl::native_control_base_setsize_val_signature_5c70f0_owned,
  &Impl::native_control_base_setcolor_signature_526620_owned,
  &Impl::native_control_base_setalpha_signature_6d8d40_owned,
  &Impl::native_control_base_getalpha_signature_4ec6e0_owned,
  &Impl::native_control_base_sortchildren_signature_6c2210_owned,
  &Impl::native_control_base_branchresetanimation_signature_5404b0_owned,
  &Impl::native_control_base_getid_signature_685430_owned,
  &Impl::native_control_base_setdisabled_signature_559fe0_owned,
  &Impl::native_control_base_isdisabled_signature_5dc890_owned,
  &Impl::native_control_base_setmodal_signature_5ad210_owned,
  &Impl::native_control_base_ismodal_signature_64a700_owned,
  &Impl::native_control_base_calculatevisibilitybox_signature_62ce50_owned,
  &Impl::native_control_base_settooltipcallback_signature_702570_owned,
  &Impl::native_control_base_istooltipenabled_signature_75ef70_owned,
  &Impl::native_control_base_enabletooltip_signature_5443e0_owned,
  &Impl::native_control_base_settooltiptype_signature_6607a0_owned,
  &Impl::native_control_base_settooltipstring_signature_726540_owned,
  &Impl::native_control_base_localizeandsettooltip_signature_5ad3b0_owned,
  &Impl::native_control_base_gettooltipstring_signature_4b05f0_owned,
  &Impl::native_control_base_changeparent_signature_4d1410_owned,
  &Impl::native_control_base_hasmouseover_signature_48a020_owned,
  &Impl::native_control_base_releasechild_signature_436250_owned,
  &Impl::native_control_base_setconsume_mouse_over_signature_613ac0_owned,
  &Impl::native_control_base_setconsume_mouse_button_signature_5fd300_owned,
  &Impl::native_control_base_setconsume_mouse_wheel_signature_770eb0_owned,
  &Impl::native_control_base_getconsume_mouse_over_signature_520f90_owned,
  &Impl::native_control_base_getconsume_mouse_button_signature_5f3680_owned,
  &Impl::native_control_base_getconsume_mouse_wheel_signature_4c7cb0_owned,
  &Impl::native_control_base_destroyallchildren_signature_7a0d50_owned};
 for(std::size_t i=0;i<r.assigned;++i){const auto&row=r.rows[i];
  if(!row.primary||!row.secondary||!row.codebuddy||row.primary_ref<0||row.secondary_ref<0||row.key_ref<0)return stop(false);
  lua_rawgeti(L,LUA_REGISTRYINDEX,row.key_ref);
  if(lua_type(L,-1)!=LUA_TSTRING)return stop(false);
  std::size_t length=0;const char*const key=lua_tolstring(L,-1,&length);
  const auto&actual_key=r.definition->records[i];
  if(!actual_key.key||!key||length!=std::strlen(actual_key.key)||std::memcmp(key,actual_key.key,length))return stop(false);
  lua_rawget(L,methods);const int function=lua_gettop(L);
  lua_rawgeti(L,LUA_REGISTRYINDEX,row.primary_ref);
  if(!lua_iscfunction(L,function)||lua_tocfunction(L,function)!=primary[i]||
   lua_topointer(L,function)!=row.primary||!lua_rawequal(L,function,-1))return stop(false);lua_pop(L,1);
  lua_rawgeti(L,LUA_REGISTRYINDEX,row.codebuddy->table_ref);
  if(!native_ordinary_codebuddy_table_physical_without_callbacks(p,L,*row.codebuddy,-1))return stop(false);
  lua_pushvalue(L,function);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,row.secondary_ref);
  if(!lua_iscfunction(L,-2)||lua_tocfunction(L,-2)!=secondary[i]||
   lua_topointer(L,-2)!=row.secondary||!lua_rawequal(L,-2,-1))return stop(false);
  lua_settop(L,methods);
 }
 return stop(true);
}
static void native_control_base_mutable_library_47c500_owned(lua_State*L){
 auto&p=owner(L);native_control_base_mutable_library_source_owned(L);
 const int before=lua_gettop(L);
 if(before<1||lua_type(L,-1)!=LUA_TTABLE)
  return (void)luaL_error(L,"ControlBase actual7A4E4C metatype caller missing");
 //Private physical keeper/query highwater. Reserve before persistent actor
 //creation; this is not an Original47C500 function/table completion.
 if(!lua_checkstack(L,24))return (void)luaL_error(L,"ControlBase lifecycle49 keeper requires actual stack reserve24");
 native_control_base_mutable_library_source_owned(L);
 const void*const offered=lua_topointer(L,-1);native_control_base_mutable_library_source_owned(L);
 p.native_control_base_mutable_library_prefixes.push_back(std::make_shared<NativeControlBaseMutableLibraryPrefix>(
  &p,p.native_control_base_constructor,p.native_context_definitions[1],offered));
 auto*r=p.native_control_base_mutable_library_prefixes.back().get();
 lua_createtable(L,0,0);native_control_base_mutable_library_source_owned(L); //Actual47C50E.
 r->table=lua_topointer(L,-1);r->stage=NativeControlBaseMutableLibraryPrefix::Stage::table_created;
 lua_pushvalue(L,-2);native_control_base_mutable_library_source_owned(L);
 r->metatype_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_control_base_mutable_library_source_owned(L);
 lua_pushvalue(L,-1);native_control_base_mutable_library_source_owned(L);
 r->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_control_base_mutable_library_source_owned(L);
 if(r->metatype_ref<0||r->table_ref<0||lua_gettop(L)!=before+1)
  return (void)luaL_error(L,"ControlBase47C50E genuine partial table/caller custody failed");
 r->stage=NativeControlBaseMutableLibraryPrefix::Stage::table_retained;
 std::size_t ordinal=0;
 for(;;){
  const auto*d=native_control_base_mutable_library_source_owned(L); //Actual47C518/current count544.
  if(d->count544!=49||ordinal>d->count544)
   return (void)luaL_error(L,"ControlBase actual49 count544 changed during real47C500 table loop");
  if(ordinal==d->count544){
   r->stage=NativeControlBaseMutableLibraryPrefix::Stage::returned;
   if(r->assigned!=49||lua_gettop(L)!=before+1||
    !native_control_base_lifecycle49_physical_without_callbacks(p,L,*r,-1))
    return (void)luaL_error(L,"ControlBase actual49 mutable table return physical POST failed");
   return; //Actual47C54C exhaustion ->47C55A RET4. No method is invoked.
  }
  native_control_base_lifecycle49_record_owned(L,ordinal);r->stage=NativeControlBaseMutableLibraryPrefix::Stage::collecting;
  lua_pushcclosure(L,native_control_base_lifecycle49_primary_owned(L,ordinal),0);
  native_control_base_mutable_library_source_owned(L); //Actual47C52B current primary0.
  auto&row=r->rows[ordinal];row.primary=lua_topointer(L,-1);
  lua_pushvalue(L,-1);native_control_base_mutable_library_source_owned(L);
  row.primary_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_control_base_mutable_library_source_owned(L);
  native_control_base_lifecycle49_associate_8f7e20_owned(L,*r,ordinal); //Actual47C534.
  const auto*record=native_control_base_lifecycle49_record_owned(L,ordinal);
  lua_setfield(L,-2,record->key);native_control_base_mutable_library_source_owned(L); //Actual47C540.
  record=native_control_base_lifecycle49_record_owned(L,ordinal);
  lua_pushstring(L,record->key);native_control_base_mutable_library_source_owned(L);
  row.key_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_control_base_mutable_library_source_owned(L);
  ++r->assigned;
  if(lua_gettop(L)!=before+1||r->assigned!=ordinal+1||
   !native_control_base_lifecycle49_physical_without_callbacks(p,L,*r,-1))
   return (void)luaL_error(L,"ControlBase actual offset record closure/association/key physical POST failed");
  ++ordinal; //Actual47C545 INC; next47C54C observes current count, not prefix count49.
 }
}
void native_control_base_mutable_library_prefix_retire()noexcept{
 for(const auto&r:native_control_base_mutable_library_prefixes)if(r){
  if(r->host!=this||r->creator!=std::this_thread::get_id())std::terminate();
  r->alive=false;r->stage=NativeControlBaseMutableLibraryPrefix::Stage::retired;
 }
 //All actual retained partial roots are reclaimed by SAME VM lua_close.
}
