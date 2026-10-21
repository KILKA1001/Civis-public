// Genuine immutable indexed Caption carrier -> lexical Paint -> DATA joins.
// This adds no coordinate conversion, local glyph translation or clip issuer.
std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan>
BnwWorldViewControlTree::normal_caption_fp32_rect_for_actual_paint_without_callbacks(
    const BnwWorldViewControlRenderPublication& offered,std::size_t index,
    const detail::BnwLabelActualPaintCallFrame::Guard& supplied_frame){
    const auto frame=supplied_frame;
    frame.require_current_without_callbacks();
    const auto publication=frame.original_publication_without_callbacks();
    require(publication&&publication.get()==&offered,
        "Caption FP32 Paint is not SAME Frame's original Publication");
    const auto original=publication->original_owner_without_callbacks();
    require(original.get()==publication.get()&&!original.owner_before(publication)&&
        !publication.owner_before(original),"Caption FP32 Paint original Publication control changed");
    frame.require_actual_paint_item_without_callbacks(*publication,index);
    const auto emission=publication->normal_caption_emission_;
    require(emission&&publication->adjusted_frame_painting_&&publication->adjusted_frame_owner_&&
        publication->paint_indices_.size()==1&&publication->paint_indices_[0]==index,
        "Caption FP32 Paint requires genuine indexed caption emission and original entered paint");
    require_normal_button_caption_emission_current_without_callbacks(*emission);
    const auto captured=emission->impl_;const auto caption=captured->caption.impl_;
    const auto tree=caption->root.impl_->tree;const auto rect=captured->rect;
    require(rect&&index==captured->index&&index<captured->count&&
        captured->origin->paints.size()==captured->count&&
        &captured->origin->paints[index]==captured->item,
        "Caption FP32 Paint emission/index/immutable carrier association changed");
    publication->require_adjusted_frame_owner_without_callbacks(captured->origin);
    frame.require_actual_control_without_callbacks(*tree,caption->control.id_);
    rect->require_current_without_callbacks();rect->require_same_caption_without_callbacks(captured->caption);
    rect->require_same_initialized_run_without_callbacks(captured->run);
    require(publication->frame_.paints.size()==captured->count&&index<publication->frame_.paints.size(),
        "Caption FP32 Paint actual bookkeeping span changed");
    const auto& item=captured->origin->paints[index];
    const auto bookkeeping=publication->frame_.paints[index].normal_caption_fp32_rect;
    require(item.normal_caption_fp32_rect&&item.normal_caption_fp32_rect.get()==rect.get()&&
        !item.normal_caption_fp32_rect.owner_before(rect)&&!rect.owner_before(item.normal_caption_fp32_rect)&&
        bookkeeping&&bookkeeping.get()==rect.get()&&!bookkeeping.owner_before(rect)&&!rect.owner_before(bookkeeping),
        "Caption FP32 Paint item/bookkeeping carrier is not SAME original rect control");
    frame.require_current_without_callbacks();
    require_normal_button_caption_emission_current_without_callbacks(*emission);
    rect->require_current_without_callbacks();return rect;
}

void BnwWorldViewControlTree::require_no_caption_fp32_rect_for_legacy_paint_without_callbacks(
    const BnwWorldViewControlRenderPublication& offered,
    const detail::BnwLabelActualPaintCallFrame::Guard& supplied_frame){
    const auto frame=supplied_frame;
    frame.require_current_without_callbacks();
    const auto publication=frame.original_publication_without_callbacks();
    require(publication&&publication.get()==&offered,
        "Legacy Paint is not SAME Frame's original Publication");
    const auto original=publication->original_owner_without_callbacks();
    require(original.get()==publication.get()&&!original.owner_before(publication)&&
        !publication.owner_before(original)&&!publication->normal_caption_emission_,
        "Actual Caption requires closed FP32 Paint entry; legacy fallback is unavailable");
    const auto owner=publication->adjusted_frame_owner_;
    require(owner&&publication->adjusted_frame_painting_,
        "Legacy Paint original adjusted carrier is not entered/owned");
    publication->require_adjusted_frame_owner_without_callbacks(owner);
    for(const auto index:publication->paint_indices_){
        require(index<owner->paints.size()&&!owner->paints[index].normal_caption_fp32_rect,
            "Legacy Paint cannot consume an actual FP32 caption item");
    }
    frame.require_current_without_callbacks();
}

std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan>
BnwWorldViewControlRenderPublication::caption_fp32_rect_for_actual_item_without_callbacks(
    std::size_t index,const detail::BnwLabelActualPaintCallFrame::Guard& frame)const{
    return BnwWorldViewControlTree::normal_caption_fp32_rect_for_actual_paint_without_callbacks(*this,index,frame);
}
void BnwWorldViewControlRenderPublication::require_no_caption_fp32_rect_for_legacy_paint_without_callbacks(
    const detail::BnwLabelActualPaintCallFrame::Guard& frame)const{
    BnwWorldViewControlTree::require_no_caption_fp32_rect_for_legacy_paint_without_callbacks(*this,frame);
}

