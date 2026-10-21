//Inside civ5::ui AFTER actual Runtime/Gui/Ordinary storage types.
void detail::NotificationLayoutRuntimeDimensions::publish_box_constructor_owned()const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    const auto core=saved->core.lock();const auto occurrence=saved->occurrence.lock();
    require(core&&occurrence&&saved->id,"Box constructor actual owners absent");
    const auto box=saved->baseline->box_;require(bool(box),"Box58E740 actual field constructor absent");
    const auto current=[&]{require_storage_without_callbacks(saved);
        require(saved->baseline->kind_==NotificationLayoutBaseline::Kind::box&&saved->baseline->actual_type_=="Box"&&
            saved->baseline->box_.get()==box.get()&&!saved->baseline->box_.owner_before(box)&&!box.owner_before(saved->baseline->box_)&&
            saved->baseline->parser_runtime_dimensions_==this&&!saved->baseline->parsed_source_serial_&&
            !box->failed&&!box->parser_attempted&&!box->color_querying&&!box->color_stored&&
            !saved->baseline->base_.receiver_owned&&!saved->baseline->base_.common_return_owned&&
            !saved->baseline->base_.children_78&&!saved->baseline->base_.id_58&&!box->id_58&&
            !saved->baseline->history_.empty()&&saved->baseline->history_.front().command==NotificationLayoutBaseline::Mutation::construct,
            "Box58E740 actual constructor/replay/phase/control changed");
        for(float v:box->color_84_90)require(std::bit_cast<std::uint32_t>(v)==0x3f800000u,
            "Box58E740 actual four literal1 fields changed before receiver publication");};
    const auto full=[&]{current();saved->operation->require_current();current();};
    try{
        full();const auto id=saved->id,serial=saved->serial;
        const auto sequence=saved->baseline->sequence_,history_size=saved->baseline->history_.size();
        //VALUE ctor: only owned weak controls/scalars are read after newhandler.
        auto pending=std::unique_ptr<NotificationLayoutOrdinaryReceiver::Impl>(
            new NotificationLayoutOrdinaryReceiver::Impl(core,occurrence,box,id,serial));full();
        auto fields=std::shared_ptr<NotificationLayoutOrdinaryReceiver::Impl>(std::move(pending));full();
        auto receiver=std::shared_ptr<const NotificationLayoutOrdinaryReceiver>(new NotificationLayoutOrdinaryReceiver(fields));full();
        const auto actual_box=fields->box.lock();
        require(fields->id==id&&fields->serial==serial&&actual_box.get()==box.get()&&
            !actual_box.owner_before(box)&&!box.owner_before(actual_box)&&fields->image.expired()&&
            saved->baseline->sequence_==sequence&&saved->baseline->history_.size()==history_size,
            "Box constructor receiver operands/history changed across allocation POST");
        auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
        fields->canonical=receiver;owner.base_.receiver_owned=receiver; //Nonalloc SAME Box publication.
        require_storage_without_callbacks(saved);saved->operation->require_current();require_storage_without_callbacks(saved);
        require(owner.base_.receiver_owned.get()==receiver.get()&&!owner.base_.receiver_owned.owner_before(receiver)&&
            !receiver.owner_before(owner.base_.receiver_owned)&&fields->box.lock()==box,
            "Box constructor actual receiver publication changed across full POST");
    }catch(...){box->failed=true;fail_storage_without_callbacks(saved);throw;}
}

