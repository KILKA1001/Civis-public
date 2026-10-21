// Inside civ5::ui AFTER distinct standalone Impl and genuine RootRead/Controls
// headers. ParentRoot/Child are derived solely from captured closed producers.
// No numeric parent, tag, common receipt or caller bool issues attachment.
void BnwWorldViewControlTree::store_standalone_root_parent_38_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::NotificationContextRootReadChildrenLoan& actual_read,
    const detail::NotificationLayoutNativeChildControlsReturn& actual_controls){
    using Label=detail::NotificationLayoutStandaloneLabelState;
    using Base=detail::NotificationLayoutBaseline;
    const auto read=actual_read.capture_guard_without_callbacks();read.require_current_without_callbacks();
    const auto controls=actual_controls.capture_guard_for_attach_owned(operation,actual_read);
    read.require_current_without_callbacks();controls.require_current_without_callbacks();
    controls.require_same_root_read_without_callbacks(actual_read);
    require_context_root_attach_returned_factory_identity_without_callbacks(read,controls);
    const auto root=read.actual_root_receiver_without_callbacks();
    const auto child=controls.actual_receiver_without_callbacks();const auto owner=core_;
    require(root&&child&&child->impl_&&root->impl_,"Standalone38 actual Root/child controls unavailable");
    const auto receiver_data=child->impl_;const auto label=receiver_data->standalone_label.lock();
    const auto occurrence=receiver_data->occurrence.lock();
    require(label&&label->impl_&&occurrence,"Standalone38 actual distinct allocation retired");
    const auto fields=label->impl_;const auto id=receiver_data->id; //closed genuine receiver DATA only
    const auto attachment=owner->context_root_attach_evidence.lock();
    require(attachment!=nullptr,"Standalone38 genuine lexical attachment frame required");
    attachment->first();
    read.require_current_without_callbacks();controls.require_current_without_callbacks();
    const auto& node=Core::node(owner->nodes,id);
    require(node.layout_baseline&&node.layout_baseline->kind_==Base::Kind::label&&
        node.layout_baseline->actual_type_=="Label"&&node.layout_baseline->standalone_label_==label&&
        node.layout_baseline->base_.receiver_owned==child&&node.layout_receiver_occurrence==occurrence&&
        fields->wrapper_returned&&!fields->initialization_entered&&!fields->completed_standalone_run,
        "Standalone38 needs true returned v64/common child before initialization");
    const auto*const base_address=&*node.layout_baseline;
    const auto old_cell=fields->fields[0x38/4];
    require(std::holds_alternative<Label::Impl::Null>(old_cell)&&node.layout_baseline->base_.parent_38.expired(),
        "Standalone38 reload/reparent path not supported; original constructorNULL required");
    const auto child_before=fields->sequence,base_before=node.layout_baseline->sequence_;
    const auto child_history=fields->history.size(),base_history=node.layout_baseline->history_.size();
    require(child_before!=std::numeric_limits<std::uint64_t>::max()&&base_before!=std::numeric_limits<std::uint64_t>::max(),
        "Standalone38 journal exhausted");
    bool stored=false;
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto first=[&]{
        attachment->first();
        read.require_current_without_callbacks();controls.require_current_without_callbacks();
        require_context_root_attach_returned_factory_identity_without_callbacks(read,controls);
        owner->layout_origin(true);require(&owner->source->current_operation_loan()==&operation,
            "Standalone38 canonical Source replaced before raw operand");
        operation.require_origin_without_callbacks();fields->first();
        require(same(read.actual_root_receiver_without_callbacks(),root)&&same(controls.actual_receiver_without_callbacks(),child)&&
            same(receiver_data->canonical.lock(),child)&&same(receiver_data->core.lock(),owner)&&
            same(receiver_data->occurrence.lock(),occurrence)&&same(receiver_data->standalone_label.lock(),label)&&
            same(fields->core.lock(),owner)&&same(fields->occurrence.lock(),occurrence)&&
            owner->context_root_attach_evidence.lock()==attachment&&attachment->operation==&operation&&
            attachment->stage==ContextRootAttachEvidence::Stage::before38&&
            same(attachment->root,root)&&same(attachment->child,child)&&owner->context_layout_baseline&&
            same(owner->context_layout_baseline->base_.receiver_owned,root),
            "Standalone38 SAME physical Root/child/entered attachment changed");
        const auto& fresh=Core::node(owner->nodes,id);
        require(fresh.alive&&!fresh.dynamic&&fresh.layout_baseline&& &*fresh.layout_baseline==base_address&&
            same(fresh.layout_baseline->standalone_label_,label)&&same(fresh.layout_baseline->base_.receiver_owned,child)&&
            same(fresh.layout_receiver_occurrence,occurrence)&&fields->wrapper_returned&&
            !fields->initialization_entered&&!fields->completed_standalone_run&&
            fields->sequence==child_before+(stored?1u:0u)&&fresh.layout_baseline->sequence_==base_before+(stored?1u:0u)&&
            fields->history.size()==child_history+(stored?1u:0u)&&fresh.layout_baseline->history_.size()==base_history+(stored?1u:0u),
            "Standalone38 paired physical allocation/journals changed");
        const auto*native_parent=std::get_if<Label::Impl::ParentReceiverBorrow>(&fields->fields[0x38/4]);
        require(stored?(native_parent&&same(native_parent->value.lock(),root)&&same(fresh.layout_baseline->base_.parent_38.lock(),root)):
            (std::holds_alternative<Label::Impl::Null>(fields->fields[0x38/4])&&fresh.layout_baseline->base_.parent_38.expired()),
            "Standalone38 actual nativeCell/Base preimage or paired POST differs");
        const auto array=owner->context_layout_baseline->base_.children_78;
        if(array)require(array->impl_&&same(array->impl_->parent.lock(),root)&&
            std::find(array->impl_->ordered.begin(),array->impl_->ordered.end(),child)==array->impl_->ordered.end(),
            "Standalone38 must precede Root78 append; no repeated attach");
        read.require_current_without_callbacks();controls.require_current_without_callbacks();
    };
    const auto full=[&]{first();read.require_source_owned(operation);first();controls.require_source_owned(operation);first();};
    try{
        full();
        // Allocate local empty buffers, retaining actual child fields strongly.
        // Do not reserve from a borrowed mutable Node vector across new_handler.
        std::vector<Label::Impl::Entry> new_child_history;
        std::vector<Base::Entry> new_base_history;
        require(child_history<new_child_history.max_size()&&base_history<new_base_history.max_size(),
            "Standalone38 history capacity exhausted");
        new_child_history.reserve(child_history+1);full();
        new_base_history.reserve(base_history+1);full();
        auto& fresh=Core::mutable_node_owned(owner->nodes,id);require(fresh.layout_baseline.has_value(),"Standalone38 Base retired");
        for(const auto&entry:fields->history)new_child_history.push_back(entry);
        for(const auto&entry:fresh.layout_baseline->history_)new_base_history.push_back(entry);
        fields->history.swap(new_child_history);fresh.layout_baseline->history_.swap(new_base_history);
        //Already owned capacity; no callbacks between paired real stores.
        fields->history.push_back({++fields->sequence,0x38});
        fresh.layout_baseline->history_.push_back({++fresh.layout_baseline->sequence_,Base::Mutation::receiver_prefix});
        fields->fields[0x38/4]=Label::Impl::ParentReceiverBorrow{root};
        fresh.layout_baseline->base_.parent_38=root;
        stored=true;first();full();
    }catch(...){fields->failed.store(true);owner->failed.store(true);throw;}
}
