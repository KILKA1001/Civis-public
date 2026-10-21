// Selected scalar views are emitted only by the real manager dispatch body.
detail::BnwNormalLabelMaterialLoan::ShaderAccess::ShaderAccess(
    std::shared_ptr<Impl> actual,const detail::BnwNormalQueueDrainPermission::Guard& selected)
    :impl_(std::move(actual)),selected_(selected),
     selected_record_([&]{selected_->require_physical_current_without_callbacks();return selected_->impl_->entry->originating_record;}()),
     selected_descriptor_([&]{selected_->require_physical_current_without_callbacks();return selected_->impl_->entry->descriptor;}()){
    selected_->require_physical_current_without_callbacks();
    require(selected_->impl_->material.get()==impl_.get()&&!selected_->impl_->material.owner_before(impl_)&&
        !impl_.owner_before(selected_->impl_->material),"Selected scalar view original Material shared control differs");
    require_current_without_callbacks();
}
void detail::BnwNormalLabelMaterialLoan::ShaderAccess::require_selected_queue_association_without_callbacks(
    const detail::BnwNormalRootBatchHistory::Guard& root,const detail::BnwNormalItemOccurrence::Guard& item,
    const detail::BnwNormalQueueDrainPermission::Guard& permission,
    const graphics::detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity& record,
    const graphics::detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt& descriptor)const{
    const auto actual=impl_;const auto actual_root=root;const auto actual_item=item;const auto actual_permission=permission;
    require_current_without_callbacks();actual->root.require_same_root_without_callbacks(actual_root);
    actual->item.require_same_item_without_callbacks(actual_item);
    actual_permission.require_queued_record_without_callbacks(actual_item,record,descriptor);
    require(selected_&&selected_->impl_==actual_permission.impl_&&
        !selected_->impl_.owner_before(actual_permission.impl_)&&!actual_permission.impl_.owner_before(selected_->impl_),
        "Historical scalar view is not SAME actually current selected manager permission");
    require_current_without_callbacks();
}

