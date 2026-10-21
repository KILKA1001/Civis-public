// Inside actual Host::Impl, following the DB cell/registrar definitions.
// These closed helpers implement the existing Database DLL GameInfo port;
// they use the same genuine DB result cells/backend as native Query.
// A backend callback can return after Lua caught a failure or manually retired
// its result. Strong query custody protects memory only. This C++ fence invokes
// no Lua API, so a throw releases retained query/row custody before luaL_error.
static void native_gameinfo_query_callback_post_cpp(Impl& p,NativeDatabaseQueryCell* cell,const ScriptQuery* query,
                                                   std::uint64_t epoch,std::uint64_t writes,std::uint64_t uncertainty){
    p.worldview_require_callback_origin();
    const auto*const scope=p.worldview_native_host;
    if(!scope||scope->child>=p.child_contexts.size())throw ScriptError("GameInfo query SAME selected native source retired");
    const auto child=scope->child;
    const auto source=p.child_contexts[child].source;
    p.child_require_current(child);p.worldview_require_callback_origin();
    if(p.worldview_native_host!=scope||scope->child!=child||child>=p.child_contexts.size()||!source||
       source.get()!=p.child_contexts[child].source.get()||source.owner_before(p.child_contexts[child].source)||
       p.child_contexts[child].source.owner_before(source)||p.child_contexts[child].retired)
        throw ScriptError("GameInfo query selected source replaced during callback POST");
    bool owned=false;for(const auto& actual:p.native_db_query_cells)if(actual.get()==cell){owned=true;break;}
    if(!owned||!cell||cell->retired||!cell->query||cell->query.get()!=query||p.failed||p.failed.exhausted_||
       epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty)
        throw ScriptError("GameInfo query cell/query/source/failure changed during backend callback");
}
static int native_gameinfo_query_operation(lua_State* L){
    static const char*const operations[]={"valid","step","row","id","type","reset","bind","release",nullptr};
    auto& p=owner(L);native_db_query_current(L);const int op=luaL_checkoption(L,1,nullptr,operations);
    if(op==0&&lua_isnil(L,lua_upvalueindex(1))){lua_pushboolean(L,0);return 1;}
    auto*const cell=native_db_cell(L,lua_upvalueindex(1));const auto*const identity=cell->query.get();
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    if(op==0){lua_pushboolean(L,1);return 1;}
    p.source_exception={};p.source_error=nullptr;bool stepped=false;
    if(op==6){
        luaL_checktype(L,2,LUA_TTABLE);const auto count=source_integer(L,3);
        if(count<0){p.record_failure();return luaL_error(L,"Negative query binding count");}
        p.query_arguments.clear();
        for(std::int64_t i=1;i<=count;++i){
            lua_pushinteger(L,i);lua_rawget(L,2);bool bad=false;
            try{
                switch(lua_type(L,-1)){
                case LUA_TNIL:p.query_arguments.emplace_back(std::monostate{});break;
                case LUA_TBOOLEAN:case LUA_TNUMBER:p.query_arguments.emplace_back(source_integer(L,-1));break;
                case LUA_TSTRING:p.query_arguments.emplace_back(std::string(lua_tostring(L,-1)));break;
                default:bad=true;break;
                }
            }catch(...){p.source_exception=std::current_exception();p.source_error="GameInfo binding conversion failed";}
            lua_pop(L,1);
            if(bad){p.query_arguments.clear();p.record_failure();return luaL_error(L,"Invalid GameInfo query parameter");}
            if(p.source_error){p.query_arguments.clear();p.record_failure();return luaL_error(L,"%s",p.source_error);}
        }
        native_db_result_post(L,cell,identity,epoch,writes,uncertainty);
    }
    {
        auto retained=cell->query;
        try{
            switch(op){
            case 1:stepped=retained->step();break;
            case 2:p.query_row=retained->row();validate_row(*p.query_row);break;
            case 3:case 4:p.query_key=retained->cache_key(op==3);validate_value(p.query_key.second);break;
            case 5:retained->reset();break;
            case 6:retained->bind(p.query_arguments);break;
            case 7:retained->release();break;
            }
            p.worldview_require_callback_origin();
        }catch(const std::exception& e){p.source_exception=std::current_exception();p.source_error=e.what();}
        catch(...){p.source_exception=std::current_exception();p.source_error="GameInfo actual query operation failed";}
    }
    p.query_arguments.clear();if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    native_db_result_post(L,cell,identity,epoch,writes,uncertainty);
    if(op==1)lua_pushboolean(L,stepped);
    else if(op==2){push_row(L,*p.query_row);p.query_row={};}
    else if(op==3||op==4){lua_pushboolean(L,p.query_key.first);push(L,p.query_key.second);p.query_key={};}
    native_db_result_post(L,cell,identity,epoch,writes,uncertainty);
    return op==3||op==4?2:op==1||op==2?1:0;
}
static int native_gameinfo_prepare(lua_State* L){
    native_db_query_current(L);const char*const sql=luaL_checkstring(L,1);
    native_db_push_result(L,sql);lua_pushcclosure(L,native_gameinfo_query_operation,1);native_db_query_current(L);return 1;
}
static int native_gameinfo_object(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);luaL_checktype(L,1,LUA_TTABLE);
    const void*const meta=lua_topointer(L,1);
    auto**const actual=static_cast<ScriptGameplayProvider**>(lua_newuserdata(L,sizeof(ScriptGameplayProvider*)));
    *actual=&p.gameplay;native_db_query_current(L);
    if(lua_type(L,1)!=LUA_TTABLE||lua_topointer(L,1)!=meta){p.record_failure();return luaL_error(L,"GameInfo actual table metatable changed");}
    lua_pushvalue(L,1);lua_setmetatable(L,-2);native_db_query_current(L);return 1;
}
static int native_game_defines_index(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);auto*const cell=native_db_cell(L,lua_upvalueindex(1));
    const auto*const identity=cell->query.get();const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    lua_pushnil(L);const char*const name=lua_tolstring(L,2,nullptr);native_db_result_post(L,cell,identity,epoch,writes,uncertainty);
    p.source_exception={};p.source_error=nullptr;p.unit_value={};p.query_arguments.clear();
    {
        auto retained=cell->query;
        try{
            if(name)p.query_arguments.emplace_back(std::string(name));else p.query_arguments.emplace_back(std::monostate{});
            retained->bind(p.query_arguments);
            native_gameinfo_query_callback_post_cpp(p,cell,identity,epoch,writes,uncertainty);
            const bool actual_step=retained->step();
            native_gameinfo_query_callback_post_cpp(p,cell,identity,epoch,writes,uncertainty);
            if(actual_step){
                auto row=retained->row();
                native_gameinfo_query_callback_post_cpp(p,cell,identity,epoch,writes,uncertainty);
                if(row.empty())throw ScriptError("GameDefines actual Value query returned no column");
                p.unit_value=std::move(row.front().second);
                if(std::holds_alternative<std::int64_t>(p.unit_value))
                    p.unit_value=static_cast<std::int64_t>(static_cast<std::int32_t>(static_cast<std::uint32_t>(std::get<std::int64_t>(p.unit_value))));
                validate_value(p.unit_value);
            }
            p.worldview_require_callback_origin();
        }catch(const std::exception& e){p.source_exception=std::current_exception();p.source_error=e.what();}
        catch(...){p.source_exception=std::current_exception();p.source_error="GameDefines actual bind/step/column failed";}
    }
    p.query_arguments.clear();if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    native_db_result_post(L,cell,identity,epoch,writes,uncertainty);
    if(!std::holds_alternative<std::monostate>(p.unit_value)){
        push(L,p.unit_value);p.unit_value={};native_db_result_post(L,cell,identity,epoch,writes,uncertainty);
        // Original40C780 caches the observed non-nil value BEFORE Reset. A
        // Lua cache write error therefore leaves the same live busy statement.
        lua_pushvalue(L,2);lua_pushvalue(L,-2);lua_rawset(L,1);native_db_result_post(L,cell,identity,epoch,writes,uncertainty);
    }
    p.source_exception={};p.source_error=nullptr;
    {
        auto retained=cell->query;
        try{retained->reset();p.worldview_require_callback_origin();}
        catch(...){p.source_exception=std::current_exception();p.source_error="GameDefines actual Reset failed";}
    }
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    native_db_result_post(L,cell,identity,epoch,writes,uncertainty);return 1;
}
