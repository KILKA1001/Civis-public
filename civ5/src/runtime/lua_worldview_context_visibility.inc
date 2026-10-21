// Inside actual LuaScriptHost::Impl, AFTER native/control/context callback
// includes. Registry receivers and C++ lender scopes already belong to Impl.
bool worldview_context_visibility_active=false;
static int worldview_context_hidden_owned(lua_State* L,std::size_t slot,bool ready,bool failed){
    auto& p=owner(L);
    if(lua_gettop(L)!=1||p.worldview_control_entry!=L||slot>=p.control_graphs.size()||
       !p.control_graphs[slot].bound||!p.control_graphs[slot].graph||
       !p.control_graphs[slot].graph->backend_||!lua_rawequal(L,1,lua_upvalueindex(3)))
        return luaL_error(L,"IsHidden requires actual protected native ContextPtr");
    child_require_bindings(L); // retained environment/receiver, no owning locals around Lua error
    bool hidden=false;p.source_exception={};p.source_error=nullptr;
    try {
        p.worldview_require_callback_origin();
        if(p.worldview_context_visibility_active){p.record_failure();throw ScriptError("Context visibility query reentered");}
        p.worldview_context_visibility_active=true;
        struct Exit{bool& active;~Exit(){active=false;}}exit{p.worldview_context_visibility_active};
        const auto frame=p.worldview_callback_frame();
        // First POST is the independent NativeHostScope facade/lifetime check,
        // before touching any saved stack loan, graph, child or owner field.
        p.worldview_require_callback_origin();
        const auto identity=[&]{
            p.worldview_require_callback_origin();
            if(p.worldview_operation!=frame.loan||p.failure_epoch!=frame.epoch||
               p.execution_uncertainty!=frame.uncertainty||slot>=p.control_graphs.size()||
               !p.control_graphs[slot].bound||!p.control_graphs[slot].graph||
               !p.control_graphs[slot].graph->backend_||!p.worldview_native_host||
               !p.worldview_native_child_controls_current(p.worldview_native_host->child))
                throw ScriptError("Context visibility actual graph/child/source occurrence changed");
            const auto child=p.worldview_native_host->child;
            if(child>=p.child_contexts.size()||p.child_contexts[child].retired||
               p.child_contexts[child].name!=p.control_graphs[slot].graph->context())
                throw ScriptError("Context visibility retained native child differs");
        };
        identity();
        p.control_graphs[slot].graph->require_current();p.worldview_require_callback_origin();identity();
        hidden=BnwWorldViewScriptUiAdapter::context_hidden_owned(
            p.control_graphs[slot].graph,*p.worldview_operation,*p.worldview_host_owner);
        // The Adapter lock was pure and is already dropped before this full
        // Source/GUI/window POST; no temporary pin hides outside retirement.
        p.worldview_callback_fence(frame);p.worldview_require_callback_origin();identity();
        if(hidden!=BnwWorldViewScriptUiAdapter::context_hidden_owned(
            p.control_graphs[slot].graph,*p.worldview_operation,*p.worldview_host_owner))
            throw ScriptError("Context visibility history changed during query POST");
        p.worldview_require_callback_origin();identity();
    }catch(const std::exception& e){p.source_exception=std::current_exception();p.source_error=e.what();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Actual Context visibility refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    // All C++ scopes/temporary owners have retired before any Lua API here.
    worldview_require_lua_origin(L);p.ready=ready;
    if(failed)p.failed.set_loader_sentinel();else p.failed=false;
    lua_pushboolean(L,hidden);return 1;
}
