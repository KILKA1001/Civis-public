// Existing detail facade is the sole public operand. Its real creator return
// is not copied into a parallel Context descriptor or caller boolean token.
namespace civ5::ui::detail {
void NotificationContextPostreadRootCDataReturn::acquire_contextptr_lookup_owned(
    const runtime::BnwWorldViewSourceOperationLoan&op,
    const std::shared_ptr<const NotificationContextUiManagerValueLoan>&ui_value,
    const std::shared_ptr<const NotificationContextTooltipManagerOperand>&tt_operand)const {
    using Host=runtime::LuaScriptHost;using Impl=Host::Impl;
    const auto fields=impl_;const auto p=fields?fields->host.lock():nullptr;
    std::shared_ptr<Impl::NativePostreadLoadTailControl> tail;
    try{
        if(!fields||!p||!fields->facade||fields->creator!=std::this_thread::get_id()||!fields->alive.load()||
            p->ui_thread!=fields->creator||!p->state||p->failed||!p->end_turn_lifetime||
            !p->end_turn_lifetime->alive||p->end_turn_lifetime->failed||
            p->end_turn_lifetime->host!=fields->facade||p->end_turn_lifetime->vm!=p->state)
            throw runtime::ScriptError("postRead ContextPtr original owning facade retired FIRST");
        if(!ui_value||!tt_operand)throw runtime::ScriptError("postRead tail actual typed GUI/embedded TT value operands absent");
        const auto&facade=*fields->facade;
        facade.require_context_postread_root_cdata_owned(op,*this);
        const auto actual=fields->actual;const auto parent=actual->parent;
        const auto canonical=actual->canonical.lock();const auto root=actual->root.lock();
        if(!canonical||canonical.get()!=this||!root||root.get()!=actual->cdata)
            throw runtime::ScriptError("postRead ContextPtr input is not SAME actual root creator return");
        for(const auto&owned:p->postread_load_tail_controls)if(owned&&owned->cdata==actual)
            throw runtime::ScriptError("postRead457571 original reference occurrence cannot replay");
        tail=std::make_shared<Impl::NativePostreadLoadTailControl>(canonical,actual,ui_value,tt_operand);
        p->postread_load_tail_controls.push_back(tail); //custody precedes every allocating Lua call
        facade.require_context_postread_root_cdata_owned(op,*this);
        {
            Impl::WorldViewNativeHostScope native(*p,facade,op);native.child=parent->child;native.child_source=parent->source;
            Impl::NativePostreadLoadTailFrame frame(*p,facade,op,*tail,root);frame.full();
            const std::string context(p->child_contexts[parent->child].name);frame.full();
            Impl::Job job{p.get(),Impl::Operation::native_context_postread_contextptr_acquire,context,{},{},{},{},{}};
            job.worldview_child=parent->child;job.context_postread_load_tail=tail.get();
            frame.actual_job=&job;
            p->call(job);frame.full();
            Impl::Job query{p.get(),Impl::Operation::native_context_postread_contextptr_query,context,{},{},{},{},{}};
            query.worldview_child=parent->child;query.context_postread_load_tail=tail.get();
            frame.actual_job=&query;
            p->call(query);frame.full();
            if(tail->stage!=Impl::NativePostreadLoadTailControl::Stage::prefix_returned||
                tail->protected_status||tail->entry_top||tail->returned_top)
                throw runtime::ScriptError("postRead ContextPtr original6 reference effects did not return");
            native.returned=true;
        }
        require_contextptr_lookup_owned(op);
    }catch(...){
        if(p){
            if(tail)p->native_postread_load_tail_release_after_abort_owned(*tail);
            p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;
        }
        throw;
    }
}
void NotificationContextPostreadRootCDataReturn::require_contextptr_lookup_owned(
    const runtime::BnwWorldViewSourceOperationLoan&op)const {
    using Host=runtime::LuaScriptHost;using Impl=Host::Impl;
    const auto fields=impl_;const auto p=fields?fields->host.lock():nullptr;
    if(!fields||!p||!fields->facade||fields->creator!=std::this_thread::get_id()||!fields->alive.load()||!fields->actual||
        p->ui_thread!=fields->creator||!p->state||p->failed||!p->end_turn_lifetime||
        !p->end_turn_lifetime->alive||p->end_turn_lifetime->failed||
        p->end_turn_lifetime->host!=fields->facade||p->end_turn_lifetime->vm!=p->state)
        throw runtime::ScriptError("postRead ContextPtr owning facade retired before current query");
    const auto&facade=*fields->facade;const auto actual=fields->actual;
    std::shared_ptr<Impl::NativePostreadLoadTailControl> tail;
    for(const auto&owned:p->postread_load_tail_controls)if(owned&&owned->cdata==actual){
        if(tail)throw runtime::ScriptError("postRead ContextPtr actual occurrence duplicated");tail=owned;
    }
    if(!tail||tail->root_return.get()!=this||tail->closed||tail->creator!=std::this_thread::get_id()||
        (tail->stage!=Impl::NativePostreadLoadTailControl::Stage::prefix_returned&&
         tail->stage!=Impl::NativePostreadLoadTailControl::Stage::registrations_returned)||
        tail->entry_top||tail->returned_top||tail->protected_status||p->calling||p->worldview_native_host||
        p->postread_load_tail_frame||p->postread_root_cdata_frame||p->postread_globals_frame)
        throw runtime::ScriptError("postRead ContextPtr6 effects/scopes have not actually returned");
    // The old Source6 named-key view is historical after its normal local10
    // destructor. Requiring it live again would reject this original phase.
    facade.require_context_postread_globals_input_owned(op,tail->globals->input);
    const auto parent=tail->globals;const auto root=actual->root.lock();
    if(!root||root.get()!=actual->cdata)throw runtime::ScriptError("postRead ContextPtr real root receiver retired");
    {
        Impl::WorldViewNativeHostScope native(*p,facade,op);native.child=parent->child;native.child_source=parent->source;
        Impl::NativePostreadLoadTailFrame frame(*p,facade,op,*tail,root);frame.full();
        const std::string context(p->child_contexts[parent->child].name);frame.full();
        Impl::Job query{p.get(),Impl::Operation::native_context_postread_contextptr_query,context,{},{},{},{},{}};
        query.worldview_child=parent->child;query.context_postread_load_tail=tail.get();
        frame.actual_job=&query;
        p->call(query);frame.full();native.returned=true;
    }
    facade.require_context_postread_globals_input_owned(op,tail->globals->input);
}
void NotificationContextPostreadRootCDataReturn::register_postread_environment_owned(
    const runtime::BnwWorldViewSourceOperationLoan&op)const {
    using Host=runtime::LuaScriptHost;using Impl=Host::Impl;
    require_contextptr_lookup_owned(op);
    const auto fields=impl_;const auto p=fields->host.lock();
    if(!p)throw runtime::ScriptError("postRead environment actual Host retired");
    const auto&facade=*fields->facade;
    std::shared_ptr<Impl::NativePostreadLoadTailControl> tail;
    for(const auto&owned:p->postread_load_tail_controls)if(owned&&owned->cdata==fields->actual){
        if(tail)throw runtime::ScriptError("postRead environment actual owner duplicated");tail=owned;
    }
    if(!tail||tail->stage!=Impl::NativePostreadLoadTailControl::Stage::prefix_returned)
        throw runtime::ScriptError("postRead environment original ordered registration cannot replay");
    try{
        const auto parent=tail->globals;const auto root=tail->cdata->root.lock();
        if(!root)throw runtime::ScriptError("postRead environment current root receiver retired");
        {
            Impl::WorldViewNativeHostScope native(*p,facade,op);native.child=parent->child;native.child_source=parent->source;
            Impl::NativePostreadLoadTailFrame frame(*p,facade,op,*tail,root);frame.full();
            const std::string context(p->child_contexts[parent->child].name);frame.full();
            Impl::Job job{p.get(),Impl::Operation::native_context_postread_registrations_acquire,context,{},{},{},{},{}};
            job.worldview_child=parent->child;job.context_postread_load_tail=tail.get();frame.actual_job=&job;
            p->call(job);frame.full();
            if(tail->stage!=Impl::NativePostreadLoadTailControl::Stage::registrations_returned||
                tail->entry_top||tail->returned_top||tail->protected_status)
                throw runtime::ScriptError("postRead real LookUpControl/UIManager/TTManager ordered body did not return");
            native.returned=true;
        }
        require_contextptr_lookup_owned(op);
    }catch(...){
        p->native_postread_load_tail_release_after_abort_owned(*tail);p->record_failure();
        if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;
    }
}
}
