// Outside actual Host::Impl, AFTER Preparation/StateName implementations.
namespace civ5::ui::detail {
struct NotificationContextLuaControlsTableReturn::Impl {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const void> host,core;
    const std::shared_ptr<const NotificationContextLuaStatePreparation> prepared;
    const std::shared_ptr<const NotificationContextLoadNameLoan> name;
    const std::shared_ptr<const NotificationContextLuaStateNamePrefixReturn> state_name;
    const runtime::LuaScriptHost*const facade;
    const std::size_t child;const std::uint64_t serial,epoch,writes,uncertainty;
    const void*const table;const int reference;
    Impl(std::weak_ptr<const void> h,std::weak_ptr<const void> c,
      std::shared_ptr<const NotificationContextLuaStatePreparation> p,
      std::shared_ptr<const NotificationContextLoadNameLoan> n,
      std::shared_ptr<const NotificationContextLuaStateNamePrefixReturn> sn,
      const runtime::LuaScriptHost* f,std::size_t slot,std::uint64_t s,std::uint64_t e,
      std::uint64_t w,std::uint64_t u,const void* t,int ref)
      :host(std::move(h)),core(std::move(c)),prepared(std::move(p)),name(std::move(n)),state_name(std::move(sn)),
       facade(f),child(slot),serial(s),epoch(e),writes(w),uncertainty(u),table(t),reference(ref){}
};
NotificationContextLuaControlsTableReturn::NotificationContextLuaControlsTableReturn(std::shared_ptr<const Impl> p):impl_(std::move(p)){}
NotificationContextLuaControlsTableReturn::~NotificationContextLuaControlsTableReturn()=default;
struct NotificationContextLuaControlsNamedReference::Impl {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const void> host;
    const std::shared_ptr<const NotificationContextLuaControlsTableReturn> table_return;
    const std::shared_ptr<const NotificationContextRootDefaultFieldsReturn> defaults;
    const std::shared_ptr<const NotificationContextLuaLoadSetupReturn> entered_setup;
    std::weak_ptr<const NotificationContextLuaControlsNamedReference> entered_canonical;
    enum class LocalLifetime {unissued,live_entered,nil_retired};
    mutable LocalLifetime local_lifetime=LocalLifetime::unissued;
    mutable lua_State* local_state=nullptr;
    mutable int local_reference=LUA_NOREF;
    mutable std::weak_ptr<const NotificationContextLuaControlsNamedRetirement> local_retirement;
    const runtime::LuaScriptHost*const facade;
    const std::size_t child;const std::uint64_t serial,epoch,writes,uncertainty;
    const void*const table;const int reference;
    Impl(std::weak_ptr<const void> h,std::shared_ptr<const NotificationContextLuaControlsTableReturn> tr,
      std::shared_ptr<const NotificationContextRootDefaultFieldsReturn> d,const runtime::LuaScriptHost* f,
      std::size_t slot,std::uint64_t s,std::uint64_t e,std::uint64_t w,std::uint64_t u,const void*t,int ref,std::shared_ptr<const NotificationContextLuaLoadSetupReturn> completed={})
      :host(std::move(h)),table_return(std::move(tr)),defaults(std::move(d)),entered_setup(std::move(completed)),facade(f),child(slot),
       serial(s),epoch(e),writes(w),uncertainty(u),table(t),reference(ref){}
};
NotificationContextLuaControlsNamedReference::NotificationContextLuaControlsNamedReference(std::shared_ptr<const Impl> p):impl_(std::move(p)){}
NotificationContextLuaControlsNamedReference::~NotificationContextLuaControlsNamedReference()=default;
}
namespace civ5::runtime {
std::shared_ptr<const ui::detail::NotificationContextLuaControlsTableReturn>
LuaScriptHost::prepare_context_lua_controls_table_owned(const BnwWorldViewSourceOperationLoan& op,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>& prepared_input,
    const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>& name_input,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn>& state_name_input)const {
    using Return=ui::detail::NotificationContextLuaControlsTableReturn;const auto p=impl_;
    const auto prepared=prepared_input;
    const auto name=name_input;
    const auto state_name=state_name_input;
    try {
        require_context_lua_setup_state_name_owned(op,prepared,name,state_name);
        if(!prepared||!prepared->impl_)throw ScriptError("Controls real preparation receipt missing");
        const auto& saved=*prepared->impl_;std::shared_ptr<const Return> result;
        {
            Impl::WorldViewNativeHostScope native(*p,*this,op);p->worldview_require_callback_origin();
            native.child=saved.child;native.child_source=saved.source;
            Impl::NativeContextControlsFrame frame(*p,*this,op,prepared,name,state_name,saved.source,saved.core,
                saved.child,saved.environment,saved.thread,saved.actual_environment,saved.actual_thread);
            frame.full();
            Impl::Job create{p.get(),Impl::Operation::native_context_controls_statement,
                p->child_contexts[saved.child].name,{},{},{},{},{}};
            create.worldview_child=saved.child;
            // Body first requires genuine registrar completion. With current
            // prefix17 this MUST STOP before compilation, table or LuaRef stores.
            p->call(create);frame.full();
            if(create.context_state_thread!=saved.actual_thread||create.context_state_environment!=saved.actual_environment||
               !create.context_controls_table||create.context_controls_reference==LUA_NOREF||
               create.context_controls_reference==LUA_REFNIL)
                throw ScriptError("Controls protected statement returned no real table518");
            const auto body=std::make_shared<const Return::Impl>(p,saved.core,prepared,name,state_name,this,saved.child,
                op.operation_serial(),p->failure_epoch,p->failed.writes_,p->execution_uncertainty,
                create.context_controls_table,create.context_controls_reference);
            frame.full();result=std::shared_ptr<const Return>(new Return(body));frame.full();
            p->child_contexts[saved.child].controls_table_return=result;native.returned=true;
        }
        require_context_lua_controls_table_owned(op,prepared,name,state_name,result);return result;
    }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
void LuaScriptHost::require_context_lua_controls_table_owned(const BnwWorldViewSourceOperationLoan& op,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>& prepared_input,
    const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>& name_input,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn>& state_name_input,
    const std::shared_ptr<const ui::detail::NotificationContextLuaControlsTableReturn>& result_input)const {
    const auto p=impl_;
    const auto prepared=prepared_input;
    const auto name=name_input;
    const auto state_name=state_name_input;
    const auto result=result_input;
    try {
        require_context_lua_setup_state_name_owned(op,prepared,name,state_name);
        if(!result||!result->impl_||!prepared||!prepared->impl_)throw ScriptError("Controls actual table return absent");
        const auto& r=*result->impl_;const auto& saved=*prepared->impl_;
        const auto pure=[&]{
            p->worldview_require_callback_origin();name->require_current_without_callbacks(op,prepared);
            if(r.creator!=std::this_thread::get_id()||r.facade!=this||r.host.expired()||r.core.expired()||
               r.host.owner_before(p)||p.owner_before(r.host)||r.core.owner_before(saved.core)||saved.core.owner_before(r.core)||
               r.prepared.get()!=prepared.get()||r.prepared.owner_before(prepared)||prepared.owner_before(r.prepared)||
               r.name.get()!=name.get()||r.name.owner_before(name)||name.owner_before(r.name)||
               r.state_name.get()!=state_name.get()||r.state_name.owner_before(state_name)||state_name.owner_before(r.state_name)||
               r.child!=saved.child||r.serial!=op.operation_serial()||r.epoch!=p->failure_epoch||r.writes!=p->failed.writes_||
               r.uncertainty!=p->execution_uncertainty||saved.child>=p->child_contexts.size()||
               p->child_contexts[saved.child].controls_table_pointer!=r.table||
               p->child_contexts[saved.child].controls_table_ref!=r.reference)
                throw ScriptError("Controls sealed SAME table/preparation/source/return changed");
        };
        {
            Impl::WorldViewNativeHostScope native(*p,*this,op);native.child=saved.child;native.child_source=saved.source;
            pure();Impl::NativeContextControlsFrame frame(*p,*this,op,prepared,name,state_name,saved.source,saved.core,
                saved.child,saved.environment,saved.thread,saved.actual_environment,saved.actual_thread,result);
            frame.full();pure();
            Impl::Job query{p.get(),Impl::Operation::native_context_controls_table_query,
                p->child_contexts[saved.child].name,{},{},{},{},{}};
            query.worldview_child=saved.child;p->call(query);frame.full();pure();
            if(query.context_controls_table!=r.table||query.context_controls_reference!=r.reference||
               query.context_state_thread!=saved.actual_thread||query.context_state_environment!=saved.actual_environment)
                throw ScriptError("Controls actual table field518 query mismatched");
            native.returned=true;
        }
        require_context_lua_setup_state_name_owned(op,prepared,name,state_name);
        op.require_origin_without_callbacks();name->require_current_without_callbacks(op,prepared);
        if(!Impl::controls_value_physical_without_callbacks(*p,saved.child,saved.environment,saved.thread,
            saved.actual_environment,saved.actual_thread,r.table,r.reference))
            throw ScriptError("Controls final physical field518 POST failed after last Source callback");
    }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
std::shared_ptr<const ui::detail::NotificationContextLuaControlsNamedReference>
LuaScriptHost::acquire_context_controls_named_reference_owned(const BnwWorldViewSourceOperationLoan& op,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>& prepared_input,
    const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>& name_input,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn>& state_name_input,
    const std::shared_ptr<const ui::detail::NotificationContextLuaControlsTableReturn>& table_return_input,
    const std::shared_ptr<const ui::detail::NotificationContextRootDefaultFieldsReturn>& defaults_input)const {
    using Return=ui::detail::NotificationContextLuaControlsNamedReference;const auto p=impl_;
    const auto prepared=prepared_input;
    const auto name=name_input;
    const auto state_name=state_name_input;
    const auto table_return=table_return_input;
    const auto defaults=defaults_input;
    try {
        require_context_lua_controls_table_owned(op,prepared,name,state_name,table_return);
        if(!defaults)throw ScriptError("Controls8F85A0 requires genuine Root5D defaults return");
        auto guard=defaults->capture_guard_without_callbacks(op,prepared);
        guard.require_current_owned(op,prepared);guard.require_current_without_callbacks(op,prepared);
        const auto& saved=*prepared->impl_;const auto& table=*table_return->impl_;
        std::shared_ptr<const Return> result;
        {
            Impl::WorldViewNativeHostScope native(*p,*this,op);native.child=saved.child;native.child_source=saved.source;
            guard.require_current_without_callbacks(op,prepared);
            Impl::NativeContextControlsFrame frame(*p,*this,op,prepared,name,state_name,saved.source,saved.core,
                saved.child,saved.environment,saved.thread,saved.actual_environment,saved.actual_thread,table_return,guard);
            frame.full();
            Impl::Job query{p.get(),Impl::Operation::native_context_controls_named_lookup,
                p->child_contexts[saved.child].name,{},{},{},{},{}};
            query.worldview_child=saved.child;p->call(query);frame.full();
            if(query.context_controls_table!=table.table||query.context_controls_reference!=table.reference||
               query.context_controls_named_reference==LUA_NOREF||query.context_controls_named_reference==LUA_REFNIL)
                throw ScriptError("Controls8F85A0 actual named-ref/SAME table return absent");
            const auto body=std::make_shared<const Return::Impl>(p,table_return,defaults,this,saved.child,
                op.operation_serial(),p->failure_epoch,p->failed.writes_,p->execution_uncertainty,
                query.context_controls_table,query.context_controls_named_reference);
            frame.full();result=std::shared_ptr<const Return>(new Return(body));frame.full();
            // Host registry is the physical reference owner until the reached
            // RootRead temporary cleanup producer is ported. This partial type
            // gives neither cleanup/wholeRead nor unrestricted Lua access.
            native.returned=true;
        }
        guard.require_current_owned(op,prepared);guard.require_current_without_callbacks(op,prepared);
        op.require_origin_without_callbacks();name->require_current_without_callbacks(op,prepared);
        if(!Impl::controls_value_physical_without_callbacks(*p,saved.child,saved.environment,saved.thread,
            saved.actual_environment,saved.actual_thread,table.table,table.reference,result->impl_->reference))
            throw ScriptError("Controls8F85A0 final physical named-ref POST failed after last Source callback");
        return result;
    }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
