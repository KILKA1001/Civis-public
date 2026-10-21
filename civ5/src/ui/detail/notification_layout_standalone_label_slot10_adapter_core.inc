void BnwWorldViewControlTree::initialize_standalone_label_slot10_owned(
    ContextChildSlot10Frame& frame,std::uint64_t domain,std::uint64_t id){
    using Label=detail::NotificationLayoutStandaloneLabelState;using Parent=detail::NotificationLayoutParentReceiver;
    using Root=detail::NotificationContextRootChildAttachReturn;
    frame.full();require(id,"Standalone slot10 requires actual concrete receiver, not Context id0");
    std::shared_ptr<detail::BnwStandalonePositionedLabelRunLoan> pending;
    frame.with_receiver_owned(domain,id,[&](Parent::TreeView& view,Parent::NodeKey key){
        view.require_full();auto& base=*Parent::node(view,key).state;
        require(base.kind_==detail::NotificationLayoutBaseline::Kind::label&&base.standalone_label_&&base.base_.receiver_owned,
            "Standalone slot10 has no actual constructor/common receiver");
        const auto label=base.standalone_label_;const auto receiver=base.base_.receiver_owned;
        //REQUIRED ROOT definitions prove current after-v64/v38/67373E attach.
        //No synthesized declaration/factory/common receipt is accepted here.
        auto attached=Root::require_actual_current_owned(*view.binding_->operation,*receiver);view.require_full();
        require(bool(attached),"Standalone slot10 genuine Root after-attach supplier required");
        auto guard=attached->capture_current_initializer_parent_guard_owned(*view.binding_->operation,*receiver);view.require_full();
        const auto& concrete=Parent::node(view,key);require(concrete.parent.has_value(),"Standalone slot10 real attached38 absent");
        const auto actual_parent=*concrete.parent; //RootGuard already proves actualRootState/receiver, never plain node0.
        const auto child_before=label->impl_->sequence,base_before=base.sequence_;
        auto loan=std::shared_ptr<Label::InitializationLoan>(new Label::InitializationLoan(
            view,key,&base,label,receiver,attached,std::move(guard),actual_parent,child_before,base_before));loan->full();
        label->initialize_slot10_body_owned(loan);loan->full();
        pending=label->pending_positioned_run_owned(loan);loan->full();
        require(pending&&pending->impl_,"Standalone slot10 pending actual Run absent");
        //Only immutable OWN data leaves the lexical init. No retired Loan/View
        //is carried into completed_run publication or a future paint borrower.
        loan->alive.store(false);
    });
    frame.full();require(pending&&frame.receiver_destruction_return,"Standalone Run no actual View DTOR return");
    const auto done=*frame.receiver_destruction_return;const auto data=pending->impl_;
    const auto owner=frame.domains.at(domain).core;
    const auto current=[&]{frame.pure();require(done.domain==domain&&done.id==id&&done.source_serial==frame.serial&&
        done.topology==owner->topology_revision&&done.effects==owner->effect_revision&&done.actual_return_sequence,
        "Standalone Run final receiver return/epoch changed");data->first_fields(*frame.operation,false);};
    current();frame.full();current();const auto child=data->child.lock();require(bool(child),"Standalone Run publication child retired");
    require(!data->published&&!child->impl_->completed_standalone_run,"Standalone completed Run replay");
    //No allocations/user callbacks between these actual owned stores.
    child->impl_->completed_standalone_run=pending;current();
    frame.full();current();require(child->impl_->completed_standalone_run==pending,"Standalone completed Run store POST changed");
    data->published=detail::BnwStandalonePositionedLabelRunLoan::Impl::PublishedReturn{
        frame.serial,done.actual_return_sequence,done.topology,done.effects};
    pending->require_current_data_owned(*frame.operation);frame.full();pending->require_current_data_owned(*frame.operation);
}
