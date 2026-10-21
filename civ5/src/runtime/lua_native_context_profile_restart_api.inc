// Outside Host::Impl AFTER complete Preparation, StateName and DebugPrefix.
namespace civ5::ui::detail {
struct NotificationContextLuaProfileReturn::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const void> host,core;
    const runtime::LuaScriptHost*const facade;
    const std::shared_ptr<const NotificationContextLuaStatePreparation> preparation;
    const std::shared_ptr<const NotificationContextLoadNameLoan> name;
    const std::shared_ptr<const NotificationContextLuaStateNamePrefixReturn> state_name;
    const std::shared_ptr<const NotificationContextLuaDebugConfigPrefix> prefix;
    const std::uint64_t serial,epoch,writes,uncertainty;
    Impl(std::weak_ptr<const void> h,std::weak_ptr<const void> c,const runtime::LuaScriptHost* f,
        std::shared_ptr<const NotificationContextLuaStatePreparation> p,
        std::shared_ptr<const NotificationContextLoadNameLoan> n,
        std::shared_ptr<const NotificationContextLuaStateNamePrefixReturn> s,
        std::shared_ptr<const NotificationContextLuaDebugConfigPrefix> original,
        std::uint64_t op,std::uint64_t e,std::uint64_t w,std::uint64_t u)
        :host(std::move(h)),core(std::move(c)),facade(f),preparation(std::move(p)),name(std::move(n)),
         state_name(std::move(s)),prefix(std::move(original)),serial(op),epoch(e),writes(w),uncertainty(u){}
};
NotificationContextLuaProfileReturn::NotificationContextLuaProfileReturn(std::shared_ptr<const Impl> p):impl_(std::move(p)){}
NotificationContextLuaProfileReturn::~NotificationContextLuaProfileReturn()=default;
}
namespace civ5::runtime {
std::shared_ptr<const ui::detail::NotificationContextLuaProfileReturn>
LuaScriptHost::complete_context_lua_profile_owned(const BnwWorldViewSourceOperationLoan& op,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>& prepared_input,
    const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>& name_input,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn>& state_name_input,
    const std::shared_ptr<const ui::detail::NotificationContextLuaDebugConfigPrefix>& prefix_input)const{
    using Return=ui::detail::NotificationContextLuaProfileReturn;
    const auto prepared=prepared_input;const auto name=name_input;const auto state_name=state_name_input;
    const auto prefix=prefix_input;const auto p=impl_;
    try{
        // Sole admission from the real prefix, not a caller GC/ready value.
        require_context_lua_debug_config_prefix_owned(op,prepared,name,state_name,prefix);
        if(!prepared||!prepared->impl_||!prefix||!prefix->impl_)
            throw ScriptError("Profile actual prefix/preparation missing");
        const auto saved=prepared->impl_;const auto original=prefix->impl_;const auto config=original->config;
        const auto pure=[&]{
            if(p->ui_thread!=std::this_thread::get_id()||!p->state)throw ScriptError("Profile restart Host thread/VM retired");
            name->require_current_without_callbacks(op,prepared);
            config->require_current_without_callbacks(op);
            if(saved->child>=p->child_contexts.size()||!p->profile_debug_master_alive||
               p->profile_debug_master_table!=original->table||p->failure_epoch!=original->epoch||
               p->failed.writes_!=original->writes||p->execution_uncertainty!=original->uncertainty)
                throw ScriptError("Profile restart actual retained prefix/control changed");
            const auto actual=p->child_contexts[saved->child].profile_debug_prefix.lock();
            if(actual.get()!=prefix.get()||actual.owner_before(prefix)||prefix.owner_before(actual)||
               p->child_contexts[saved->child].profile_debug_ref!=original->reference)
                throw ScriptError("Profile restart actual prefix publication changed");
        };
        pure();
        if(!p->child_contexts[saved->child].profile_completed_return.expired())
            throw ScriptError("Profile genuine completed return replay");
        const auto run=[&](Impl::Operation operation){
            pure();
            {
                Impl::WorldViewNativeHostScope native(*p,*this,op);p->worldview_require_callback_origin();
                native.child=saved->child;native.child_source=saved->source;
                pure();Impl::Job job{p.get(),operation,p->child_contexts[saved->child].name,{},{},{},{},{}};
                job.worldview_child=saved->child;job.context_setup_preparation=prepared;
                job.context_setup_name_loan=name;job.context_profile_state_name=state_name;job.context_profile_config=config;
                pure();op.require_current();pure();config->require_current_owned(op);pure();
                {Impl::NativeContextIncludeTableFrame frame(*p,job);p->call(job);}pure();
                if(!job.context_profile_debug_matched||job.context_profile_debug_table!=original->table||
                   job.context_state_thread!=saved->actual_thread||job.context_state_environment!=saved->actual_environment||
                   p->profile_gc_stopped)
                    throw ScriptError("Profile actual GC restart/SAME environment return failed");
                pure();native.returned=true;
            }
            // Real frame and NativeScope DTOR occurred before caller admission.
            require_context_lua_setup_state_name_owned(op,prepared,name,state_name);
            config->require_current_owned(op);pure();
        };
        run(Impl::Operation::native_context_profile_restart);
        const auto full=[&]{run(Impl::Operation::native_context_profile_completed_query);};
        full();const auto data=std::make_shared<const Return::Impl>(p,saved->core,this,prepared,name,state_name,prefix,
            op.operation_serial(),p->failure_epoch,p->failed.writes_,p->execution_uncertainty);full();
        const auto returned=std::shared_ptr<const Return>(new Return(data));full();
        p->child_contexts[saved->child].profile_completed_return=returned;
        require_context_lua_profile_completed_owned(op,prepared,name,state_name,returned);
        return returned;
    }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
void LuaScriptHost::require_context_lua_profile_completed_owned(const BnwWorldViewSourceOperationLoan& op,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>& prepared_input,
    const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>& name_input,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn>& state_name_input,
    const std::shared_ptr<const ui::detail::NotificationContextLuaProfileReturn>& returned_input)const{
    const auto prepared=prepared_input;const auto name=name_input;const auto state_name=state_name_input;
    const auto returned=returned_input;const auto p=impl_;
    try{
        if(!prepared||!prepared->impl_||!returned||!returned->impl_||!name||!state_name)
            throw ScriptError("Profile actual completed return/preparation missing");
        const auto saved=prepared->impl_;const auto r=returned->impl_;const auto prefix=r->prefix;
        if(!prefix||!prefix->impl_)throw ScriptError("Profile actual original prefix missing");
        const auto original=prefix->impl_;const auto config=original->config;
        const auto pure=[&]{
            if(p->ui_thread!=std::this_thread::get_id()||!p->state)throw ScriptError("Profile completed Host thread/VM retired");
            name->require_current_without_callbacks(op,prepared);
            config->require_current_without_callbacks(op);
            if(r->creator!=std::this_thread::get_id()||r->facade!=this||r->host.expired()||
               r->host.owner_before(p)||p.owner_before(r->host)||r->core.expired()||
               r->core.owner_before(saved->core)||saved->core.owner_before(r->core)||
               r->preparation.get()!=prepared.get()||r->preparation.owner_before(prepared)||prepared.owner_before(r->preparation)||
               r->name.get()!=name.get()||r->name.owner_before(name)||name.owner_before(r->name)||
               r->state_name.get()!=state_name.get()||r->state_name.owner_before(state_name)||state_name.owner_before(r->state_name)||
               r->serial!=op.operation_serial()||r->epoch!=p->failure_epoch||r->writes!=p->failed.writes_||
               r->uncertainty!=p->execution_uncertainty||saved->child>=p->child_contexts.size()||p->profile_gc_stopped)
                throw ScriptError("Profile completed actual owner/currentness changed");
            const auto actual=p->child_contexts[saved->child].profile_completed_return.lock();
            const auto old=p->child_contexts[saved->child].profile_debug_prefix.lock();
            if(actual.get()!=returned.get()||actual.owner_before(returned)||returned.owner_before(actual)||
               old.get()!=prefix.get()||old.owner_before(prefix)||prefix.owner_before(old)||
               !p->profile_debug_master_alive||p->profile_debug_master_table!=original->table||
               p->child_contexts[saved->child].profile_debug_ref!=original->reference)
                throw ScriptError("Profile actual completed publication/ref changed");
        };
        pure();require_context_lua_setup_state_name_owned(op,prepared,name,state_name);pure();
        {
            Impl::WorldViewNativeHostScope native(*p,*this,op);p->worldview_require_callback_origin();
            native.child=saved->child;native.child_source=saved->source;
            pure();Impl::Job query{p.get(),Impl::Operation::native_context_profile_completed_query,
                p->child_contexts[saved->child].name,{},{},{},{},{}};
            query.worldview_child=saved->child;query.context_setup_preparation=prepared;
            query.context_setup_name_loan=name;query.context_profile_state_name=state_name;query.context_profile_config=config;
            pure();op.require_current();pure();config->require_current_owned(op);pure();
            {Impl::NativeContextIncludeTableFrame frame(*p,query);p->call(query);}pure();
            if(!query.context_profile_debug_matched||query.context_profile_debug_table!=original->table||
               query.context_state_thread!=saved->actual_thread||query.context_state_environment!=saved->actual_environment)
                throw ScriptError("Profile completed actual physical environment query failed");
            pure();native.returned=true;
        }
        require_context_lua_setup_state_name_owned(op,prepared,name,state_name);config->require_current_owned(op);pure();
    }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