void BnwWorldViewControlTree::require_normal_caption_item_fp32_rect_physical_without_callbacks(
    const detail::BnwNormalItemOccurrence::Impl& saved){
    // Called only from the genuine earned historical Item checker. Root and
    // child scope health precede every saved-control read; no retired CURRENT.
    saved.root.require_physical_current_without_callbacks();saved.scope.require_physical_current_without_callbacks();
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require(saved.caption_history&&saved.caption_emission&&saved.embedded_paint_storage&&saved.data_storage&&
        saved.adjusted_frame&&saved.publication,"Caption historical rect originals absent");
    const auto caption=saved.caption_history->impl_;const auto emission=saved.caption_emission->impl_;
    const auto run=caption->run->impl_;const auto paint=saved.embedded_paint_storage;const auto data=saved.data_storage;
    const auto rect=emission->rect;const auto rect_data=rect?rect->impl_:nullptr;
    require(rect&&rect_data&&rect_data->creator==saved.creator&&rect_data->sealed&&
        same(rect_data->canonical_self.lock(),rect_data)&&same(rect_data->canonical_facade.lock(),rect)&&
        same(rect_data->caption.impl_,caption)&&same(rect_data->run,caption->run)&&
        caption->original_fp32_rect_control&&same(caption->original_fp32_rect_control->lock(),rect_data)&&
        !caption->capturing_fp32_rect&&caption->paint_arguments&&
        rect_data->caption_arguments_xy_alpha==caption->paint_arguments->result&&
        rect_data->numeric==caption->paint_arguments->numeric&&
        rect_data->own_geometry_28_34==std::array<std::uint32_t,4>{run->value.geometry[6],run->value.geometry[7],
            run->value.geometry[8],run->value.geometry[9]}&&
        saved.index<saved.adjusted_frame->paints.size()&&
        same(saved.adjusted_frame->paints[saved.index].normal_caption_fp32_rect,rect)&&
        saved.publication->frame_.paints.size()==emission->count&&saved.index<emission->count&&
        same(saved.publication->frame_.paints[saved.index].normal_caption_fp32_rect,rect)&&
        same(paint->caption_rect,rect)&&same(paint->caption_publication,saved.publication)&&
        paint->caption_index&&*paint->caption_index==saved.index&&same(data->caption_rect,rect),
        "Embedded178 historical SAME physical FP32 originals changed");
    rect_data->root.require_same_root_without_callbacks(saved.root);
    saved.root.require_physical_current_without_callbacks();saved.scope.require_physical_current_without_callbacks();
}

std::unique_ptr<detail::BnwLabelPaintTextLoan> detail::BnwLabelPaintTextLoan::begin_caption_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    std::shared_ptr<const BnwWorldViewControlTree> supplied_tree,std::uint64_t actual_control_id,
    std::shared_ptr<const BnwPositionedLabelRunLoan> supplied_run,
    std::shared_ptr<const BnwWorldViewControlRenderPublication> supplied_publication,std::size_t index,
    const BnwLabelActualPaintCallFrame& actual_frame){
    // All original facades and then the closed loan are strong BEFORE the
    // entered Run's Source/allocator cuts. No raw caller facade is reread.
    const auto tree=std::move(supplied_tree);const auto run_owner=std::move(supplied_run);
    const auto publication=std::move(supplied_publication);
    const auto frame=actual_frame.capture_guard_without_callbacks();frame.require_current_without_callbacks();
    require(tree&&run_owner&&publication,"Caption Paint actual original Tree/Run/Publication absent");
    const auto rect=publication->caption_fp32_rect_for_actual_item_without_callbacks(index,frame);
    rect->require_same_initialized_run_without_callbacks(run_owner);
    frame.require_actual_control_without_callbacks(*tree,actual_control_id);
    const auto early_first=[&]{frame.require_current_without_callbacks();
        const auto actual=publication->caption_fp32_rect_for_actual_item_without_callbacks(index,frame);
        require(actual.get()==rect.get()&&!actual.owner_before(rect)&&!rect.owner_before(actual),
            "Caption Paint pre-capture Source cut replaced SAME rect loan");
        rect->require_same_initialized_run_without_callbacks(run_owner);
        frame.require_actual_control_without_callbacks(*tree,actual_control_id);frame.require_current_without_callbacks();};
    early_first();
    const auto run=run_owner->capture_guard_for_paint_owned(operation,tree,actual_control_id,frame);
    early_first();run.require_current_without_callbacks();rect->require_same_entered_run_without_callbacks(run);
    const auto first=[&]{early_first();run.require_current_without_callbacks();
        run.require_actual_control_without_callbacks(*tree,actual_control_id);
        rect->require_same_entered_run_without_callbacks(run);early_first();};
    const auto full=[&]{first();frame.require_source_owned(operation);first();
        run.require_source_owned(operation);first();rect->require_source_owned(operation);first();};
    first();full();first();
    const auto system=run.actual_system_font_without_callbacks();first();
    require(bool(system),"Caption Paint actual170 is NULL; system-font path unavailable");
    auto data=std::make_shared<Impl>(run,frame,tree,actual_control_id,system,operation,rect,publication,index);
    first();data->first();data->full(operation);first();
    auto result=std::unique_ptr<BnwLabelPaintTextLoan>(new BnwLabelPaintTextLoan(data));
    first();data->first();data->full(operation);first();
    return result;
}

std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan>
detail::BnwLabelPaintTextLoan::Guard::actual_caption_fp32_rect_without_callbacks()const{
    require_current_without_callbacks();
    const auto rect=standalone_?std::shared_ptr<const BnwNormalCaptionFp32RectLoan>{}:impl_->caption_rect;
    require_current_without_callbacks();return rect;
}
std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan>
detail::BnwLabelPaintPositionedData::Guard::actual_caption_fp32_rect_without_callbacks()const{
    const auto state=impl_->state;state->first();const auto rect=state->caption_rect;
    const auto paint=state->paint.actual_caption_fp32_rect_without_callbacks();
    require(paint.get()==rect.get()&&!paint.owner_before(rect)&&!rect.owner_before(paint),
        "DATA FP32 rect identity differs from SAME captured Paint loan");
    state->first();return rect;
}
