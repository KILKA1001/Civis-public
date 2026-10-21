namespace civ5::runtime {
void LuaScriptHost::register_context_initial_lua_graph_owned(const BnwWorldViewSourceOperationLoan& loan,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>& prepared,
    const std::shared_ptr<ui::BnwWorldViewControlTree>& tree)const {
    const auto p=impl_;
    try{
        if(!tree||!prepared||!prepared->impl_)throw ScriptError("Context original graph factory needs actual retained Tree/state preparation");
        // The genuine preparation carries opaque physical Core/cursor identity.
        // Core's representation belongs to the UI translation unit, not Host.
        const auto& saved=*prepared->impl_;
        const auto core=saved.core;
        require_context_lua_state_owned(loan,prepared,core,saved.cursor);
        std::vector<std::int64_t> events;
        {
        Impl::WorldViewNativeHostScope native(*p,*this,loan);p->worldview_require_callback_origin();
        native.child=saved.child;native.child_source=saved.source;
        const auto& child=p->child_contexts[saved.child];
        if(child.loaded||child.loading||child.retired)throw ScriptError("Context native graph factory replay after original Lua load");
        // Source-issued real enum values were acquired by actual bind_sources;
        // this is not a guessed left/right pair or XML capability grant.
        const auto source_owner=p->worldview_sources.lock();if(!source_owner)throw ScriptError("Context actual enum/source owner retired");
        events=source_owner->native_mouse_callback_events();p->worldview_require_callback_origin();
        if(events!=p->worldview_mouse_events||events.size()!=2||events[0]==events[1])
            throw ScriptError("Context actual selected Mouse enum registry changed");
        std::shared_ptr<ScriptUiControlGraph> existing;
        for(const auto& entry:p->control_graphs)if(entry.graph->context()==child.name){
            if(existing)throw ScriptError("Context original native graph registration ambiguous");existing=entry.graph;
        }
        if(existing){
            BnwWorldViewScriptUiAdapter::require_initial_lua_core_owned(existing,core,saved.source,loan,*this);
            p->worldview_require_callback_origin();native.returned=true;return; // actual already issued SAME owner, not replacement
        }
        native.returned=true;
        } // Retire native scope before the Tree's genuine protected state query.
        // No recursive native-scope admission: factory's closed Tree supplier
        // invokes the actual Host query through its own protected scope.
        const auto graph=BnwWorldViewScriptUiAdapter::build_context_initial_lua_owned(tree,saved.source,*this,events,loan);
        require_context_lua_state_owned(loan,prepared,core,saved.cursor);
        Impl::WorldViewNativeHostScope native(*p,*this,loan);p->worldview_require_callback_origin();
        native.child=saved.child;native.child_source=saved.source;
        const auto& child=p->child_contexts[saved.child];
        if(child.loaded||child.loading||child.retired)throw ScriptError("Context native graph source phase changed during factory");
        BnwWorldViewScriptUiAdapter::require_initial_lua_core_owned(graph,core,saved.source,loan,*this);
        p->worldview_require_callback_origin();
        if(p->calling||p->failed||p->executed_contexts.contains(child.name))
            throw ScriptError("Context native graph publication lost actual idle/healthy new source phase");
        p->control_graphs.push_back({graph,false,false,LUA_NOREF});
        p->worldview_require_callback_origin();native.returned=true;
    }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
