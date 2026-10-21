// Inside civ5::runtime after complete actual Bootstrap::Impl.
std::shared_ptr<ui::CivisInputPolicyOwner>
BnwSelectedStockLuaBootstrap::ensure_context_input_policy_before_load_owned(
    const BnwWorldViewSourceOperationLoan& loan)const{
    impl_->healthy();loan.require_same_vm(*impl_->host);impl_->healthy();
    const auto serial=loan.operation_serial();
    need(impl_->civis_sources&&impl_->worldview_sources,"Context policy needs actual Civis source owner before load");
    impl_->civis_sources->require_current();loan.require_operation(serial);impl_->healthy();
    if(!impl_->worldview_input_policy){
        // One actual supported mousekeyboard initializer, at the first
        // physical flag-read boundary. Later VM binding reuses this owner.
        auto owner=ui::CivisInputPolicyOwner::initialize_mouse_keyboard(
            impl_->civis_sources->host_,std::weak_ptr<assets::CivisResourceBankOwner>(impl_->civis_sources->bank_),loan);
        loan.require_operation(serial);impl_->healthy();impl_->civis_sources->require_current();
        need(!impl_->worldview_input_policy,"Context input initializer reentered/replaced canonical owner");
        impl_->worldview_input_policy=std::move(owner);
    }
    const auto result=impl_->worldview_input_policy;result->require_for_source(loan);
    loan.require_operation(serial);impl_->healthy();impl_->civis_sources->require_current();
    need(impl_->worldview_input_policy.get()==result.get()&&!impl_->worldview_input_policy.owner_before(result)&&
        !result.owner_before(impl_->worldview_input_policy),"Context input policy canonical allocation changed");
    return result;
}
