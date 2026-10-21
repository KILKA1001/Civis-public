//Outside Host::Impl after genuine Preparation and Source6 Globals APIs.
namespace civ5::ui::detail {
struct NotificationContextPostreadRootCDataReturn::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    mutable std::atomic<bool> alive{true};
    const std::weak_ptr<runtime::LuaScriptHost::Impl> host;
    const runtime::LuaScriptHost*const facade;
    const std::shared_ptr<runtime::LuaScriptHost::Impl::NativePostreadRootCDataControl> actual;
    Impl(std::weak_ptr<runtime::LuaScriptHost::Impl>h,const runtime::LuaScriptHost*f,
        std::shared_ptr<runtime::LuaScriptHost::Impl::NativePostreadRootCDataControl>r)
        :host(std::move(h)),facade(f),actual(std::move(r)){}
};
NotificationContextPostreadRootCDataReturn::NotificationContextPostreadRootCDataReturn(std::shared_ptr<const Impl>p):impl_(std::move(p)){}
NotificationContextPostreadRootCDataReturn::~NotificationContextPostreadRootCDataReturn(){impl_->alive.store(false);}
}
namespace civ5::runtime {
void LuaScriptHost::require_context_postread_root_cdata_without_callbacks(const BnwWorldViewSourceOperationLoan&op,
    const ui::detail::NotificationContextPostreadRootCDataReturn&returned)const {
    const auto p=impl_;const auto fields=returned.impl_;
    if(!fields||fields->creator!=std::this_thread::get_id()||!fields->alive.load()||fields->facade!=this||
        fields->host.lock()!=p||!fields->actual)throw ScriptError("postRead root reflection view/Host retired FIRST");
    const auto r=fields->actual;const auto canonical=r->canonical.lock();
    if(!r->globals||!r->parent)throw ScriptError("postRead root reflection has no actual Globals predecessor");
    require_context_postread_globals_reference_without_callbacks(op,*r->globals);
    const auto root=r->root.lock();const auto expected=r->parent->input->root_.lock();
    if(!canonical||canonical.get()!=&returned||canonical->impl_!=fields||r->closed||
        r->stage!=Impl::NativePostreadRootCDataControl::Stage::returned||r->creator!=std::this_thread::get_id()||
        r->entry_top!=0||r->returned_top!=0||!root||root!=expected||root.get()!=r->cdata||
        !r->type||r->type!=p->native_context_cdata_creator||r->type->host!=p.get()||!r->type->alive.load()||
        r->type->creator!=r->creator||std::strcmp(r->type->type_name,"LuaContext")||
        r->type->push_metatype!=&Impl::native_child_context_push_existing_metatable||p->postread_root_cdata_frame||
        !Impl::native_postread_globals_retained_thread_without_callbacks(*p,*r->parent)||
        !Impl::native_postread_root_cdata_physical(r->parent->thread,*r))
        throw ScriptError("postRead actual root Context CData/creator/registry/cache/caller return changed");
    require_context_postread_globals_reference_without_callbacks(op,*r->globals);
}
void LuaScriptHost::require_context_postread_root_cdata_owned(const BnwWorldViewSourceOperationLoan&op,
    const ui::detail::NotificationContextPostreadRootCDataReturn&returned)const {
    require_context_postread_root_cdata_without_callbacks(op,returned);
    require_context_postread_globals_input_owned(op,returned.impl_->actual->parent->input);
    require_context_postread_root_cdata_without_callbacks(op,returned);
}
std::shared_ptr<const ui::detail::NotificationContextPostreadRootCDataReturn>
LuaScriptHost::create_context_postread_root_cdata_owned(const BnwWorldViewSourceOperationLoan&op,
    const std::shared_ptr<const ui::detail::NotificationContextPostreadGlobalsReference>&globals)const {
    using Return=ui::detail::NotificationContextPostreadRootCDataReturn;
    const auto p=impl_;std::shared_ptr<Impl::NativePostreadRootCDataControl> actual;
    try{
        if(!globals)throw ScriptError("postRead42F060 actual Globals local1C/name local10 absent");
        require_context_postread_globals_reference_owned(op,*globals);
        const auto parent=globals->impl_->control;const auto prepared=parent->prepared;const auto saved=prepared->impl_;
        const auto root=parent->input->root_.lock();const auto type=p->native_context_cdata_creator;
        if(!root||!type||!type->alive.load()||type->host!=p.get()||type->creator!=std::this_thread::get_id()||
            std::strcmp(type->type_name,"LuaContext")||type->push_metatype!=&Impl::native_child_context_push_existing_metatable)
            throw ScriptError("postRead required genuine current ROOT Context operand/actual static creator absent");
        for(const auto&r:p->postread_root_cdata_controls)if(r&&r->parent==parent)
            throw ScriptError("postRead root reflection occurrence cannot replay prior/partial return");
        require_context_postread_globals_reference_owned(op,*globals);
        actual=std::make_shared<Impl::NativePostreadRootCDataControl>(globals,parent,root,type,root.get());
        p->postread_root_cdata_controls.push_back(actual); //Host custody before any Lua allocation
        require_context_postread_globals_reference_owned(op,*globals);
        {
            Impl::WorldViewNativeHostScope native(*p,*this,op);native.child=saved->child;native.child_source=saved->source;
            Impl::NativePostreadRootCDataFrame frame(*p,*this,op,*actual,root);frame.full();
            const std::string context(p->child_contexts[saved->child].name);frame.full();
            Impl::Job job{p.get(),Impl::Operation::native_context_postread_root_cdata_create,context,{},{},{},{},{}};
            job.worldview_child=saved->child;job.context_postread_root_cdata=actual.get();p->call(job);frame.full();
            Impl::Job query{p.get(),Impl::Operation::native_context_postread_root_cdata_query,context,{},{},{},{},{}};
            query.worldview_child=saved->child;query.context_postread_root_cdata=actual.get();p->call(query);frame.full();
            if(actual->stage!=Impl::NativePostreadRootCDataControl::Stage::returned||actual->returned_top!=0)
                throw ScriptError("postRead42F060 required actual root creator/callback/settable return absent");
            native.returned=true;
        } //native Job/frame/root physical custody scopes retire before final FULL
        require_context_postread_globals_reference_owned(op,*globals);
        if(p->calling||p->worldview_native_host||p->postread_root_cdata_frame)
            throw ScriptError("postRead root reflection protected/native/frame scopes did not return");
        auto fields=std::make_shared<const Return::Impl>(p,this,actual);
        require_context_postread_globals_reference_owned(op,*globals);
        auto returned=std::shared_ptr<const Return>(new Return(fields));
        require_context_postread_globals_reference_owned(op,*globals);
        actual->canonical=returned;require_context_postread_root_cdata_owned(op,*returned);return returned;
    }catch(...){
        if(actual)p->native_postread_root_cdata_release_after_abort(*actual);
        p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;
    }
}
}
