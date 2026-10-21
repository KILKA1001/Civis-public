void BnwWorldViewControlTree::require_normal_payload_pending_label_without_callbacks(
    const detail::BnwNormalGuiBatchPrefix::Impl& prefix,
    const detail::BnwLabelPaintPositionedData::Guard& data,const detail::BnwLabelPaintTextLoan::Guard& paint){
    require_normal_root_epoch_prefix_without_callbacks(prefix);
    data.require_current_without_callbacks();paint.require_current_without_callbacks();
    data.require_same_paint_guard_without_callbacks(paint);
    const auto actual=data.impl_->normal_item_occurrence.lock();
    require(actual&&data.impl_->normal_item_occurrence_issued,
        "Normal payload requires already earned distinct Item from SAME DATA");
    const detail::BnwNormalItemOccurrence::IssuerKey key;
    const detail::BnwNormalItemOccurrence::Guard item(actual,key);
    item.require_actual_positioned_data_without_callbacks(data);
    require(actual->paint.standalone_==paint.standalone_&&!paint.impl_&&
        actual->publication.get()==paint.standalone_->publication&&actual->index==paint.standalone_->index&&
        actual->control==paint.standalone_->control,
        "Normal payload original Item/Paint/publication/index/control differs");
    require_normal_pending_label_item_without_callbacks(prefix,item);
    data.require_current_without_callbacks();paint.require_current_without_callbacks();
    require_normal_root_epoch_prefix_without_callbacks(prefix);
}
