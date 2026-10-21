// Actual457414 ->4E4ED0 ->6DFB60/protected5605C0, UI ctor668A80.
// The ctor creates one real table and6FD370 registers207 original ordered
// zero-upvalue callbacks. Registration never invokes any consumer backend.
template<std::uint32_t Original> static int native_context_ui_method(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    lua_State*const vm=p.state;global_State*const global=G(L);
    p.source_exception={};p.source_error=nullptr;
    try{p.worldview_require_callback_origin();
        if(!vm||p.state!=vm||G(vm)!=global||std::this_thread::get_id()!=p.ui_thread||p.failed.exhausted_)
            throw ScriptError("UI native consumer actual Host/VM/source retired");
    }catch(...){p.source_exception=std::current_exception();p.source_error="UI native consumer FIRST refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    if constexpr(Original==0x006a44b0u||Original==0x00440a90u||Original==0x00465760u){
        // SAME canonical current input-policy body, with its actual required
        // upvalue. The public original UI callback itself still has0upvalues.
        // No raw call to a one-upvalue CFunction, fabricated return or owner.
        constexpr int policy=Original==0x006a44b0u?0:(Original==0x00440a90u?1:2);
        const int arguments=lua_gettop(L);
        lua_pushinteger(L,policy);lua_pushcclosure(L,worldview_input_policy_call,1);lua_insert(L,1);
        native_db_query_current(L);
        p.source_exception={};p.source_error=nullptr;
        try{p.worldview_require_callback_origin();
            if(p.state!=vm||G(L)!=global||G(vm)!=global||epoch!=p.failure_epoch||writes!=p.failed.writes_||
               uncertainty!=p.execution_uncertainty||p.failed.exhausted_)
                throw ScriptError("UI policy closure allocation retired actual source/VM");
        }catch(...){p.source_exception=std::current_exception();p.source_error="UI canonical policy before actual invocation refused";}
        if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
        lua_call(L,arguments,LUA_MULTRET);native_db_query_current(L);
        p.source_exception={};p.source_error=nullptr;
        try{p.worldview_require_callback_origin();
            if(p.state!=vm||G(L)!=global||G(vm)!=global||epoch!=p.failure_epoch||writes!=p.failed.writes_||
               uncertainty!=p.execution_uncertainty||p.failed.exhausted_)
                throw ScriptError("UI actual policy callback source/VM changed at POST");
        }catch(...){p.source_exception=std::current_exception();p.source_error="UI canonical policy POST refused";}
        if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
        const int count=lua_gettop(L);
        if(count!=(policy==2?0:1)){p.record_failure();return luaL_error(L,"UI actual policy result count changed");}
        return count;
    }else{
        p.record_failure();return luaL_error(L,"UI native consumer %08X requires its genuine backend; provider unissued",static_cast<unsigned>(Original));
    }
}
struct NativeContextUiEntry{const char* name;std::uint32_t original_function;lua_CFunction function;};
static constexpr NativeContextUiEntry native_context_ui_entries[]={
    {"LookAtSelectionPlot",0x553460u,native_context_ui_method<0x553460u>}, // name store 0x6fd382
    {"CanDoInterfaceMode",0x70c510u,native_context_ui_method<0x70c510u>}, // name store 0x6fd39c
    {"GetSelectionPlot",0x689960u,native_context_ui_method<0x689960u>}, // name store 0x6fd3b6
    {"GetHeadSelectedUnit",0x670bb0u,native_context_ui_method<0x670bb0u>}, // name store 0x6fd3d3
    {"GetSelectedUnitID",0x6b08a0u,native_context_ui_method<0x6b08a0u>}, // name store 0x6fd3ed
    {"SelectUnit",0x5cef00u,native_context_ui_method<0x5cef00u>}, // name store 0x6fd407
    {"SelectGroup",0x71f7b0u,native_context_ui_method<0x71f7b0u>}, // name store 0x6fd424
    {"SelectAll",0x53f9f0u,native_context_ui_method<0x53f9f0u>}, // name store 0x6fd43e
    {"RemoveFromSelectionList",0x450990u,native_context_ui_method<0x450990u>}, // name store 0x6fd458
    {"MakeSelectionListDirty",0x489840u,native_context_ui_method<0x489840u>}, // name store 0x6fd475
    {"CanSelectionListFound",0x6ae550u,native_context_ui_method<0x6ae550u>}, // name store 0x6fd48f
    {"CanSelectionListWork",0x578110u,native_context_ui_method<0x578110u>}, // name store 0x6fd4a9
    {"IsPopupUp",0x5fc6d0u,native_context_ui_method<0x5fc6d0u>}, // name store 0x6fd4c6
    {"IsPopupQueued",0x4f7f00u,native_context_ui_method<0x4f7f00u>}, // name store 0x6fd4e0
    {"IsDiploOrPopupWaiting",0x5783d0u,native_context_ui_method<0x5783d0u>}, // name store 0x6fd4fa
    {"IsPopupTypeOpen",0x4424e0u,native_context_ui_method<0x4424e0u>}, // name store 0x6fd517
    {"GetPopupType",0x571370u,native_context_ui_method<0x571370u>}, // name store 0x6fd531
    {"GetPopupTypeName",0x4767f0u,native_context_ui_method<0x4767f0u>}, // name store 0x6fd54b
    {"GetPopupTypeByIndex",0x660400u,native_context_ui_method<0x660400u>}, // name store 0x6fd568
    {"GetPopupTypeCount",0x672f30u,native_context_ui_method<0x672f30u>}, // name store 0x6fd582
    {"RegisterPopupType",0x427fa0u,native_context_ui_method<0x427fa0u>}, // name store 0x6fd59c
    {"IsCityScreenViewingMode",0x77a580u,native_context_ui_method<0x77a580u>}, // name store 0x6fd5b9
    {"SetCityScreenViewingMode",0x786640u,native_context_ui_method<0x786640u>}, // name store 0x6fd5d3
    {"GetMultiplayerLobbyMode",0x43dd20u,native_context_ui_method<0x43dd20u>}, // name store 0x6fd5ed
    {"SetMultiplayerLobbyMode",0x763b60u,native_context_ui_method<0x763b60u>}, // name store 0x6fd60a
    {"GetLastSelectedUnit",0x5e6760u,native_context_ui_method<0x5e6760u>}, // name store 0x6fd624
    {"SetLastSelectedUnit",0x6ad6c0u,native_context_ui_method<0x6ad6c0u>}, // name store 0x6fd63e
    {"ChangePlotListColumn",0x40e640u,native_context_ui_method<0x40e640u>}, // name store 0x6fd65b
    {"GetOriginalPlot",0x588690u,native_context_ui_method<0x588690u>}, // name store 0x6fd675
    {"ClearSelectionList",0x75c8f0u,native_context_ui_method<0x75c8f0u>}, // name store 0x6fd68f
    {"InsertIntoSelectionList",0x4312a0u,native_context_ui_method<0x4312a0u>}, // name store 0x6fd6ac
    {"SelectCity",0x58e210u,native_context_ui_method<0x58e210u>}, // name store 0x6fd6c6
    {"SelectLookAtCity",0x416ff0u,native_context_ui_method<0x416ff0u>}, // name store 0x6fd6e0
    {"AddSelectedCity",0x54e350u,native_context_ui_method<0x54e350u>}, // name store 0x6fd6fd
    {"ClearSelectedCities",0x4102c0u,native_context_ui_method<0x4102c0u>}, // name store 0x6fd717
    {"IsCitySelected",0x6098f0u,native_context_ui_method<0x6098f0u>}, // name store 0x6fd731
    {"GetHeadSelectedCity",0x643b90u,native_context_ui_method<0x643b90u>}, // name store 0x6fd74e
    {"IsCitySelection",0x6096c0u,native_context_ui_method<0x6096c0u>}, // name store 0x6fd768
    {"UpdateCityScreen",0x634570u,native_context_ui_method<0x634570u>}, // name store 0x6fd782
    {"SetPlaceUnit",0x691c30u,native_context_ui_method<0x691c30u>}, // name store 0x6fd79f
    {"GetPlaceUnit",0x64a690u,native_context_ui_method<0x64a690u>}, // name store 0x6fd7b9
    {"ClearPlaceUnit",0x5debf0u,native_context_ui_method<0x5debf0u>}, // name store 0x6fd7d3
    {"CanPlaceUnitAt",0x54f120u,native_context_ui_method<0x54f120u>}, // name store 0x6fd7f0
    {"HighlightCanPlacePlots",0x7a9600u,native_context_ui_method<0x7a9600u>}, // name store 0x6fd80a
    {"ClearEventMessages",0x636410u,native_context_ui_method<0x636410u>}, // name store 0x6fd824
    {"GetCycleSelectionCounter",0x6a6520u,native_context_ui_method<0x6a6520u>}, // name store 0x6fd841
    {"SetCycleSelectionCounter",0x740600u,native_context_ui_method<0x740600u>}, // name store 0x6fd85b
    {"ChangeCycleSelectionCounter",0x62f480u,native_context_ui_method<0x62f480u>}, // name store 0x6fd875
    {"GetEndTurnCounter",0x4603d0u,native_context_ui_method<0x4603d0u>}, // name store 0x6fd892
    {"SetEndTurnCounter",0x46f740u,native_context_ui_method<0x46f740u>}, // name store 0x6fd8ac
    {"ChangeEndTurnCounter",0x6db1d0u,native_context_ui_method<0x6db1d0u>}, // name store 0x6fd8c6
    {"CheckForCommandLineInvitation",0x456120u,native_context_ui_method<0x456120u>}, // name store 0x6fd8e3
    {"IsCombatFocus",0x4c5430u,native_context_ui_method<0x4c5430u>}, // name store 0x6fd8fd
    {"SetCombatFocus",0x4d9690u,native_context_ui_method<0x4d9690u>}, // name store 0x6fd917
    {"IsDirty",0x43ef50u,native_context_ui_method<0x43ef50u>}, // name store 0x6fd934
    {"SetDirty",0x412fe0u,native_context_ui_method<0x412fe0u>}, // name store 0x6fd94e
    {"MakeInterfaceDirty",0x566990u,native_context_ui_method<0x566990u>}, // name store 0x6fd968
    {"UpdateCursorType",0x670940u,native_context_ui_method<0x670940u>}, // name store 0x6fd985
    {"LookAt",0x5cee10u,native_context_ui_method<0x5cee10u>}, // name store 0x6fd99f
    {"CenterCamera",0x579ef0u,native_context_ui_method<0x579ef0u>}, // name store 0x6fd9b9
    {"ReleaseLockedCamera",0x5d9ea0u,native_context_ui_method<0x5d9ea0u>}, // name store 0x6fd9d6
    {"IsCameraMoving",0x5a0c20u,native_context_ui_method<0x5a0c20u>}, // name store 0x6fd9f0
    {"IsFocused",0x61e5a0u,native_context_ui_method<0x61e5a0u>}, // name store 0x6fda0a
    {"RefreshYieldVisibleMode",0x66f5e0u,native_context_ui_method<0x66f5e0u>}, // name store 0x6fda27
    {"ToggleYieldVisibleMode",0x42a130u,native_context_ui_method<0x42a130u>}, // name store 0x6fda41
    {"SetYieldVisibleMode",0x6eb890u,native_context_ui_method<0x6eb890u>}, // name store 0x6fda5b
    {"ToggleResourceVisibleMode",0x6af5b0u,native_context_ui_method<0x6af5b0u>}, // name store 0x6fda78
    {"SetResourceVisibleMode",0x7544a0u,native_context_ui_method<0x7544a0u>}, // name store 0x6fda92
    {"ToggleGridVisibleMode",0x66f8a0u,native_context_ui_method<0x66f8a0u>}, // name store 0x6fdaac
    {"SetGridVisibleMode",0x4ccde0u,native_context_ui_method<0x4ccde0u>}, // name store 0x6fdac9
    {"GetOriginalPlotCount",0x512680u,native_context_ui_method<0x512680u>}, // name store 0x6fdae3
    {"IsCityScreenUp",0x403d20u,native_context_ui_method<0x403d20u>}, // name store 0x6fdafd
    {"SetCityScreenUp",0x44ec30u,native_context_ui_method<0x44ec30u>}, // name store 0x6fdb1a
    {"CanEndTurn",0x6e95a0u,native_context_ui_method<0x6e95a0u>}, // name store 0x6fdb34
    {"WaitingForRemotePlayers",0x779700u,native_context_ui_method<0x779700u>}, // name store 0x6fdb4e
    {"SetInterfaceMode",0x465760u,native_context_ui_method<0x465760u>}, // name store 0x6fdb6b
    {"GetInterfaceMode",0x440a90u,native_context_ui_method<0x440a90u>}, // name store 0x6fdb85
    {"SetInterfaceModeValue",0x6a0900u,native_context_ui_method<0x6a0900u>}, // name store 0x6fdb9f
    {"GetInterfaceModeValue",0x54e1e0u,native_context_ui_method<0x54e1e0u>}, // name store 0x6fdbbc
    {"GetInterfaceModeDebugItemID1",0x71a390u,native_context_ui_method<0x71a390u>}, // name store 0x6fdbd6
    {"SetInterfaceModeDebugItemID1",0x42e120u,native_context_ui_method<0x42e120u>}, // name store 0x6fdbf0
    {"GetInterfaceModeDebugItemID2",0x6707b0u,native_context_ui_method<0x6707b0u>}, // name store 0x6fdc0d
    {"SetInterfaceModeDebugItemID2",0x498530u,native_context_ui_method<0x498530u>}, // name store 0x6fdc27
    {"SetFlashing",0x528cb0u,native_context_ui_method<0x528cb0u>}, // name store 0x6fdc41
    {"IsFlashing",0x776800u,native_context_ui_method<0x776800u>}, // name store 0x6fdc5e
    {"SetDiplomacyLocked",0x5dc700u,native_context_ui_method<0x5dc700u>}, // name store 0x6fdc78
    {"IsDiplomacyLocked",0x61eed0u,native_context_ui_method<0x61eed0u>}, // name store 0x6fdc92
    {"SetCanEndTurn",0x6e5ba0u,native_context_ui_method<0x6e5ba0u>}, // name store 0x6fdcaf
    {"IsHasMovedUnit",0x72e8a0u,native_context_ui_method<0x72e8a0u>}, // name store 0x6fdcc9
    {"SetHasMovedUnit",0x4531e0u,native_context_ui_method<0x4531e0u>}, // name store 0x6fdce3
    {"IsForcePopup",0x4393f0u,native_context_ui_method<0x4393f0u>}, // name store 0x6fdd00
    {"SetForcePopup",0x510760u,native_context_ui_method<0x510760u>}, // name store 0x6fdd1a
    {"LookAtCityOffset",0x53f020u,native_context_ui_method<0x53f020u>}, // name store 0x6fdd34
    {"GetPlotListColumn",0x718460u,native_context_ui_method<0x718460u>}, // name store 0x6fdd51
    {"VerifyPlotListColumn",0x6df880u,native_context_ui_method<0x6df880u>}, // name store 0x6fdd6b
    {"GetPlotListOffset",0x4cf950u,native_context_ui_method<0x4cf950u>}, // name store 0x6fdd85
    {"ExitingToMainMenu",0x5ee510u,native_context_ui_method<0x5ee510u>}, // name store 0x6fdda2
    {"IsInAdvancedStart",0x753ed0u,native_context_ui_method<0x753ed0u>}, // name store 0x6fddbc
    {"SetInAdvancedStart",0x44f080u,native_context_ui_method<0x44f080u>}, // name store 0x6fddd6
    {"SetBusy",0x401a40u,native_context_ui_method<0x401a40u>}, // name store 0x6fddf3
    {"IsNoSelectionListCycle",0x701d40u,native_context_ui_method<0x701d40u>}, // name store 0x6fde0d
    {"SetNoSelectionListCycle",0x637310u,native_context_ui_method<0x637310u>}, // name store 0x6fde27
    {"LocationSelect",0x68c770u,native_context_ui_method<0x68c770u>}, // name store 0x6fde44
    {"DoSelectCityAtPlot",0x5b2540u,native_context_ui_method<0x5b2540u>}, // name store 0x6fde5e
    {"AltKeyDown",0x494340u,native_context_ui_method<0x494340u>}, // name store 0x6fde78
    {"ShiftKeyDown",0x6a8950u,native_context_ui_method<0x6a8950u>}, // name store 0x6fde95
    {"CtrlKeyDown",0x4f9f10u,native_context_ui_method<0x4f9f10u>}, // name store 0x6fdeaf
    {"GetMouseOverHex",0x44a1a0u,native_context_ui_method<0x44a1a0u>}, // name store 0x6fdec9
    {"GetScratchDeal",0x56f9e0u,native_context_ui_method<0x56f9e0u>}, // name store 0x6fdee6
    {"ActivateNotification",0x4ab020u,native_context_ui_method<0x4ab020u>}, // name store 0x6fdf00
    {"RemoveNotification",0x52ee10u,native_context_ui_method<0x52ee10u>}, // name store 0x6fdf1a
    {"RebroadcastNotifications",0x5dd530u,native_context_ui_method<0x5dd530u>}, // name store 0x6fdf37
    {"GetVersionInfo",0x497fd0u,native_context_ui_method<0x497fd0u>}, // name store 0x6fdf51
    {"ExitGame",0x6454a0u,native_context_ui_method<0x6454a0u>}, // name store 0x6fdf6b
    {"SaveFileList",0x6084e0u,native_context_ui_method<0x6084e0u>}, // name store 0x6fdf88
    {"LoadSaveFile",0x699d20u,native_context_ui_method<0x699d20u>}, // name store 0x6fdfa2
    {"QuickSave",0x6626b0u,native_context_ui_method<0x6626b0u>}, // name store 0x6fdfbc
    {"SaveGame",0x729970u,native_context_ui_method<0x729970u>}, // name store 0x6fdfd9
    {"DeleteSavedGame",0x711320u,native_context_ui_method<0x711320u>}, // name store 0x6fdff3
    {"GetSavedGameModificationTime",0x6756c0u,native_context_ui_method<0x6756c0u>}, // name store 0x6fe00d
    {"GetSavedGameModificationTimeRaw",0x457db0u,native_context_ui_method<0x457db0u>}, // name store 0x6fe02a
    {"CopyLastAutoSave",0x58ebf0u,native_context_ui_method<0x58ebf0u>}, // name store 0x6fe044
    {"SaveMap",0x642ad0u,native_context_ui_method<0x642ad0u>}, // name store 0x6fe05e
    {"GetMapPreview",0x5174e0u,native_context_ui_method<0x5174e0u>}, // name store 0x6fe07b
    {"GetMapPlayers",0x48ab30u,native_context_ui_method<0x48ab30u>}, // name store 0x6fe095
    {"IsMapScenario",0x6e8950u,native_context_ui_method<0x6e8950u>}, // name store 0x6fe0af
    {"ResetScenarioPlayerSlots",0x54be80u,native_context_ui_method<0x54be80u>}, // name store 0x6fe0cc
    {"MoveScenarioPlayerToSlot",0x685a30u,native_context_ui_method<0x685a30u>}, // name store 0x6fe0e6
    {"SetNextGameState",0x4f7620u,native_context_ui_method<0x4f7620u>}, // name store 0x6fe100
    {"GetCurrentGameState",0x7a3680u,native_context_ui_method<0x7a3680u>}, // name store 0x6fe11d
    {"incTurnTimerSemaphore",0x6370e0u,native_context_ui_method<0x6370e0u>}, // name store 0x6fe137
    {"decTurnTimerSemaphore",0x4d1320u,native_context_ui_method<0x4d1320u>}, // name store 0x6fe151
    {"interruptTurnTimer",0x6e9560u,native_context_ui_method<0x6e9560u>}, // name store 0x6fe16e
    {"OnHumanOpenedTradeScreen",0x6d2700u,native_context_ui_method<0x6d2700u>}, // name store 0x6fe188
    {"OnHumanDemand",0x43aeb0u,native_context_ui_method<0x43aeb0u>}, // name store 0x6fe1a2
    {"DoProposeDeal",0x4ccd60u,native_context_ui_method<0x4ccd60u>}, // name store 0x6fe1bf
    {"DoDemand",0x7ad360u,native_context_ui_method<0x7ad360u>}, // name store 0x6fe1d9
    {"DoEqualizeDealWithHuman",0x638560u,native_context_ui_method<0x638560u>}, // name store 0x6fe1f3
    {"DoWhatWillAIGive",0x5a51f0u,native_context_ui_method<0x5a51f0u>}, // name store 0x6fe210
    {"DoWhatDoesAIWant",0x669470u,native_context_ui_method<0x669470u>}, // name store 0x6fe22a
    {"IsAIRequestingConcessions",0x407590u,native_context_ui_method<0x407590u>}, // name store 0x6fe244
    {"IsHumanMakingDemand",0x48e220u,native_context_ui_method<0x48e220u>}, // name store 0x6fe261
    {"GetRepeatActionPlayer",0x7ae560u,native_context_ui_method<0x7ae560u>}, // name store 0x6fe27b
    {"SetRepeatActionPlayer",0x68ab50u,native_context_ui_method<0x68ab50u>}, // name store 0x6fe295
    {"GetStartDiploRepeatCount",0x415b10u,native_context_ui_method<0x415b10u>}, // name store 0x6fe2b2
    {"ChangeStartDiploRepeatCount",0x6381a0u,native_context_ui_method<0x6381a0u>}, // name store 0x6fe2cc
    {"GetOfferTradeRepeatCount",0x75f920u,native_context_ui_method<0x75f920u>}, // name store 0x6fe2e6
    {"SetOfferTradeRepeatCount",0x4ae230u,native_context_ui_method<0x4ae230u>}, // name store 0x6fe303
    {"ChangeOfferTradeRepeatCount",0x6843d0u,native_context_ui_method<0x6843d0u>}, // name store 0x6fe31d
    {"OpenTechTree",0x510c00u,native_context_ui_method<0x510c00u>}, // name store 0x6fe337
    {"BindMouse",0x6e6640u,native_context_ui_method<0x6e6640u>}, // name store 0x6fe354
    {"DebugKeyHandler",0x4e3250u,native_context_ui_method<0x4e3250u>}, // name store 0x6fe36e
    {"BulkHideUI",0x4ab710u,native_context_ui_method<0x4ab710u>}, // name store 0x6fe388
    {"HasMadeProposal",0x62c480u,native_context_ui_method<0x62c480u>}, // name store 0x6fe3a5
    {"ProposedDealExists",0x6ad350u,native_context_ui_method<0x6ad350u>}, // name store 0x6fe3bf
    {"LoadProposedDeal",0x53d290u,native_context_ui_method<0x53d290u>}, // name store 0x6fe3d9
    {"LoadCurrentDeal",0x6bebe0u,native_context_ui_method<0x6bebe0u>}, // name store 0x6fe3f6
    {"LoadHistoricDeal",0x6ac950u,native_context_ui_method<0x6ac950u>}, // name store 0x6fe410
    {"GetNumCurrentDeals",0x781390u,native_context_ui_method<0x781390u>}, // name store 0x6fe42a
    {"GetNumHistoricDeals",0x75ddb0u,native_context_ui_method<0x75ddb0u>}, // name store 0x6fe447
    {"DoFinalizePlayerDeal",0x4f5610u,native_context_ui_method<0x4f5610u>}, // name store 0x6fe461
    {"RequestMinimapBroadcast",0x43aee0u,native_context_ui_method<0x43aee0u>}, // name store 0x6fe47b
    {"GetTempString",0x535cd0u,native_context_ui_method<0x535cd0u>}, // name store 0x6fe498
    {"IsForceDiscussionModeQuitOnBack",0x46e680u,native_context_ui_method<0x46e680u>}, // name store 0x6fe4b2
    {"SetForceDiscussionModeQuitOnBack",0x6b86d0u,native_context_ui_method<0x6b86d0u>}, // name store 0x6fe4cc
    {"IsLoadedGame",0x670050u,native_context_ui_method<0x670050u>}, // name store 0x6fe4e9
    {"GetCredits",0x4da9e0u,native_context_ui_method<0x4da9e0u>}, // name store 0x6fe503
    {"GetHallofFameData",0x6a1ce0u,native_context_ui_method<0x6a1ce0u>}, // name store 0x6fe51d
    {"RequestLeaderboardScores",0x5daca0u,native_context_ui_method<0x5daca0u>}, // name store 0x6fe53a
    {"GetLeaderboardScores",0x590e60u,native_context_ui_method<0x590e60u>}, // name store 0x6fe554
    {"GetLeaderboardTitle",0x742910u,native_context_ui_method<0x742910u>}, // name store 0x6fe56e
    {"GetLeaderboardUserDataVersion",0x78ead0u,native_context_ui_method<0x78ead0u>}, // name store 0x6fe58b
    {"GetAvailableLeaderboards",0x672440u,native_context_ui_method<0x672440u>}, // name store 0x6fe5a5
    {"SetLeaderboard",0x6c5d10u,native_context_ui_method<0x6c5d10u>}, // name store 0x6fe5bf
    {"SetLeaderboardCategory",0x4cd2d0u,native_context_ui_method<0x4cd2d0u>}, // name store 0x6fe5dc
    {"ScrollLeaderboardUp",0x530490u,native_context_ui_method<0x530490u>}, // name store 0x6fe5f6
    {"ScrollLeaderboardDown",0x5585a0u,native_context_ui_method<0x5585a0u>}, // name store 0x6fe610
    {"GetDllGUID",0x786570u,native_context_ui_method<0x786570u>}, // name store 0x6fe62d
    {"SendPathfinderUpdate",0x5f1820u,native_context_ui_method<0x5f1820u>}, // name store 0x6fe647
    {"DebugFlag",0x6069b0u,native_context_ui_method<0x6069b0u>}, // name store 0x6fe661
    {"HasShownLegal",0x5070b0u,native_context_ui_method<0x5070b0u>}, // name store 0x6fe67e
    {"RequestLeaveLeader",0x455b10u,native_context_ui_method<0x455b10u>}, // name store 0x6fe698
    {"GetLeaderHeadRootUp",0x65ae60u,native_context_ui_method<0x65ae60u>}, // name store 0x6fe6b2
    {"SetLeaderHeadRootUp",0x5299c0u,native_context_ui_method<0x5299c0u>}, // name store 0x6fe6cf
    {"AddPopup",0x40f560u,native_context_ui_method<0x40f560u>}, // name store 0x6fe6e9
    {"IsDX9",0x59a870u,native_context_ui_method<0x59a870u>}, // name store 0x6fe703
    {"AreMediumLeadersAllowed",0x6363f0u,native_context_ui_method<0x6363f0u>}, // name store 0x6fe720
    {"UnlockAchievement",0x627030u,native_context_ui_method<0x627030u>}, // name store 0x6fe73a
    {"TestLeaderMessage",0x74fee0u,native_context_ui_method<0x74fee0u>}, // name store 0x6fe754
    {"GetReplayFiles",0x54c050u,native_context_ui_method<0x54c050u>}, // name store 0x6fe771
    {"DeleteReplayFile",0x62c800u,native_context_ui_method<0x62c800u>}, // name store 0x6fe78b
    {"GetReplayFileHeader",0x4e7a90u,native_context_ui_method<0x4e7a90u>}, // name store 0x6fe7a5
    {"GetReplayModificationTime",0x48c6c0u,native_context_ui_method<0x48c6c0u>}, // name store 0x6fe7c2
    {"GetReplayModificationTimeRaw",0x5d5470u,native_context_ui_method<0x5d5470u>}, // name store 0x6fe7dc
    {"GetReplayInfo",0x4dd920u,native_context_ui_method<0x4dd920u>}, // name store 0x6fe7f6
    {"CompareFileTime",0x540f00u,native_context_ui_method<0x540f00u>}, // name store 0x6fe813
    {"SetAdvisorMessageHasBeenSeen",0x42be40u,native_context_ui_method<0x42be40u>}, // name store 0x6fe82d
    {"HasAdvisorMessageBeenSeen",0x631040u,native_context_ui_method<0x631040u>}, // name store 0x6fe847
    {"PauseSounds",0x4c7ed0u,native_context_ui_method<0x4c7ed0u>}, // name store 0x6fe864
    {"IsSoundPaused",0x5dacc0u,native_context_ui_method<0x5dacc0u>}, // name store 0x6fe87e
    {"AddPopupText",0x409460u,native_context_ui_method<0x409460u>}, // name store 0x6fe898
    {"SetDontShowPopups",0x5fc1b0u,native_context_ui_method<0x5fc1b0u>}, // name store 0x6fe8b5
    {"GetDontShowPopups",0x575b70u,native_context_ui_method<0x575b70u>}, // name store 0x6fe8cf
    {"PostKeyMessage",0x575a60u,native_context_ui_method<0x575a60u>}, // name store 0x6fe8e9
    {"IsTouchScreenEnabled",0x6a44b0u,native_context_ui_method<0x6a44b0u>}, // name store 0x6fe906
    {"GetUnitFlagIcon",0x6e4a10u,native_context_ui_method<0x6e4a10u>}, // name store 0x6fe920
    {"GetUnitPortraitIcon",0x4b1f10u,native_context_ui_method<0x4b1f10u>}, // name store 0x6fe93a
};
static_assert(std::size(native_context_ui_entries)==207);
struct NativeContextUiRegistrationFrame final {
    Impl& host;const NativeContextOptionsFrame*const predecessor;
    const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;
    int keys_ref=LUA_NOREF;const void* keys=nullptr;
    int master_ref=LUA_NOREF,proxy_ref=LUA_NOREF;const void* master=nullptr;const void* proxy=nullptr;const void* meta=nullptr;
    std::size_t fields_created=0,stores=0;Job* actual_job=nullptr;
    explicit NativeContextUiRegistrationFrame(Impl& p):host(p),predecessor(p.options_frame),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(host.ui_registration_frame||!predecessor)throw ScriptError("UI registrar requires actual OptionsManager predecessor");
        predecessor->first();if(predecessor->stores!=1||predecessor->master_ref<0||predecessor->proxy_ref<0||!predecessor->master||!predecessor->proxy)throw ScriptError("UI original predecessor did not execute original master/copies");
        host.ui_registration_frame=this;
    }
    ~NativeContextUiRegistrationFrame(){if(host.ui_registration_frame==this)host.ui_registration_frame=nullptr;}
    void first()const {
        if(host.ui_registration_frame!=this||creator!=std::this_thread::get_id()||creator!=host.ui_thread||!predecessor||host.options_frame!=predecessor||
           epoch!=host.failure_epoch||writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty)
            throw ScriptError("UI registrar actual source/frame changed");
        predecessor->first();
    }
    void full()const {first();predecessor->full();first();}
};
// Retained physical publication data minted only after the actual selected
// UI store returned. Weak Source controls do not prolong the outside owner.
// Registry roots remain under this Host/VM custody until its real lua_close.
struct NativeContextUiNamespaceReturn final{
    lua_State*const vm;global_State*const global;
    const std::size_t child;const int environment_ref;const void*const environment;
    const std::weak_ptr<const ScriptUiChildSource> source;const ScriptUiChildSource*const source_get;
    const int ui_keys_ref;const void*const ui_keys;
    const int readonly_keys_ref;const void*const readonly_keys;
    const int master_globals_ref;const void*const master_globals;
    const int master_ref,proxy_ref;const void*const master;const void*const proxy;const void*const meta;
    NativeContextUiNamespaceReturn(const Impl& p,const NativeContextUiRegistrationFrame& f)
        :vm(p.state),global(G(p.state)),child(p.lua_events_frame->child),
         environment_ref(p.lua_events_frame->environment),environment(p.lua_events_frame->actual_environment),
         source(p.child_contexts[child].source),source_get(p.child_contexts[child].source.get()),
         ui_keys_ref(f.keys_ref),ui_keys(f.keys),readonly_keys_ref(p.api_registration_frame->keys_ref),readonly_keys(p.api_registration_frame->keys),
         master_globals_ref(p.native_master_thread_bootstrap->globals_ref),master_globals(p.native_master_thread_bootstrap->globals),master_ref(f.master_ref),proxy_ref(f.proxy_ref),
         master(f.master),proxy(f.proxy),meta(f.meta){}
    NativeContextUiNamespaceReturn(const NativeContextUiNamespaceReturn&)=delete;
};
static void native_context_ui_key(lua_State* L,int i){
    const auto& p=owner(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_context_ui_registration_keys_ref);lua_rawgeti(L,-1,i);lua_remove(L,-2);
}
static void native_context_ui_registration_full(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.ui_registration_frame;p.source_exception={};p.source_error=nullptr;
    if(job.owner!=&p||job.operation!=Operation::native_context_ui_registration||!p.calling||!p.state||L!=p.state||G(L)!=G(p.state))
        return (void)luaL_error(L,"UI registrar actual Job/VM changed");
    try{if(!f)throw ScriptError("UI registrar has no genuine entered frame");f->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="UI registrar SAME source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    if(f->actual_job&&f->actual_job!=&job)return (void)luaL_error(L,"UI registrar actual entered Job replaced");
    if(!f->predecessor->actual_job)return (void)luaL_error(L,"UI genuine OptionsManager predecessor Job retired");
    native_options_registration_full(L,*f->predecessor->actual_job);
    p.source_exception={};p.source_error=nullptr;
    try{if(p.ui_registration_frame!=f)throw ScriptError("UI frame replaced after source callbacks");f->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="UI registrar FIRST after callbacks refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    const int top=lua_gettop(L);
    if(f->keys_ref!=LUA_NOREF){
        if(f->keys_ref!=p.native_context_ui_registration_keys_ref)return (void)luaL_error(L,"UI registrar keys custody changed");
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->keys||lua_objlen(L,-1)!=208||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"UI registrar retained key table changed");
        for(int i=0;i<208;++i){
            lua_rawgeti(L,-1,i+1);std::size_t n=0;const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
            if(!key||n!=std::strlen(i==0?"UI":native_context_ui_entries[i-1].name)||std::memcmp(key,i==0?"UI":native_context_ui_entries[i-1].name,n))
                return (void)luaL_error(L,"UI registrar retained key TString changed");lua_pop(L,1);
        }lua_pop(L,1);
    }
    if(f->master_ref>=0){
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->master)return (void)luaL_error(L,"UI actual master table changed");
        native_context_ui_key(L,1);lua_rawget(L,LUA_GLOBALSINDEX);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"UI original master cache changed");lua_pop(L,2);
    }
    if(f->stores){
        if(f->stores!=1||f->proxy_ref<0||!f->proxy||!f->meta||f->master_ref<0)
            return (void)luaL_error(L,"UI selected store lacks actual return");
        lua_rawgeti(L,LUA_REGISTRYINDEX,p.lua_events_frame->environment);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.lua_events_frame->actual_environment||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"UI actual selected fenv changed");
        native_context_ui_key(L,1);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->proxy_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->proxy||!lua_rawequal(L,-1,-2)||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=f->meta)
            return (void)luaL_error(L,"UI selected proxy/ref/metatable changed");
        native_api_prefix_key(L,2);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"UI proxy actual master __index changed");lua_pop(L,2);
        native_api_prefix_key(L,3);lua_rawget(L,-2);
        if(lua_type(L,-1)!=LUA_TBOOLEAN||lua_toboolean(L,-1))return (void)luaL_error(L,"UI proxy original false __metatable changed");lua_pop(L,1);
        native_api_prefix_key(L,4);lua_rawget(L,-2);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_api_readonly_write||lua_getupvalue(L,-1,1))
            return (void)luaL_error(L,"UI proxy original write-error consumer changed");
    }
    lua_settop(L,top);
}
static int native_context_ui_factory(lua_State* L){
    auto& p=owner(L);auto*const f=p.ui_registration_frame;
    if(!f||!f->actual_job)return luaL_error(L,"Native UI constructor lacks its actual entered Job");
    auto& job=*f->actual_job;
    native_context_ui_registration_full(L,job);
    if(!f||f->fields_created||f->master_ref!=LUA_NOREF)return luaL_error(L,"Native UI668A80/6FD370 constructor replay refused");
    lua_createtable(L,0,0);const int table=lua_gettop(L);const void*const value=lua_topointer(L,table);native_context_ui_registration_full(L,job);
    for(int i=0;i<207;++i){
        native_context_ui_key(L,i+2);lua_pushcfunction(L,native_context_ui_entries[i].function);lua_rawset(L,table);++f->fields_created;native_context_ui_registration_full(L,job);
    }
    if(f->fields_created!=207||!lua_istable(L,table)||lua_topointer(L,table)!=value||lua_getmetatable(L,table))
        return luaL_error(L,"Native UI668A80/6FD370 actual namespace constructor changed");
    for(int i=0;i<207;++i){native_context_ui_key(L,i+2);lua_rawget(L,table);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_context_ui_entries[i].function||lua_getupvalue(L,-1,1))return luaL_error(L,"UI original zero-upvalue consumer changed");lua_pop(L,1);}
    return 1;
}
static void perform_native_context_ui_registration(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.ui_registration_frame;native_context_ui_registration_full(L,job);
    if(!f||f->stores||f->keys_ref!=LUA_NOREF||f->actual_job||!lua_checkstack(L,32))
        return (void)luaL_error(L,"UI original registrar replay/key custody/stack refused");
    f->actual_job=&job;
    if(p.native_context_ui_registration_keys_ref==LUA_NOREF){
        lua_createtable(L,208,0);const int keys=lua_gettop(L);const void*const key_value=lua_topointer(L,keys);native_context_ui_registration_full(L,job);
        for(int i=0;i<208;++i){lua_pushstring(L,i==0?"UI":native_context_ui_entries[i-1].name);lua_rawseti(L,keys,i+1);native_context_ui_registration_full(L,job);}
        lua_pushvalue(L,keys);const int key_ref=luaL_ref(L,LUA_REGISTRYINDEX);
        if(key_ref<0)return (void)luaL_error(L,"UI actual retained key allocation failed");
        f->keys_ref=key_ref;f->keys=key_value;p.native_context_ui_registration_keys_ref=key_ref;p.native_context_ui_registration_keys_pointer=key_value;
        native_context_ui_registration_full(L,job);lua_pop(L,1);
    }else{
        f->keys_ref=p.native_context_ui_registration_keys_ref;f->keys=p.native_context_ui_registration_keys_pointer;native_context_ui_registration_full(L,job);
    }
    // Original6DFB60/5605C0 selects actual518 before inspecting
    // GLOBALS.UI; a cache miss runs668A80/6FD370 zero args/one result.
    perform_native_context_state_query(L,job);native_context_ui_registration_full(L,job);const int environment=lua_gettop(L);
    if(lua_topointer(L,environment)!=p.lua_events_frame->actual_environment)
        return (void)luaL_error(L,"UI registrar actual prepared518 environment changed");
    native_context_ui_key(L,1);lua_gettable(L,LUA_GLOBALSINDEX);native_context_ui_registration_full(L,job);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);lua_pushcfunction(L,native_context_ui_factory);lua_call(L,0,1);native_context_ui_registration_full(L,job);
        native_context_ui_key(L,1);lua_pushvalue(L,-2);lua_settable(L,LUA_GLOBALSINDEX);native_context_ui_registration_full(L,job);
    }
    if(!lua_istable(L,-1))return (void)luaL_error(L,"UI original namespace master is not a table");
    f->master=lua_topointer(L,-1);lua_pushvalue(L,-1);f->master_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_context_ui_registration_full(L,job);
    native_api_readonly_proxy(L,job);native_context_ui_registration_full(L,job);const int proxy=lua_gettop(L);f->proxy=lua_topointer(L,proxy);
    if(!lua_getmetatable(L,proxy))return (void)luaL_error(L,"UI original49B190 proxy has no metatable");f->meta=lua_topointer(L,-1);lua_pop(L,1);
    native_context_ui_key(L,1);lua_pushvalue(L,proxy);lua_settable(L,environment);native_context_ui_registration_full(L,job);
    lua_pushvalue(L,proxy);f->proxy_ref=luaL_ref(L,LUA_REGISTRYINDEX);++f->stores;native_context_ui_registration_full(L,job);
    p.source_exception={};p.source_error=nullptr;
    try{
        f->first();const auto child=p.lua_events_frame->child;
        if(child>=p.child_contexts.size()||p.child_contexts[child].retired||!p.child_contexts[child].source||
           !p.api_registration_frame||p.api_registration_frame->keys_ref<0||!p.api_registration_frame->keys||
           !p.native_master_thread_bootstrap||p.native_master_thread_bootstrap->globals_ref<0||!p.native_master_thread_bootstrap->globals)
            throw ScriptError("UI selected publication has no actual retained child/source");
        for(const auto& prior:p.native_context_ui_namespace_returns)
            if(prior&&prior->child==child)throw ScriptError("UI same selected child publication repeated");
        p.native_context_ui_namespace_returns.push_back(std::unique_ptr<const NativeContextUiNamespaceReturn>(new NativeContextUiNamespaceReturn(p,*f)));
        f->first();
    }catch(...){p.source_exception=std::current_exception();p.source_error="UI actual namespace return custody refused";}
    if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
    native_context_ui_registration_full(L,job);
    job.context_ui_master_reference=f->master_ref;job.context_ui_proxy_reference=f->proxy_ref;
    job.context_ui_master_table=f->master;job.context_ui_proxy_table=f->proxy;lua_pop(L,2);
}

