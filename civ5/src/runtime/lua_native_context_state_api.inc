// Included outside LuaScriptHost::Impl, with the actual private Host body visible.
namespace civ5::ui::detail {
struct NotificationContextLuaStatePreparation::Impl {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const void> host,core;
    const std::shared_ptr<const NotificationContextCursorLoadPrefixReturn> cursor;
    const std::shared_ptr<const runtime::ScriptUiChildSource> source;
    const runtime::LuaScriptHost*const facade;
    const std::size_t child;
    const std::uint64_t serial,epoch,writes,uncertainty;
    const int environment,thread;
    const void*const actual_environment;
    lua_State*const actual_thread;
    Impl(std::weak_ptr<const void> h,std::weak_ptr<const void> c,
        std::shared_ptr<const NotificationContextCursorLoadPrefixReturn> prefix,
        std::shared_ptr<const runtime::ScriptUiChildSource> descriptor,
        const runtime::LuaScriptHost* f,std::size_t slot,std::uint64_t op,std::uint64_t e,
        std::uint64_t w,std::uint64_t u,int env,int coroutine,const void*table,lua_State*state)
        :host(std::move(h)),core(std::move(c)),cursor(std::move(prefix)),source(std::move(descriptor)),
         facade(f),child(slot),serial(op),epoch(e),writes(w),uncertainty(u),environment(env),thread(coroutine),
         actual_environment(table),actual_thread(state){}
};
NotificationContextLuaStatePreparation::NotificationContextLuaStatePreparation(std::shared_ptr<const Impl> p):impl_(std::move(p)){}
NotificationContextLuaStatePreparation::~NotificationContextLuaStatePreparation()=default;
}
namespace civ5::runtime {
std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>
LuaScriptHost::prepare_context_lua_state_owned(const BnwWorldViewSourceOperationLoan& loan,
    std::shared_ptr<const ScriptUiChildSource> source,const std::weak_ptr<const void>& core,
    std::shared_ptr<const ui::detail::NotificationContextCursorLoadPrefixReturn> cursor)const {
    using Return=ui::detail::NotificationContextLuaStatePreparation;
    const auto p=impl_;
    try{
        Impl::WorldViewNativeHostScope native(*p,*this,loan);
        p->worldview_require_callback_origin();loan.require_same_vm(*this);p->worldview_require_callback_origin();
        if(!source||core.expired()||!cursor)throw ScriptError("Context518 canonical Core/source/cursor absent");
        loan.require_same_source(*source);p->worldview_require_callback_origin();
        const std::string context(source->context());p->worldview_require_callback_origin();
        if(context.empty()||context.find('\0')!=context.npos)throw ScriptError("Context518 authentic context name invalid");
        if(p->executed_contexts.contains(context))throw ScriptError("Context518 source already executed");
        for(const auto& child:p->child_contexts)if(child.name==context)
            throw ScriptError("Context518 real constructor cannot replay existing/partial child");
        const auto slot=p->child_contexts.size();p->child_contexts.emplace_back();
        auto& child=p->child_contexts.back();child.name=context;child.source=source;
        native.child=slot;native.child_source=source;
        source->require_current();p->worldview_require_callback_origin();
        child.asset=&source->asset();p->worldview_require_callback_origin();
        p->child_require_current(slot);p->worldview_require_callback_origin();
        Impl::Job create{p.get(),Impl::Operation::native_context_state_prepare,context,{},{},{},{},{}};
        create.worldview_child=slot;p->call(create);p->worldview_require_callback_origin();
        Impl::Job query{p.get(),Impl::Operation::native_context_state_query,context,{},{},{},{},{}};
        query.worldview_child=slot;p->call(query);p->worldview_require_callback_origin();
        loan.require_same_source(*source);p->worldview_require_callback_origin();loan.require_same_vm(*this);
        if(core.expired()||!query.context_state_thread||!query.context_state_environment)
            throw ScriptError("Context518 creator/query returned no retained physical state");
        const auto done=std::make_shared<const Return::Impl>(p,core,std::move(cursor),source,this,slot,
            loan.operation_serial(),p->failure_epoch,p->failed.writes_,p->execution_uncertainty,
            child.environment,child.execution_thread,query.context_state_environment,query.context_state_thread);
        const auto result=std::shared_ptr<const Return>(new Return(done));
        p->worldview_require_callback_origin();native.returned=true;return result;
    }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
void LuaScriptHost::require_context_lua_state_owned(const BnwWorldViewSourceOperationLoan& loan,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>& prepared,
    const std::weak_ptr<const void>& core,
    const std::shared_ptr<const ui::detail::NotificationContextCursorLoadPrefixReturn>& cursor)const {
    const auto p=impl_;
    try{
        Impl::WorldViewNativeHostScope native(*p,*this,loan);
        p->worldview_require_callback_origin();loan.require_same_vm(*this);p->worldview_require_callback_origin();
        if(!prepared||!prepared->impl_)throw ScriptError("Context518 preparation receipt absent");
        const auto& saved=*prepared->impl_;
        if(saved.creator!=std::this_thread::get_id()||saved.facade!=this||saved.core.expired()||core.expired()||
           saved.host.expired()||saved.host.owner_before(p)||p.owner_before(saved.host)||
           saved.core.owner_before(core)||core.owner_before(saved.core)||saved.cursor.get()!=cursor.get()||
           saved.cursor.owner_before(cursor)||cursor.owner_before(saved.cursor)||
           saved.serial!=loan.operation_serial()||saved.epoch!=p->failure_epoch||saved.writes!=p->failed.writes_||
           saved.uncertainty!=p->execution_uncertainty||saved.child>=p->child_contexts.size())
            throw ScriptError("Context518 SAME creator/Core/cursor/Host/operation changed");
        native.child=saved.child;native.child_source=saved.source;
        const auto& child=p->child_contexts[saved.child];
        if(child.source.get()!=saved.source.get()||child.source.owner_before(saved.source)||saved.source.owner_before(child.source)||
           child.environment!=saved.environment||child.execution_thread!=saved.thread||!child.state_prepared||child.retired)
            throw ScriptError("Context518 real child allocation/source/ref changed");
        loan.require_same_source(*saved.source);p->worldview_require_callback_origin();
        p->child_require_current(saved.child);p->worldview_require_callback_origin();
        Impl::Job query{p.get(),Impl::Operation::native_context_state_query,child.name,{},{},{},{},{}};
        query.worldview_child=saved.child;p->call(query);p->worldview_require_callback_origin();
        if(query.context_state_thread!=saved.actual_thread||query.context_state_environment!=saved.actual_environment)
            throw ScriptError("Context518 actual coroutine or environment rebound");
        p->worldview_require_callback_origin();native.returned=true;
    }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
// Historical prepared state has a real immutable creation serial. Fresh Source
// and SAME VM are still required; the expired creation loan is never touched.
void LuaScriptHost::require_context_lua_state_history_owned(const BnwWorldViewSourceOperationLoan& loan,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>& prepared,
    const std::weak_ptr<const void>& core,
    const std::shared_ptr<const ui::detail::NotificationContextCursorLoadPrefixReturn>& cursor,
    std::uint64_t original_serial)const {
    const auto p=impl_;
    try{
        Impl::WorldViewNativeHostScope native(*p,*this,loan);
        p->worldview_require_callback_origin();loan.require_same_vm(*this);p->worldview_require_callback_origin();
        if(!prepared||!prepared->impl_)throw ScriptError("Context518 preparation receipt absent");
        const auto& saved=*prepared->impl_;
        if(saved.creator!=std::this_thread::get_id()||saved.facade!=this||saved.core.expired()||core.expired()||
           saved.host.expired()||saved.host.owner_before(p)||p.owner_before(saved.host)||
           saved.core.owner_before(core)||core.owner_before(saved.core)||saved.cursor.get()!=cursor.get()||
           saved.cursor.owner_before(cursor)||cursor.owner_before(saved.cursor)||
           !original_serial||saved.serial!=original_serial||saved.epoch!=p->failure_epoch||saved.writes!=p->failed.writes_||
           saved.uncertainty!=p->execution_uncertainty||saved.child>=p->child_contexts.size())
            throw ScriptError("Context518 historical creation/current Core/cursor/Host changed");
        native.child=saved.child;native.child_source=saved.source;
        const auto& child=p->child_contexts[saved.child];
        if(child.source.get()!=saved.source.get()||child.source.owner_before(saved.source)||saved.source.owner_before(child.source)||
           child.environment!=saved.environment||child.execution_thread!=saved.thread||!child.state_prepared||child.retired)
            throw ScriptError("Context518 real child allocation/source/ref changed");
        loan.require_same_source(*saved.source);p->worldview_require_callback_origin();
        p->child_require_current(saved.child);p->worldview_require_callback_origin();
        Impl::Job query{p.get(),Impl::Operation::native_context_state_query,child.name,{},{},{},{},{}};
        query.worldview_child=saved.child;p->call(query);p->worldview_require_callback_origin();
        if(query.context_state_thread!=saved.actual_thread||query.context_state_environment!=saved.actual_environment)
            throw ScriptError("Context518 actual coroutine or environment rebound");
        p->worldview_require_callback_origin();native.returned=true;
    }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
