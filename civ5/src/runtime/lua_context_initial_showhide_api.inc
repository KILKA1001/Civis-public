namespace civ5::runtime {
void LuaScriptHost::invoke_context_initial_showhide_owned(const BnwWorldViewSourceOperationLoan& loan,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>& prepared,
    const std::weak_ptr<const void>& core,
    const std::shared_ptr<const ui::detail::NotificationContextCursorLoadPrefixReturn>& cursor,
    const std::shared_ptr<const ui::detail::NotificationContextCursorRestoreReturn>& restored,bool hidden)const{
    const auto p=impl_;
    try{
        // This query's scope retires before the callback scope is created.
        require_context_initial_lua_state_owned(loan,prepared,core,cursor);
        if(!restored)throw ScriptError("Initial showhide requires actual post-GC restored cursor");
        Impl::WorldViewNativeHostScope native(*p,*this,loan);p->worldview_require_callback_origin();
        const auto& saved=*prepared->impl_;native.child=saved.child;native.child_source=saved.source;
        const auto& child=p->child_contexts[saved.child];
        if(!child.loader_temporaries||child.loader_temporaries->cleanup_started||child.loader_temporaries->cleanup_returned)
            throw ScriptError("Initial showhide requires real before-Lua temporaries still alive before cleanup");
        std::shared_ptr<ScriptUiControlGraph> graph;
        for(const auto& entry:p->control_graphs)if(entry.graph->context()==child.name){
            if(graph)throw ScriptError("Initial showhide canonical graph ambiguous");graph=entry.graph;}
        if(!graph)throw ScriptError("Initial showhide actual Context backend absent");
        BnwWorldViewScriptUiAdapter::require_initial_lua_core_owned(graph,core,saved.source,loan,*this);
        p->worldview_require_callback_origin();
        if(BnwWorldViewScriptUiAdapter::context_hidden_owned(graph,loan,*this)!=hidden)
            throw ScriptError("Initial showhide actual current hide-bit operand changed");
        p->worldview_require_callback_origin();p->worldview_dispatch_initial_showhide(saved.child,hidden);
        p->worldview_require_callback_origin();
        Impl::Job query{p.get(),Impl::Operation::native_context_initial_state_query,child.name,{},{},{},{},{}};
        query.worldview_child=saved.child;p->call(query);p->worldview_require_callback_origin();
        if(query.context_state_thread!=saved.actual_thread||query.context_state_environment!=saved.actual_environment||
            !p->worldview_initial_showhide_returns.contains(saved.child)||child.loader_temporaries->cleanup_started)
            throw ScriptError("Initial showhide genuine callback/query/lifetime did not return");
        native.returned=true;
    }catch(...){p->worldview_retire_callback_registrations();p->worldview_retire_context_callback_registrations();
        for(auto& e:p->worldview_showhide_callbacks)e.retired=true;
        p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
