// Inside runtime AFTER actual SourceOp Impl. Root joins Source pointer/scope
// FIRST helper from frozen Name2. This package has NO successful issuer today.
struct BnwContextLuaIncludeCollectionLoan::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    const std::weak_ptr<BnwSelectedWorldViewContextOwner> source;
    const std::weak_ptr<const assets::CivisResourceBankOwner> bank;
    const BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t scope,serial;
    const std::vector<std::string> names;
    Impl(std::weak_ptr<BnwSelectedWorldViewContextOwner> s,
        std::weak_ptr<const assets::CivisResourceBankOwner> b,
        const BnwWorldViewSourceOperationLoan*op,std::uint64_t n,std::uint64_t id,
        std::vector<std::string> actual_names)
        :source(std::move(s)),bank(std::move(b)),operation(op),scope(n),serial(id),names(std::move(actual_names)){}
    void first(const BnwWorldViewSourceOperationLoan& op)const{
        need(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),"Lua collection Loan retired/foreign/failed FIRST");
        need(operation==&op&&scope&&serial,"Lua collection substituted Source pointer/scope");
        const auto s=source.lock();need(bool(s),"Lua collection Source retired FIRST");
        need(s->capture_active_operation_scope_serial_without_callbacks(&op)==scope,
            "Lua collection actual Source active pointer/scope changed FIRST");
        s->require_operation_origin_without_callbacks(op,scope);
        const auto actual=bank.lock();need(bool(actual),"Lua collection Bank facade retired FIRST");
        actual->require_current(); //pure; no paths/IO/callbacks
        op.require_origin_without_callbacks();
    }
    void full(const BnwWorldViewSourceOperationLoan& op)const{
        first(op);const auto actual=bank.lock();need(bool(actual),"Lua collection Bank retired");
        actual->require_boundary();first(op);op.require_operation(serial);first(op);
        const auto current=op.selected_provider_startup_bank_owned();first(op);
        need(current&&current.get()==actual.get()&&!current.owner_before(actual)&&!actual.owner_before(current),
            "Lua collection SAME selected Source Bank control changed");first(op);
    }
};
BnwContextLuaIncludeCollectionLoan::BnwContextLuaIncludeCollectionLoan(std::shared_ptr<Impl> value):impl_(std::move(value)){}
BnwContextLuaIncludeCollectionLoan::~BnwContextLuaIncludeCollectionLoan(){impl_->alive.store(false);}
void BnwContextLuaIncludeCollectionLoan::require_current_without_callbacks(const BnwWorldViewSourceOperationLoan& op)const{
    const auto saved=impl_;saved->first(op);
}
void BnwContextLuaIncludeCollectionLoan::require_current_owned(const BnwWorldViewSourceOperationLoan& op)const{
    const auto saved=impl_;try{saved->full(op);}catch(...){saved->failed.store(true);throw;}
}
std::vector<std::string> BnwContextLuaIncludeCollectionLoan::ordered_names_owned(const BnwWorldViewSourceOperationLoan& op)const{
    const auto saved=impl_;try{
        saved->full(op); //strong immutable owned names are DATA, never raw XML
        auto result=saved->names;saved->first(op);saved->full(op);saved->first(op);return result;
    }catch(...){saved->failed.store(true);throw;}
}
std::shared_ptr<const BnwContextLuaIncludeCollectionLoan>
BnwWorldViewSourceOperationLoan::context_lua_include_collection_owned()const{
    require_origin_without_callbacks();
    const auto source=impl_->owner.lock();const auto scope=impl_->scope_serial;const auto serial=impl_->identity.serial;
    need(bool(source),"Lua collection actual Source facade absent");
    const auto first=[&]{source->require_operation_origin_without_callbacks(*this,scope);require_origin_without_callbacks();};
    const auto full=[&]{first();require_operation(serial);first();};
    try{
        full();const auto bank=selected_provider_startup_bank_owned();full();
        need(bool(bank),"Lua collection actual selected provider Bank absent");bank->require_current();first();
        //Mandatory HOLD occurs inside this real Bank call BEFORE returning names.
        //No caller input, sorted keys, compiler list or empty App can bypass it.
        auto names=bank->context_lua_include_names_owned();full();bank->require_current();first();
        auto state=std::make_shared<BnwContextLuaIncludeCollectionLoan::Impl>(source,bank,this,scope,serial,std::move(names));
        full();bank->require_current();first();
        auto result=std::shared_ptr<const BnwContextLuaIncludeCollectionLoan>(new BnwContextLuaIncludeCollectionLoan(state));
        full();state->first(*this);return result;
    }catch(...){poison_owned();throw;}
}
