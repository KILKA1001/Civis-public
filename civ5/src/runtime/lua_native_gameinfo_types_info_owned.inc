// Inside actual Host::Impl after native_db_query_current definition.
// SDK CvLuaGameInfo::pRegisterTypesInfo only; Actions must genuinely finish first.
// Reads the existing SAME selected rules/GC catalog callback. No DB table alias.
bool native_gameinfo_types_copy_active=false;
std::vector<std::pair<std::string,std::int64_t>> native_gameinfo_types_entries_owned;
static void native_gameinfo_types_info_current(lua_State* L,std::uint64_t epoch,
    std::uint64_t writes,std::uint64_t uncertainty){
    native_db_query_current(L);auto& p=owner(L);
    if(!p.native_gameinfo_types_copy_active||p.failure_epoch!=epoch||
       p.failed.writes_!=writes||p.execution_uncertainty!=uncertainty){
        p.record_failure();luaL_error(L,"GameInfoTypes SAME actual Source/operation changed");
    }
}
static int native_gameinfo_types_info_owned(lua_State* L){
    native_db_query_current(L);auto& p=owner(L);
    if(p.native_gameinfo_types_copy_active){p.record_failure();return luaL_error(L,"GameInfoTypes actual catalog copy reentry");}
    p.native_gameinfo_types_copy_active=true;
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    p.source_exception={};p.source_error=nullptr;
    try{
        if(!p.gameplay.info_types)throw ScriptError("GameInfoTypes genuine selected GC catalog supplier absent");
        p.native_gameinfo_types_entries_owned=p.gameplay.info_types();
        for(const auto& entry:p.native_gameinfo_types_entries_owned)validate_value(entry.second);
    }catch(...){p.source_exception=std::current_exception();p.source_error="GameInfoTypes genuine selected catalog snapshot failed";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    native_gameinfo_types_info_current(L,epoch,writes,uncertainty);
    lua_newtable(L);native_gameinfo_types_info_current(L,epoch,writes,uncertainty);
    lua_pushvalue(L,-1);lua_setglobal(L,"GameInfoTypes");native_gameinfo_types_info_current(L,epoch,writes,uncertainty);
    for(std::size_t i=0;i<p.native_gameinfo_types_entries_owned.size();++i){
        lua_pushinteger(L,p.native_gameinfo_types_entries_owned[i].second);
        lua_setfield(L,-2,p.native_gameinfo_types_entries_owned[i].first.c_str());
        native_gameinfo_types_info_current(L,epoch,writes,uncertainty);
    }
    p.native_gameinfo_types_entries_owned.clear();p.native_gameinfo_types_copy_active=false;
    return 0; // Original protected C function discards the retained construction table.
}
