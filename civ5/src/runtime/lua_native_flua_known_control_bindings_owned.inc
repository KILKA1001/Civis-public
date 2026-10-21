//Inside the actual Host::Impl. Static known originalVA -> REAL x64 CFunction.
//Lookup is metadata-only; it never invokes or relaxes a callable borrower.
//Unknown is nullptr: eager registration still creates a real typed endpoint.
//At ACTUAL invocation its descriptor/method dispatcher must refuse an unknown
//backend explicitly, preserving original identity and producing no fake values.
//This finite map is49 known ControlBase bodies, not all250 builtin readiness.
static lua_CFunction native_flua_exposure_known_control_primary_body(std::uint32_t original){
 switch(original){
  case 0x4e4eb0u:return &Impl::native_control_base_setoffset_primary_4e4eb0_owned;
  case 0x6c44b0u:return &Impl::native_control_base_setoffset_x_primary_6c44b0_owned;
  case 0x410610u:return &Impl::native_control_base_setoffset_y_primary_410610_owned;
  case 0x688850u:return &Impl::native_control_base_setoffset_val_primary_688850_owned;
  case 0x51a6a0u:return &Impl::native_control_base_getoffset_x_primary_51a6a0_owned;
  case 0x6bb2e0u:return &Impl::native_control_base_getoffset_y_primary_6bb2e0_owned;
  case 0x6cba70u:return &Impl::native_control_base_getoffset_val_primary_6cba70_owned;
  case 0x5ab320u:return &Impl::native_control_base_setanchor_primary_5ab320_owned;
  case 0x7a54b0u:return &Impl::native_control_base_sethide_primary_7a54b0_owned;
  case 0x723910u:return &Impl::native_control_base_setnoclip_primary_723910_owned;
  case 0x4c4bd0u:return &Impl::native_control_base_ishidden_primary_4c4bd0_owned;
  case 0x47e630u:return &Impl::native_control_base_getnumchildren_primary_47e630_owned;
  case 0x4f7570u:return &Impl::native_control_base_reprocessanchoring_primary_4f7570_owned;
  case 0x537c60u:return &Impl::native_control_base_getsize_primary_537c60_owned;
  case 0x600630u:return &Impl::native_control_base_getsize_x_primary_600630_owned;
  case 0x421820u:return &Impl::native_control_base_getsize_y_primary_421820_owned;
  case 0x49f370u:return &Impl::native_control_base_getsize_val_primary_49f370_owned;
  case 0x676030u:return &Impl::native_control_base_setsize_primary_676030_owned;
  case 0x4bad50u:return &Impl::native_control_base_setsize_x_primary_4bad50_owned;
  case 0x57a330u:return &Impl::native_control_base_setsize_y_primary_57a330_owned;
  case 0x6eee90u:return &Impl::native_control_base_setsize_val_primary_6eee90_owned;
  case 0x6b4b50u:return &Impl::native_control_base_setcolor_primary_6b4b50_owned;
  case 0x766e70u:return &Impl::native_control_base_setalpha_primary_766e70_owned;
  case 0x78f870u:return &Impl::native_control_base_getalpha_primary_78f870_owned;
  case 0x5bf0b0u:return &Impl::native_control_base_sortchildren_primary_5bf0b0_owned;
  case 0x75eb20u:return &Impl::native_control_base_branchresetanimation_primary_75eb20_owned;
  case 0x4dd890u:return &Impl::native_control_base_getid_primary_4dd890_owned;
  case 0x5acf00u:return &Impl::native_control_base_setdisabled_primary_5acf00_owned;
  case 0x6d2930u:return &Impl::native_control_base_isdisabled_primary_6d2930_owned;
  case 0x58e600u:return &Impl::native_control_base_setmodal_primary_58e600_owned;
  case 0x70c030u:return &Impl::native_control_base_ismodal_primary_70c030_owned;
  case 0x6ad460u:return &Impl::native_control_base_calculatevisibilitybox_primary_6ad460_owned;
  case 0x5180c0u:return &Impl::native_control_base_settooltipcallback_primary_5180c0_owned;
  case 0x453350u:return &Impl::native_control_base_istooltipenabled_primary_453350_owned;
  case 0x5f8490u:return &Impl::native_control_base_enabletooltip_primary_5f8490_owned;
  case 0x5b2e60u:return &Impl::native_control_base_settooltiptype_primary_5b2e60_owned;
  case 0x564ef0u:return &Impl::native_control_base_settooltipstring_primary_564ef0_owned;
  case 0x67e230u:return &Impl::native_control_base_localizeandsettooltip_primary_67e230_owned;
  case 0x74ffa0u:return &Impl::native_control_base_gettooltipstring_primary_74ffa0_owned;
  case 0x661630u:return &Impl::native_control_base_changeparent_primary_661630_owned;
  case 0x669850u:return &Impl::native_control_base_hasmouseover_primary_669850_owned;
  case 0x43cbf0u:return &Impl::native_control_base_releasechild_primary_43cbf0_owned;
  case 0x48edf0u:return &Impl::native_control_base_setconsume_mouse_over_primary_48edf0_owned;
  case 0x624b60u:return &Impl::native_control_base_setconsume_mouse_button_primary_624b60_owned;
  case 0x62af80u:return &Impl::native_control_base_setconsume_mouse_wheel_primary_62af80_owned;
  case 0x75d480u:return &Impl::native_control_base_getconsume_mouse_over_primary_75d480_owned;
  case 0x6d8390u:return &Impl::native_control_base_getconsume_mouse_button_primary_6d8390_owned;
  case 0x488050u:return &Impl::native_control_base_getconsume_mouse_wheel_primary_488050_owned;
  case 0x6fbb50u:return &Impl::native_control_base_destroyallchildren_primary_6fbb50_owned;
 }
 return nullptr;
}
