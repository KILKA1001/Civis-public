//After complete Parent::TreeView::Binding and actual Box/common return storage.
void detail::NotificationLayoutParentReceiver::TreeView::require_box_native_common_return_owned(NodeKey key)const{
    const auto saved=binding_;saved->pure(true);
    const auto core=saved->domains.at(key.domain).core;
    BnwWorldViewControlTree::require_box_native_common_return_owned(core,key.id,*saved->operation);
    saved->pure(true);const auto& receiver=NotificationLayoutParentReceiver::node(*this,key);
    const auto& actual=BnwWorldViewControlTree::Core::node(core->nodes,key.id);
    require(receiver.state==&*actual.layout_baseline&&receiver.state->parsed_source_serial_&&
        *receiver.state->parsed_source_serial_==actual.layout_receiver_occurrence->constructor_serial,
        "Box Parent receiver is not SAME returned canonical Baseline");
    const auto native_parent=receiver.state->base_.parent_38.lock();
    if(!native_parent){
        //Ctor38NULL remains real, but flat Context-id0 metadata is no native
        //parent38/Context virtual38 attachment. Do not call79D300 with that alias.
        require(!receiver.parent,"Box Parent Context/native parent38 attachment producer required");
    }else{
        require(receiver.parent&&native_parent->impl_&&native_parent->impl_->creator==std::this_thread::get_id(),
            "Box Parent actual parent38 retired or captured NULL substituted");
        const auto& parent=NotificationLayoutParentReceiver::node(*this,*receiver.parent);
        const auto parent_core=saved->domains.at(receiver.parent->domain).core;
        const auto& canonical=BnwWorldViewControlTree::Core::node(parent_core->nodes,receiver.parent->id);
        require(native_parent->impl_->core.lock()==parent_core&&native_parent->impl_->id==receiver.parent->id&&
            canonical.layout_baseline&&parent.state==&*canonical.layout_baseline&&
            parent.state->base_.receiver_owned==native_parent&&native_parent->impl_->canonical.lock()==native_parent&&
            native_parent->impl_->occurrence.lock()==canonical.layout_receiver_occurrence&&
            parent.state->base_.children_78&&parent.state->base_.common_return_owned,
            "Box Parent captured parent differs from actual opaque parent38/control");
        require((parent.state->kind_==NotificationLayoutBaseline::Kind::box&&parent.state->box_&&!parent.state->box_->failed&&
                native_parent->impl_->box.lock()==parent.state->box_&&native_parent->impl_->image.expired())||
            (parent.state->kind_==NotificationLayoutBaseline::Kind::image&&parent.state->image_&&
                native_parent->impl_->image.lock()==parent.state->image_&&native_parent->impl_->box.expired()),
            "Box Parent current real parent subtype owner unissued");
    }
    //The actual78 array, not metadata or flat attached_order, earns ordinary
    //child membership. Binding may admit true Context children independently;
    //any absent physical78 membership remains required STOP in this slice.
    require_children_owned(key);saved->pure(true);
    const auto& ordered=receiver.state->base_.children_78->impl_->ordered;
    std::size_t actual_count=0;
    for(const auto& n:nodes_)if(n.parent&&*n.parent==key)++actual_count;
    require(actual_count==ordered.size(),"Box Parent metadata/context children differ from true78 vector");
    std::uint64_t previous_order=0;
    for(const auto& child:ordered){require(child&&child->impl_,"Box Parent actual78 child missing");
        const auto& actual_child=NotificationLayoutParentReceiver::node(*this,{key.domain,child->impl_->id});
        require(actual_child.parent&&*actual_child.parent==key&&actual_child.state->base_.receiver_owned==child&&
            actual_child.attached_order>previous_order,"Box Parent actual78 order/control differs from captured closure");
        previous_order=actual_child.attached_order;
    }
    saved->pure(true);BnwWorldViewControlTree::require_box_native_common_return_owned(core,key.id,*saved->operation);
}
