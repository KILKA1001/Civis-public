// After complete Preparation/StateName Prefix Impl, outside Host::Impl.
namespace civ5::ui::detail {
struct NotificationContextLuaDebugConfigPrefix::Impl {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const void> host,core;
    const std::shared_ptr<const NotificationContextLuaStatePreparation> preparation;
    const std::shared_ptr<const NotificationContextLuaStateNamePrefixReturn> state_name;
    const std::shared_ptr<const runtime::BnwContextLuaProfileConfigLoan> config;
    const runtime::LuaScriptHost*const facade;
    const std::size_t child;const std::uint64_t serial,epoch,writes,uncertainty;
    const void*const table;const int reference;const std::int32_t debug;
    Impl(std::weak_ptr<const void> h,std::weak_ptr<const void> c,
        std::shared_ptr<const NotificationContextLuaStatePreparation> p,
        std::shared_ptr<const NotificationContextLuaStateNamePrefixReturn> n,
        std::shared_ptr<const runtime::BnwContextLuaProfileConfigLoan> cfg,
        const runtime::LuaScriptHost* f,std::size_t slot,std::uint64_t op,std::uint64_t e,
        std::uint64_t w,std::uint64_t u,const void* t,int r,std::int32_t d)
        :host(std::move(h)),core(std::move(c)),preparation(std::move(p)),state_name(std::move(n)),config(std::move(cfg)),
         facade(f),child(slot),serial(op),epoch(e),writes(w),uncertainty(u),table(t),reference(r),debug(d){}
};
NotificationContextLuaDebugConfigPrefix::NotificationContextLuaDebugConfigPrefix(std::shared_ptr<const Impl> p):impl_(std::move(p)){}
NotificationContextLuaDebugConfigPrefix::~NotificationContextLuaDebugConfigPrefix()=default;
}
namespace civ5::runtime {
std::shared_ptr<const ui::detail::NotificationContextLuaDebugConfigPrefix>
LuaScriptHost::prepare_context_lua_debug_config_prefix_owned(const BnwWorldViewSourceOperationLoan& op,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>& prepared_input,
    const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>& name_input,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn>& state_name_input)const{
    const auto prepared=prepared_input;const auto name=name_input;const auto state_name=state_name_input;
    using Return=ui::detail::NotificationContextLuaDebugConfigPrefix;const auto p=impl_;
    try{
        require_context_lua_setup_state_name_owned(op,prepared,name,state_name);
        if(!prepared||!prepared->impl_)throw ScriptError("Profile actual prepared state missing");
        const auto& saved=*prepared->impl_;
        const auto full=[&]{require_context_lua_setup_state_name_owned(op,prepared,name,state_name);};
        std::shared_ptr<const Return> result;
        {
            Impl::WorldViewNativeHostScope native(*p,*this,op);p->worldview_require_callback_origin();
            native.child=saved.child;native.child_source=saved.source;
            if(saved.child>=p->child_contexts.size())throw ScriptError("Profile actual child retired");
            auto& child=p->child_contexts[saved.child];
            if(child.profile_debug_attempted||child.profile_debug_ref!=LUA_NOREF||!child.profile_debug_prefix.expired())
                throw ScriptError("Profile prefix actual constructor replay/partial");
            Impl::Job create{p.get(),Impl::Operation::native_context_debug_config_prefix,child.name,{},{},{},{},{}};
            create.worldview_child=saved.child;create.context_setup_preparation=prepared;
            create.context_setup_name_loan=name;create.context_profile_state_name=state_name;
            p->worldview_require_callback_origin();op.require_current();p->worldview_require_callback_origin();
            name->require_current_owned(op,prepared);p->worldview_require_callback_origin();
            child.profile_debug_attempted=true;{Impl::NativeContextIncludeTableFrame frame(*p,create);p->call(create);}p->worldview_require_callback_origin();
            if(!create.context_profile_config)throw ScriptError("Profile selected config capture failed");
            create.context_profile_config->require_current_owned(op);p->worldview_require_callback_origin();
            Impl::Job query{p.get(),Impl::Operation::native_context_debug_config_query,child.name,{},{},{},{},{}};
            query.worldview_child=saved.child;query.context_setup_preparation=prepared;
            query.context_setup_name_loan=name;query.context_profile_state_name=state_name;
            query.context_profile_config=create.context_profile_config;
            p->worldview_require_callback_origin();op.require_current();p->worldview_require_callback_origin();
            name->require_current_owned(op,prepared);p->worldview_require_callback_origin();
            {Impl::NativeContextIncludeTableFrame frame(*p,query);p->call(query);}p->worldview_require_callback_origin();
            query.context_profile_config->require_current_owned(op);p->worldview_require_callback_origin();
            if(!query.context_profile_debug_matched||!query.context_profile_debug_table||
               query.context_state_thread!=saved.actual_thread||query.context_state_environment!=saved.actual_environment)
                throw ScriptError("Profile genuine debug/SAME environment query failed");
            const auto done=std::make_shared<const Return::Impl>(p,saved.core,prepared,state_name,create.context_profile_config,
                this,saved.child,op.operation_serial(),p->failure_epoch,p->failed.writes_,p->execution_uncertainty,
                query.context_profile_debug_table,child.profile_debug_ref,create.context_profile_debug);
            p->worldview_require_callback_origin();op.require_current();p->worldview_require_callback_origin();
            create.context_profile_config->require_current_owned(op);p->worldview_require_callback_origin();
            result=std::shared_ptr<const Return>(new Return(done));
            p->worldview_require_callback_origin();op.require_current();p->worldview_require_callback_origin();
            create.context_profile_config->require_current_owned(op);p->worldview_require_callback_origin();
            child.profile_debug_prefix=result;native.returned=true;
        }
        full();require_context_lua_debug_config_prefix_owned(op,prepared,name,state_name,result);full();return result;
    }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
void LuaScriptHost::require_context_lua_debug_config_prefix_owned(const BnwWorldViewSourceOperationLoan& op,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>& prepared_input,
    const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>& name_input,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn>& state_name_input,
    const std::shared_ptr<const ui::detail::NotificationContextLuaDebugConfigPrefix>& result_input)const{
    const auto prepared=prepared_input;const auto name=name_input;const auto state_name=state_name_input;const auto result=result_input;
    const auto p=impl_;
    try{
        require_context_lua_setup_state_name_owned(op,prepared,name,state_name);
        if(!result||!result->impl_)throw ScriptError("Profile retained prefix absent");
        const auto& saved=*prepared->impl_;const auto& r=*result->impl_;
        r.config->require_current_owned(op);
        {
            Impl::WorldViewNativeHostScope native(*p,*this,op);p->worldview_require_callback_origin();
            native.child=saved.child;native.child_source=saved.source;
            const auto pure=[&]{
                p->worldview_require_callback_origin();name->require_current_without_callbacks(op,prepared);
                r.config->require_current_without_callbacks(op);
                if(r.creator!=std::this_thread::get_id()||r.facade!=this||r.host.expired()||
                   r.host.owner_before(p)||p.owner_before(r.host)||r.core.expired()||
                   r.core.owner_before(saved.core)||saved.core.owner_before(r.core)||
                   r.preparation.get()!=prepared.get()||r.preparation.owner_before(prepared)||prepared.owner_before(r.preparation)||
                   r.state_name.get()!=state_name.get()||r.state_name.owner_before(state_name)||state_name.owner_before(r.state_name)||
                   r.child!=saved.child||r.serial!=op.operation_serial()||r.epoch!=p->failure_epoch||
                   r.writes!=p->failed.writes_||r.uncertainty!=p->execution_uncertainty||saved.child>=p->child_contexts.size())
                    throw ScriptError("Profile actual prefix owner/currentness changed");
                const auto actual=p->child_contexts[saved.child].profile_debug_prefix.lock();
                if(actual.get()!=result.get()||actual.owner_before(result)||result.owner_before(actual)||
                   !p->profile_debug_master_alive||!p->profile_gc_stopped||
                   p->profile_debug_master_table!=r.table||
                   !p->child_contexts[saved.child].profile_debug_attempted||p->child_contexts[saved.child].profile_debug_ref!=r.reference)
                    throw ScriptError("Profile actual child prefix publication/ref changed");
            };
            pure();Impl::Job query{p.get(),Impl::Operation::native_context_debug_config_query,
                p->child_contexts[saved.child].name,{},{},{},{},{}};
            query.worldview_child=saved.child;query.context_setup_preparation=prepared;query.context_setup_name_loan=name;
            query.context_profile_state_name=state_name;query.context_profile_config=r.config;
            pure();op.require_current();pure();r.config->require_current_owned(op);pure();
            {Impl::NativeContextIncludeTableFrame frame(*p,query);p->call(query);}pure();r.config->require_current_owned(op);pure();
            if(!query.context_profile_debug_matched||query.context_profile_debug_table!=r.table||
               query.context_state_thread!=saved.actual_thread||query.context_state_environment!=saved.actual_environment||
               r.config->debug_library_value_owned(op)!=r.debug)
                throw ScriptError("Profile actual debug/environment/config changed");
            pure();native.returned=true;
        }
        require_context_lua_setup_state_name_owned(op,prepared,name,state_name);r.config->require_current_owned(op);
        name->require_current_without_callbacks(op,prepared);
    }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