//Closed current DATA verifier, not a supplied completion boolean or init grant.
//Parser caller has its own Runtime FIRST; later TreeView caller has Binding FIRST.
void BnwWorldViewControlTree::require_box_native_common_return_owned(
    const std::shared_ptr<Core>& core,std::uint64_t id,
    const runtime::BnwWorldViewSourceOperationLoan& operation,bool expected_busy){
    require(core&&id,"Box native common return actual Core/id absent");
    core->layout_origin(expected_busy);operation.require_origin_without_callbacks();
    require(&core->source->current_operation_loan()==&operation,"Box native common return SAME current Source changed");
    const auto& node=Core::node(core->nodes,id);
    require(node.layout_baseline&&node.layout_receiver_occurrence&&node.full_xml_occurrence&&!node.dynamic,
        "Box native common return canonical constructor/XML occurrence absent");
    const auto& state=*node.layout_baseline;const auto box=state.box_;
    require(state.kind_==detail::NotificationLayoutBaseline::Kind::box&&state.actual_type_=="Box"&&box&&!box->failed&&
        box->resize_registration_&&box->common_color_stored&&box->id_58&&
        box->frontier==detail::NotificationLayoutBoxState::Frontier::common_initializer_required&&
        state.base_.receiver_owned&&state.base_.receiver_owned->impl_&&
        state.base_.common_return_owned&&state.base_.common_return_owned->impl_&&
        box->common_children_return_.get()==state.base_.common_return_owned.get()&&
        !box->common_children_return_.owner_before(state.base_.common_return_owned)&&
        !state.base_.common_return_owned.owner_before(box->common_children_return_)&&
        box->id_58.get()==state.base_.id_58.get()&&!box->id_58.owner_before(state.base_.id_58)&&
        !state.base_.id_58.owner_before(box->id_58)&&(state.base_.flags_40&8u),
        "Box native common returning fields/control unissued or changed");
    const auto receiver=state.base_.receiver_owned;const auto array=state.base_.children_78;
    const auto& fields=*receiver->impl_;const auto& done=*state.base_.common_return_owned->impl_;
    const auto canonical=fields.canonical.lock();const auto real_box=fields.box.lock();
    const auto source=node.full_xml_occurrence->source.lock();
    require(fields.creator==std::this_thread::get_id()&&done.creator==std::this_thread::get_id()&&
        fields.image.expired()&&
        real_box.get()==box.get()&&!real_box.owner_before(box)&&!box.owner_before(real_box)&&
        canonical.get()==receiver.get()&&!canonical.owner_before(receiver)&&!receiver.owner_before(canonical)&&
        fields.id==id&&fields.serial==node.layout_receiver_occurrence->constructor_serial&&
        done.serial==fields.serial&&fields.core.lock()==core&&done.core.lock()==core&&
        fields.occurrence.lock()==node.layout_receiver_occurrence&&done.occurrence.lock()==node.layout_receiver_occurrence&&
        done.selected.lock()==node.full_xml_occurrence&&done.receiver.lock()==receiver&&
        done.count==node.full_xml_occurrence->value.children.size()&&
        source.get()==core->source.get()&&
        !source.owner_before(core->source)&&!core->source.owner_before(source),
        "Box native common opaque constructor/return/XML/Source identity changed");
    require_native_ordinary_current_children_without_callbacks(receiver,state.base_.common_return_owned,array);
    //Only true Image/Box factories returned in this finite producer. Their
    //opaque receiver/control and genuine44BF40 field stores are mandatory.
    for(std::size_t i=0;array&&i<array->impl_->ordered.size();++i){const auto& child_receiver=array->impl_->ordered[i];
        require(child_receiver&&child_receiver->impl_&&child_receiver->impl_->creator==std::this_thread::get_id(),
            "Box native common actual ordered child receiver absent");
        const auto& child_fields=*child_receiver->impl_;const auto child_core=child_fields.core.lock();
        require(child_core.get()==core.get()&&!child_core.owner_before(core)&&!core.owner_before(child_core),
            "Box native common child foreign Core/control");
        const auto& child=Core::node(core->nodes,child_fields.id);
        require(child.layout_baseline&&child.layout_receiver_occurrence&&child.full_xml_occurrence&&
            child_fields.canonical.lock()==child_receiver&&child_fields.occurrence.lock()==child.layout_receiver_occurrence&&
            child_fields.serial==fields.serial&&child.full_xml_occurrence->parent.lock()==node.full_xml_occurrence&&
            child.full_xml_occurrence->ordinal<done.count&&child.layout_baseline->base_.receiver_owned==child_receiver&&
            child.layout_baseline->base_.parent_38.lock()==receiver&&child.layout_baseline->base_.common_return_owned&&
            child.layout_baseline->base_.context_realm_3c==state.base_.context_realm_3c&&
            child.layout_baseline->base_.context_3c_observation==state.base_.context_3c_observation,
            "Box native common child returning factory/order/parent38/Realm3C changed");
        const auto& baseline=*child.layout_baseline;
        require((baseline.kind_==detail::NotificationLayoutBaseline::Kind::image&&baseline.actual_type_=="Image"&&
                baseline.image_&&child_fields.image.lock()==baseline.image_&&child_fields.box.expired())||
            (baseline.kind_==detail::NotificationLayoutBaseline::Kind::box&&baseline.actual_type_=="Box"&&
                baseline.box_&&!baseline.box_->failed&&child_fields.box.lock()==baseline.box_&&child_fields.image.expired()),
            "Box native common child actual subtype constructor owner changed");
    }
    core->layout_origin(expected_busy);operation.require_origin_without_callbacks();
}

void detail::NotificationLayoutRuntimeDimensions::require_box_common_return_from_parser_owned()const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    const auto core=saved->core.lock();
    BnwWorldViewControlTree::require_box_native_common_return_owned(core,saved->id,*saved->operation);
    require_storage_without_callbacks(saved);saved->operation->require_current();require_storage_without_callbacks(saved);
    BnwWorldViewControlTree::require_box_native_common_return_owned(core,saved->id,*saved->operation);
}
