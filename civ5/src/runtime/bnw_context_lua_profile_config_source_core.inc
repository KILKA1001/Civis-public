// After complete canonical Source State. No public value/serial issuer.
struct BnwContextLuaProfileConfigLoan::Impl {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<BnwSelectedWorldViewContextOwner> source;
    const std::weak_ptr<const BnwSelectedLanguageInputOwner> input;
    const std::weak_ptr<const assets::CivisResourceBankOwner> bank;
    const std::uint64_t serial;
    const std::int32_t value;
    Impl(std::weak_ptr<BnwSelectedWorldViewContextOwner> s,
        std::weak_ptr<const BnwSelectedLanguageInputOwner> i,
        std::weak_ptr<const assets::CivisResourceBankOwner> b,std::uint64_t op,std::int32_t v)
        :source(std::move(s)),input(std::move(i)),bank(std::move(b)),serial(op),value(v){}
};
BnwContextLuaProfileConfigLoan::BnwContextLuaProfileConfigLoan(std::shared_ptr<const Impl> p):impl_(std::move(p)){}
BnwContextLuaProfileConfigLoan::~BnwContextLuaProfileConfigLoan()=default;
void BnwContextLuaProfileConfigLoan::require_current_without_callbacks(const BnwWorldViewSourceOperationLoan& op)const{
    op.require_origin_without_callbacks();op.require_render_operation_owned();
    const auto p=impl_;const auto actual=p->source.lock();
    if(p->creator!=std::this_thread::get_id()||!actual||p->input.expired()||p->bank.expired())
        throw ScriptError("Lua config creator/Source/input/Bank retired");
    actual->require_operation_origin_without_callbacks(op,p->serial);
    const auto saved=actual->state_;
    const auto issued=saved->lua_debug_config_return.lock();
    if(issued.get()!=this||issued.owner_before(saved->lua_debug_config_return)||
       saved->lua_debug_config_return.owner_before(issued)||
       !saved->lua_profile_selected_input||p->input.owner_before(saved->lua_profile_selected_input)||
       saved->lua_profile_selected_input.owner_before(p->input)||!saved->civis_sources||
       !saved->lua_profile_bank||p->bank.owner_before(saved->lua_profile_bank)||saved->lua_profile_bank.owner_before(p->bank))
        throw ScriptError("Lua config actual selected Source publication/input changed");
    op.require_origin_without_callbacks();
}
void BnwContextLuaProfileConfigLoan::require_current_owned(const BnwWorldViewSourceOperationLoan& op)const{
    require_current_without_callbacks(op);const auto p=impl_;const auto actual=p->source.lock();
    if(!actual)throw ScriptError("Lua config Source expired before full");
    try{
        const auto input=p->input.lock();const auto bank=p->bank.lock();
        if(!input||!bank)throw ScriptError("Lua config actual input/Bank expired before full");
        const auto value=input->selected_lua_debug_value_owned(bank);
        require_current_without_callbacks(op);op.require_current();require_current_without_callbacks(op);
        if(value!=p->value)throw ScriptError("Lua config real selected operand changed");
    }catch(...){actual->state_->failed.store(true);throw;}
}
std::int32_t BnwContextLuaProfileConfigLoan::logging_enabled_value_owned(const BnwWorldViewSourceOperationLoan& op)const{
    require_current_owned(op);const auto p=impl_;const auto actual=p->source.lock();
    try{
        const auto input=p->input.lock();const auto bank=p->bank.lock();
        if(!actual||!input||!bank)throw ScriptError("Lua logging actual Source/input/Bank retired");
        const auto value=input->selected_lua_logging_value_owned(bank);
        require_current_without_callbacks(op);op.require_current();require_current_without_callbacks(op);
        if(value!=0&&value!=1)throw ScriptError("Lua logging explicit selected0/1 operand unsupported");
        require_current_owned(op);return value;
    }catch(...){if(actual)actual->state_->failed.store(true);throw;}
}
std::int32_t BnwContextLuaProfileConfigLoan::debug_library_value_owned(const BnwWorldViewSourceOperationLoan& op)const{
    require_current_owned(op);const auto value=impl_->value;require_current_owned(op);return value;
}
std::shared_ptr<const BnwContextLuaProfileConfigLoan>
BnwSelectedWorldViewContextOwner::selected_lua_profile_config_owned(const BnwWorldViewSourceOperationLoan& op)const{
    op.require_origin_without_callbacks();op.require_render_operation_owned();
    const auto saved=state_;const auto serial=saved->scope_serial;
    const auto pure=[&]{require_operation_origin_without_callbacks(op,serial);};
    try{
        pure();require(saved->civis_sources&&saved->lua_profile_selected_input&&!saved->lua_debug_config_querying,
            "Lua config real cold-selected INI input absent/reentered");
        saved->lua_debug_config_querying=true;
        struct End{std::shared_ptr<State> s;~End(){s->lua_debug_config_querying=false;}}end{saved};
        if(const auto existing=saved->lua_debug_config_return.lock()){
            // Physical cachedcontrol must stilloriginate fromthisactualSource/input/Bank.
            // Oldop authority is NOT renewed; changedserial needs a freshcontrol.
            const auto cached=existing->impl_;pure();
            if(!cached||cached->creator!=std::this_thread::get_id()||cached->source.expired()||
               cached->source.owner_before(canonical_owner_without_callbacks())||canonical_owner_without_callbacks().owner_before(cached->source)||
               cached->input.owner_before(saved->lua_profile_selected_input)||
               saved->lua_profile_selected_input.owner_before(cached->input)||
               cached->bank.owner_before(saved->lua_profile_bank)||saved->lua_profile_bank.owner_before(cached->bank))
                throw ScriptError("Lua config cachedactualorigin substituted");
            if(cached->serial==serial){existing->require_current_owned(op);pure();return existing;}
        }
        const auto input=saved->lua_profile_selected_input;
        const auto bank=saved->civis_sources->bank_->retain_owner();pure();
        require(bank&&bank.get()==saved->civis_sources->bank_.get()&&!bank.owner_before(saved->civis_sources->bank_)&&
            !saved->civis_sources->bank_.owner_before(bank),"Lua config canonical Bank substituted");
        const auto value=input->selected_lua_debug_value_owned(bank);pure();op.require_current();pure();
        const auto done=std::make_shared<const BnwContextLuaProfileConfigLoan::Impl>(
            canonical_owner_without_callbacks(),input,bank,serial,value);
        pure();op.require_current();pure();
        const auto result=std::shared_ptr<const BnwContextLuaProfileConfigLoan>(new BnwContextLuaProfileConfigLoan(done));
        pure();op.require_current();pure();
        if(const auto stale=saved->lua_debug_config_return.lock()){
            require(stale->impl_&&stale->impl_->serial!=serial,"Lua config current issuer reentry/replay");
        }
        saved->lua_debug_config_return=result;
        result->require_current_owned(op);pure();return result;
    }catch(...){saved->failed.store(true);throw;}
}