static void native_context_ui_retained_proxy_key_owned(lua_State* L,const NativeContextUiNamespaceReturn& actual,int i){
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual.readonly_keys_ref);lua_rawgeti(L,-1,i);lua_remove(L,-2);
}
// Called by the later genuine selected installer; never admits a caller table
// by shape alone. Return false retains its existing foreign-namespace refusal.
static bool native_context_ui_selected_namespace_physical_owned(lua_State* L,int environment){
    auto& p=owner(L);
    const int top=lua_gettop(L);if(environment<0)environment=top+environment+1;
    if(environment<=0||environment>top||!lua_istable(L,environment))return false;
    const NativeContextUiNamespaceReturn* actual=nullptr;
    for(const auto& candidate:p.native_context_ui_namespace_returns)
        if(candidate&&candidate->environment==lua_topointer(L,environment)){
            if(actual)return false;actual=candidate.get();
        }
    if(!actual||actual->vm!=p.state||actual->global!=G(L)||G(p.state)!=actual->global||
       actual->child>=p.child_contexts.size()||actual->master_ref<0||actual->proxy_ref<0||actual->readonly_keys_ref<0||!actual->readonly_keys||
       actual->ui_keys_ref<0||!actual->ui_keys||p.native_context_ui_registration_keys_ref!=actual->ui_keys_ref||
       p.native_context_ui_registration_keys_pointer!=actual->ui_keys||
       actual->master_globals_ref<0||!actual->master_globals||!ttistable(gt(p.state))||hvalue(gt(p.state))!=actual->master_globals||
       !actual->master||!actual->proxy||!actual->meta||actual->source.expired())return false;
    if(!p.worldview_native_host||p.worldview_native_host->child!=actual->child||
       actual->source.owner_before(p.worldview_native_host->child_source)||p.worldview_native_host->child_source.owner_before(actual->source)||
       !p.worldview_native_child_controls_current(actual->child))return false;
    const auto& child=p.child_contexts[actual->child];
    if(child.retired||!child.state_prepared||child.environment!=actual->environment_ref||!child.source||child.source.get()!=actual->source_get||
       actual->source.owner_before(child.source)||child.source.owner_before(actual->source))return false;
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->ui_keys_ref);
    bool ui_keys_current=lua_istable(L,-1)&&lua_topointer(L,-1)==actual->ui_keys&&lua_objlen(L,-1)==208&&!lua_getmetatable(L,-1);
    if(ui_keys_current)for(int i=0;i<208;++i){lua_rawgeti(L,-1,i+1);std::size_t n=0;
        const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        const char*const expected=i==0?"UI":native_context_ui_entries[i-1].name;
        ui_keys_current=key&&n==std::strlen(expected)&&!std::memcmp(key,expected,n);
        lua_pop(L,1);if(!ui_keys_current)break;
    }
    lua_settop(L,top);if(!ui_keys_current)return false;
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->readonly_keys_ref);
    bool keys_current=lua_istable(L,-1)&&lua_topointer(L,-1)==actual->readonly_keys&&lua_objlen(L,-1)==4&&!lua_getmetatable(L,-1);
    if(keys_current)for(int i=0;i<4;++i){lua_rawgeti(L,-1,i+1);std::size_t n=0;
        const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        keys_current=key&&n==std::strlen(native_api_prefix_keys[i])&&!std::memcmp(key,native_api_prefix_keys[i],n);
        lua_pop(L,1);if(!keys_current)break;
    }
    lua_settop(L,top);if(!keys_current)return false;
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->environment_ref);
    bool current=lua_istable(L,-1)&&lua_rawequal(L,-1,environment)&&!lua_getmetatable(L,-1);
    lua_settop(L,top);if(!current)return false;
    native_context_ui_key(L,1);lua_rawget(L,environment);lua_rawgeti(L,LUA_REGISTRYINDEX,actual->proxy_ref);
    current=lua_istable(L,-1)&&lua_topointer(L,-1)==actual->proxy&&lua_rawequal(L,-1,-2)&&
        lua_getmetatable(L,-1)&&lua_topointer(L,-1)==actual->meta;
    lua_settop(L,top);if(!current)return false;
    // Original49B190 creates an empty readonly proxy. A raw own field can
    // shadow its genuine master despite an unchanged readonly metatable.
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->proxy_ref);lua_pushnil(L);
    current=lua_next(L,-2)==0;lua_settop(L,top);if(!current)return false;
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->master_ref);const int master=lua_gettop(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->master_globals_ref);
    current=lua_istable(L,-1)&&lua_topointer(L,-1)==actual->master_globals;
    if(!current){lua_settop(L,top);return false;}
    native_context_ui_key(L,1);lua_rawget(L,-2);lua_remove(L,-2);
    current=lua_istable(L,master)&&lua_topointer(L,master)==actual->master&&lua_rawequal(L,-1,master)&&!lua_getmetatable(L,master);
    lua_settop(L,master);if(!current){lua_settop(L,top);return false;}
    for(int i=0;i<207;++i){native_context_ui_key(L,i+2);lua_rawget(L,master);
        current=lua_iscfunction(L,-1)&&lua_tocfunction(L,-1)==native_context_ui_entries[i].function&&!lua_getupvalue(L,-1,1);
        lua_settop(L,master);if(!current){lua_settop(L,top);return false;}
    }
    lua_settop(L,top);lua_rawgeti(L,LUA_REGISTRYINDEX,actual->proxy_ref);lua_getmetatable(L,-1);const int meta=lua_gettop(L);
    native_context_ui_retained_proxy_key_owned(L,*actual,2);lua_rawget(L,meta);lua_rawgeti(L,LUA_REGISTRYINDEX,actual->master_ref);
    current=lua_rawequal(L,-1,-2);lua_settop(L,meta);
    native_context_ui_retained_proxy_key_owned(L,*actual,3);lua_rawget(L,meta);
    current=current&&lua_type(L,-1)==LUA_TBOOLEAN&&!lua_toboolean(L,-1);lua_settop(L,meta);
    native_context_ui_retained_proxy_key_owned(L,*actual,4);lua_rawget(L,meta);
    current=current&&lua_iscfunction(L,-1)&&lua_tocfunction(L,-1)==native_api_readonly_write&&!lua_getupvalue(L,-1,1);
    lua_settop(L,top);return current;
}
static bool native_context_ui_selected_namespace_current_owned(lua_State* L,int environment){
    if(!lua_checkstack(L,16))return false;
    native_db_query_current(L);worldview_require_lua_origin(L);
    if(!native_context_ui_selected_namespace_physical_owned(L,environment))return false;
    // Reacquire all record/source/table data after callbackful Source POST.
    // No source borrower or stale record pointer crosses this boundary.
    native_db_query_current(L);worldview_require_lua_origin(L);
    return native_context_ui_selected_namespace_physical_owned(L,environment);
}
void native_context_ui_registration_retire()noexcept{ui_registration_frame=nullptr;}
