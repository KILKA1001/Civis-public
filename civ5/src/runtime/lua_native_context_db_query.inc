// Inside Host::Impl. Actual725010/6698C0/452720/4D75B0 SQL lifecycle.
// Lua-rooted userdata and separate actual C++ custody prevent backend callbacks
// which manually invoke __gc from destroying the query while it executes.
struct NativeDatabaseQueryCell final {
    std::shared_ptr<ScriptQuery> query;
    const void* userdata=nullptr;const void* metatable=nullptr;
    bool retired=false;
};
struct NativeDatabaseQueryHandle final {Impl* host;std::size_t slot;};
static void native_db_query_current(lua_State* L){
    auto& p=owner(L);p.source_exception={};p.source_error=nullptr;
    try{
        p.worldview_require_callback_origin();
        if(p.worldview_native_host->child>=p.child_contexts.size())throw ScriptError("Native DB selected context retired");
    }catch(...){p.source_exception=std::current_exception();p.source_error="Native DB current selected owner/source refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    child_require_bindings(L);worldview_require_lua_origin(L);
}
static NativeDatabaseQueryCell* native_db_cell(lua_State* L,int index,bool allow_invalid=false){
    auto& p=owner(L);
    if(lua_type(L,index)!=LUA_TUSERDATA||lua_objlen(L,index)!=sizeof(NativeDatabaseQueryHandle)){
        p.record_failure();return (luaL_error(L,"Failed to iterate table due to query error.  Check Database.log for more details."),nullptr);
    }
    const auto*const handle=static_cast<const NativeDatabaseQueryHandle*>(lua_touserdata(L,index));
    if(!handle||handle->host!=&p||handle->slot>=p.native_db_query_cells.size()){
        p.record_failure();return (luaL_error(L,"Native DB result belongs to a different live Host"),nullptr);
    }
    auto*const cell=p.native_db_query_cells[handle->slot].get();
    if(!cell||cell->userdata!=handle||cell->retired||(!allow_invalid&&!cell->query)){
        p.record_failure();return (luaL_error(L,"Failed to iterate table due to query error.  Check Database.log for more details."),nullptr);
    }
    // Raw physical lookup uses the genuinely retained __gc TString; no string
    // allocation or source callback occurs while checking the actual userdata.
    if(p.native_db_registration_keys_ref<0||!lua_getmetatable(L,index)||lua_topointer(L,-1)!=cell->metatable){
        p.record_failure();return (luaL_error(L,"Native DB actual result metatable changed"),nullptr);
    }
    native_db_key(L,8);lua_rawget(L,-2);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_db_query_gc||lua_getupvalue(L,-1,1)){
        p.record_failure();return (luaL_error(L,"Native DB actual result GC consumer changed"),nullptr);
    }
    lua_pop(L,2);
    return cell;
}
static int native_db_query_gc(lua_State* L){
    auto& p=owner(L);
    if(lua_type(L,1)!=LUA_TUSERDATA||lua_objlen(L,1)!=sizeof(NativeDatabaseQueryHandle))return 0;
    const auto*const handle=static_cast<const NativeDatabaseQueryHandle*>(lua_touserdata(L,1));
    if(!handle||handle->host!=&p||handle->slot>=p.native_db_query_cells.size())return 0;
    auto*const cell=p.native_db_query_cells[handle->slot].get();
    if(cell&&cell->userdata==handle){cell->retired=true;cell->query.reset();}
    return 0;
}
static void native_db_result_post(lua_State* L,NativeDatabaseQueryCell* cell,const ScriptQuery* query,
                                  std::uint64_t epoch,std::uint64_t writes,std::uint64_t uncertainty){
    auto& p=owner(L);native_db_query_current(L);
    if(native_db_cell(L,lua_upvalueindex(1))!=cell||cell->query.get()!=query||
       epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty){
        p.record_failure();luaL_error(L,"Native DB result retired or changed during actual backend callback");
    }
}
// A backend callback can return after Lua caught a failure or manually retired
// its result. Strong query custody protects memory only. This C++ fence invokes
// no Lua API, so a throw releases retained query/row custody before luaL_error.
static void native_db_query_callback_post_cpp(Impl& p,NativeDatabaseQueryCell* cell,const ScriptQuery* query,
                                                   std::uint64_t epoch,std::uint64_t writes,std::uint64_t uncertainty){
    p.worldview_require_callback_origin();
    const auto*const scope=p.worldview_native_host;
    if(!scope||scope->child>=p.child_contexts.size())throw ScriptError("Native DB query SAME selected native source retired");
    const auto child=scope->child;
    const auto source=p.child_contexts[child].source;
    p.child_require_current(child);p.worldview_require_callback_origin();
    if(p.worldview_native_host!=scope||scope->child!=child||child>=p.child_contexts.size()||!source||
       source.get()!=p.child_contexts[child].source.get()||source.owner_before(p.child_contexts[child].source)||
       p.child_contexts[child].source.owner_before(source)||p.child_contexts[child].retired)
        throw ScriptError("Native DB query selected source replaced during callback POST");
    bool owned=false;for(const auto& actual:p.native_db_query_cells)if(actual.get()==cell){owned=true;break;}
    if(!owned||!cell||cell->retired||!cell->query||cell->query.get()!=query||p.failed||p.failed.exhausted_||
       epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty)
        throw ScriptError("Native DB query cell/query/source/failure changed during backend callback");
}
static int native_db_query_next(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);auto*const cell=native_db_cell(L,lua_upvalueindex(1));
    const auto*const identity=cell->query.get();
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    p.query_row={};p.source_exception={};p.source_error=nullptr;
    {
        // Custody ends before any Lua API which can longjmp.
        auto retained=cell->query;
        try{
            const bool actual_step=retained->step();
            native_db_query_callback_post_cpp(p,cell,identity,epoch,writes,uncertainty);
            if(actual_step){
                p.query_row=retained->row();
                native_db_query_callback_post_cpp(p,cell,identity,epoch,writes,uncertainty);
                validate_row(*p.query_row);
            }else{
                retained->reset(); // original failure Reset then nil
                native_db_query_callback_post_cpp(p,cell,identity,epoch,writes,uncertainty);
            }
            p.worldview_require_callback_origin();
        }catch(const std::exception& e){p.source_exception=std::current_exception();p.source_error=e.what();}
        catch(...){p.source_exception=std::current_exception();p.source_error="Native DB actual result Step/Reset failed";}
    }
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    native_db_result_post(L,cell,identity,epoch,writes,uncertainty);
    if(!p.query_row){lua_pushnil(L);return 1;}
    push_row(L,*p.query_row);p.query_row={};
    native_db_result_post(L,cell,identity,epoch,writes,uncertainty);return 1;
}
static void native_db_bind_args(lua_State* L,int count,int first){
    auto& p=owner(L);p.query_arguments.clear();
    p.source_exception={};p.source_error=nullptr;int bad=0;
    try{
        for(int i=first;i<=count;++i){
            switch(lua_type(L,i)){
            case LUA_TNIL:p.query_arguments.emplace_back(std::monostate{});break;
            case LUA_TBOOLEAN:p.query_arguments.emplace_back(lua_toboolean(L,i)!=0);break;
            case LUA_TNUMBER:p.query_arguments.emplace_back(static_cast<double>(lua_tonumber(L,i)));break;
            case LUA_TSTRING:p.query_arguments.emplace_back(std::string(lua_tostring(L,i)));break;
            default:bad=i;break;
            }
            if(bad)break;
        }
    }catch(...){p.source_exception=std::current_exception();p.source_error="Native DB binding conversion failed";}
    if(bad){p.query_arguments.clear();p.record_failure();luaL_argerror(L,bad,"Invalid argument for database query.");}
    if(p.source_error){p.query_arguments.clear();p.record_failure();luaL_error(L,"%s",p.source_error);}
}
static void native_db_push_result(lua_State* L,const char* sql){
    auto& p=owner(L);native_db_query_current(L);
    auto*const handle=static_cast<NativeDatabaseQueryHandle*>(lua_newuserdata(L,sizeof(NativeDatabaseQueryHandle)));
    handle->host=&p;handle->slot=std::numeric_limits<std::size_t>::max();
    lua_createtable(L,0,1);const void*const meta=lua_topointer(L,-1);
    native_db_key(L,8);lua_pushcfunction(L,native_db_query_gc);lua_rawset(L,-3);lua_setmetatable(L,-2);
    native_db_query_current(L);const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    p.source_exception={};p.source_error=nullptr;const ScriptQuery* identity=nullptr;
    try{
        if(!p.gameplay.query)throw ScriptError("Native DB requires the real selected rules query backend; provider unissued");
        auto cell=std::make_unique<NativeDatabaseQueryCell>();cell->userdata=handle;cell->metatable=meta;
        const auto slot=p.native_db_query_cells.size();p.native_db_query_cells.push_back(std::move(cell));handle->slot=slot;
        auto query=p.gameplay.query(sql,{});p.worldview_require_callback_origin();
        auto& actual=*p.native_db_query_cells[slot];
        if(actual.retired||epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty)
            throw ScriptError("Native DB preparation changed during actual backend callback");
        actual.query=std::move(query);identity=actual.query.get();
    }catch(const std::exception& e){p.source_exception=std::current_exception();p.source_error=e.what();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Native DB actual result preparation failed";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    native_db_query_current(L);auto*const cell=native_db_cell(L,-1,true);
    if(cell->query.get()!=identity||epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty){
        p.record_failure();luaL_error(L,"Native DB prepared result changed before closure capture");
    }
    // Actual DLL1000B4C0 deletes failed Results and pushes nil. The iterator
    // raises its original error only when it is later invoked.
    if(!identity){lua_pop(L,1);lua_pushnil(L);}
}
static int native_db_bound_query(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);auto*const cell=native_db_cell(L,lua_upvalueindex(1));
    const int count=lua_gettop(L);const auto*const identity=cell->query.get();
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    p.source_exception={};p.source_error=nullptr;
    {
        auto retained=cell->query;
        try{retained->reset();p.worldview_require_callback_origin();}
        catch(...){p.source_exception=std::current_exception();p.source_error="Native DB prepared query Reset failed";}
    }
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    native_db_result_post(L,cell,identity,epoch,writes,uncertainty);
    native_db_bind_args(L,count,1);native_db_result_post(L,cell,identity,epoch,writes,uncertainty);
    p.source_exception={};p.source_error=nullptr;
    {
        auto retained=cell->query;
        try{retained->bind(p.query_arguments);p.worldview_require_callback_origin();}
        catch(...){p.source_exception=std::current_exception();p.source_error="Native DB prepared query Bind failed";}
    }
    p.query_arguments.clear();if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    native_db_result_post(L,cell,identity,epoch,writes,uncertainty);
    lua_pushvalue(L,lua_upvalueindex(1));lua_pushcclosure(L,native_db_query_next,1);
    native_db_result_post(L,cell,identity,epoch,writes,uncertainty);return 1;
}
static int native_db_create_query(lua_State* L){
    native_db_query_current(L);const char*const sql=luaL_checkstring(L,1);
    native_db_push_result(L,sql);lua_pushcclosure(L,native_db_bound_query,1);native_db_query_current(L);return 1;
}
static int native_db_query(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);const int count=lua_gettop(L);const char*const sql=luaL_checkstring(L,1);
    native_db_push_result(L,sql);
    if(!lua_isnil(L,-1)&&count>1){
        auto*const cell=native_db_cell(L,-1);const auto*const identity=cell->query.get();
        const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
        native_db_bind_args(L,count,2);native_db_query_current(L);p.source_exception={};p.source_error=nullptr;
        if(native_db_cell(L,-1)!=cell||cell->query.get()!=identity){p.record_failure();return luaL_error(L,"Native DB Query result changed before Bind");}
        {
            auto retained=cell->query;
            try{retained->bind(p.query_arguments);p.worldview_require_callback_origin();}
            catch(...){p.source_exception=std::current_exception();p.source_error="Native DB Query Bind failed";}
        }
        p.query_arguments.clear();if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
        native_db_query_current(L);
        if(native_db_cell(L,-1)!=cell||cell->query.get()!=identity||epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty){
            p.record_failure();return luaL_error(L,"Native DB Query result changed after Bind");
        }
    }
    native_db_query_current(L);lua_pushcclosure(L,native_db_query_next,1);native_db_query_current(L);return 1;
}
