//Inside civ5::ui after standalone factory/common and actual OrdinaryReceiver storage.
//Root alone owns StoreLoan issuer/guard/stages. No receiver/realm/tag input mints it.
void BnwWorldViewControlTree::store_standalone_root_attached_context_3c_owned(
    const detail::NotificationContextRootChildAttachStoreLoan& store,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::NotificationLayoutOrdinaryReceiver& receiver){
    using Label=detail::NotificationLayoutStandaloneLabelState;
    using Base=detail::NotificationLayoutBaseline;
    const auto guard=store.capture_guard_without_callbacks(operation,receiver);
    guard.require_current_without_callbacks(operation,receiver);
    const auto value=guard.value_3c_without_callbacks();
    guard.require_current_without_callbacks(operation,receiver);
    const auto owner=core_;owner->layout_origin(true);
    const auto receiver_data=receiver.impl_;
    require(receiver_data&&receiver_data->creator==std::this_thread::get_id(),
        "Standalone root3C actual receiver storage unavailable");
    const auto canonical=receiver_data->canonical.lock();
    const auto occurrence=receiver_data->occurrence.lock();
    const auto label=receiver_data->standalone_label.lock();
    require(canonical&&canonical.get()==&receiver&&occurrence&&label&&
        receiver_data->core.lock()==owner&&receiver_data->image.expired()&&receiver_data->box.expired(),
        "Standalone root3C distinct physical receiver required");
    const auto fields=label->impl_;fields->first();
    auto& node=Core::mutable_node_owned(owner->nodes,receiver_data->id);
    require(node.layout_baseline.has_value(),"Standalone root3C canonical Base absent");
    auto*const base=&*node.layout_baseline;
    require(base->kind_==Base::Kind::label&&base->actual_type_=="Label"&&
        base->standalone_label_==label&&base->base_.receiver_owned==canonical&&
        node.layout_receiver_occurrence==occurrence&&fields->wrapper_returned&&
        !fields->initialization_entered&&!fields->completed_standalone_run,
        "Standalone root3C requires actual returned common receiver before initialization");
    const auto old_realm=base->base_.context_realm_3c;
    const auto old_observation=base->base_.context_3c_observation;
    const auto old_cell=fields->fields[0x3c/4]; //OWN typed value pins old descriptor through both stores.
    const auto child_before=fields->sequence,base_before=base->sequence_;
    require(child_before!=std::numeric_limits<std::uint64_t>::max()&&base_before!=std::numeric_limits<std::uint64_t>::max(),
        "Standalone root3C journal exhausted");
    bool stored=false;
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto first=[&]{
        guard.require_current_without_callbacks(operation,receiver);owner->layout_origin(true);
        require(&owner->source->current_operation_loan()==&operation,
            "Standalone root3C current Source replaced before raw operation access");
        operation.require_origin_without_callbacks();fields->first();
        const auto& current=Core::node(owner->nodes,receiver_data->id);
        require(receiver_data->creator==std::this_thread::get_id()&&same(receiver_data->canonical.lock(),canonical)&&
            same(receiver_data->core.lock(),owner)&&same(receiver_data->occurrence.lock(),occurrence)&&
            same(receiver_data->standalone_label.lock(),label)&&current.layout_baseline&&
            &*current.layout_baseline==base&&same(current.layout_receiver_occurrence,occurrence)&&
            same(base->standalone_label_,label)&&same(base->base_.receiver_owned,canonical)&&
            same(fields->core.lock(),owner)&&same(fields->occurrence.lock(),occurrence)&&
            !fields->initialization_entered&&!fields->completed_standalone_run&&
            fields->sequence==child_before+(stored?1u:0u)&&base->sequence_==base_before+(stored?1u:0u),
            "Standalone root3C actual allocation/journals changed");
        const auto* realm=std::get_if<Label::Impl::ParsedRealm>(&fields->fields[0x3c/4]);
        const auto null=std::holds_alternative<Label::Impl::Null>(fields->fields[0x3c/4]);
        if(stored){
            require(value?(realm&&same(realm->value,value)&&same(base->base_.context_realm_3c,value)&&
                base->base_.context_3c_observation==Base::Base::Context3CObservation::parsed_realm):
                (null&&!base->base_.context_realm_3c&&base->base_.context_3c_observation==Base::Base::Context3CObservation::root_inherited_null),
                "Standalone root3C paired store POST changed");
        }else{
            const auto* before=std::get_if<Label::Impl::ParsedRealm>(&old_cell);
            require(base->base_.context_3c_observation==old_observation&&same(base->base_.context_realm_3c,old_realm)&&
                (before?(realm&&same(realm->value,before->value)&&same(realm->value,old_realm)):
                    (std::holds_alternative<Label::Impl::Null>(old_cell)&&null&&!old_realm)),
                "Standalone root3C actual previous cell/Base mismatch");
        }
        guard.require_current_without_callbacks(operation,receiver);
    };
    const auto full=[&]{first();guard.require_source_owned(operation,receiver);first();};
    try{
        full();fields->history.reserve(fields->history.size()+1);full();
        base->history_.reserve(base->history_.size()+1);full();
        //No allocation or external callback between actual native cell and Base aliases.
        fields->history.push_back({++fields->sequence,0x3c});
        base->history_.push_back({++base->sequence_,Base::Mutation::context_realm_3c});
        fields->fields[0x3c/4]=value?Label::Impl::Cell{Label::Impl::ParsedRealm{value}}:Label::Impl::Cell{Label::Impl::Null{}};
        base->base_.context_realm_3c=value;
        base->base_.context_3c_observation=value?Base::Base::Context3CObservation::parsed_realm:
            Base::Base::Context3CObservation::root_inherited_null;
        stored=true;first();full();
        //Only paired DATA stores returned. Root issuer separately advances exact native stage.
    }catch(...){fields->failed.store(true);owner->failed.store(true);throw;}
}
