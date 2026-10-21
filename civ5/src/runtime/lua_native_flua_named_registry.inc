// Inside Host::Impl after the genuine FLua registration scope and CodeBuddy
// table owner. Original VAs are identity keys; no original executable is called.
// The known-body lookup is supplied by the canonical Source28/Root composition:
// native_flua_exposure_existing_primary_callback_owned(L, originalVA).
// It is used only by an invoked consumer, never by either registrar.
template<std::uint32_t Original> static int native_flua_exposure_primary_endpoint_owned(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    lua_State*const vm=p.state;global_State*const global=G(L);
    const ScriptSourceProvider*const source=&p.sources;
    lua_CFunction callback=nullptr;p.source_exception={};p.source_error=nullptr;
    try{
        p.worldview_require_callback_origin();
        if(!vm||p.state!=vm||G(vm)!=global||source!=&p.sources||std::this_thread::get_id()!=p.ui_thread||p.failed.exhausted_)
            throw ScriptError("FLua invoked primary lost actual Host/VM/Source");
        callback=native_flua_exposure_existing_primary_callback_owned(L,Original);
        if(!callback||callback==&Impl::native_flua_exposure_primary_endpoint_owned<Original>)
            throw ScriptError("FLua invoked primary requires its genuine canonical backend body");
        if(p.state!=vm||G(L)!=global||epoch!=p.failure_epoch||writes!=p.failed.writes_||
           uncertainty!=p.execution_uncertainty||p.failed.exhausted_)
            throw ScriptError("FLua invoked primary Source changed before actual callback");
    }catch(...){p.source_exception=std::current_exception();p.source_error="FLua invoked primary backend FIRST refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s (%08X)",p.source_error,static_cast<unsigned>(Original));}
    // Only a real canonical CFunction is invoked. No owning C++ local crosses
    // its Lua jump boundary; its native argument/result/backend effects remain.
    const int count=callback(L);native_db_query_current(L);
    p.source_exception={};p.source_error=nullptr;
    try{
        p.worldview_require_callback_origin();
        if(p.state!=vm||G(L)!=global||G(vm)!=global||source!=&p.sources||
           epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty||
           p.failed.exhausted_||native_flua_exposure_existing_primary_callback_owned(L,Original)!=callback)
            throw ScriptError("FLua actual invoked callback/VM/Source changed at POST");
    }catch(...){p.source_exception=std::current_exception();p.source_error="FLua invoked primary backend POST refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    if(count<0||count>lua_gettop(L)){p.record_failure();return luaL_error(L,"FLua actual primary returned invalid result count");}
    return count;
}
static lua_CFunction native_flua_exposure_primary_body_owned(lua_State* L,std::uint32_t original){
    native_flua_exposure_registration_full(L);
    switch(original){
        case 0x00401e00u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00401e00u>;
        case 0x004075c0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004075c0u>;
        case 0x00407e50u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00407e50u>;
        case 0x00408d20u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00408d20u>;
        case 0x0040c4e0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0040c4e0u>;
        case 0x0040fe00u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0040fe00u>;
        case 0x00410610u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00410610u>;
        case 0x00411750u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00411750u>;
        case 0x00416520u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00416520u>;
        case 0x004179b0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004179b0u>;
        case 0x00418fa0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00418fa0u>;
        case 0x00421820u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00421820u>;
        case 0x00422990u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00422990u>;
        case 0x0042c550u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0042c550u>;
        case 0x0042c5b0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0042c5b0u>;
        case 0x0042e980u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0042e980u>;
        case 0x004348c0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004348c0u>;
        case 0x00434d00u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00434d00u>;
        case 0x0043a2a0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0043a2a0u>;
        case 0x0043be90u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0043be90u>;
        case 0x0043cbf0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0043cbf0u>;
        case 0x0043e0c0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0043e0c0u>;
        case 0x00453350u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00453350u>;
        case 0x00454ee0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00454ee0u>;
        case 0x00460cf0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00460cf0u>;
        case 0x00461450u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00461450u>;
        case 0x00462230u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00462230u>;
        case 0x00462de0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00462de0u>;
        case 0x00465530u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00465530u>;
        case 0x00468230u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00468230u>;
        case 0x0046e440u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0046e440u>;
        case 0x0046fe00u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0046fe00u>;
        case 0x00478140u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00478140u>;
        case 0x0047ab10u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0047ab10u>;
        case 0x0047c4c0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0047c4c0u>;
        case 0x0047e630u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0047e630u>;
        case 0x00484c00u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00484c00u>;
        case 0x00488050u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00488050u>;
        case 0x0048ab80u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0048ab80u>;
        case 0x0048d290u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0048d290u>;
        case 0x0048e070u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0048e070u>;
        case 0x0048e920u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0048e920u>;
        case 0x0048edf0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0048edf0u>;
        case 0x0049c390u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0049c390u>;
        case 0x0049edd0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0049edd0u>;
        case 0x0049f370u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0049f370u>;
        case 0x004a3120u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004a3120u>;
        case 0x004a8c60u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004a8c60u>;
        case 0x004aa340u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004aa340u>;
        case 0x004aa3a0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004aa3a0u>;
        case 0x004abef0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004abef0u>;
        case 0x004b1ef0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004b1ef0u>;
        case 0x004b2460u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004b2460u>;
        case 0x004b8ed0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004b8ed0u>;
        case 0x004b9f40u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004b9f40u>;
        case 0x004bad50u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004bad50u>;
        case 0x004c4bd0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004c4bd0u>;
        case 0x004c5410u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004c5410u>;
        case 0x004c84b0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004c84b0u>;
        case 0x004ca870u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004ca870u>;
        case 0x004cb2e0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004cb2e0u>;
        case 0x004cc0f0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004cc0f0u>;
        case 0x004da590u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004da590u>;
        case 0x004dd890u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004dd890u>;
        case 0x004e1580u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004e1580u>;
        case 0x004e4eb0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004e4eb0u>;
        case 0x004e67f0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004e67f0u>;
        case 0x004e7960u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004e7960u>;
        case 0x004e82a0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004e82a0u>;
        case 0x004e85b0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004e85b0u>;
        case 0x004ea870u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004ea870u>;
        case 0x004eb900u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004eb900u>;
        case 0x004f1c00u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004f1c00u>;
        case 0x004f4570u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004f4570u>;
        case 0x004f4880u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004f4880u>;
        case 0x004f6950u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004f6950u>;
        case 0x004f6990u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004f6990u>;
        case 0x004f7570u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004f7570u>;
        case 0x004f78b0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004f78b0u>;
        case 0x004f98b0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004f98b0u>;
        case 0x004fe870u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x004fe870u>;
        case 0x0050a1c0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0050a1c0u>;
        case 0x00512b80u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00512b80u>;
        case 0x00515f90u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00515f90u>;
        case 0x00517480u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00517480u>;
        case 0x005180c0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005180c0u>;
        case 0x00518730u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00518730u>;
        case 0x00518910u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00518910u>;
        case 0x0051a6a0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0051a6a0u>;
        case 0x0051b6b0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0051b6b0u>;
        case 0x0051f110u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0051f110u>;
        case 0x0052c5d0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0052c5d0u>;
        case 0x0052e200u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0052e200u>;
        case 0x00531cf0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00531cf0u>;
        case 0x00535cb0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00535cb0u>;
        case 0x005364d0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005364d0u>;
        case 0x00536fa0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00536fa0u>;
        case 0x00537c60u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00537c60u>;
        case 0x0053fd90u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0053fd90u>;
        case 0x00543fa0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00543fa0u>;
        case 0x00544440u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00544440u>;
        case 0x00546bb0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00546bb0u>;
        case 0x00547b70u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00547b70u>;
        case 0x00549cd0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00549cd0u>;
        case 0x0054e070u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0054e070u>;
        case 0x0054f950u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0054f950u>;
        case 0x00552f50u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00552f50u>;
        case 0x00554820u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00554820u>;
        case 0x0055dd60u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0055dd60u>;
        case 0x0055ea40u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0055ea40u>;
        case 0x00562920u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00562920u>;
        case 0x00563640u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00563640u>;
        case 0x00563730u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00563730u>;
        case 0x00564ef0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00564ef0u>;
        case 0x0056ff90u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0056ff90u>;
        case 0x00574440u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00574440u>;
        case 0x00576370u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00576370u>;
        case 0x00576c90u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00576c90u>;
        case 0x0057a330u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0057a330u>;
        case 0x00580510u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00580510u>;
        case 0x00582810u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00582810u>;
        case 0x00585080u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00585080u>;
        case 0x00587880u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00587880u>;
        case 0x00587ba0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00587ba0u>;
        case 0x0058e600u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0058e600u>;
        case 0x00593d90u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00593d90u>;
        case 0x005a1500u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005a1500u>;
        case 0x005a22c0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005a22c0u>;
        case 0x005a2570u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005a2570u>;
        case 0x005a9450u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005a9450u>;
        case 0x005aacf0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005aacf0u>;
        case 0x005ab320u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005ab320u>;
        case 0x005ac300u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005ac300u>;
        case 0x005ac660u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005ac660u>;
        case 0x005acf00u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005acf00u>;
        case 0x005ad470u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005ad470u>;
        case 0x005adee0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005adee0u>;
        case 0x005aeb60u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005aeb60u>;
        case 0x005b2e60u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005b2e60u>;
        case 0x005b84b0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005b84b0u>;
        case 0x005b8510u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005b8510u>;
        case 0x005bc760u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005bc760u>;
        case 0x005bd8d0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005bd8d0u>;
        case 0x005be450u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005be450u>;
        case 0x005bf0b0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005bf0b0u>;
        case 0x005c0890u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005c0890u>;
        case 0x005c08c0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005c08c0u>;
        case 0x005c0f80u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005c0f80u>;
        case 0x005daf90u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005daf90u>;
        case 0x005db1d0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005db1d0u>;
        case 0x005db430u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005db430u>;
        case 0x005de480u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005de480u>;
        case 0x005de650u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005de650u>;
        case 0x005de770u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005de770u>;
        case 0x005e2aa0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005e2aa0u>;
        case 0x005ee250u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005ee250u>;
        case 0x005f7d50u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005f7d50u>;
        case 0x005f8490u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x005f8490u>;
        case 0x00600630u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00600630u>;
        case 0x00604740u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00604740u>;
        case 0x00606dc0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00606dc0u>;
        case 0x00608650u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00608650u>;
        case 0x00609f00u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00609f00u>;
        case 0x0060f8f0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0060f8f0u>;
        case 0x0060f9b0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0060f9b0u>;
        case 0x0060fad0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0060fad0u>;
        case 0x00610250u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00610250u>;
        case 0x0061cf90u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0061cf90u>;
        case 0x0061ff20u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0061ff20u>;
        case 0x00621990u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00621990u>;
        case 0x00624b60u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00624b60u>;
        case 0x006286a0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006286a0u>;
        case 0x0062af80u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0062af80u>;
        case 0x00632be0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00632be0u>;
        case 0x00633ac0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00633ac0u>;
        case 0x00633b60u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00633b60u>;
        case 0x00637df0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00637df0u>;
        case 0x00638130u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00638130u>;
        case 0x00639480u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00639480u>;
        case 0x0063b9f0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0063b9f0u>;
        case 0x0063be00u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0063be00u>;
        case 0x0063c190u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0063c190u>;
        case 0x006400c0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006400c0u>;
        case 0x00641130u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00641130u>;
        case 0x00641700u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00641700u>;
        case 0x00641ca0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00641ca0u>;
        case 0x00645a30u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00645a30u>;
        case 0x0064a050u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0064a050u>;
        case 0x0064c740u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0064c740u>;
        case 0x006518c0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006518c0u>;
        case 0x00657a80u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00657a80u>;
        case 0x0065a560u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0065a560u>;
        case 0x0065be90u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0065be90u>;
        case 0x0065f2e0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0065f2e0u>;
        case 0x00661630u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00661630u>;
        case 0x006617a0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006617a0u>;
        case 0x00669850u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00669850u>;
        case 0x00670530u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00670530u>;
        case 0x00670cc0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00670cc0u>;
        case 0x00672170u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00672170u>;
        case 0x006739c0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006739c0u>;
        case 0x00673c20u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00673c20u>;
        case 0x00676030u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00676030u>;
        case 0x00676090u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00676090u>;
        case 0x00676430u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00676430u>;
        case 0x0067a770u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0067a770u>;
        case 0x0067ae40u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0067ae40u>;
        case 0x0067bd00u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0067bd00u>;
        case 0x0067e230u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0067e230u>;
        case 0x0067ff90u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0067ff90u>;
        case 0x00682380u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00682380u>;
        case 0x00682c00u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00682c00u>;
        case 0x00687be0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00687be0u>;
        case 0x00688850u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00688850u>;
        case 0x0068a630u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0068a630u>;
        case 0x00691950u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00691950u>;
        case 0x00693f60u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00693f60u>;
        case 0x00696a70u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00696a70u>;
        case 0x006a0700u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006a0700u>;
        case 0x006a45a0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006a45a0u>;
        case 0x006a4a20u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006a4a20u>;
        case 0x006ad460u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006ad460u>;
        case 0x006ad4c0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006ad4c0u>;
        case 0x006aeae0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006aeae0u>;
        case 0x006b0a80u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006b0a80u>;
        case 0x006b12e0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006b12e0u>;
        case 0x006b4b50u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006b4b50u>;
        case 0x006b6360u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006b6360u>;
        case 0x006bb2e0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006bb2e0u>;
        case 0x006bd4f0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006bd4f0u>;
        case 0x006be590u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006be590u>;
        case 0x006c00c0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006c00c0u>;
        case 0x006c3930u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006c3930u>;
        case 0x006c44b0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006c44b0u>;
        case 0x006c5170u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006c5170u>;
        case 0x006cae30u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006cae30u>;
        case 0x006cba70u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006cba70u>;
        case 0x006d0020u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006d0020u>;
        case 0x006d0530u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006d0530u>;
        case 0x006d2400u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006d2400u>;
        case 0x006d2930u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006d2930u>;
        case 0x006d5970u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006d5970u>;
        case 0x006d8390u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006d8390u>;
        case 0x006dadd0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006dadd0u>;
        case 0x006e13a0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006e13a0u>;
        case 0x006ebf50u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006ebf50u>;
        case 0x006eee90u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006eee90u>;
        case 0x006f3570u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006f3570u>;
        case 0x006f57b0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006f57b0u>;
        case 0x006f5e50u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006f5e50u>;
        case 0x006fbb50u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x006fbb50u>;
        case 0x00701390u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00701390u>;
        case 0x00701a70u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00701a70u>;
        case 0x0070b930u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0070b930u>;
        case 0x0070bc50u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0070bc50u>;
        case 0x0070c030u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0070c030u>;
        case 0x0070c100u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0070c100u>;
        case 0x0070d250u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0070d250u>;
        case 0x0070dea0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0070dea0u>;
        case 0x00710800u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00710800u>;
        case 0x00712c20u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00712c20u>;
        case 0x00718ad0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00718ad0u>;
        case 0x00719730u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00719730u>;
        case 0x0071d710u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0071d710u>;
        case 0x00720150u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00720150u>;
        case 0x007228a0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x007228a0u>;
        case 0x00722f50u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00722f50u>;
        case 0x00723910u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00723910u>;
        case 0x007266c0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x007266c0u>;
        case 0x0072a560u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0072a560u>;
        case 0x0072dd30u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0072dd30u>;
        case 0x0072fb70u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0072fb70u>;
        case 0x00736170u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00736170u>;
        case 0x007376e0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x007376e0u>;
        case 0x0073c150u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0073c150u>;
        case 0x0074f6e0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0074f6e0u>;
        case 0x0074ffa0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0074ffa0u>;
        case 0x007562b0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x007562b0u>;
        case 0x00758290u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00758290u>;
        case 0x0075d140u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0075d140u>;
        case 0x0075d480u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0075d480u>;
        case 0x0075d5c0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0075d5c0u>;
        case 0x0075eb20u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0075eb20u>;
        case 0x00760730u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00760730u>;
        case 0x00764ac0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00764ac0u>;
        case 0x00766e70u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00766e70u>;
        case 0x00766e90u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00766e90u>;
        case 0x0076ac60u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0076ac60u>;
        case 0x0076b270u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0076b270u>;
        case 0x0076b410u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0076b410u>;
        case 0x0076fd40u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0076fd40u>;
        case 0x00770260u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00770260u>;
        case 0x00771540u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00771540u>;
        case 0x00773b30u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00773b30u>;
        case 0x007752c0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x007752c0u>;
        case 0x007760d0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x007760d0u>;
        case 0x00777170u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00777170u>;
        case 0x0077c580u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0077c580u>;
        case 0x0077c610u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0077c610u>;
        case 0x0077c630u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0077c630u>;
        case 0x0077c690u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0077c690u>;
        case 0x0077d240u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0077d240u>;
        case 0x0077e750u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0077e750u>;
        case 0x00781780u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00781780u>;
        case 0x007885b0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x007885b0u>;
        case 0x00789730u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00789730u>;
        case 0x0078abb0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0078abb0u>;
        case 0x0078e340u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0078e340u>;
        case 0x0078f870u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0078f870u>;
        case 0x00790f70u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x00790f70u>;
        case 0x007950b0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x007950b0u>;
        case 0x0079ca90u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0079ca90u>;
        case 0x0079cca0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x0079cca0u>;
        case 0x007a54b0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x007a54b0u>;
        case 0x007a7980u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x007a7980u>;
        case 0x007a82f0u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x007a82f0u>;
        case 0x007b1550u:return &Impl::native_flua_exposure_primary_endpoint_owned<0x007b1550u>;
    }
    return (native_master_thread_refuse(L,"FLua primary identity absent from qualified named/builtin producers"),nullptr);
}
// Complete qualified secure-CRT signature callbacks. Each has one genuine
// lua_pushstring result and preserves the original 0x100-byte scratch buffer.
static int native_flua_named_signature_005b3fd0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","SetGameViewRenderType","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_007667e0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","int","","GetGameViewRenderType");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00597760_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","PublishRecordCommandStreams","bool","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0066d8f0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","TogglePauseGameplay","bool","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_004552a0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","FOW_SetAll","uint","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00773fd0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","Water_SetAutoReload","bool","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_004e6240_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","Water_SetAmplitude","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00626700_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","Water_SetGradientScale","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_004452a0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","Water_SetSpeed","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_004992f0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s, %s%s, %s%s )","void","","Water_SetFlowParameters","int","","int","","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0069af50_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","Water_SetScale","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_004b43e0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s, %s%s )","void","","Water_SetSpec","int","","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_005b6670_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","void","","Ice_Reload");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00765f60_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","void","","Waves_Reload");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_006fd2a0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s, %s%s )","bool","","UnitMoving","int","","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_005ceea0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","void","","ToggleStrategicView");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0067e780_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","bool","","InStrategicView");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00660030_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","bool","","ProcessStrategicViewMouseClick");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_005278c0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","bool","","MouseOverStrategicViewResource");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_005e72c0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"CFunction: %s","GetStrategicViewOverlays");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_005e4ef0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","SetStrategicViewOverlay","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00770d90_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"CFunction: %s","GetOverlayLegend");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0043cfa0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"CFunction: %s","GetStrategicViewIconSettings");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_004d9200_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","SetStrategicViewIconSetting","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0070b960_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","StrategicViewShowFeatures","bool","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0074cd20_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","StrategicViewShowFogOfWar","bool","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00687020_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s, %s%s )","uint","","GetUnitSimHandle","int","","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_005966d0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s, %s%s, %s%s )","void","","TunerSetUnitState","int","","int","","const char","*");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_006d7630_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"CFunction: %s","ToGridFromHex");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00637770_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","FGXInt2","","ToHexFromGrid","FGXInt2","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_006fc410_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"CFunction: %s","GridToWorld");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00488bb0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","FGXVector4","","HexToWorld","HEXVec","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0044a5c0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s, %s%s )","FGXVector4","","HexVertexToWorld","HEXVec","","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_006bd460_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s, %s%s )","void","","ReloadTextures","int","","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0064c0c0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","bool","","SystemBuildYield");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0075bf80_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","bool","","IsOnLiveActive");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0073b7d0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","bool","","IsSteamActive");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00461b60_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"CFunction: %s","LookUpControl");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_006f9780_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","bool","","IsGameCoreBusy");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_004d5a20_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","setEndTurnTimerLength","float","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_006377d0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","float","","getEndTurnTimerLength");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0065c130_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","void","","ResetAchievements");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0052ed50_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s, %s%s, %s%s )","void","","SetUnitActionCodeDebug","int","","int","","float","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_004b9e20_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","const char","*","GetSoundScapeInfo","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_005d62c0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","const char","*","GetSoundScapeQuadrantInfo","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00636300_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s, %s%s )","const char","*","GetSoundInstanceInfo","int","","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00443570_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","const char","*","GetSoundListenerInfo");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0043e0d0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","const char","*","Get3DSoundInstanceChannelInfo","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0050d2e0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","const char","*","GetCurCivSong");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_005fe580_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","void","","SaveAudioOptions");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0057faa0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","void","","LoadAudioOptions");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0047c210_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","float","","GetVolumeKnobValue","int","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_005d06b0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s, %s%s )","void","","SetVolumeKnobValue","int","","float","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_004dba70_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","int","","GetVolumeKnobIDFromName","const char","*");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00435bf0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","float","","GetSoundRolloffFactor");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0064c310_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","SetSoundRolloffFactor","float","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00550840_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","float","","GetSpeakerFalloffFactor");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00740e10_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","SetSpeakerFalloffFactor","float","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00651fa0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","void","","BeginAudioLogging");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0076da00_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","void","","EndAudioLogging");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_006a6850_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","table","","GetAudioLog");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0045ac90_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","bool","","SaveAudioLog","const char","*");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_004d3180_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","MemTrackerEnable","bool","");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00763b00_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","MemTrackerIncrementMarker","const char","*");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00748bc0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","MemTrackerDump","const char","*");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_0067b0c0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","void","","MemTrackerDisplayPools");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_006ea130_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s, %s%s )","void","","MemTrackerDumpPools","const char","*","const char","*");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static int native_flua_named_signature_00570ce0_owned(lua_State* L){
    native_flua_exposure_registration_full(L);char buffer[0x100];
    (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","MemTrackerDumpLeaderheadStats","const char","*");
    native_flua_exposure_registration_full(L);lua_pushstring(L,buffer);
    native_flua_exposure_registration_full(L);return 1;
}
static lua_CFunction native_flua_named_secondary_body_owned(lua_State* L,std::uint32_t original){
    native_flua_exposure_registration_full(L);
    switch(original){
        case 0x005b3fd0u:return &Impl::native_flua_named_signature_005b3fd0_owned;
        case 0x007667e0u:return &Impl::native_flua_named_signature_007667e0_owned;
        case 0x00597760u:return &Impl::native_flua_named_signature_00597760_owned;
        case 0x0066d8f0u:return &Impl::native_flua_named_signature_0066d8f0_owned;
        case 0x004552a0u:return &Impl::native_flua_named_signature_004552a0_owned;
        case 0x00773fd0u:return &Impl::native_flua_named_signature_00773fd0_owned;
        case 0x004e6240u:return &Impl::native_flua_named_signature_004e6240_owned;
        case 0x00626700u:return &Impl::native_flua_named_signature_00626700_owned;
        case 0x004452a0u:return &Impl::native_flua_named_signature_004452a0_owned;
        case 0x004992f0u:return &Impl::native_flua_named_signature_004992f0_owned;
        case 0x0069af50u:return &Impl::native_flua_named_signature_0069af50_owned;
        case 0x004b43e0u:return &Impl::native_flua_named_signature_004b43e0_owned;
        case 0x005b6670u:return &Impl::native_flua_named_signature_005b6670_owned;
        case 0x00765f60u:return &Impl::native_flua_named_signature_00765f60_owned;
        case 0x006fd2a0u:return &Impl::native_flua_named_signature_006fd2a0_owned;
        case 0x005ceea0u:return &Impl::native_flua_named_signature_005ceea0_owned;
        case 0x0067e780u:return &Impl::native_flua_named_signature_0067e780_owned;
        case 0x00660030u:return &Impl::native_flua_named_signature_00660030_owned;
        case 0x005278c0u:return &Impl::native_flua_named_signature_005278c0_owned;
        case 0x005e72c0u:return &Impl::native_flua_named_signature_005e72c0_owned;
        case 0x005e4ef0u:return &Impl::native_flua_named_signature_005e4ef0_owned;
        case 0x00770d90u:return &Impl::native_flua_named_signature_00770d90_owned;
        case 0x0043cfa0u:return &Impl::native_flua_named_signature_0043cfa0_owned;
        case 0x004d9200u:return &Impl::native_flua_named_signature_004d9200_owned;
        case 0x0070b960u:return &Impl::native_flua_named_signature_0070b960_owned;
        case 0x0074cd20u:return &Impl::native_flua_named_signature_0074cd20_owned;
        case 0x00687020u:return &Impl::native_flua_named_signature_00687020_owned;
        case 0x005966d0u:return &Impl::native_flua_named_signature_005966d0_owned;
        case 0x006d7630u:return &Impl::native_flua_named_signature_006d7630_owned;
        case 0x00637770u:return &Impl::native_flua_named_signature_00637770_owned;
        case 0x006fc410u:return &Impl::native_flua_named_signature_006fc410_owned;
        case 0x00488bb0u:return &Impl::native_flua_named_signature_00488bb0_owned;
        case 0x0044a5c0u:return &Impl::native_flua_named_signature_0044a5c0_owned;
        case 0x006bd460u:return &Impl::native_flua_named_signature_006bd460_owned;
        case 0x0064c0c0u:return &Impl::native_flua_named_signature_0064c0c0_owned;
        case 0x0075bf80u:return &Impl::native_flua_named_signature_0075bf80_owned;
        case 0x0073b7d0u:return &Impl::native_flua_named_signature_0073b7d0_owned;
        case 0x00461b60u:return &Impl::native_flua_named_signature_00461b60_owned;
        case 0x006f9780u:return &Impl::native_flua_named_signature_006f9780_owned;
        case 0x004d5a20u:return &Impl::native_flua_named_signature_004d5a20_owned;
        case 0x006377d0u:return &Impl::native_flua_named_signature_006377d0_owned;
        case 0x0065c130u:return &Impl::native_flua_named_signature_0065c130_owned;
        case 0x0052ed50u:return &Impl::native_flua_named_signature_0052ed50_owned;
        case 0x004b9e20u:return &Impl::native_flua_named_signature_004b9e20_owned;
        case 0x005d62c0u:return &Impl::native_flua_named_signature_005d62c0_owned;
        case 0x00636300u:return &Impl::native_flua_named_signature_00636300_owned;
        case 0x00443570u:return &Impl::native_flua_named_signature_00443570_owned;
        case 0x0043e0d0u:return &Impl::native_flua_named_signature_0043e0d0_owned;
        case 0x0050d2e0u:return &Impl::native_flua_named_signature_0050d2e0_owned;
        case 0x005fe580u:return &Impl::native_flua_named_signature_005fe580_owned;
        case 0x0057faa0u:return &Impl::native_flua_named_signature_0057faa0_owned;
        case 0x0047c210u:return &Impl::native_flua_named_signature_0047c210_owned;
        case 0x005d06b0u:return &Impl::native_flua_named_signature_005d06b0_owned;
        case 0x004dba70u:return &Impl::native_flua_named_signature_004dba70_owned;
        case 0x00435bf0u:return &Impl::native_flua_named_signature_00435bf0_owned;
        case 0x0064c310u:return &Impl::native_flua_named_signature_0064c310_owned;
        case 0x00550840u:return &Impl::native_flua_named_signature_00550840_owned;
        case 0x00740e10u:return &Impl::native_flua_named_signature_00740e10_owned;
        case 0x00651fa0u:return &Impl::native_flua_named_signature_00651fa0_owned;
        case 0x0076da00u:return &Impl::native_flua_named_signature_0076da00_owned;
        case 0x006a6850u:return &Impl::native_flua_named_signature_006a6850_owned;
        case 0x0045ac90u:return &Impl::native_flua_named_signature_0045ac90_owned;
        case 0x004d3180u:return &Impl::native_flua_named_signature_004d3180_owned;
        case 0x00763b00u:return &Impl::native_flua_named_signature_00763b00_owned;
        case 0x00748bc0u:return &Impl::native_flua_named_signature_00748bc0_owned;
        case 0x0067b0c0u:return &Impl::native_flua_named_signature_0067b0c0_owned;
        case 0x006ea130u:return &Impl::native_flua_named_signature_006ea130_owned;
        case 0x00570ce0u:return &Impl::native_flua_named_signature_00570ce0_owned;
    }
    return (native_master_thread_refuse(L,"FLua named secondary lacks its complete qualified signature body"),nullptr);
}
// Complete original 8F7E20: preserve the offered primary on the stack and
// associate the zero-upvalue secondary in the genuine 8F7D40 weak-key table.
static void native_flua_associate_codebuddy_8f7e20_owned(lua_State* L,lua_CFunction secondary){
    native_flua_exposure_registration_full(L);
    if(!secondary)return (void)native_master_thread_refuse(L,"FLua CodeBuddy secondary body absent");
    native_ordinary_push_codebuddy_table_8f7d40_owned(L);native_flua_exposure_registration_full(L);
    (void)lua_checkstack(L,2);native_flua_exposure_registration_full(L);
    lua_pushvalue(L,-2);native_flua_exposure_registration_full(L);
    lua_pushcclosure(L,secondary,0);native_flua_exposure_registration_full(L);
    lua_settable(L,-3);native_flua_exposure_registration_full(L);
    lua_settop(L,-2);native_flua_exposure_registration_full(L);
}
class NativeFLuaNamedRegistryOwner final {
    friend struct Impl;
    struct Record {const char* name=nullptr;std::uint32_t flags=0,primary_va=0,secondary_va=0;
        lua_CFunction primary=nullptr,secondary=nullptr;};
    using Callback=void(*)(lua_State*);
    Impl*const host;const ScriptSourceProvider*const source;const std::thread::id creator;
    const std::uint64_t epoch,writes,uncertainty;
    enum class Phase {fresh_image,vm_bound,retired};Phase phase=Phase::fresh_image;
    lua_State* master=nullptr;global_State* actual_global=nullptr;const Table* lua_registry=nullptr;
    std::uint32_t guard=0,count=0,callback_count=0;
    std::array<Record,1024> records{};std::array<Callback,1024> callbacks{};
    std::uint64_t actual_constructor_writes=0,actual_readers=0,lazy_initializers=0;
    explicit NativeFLuaNamedRegistryOwner(Impl& p):host(&p),source(&p.sources),creator(std::this_thread::get_id()),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){}
    NativeFLuaNamedRegistryOwner(const NativeFLuaNamedRegistryOwner&)=delete;
    NativeFLuaNamedRegistryOwner& operator=(const NativeFLuaNamedRegistryOwner&)=delete;
    void physical_current_without_callbacks()const {
        if(host->native_flua_named_registry.get()!=this||source!=&host->sources||phase!=Phase::vm_bound||
           creator!=std::this_thread::get_id()||creator!=host->ui_thread||epoch!=host->failure_epoch||
           writes!=host->failed.writes_||uncertainty!=host->execution_uncertainty||host->failed.exhausted_||
           !master||host->state!=master||!actual_global||G(master)!=actual_global||
           !ttistable(&actual_global->l_registry)||hvalue(&actual_global->l_registry)!=lua_registry||
           count>records.size()||callback_count>callbacks.size())
            throw ScriptError("Native8F9300 actual named registry/Source/VM/count retired");
    }
public:~NativeFLuaNamedRegistryOwner()=default;
};
// Exact selected constructor order from real CRT pointer cells. These native
// function pointers are our source bodies; they are never x86 address casts.
static constexpr NativeFLuaNamedRegistryOwner::Record native_flua_named_constructor_records[]={
    {"SetGameViewRenderType",0xffffffffu,0x005b84b0u,0x005b3fd0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x005b84b0u>,&Impl::native_flua_named_signature_005b3fd0_owned}, // CRT 009c9f14, ctor 0x9b8560
    {"GetGameViewRenderType",0xffffffffu,0x00546bb0u,0x007667e0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00546bb0u>,&Impl::native_flua_named_signature_007667e0_owned}, // CRT 009c9f18, ctor 0x9b85c0
    {"PublishRecordCommandStreams",0xffffffffu,0x004f1c00u,0x00597760u,&Impl::native_flua_exposure_primary_endpoint_owned<0x004f1c00u>,&Impl::native_flua_named_signature_00597760_owned}, // CRT 009c9f1c, ctor 0x9b8620
    {"TogglePauseGameplay",0xffffffffu,0x0079cca0u,0x0066d8f0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0079cca0u>,&Impl::native_flua_named_signature_0066d8f0_owned}, // CRT 009c9f20, ctor 0x9b8680
    {"FOW_SetAll",0xffffffffu,0x00610250u,0x004552a0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00610250u>,&Impl::native_flua_named_signature_004552a0_owned}, // CRT 009c9f24, ctor 0x9b86e0
    {"Water_SetAutoReload",0xffffffffu,0x0063b9f0u,0x00773fd0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0063b9f0u>,&Impl::native_flua_named_signature_00773fd0_owned}, // CRT 009c9f28, ctor 0x9b8740
    {"Water_SetAmplitude",0xffffffffu,0x005ee250u,0x004e6240u,&Impl::native_flua_exposure_primary_endpoint_owned<0x005ee250u>,&Impl::native_flua_named_signature_004e6240_owned}, // CRT 009c9f2c, ctor 0x9b87a0
    {"Water_SetGradientScale",0xffffffffu,0x004e82a0u,0x00626700u,&Impl::native_flua_exposure_primary_endpoint_owned<0x004e82a0u>,&Impl::native_flua_named_signature_00626700_owned}, // CRT 009c9f30, ctor 0x9b8800
    {"Water_SetSpeed",0xffffffffu,0x0054e070u,0x004452a0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0054e070u>,&Impl::native_flua_named_signature_004452a0_owned}, // CRT 009c9f34, ctor 0x9b8860
    {"Water_SetFlowParameters",0xffffffffu,0x00465530u,0x004992f0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00465530u>,&Impl::native_flua_named_signature_004992f0_owned}, // CRT 009c9f38, ctor 0x9b88c0
    {"Water_SetScale",0xffffffffu,0x0077d240u,0x0069af50u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0077d240u>,&Impl::native_flua_named_signature_0069af50_owned}, // CRT 009c9f3c, ctor 0x9b8920
    {"Water_SetSpec",0xffffffffu,0x00701a70u,0x004b43e0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00701a70u>,&Impl::native_flua_named_signature_004b43e0_owned}, // CRT 009c9f40, ctor 0x9b8980
    {"Ice_Reload",0xffffffffu,0x006aeae0u,0x005b6670u,&Impl::native_flua_exposure_primary_endpoint_owned<0x006aeae0u>,&Impl::native_flua_named_signature_005b6670_owned}, // CRT 009c9f44, ctor 0x9b89e0
    {"Waves_Reload",0xffffffffu,0x00621990u,0x00765f60u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00621990u>,&Impl::native_flua_named_signature_00765f60_owned}, // CRT 009c9f48, ctor 0x9b8a40
    {"UnitMoving",0xffffffffu,0x0053fd90u,0x006fd2a0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0053fd90u>,&Impl::native_flua_named_signature_006fd2a0_owned}, // CRT 009ca004, ctor 0x9b93f0
    {"ToggleStrategicView",0xffffffffu,0x004da590u,0x005ceea0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x004da590u>,&Impl::native_flua_named_signature_005ceea0_owned}, // CRT 009ca1ac, ctor 0x9b9df0
    {"InStrategicView",0xffffffffu,0x00641700u,0x0067e780u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00641700u>,&Impl::native_flua_named_signature_0067e780_owned}, // CRT 009ca1b0, ctor 0x9b9e50
    {"ProcessStrategicViewMouseClick",0xffffffffu,0x006e13a0u,0x00660030u,&Impl::native_flua_exposure_primary_endpoint_owned<0x006e13a0u>,&Impl::native_flua_named_signature_00660030_owned}, // CRT 009ca1b4, ctor 0x9b9eb0
    {"MouseOverStrategicViewResource",0xffffffffu,0x0074f6e0u,0x005278c0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0074f6e0u>,&Impl::native_flua_named_signature_005278c0_owned}, // CRT 009ca1b8, ctor 0x9b9f10
    {"GetStrategicViewOverlays",0xffffffffu,0x00536fa0u,0x005e72c0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00536fa0u>,&Impl::native_flua_named_signature_005e72c0_owned}, // CRT 009ca1bc, ctor 0x9b9f70
    {"SetStrategicViewOverlay",0xffffffffu,0x00673c20u,0x005e4ef0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00673c20u>,&Impl::native_flua_named_signature_005e4ef0_owned}, // CRT 009ca1c0, ctor 0x9b9fd0
    {"GetOverlayLegend",0xffffffffu,0x0043e0c0u,0x00770d90u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0043e0c0u>,&Impl::native_flua_named_signature_00770d90_owned}, // CRT 009ca1c4, ctor 0x9ba030
    {"GetStrategicViewIconSettings",0xffffffffu,0x00401e00u,0x0043cfa0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00401e00u>,&Impl::native_flua_named_signature_0043cfa0_owned}, // CRT 009ca1c8, ctor 0x9ba090
    {"SetStrategicViewIconSetting",0xffffffffu,0x006c3930u,0x004d9200u,&Impl::native_flua_exposure_primary_endpoint_owned<0x006c3930u>,&Impl::native_flua_named_signature_004d9200_owned}, // CRT 009ca1cc, ctor 0x9ba0f0
    {"StrategicViewShowFeatures",0xffffffffu,0x006d0020u,0x0070b960u,&Impl::native_flua_exposure_primary_endpoint_owned<0x006d0020u>,&Impl::native_flua_named_signature_0070b960_owned}, // CRT 009ca1d0, ctor 0x9ba150
    {"StrategicViewShowFogOfWar",0xffffffffu,0x004e7960u,0x0074cd20u,&Impl::native_flua_exposure_primary_endpoint_owned<0x004e7960u>,&Impl::native_flua_named_signature_0074cd20_owned}, // CRT 009ca1d4, ctor 0x9ba1b0
    {"GetUnitSimHandle",0xffffffffu,0x004f78b0u,0x00687020u,&Impl::native_flua_exposure_primary_endpoint_owned<0x004f78b0u>,&Impl::native_flua_named_signature_00687020_owned}, // CRT 009ca220, ctor 0x9ba590
    {"TunerSetUnitState",0xffffffffu,0x0043be90u,0x005966d0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0043be90u>,&Impl::native_flua_named_signature_005966d0_owned}, // CRT 009ca224, ctor 0x9ba5f0
    {"ToGridFromHex",0xffffffffu,0x0070c100u,0x006d7630u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0070c100u>,&Impl::native_flua_named_signature_006d7630_owned}, // CRT 009ca2a0, ctor 0x9baac0
    {"ToHexFromGrid",0xffffffffu,0x00462de0u,0x00637770u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00462de0u>,&Impl::native_flua_named_signature_00637770_owned}, // CRT 009ca2a4, ctor 0x9bab20
    {"GridToWorld",0xffffffffu,0x004aa3a0u,0x006fc410u,&Impl::native_flua_exposure_primary_endpoint_owned<0x004aa3a0u>,&Impl::native_flua_named_signature_006fc410_owned}, // CRT 009ca2a8, ctor 0x9bab80
    {"HexToWorld",0xffffffffu,0x006cae30u,0x00488bb0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x006cae30u>,&Impl::native_flua_named_signature_00488bb0_owned}, // CRT 009ca2ac, ctor 0x9babe0
    {"HexVertexToWorld",0xffffffffu,0x007752c0u,0x0044a5c0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x007752c0u>,&Impl::native_flua_named_signature_0044a5c0_owned}, // CRT 009ca2b0, ctor 0x9bac40
    {"ReloadTextures",0xffffffffu,0x004e1580u,0x006bd460u,&Impl::native_flua_exposure_primary_endpoint_owned<0x004e1580u>,&Impl::native_flua_named_signature_006bd460_owned}, // CRT 009ca2b4, ctor 0x9baca0
    {"SystemBuildYield",0xffffffffu,0x0046e440u,0x0064c0c0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0046e440u>,&Impl::native_flua_named_signature_0064c0c0_owned}, // CRT 009ca2b8, ctor 0x9bad00
    {"IsOnLiveActive",0xffffffffu,0x0072a560u,0x0075bf80u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0072a560u>,&Impl::native_flua_named_signature_0075bf80_owned}, // CRT 009ca2bc, ctor 0x9bad60
    {"IsSteamActive",0xffffffffu,0x00587ba0u,0x0073b7d0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00587ba0u>,&Impl::native_flua_named_signature_0073b7d0_owned}, // CRT 009ca2c0, ctor 0x9badc0
    {"LookUpControl",0xffffffffu,0x005a2570u,0x00461b60u,&Impl::native_flua_exposure_primary_endpoint_owned<0x005a2570u>,&Impl::native_flua_named_signature_00461b60_owned}, // CRT 009ca354, ctor 0x9bb6d0
    {"IsGameCoreBusy",0xffffffffu,0x0076b270u,0x006f9780u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0076b270u>,&Impl::native_flua_named_signature_006f9780_owned}, // CRT 009ca85c, ctor 0x9c1910
    {"setEndTurnTimerLength",0xffffffffu,0x005e2aa0u,0x004d5a20u,&Impl::native_flua_exposure_primary_endpoint_owned<0x005e2aa0u>,&Impl::native_flua_named_signature_004d5a20_owned}, // CRT 009ca860, ctor 0x9c1970
    {"getEndTurnTimerLength",0xffffffffu,0x0075d5c0u,0x006377d0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0075d5c0u>,&Impl::native_flua_named_signature_006377d0_owned}, // CRT 009ca864, ctor 0x9c19d0
    {"ResetAchievements",0xffffffffu,0x0070d250u,0x0065c130u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0070d250u>,&Impl::native_flua_named_signature_0065c130_owned}, // CRT 009ca874, ctor 0x9c1b10
    {"SetUnitActionCodeDebug",0xffffffffu,0x00576370u,0x0052ed50u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00576370u>,&Impl::native_flua_named_signature_0052ed50_owned}, // CRT 009ca880, ctor 0x9c1be0
    {"GetSoundScapeInfo",0xffffffffu,0x00462230u,0x004b9e20u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00462230u>,&Impl::native_flua_named_signature_004b9e20_owned}, // CRT 009ca884, ctor 0x9c1c40
    {"GetSoundScapeQuadrantInfo",0xffffffffu,0x00606dc0u,0x005d62c0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00606dc0u>,&Impl::native_flua_named_signature_005d62c0_owned}, // CRT 009ca888, ctor 0x9c1ca0
    {"GetSoundInstanceInfo",0xffffffffu,0x005aacf0u,0x00636300u,&Impl::native_flua_exposure_primary_endpoint_owned<0x005aacf0u>,&Impl::native_flua_named_signature_00636300_owned}, // CRT 009ca88c, ctor 0x9c1d00
    {"GetSoundListenerInfo",0xffffffffu,0x00543fa0u,0x00443570u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00543fa0u>,&Impl::native_flua_named_signature_00443570_owned}, // CRT 009ca890, ctor 0x9c1d60
    {"Get3DSoundInstanceChannelInfo",0xffffffffu,0x0052c5d0u,0x0043e0d0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0052c5d0u>,&Impl::native_flua_named_signature_0043e0d0_owned}, // CRT 009ca894, ctor 0x9c1dc0
    {"GetCurCivSong",0xffffffffu,0x00633ac0u,0x0050d2e0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00633ac0u>,&Impl::native_flua_named_signature_0050d2e0_owned}, // CRT 009ca8ac, ctor 0x9c1e20
    {"SaveAudioOptions",0xffffffffu,0x004f98b0u,0x005fe580u,&Impl::native_flua_exposure_primary_endpoint_owned<0x004f98b0u>,&Impl::native_flua_named_signature_005fe580_owned}, // CRT 009ca8b8, ctor 0x9c1f10
    {"LoadAudioOptions",0xffffffffu,0x0079ca90u,0x0057faa0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0079ca90u>,&Impl::native_flua_named_signature_0057faa0_owned}, // CRT 009ca8bc, ctor 0x9c1f70
    {"GetVolumeKnobValue",0xffffffffu,0x005b8510u,0x0047c210u,&Impl::native_flua_exposure_primary_endpoint_owned<0x005b8510u>,&Impl::native_flua_named_signature_0047c210_owned}, // CRT 009ca8c0, ctor 0x9c1fd0
    {"SetVolumeKnobValue",0xffffffffu,0x00563730u,0x005d06b0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00563730u>,&Impl::native_flua_named_signature_005d06b0_owned}, // CRT 009ca8c4, ctor 0x9c2030
    {"GetVolumeKnobIDFromName",0xffffffffu,0x00691950u,0x004dba70u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00691950u>,&Impl::native_flua_named_signature_004dba70_owned}, // CRT 009ca8c8, ctor 0x9c2090
    {"GetSoundRolloffFactor",0xffffffffu,0x00478140u,0x00435bf0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00478140u>,&Impl::native_flua_named_signature_00435bf0_owned}, // CRT 009ca8cc, ctor 0x9c20f0
    {"SetSoundRolloffFactor",0xffffffffu,0x0064c740u,0x0064c310u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0064c740u>,&Impl::native_flua_named_signature_0064c310_owned}, // CRT 009ca8d0, ctor 0x9c2150
    {"GetSpeakerFalloffFactor",0xffffffffu,0x0048e070u,0x00550840u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0048e070u>,&Impl::native_flua_named_signature_00550840_owned}, // CRT 009ca8d4, ctor 0x9c21b0
    {"SetSpeakerFalloffFactor",0xffffffffu,0x0077c630u,0x00740e10u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0077c630u>,&Impl::native_flua_named_signature_00740e10_owned}, // CRT 009ca8d8, ctor 0x9c2210
    {"BeginAudioLogging",0xffffffffu,0x0078e340u,0x00651fa0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0078e340u>,&Impl::native_flua_named_signature_00651fa0_owned}, // CRT 009ca8dc, ctor 0x9c2270
    {"EndAudioLogging",0xffffffffu,0x00770260u,0x0076da00u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00770260u>,&Impl::native_flua_named_signature_0076da00_owned}, // CRT 009ca8e0, ctor 0x9c22d0
    {"GetAudioLog",0xffffffffu,0x006286a0u,0x006a6850u,&Impl::native_flua_exposure_primary_endpoint_owned<0x006286a0u>,&Impl::native_flua_named_signature_006a6850_owned}, // CRT 009ca8e4, ctor 0x9c2330
    {"SaveAudioLog",0xffffffffu,0x0051b6b0u,0x0045ac90u,&Impl::native_flua_exposure_primary_endpoint_owned<0x0051b6b0u>,&Impl::native_flua_named_signature_0045ac90_owned}, // CRT 009ca8e8, ctor 0x9c2390
    {"MemTrackerEnable",0xffffffffu,0x005de650u,0x004d3180u,&Impl::native_flua_exposure_primary_endpoint_owned<0x005de650u>,&Impl::native_flua_named_signature_004d3180_owned}, // CRT 009ca8f4, ctor 0x9c2450
    {"MemTrackerIncrementMarker",0xffffffffu,0x005daf90u,0x00763b00u,&Impl::native_flua_exposure_primary_endpoint_owned<0x005daf90u>,&Impl::native_flua_named_signature_00763b00_owned}, // CRT 009ca8f8, ctor 0x9c24b0
    {"MemTrackerDump",0xffffffffu,0x00781780u,0x00748bc0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00781780u>,&Impl::native_flua_named_signature_00748bc0_owned}, // CRT 009ca8fc, ctor 0x9c2510
    {"MemTrackerDisplayPools",0xffffffffu,0x006f57b0u,0x0067b0c0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x006f57b0u>,&Impl::native_flua_named_signature_0067b0c0_owned}, // CRT 009ca900, ctor 0x9c2570
    {"MemTrackerDumpPools",0xffffffffu,0x004348c0u,0x006ea130u,&Impl::native_flua_exposure_primary_endpoint_owned<0x004348c0u>,&Impl::native_flua_named_signature_006ea130_owned}, // CRT 009ca904, ctor 0x9c25d0
    {"MemTrackerDumpLeaderheadStats",0xffffffffu,0x00760730u,0x00570ce0u,&Impl::native_flua_exposure_primary_endpoint_owned<0x00760730u>,&Impl::native_flua_named_signature_00570ce0_owned}, // CRT 009ca908, ctor 0x9c2630
};
void initialize_native_flua_named_registry_before_vm_owned(){
    if(native_flua_named_registry||state||calling||std::this_thread::get_id()!=ui_thread||failed.exhausted_)
        throw ScriptError("Native named constructors require the actual first pre-VM session");
    native_flua_named_registry.reset(new NativeFLuaNamedRegistryOwner(*this));
    auto& actual=*native_flua_named_registry;
    for(const auto& row:native_flua_named_constructor_records){
        std::uint32_t ordinal=0;
        if((actual.guard&1u)==0){actual.guard|=1u;actual.callback_count=0;}
        else ordinal=actual.count;
        if(ordinal>=actual.records.size())continue; // authentic original capacity return
        actual.count=ordinal+1;actual.records[ordinal]=row;++actual.actual_constructor_writes;
    }
}
void bind_native_flua_named_registry_actual_vm_owned(){
    auto*const actual=native_flua_named_registry.get();
    if(!actual||actual->host!=this||actual->source!=&sources||actual->phase!=NativeFLuaNamedRegistryOwner::Phase::fresh_image||
       !state||calling||std::this_thread::get_id()!=ui_thread||actual->epoch!=failure_epoch||
       actual->writes!=failed.writes_||actual->uncertainty!=execution_uncertainty||failed.exhausted_||
       actual->actual_constructor_writes!=std::size(native_flua_named_constructor_records)||!ttistable(&G(state)->l_registry))
        throw ScriptError("Native named registry requires its actual CRT constructors and newly created master VM");
    actual->master=state;actual->actual_global=G(state);actual->lua_registry=hvalue(&G(state)->l_registry);
    actual->phase=NativeFLuaNamedRegistryOwner::Phase::vm_bound;actual->physical_current_without_callbacks();
}
static NativeFLuaNamedRegistryOwner* native_flua_named_registry_current_owned(lua_State* L){
    auto& p=owner(L);auto*const actual=p.native_flua_named_registry.get();
    if(!native_flua_exposure_metadata_current_if_entered(L))
        return (native_master_thread_refuse(L,"Native8F9300 requires a genuine entered exposure registrar"),nullptr);
    if(p.native_flua_named_registry.get()!=actual)
        return (native_master_thread_refuse(L,"Native8F9300 owning registry changed during Source FULL"),nullptr);
    auto*const scope=p.native_flua_exposure_registration.get();
    if(!scope||scope->stage!=NativeFLuaExposureRegistrationOwner::Stage::named||scope->descriptor_depth)
        return (native_master_thread_refuse(L,"Native8F9300 escaped actual named-stage callback ordering"),nullptr);
    p.source_exception={};p.source_error=nullptr;
    try{if(!actual||G(L)!=G(p.state))throw ScriptError("Native8F9300 actual named registry/VM absent");
        actual->physical_current_without_callbacks();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Native8F9300 named registry FIRST refused";}
    if(p.source_error)return (native_master_thread_refuse(L,p.source_error),nullptr);
    return actual;
}
static void native_flua_named_registry_8f9300_owned(lua_State* L){
    auto& p=owner(L);auto*const actual=native_flua_named_registry_current_owned(L);
    if(actual->actual_readers==UINT64_MAX)return (void)native_master_thread_refuse(L,"Native8F9300 actual reader history exhausted");
    ++actual->actual_readers;
    if((actual->guard&1u)==0){actual->guard|=1u;actual->count=0;actual->callback_count=0;++actual->lazy_initializers;}
    std::uint32_t ordinal=0;
    while(ordinal<actual->count){
        if(ordinal>=actual->records.size())return (void)native_master_thread_refuse(L,"Native8F9300 real current row count overflow");
        const auto primary=actual->records[ordinal].primary,secondary=actual->records[ordinal].secondary;
        const char*const name=actual->records[ordinal].name;
        if(!primary||!secondary||!name)return (void)native_master_thread_refuse(L,"Native8F9300 live row lost authentic primary/secondary/name");
        p.native_flua_exposure_registration->actual_callback=0x8f9300u;
        p.native_flua_exposure_registration->actual_ordinal=ordinal;
        lua_pushcclosure(L,primary,0);if(native_flua_named_registry_current_owned(L)!=actual)
            return (void)native_master_thread_refuse(L,"Native8F9300 actual row custody changed after primary closure");
        native_flua_associate_codebuddy_8f7e20_owned(L,secondary);if(native_flua_named_registry_current_owned(L)!=actual)
            return (void)native_master_thread_refuse(L,"Native8F9300 actual row custody changed after CodeBuddy association");
        lua_setfield(L,LUA_GLOBALSINDEX,name);if(native_flua_named_registry_current_owned(L)!=actual)
            return (void)native_master_thread_refuse(L,"Native8F9300 actual row custody changed after genuine global store");
        ++ordinal; // next condition rereads the actual mutable named count
    }
    ordinal=0;
    while(ordinal<actual->callback_count){
        if(ordinal>=actual->callbacks.size())return (void)native_master_thread_refuse(L,"Native8F9300 actual callback-array count overflow");
        const auto callback=actual->callbacks[ordinal];
        if(!callback)return (void)native_master_thread_refuse(L,"Native8F9300 live callback lacks its genuine writer");
        p.source_exception={};p.source_error=nullptr;
        try{callback(L);}catch(...){p.source_exception=std::current_exception();p.source_error="Native8F9300 authentic registrar callback threw";}
        if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
        if(native_flua_named_registry_current_owned(L)!=actual)
            return (void)native_master_thread_refuse(L,"Native8F9300 callback changed actual registry custody");
        ++ordinal; // authentic fresh image has callback_count0, no writer found
    }
    (void)native_flua_named_registry_current_owned(L);
}
void native_flua_named_registry_retire()noexcept{
    if(native_flua_named_registry){native_flua_named_registry->phase=NativeFLuaNamedRegistryOwner::Phase::retired;
        native_flua_named_registry->master=nullptr;native_flua_named_registry->actual_global=nullptr;native_flua_named_registry->lua_registry=nullptr;}
}
