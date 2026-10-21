// Actual CvLuaEnums::RegisterDynamicEnums(Missions,ID,Type,NUM_MISSION_TYPES).
// The query is Host-owned across protected Lua calls/error unwinding; no C++
// automatic owning object is abandoned by Lua longjmp.
std::unique_ptr<ScriptQuery> native_gamecore_mission_query;
static std::size_t native_gamecore_register_mission_entries_owned(lua_State*L){
    native_gamecore_export_current_owned(L);auto&p=owner(L);
    if(p.native_gamecore_mission_query||!p.gameplay.query)
        return (luaL_error(L,"GameCore Missions actual selected query supplier unavailable/reentered"),0);
    try{p.native_gamecore_mission_query=p.gameplay.query("select ID, Type from Missions",{});}
    catch(...){p.source_exception=std::current_exception();return (luaL_error(L,"GameCore Missions actual query failed"),0);}
    native_gamecore_export_current_owned(L);
    if(!p.native_gamecore_mission_query)return (luaL_error(L,"GameCore Missions actual query returned no reader"),0);
    std::size_t writes=0;
    for(;;){
        bool found=false;try{found=p.native_gamecore_mission_query->step();}
        catch(...){p.source_exception=std::current_exception();return (luaL_error(L,"GameCore Missions actual Step failed"),0);}
        native_gamecore_export_current_owned(L);if(!found)break;
        // Row strings remain owned by the Host scratch through the Lua writes.
        try{p.query_row=p.native_gamecore_mission_query->row();}
        catch(...){p.source_exception=std::current_exception();return (luaL_error(L,"GameCore Missions actual row failed"),0);}
        native_gamecore_export_current_owned(L);
        if(p.query_row->size()!=2)return (luaL_error(L,"GameCore Missions ID/Type row changed"),0);
        const auto*id=std::get_if<std::int64_t>(&(*p.query_row)[0].second);
        const auto*name=std::get_if<std::string>(&(*p.query_row)[1].second);
        if(!id||*id<INT32_MIN||*id>INT32_MAX||!name||name->find('\0')!=std::string::npos)
            return (luaL_error(L,"GameCore Missions native ID/Type invalid"),0);
        const int integer=static_cast<int>(*id);
        lua_pushlstring(L,name->data(),name->size());lua_pushvalue(L,-1);
        lua_pushinteger(L,integer);lua_rawset(L,-4);++writes;
        lua_rawseti(L,-2,integer);++writes;
        native_gamecore_export_current_owned(L);p.query_row.reset();
    }
    try{p.native_gamecore_mission_query->release();p.native_gamecore_mission_query.reset();
        p.native_gamecore_mission_query=p.gameplay.query("select count(*) from Missions",{});}
    catch(...){p.source_exception=std::current_exception();return (luaL_error(L,"GameCore Missions actual Count query failed"),0);}
    native_gamecore_export_current_owned(L);
    bool found=false;try{found=p.native_gamecore_mission_query&&p.native_gamecore_mission_query->step();if(found)p.query_row=p.native_gamecore_mission_query->row();}
    catch(...){p.source_exception=std::current_exception();return (luaL_error(L,"GameCore Missions actual Count reader failed"),0);}
    native_gamecore_export_current_owned(L);
    if(!found||p.query_row->size()!=1)return (luaL_error(L,"GameCore Missions actual Count absent"),0);
    const auto*count=std::get_if<std::int64_t>(&(*p.query_row)[0].second);
    if(!count||*count<0||*count>INT32_MAX)return (luaL_error(L,"GameCore Missions actual Count outside native int"),0);
    lua_pushstring(L,"NUM_MISSION_TYPES");lua_pushinteger(L,*count);lua_rawset(L,-3);++writes;
    native_gamecore_export_current_owned(L);p.query_row.reset();
    try{p.native_gamecore_mission_query->release();p.native_gamecore_mission_query.reset();}
    catch(...){p.source_exception=std::current_exception();return (luaL_error(L,"GameCore Missions actual reader release failed"),0);}
    native_gamecore_export_current_owned(L);return writes;
}
