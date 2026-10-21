// Inside Host::Impl AFTER native DB query helpers. Original6D9910/42EDC0 only.
// Registry and VM issuance is private Bootstrap->fresh Source, not provider presence.
// Own projection scratch cannot be invalidated by general DB/query_next callbacks.
// A Lua jump leaves the entered latch quarantined; only genuine FULL POST clears it.
std::optional<ScriptRow> native_modding_entrypoint_row;
bool native_modding_entrypoint_entered=false;
static void native_modding_entrypoint_enter_owned(lua_State*L,Impl&p){
    if(std::this_thread::get_id()!=p.ui_thread){luaL_error(L,"Native Modding query foreign Host creator");}
    if(p.native_modding_entrypoint_entered){p.record_failure();luaL_error(L,"Native Modding query reentry quarantined");}
    p.native_modding_entrypoint_entered=true;
}
static void native_modding_entrypoint_query_full_cpp_owned(Impl&p,NativeDatabaseQueryCell*cell,
    const std::shared_ptr<ScriptQuery>&query,std::uint64_t epoch,std::uint64_t writes,std::uint64_t uncertainty,
    bool actual_failed,bool actual_ready){
    p.worldview_require_callback_origin();
    bool owned=false;for(const auto&held:p.native_db_query_cells)if(held.get()==cell){owned=true;break;}
    if(!owned||!cell||cell->retired||!query||cell->query.get()!=query.get()||
        epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty||p.failed.exhausted_||
        static_cast<bool>(p.failed)!=actual_failed||p.ready!=actual_ready||!p.worldview_host_owner)
        throw ScriptError("Native Modding actual query/Host/failure occurrence changed");
    const auto source=p.worldview_sources.lock();
    if(!source)throw ScriptError("Native Modding canonical Source expired");
    const auto&loan=source->current_operation_loan();
    if(&loan!=p.worldview_operation||!p.worldview_native_host||p.worldview_native_host->loan!=&loan||
        p.worldview_native_host->child>=p.child_contexts.size())
        throw ScriptError("Native Modding active actual native Source/child replaced");
    const auto*const scope=p.worldview_native_host;const auto child=scope->child;
    const auto selected=p.child_contexts[child].source;const auto env=p.child_contexts[child].environment;
    const auto thread=p.child_contexts[child].execution_thread;const auto execution=p.child_contexts[child].execution_source;
    if(!selected||p.child_contexts[child].retired||!p.worldview_native_child_controls_current(child))
        throw ScriptError("Native Modding actual selected child retired");
    loan.require_same_vm(*p.worldview_host_owner);loan.require_same_source(*selected);
    loan.require_modding_activated_entrypoint_query_owned(query); // real Source/VM/Bank/registry FULL
    p.child_require_current(child);p.worldview_require_callback_origin();
    if(p.worldview_native_host!=scope||scope->child!=child||p.worldview_operation!=&loan||
        child>=p.child_contexts.size()||p.child_contexts[child].source!=selected||p.child_contexts[child].retired||
        p.child_contexts[child].environment!=env||p.child_contexts[child].execution_thread!=thread||
        p.child_contexts[child].execution_source!=execution||!p.worldview_native_child_controls_current(child)||
        cell->retired||cell->query.get()!=query.get()||epoch!=p.failure_epoch||writes!=p.failed.writes_||
        uncertainty!=p.execution_uncertainty||p.failed.exhausted_||static_cast<bool>(p.failed)!=actual_failed||p.ready!=actual_ready)
        throw ScriptError("Native Modding actual child/query/Source FULL POST changed");
}
static int native_modding_entrypoint_iterator_owned(lua_State*L){
    auto&p=owner(L);native_modding_entrypoint_enter_owned(L,p);native_db_query_current(L);
    if(!p.state||G(L)!=G(p.state)){p.record_failure();return luaL_error(L,"Native Modding iterator belongs to another actual VM");}
    auto*const cell=native_db_cell(L,lua_upvalueindex(1));
    const auto*const identity=cell->query.get();const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    const bool actual_failed=p.failed,actual_ready=p.ready;
    p.native_modding_entrypoint_row={};p.source_exception={};p.source_error=nullptr;
    {
        auto retained=cell->query;
        try{
            native_modding_entrypoint_query_full_cpp_owned(p,cell,retained,epoch,writes,uncertainty,actual_failed,actual_ready);
            const bool stepped=retained->step();
            native_modding_entrypoint_query_full_cpp_owned(p,cell,retained,epoch,writes,uncertainty,actual_failed,actual_ready);
            if(stepped){
                p.native_modding_entrypoint_row=retained->row();
                native_modding_entrypoint_query_full_cpp_owned(p,cell,retained,epoch,writes,uncertainty,actual_failed,actual_ready);
                validate_row(*p.native_modding_entrypoint_row);
            }else{
                retained->reset();
                native_modding_entrypoint_query_full_cpp_owned(p,cell,retained,epoch,writes,uncertainty,actual_failed,actual_ready);
            }
        }catch(const std::exception&e){p.source_exception=std::current_exception();p.source_error=e.what();}
        catch(...){p.source_exception=std::current_exception();p.source_error="Native Modding actual Step/row/Reset failed";}
    }
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    native_db_result_post(L,cell,identity,epoch,writes,uncertainty);
    if(p.native_modding_entrypoint_row)push_row(L,*p.native_modding_entrypoint_row);else lua_pushnil(L);
    native_db_result_post(L,cell,identity,epoch,writes,uncertainty);
    // Allocating row conversion can run GC/caught callbacks. Actual FULL is
    // repeated after projection with strong custody dropped before lua_error.
    p.source_exception={};p.source_error=nullptr;
    {
        auto retained=cell->query;
        try{native_modding_entrypoint_query_full_cpp_owned(p,cell,retained,epoch,writes,uncertainty,actual_failed,actual_ready);}
        catch(...){p.source_exception=std::current_exception();p.source_error="Native Modding row projection Source FULL POST failed";}
    }
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    p.native_modding_entrypoint_row.reset();p.native_modding_entrypoint_entered=false;return 1;
}
static int native_modding_activated_entry_points_owned(lua_State*L){
    auto&p=owner(L);native_modding_entrypoint_enter_owned(L,p);native_db_query_current(L);
    if(!p.state||G(L)!=G(p.state)){p.record_failure();return luaL_error(L,"Native Modding getter belongs to another actual VM");}
    const char*const kind=luaL_checklstring(L,1,nullptr); // Original85B ignores extra args; C-string bind1.
    if(!lua_checkstack(L,12)){p.record_failure();return luaL_error(L,"Native Modding query stack unavailable");}
    auto*const handle=static_cast<NativeDatabaseQueryHandle*>(lua_newuserdata(L,sizeof(NativeDatabaseQueryHandle)));
    handle->host=&p;handle->slot=std::numeric_limits<std::size_t>::max();
    lua_createtable(L,0,1);const void*const meta=lua_topointer(L,-1);
    native_db_key(L,8);lua_pushcfunction(L,native_db_query_gc);lua_rawset(L,-3);lua_setmetatable(L,-2);
    native_db_query_current(L);const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    const bool actual_failed=p.failed,actual_ready=p.ready;const ScriptQuery*identity=nullptr;
    p.source_exception={};p.source_error=nullptr;
    try{
        p.worldview_require_callback_origin();const auto source=p.worldview_sources.lock();
        if(!source)throw ScriptError("Native Modding actual selected Source absent");
        const auto&loan=source->current_operation_loan();
        if(&loan!=p.worldview_operation||!p.worldview_native_host||p.worldview_native_host->loan!=&loan)
            throw ScriptError("Native Modding current Source/Host loan replaced");
        auto cell=std::make_unique<NativeDatabaseQueryCell>();cell->userdata=handle;cell->metatable=meta;
        const auto slot=p.native_db_query_cells.size();p.native_db_query_cells.push_back(std::move(cell));handle->slot=slot;
        auto query=loan.context_modding_activated_entrypoint_query_owned(std::string_view(kind));
        auto&actual=*p.native_db_query_cells[slot];
        if(actual.retired||!query)throw ScriptError("Native Modding actual SQL result retired/unprepared");
        actual.query=std::move(query);identity=actual.query.get();
        const auto retained=actual.query;
        native_modding_entrypoint_query_full_cpp_owned(p,&actual,retained,epoch,writes,uncertainty,actual_failed,actual_ready);
    }catch(const std::exception&e){p.source_exception=std::current_exception();p.source_error=e.what();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Native Modding actual registry SQL preparation failed";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    native_db_query_current(L);auto*const cell=native_db_cell(L,-1);
    if(cell->query.get()!=identity||epoch!=p.failure_epoch||writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty){
        p.record_failure();return luaL_error(L,"Native Modding actual result changed before closure");}
    lua_pushcclosure(L,native_modding_entrypoint_iterator_owned,1);const int closure=lua_gettop(L);native_db_query_current(L);
    // This getter has zero upvalues: authenticate the returned closure's actual
    // sole capture, rather than inspecting a nonexistent getter upvalue.
    if(!lua_iscfunction(L,closure)||lua_tocfunction(L,closure)!=native_modding_entrypoint_iterator_owned||!lua_getupvalue(L,closure,1)){
        p.record_failure();return luaL_error(L,"Native Modding actual iterator closure changed");}
    auto*const captured=native_db_cell(L,-1);lua_pop(L,1);
    if(captured!=cell||lua_getupvalue(L,closure,2)||cell->query.get()!=identity||epoch!=p.failure_epoch||
        writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty){
        p.record_failure();return luaL_error(L,"Native Modding actual closure capture/result changed");}
    // Closure allocation is part of publication: it may run caught callbacks/GC.
    // Keep only POD/references across Lua calls; actual query custody is below.
    p.source_exception={};p.source_error=nullptr;
    {
        const auto retained=cell->query;
        try{native_modding_entrypoint_query_full_cpp_owned(p,cell,retained,epoch,writes,uncertainty,actual_failed,actual_ready);}
        catch(...){p.source_exception=std::current_exception();p.source_error="Native Modding closure publication Source FULL POST failed";}
    }
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    p.native_modding_entrypoint_entered=false;return 1;
}
