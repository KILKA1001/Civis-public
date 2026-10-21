// Outside Host::Impl, AFTER real Preparation and entered Setup APIs.
namespace civ5::ui::detail {
struct NotificationContextPostreadGlobalsReference::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    mutable std::atomic<bool> alive{true};
    const std::weak_ptr<runtime::LuaScriptHost::Impl> host;
    const runtime::LuaScriptHost*const facade;
    const std::shared_ptr<runtime::LuaScriptHost::Impl::NativePostreadGlobalsControl> control;
    Impl(std::weak_ptr<runtime::LuaScriptHost::Impl> h,const runtime::LuaScriptHost*f,
        std::shared_ptr<runtime::LuaScriptHost::Impl::NativePostreadGlobalsControl>r)
        :host(std::move(h)),facade(f),control(std::move(r)){}
};
NotificationContextPostreadGlobalsReference::NotificationContextPostreadGlobalsReference(std::shared_ptr<const Impl>p):impl_(std::move(p)){}
NotificationContextPostreadGlobalsReference::~NotificationContextPostreadGlobalsReference(){
    // A public view destructor is not the original normal local1C destructor
    // callsite. Invalidate even on foreign thread, never touch a stale Lua VM.
    impl_->alive.store(false);
    // Actual refs remain in Host-owned quarantine and are released by the real
    // separately required original normal caller, abort cleanup, or protected
    // Host-close path. No normal caller completion is issued here.
}
}
namespace civ5::runtime {
void LuaScriptHost::require_context_postread_globals_input_without_callbacks(
    const BnwWorldViewSourceOperationLoan&op,
    const std::shared_ptr<const ui::detail::NotificationContextPostreadGlobalsInputLoan>&input)const {
    const auto p=impl_;
    if(!input||input->creator_!=std::this_thread::get_id()||input->core_.expired()||!input->prepared_||
        !input->first_||!input->full_)throw ScriptError("postRead Globals actual Tree-issued input absent FIRST");
    input->first_(); // actual Source operation origin before using saved op
    const auto saved=input->prepared_->impl_;
    if(!saved||saved->creator!=std::this_thread::get_id()||saved->facade!=this||saved->host.expired()||
        saved->host.owner_before(p)||p.owner_before(saved->host)||saved->core.expired()||
        saved->core.owner_before(input->core_)||input->core_.owner_before(saved->core)||
        saved->child>=p->child_contexts.size()||!saved->actual_thread||!saved->actual_environment||
        saved->actual_thread==p->state||p->ui_thread!=saved->creator||!p->state||p->failed||
        p->end_turn_native_active||!p->end_turn_lifetime||!p->end_turn_lifetime->alive||p->end_turn_lifetime->failed||
        p->end_turn_lifetime->host!=this||p->end_turn_lifetime->vm!=p->state||
        saved->serial!=op.operation_serial()||saved->epoch!=p->failure_epoch||saved->writes!=p->failed.writes_||
        saved->uncertainty!=p->execution_uncertainty)
        throw ScriptError("postRead Globals SAME actual518/Core/Host/Source/failure epoch changed");
    const auto&c=p->child_contexts[saved->child];
    if(c.retired||!c.state_prepared||c.loaded||c.loading||c.source!=saved->source||
        c.environment!=saved->environment||c.execution_thread!=saved->thread)
        throw ScriptError("postRead Globals current registered child/coroutine allocation changed");
    if(p->calling&&!p->worldview_native_host)throw ScriptError("postRead Globals foreign Lua call active");
    if(p->worldview_native_host){
        p->worldview_require_callback_origin();
        if(p->worldview_native_host->loan!=&op||p->worldview_native_host->child!=saved->child||
            p->worldview_native_host->child_source.lock()!=saved->source)
            throw ScriptError("postRead Globals active Host scope is another child/source");
    }
    input->first_();
}
void LuaScriptHost::require_context_postread_globals_input_owned(const BnwWorldViewSourceOperationLoan&op,
    const std::shared_ptr<const ui::detail::NotificationContextPostreadGlobalsInputLoan>&input)const {
    require_context_postread_globals_input_without_callbacks(op,input);
    input->full_();require_context_postread_globals_input_without_callbacks(op,input);
    op.require_same_vm(*this);require_context_postread_globals_input_without_callbacks(op,input);
    op.require_same_source(*input->prepared_->impl_->source);require_context_postread_globals_input_without_callbacks(op,input);
}
void LuaScriptHost::require_context_postread_globals_reference_without_callbacks(
    const BnwWorldViewSourceOperationLoan&op,
    const ui::detail::NotificationContextPostreadGlobalsReference&ref)const {
    const auto p=impl_;const auto fields=ref.impl_;
    if(!fields||fields->creator!=std::this_thread::get_id()||!fields->alive.load()||fields->facade!=this||
        fields->host.lock()!=p||!fields->control)throw ScriptError("postRead Globals canonical reference view retired FIRST");
    const auto r=fields->control;
    require_context_postread_globals_input_without_callbacks(op,r->input);
    const auto canonical=r->canonical.lock();
    if(!canonical||canonical.get()!=&ref||canonical->impl_!=fields||r->dropped||r->vm_retired||
        r->creator!=std::this_thread::get_id()||r->master!=p->state||r->thread!=r->prepared->impl_->actual_thread||
        r->phase!=Impl::NativePostreadGlobalsControl::Phase::prefix_returned||
        r->entry_top!=0||r->returned_top!=0||r->globals_reference<0||r->key_reference<0||
        r->globals_reference==r->key_reference||r->serial!=op.operation_serial()||r->epoch!=p->failure_epoch||
        r->writes!=p->failed.writes_||r->uncertainty!=p->execution_uncertainty||
        !Impl::native_postread_globals_retained_thread_without_callbacks(*p,*r)||
        r->thread->ci->top-r->thread->top<2||lua_gettop(r->thread)!=0||
        !Impl::native_postread_globals_physical(r->thread,*r,true))
        throw ScriptError("postRead Globals actual returned refs/referents/idle stack/owner changed");
    require_context_postread_globals_input_without_callbacks(op,r->input);
}
void LuaScriptHost::require_context_postread_globals_reference_owned(const BnwWorldViewSourceOperationLoan&op,
    const ui::detail::NotificationContextPostreadGlobalsReference&ref)const {
    require_context_postread_globals_reference_without_callbacks(op,ref);
    require_context_postread_globals_input_owned(op,ref.impl_->control->input);
    require_context_postread_globals_reference_without_callbacks(op,ref);
}
std::shared_ptr<const ui::detail::NotificationContextPostreadGlobalsReference>
LuaScriptHost::acquire_context_postread_globals_reference_owned(const BnwWorldViewSourceOperationLoan&op,
    const std::shared_ptr<const ui::detail::NotificationContextPostreadGlobalsInputLoan>&input)const {
    using Return=ui::detail::NotificationContextPostreadGlobalsReference;
    const auto p=impl_;std::shared_ptr<Impl::NativePostreadGlobalsControl> control;
    try{
        require_context_postread_globals_input_owned(op,input);
        const auto prepared=input->prepared_;const auto saved=prepared->impl_;
        require_context_lua_state_owned(op,prepared,input->core_,saved->cursor);
        require_context_postread_globals_input_owned(op,input);
        for(const auto&r:p->postread_globals_controls)if(r&&r->prepared==prepared)
            throw ScriptError("postRead Globals actual local occurrence cannot replay even after failure");
        control=std::make_shared<Impl::NativePostreadGlobalsControl>(input,prepared,saved->source,p->state,
            saved->actual_thread,saved->actual_environment,saved->environment,saved->thread,saved->child,op.operation_serial(),p->failure_epoch,
            p->failed.writes_,p->execution_uncertainty);
        require_context_postread_globals_input_owned(op,input);
        p->postread_globals_controls.push_back(control); //custody BEFORE any Lua allocation
        require_context_postread_globals_input_owned(op,input);
        {
            Impl::WorldViewNativeHostScope native(*p,*this,op);native.child=saved->child;native.child_source=saved->source;
            Impl::NativePostreadGlobalsFrame frame(*p,*this,op,*control);frame.full();
            const std::string context(p->child_contexts[saved->child].name);frame.full();
            Impl::Job acquire{p.get(),Impl::Operation::native_context_postread_globals_acquire,context,{},{},{},{},{}};
            acquire.worldview_child=saved->child;acquire.context_postread_globals=control.get();
            p->call(acquire);frame.full();
            Impl::Job query{p.get(),Impl::Operation::native_context_postread_globals_query,context,{},{},{},{},{}};
            query.worldview_child=saved->child;query.context_postread_globals=control.get();
            p->call(query);frame.full();
            if(control->phase!=Impl::NativePostreadGlobalsControl::Phase::prefix_returned||control->returned_top!=0)
                throw ScriptError("postRead Globals actual coroutine cpcall did not return prefix");
            native.returned=true;
        } //actual protected Job/frame/scope destructors BEFORE final FULL
        require_context_postread_globals_input_owned(op,input);
        if(p->calling||p->worldview_native_host||p->postread_globals_frame)
            throw ScriptError("postRead Globals protected/native/frame scopes have not retired");
        auto fields=std::make_shared<const Return::Impl>(p,this,control);
        require_context_postread_globals_input_owned(op,input);
        auto returned=std::shared_ptr<const Return>(new Return(fields));
        require_context_postread_globals_input_owned(op,input);
        control->canonical=returned;
        require_context_postread_globals_reference_owned(op,*returned);return returned;
    }catch(...){
        // Cleanup only after actual C/native/frame scopes have unwound. Genuine
        // held coroutine/Host storage is still retained; Source readiness is
        // never repaired or renewed by these mandatory exception effects.
        if(control)p->native_postread_globals_release_after_abort(*control);
        p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;
    }
}
}
