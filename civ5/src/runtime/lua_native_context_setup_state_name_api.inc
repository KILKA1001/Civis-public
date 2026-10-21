// Outside actual Host::Impl, after complete Preparation::Impl.
namespace civ5::ui::detail {
struct NotificationContextLuaStateNamePrefixReturn::Impl {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const void> host,core;
    const std::weak_ptr<const NotificationContextLoadNameLoan> name;
    const std::shared_ptr<const NotificationContextLuaStatePreparation> preparation;
    const runtime::LuaScriptHost*const facade;
    const std::size_t child;
    const std::uint64_t serial,epoch,writes,uncertainty;
    const std::string value;
    Impl(std::weak_ptr<const void> h,std::weak_ptr<const void> c,
         std::weak_ptr<const NotificationContextLoadNameLoan> n,
         std::shared_ptr<const NotificationContextLuaStatePreparation> p,const runtime::LuaScriptHost* f,
         std::size_t slot,std::uint64_t op,std::uint64_t e,std::uint64_t w,std::uint64_t u,std::string v)
        :host(std::move(h)),core(std::move(c)),name(std::move(n)),preparation(std::move(p)),facade(f),
         child(slot),serial(op),epoch(e),writes(w),uncertainty(u),value(std::move(v)){}
};
NotificationContextLuaStateNamePrefixReturn::NotificationContextLuaStateNamePrefixReturn(std::shared_ptr<const Impl> p):impl_(std::move(p)){}
NotificationContextLuaStateNamePrefixReturn::~NotificationContextLuaStateNamePrefixReturn()=default;
}
namespace civ5::runtime {
std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn>
LuaScriptHost::prepare_context_lua_setup_state_name_owned(const BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>& prepared,
    const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>& name)const{
    using Return=ui::detail::NotificationContextLuaStateNamePrefixReturn;
    const auto p=impl_;
    try{
        if(!prepared||!prepared->impl_||!name)throw ScriptError("StateName genuine518/load name required");
        const auto& saved=*prepared->impl_;
        const auto full=[&]{
            name->require_current_without_callbacks(operation,prepared);
            operation.require_current();name->require_current_owned(operation,prepared);
            name->require_current_without_callbacks(operation,prepared);
            require_context_lua_state_owned(operation,prepared,saved.core,saved.cursor);
            name->require_current_without_callbacks(operation,prepared);
        };
        full();
        // OWN UTF16 DATA is copied from the executed81FEC0 field, not Source
        // context metadata. Last '/' wins, otherwise first ':' then whole.
        auto wide=name->b4_utf16_owned(operation,prepared);full();
        if(wide.size()>=260||wide.find(u'\0')!=wide.npos)
            throw ScriptError("StateName nativecvPath260 bound/terminated B4 unsupported");
        auto position=wide.find_last_of(u'/');
        if(position==wide.npos)position=wide.find(u':');
        std::u16string basename=position==wide.npos?wide:wide.substr(position+1);full();
        static_assert(sizeof(wchar_t)==sizeof(char16_t));
        // Actual6A6780 CP_UTF8/flags0/-1 calls, preserving extension and native
        // replacement semantics. No backslash basename or guessed encoding.
        const int length=WideCharToMultiByte(CP_UTF8,0,reinterpret_cast<const wchar_t*>(basename.c_str()),-1,
            nullptr,0,nullptr,nullptr);full();
        if(length<=0)throw ScriptError("StateName native UTF8 length call failed");
        std::string value;value.resize(static_cast<std::size_t>(length));full();
        const int copied=WideCharToMultiByte(CP_UTF8,0,reinterpret_cast<const wchar_t*>(basename.c_str()),-1,
            value.data(),length,nullptr,nullptr);full();
        if(copied!=length||value.back()!=0)throw ScriptError("StateName native UTF8 copy failed");
        value.pop_back();full();
        std::shared_ptr<const Return> result;
        {
            Impl::WorldViewNativeHostScope native(*p,*this,operation);p->worldview_require_callback_origin();
            native.child=saved.child;native.child_source=saved.source;
            if(saved.child>=p->child_contexts.size())throw ScriptError("StateName physical child retired");
            auto& child=p->child_contexts[saved.child];
            if(child.setup_state_name_attempted||!child.setup_state_name_return.expired()||child.loaded||child.loading||child.retired)
                throw ScriptError("StateName real setter replay/partial child");
            Impl::Job set{p.get(),Impl::Operation::native_context_setup_state_name,child.name,{},{},{},{},{}};
            set.worldview_child=saved.child;set.context_setup_name_loan=name;set.context_setup_preparation=prepared;
            set.context_state_name=value;
            p->worldview_require_callback_origin();operation.require_current();p->worldview_require_callback_origin();
            name->require_current_owned(operation,prepared);p->worldview_require_callback_origin();
            child.setup_state_name_attempted=true;
            p->call(set);p->worldview_require_callback_origin();
            name->require_current_owned(operation,prepared);p->worldview_require_callback_origin();
            Impl::Job query{p.get(),Impl::Operation::native_context_setup_state_name_query,child.name,{},{},{},{},{}};
            query.worldview_child=saved.child;query.context_setup_name_loan=name;query.context_setup_preparation=prepared;
            query.context_state_name=value;
            p->worldview_require_callback_origin();operation.require_current();p->worldview_require_callback_origin();
            name->require_current_owned(operation,prepared);p->worldview_require_callback_origin();
            p->call(query);p->worldview_require_callback_origin();
            name->require_current_owned(operation,prepared);p->worldview_require_callback_origin();
            if(!query.context_state_name_matched||query.context_state_thread!=saved.actual_thread||
                query.context_state_environment!=saved.actual_environment||
                child.setup_state_name_table_ref!=LUA_NOREF||child.setup_state_name_key_ref!=LUA_NOREF)
                throw ScriptError("StateName actual write/query/temp DTOR failed");
            const auto done=std::make_shared<const Return::Impl>(p,saved.core,name,prepared,this,saved.child,
                operation.operation_serial(),p->failure_epoch,p->failed.writes_,p->execution_uncertainty,value);
            p->worldview_require_callback_origin();operation.require_current();p->worldview_require_callback_origin();
            name->require_current_owned(operation,prepared);p->worldview_require_callback_origin();
            result=std::shared_ptr<const Return>(new Return(done));
            p->worldview_require_callback_origin();operation.require_current();p->worldview_require_callback_origin();
            name->require_current_owned(operation,prepared);p->worldview_require_callback_origin();
            child.setup_state_name_return=result;native.returned=true;
        } // Real NativeScope DTOR before the returned receipt is published.
        full();
        require_context_lua_setup_state_name_owned(operation,prepared,name,result);
        full();return result;
    }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
void LuaScriptHost::require_context_lua_setup_state_name_owned(const BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>& prepared,
    const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>& name,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn>& result)const{
    const auto p=impl_;
    try{
        if(!prepared||!prepared->impl_||!name||!result||!result->impl_)
            throw ScriptError("StateName retained actual prefix lenders absent");
        name->require_current_without_callbacks(operation,prepared);
        name->require_current_owned(operation,prepared);
        const auto& saved=*prepared->impl_;const auto& receipt=*result->impl_;
        require_context_lua_state_owned(operation,prepared,saved.core,saved.cursor);
        name->require_current_without_callbacks(operation,prepared);
        {
            Impl::WorldViewNativeHostScope native(*p,*this,operation);p->worldview_require_callback_origin();
            native.child=saved.child;native.child_source=saved.source;
            const auto pure=[&]{
                p->worldview_require_callback_origin();name->require_current_without_callbacks(operation,prepared);
                if(receipt.creator!=std::this_thread::get_id()||receipt.facade!=this||
                    receipt.host.expired()||receipt.host.owner_before(p)||p.owner_before(receipt.host)||
                    receipt.core.expired()||receipt.core.owner_before(saved.core)||saved.core.owner_before(receipt.core)||
                    receipt.name.expired()||receipt.name.owner_before(name)||name.owner_before(receipt.name)||
                    receipt.preparation.get()!=prepared.get()||receipt.preparation.owner_before(prepared)||
                    prepared.owner_before(receipt.preparation)||receipt.child!=saved.child||
                    receipt.serial!=operation.operation_serial()||receipt.epoch!=p->failure_epoch||
                    receipt.writes!=p->failed.writes_||receipt.uncertainty!=p->execution_uncertainty||
                    saved.child>=p->child_contexts.size())throw ScriptError("StateName actual receipt owner/currentness changed");
                const auto actual=p->child_contexts[saved.child].setup_state_name_return.lock();
                if(actual.get()!=result.get()||actual.owner_before(result)||result.owner_before(actual)||
                    !p->child_contexts[saved.child].setup_state_name_attempted||
                    p->child_contexts[saved.child].setup_state_name_table_ref!=LUA_NOREF||
                    p->child_contexts[saved.child].setup_state_name_key_ref!=LUA_NOREF)
                    throw ScriptError("StateName actual child publication/temp cleanup changed");
            };
            pure();
            Impl::Job query{p.get(),Impl::Operation::native_context_setup_state_name_query,
                p->child_contexts[saved.child].name,{},{},{},{},{}};
            query.worldview_child=saved.child;query.context_setup_name_loan=name;query.context_setup_preparation=prepared;
            query.context_state_name=receipt.value;
            pure();operation.require_current();pure();name->require_current_owned(operation,prepared);pure();
            p->call(query);pure();name->require_current_owned(operation,prepared);pure();
            if(!query.context_state_name_matched||query.context_state_thread!=saved.actual_thread||
                query.context_state_environment!=saved.actual_environment)
                throw ScriptError("StateName actual Lua named-field query changed");
            native.returned=true;
        }
        name->require_current_without_callbacks(operation,prepared);
        operation.require_current();name->require_current_owned(operation,prepared);
        require_context_lua_state_owned(operation,prepared,saved.core,saved.cursor);
        name->require_current_without_callbacks(operation,prepared);
    }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
