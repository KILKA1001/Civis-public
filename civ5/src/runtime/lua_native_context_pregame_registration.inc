// Inside the genuine Host::Impl, after the exact Profiler registration unit.
// Actual521E90 calls protected8F8430(selected518,751450,0). The callback
// publishes a writable selected-global table before its104 method closures.
// Registration invokes no PreGame method and reads no invented game value.
class NativePreGameBackendBinding final {
    NativePreGameBackendBinding()=delete;
    NativePreGameBackendBinding(const NativePreGameBackendBinding&)=delete;
    NativePreGameBackendBinding& operator=(const NativePreGameBackendBinding&)=delete;
    Impl* actual_host=nullptr;lua_State* actual_vm=nullptr;
    const ScriptSourceProvider* actual_source=nullptr;
    std::uint32_t original_function=0;lua_CFunction actual_callback=nullptr;
    std::weak_ptr<EndTurnQueue::VmLifetime> lifetime;bool retired=false;
    friend struct Impl;
public:~NativePreGameBackendBinding()=default;
};
template<std::uint32_t Original> static int native_pregame_method_owned(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);
    // This existing complete Source-backed consumer owns its protected lender,
    // actual ActionInfoPanel caller/fenv/thread and real multiplayer bool read.
    if constexpr(Original==0x00428c70u)return worldview_pregame_multiplayer(L);
    NativePreGameBackendBinding* binding=nullptr;
    for(const auto& issued:p.native_pregame_backend_bindings)
        if(issued&&issued->original_function==Original){
            if(binding)return luaL_error(L,"PreGame actual backend identity duplicated");binding=issued.get();
        }
    if(!binding){p.record_failure();return luaL_error(L,"PreGame native consumer %08X requires its genuine backend; provider unissued",static_cast<unsigned>(Original));}
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    lua_CFunction callback=nullptr;lua_State* vm=nullptr;const EndTurnQueue::VmLifetime* lifetime=nullptr;
    p.source_exception={};p.source_error=nullptr;
    try{
        p.worldview_require_callback_origin();
        bool owned=false;for(const auto& issued:p.native_pregame_backend_bindings)if(issued.get()==binding){owned=true;break;}
        if(!owned)throw ScriptError("PreGame binding retired during actual Source FIRST");
        const auto life=binding->lifetime.lock();
        if(binding->retired||binding->original_function!=Original||binding->actual_host!=&p||
           binding->actual_source!=&p.sources||binding->actual_vm!=p.state||!binding->actual_callback||
           binding->actual_callback==&Impl::native_pregame_method_owned<Original>||
           !life||life.get()!=p.end_turn_lifetime.get()||life.owner_before(p.end_turn_lifetime)||
           p.end_turn_lifetime.owner_before(life)||!life->alive||life->failed||life->vm!=p.state||life->host!=p.worldview_host_owner||
           epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty)
            throw ScriptError("PreGame actual backend/VM/Source custody retired");
        callback=binding->actual_callback;vm=binding->actual_vm;lifetime=life.get();
    }catch(...){p.source_exception=std::current_exception();p.source_error="PreGame genuine invoked backend FIRST refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    // No owning C++ local crosses the real complete callback's Lua jump path.
    const int count=callback(L);native_db_query_current(L);
    p.source_exception={};p.source_error=nullptr;
    try{
        p.worldview_require_callback_origin();
        bool owned=false;for(const auto& issued:p.native_pregame_backend_bindings)if(issued.get()==binding){owned=true;break;}
        if(!owned)throw ScriptError("PreGame binding retired during callback/Source POST");
        const auto life=binding->lifetime.lock();
        if(binding->retired||binding->original_function!=Original||binding->actual_host!=&p||binding->actual_source!=&p.sources||
           binding->actual_vm!=vm||vm!=p.state||binding->actual_callback!=callback||
           !life||life.get()!=lifetime||life.get()!=p.end_turn_lifetime.get()||life.owner_before(p.end_turn_lifetime)||
           p.end_turn_lifetime.owner_before(life)||!life->alive||life->failed||life->vm!=vm||life->host!=p.worldview_host_owner||
           epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty)
            throw ScriptError("PreGame real callback/lifetime/VM/Source changed");
    }catch(...){p.source_exception=std::current_exception();p.source_error="PreGame genuine invoked backend POST refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    if(count<0||count>lua_gettop(L)){p.record_failure();return luaL_error(L,"PreGame actual callback returned invalid result count");}
    return count;
}
struct NativePreGameEntry {const char*name;std::uint32_t original;lua_CFunction body;};
static constexpr NativePreGameEntry native_pregame_entries[]={
    {"GetCivilization",0x0050c180u,&Impl::native_pregame_method_owned<0x0050c180u>}, // 0x75147f -> 0x75148c
    {"SetCivilization",0x007a7220u,&Impl::native_pregame_method_owned<0x007a7220u>}, // 0x751499 -> 0x7514a9
    {"GetMultiplayerAIEnabled",0x00774030u,&Impl::native_pregame_method_owned<0x00774030u>}, // 0x7514b6 -> 0x7514c3
    {"SetMultiplayerAIEnabled",0x00576390u,&Impl::native_pregame_method_owned<0x00576390u>}, // 0x7514d0 -> 0x7514dd
    {"GetMapScript",0x00511a40u,&Impl::native_pregame_method_owned<0x00511a40u>}, // 0x7514ea -> 0x7514fa
    {"SetMapScript",0x005baae0u,&Impl::native_pregame_method_owned<0x005baae0u>}, // 0x751507 -> 0x751514
    {"RandomizeMapSeed",0x0057a7b0u,&Impl::native_pregame_method_owned<0x0057a7b0u>}, // 0x751521 -> 0x75152e
    {"GetLoadWBScenario",0x007ac5a0u,&Impl::native_pregame_method_owned<0x007ac5a0u>}, // 0x75153b -> 0x75154b
    {"SetLoadWBScenario",0x00675f10u,&Impl::native_pregame_method_owned<0x00675f10u>}, // 0x751558 -> 0x751565
    {"GetOverrideScenarioHandicap",0x00600ad0u,&Impl::native_pregame_method_owned<0x00600ad0u>}, // 0x751572 -> 0x75157f
    {"SetOverrideScenarioHandicap",0x00430f00u,&Impl::native_pregame_method_owned<0x00430f00u>}, // 0x75158c -> 0x75159c
    {"IsRandomMapScript",0x0042dce0u,&Impl::native_pregame_method_owned<0x0042dce0u>}, // 0x7515a9 -> 0x7515b6
    {"SetRandomMapScript",0x00631da0u,&Impl::native_pregame_method_owned<0x00631da0u>}, // 0x7515c3 -> 0x7515d0
    {"GetGameSpeed",0x005171d0u,&Impl::native_pregame_method_owned<0x005171d0u>}, // 0x7515dd -> 0x7515ed
    {"SetGameSpeed",0x005e4bd0u,&Impl::native_pregame_method_owned<0x005e4bd0u>}, // 0x7515fa -> 0x751607
    {"GetWorldSize",0x005b7b00u,&Impl::native_pregame_method_owned<0x005b7b00u>}, // 0x751614 -> 0x751621
    {"SetWorldSize",0x00561af0u,&Impl::native_pregame_method_owned<0x00561af0u>}, // 0x75162e -> 0x75163e
    {"IsEarthMap",0x005adec0u,&Impl::native_pregame_method_owned<0x005adec0u>}, // 0x75164b -> 0x751658
    {"SetEarthMap",0x00636410u,&Impl::native_pregame_method_owned<0x00636410u>}, // 0x751665 -> 0x751672
    {"IsRandomWorldSize",0x00784420u,&Impl::native_pregame_method_owned<0x00784420u>}, // 0x75167f -> 0x75168f
    {"SetRandomWorldSize",0x004077a0u,&Impl::native_pregame_method_owned<0x004077a0u>}, // 0x75169c -> 0x7516a9
    {"GetHandicap",0x0058f860u,&Impl::native_pregame_method_owned<0x0058f860u>}, // 0x7516b6 -> 0x7516c3
    {"SetHandicap",0x005f3c60u,&Impl::native_pregame_method_owned<0x005f3c60u>}, // 0x7516d0 -> 0x7516e0
    {"GetTeam",0x005fcd70u,&Impl::native_pregame_method_owned<0x005fcd70u>}, // 0x7516ed -> 0x7516fa
    {"SetTeam",0x004db400u,&Impl::native_pregame_method_owned<0x004db400u>}, // 0x751707 -> 0x751714
    {"GetLeaderName",0x005f1f50u,&Impl::native_pregame_method_owned<0x005f1f50u>}, // 0x751721 -> 0x751731
    {"SetLeaderName",0x005570e0u,&Impl::native_pregame_method_owned<0x005570e0u>}, // 0x75173e -> 0x75174b
    {"GetLeaderType",0x00565750u,&Impl::native_pregame_method_owned<0x00565750u>}, // 0x751758 -> 0x751765
    {"SetLeaderType",0x005ea070u,&Impl::native_pregame_method_owned<0x005ea070u>}, // 0x751772 -> 0x751782
    {"SetPlayerColor",0x00442490u,&Impl::native_pregame_method_owned<0x00442490u>}, // 0x75178f -> 0x75179c
    {"GetLeaderKey",0x004fc1b0u,&Impl::native_pregame_method_owned<0x004fc1b0u>}, // 0x7517a9 -> 0x7517b6
    {"SetLeaderKey",0x0070bea0u,&Impl::native_pregame_method_owned<0x0070bea0u>}, // 0x7517c3 -> 0x7517d3
    {"IsLeaderKeyAvailable",0x00610b70u,&Impl::native_pregame_method_owned<0x00610b70u>}, // 0x7517e0 -> 0x7517ed
    {"GetLeaderPackageID",0x006cca80u,&Impl::native_pregame_method_owned<0x006cca80u>}, // 0x7517fa -> 0x751807
    {"SetLeaderPackageID",0x004d6f20u,&Impl::native_pregame_method_owned<0x004d6f20u>}, // 0x751814 -> 0x751824
    {"GetLeaderPackageTextKey",0x005b7cc0u,&Impl::native_pregame_method_owned<0x005b7cc0u>}, // 0x751831 -> 0x75183e
    {"GetCivilizationDescription",0x006913d0u,&Impl::native_pregame_method_owned<0x006913d0u>}, // 0x75184b -> 0x751858
    {"SetCivilizationDescription",0x0068dbb0u,&Impl::native_pregame_method_owned<0x0068dbb0u>}, // 0x751865 -> 0x751875
    {"GetCivilizationShortDescription",0x005aafd0u,&Impl::native_pregame_method_owned<0x005aafd0u>}, // 0x751882 -> 0x75188f
    {"SetCivilizationShortDescription",0x005ee1d0u,&Impl::native_pregame_method_owned<0x005ee1d0u>}, // 0x75189c -> 0x7518a9
    {"GetCivilizationAdjective",0x004f85e0u,&Impl::native_pregame_method_owned<0x004f85e0u>}, // 0x7518b6 -> 0x7518c6
    {"SetCivilizationAdjective",0x0077faa0u,&Impl::native_pregame_method_owned<0x0077faa0u>}, // 0x7518d3 -> 0x7518e0
    {"GetCivilizationColor",0x00528e00u,&Impl::native_pregame_method_owned<0x00528e00u>}, // 0x7518ed -> 0x7518fa
    {"GetCivilizationKey",0x006aeb30u,&Impl::native_pregame_method_owned<0x006aeb30u>}, // 0x751907 -> 0x751917
    {"SetCivilizationKey",0x0051f210u,&Impl::native_pregame_method_owned<0x0051f210u>}, // 0x751924 -> 0x751931
    {"IsCivilizationKeyAvailable",0x006723c0u,&Impl::native_pregame_method_owned<0x006723c0u>}, // 0x75193e -> 0x75194b
    {"GetCivilizationPackageID",0x006220b0u,&Impl::native_pregame_method_owned<0x006220b0u>}, // 0x751958 -> 0x751968
    {"SetCivilizationPackageID",0x006a53a0u,&Impl::native_pregame_method_owned<0x006a53a0u>}, // 0x751975 -> 0x751982
    {"GetCivilizationPackageTextKey",0x00692fb0u,&Impl::native_pregame_method_owned<0x00692fb0u>}, // 0x75198f -> 0x75199c
    {"GetPassword",0x0064a070u,&Impl::native_pregame_method_owned<0x0064a070u>}, // 0x7519a9 -> 0x7519b9
    {"SetPassword",0x00517210u,&Impl::native_pregame_method_owned<0x00517210u>}, // 0x7519c6 -> 0x7519d3
    {"HasPassword",0x00657ae0u,&Impl::native_pregame_method_owned<0x00657ae0u>}, // 0x7519e0 -> 0x7519ed
    {"TestPassword",0x004c8c20u,&Impl::native_pregame_method_owned<0x004c8c20u>}, // 0x7519fa -> 0x751a0a
    {"GetLoadFileName",0x004fe420u,&Impl::native_pregame_method_owned<0x004fe420u>}, // 0x751a17 -> 0x751a24
    {"SetLoadFileName",0x005149b0u,&Impl::native_pregame_method_owned<0x005149b0u>}, // 0x751a31 -> 0x751a3e
    {"GetNickName",0x004da250u,&Impl::native_pregame_method_owned<0x004da250u>}, // 0x751a4b -> 0x751a5b
    {"SetNickName",0x004099e0u,&Impl::native_pregame_method_owned<0x004099e0u>}, // 0x751a68 -> 0x751a75
    {"GetSlotStatus",0x00710b70u,&Impl::native_pregame_method_owned<0x00710b70u>}, // 0x751a82 -> 0x751a8f
    {"SetSlotStatus",0x005ec5f0u,&Impl::native_pregame_method_owned<0x005ec5f0u>}, // 0x751a9c -> 0x751aac
    {"GetSlotClaim",0x005ece60u,&Impl::native_pregame_method_owned<0x005ece60u>}, // 0x751ab9 -> 0x751ac6
    {"SetSlotClaim",0x004353c0u,&Impl::native_pregame_method_owned<0x004353c0u>}, // 0x751ad3 -> 0x751ae0
    {"GetMaxTurns",0x00726b00u,&Impl::native_pregame_method_owned<0x00726b00u>}, // 0x751aed -> 0x751afd
    {"SetMaxTurns",0x004bacc0u,&Impl::native_pregame_method_owned<0x004bacc0u>}, // 0x751b0a -> 0x751b17
    {"GetPitbossTurnTime",0x005f0970u,&Impl::native_pregame_method_owned<0x005f0970u>}, // 0x751b24 -> 0x751b31
    {"SetPitbossTurnTime",0x0054a630u,&Impl::native_pregame_method_owned<0x0054a630u>}, // 0x751b3e -> 0x751b4e
    {"GetEra",0x00604850u,&Impl::native_pregame_method_owned<0x00604850u>}, // 0x751b5b -> 0x751b68
    {"SetEra",0x00663cb0u,&Impl::native_pregame_method_owned<0x00663cb0u>}, // 0x751b75 -> 0x751b82
    {"GetNumMinorCivs",0x004dfb00u,&Impl::native_pregame_method_owned<0x004dfb00u>}, // 0x751b8f -> 0x751b9f
    {"SetNumMinorCivs",0x0078cb40u,&Impl::native_pregame_method_owned<0x0078cb40u>}, // 0x751bac -> 0x751bb9
    {"SetPrivateGame",0x00724a20u,&Impl::native_pregame_method_owned<0x00724a20u>}, // 0x751bc6 -> 0x751bd3
    {"IsPrivateGame",0x00638f70u,&Impl::native_pregame_method_owned<0x00638f70u>}, // 0x751be0 -> 0x751bf0
    {"GetPersistSettings",0x006edfb0u,&Impl::native_pregame_method_owned<0x006edfb0u>}, // 0x751bfd -> 0x751c0a
    {"SetPersistSettings",0x0050bd70u,&Impl::native_pregame_method_owned<0x0050bd70u>}, // 0x751c17 -> 0x751c24
    {"LoadPreGameSettings",0x0059f130u,&Impl::native_pregame_method_owned<0x0059f130u>}, // 0x751c31 -> 0x751c41
    {"SetReady",0x00421910u,&Impl::native_pregame_method_owned<0x00421910u>}, // 0x751c4e -> 0x751c5b
    {"IsReady",0x0040d500u,&Impl::native_pregame_method_owned<0x0040d500u>}, // 0x751c68 -> 0x751c75
    {"CanReadyLocalPlayer",0x00486f10u,&Impl::native_pregame_method_owned<0x00486f10u>}, // 0x751c82 -> 0x751c92
    {"SetInternetGame",0x005e3370u,&Impl::native_pregame_method_owned<0x005e3370u>}, // 0x751c9f -> 0x751cac
    {"IsInternetGame",0x004c1670u,&Impl::native_pregame_method_owned<0x004c1670u>}, // 0x751cb9 -> 0x751cc6
    {"SetMultiplayerGame",0x00478910u,&Impl::native_pregame_method_owned<0x00478910u>}, // 0x751cd3 -> 0x751ce3
    {"IsMultiplayerGame",0x00428c70u,&Impl::native_pregame_method_owned<0x00428c70u>}, // 0x751cf0 -> 0x751cfd
    {"IsHotSeatGame",0x0061d720u,&Impl::native_pregame_method_owned<0x0061d720u>}, // 0x751d0a -> 0x751d17
    {"SetGameType",0x004aa3b0u,&Impl::native_pregame_method_owned<0x004aa3b0u>}, // 0x751d24 -> 0x751d34
    {"GetGameType",0x005115e0u,&Impl::native_pregame_method_owned<0x005115e0u>}, // 0x751d41 -> 0x751d4e
    {"GetGameOption",0x00614bc0u,&Impl::native_pregame_method_owned<0x00614bc0u>}, // 0x751d5b -> 0x751d68
    {"SetGameOption",0x006b4e20u,&Impl::native_pregame_method_owned<0x006b4e20u>}, // 0x751d75 -> 0x751d85
    {"ResetGameOptions",0x005130c0u,&Impl::native_pregame_method_owned<0x005130c0u>}, // 0x751d92 -> 0x751d9f
    {"GetMapOption",0x00631df0u,&Impl::native_pregame_method_owned<0x00631df0u>}, // 0x751dac -> 0x751db9
    {"SetMapOption",0x006cdfa0u,&Impl::native_pregame_method_owned<0x006cdfa0u>}, // 0x751dc6 -> 0x751dd6
    {"ResetMapOptions",0x00499dd0u,&Impl::native_pregame_method_owned<0x00499dd0u>}, // 0x751de3 -> 0x751df0
    {"ResetSlots",0x00416e70u,&Impl::native_pregame_method_owned<0x00416e70u>}, // 0x751dfd -> 0x751e0a
    {"Reset",0x006a7f60u,&Impl::native_pregame_method_owned<0x006a7f60u>}, // 0x751e17 -> 0x751e27
    {"GetFileHeader",0x004109e0u,&Impl::native_pregame_method_owned<0x004109e0u>}, // 0x751e34 -> 0x751e41
    {"IsVictory",0x004d5050u,&Impl::native_pregame_method_owned<0x004d5050u>}, // 0x751e4e -> 0x751e5b
    {"SetVictory",0x0058f2c0u,&Impl::native_pregame_method_owned<0x0058f2c0u>}, // 0x751e68 -> 0x751e78
    {"SetQuickCombat",0x006a6270u,&Impl::native_pregame_method_owned<0x006a6270u>}, // 0x751e85 -> 0x751e92
    {"GetQuickCombat",0x006da400u,&Impl::native_pregame_method_owned<0x006da400u>}, // 0x751e9f -> 0x751eac
    {"SetQuickMovement",0x00676310u,&Impl::native_pregame_method_owned<0x00676310u>}, // 0x751eb9 -> 0x751ec9
    {"GetQuickMovement",0x00649a20u,&Impl::native_pregame_method_owned<0x00649a20u>}, // 0x751ed6 -> 0x751ee3
    {"ReadActiveSlotCountFromSaveGame",0x005352d0u,&Impl::native_pregame_method_owned<0x005352d0u>}, // 0x751ef0 -> 0x751efd
    {"IsDLCAllowed",0x00436cd0u,&Impl::native_pregame_method_owned<0x00436cd0u>}, // 0x751f0a -> 0x751f1a
    {"SetDLCAllowed",0x006e1010u,&Impl::native_pregame_method_owned<0x006e1010u>}, // 0x751f27 -> 0x751f34
    {"ClearDLCAllowed",0x0043f240u,&Impl::native_pregame_method_owned<0x0043f240u>}, // 0x751f41 -> 0x751f4e
    {"GameStarted",0x0077d1f0u,&Impl::native_pregame_method_owned<0x0077d1f0u>}, // 0x751f5b -> 0x751f6b
};
struct NativeContextPreGameFrame final {
    Impl& host;const NativeContextProfilerRegistrationFrame*const predecessor;
    const NativeContextLuaEventsFrame*const selected;
    const ScriptSourceProvider*const source;
    const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;
    const std::size_t child;const int environment_ref,thread_ref;
    const void*const actual_environment;lua_State*const selected_thread;
    Job* actual_job=nullptr;const NativeMasterProtectedLuaBoundary* boundary=nullptr;
    int table_ref=LUA_NOREF;const void* table=nullptr;
    std::size_t fields_created=0;int protected_status=-1;
    bool global_published=false,callback_returned=false,returned=false;
    explicit NativeContextPreGameFrame(Impl&p):host(p),predecessor(p.profiler_registration_frame),selected(p.lua_events_frame),
        source(&p.sources),epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty),
        child(selected?selected->child:std::numeric_limits<std::size_t>::max()),
        environment_ref(selected?selected->environment:LUA_NOREF),thread_ref(selected?selected->thread:LUA_NOREF),
        actual_environment(selected?selected->actual_environment:nullptr),selected_thread(selected?selected->actual_thread:nullptr){
        if(p.pregame_frame||!predecessor||!selected||!selected_thread||!actual_environment)
            throw ScriptError("PreGame521E90 requires its genuine completed Profiler and actual selected518");
        predecessor->first();
        if(!predecessor->returned||predecessor->table_ref<0||!predecessor->table||predecessor->status!=0||!predecessor->actual_job||
           environment_ref<0||thread_ref<0||child>=p.child_contexts.size())
            throw ScriptError("PreGame521E90 predecessor did not return actual Profiler data/selected roots");
        p.pregame_frame=this;
    }
    ~NativeContextPreGameFrame(){host.native_selected_registrar_retire_for_frame_owned(this);if(host.pregame_frame==this)host.pregame_frame=nullptr;}
    NativeContextPreGameFrame(const NativeContextPreGameFrame&)=delete;
    NativeContextPreGameFrame& operator=(const NativeContextPreGameFrame&)=delete;
    void first()const {
        if(host.pregame_frame!=this||host.profiler_registration_frame!=predecessor||host.lua_events_frame!=selected||
           source!=&host.sources||creator!=std::this_thread::get_id()||creator!=host.ui_thread||
           epoch!=host.failure_epoch||writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty||host.failed.exhausted_||
           !predecessor||!selected||selected->child!=child||selected->environment!=environment_ref||selected->thread!=thread_ref||
           selected->actual_environment!=actual_environment||selected->actual_thread!=selected_thread||child>=host.child_contexts.size())
            throw ScriptError("PreGame actual Profiler/selected518/Source/failure frame changed");
        predecessor->first();selected->first();
        const auto& actual=host.child_contexts[child];
        if(actual.retired||!actual.state_prepared||actual.environment!=environment_ref||actual.execution_thread!=thread_ref)
            throw ScriptError("PreGame selected child lost its actual retained thread/environment");
    }
    void full()const{first();predecessor->full();first();}
};
// Retains only the actual completed constructor return. Original PreGame is
// writable: this witness does not freeze or regenerate its method fields.
class NativePreGameNamespaceReturn final {
    friend struct Impl;
    Impl*const host;const ScriptSourceProvider*const source;
    lua_State*const master;global_State*const actual_global;
    const std::thread::id creator;
    const std::uint64_t epoch,writes,uncertainty;
    const std::size_t child;const int environment_ref,thread_ref,table_ref;
    const void*const environment;const void*const table;lua_State*const thread;
    bool retired=false;
    NativePreGameNamespaceReturn(Impl&p,const NativeContextPreGameFrame&f):host(&p),source(&p.sources),master(p.state),
        actual_global(G(p.state)),creator(f.creator),epoch(f.epoch),writes(f.writes),uncertainty(f.uncertainty),
        child(f.child),environment_ref(f.environment_ref),thread_ref(f.thread_ref),table_ref(f.table_ref),
        environment(f.actual_environment),table(f.table),thread(f.selected_thread){
        f.first();
        if(!f.returned||!f.callback_returned||!f.global_published||f.protected_status||f.fields_created!=104||
           !f.actual_job||f.actual_job->owner!=&p||f.actual_job->operation!=Operation::native_context_pregame_registration||
           !master||!actual_global||table_ref<0||!table)
            throw ScriptError("PreGame namespace retention lacks the actual104-field protected normal return");
    }
    NativePreGameNamespaceReturn(const NativePreGameNamespaceReturn&)=delete;
    NativePreGameNamespaceReturn& operator=(const NativePreGameNamespaceReturn&)=delete;
public:~NativePreGameNamespaceReturn()=default;
};
// Seventh argument to the existing PreGame compatibility bootstrap. It decides
// from canonical Host custody, never from a caller native/prepared boolean.
static int native_pregame_preserve_prepared_namespace_owned(lua_State* L){
    auto& p=owner(L);
    if(!p.worldview_native_host){lua_pushboolean(L,0);return 1;}
    p.source_exception={};p.source_error=nullptr;
    try{p.worldview_require_callback_origin();}
    catch(...){p.source_exception=std::current_exception();p.source_error="PreGame prepared namespace preservation Source FIRST refused";}
    if(p.source_error)return native_master_thread_refuse(L,p.source_error);
    if(lua_gettop(L)!=1||lua_type(L,1)!=LUA_TTABLE)
        return native_master_thread_refuse(L,"PreGame namespace preserver requires its real selected environment operand");
    const auto child=p.worldview_native_host->child;
    NativePreGameNamespaceReturn* actual=nullptr;
    for(const auto& issued:p.native_pregame_namespace_returns)if(issued&&issued->child==child){
        if(actual)return native_master_thread_refuse(L,"PreGame namespace normal-return child custody duplicated");actual=issued.get();
    }
    if(!actual)return native_master_thread_refuse(L,"Native prepared PreGame has no genuine104-field normal-return owner");
    if(actual->retired||actual->host!=&p||actual->source!=&p.sources||actual->master!=p.state||!p.state||
       actual->actual_global!=G(L)||G(p.state)!=actual->actual_global||actual->creator!=std::this_thread::get_id()||
       actual->creator!=p.ui_thread||actual->epoch!=p.failure_epoch||actual->writes!=p.failed.writes_||
       actual->uncertainty!=p.execution_uncertainty||p.failed.exhausted_||child>=p.child_contexts.size()||
       p.child_contexts[child].retired||!p.child_contexts[child].state_prepared||
       p.child_contexts[child].environment!=actual->environment_ref||p.child_contexts[child].execution_thread!=actual->thread_ref)
        return native_master_thread_refuse(L,"PreGame actual constructor return/Source/VM/child roots retired");
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->environment_ref);
    if(lua_topointer(L,-1)!=actual->environment||!lua_rawequal(L,1,-1))
        return native_master_thread_refuse(L,"PreGame offered environment differs from genuine selected fenv");
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->thread_ref);
    if(lua_tothread(L,-1)!=actual->thread)return native_master_thread_refuse(L,"PreGame selected thread root changed");
    lua_getfenv(L,-1);
    if(!lua_rawequal(L,-1,1))return native_master_thread_refuse(L,"PreGame selected thread actual fenv rebound");
    lua_settop(L,1);lua_pushliteral(L,"PreGame");lua_rawget(L,1);lua_rawgeti(L,LUA_REGISTRYINDEX,actual->table_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=actual->table||!lua_rawequal(L,-1,-2))
        return native_master_thread_refuse(L,"PreGame canonical mutable table was replaced before compatibility configuration");
    lua_settop(L,1);p.source_exception={};p.source_error=nullptr;
    try{p.worldview_require_callback_origin();}
    catch(...){p.source_exception=std::current_exception();p.source_error="PreGame namespace preservation Source POST refused";}
    if(p.source_error)return native_master_thread_refuse(L,p.source_error);
    bool owned=false;for(const auto& issued:p.native_pregame_namespace_returns)if(issued.get()==actual){owned=true;break;}
    if(!owned||actual->retired||actual->host!=&p||actual->source!=&p.sources||actual->master!=p.state||!p.state||
       actual->actual_global!=G(L)||G(p.state)!=actual->actual_global||actual->creator!=std::this_thread::get_id()||
       actual->creator!=p.ui_thread||actual->epoch!=p.failure_epoch||actual->writes!=p.failed.writes_||
       actual->uncertainty!=p.execution_uncertainty||p.failed.exhausted_||!p.worldview_native_host||
       p.worldview_native_host->child!=child||child>=p.child_contexts.size()||p.child_contexts[child].retired||
       !p.child_contexts[child].state_prepared||p.child_contexts[child].environment!=actual->environment_ref||
       p.child_contexts[child].execution_thread!=actual->thread_ref)
        return native_master_thread_refuse(L,"PreGame actual namespace owner retired during Source POST");
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->environment_ref);
    if(lua_topointer(L,-1)!=actual->environment||!lua_rawequal(L,1,-1))
        return native_master_thread_refuse(L,"PreGame actual environment changed during Source POST");
    lua_rawgeti(L,LUA_REGISTRYINDEX,actual->thread_ref);
    if(lua_tothread(L,-1)!=actual->thread)return native_master_thread_refuse(L,"PreGame actual selected thread changed during Source POST");
    lua_getfenv(L,-1);
    if(!lua_rawequal(L,-1,1))return native_master_thread_refuse(L,"PreGame selected fenv changed during Source POST");
    lua_settop(L,1);lua_pushliteral(L,"PreGame");lua_rawget(L,1);lua_rawgeti(L,LUA_REGISTRYINDEX,actual->table_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=actual->table||!lua_rawequal(L,-1,-2))
        return native_master_thread_refuse(L,"PreGame actual table publication changed during Source POST");
    lua_settop(L,1);
    // This boolean reports an actual verified table identity; it is not a game
    // value or a registration/grant supplied by a script.
    lua_pushboolean(L,1);return 1;
}
static void native_context_pregame_registration_full(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.pregame_frame;p.source_exception={};p.source_error=nullptr;
    try{if(!f)throw ScriptError("PreGame registrar has no actual entered frame");f->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="PreGame registrar genuine Source FULL refused";}
    if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
    if(p.pregame_frame!=f||job.owner!=&p||job.operation!=Operation::native_context_pregame_registration||
       (f->actual_job&&f->actual_job!=&job)||job.worldview_child!=f->child||!p.state||G(L)!=G(p.state)||
       !f->predecessor->actual_job)
        return (void)native_master_thread_refuse(L,"PreGame registrar actual Job/VM/Profiler custody changed");
    native_context_profiler_registration_full(L,*f->predecessor->actual_job);
    if(p.pregame_frame!=f)return (void)native_master_thread_refuse(L,"PreGame owning frame replaced during predecessor FULL");
    p.source_exception={};p.source_error=nullptr;
    try{f->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="PreGame registrar FIRST after callbacks refused";}
    if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
    if(f->boundary){
        if(p.native_master_protected_boundary!=f->boundary||&f->boundary->actual_job!=&job)
            return (void)native_master_thread_refuse(L,"PreGame genuine protected boundary/Job changed");
        p.source_exception={};p.source_error=nullptr;
        try{f->boundary->current_without_lua();}
        catch(...){p.source_exception=std::current_exception();p.source_error="PreGame actual process CS/VM boundary FIRST refused";}
        if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
    }
    const int top=lua_gettop(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->thread_ref);
    if(lua_tothread(L,-1)!=f->selected_thread||lua_status(f->selected_thread))
        return (void)native_master_thread_refuse(L,"PreGame actual retained518 coroutine changed");
    lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,f->environment_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->actual_environment||!lua_rawequal(L,-1,-2)||lua_getmetatable(L,-1))
        return (void)native_master_thread_refuse(L,"PreGame actual selected518 fenv/root changed");
    if(L==f->selected_thread){lua_pushvalue(L,LUA_GLOBALSINDEX);
        if(!lua_rawequal(L,-1,-2))return (void)native_master_thread_refuse(L,"PreGame callback does not operate on actual selected fenv");lua_pop(L,1);}
    lua_settop(L,top);
    if(f->table_ref>=0){
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->table_ref);const int table=lua_gettop(L);
        if(!lua_istable(L,table)||lua_topointer(L,table)!=f->table||lua_getmetatable(L,table))
            return (void)native_master_thread_refuse(L,"PreGame genuine writable constructor table changed");
        if(f->global_published){
            lua_rawgeti(L,LUA_REGISTRYINDEX,f->environment_ref);lua_pushliteral(L,"PreGame");lua_rawget(L,-2);
            if(!lua_rawequal(L,-1,table))return (void)native_master_thread_refuse(L,"PreGame actual selected global publication changed");lua_pop(L,2);
        }
        if(f->fields_created>std::size(native_pregame_entries))return (void)native_master_thread_refuse(L,"PreGame constructor field journal overflow");
        for(std::size_t ordinal=0;ordinal<f->fields_created;++ordinal){
            lua_pushstring(L,native_pregame_entries[ordinal].name);lua_rawget(L,table);
            if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_pregame_entries[ordinal].body||lua_getupvalue(L,-1,1))
                return (void)native_master_thread_refuse(L,"PreGame actual zero-upvalue field/identity changed");lua_pop(L,1);
        }
    }
    lua_settop(L,top);
}
static int native_pregame_register_751450_owned(lua_State* L){
    auto& p=owner(L);auto*const f=p.pregame_frame;
    if(!f||!f->actual_job||L!=f->selected_thread||f->table_ref!=LUA_NOREF||f->fields_created||f->callback_returned)
        return native_master_thread_refuse(L,"PreGame751450 requires its actual selected callback without replay");
    native_selected_registrar_callback_enter_owned(L);
    auto& job=*f->actual_job;native_context_pregame_registration_full(L,job);
    if(!lua_checkstack(L,24))return native_master_thread_refuse(L,"PreGame original selected callback stack unavailable");
    lua_createtable(L,0,0);const int table=lua_gettop(L);f->table=lua_topointer(L,table);
    native_context_pregame_registration_full(L,job);
    // This is the actual original early publication, before all104 closures.
    lua_pushvalue(L,-1);native_context_pregame_registration_full(L,job);
    lua_setfield(L,LUA_GLOBALSINDEX,"PreGame");f->global_published=true;native_context_pregame_registration_full(L,job);
    // Separate Host retention does not replace or defer that selected write.
    lua_pushvalue(L,table);f->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    if(f->table_ref<0)return native_master_thread_refuse(L,"PreGame actual constructor table retention failed");
    native_context_pregame_registration_full(L,job);
    for(std::size_t ordinal=0;ordinal<std::size(native_pregame_entries);++ordinal){
        lua_pushcclosure(L,native_pregame_entries[ordinal].body,0);native_context_pregame_registration_full(L,job);
        lua_setfield(L,table,native_pregame_entries[ordinal].name);++f->fields_created;
        native_context_pregame_registration_full(L,job);
    }
    if(f->fields_created!=104||lua_topointer(L,table)!=f->table)
        return native_master_thread_refuse(L,"PreGame actual constructor did not finish104 genuine fields");
    native_context_pregame_registration_full(L,job);f->callback_returned=true;native_selected_registrar_callback_return_owned(L);return 0;
}
static void native_pregame_cpcall_error_8f7e60_owned(lua_State* L,lua_State* selected,int status){
    auto& p=owner(L);auto*const f=p.pregame_frame;
    if(!f||!f->actual_job||L!=p.state||selected!=f->selected_thread||status==0)
        return (void)native_master_thread_refuse(L,"PreGame actual protected failure/logger operands absent");
    native_context_pregame_registration_full(L,*f->actual_job);
    // Preserve the genuine selected error until original8F8430's late pop.
    // An exact shared-VM stack copy allows allocating/error Lua APIs to stay
    // under the real outer master cpcall; an idle selected thread has no jump
    // boundary after its own cpcall returned. No error/result is fabricated.
    if(!lua_checkstack(L,1))return (void)native_master_thread_refuse(L,"PreGame actual protected error projection stack unavailable");
    lua_pushvalue(selected,-1);lua_xmove(selected,L,1);
    const char*const format=status==LUA_ERRRUN?"Runtime Error: %s":status==LUA_ERRMEM?"Memory Allocation Error: %s":
        status==LUA_ERRERR?"Error Handler Error: %s":"%s";
    const char*const error=lua_tolstring(L,-1,nullptr);native_context_pregame_registration_full(L,*f->actual_job);
    char message[0x400];(void)::sprintf_s(message,sizeof(message),format,error);
    native_context_pregame_registration_full(L,*f->actual_job);
    auto*const boundary=p.native_master_protected_boundary;auto*const critical=p.native_master_lua_critical_section.get();
    if(!boundary||boundary!=f->boundary||!critical||critical!=boundary->critical.get())
        return (void)native_master_thread_refuse(L,"PreGame protected error lost its actual process CS/logger owner");
    // Same real lazy default8F74F0 owner as the existing closed helper. A custom
    // native sink remains an explicit supplier frontier, never ignored.
    if(!critical->default_logger_initialized){critical->actual_default_error_sink=native_master_default_error_sink_8f74f0;critical->default_logger_initialized=true;}
    if(critical->actual_default_error_sink!=native_master_default_error_sink_8f74f0)
        return (void)native_master_thread_refuse(L,"PreGame actual custom error sink requires its genuine owner");
    critical->actual_default_error_sink(message);native_context_pregame_registration_full(L,*f->actual_job);lua_pop(L,1);
}
static void perform_native_context_pregame_registration(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.pregame_frame;native_context_pregame_registration_full(L,job);
    if(!f||f->actual_job||f->returned||f->protected_status!=-1||f->table_ref!=LUA_NOREF||L!=p.state)
        return (void)native_master_thread_refuse(L,"PreGame521E90 actual selected registrar replay/current caller refused");
    f->actual_job=&job;const int top=lua_gettop(L);
    perform_native_context_state_query(L,job);native_context_pregame_registration_full(L,job);
    if(job.context_state_thread!=f->selected_thread||job.context_state_environment!=f->actual_environment||
       lua_topointer(L,-1)!=f->actual_environment||lua_gettop(f->selected_thread)||lua_status(f->selected_thread))
        return (void)native_master_thread_refuse(L,"PreGame521E90 offered thread is not the genuine idle selected518/fenv");
    lua_settop(L,top);
    auto*const boundary=p.native_master_protected_boundary;native_master_protected_boundary_current(L,boundary);
    if(!boundary||&boundary->actual_job!=&job||boundary->held_locks==UINT64_MAX)
        return (void)native_master_thread_refuse(L,"PreGame actual protected Job/process lock history absent");
    f->boundary=boundary;
    ::EnterCriticalSection(&boundary->critical->actual);++boundary->held_locks;
    // Exact protected8F8430 call: selected coroutine, real751450 body, userdata0.
    native_selected_registrar_begin_owned(L,job,f,f->selected_thread,native_pregame_register_751450_owned);
    native_context_pregame_registration_full(L,job);
    const int status=lua_cpcall(f->selected_thread,native_pregame_register_751450_owned,nullptr);
    f->protected_status=status;native_selected_registrar_after_cpcall_owned(L,status);
    if(status!=0){
        native_pregame_cpcall_error_8f7e60_owned(L,f->selected_thread,status);
        lua_settop(f->selected_thread,-2); // actual error pop before lock release
    }
    ::LeaveCriticalSection(&boundary->critical->actual);--boundary->held_locks;
    native_selected_registrar_finish_owned(L);
    // The stack boundary belongs to this protected call only. Later namespace
    // suppliers must validate the retained return without dereferencing it.
    f->boundary=nullptr;
    job.context_pregame_status=status;
    if(status!=0)return (void)luaL_error(L,"PreGame actual protected751450 returned status%d; no completed namespace granted",status);
    native_context_pregame_registration_full(L,job);
    if(!f->callback_returned||!f->global_published||f->fields_created!=104||f->table_ref<0||!f->table||
       lua_gettop(f->selected_thread)||lua_status(f->selected_thread))
        return (void)native_master_thread_refuse(L,"PreGame actual protected constructor return/selected stack changed");
    job.context_pregame_reference=f->table_ref;job.context_pregame_table=f->table;job.context_pregame_fields=f->fields_created;
    native_context_pregame_registration_full(L,job);f->returned=true;
    p.source_exception={};p.source_error=nullptr;
    try{p.native_pregame_namespace_returns.push_back(std::unique_ptr<NativePreGameNamespaceReturn>(new NativePreGameNamespaceReturn(p,*f)));}
    catch(...){p.source_exception=std::current_exception();p.source_error="PreGame genuine namespace normal-return retention refused";}
    if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
}
void native_pregame_registration_retire()noexcept{
    pregame_frame=nullptr;for(const auto& binding:native_pregame_backend_bindings)if(binding)binding->retired=true;
    for(const auto& result:native_pregame_namespace_returns)if(result)result->retired=true;
}
