// Inside the SAME Host::Impl, after the genuine completed FLua supplier.
// Original6704C0 ->6DFB60/5605C0 ->4C2D10: actual readonly namespace
// publication,194 ordered zero-upvalue closures,193 keys. Primary methods
// are never invoked by registration. Their unported backend is an explicit
// invocation-time error; no option value or successful setter is fabricated.
// Original77B630 reads current SmallUIAssets byteCBEF0F, pushes its
// boolean truth value and returns1; it does not check argument count.
// The portable reader consumes the real normalized UserOptions field through
// SAME current Source/Bank/Boot custody. No cached/default false is returned.
template<std::uint32_t Original>static int native_options_current_bool_owned(lua_State*L){
    auto&p=owner(L);native_db_query_current(L);
    const auto*const scope=p.worldview_native_host;
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    std::optional<bool> result;p.source_error=nullptr;p.source_exception={};
    try{
        p.worldview_require_callback_origin();
        if(!scope||p.worldview_native_host!=scope||!scope->loan)
            throw ScriptError("Options current bool actual native Source scope absent");
        if constexpr(Original==0x77b630u)result=scope->loan->context_options_small_ui_assets_owned();
        else if constexpr(Original==0x6709b0u)result=scope->loan->context_options_minimap_game_setting_owned("ShowTradeOn");
        else if constexpr(Original==0x410480u)result=scope->loan->context_options_minimap_game_setting_owned("GridOn");
        else if constexpr(Original==0x6cb9e0u)result=scope->loan->context_options_minimap_game_setting_owned("YieldOn");
        else if constexpr(Original==0x600490u)result=scope->loan->context_options_minimap_game_setting_owned("ResourceOn");
        else if constexpr(Original==0x4e9f90u)result=scope->loan->context_options_minimap_game_setting_owned("NoTileRecommendations");
        else throw ScriptError("unqualified Options current bool identity");
        p.worldview_require_callback_origin();scope->pure();
        if(p.worldview_native_host!=scope||p.failure_epoch!=epoch||p.failed.writes_!=writes||
           p.execution_uncertainty!=uncertainty||!result)
            throw ScriptError("Options current bool Source/field producer changed before push");
    }catch(...){p.source_exception=std::current_exception();p.source_error="Options current bool actual UserOptions field FIRST refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    lua_pushboolean(L,*result);native_db_query_current(L);
    p.source_error=nullptr;p.source_exception={};
    try{
        p.worldview_require_callback_origin();
        if(p.worldview_native_host!=scope||p.failure_epoch!=epoch||p.failed.writes_!=writes||
           p.execution_uncertainty!=uncertainty)
            throw ScriptError("Options current bool Source retired during real Boolean push");
        scope->pure();
    }catch(...){p.source_exception=std::current_exception();p.source_error="Options current bool actual Source POST refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    return 1;
}
template<std::uint32_t Original>static int native_options_method(lua_State*L){
    if constexpr(Original==0x77b630u||Original==0x6709b0u||Original==0x410480u||
                 Original==0x6cb9e0u||Original==0x600490u||Original==0x4e9f90u)
        return native_options_current_bool_owned<Original>(L);
    auto&p=owner(L);native_db_query_current(L);
    const char*name=nullptr;
    for(const auto&r:native_options_entries)if(r.original_function==Original){name=r.name;break;}
    if(!name){p.record_failure();return luaL_error(L,"OptionsManager registered endpoint identity %08X is absent",static_cast<unsigned>(Original));}
    p.record_failure();return luaL_error(L,"OptionsManager.%s original %08X requires its actual Options backend",name,static_cast<unsigned>(Original));
}
struct NativeOptionsEntry {const char*name;std::uint32_t original_function;lua_CFunction function;};
static constexpr NativeOptionsEntry native_options_entries[]={
    {"GetHDStrategicView_Cached",0x521500u,&Impl::native_options_method<0x521500u>}, //0x4c2d34
    {"SetHDStrategicView_Cached",0x46c620u,&Impl::native_options_method<0x46c620u>}, //0x4c2d4e
    {"GetGPUTextureDecode_Cached",0x74c6a0u,&Impl::native_options_method<0x74c6a0u>}, //0x4c2d6b
    {"SetGPUTextureDecode_Cached",0x5f6440u,&Impl::native_options_method<0x5f6440u>}, //0x4c2d85
    {"IsGPUTextureDecodeSupported",0x6151a0u,&Impl::native_options_method<0x6151a0u>}, //0x4c2d9f
    {"GetOverlayLevel_Cached",0x51d790u,&Impl::native_options_method<0x51d790u>}, //0x4c2dbc
    {"SetOverlayLevel_Cached",0x59bc40u,&Impl::native_options_method<0x59bc40u>}, //0x4c2dd6
    {"GetShadowLevel_Cached",0x797a30u,&Impl::native_options_method<0x797a30u>}, //0x4c2df0
    {"SetShadowLevel_Cached",0x5d8800u,&Impl::native_options_method<0x5d8800u>}, //0x4c2e0d
    {"GetFOWLevel_Cached",0x78d300u,&Impl::native_options_method<0x78d300u>}, //0x4c2e27
    {"SetFOWLevel_Cached",0x496600u,&Impl::native_options_method<0x496600u>}, //0x4c2e41
    {"GetTerrainDetailLevel_Cached",0x66f5c0u,&Impl::native_options_method<0x66f5c0u>}, //0x4c2e5e
    {"SetTerrainDetailLevel_Cached",0x6293f0u,&Impl::native_options_method<0x6293f0u>}, //0x4c2e78
    {"GetTerrainTessLevel_Cached",0x600c30u,&Impl::native_options_method<0x600c30u>}, //0x4c2e92
    {"SetTerrainTessLevel_Cached",0x545b60u,&Impl::native_options_method<0x545b60u>}, //0x4c2eaf
    {"GetTerrainShadowQuality_Cached",0x59f410u,&Impl::native_options_method<0x59f410u>}, //0x4c2ec9
    {"SetTerrainShadowQuality_Cached",0x671ad0u,&Impl::native_options_method<0x671ad0u>}, //0x4c2ee3
    {"GetWaterQuality_Cached",0x437df0u,&Impl::native_options_method<0x437df0u>}, //0x4c2f00
    {"SetWaterQuality_Cached",0x523330u,&Impl::native_options_method<0x523330u>}, //0x4c2f1a
    {"GetTextureQuality_Cached",0x664bd0u,&Impl::native_options_method<0x664bd0u>}, //0x4c2f34
    {"SetTextureQuality_Cached",0x79d140u,&Impl::native_options_method<0x79d140u>}, //0x4c2f51
    {"GetNumContinents_Cached",0x710910u,&Impl::native_options_method<0x710910u>}, //0x4c2f6b
    {"SetNumContinents_Cached",0x795120u,&Impl::native_options_method<0x795120u>}, //0x4c2f85
    {"GetVSync_Cached",0x748ba0u,&Impl::native_options_method<0x748ba0u>}, //0x4c2fa2
    {"SetVSync_Cached",0x793410u,&Impl::native_options_method<0x793410u>}, //0x4c2fbc
    {"GetMinimizeGrayTiles_Cached",0x496a70u,&Impl::native_options_method<0x496a70u>}, //0x4c2fd6
    {"SetMinimizeGrayTiles_Cached",0x4780b0u,&Impl::native_options_method<0x4780b0u>}, //0x4c2ff3
    {"GetFadeShadows_Cached",0x4632a0u,&Impl::native_options_method<0x4632a0u>}, //0x4c300d
    {"SetFadeShadows_Cached",0x4cba90u,&Impl::native_options_method<0x4cba90u>}, //0x4c3027
    {"GetLeaderQuality_Cached",0x5e0490u,&Impl::native_options_method<0x5e0490u>}, //0x4c3044
    {"SetLeaderQuality_Cached",0x514f20u,&Impl::native_options_method<0x514f20u>}, //0x4c305e
    {"ResetDefaultGraphicsOptions",0x79f4c0u,&Impl::native_options_method<0x79f4c0u>}, //0x4c3078
    {"HasUserChangedGraphicsOptions",0x575e40u,&Impl::native_options_method<0x575e40u>}, //0x4c3095
    {"GetAASamples_Cached",0x4a74c0u,&Impl::native_options_method<0x4a74c0u>}, //0x4c30af
    {"SetAASamples_Cached",0x58e780u,&Impl::native_options_method<0x58e780u>}, //0x4c30c9
    {"IsAALevelSupported",0x5938a0u,&Impl::native_options_method<0x5938a0u>}, //0x4c30e6
    {"GetFullscreen",0x6fc060u,&Impl::native_options_method<0x6fc060u>}, //0x4c3100
    {"GetFullscreen_Cached",0x6c00e0u,&Impl::native_options_method<0x6c00e0u>}, //0x4c311a
    {"SetFullscreen_Cached",0x498710u,&Impl::native_options_method<0x498710u>}, //0x4c3137
    {"GetResolution_Cached",0x544080u,&Impl::native_options_method<0x544080u>}, //0x4c3151
    {"SetResolution_Cached",0x4fe000u,&Impl::native_options_method<0x4fe000u>}, //0x4c316b
    {"GetWindowResolution_Cached",0x79f380u,&Impl::native_options_method<0x79f380u>}, //0x4c3188
    {"SetWindowResolution_Cached",0x660900u,&Impl::native_options_method<0x660900u>}, //0x4c31a2
    {"HasUserChangedResolution",0x5a27e0u,&Impl::native_options_method<0x5a27e0u>}, //0x4c31bc
    {"GetMaxResolution",0x63f290u,&Impl::native_options_method<0x63f290u>}, //0x4c31d9
    {"GetPolicyInfo",0x6fc620u,&Impl::native_options_method<0x6fc620u>}, //0x4c31f3
    {"GetPolicyInfo_Cached",0x6c0800u,&Impl::native_options_method<0x6c0800u>}, //0x4c320d
    {"SetPolicyInfo_Cached",0x6a5380u,&Impl::native_options_method<0x6a5380u>}, //0x4c322a
    {"GetSmallUIAssets",0x77b630u,&Impl::native_options_method<0x77b630u>}, //0x4c3244
    {"GetSmallUIAssets_Cached",0x4fa730u,&Impl::native_options_method<0x4fa730u>}, //0x4c325e
    {"SetSmallUIAssets_Cached",0x51fbb0u,&Impl::native_options_method<0x51fbb0u>}, //0x4c327b
    {"GetAutoUIAssets",0x48ed70u,&Impl::native_options_method<0x48ed70u>}, //0x4c3295
    {"GetAutoUIAssets_Cached",0x7362a0u,&Impl::native_options_method<0x7362a0u>}, //0x4c32af
    {"SetAutoUIAssets_Cached",0x4e1f10u,&Impl::native_options_method<0x4e1f10u>}, //0x4c32cc
    {"GetAutoUnitCycle",0x637b60u,&Impl::native_options_method<0x637b60u>}, //0x4c32e6
    {"GetAutoUnitCycle_Cached",0x600c10u,&Impl::native_options_method<0x600c10u>}, //0x4c3300
    {"SetAutoUnitCycle_Cached",0x401e10u,&Impl::native_options_method<0x401e10u>}, //0x4c331d
    {"GetGridOn",0x410480u,&Impl::native_options_method<0x410480u>}, //0x4c3337
    {"GetGridOn_Cached",0x69ec60u,&Impl::native_options_method<0x69ec60u>}, //0x4c3351
    {"SetGridOn_Cached",0x654a80u,&Impl::native_options_method<0x654a80u>}, //0x4c336e
    {"GetShowTradeOn",0x6709b0u,&Impl::native_options_method<0x6709b0u>}, //0x4c3388
    {"GetShowTradeOn_Cached",0x7aec00u,&Impl::native_options_method<0x7aec00u>}, //0x4c33a2
    {"SetShowTradeOn_Cached",0x5d5ac0u,&Impl::native_options_method<0x5d5ac0u>}, //0x4c33bf
    {"GetScoreList",0x43bc30u,&Impl::native_options_method<0x43bc30u>}, //0x4c33d9
    {"GetScoreList_Cached",0x592410u,&Impl::native_options_method<0x592410u>}, //0x4c33f3
    {"SetScoreList_Cached",0x5d56d0u,&Impl::native_options_method<0x5d56d0u>}, //0x4c3410
    {"GetMPScoreList",0x660440u,&Impl::native_options_method<0x660440u>}, //0x4c342a
    {"GetMPScoreList_Cached",0x4abe40u,&Impl::native_options_method<0x4abe40u>}, //0x4c3444
    {"SetMPScoreList_Cached",0x7af4e0u,&Impl::native_options_method<0x7af4e0u>}, //0x4c3461
    {"GetResourceOn",0x600490u,&Impl::native_options_method<0x600490u>}, //0x4c347b
    {"GetResourceOn_Cached",0x73dee0u,&Impl::native_options_method<0x73dee0u>}, //0x4c3495
    {"SetResourceOn_Cached",0x410200u,&Impl::native_options_method<0x410200u>}, //0x4c34b2
    {"GetYieldOn",0x6cb9e0u,&Impl::native_options_method<0x6cb9e0u>}, //0x4c34cc
    {"GetYieldOn_Cached",0x695150u,&Impl::native_options_method<0x695150u>}, //0x4c34e6
    {"SetYieldOn_Cached",0x4dd870u,&Impl::native_options_method<0x4dd870u>}, //0x4c3503
    {"GetHideTouchHelp",0x6bd830u,&Impl::native_options_method<0x6bd830u>}, //0x4c351d
    {"GetHideTouchHelp_Cached",0x69afb0u,&Impl::native_options_method<0x69afb0u>}, //0x4c3537
    {"SetHideTouchHelp_Cached",0x73eb60u,&Impl::native_options_method<0x73eb60u>}, //0x4c3554
    {"IsAutoWorkersDontReplace",0x549cb0u,&Impl::native_options_method<0x549cb0u>}, //0x4c356e
    {"IsAutoWorkersDontReplace_Cached",0x5e6c10u,&Impl::native_options_method<0x5e6c10u>}, //0x4c3588
    {"SetAutoWorkersDontReplace_Cached",0x55b0b0u,&Impl::native_options_method<0x55b0b0u>}, //0x4c35a5
    {"IsAutoWorkersDontRemoveFeatures",0x6bc200u,&Impl::native_options_method<0x6bc200u>}, //0x4c35bf
    {"IsAutoWorkersDontRemoveFeatures_Cached",0x4149a0u,&Impl::native_options_method<0x4149a0u>}, //0x4c35d9
    {"SetAutoWorkersDontRemoveFeatures_Cached",0x78c530u,&Impl::native_options_method<0x78c530u>}, //0x4c35f6
    {"IsNoCitizenWarning",0x6f9b00u,&Impl::native_options_method<0x6f9b00u>}, //0x4c3610
    {"IsNoCitizenWarning_Cached",0x6cb9c0u,&Impl::native_options_method<0x6cb9c0u>}, //0x4c362a
    {"SetNoCitizenWarning_Cached",0x59cf50u,&Impl::native_options_method<0x59cf50u>}, //0x4c3647
    {"GetTutorialLevel",0x549250u,&Impl::native_options_method<0x549250u>}, //0x4c3661
    {"GetTutorialLevel_Cached",0x6da050u,&Impl::native_options_method<0x6da050u>}, //0x4c367b
    {"SetTutorialLevel_Cached",0x759fb0u,&Impl::native_options_method<0x759fb0u>}, //0x4c3698
    {"IsNoRewardPopups",0x689930u,&Impl::native_options_method<0x689930u>}, //0x4c36b2
    {"IsNoRewardPopups_Cached",0x57fb80u,&Impl::native_options_method<0x57fb80u>}, //0x4c36cc
    {"SetNoRewardPopups_Cached",0x434dd0u,&Impl::native_options_method<0x434dd0u>}, //0x4c36e9
    {"IsNoTileRecommendations",0x4e9f90u,&Impl::native_options_method<0x4e9f90u>}, //0x4c3703
    {"IsNoTileRecommendations_Cached",0x6b5090u,&Impl::native_options_method<0x6b5090u>}, //0x4c371d
    {"SetNoTileRecommendations_Cached",0x4a1ab0u,&Impl::native_options_method<0x4a1ab0u>}, //0x4c373a
    {"IsCivilianYields",0x5c43d0u,&Impl::native_options_method<0x5c43d0u>}, //0x4c3754
    {"IsCivilianYields_Cached",0x487f70u,&Impl::native_options_method<0x487f70u>}, //0x4c376e
    {"SetCivilianYields_Cached",0x604760u,&Impl::native_options_method<0x604760u>}, //0x4c378b
    {"IsNoBasicHelp",0x6dd960u,&Impl::native_options_method<0x6dd960u>}, //0x4c37a5
    {"IsNoBasicHelp_Cached",0x774490u,&Impl::native_options_method<0x774490u>}, //0x4c37bf
    {"SetNoBasicHelp_Cached",0x5a82b0u,&Impl::native_options_method<0x5a82b0u>}, //0x4c37dc
    {"GetQuickSelectionAdvanceEnabled",0x60f990u,&Impl::native_options_method<0x60f990u>}, //0x4c37f6
    {"GetQuickSelectionAdvanceEnabled_Cached",0x447ab0u,&Impl::native_options_method<0x447ab0u>}, //0x4c3810
    {"SetQuickSelectionAdvanceEnabled_Cached",0x786520u,&Impl::native_options_method<0x786520u>}, //0x4c382d
    {"GetLANNickName",0x60a360u,&Impl::native_options_method<0x60a360u>}, //0x4c3847
    {"GetLANNickName_Cached",0x509af0u,&Impl::native_options_method<0x509af0u>}, //0x4c3861
    {"SetLANNickName_Cached",0x662500u,&Impl::native_options_method<0x662500u>}, //0x4c387e
    {"GetTurnNotifySteamInvite",0x5781e0u,&Impl::native_options_method<0x5781e0u>}, //0x4c3898
    {"GetTurnNotifySteamInvite_Cached",0x476400u,&Impl::native_options_method<0x476400u>}, //0x4c38b2
    {"SetTurnNotifySteamInvite_Cached",0x5a6ca0u,&Impl::native_options_method<0x5a6ca0u>}, //0x4c38cf
    {"GetTurnNotifyEmail",0x50c230u,&Impl::native_options_method<0x50c230u>}, //0x4c38e9
    {"GetTurnNotifyEmail_Cached",0x5bf4a0u,&Impl::native_options_method<0x5bf4a0u>}, //0x4c3903
    {"SetTurnNotifyEmail_Cached",0x4a0ac0u,&Impl::native_options_method<0x4a0ac0u>}, //0x4c3920
    {"GetTurnNotifyEmailAddress",0x6ea420u,&Impl::native_options_method<0x6ea420u>}, //0x4c393a
    {"GetTurnNotifyEmailAddress_Cached",0x69dfd0u,&Impl::native_options_method<0x69dfd0u>}, //0x4c3954
    {"SetTurnNotifyEmailAddress_Cached",0x5e8e80u,&Impl::native_options_method<0x5e8e80u>}, //0x4c3971
    {"GetTurnNotifySmtpEmailAddress",0x4199c0u,&Impl::native_options_method<0x4199c0u>}, //0x4c398b
    {"GetTurnNotifySmtpEmailAddress_Cached",0x5db700u,&Impl::native_options_method<0x5db700u>}, //0x4c39a5
    {"SetTurnNotifySmtpEmailAddress_Cached",0x4b0e40u,&Impl::native_options_method<0x4b0e40u>}, //0x4c39c2
    {"GetTurnNotifySmtpHost",0x50a080u,&Impl::native_options_method<0x50a080u>}, //0x4c39dc
    {"GetTurnNotifySmtpHost_Cached",0x51fee0u,&Impl::native_options_method<0x51fee0u>}, //0x4c39f6
    {"SetTurnNotifySmtpHost_Cached",0x5941d0u,&Impl::native_options_method<0x5941d0u>}, //0x4c3a13
    {"GetTurnNotifySmtpPort",0x5ae090u,&Impl::native_options_method<0x5ae090u>}, //0x4c3a2d
    {"GetTurnNotifySmtpPort_Cached",0x6dd780u,&Impl::native_options_method<0x6dd780u>}, //0x4c3a47
    {"SetTurnNotifySmtpPort_Cached",0x489bd0u,&Impl::native_options_method<0x489bd0u>}, //0x4c3a64
    {"GetTurnNotifySmtpUsername",0x47dcb0u,&Impl::native_options_method<0x47dcb0u>}, //0x4c3a7e
    {"GetTurnNotifySmtpUsername_Cached",0x6ee680u,&Impl::native_options_method<0x6ee680u>}, //0x4c3a98
    {"SetTurnNotifySmtpUsername_Cached",0x4d1060u,&Impl::native_options_method<0x4d1060u>}, //0x4c3ab5
    {"GetTurnNotifySmtpPassword_Cached",0x50fe20u,&Impl::native_options_method<0x50fe20u>}, //0x4c3acf
    {"SetTurnNotifySmtpPassword_Cached",0x6c1a60u,&Impl::native_options_method<0x6c1a60u>}, //0x4c3ae9
    {"GetTurnNotifySmtpTLS",0x415d30u,&Impl::native_options_method<0x415d30u>}, //0x4c3b06
    {"GetTurnNotifySmtpTLS_Cached",0x5f9df0u,&Impl::native_options_method<0x5f9df0u>}, //0x4c3b20
    {"SetTurnNotifySmtpTLS_Cached",0x7a3050u,&Impl::native_options_method<0x7a3050u>}, //0x4c3b3a
    {"GetStraightZoom",0x765630u,&Impl::native_options_method<0x765630u>}, //0x4c3b57
    {"GetStraightZoom_Cached",0x450210u,&Impl::native_options_method<0x450210u>}, //0x4c3b71
    {"SetStraightZoom_Cached",0x6c4530u,&Impl::native_options_method<0x6c4530u>}, //0x4c3b8b
    {"GetBindMouseMode",0x66b720u,&Impl::native_options_method<0x66b720u>}, //0x4c3ba8
    {"GetBindMouseMode_Cached",0x6713f0u,&Impl::native_options_method<0x6713f0u>}, //0x4c3bc2
    {"SetBindMouseMode_Cached",0x712cb0u,&Impl::native_options_method<0x712cb0u>}, //0x4c3bdc
    {"GetTooltip1Seconds",0x50c700u,&Impl::native_options_method<0x50c700u>}, //0x4c3bf9
    {"GetTooltip1Seconds_Cached",0x563700u,&Impl::native_options_method<0x563700u>}, //0x4c3c13
    {"SetTooltip1Seconds_Cached",0x57c820u,&Impl::native_options_method<0x57c820u>}, //0x4c3c2d
    {"GetTooltip2Seconds",0x53ec80u,&Impl::native_options_method<0x53ec80u>}, //0x4c3c4a
    {"GetTooltip2Seconds_Cached",0x798fb0u,&Impl::native_options_method<0x798fb0u>}, //0x4c3c64
    {"SetTooltip2Seconds_Cached",0x781fb0u,&Impl::native_options_method<0x781fb0u>}, //0x4c3c7e
    {"IsDebugMode",0x631a10u,&Impl::native_options_method<0x631a10u>}, //0x4c3c9b
    {"IsDebugMode_Cached",0x6a09a0u,&Impl::native_options_method<0x6a09a0u>}, //0x4c3cb5
    {"SetDebugMode_Cached",0x43cbb0u,&Impl::native_options_method<0x43cbb0u>}, //0x4c3ccf
    {"GetTurnsBetweenAutosave",0x69fb20u,&Impl::native_options_method<0x69fb20u>}, //0x4c3cec
    {"GetTurnsBetweenAutosave_Cached",0x7800f0u,&Impl::native_options_method<0x7800f0u>}, //0x4c3d06
    {"SetTurnsBetweenAutosave_Cached",0x465aa0u,&Impl::native_options_method<0x465aa0u>}, //0x4c3d20
    {"GetNumAutosavesKept",0x784b30u,&Impl::native_options_method<0x784b30u>}, //0x4c3d3d
    {"GetNumAutosavesKept_Cached",0x474f10u,&Impl::native_options_method<0x474f10u>}, //0x4c3d57
    {"SetNumAutosavesKept_Cached",0x5c12d0u,&Impl::native_options_method<0x5c12d0u>}, //0x4c3d71
    {"GetSinglePlayerAutoEndTurnEnabled",0x53b880u,&Impl::native_options_method<0x53b880u>}, //0x4c3d8e
    {"GetSinglePlayerAutoEndTurnEnabled_Cached",0x6cc320u,&Impl::native_options_method<0x6cc320u>}, //0x4c3da8
    {"SetSinglePlayerAutoEndTurnEnabled_Cached",0x494130u,&Impl::native_options_method<0x494130u>}, //0x4c3dc2
    {"GetMultiplayerAutoEndTurnEnabled",0x4d2aa0u,&Impl::native_options_method<0x4d2aa0u>}, //0x4c3ddf
    {"GetMultiplayerAutoEndTurnEnabled_Cached",0x659600u,&Impl::native_options_method<0x659600u>}, //0x4c3df9
    {"SetMultiplayerAutoEndTurnEnabled_Cached",0x5af150u,&Impl::native_options_method<0x5af150u>}, //0x4c3e13
    {"GetSinglePlayerQuickCombatEnabled",0x469580u,&Impl::native_options_method<0x469580u>}, //0x4c3e30
    {"GetSinglePlayerQuickCombatEnabled_Cached",0x72d7a0u,&Impl::native_options_method<0x72d7a0u>}, //0x4c3e4a
    {"SetSinglePlayerQuickCombatEnabled_Cached",0x5878a0u,&Impl::native_options_method<0x5878a0u>}, //0x4c3e64
    {"GetSinglePlayerQuickMovementEnabled",0x7215a0u,&Impl::native_options_method<0x7215a0u>}, //0x4c3e81
    {"GetSinglePlayerQuickMovementEnabled_Cached",0x5fab90u,&Impl::native_options_method<0x5fab90u>}, //0x4c3e9b
    {"SetSinglePlayerQuickMovementEnabled_Cached",0x74ea50u,&Impl::native_options_method<0x74ea50u>}, //0x4c3eb5
    {"GetMultiplayerQuickCombatEnabled",0x6a93b0u,&Impl::native_options_method<0x6a93b0u>}, //0x4c3ed2
    {"GetMultiplayerQuickCombatEnabled_Cached",0x546c20u,&Impl::native_options_method<0x546c20u>}, //0x4c3eec
    {"SetMultiplayerQuickCombatEnabled_Cached",0x4030c0u,&Impl::native_options_method<0x4030c0u>}, //0x4c3f06
    {"GetMultiplayerQuickMovementEnabled",0x6ded60u,&Impl::native_options_method<0x6ded60u>}, //0x4c3f23
    {"GetMultiplayerQuickMovementEnabled_Cached",0x5e9740u,&Impl::native_options_method<0x5e9740u>}, //0x4c3f3d
    {"SetMultiplayerQuickMovementEnabled_Cached",0x6049f0u,&Impl::native_options_method<0x6049f0u>}, //0x4c3f57
    {"GetEnableMapInertia",0x5999a0u,&Impl::native_options_method<0x5999a0u>}, //0x4c3f74
    {"GetEnableMapInertia_Cached",0x6518a0u,&Impl::native_options_method<0x6518a0u>}, //0x4c3f8e
    {"SetEnableMapInertia_Cached",0x6f8210u,&Impl::native_options_method<0x6f8210u>}, //0x4c3fa8
    {"GetSkipIntroVideo",0x74bc60u,&Impl::native_options_method<0x74bc60u>}, //0x4c3fc5
    {"GetSkipIntroVideo_Cached",0x611a50u,&Impl::native_options_method<0x611a50u>}, //0x4c3fdf
    {"SetSkipIntroVideo_Cached",0x4d3120u,&Impl::native_options_method<0x4d3120u>}, //0x4c3ff9
    {"GetDragSpeed",0x53a800u,&Impl::native_options_method<0x53a800u>}, //0x4c4016
    {"GetDragSpeed_Cached",0x4f6970u,&Impl::native_options_method<0x4f6970u>}, //0x4c4030
    {"SetDragSpeed_Cached",0x6d68b0u,&Impl::native_options_method<0x6d68b0u>}, //0x4c404a
    {"GetPinchSpeed",0x6792d0u,&Impl::native_options_method<0x6792d0u>}, //0x4c4067
    {"GetPinchSpeed_Cached",0x7233c0u,&Impl::native_options_method<0x7233c0u>}, //0x4c4081
    {"SetPinchSpeed_Cached",0x70dfb0u,&Impl::native_options_method<0x70dfb0u>}, //0x4c409b
    {"ResetTutorial",0x507330u,&Impl::native_options_method<0x507330u>}, //0x4c40b8
    {"CommitGameOptions",0x63f920u,&Impl::native_options_method<0x63f920u>}, //0x4c40d2
    {"SyncGameOptionsCache",0x633ba0u,&Impl::native_options_method<0x633ba0u>}, //0x4c40ec
    {"CommitGraphicsOptions",0x6c7170u,&Impl::native_options_method<0x6c7170u>}, //0x4c4109
    {"SyncGraphicsOptionsCache",0x733460u,&Impl::native_options_method<0x733460u>}, //0x4c4123
    {"CommitResolutionOptions",0x4a8620u,&Impl::native_options_method<0x4a8620u>}, //0x4c413d
    {"SyncResolutionOptionsCache",0x46def0u,&Impl::native_options_method<0x46def0u>}, //0x4c415a
    {"HasUserChangedResolution",0x5a27e0u,&Impl::native_options_method<0x5a27e0u>}, //0x4c4174
    {"ResetDefaultGameOptions",0x5add80u,&Impl::native_options_method<0x5add80u>}, //0x4c418e
};
struct NativeContextOptionsFrame final {
    Impl& host;const NativeContextFLuaFrame*const predecessor;
    const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;
    int keys_ref=LUA_NOREF;const void* keys=nullptr;
    int master_ref=LUA_NOREF,proxy_ref=LUA_NOREF;const void* master=nullptr;const void* proxy=nullptr;const void* meta=nullptr;
    std::size_t fields_created=0,stores=0;Job* actual_job=nullptr;
    explicit NativeContextOptionsFrame(Impl& p):host(p),predecessor(p.flua_frame),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(host.options_frame||!predecessor)throw ScriptError("Options registrar requires actual FLua predecessor");
        predecessor->first();if(!predecessor->returned||predecessor->table_ref<0||!predecessor->table)
            throw ScriptError("Options original FLua predecessor did not complete its actual exposure/lifetime/selected copy");
        host.options_frame=this;
    }
    ~NativeContextOptionsFrame(){if(host.options_frame==this)host.options_frame=nullptr;}
    NativeContextOptionsFrame(const NativeContextOptionsFrame&)=delete;
    NativeContextOptionsFrame&operator=(const NativeContextOptionsFrame&)=delete;
    void first()const {
        if(host.options_frame!=this||creator!=std::this_thread::get_id()||!predecessor||host.flua_frame!=predecessor||
           epoch!=host.failure_epoch||writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty)
            throw ScriptError("Options registrar actual source/frame changed");
        predecessor->first();
    }
    void full()const {first();predecessor->full();first();}
};
static void native_options_key(lua_State* L,int i){
    const auto& p=owner(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_options_registration_keys_ref);lua_rawgeti(L,-1,i);lua_remove(L,-2);
}
static void native_options_registration_full(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.options_frame;p.source_exception={};p.source_error=nullptr;
    try{if(!f)throw ScriptError("Options registrar has no genuine entered frame");f->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Options registrar SAME source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    if(f->actual_job&&f->actual_job!=&job)return (void)luaL_error(L,"Options registrar actual entered Job replaced");
    if(!f->predecessor->actual_job)return (void)luaL_error(L,"Options genuine FLua predecessor Job retired");
    auto*const flua=native_flua_registration_full(L);
    auto*const bootstrap=native_flua_owner_current(L);
    if(flua!=f->predecessor||!flua->returned||!bootstrap||
       bootstrap->phase!=NativeFLuaBootstrapOwner::Phase::returned||
       flua->table!=bootstrap->table||flua->table_ref!=bootstrap->table_ref)
        return (void)luaL_error(L,"Options actual completed FLua predecessor storage changed");
    p.source_exception={};p.source_error=nullptr;
    try{if(p.options_frame!=f)throw ScriptError("Options frame replaced after source callbacks");f->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Options registrar FIRST after callbacks refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    const int top=lua_gettop(L);
    if(f->keys_ref!=LUA_NOREF){
        if(f->keys_ref!=p.native_options_registration_keys_ref)return (void)luaL_error(L,"Options registrar keys custody changed");
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->keys||lua_objlen(L,-1)!=195||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Options registrar retained key table changed");
        for(int i=0;i<195;++i){
            lua_rawgeti(L,-1,i+1);std::size_t n=0;const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
            if(!key||n!=std::strlen(i==0?"OptionsManager":native_options_entries[i-1].name)||std::memcmp(key,i==0?"OptionsManager":native_options_entries[i-1].name,n))
                return (void)luaL_error(L,"Options registrar retained key TString changed");lua_pop(L,1);
        }lua_pop(L,1);
    }
    if(f->master_ref>=0){
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->master)return (void)luaL_error(L,"Options actual master table changed");
        native_options_key(L,1);lua_rawget(L,LUA_GLOBALSINDEX);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Options original master cache changed");lua_pop(L,2);
    }
    if(f->stores){
        if(f->stores!=1||f->proxy_ref<0||!f->proxy||!f->meta||f->master_ref<0)
            return (void)luaL_error(L,"Options selected store lacks actual return");
        lua_rawgeti(L,LUA_REGISTRYINDEX,p.lua_events_frame->environment);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.lua_events_frame->actual_environment||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Options actual selected fenv changed");
        native_options_key(L,1);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->proxy_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->proxy||!lua_rawequal(L,-1,-2)||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=f->meta)
            return (void)luaL_error(L,"Options selected proxy/ref/metatable changed");
        native_api_prefix_key(L,2);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Options proxy actual master __index changed");lua_pop(L,2);
        native_api_prefix_key(L,3);lua_rawget(L,-2);
        if(lua_type(L,-1)!=LUA_TBOOLEAN||lua_toboolean(L,-1))return (void)luaL_error(L,"Options proxy original false __metatable changed");lua_pop(L,1);
        native_api_prefix_key(L,4);lua_rawget(L,-2);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_api_readonly_write||lua_getupvalue(L,-1,1))
            return (void)luaL_error(L,"Options proxy original write-error consumer changed");
    }
    lua_settop(L,top);
}
static int native_options_factory(lua_State* L){
    auto& p=owner(L);auto*const f=p.options_frame;
    if(!f||!f->actual_job)return luaL_error(L,"Native Options constructor lacks its actual entered Job");
    auto& job=*f->actual_job;
    native_options_registration_full(L,job);
    if(!f||f->fields_created||f->master_ref!=LUA_NOREF)return luaL_error(L,"Native Options4C2D10 constructor replay refused");
    lua_createtable(L,0,0);const int table=lua_gettop(L);const void*const value=lua_topointer(L,table);native_options_registration_full(L,job);
    for(int i=0;i<194;++i){
        native_options_key(L,i+2);lua_pushcfunction(L,native_options_entries[i].function);lua_rawset(L,table);++f->fields_created;native_options_registration_full(L,job);
    }
    if(f->fields_created!=194||!lua_istable(L,table)||lua_topointer(L,table)!=value||lua_getmetatable(L,table))
        return luaL_error(L,"Native Options4C2D10 actual namespace constructor changed");
    for(int i=0;i<194;++i){native_options_key(L,i+2);lua_rawget(L,table);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_options_entries[i].function||lua_getupvalue(L,-1,1))return luaL_error(L,"Options original zero-upvalue consumer changed");lua_pop(L,1);}
    return 1;
}
static void perform_native_context_options_registration(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.options_frame;native_options_registration_full(L,job);
    if(!f||job.owner!=&p||job.operation!=Operation::native_context_options_registration||
       L!=p.state||!p.calling||f->stores||f->keys_ref!=LUA_NOREF||f->actual_job||!lua_checkstack(L,32))
        return (void)luaL_error(L,"Options original registrar replay/key custody/stack refused");
    f->actual_job=&job;
    if(p.native_options_registration_keys_ref==LUA_NOREF){
        lua_createtable(L,195,0);const int keys=lua_gettop(L);const void*const key_value=lua_topointer(L,keys);native_options_registration_full(L,job);
        for(int i=0;i<195;++i){lua_pushstring(L,i==0?"OptionsManager":native_options_entries[i-1].name);lua_rawseti(L,keys,i+1);native_options_registration_full(L,job);}
        lua_pushvalue(L,keys);const int key_ref=luaL_ref(L,LUA_REGISTRYINDEX);
        if(key_ref<0)return (void)luaL_error(L,"Options actual retained key allocation failed");
        f->keys_ref=key_ref;f->keys=key_value;p.native_options_registration_keys_ref=key_ref;p.native_options_registration_keys_pointer=key_value;
        native_options_registration_full(L,job);lua_pop(L,1);
    }else{
        f->keys_ref=p.native_options_registration_keys_ref;f->keys=p.native_options_registration_keys_pointer;native_options_registration_full(L,job);
    }
    // Original6704C0/6DFB60/5605C0 selects actual518 before inspecting
    // GLOBALS.OptionsManager; a cache miss runs4C2D10 zero args/one result.
    perform_native_context_state_query(L,job);native_options_registration_full(L,job);const int environment=lua_gettop(L);
    if(lua_topointer(L,environment)!=p.lua_events_frame->actual_environment)
        return (void)luaL_error(L,"Options registrar actual prepared518 environment changed");
    native_options_key(L,1);lua_gettable(L,LUA_GLOBALSINDEX);native_options_registration_full(L,job);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);lua_pushcfunction(L,native_options_factory);lua_call(L,0,1);native_options_registration_full(L,job);
        native_options_key(L,1);lua_pushvalue(L,-2);lua_settable(L,LUA_GLOBALSINDEX);native_options_registration_full(L,job);
    }
    if(!lua_istable(L,-1))return (void)luaL_error(L,"Options original namespace master is not a table");
    f->master=lua_topointer(L,-1);lua_pushvalue(L,-1);f->master_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_options_registration_full(L,job);
    native_api_readonly_proxy(L,job);native_options_registration_full(L,job);const int proxy=lua_gettop(L);f->proxy=lua_topointer(L,proxy);
    if(!lua_getmetatable(L,proxy))return (void)luaL_error(L,"Options original49B190 proxy has no metatable");f->meta=lua_topointer(L,-1);lua_pop(L,1);
    native_options_key(L,1);lua_pushvalue(L,proxy);lua_settable(L,environment);native_options_registration_full(L,job);
    lua_pushvalue(L,proxy);f->proxy_ref=luaL_ref(L,LUA_REGISTRYINDEX);++f->stores;native_options_registration_full(L,job);
    job.context_options_master_reference=f->master_ref;job.context_options_proxy_reference=f->proxy_ref;
    job.context_options_table=f->master;job.context_options_proxy=f->proxy;lua_pop(L,2);
}
