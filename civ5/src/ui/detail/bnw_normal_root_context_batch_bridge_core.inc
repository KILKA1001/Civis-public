//Inside civ5::ui AFTER original Prefix/History/Execution and genuine Context
//Entry, lexical Scope and Publication bodies. Shared declarations are Root-only.
//This joins existing capabilities; it cannot issue an Entry, Frame or Control.
void BnwWorldViewControlTree::require_normal_gui_batch_receiver_without_callbacks(
    const detail::BnwNormalGuiBatchPrefix::Impl& prefix){
    const bool indexed=prefix.index.has_value();
    require(indexed==prefix.indexed_control.has_value()&&indexed==prefix.control.has_value()&&
        indexed!=prefix.context_entry.has_value(),
        "Normal GUI prefix requires genuine indexed Control/index XOR actual Context entry");
    prefix.frame.require_current_without_callbacks();
    if(prefix.context_entry){
        //Actual Entry FIRST checks its original alive lexical Scope and Source
        //before any borrowed Tree/Publication. No id or paint index is read.
        const auto& entry=*prefix.context_entry;entry.require_current_without_callbacks();
        require(prefix.tree&&prefix.publication,"Context prefix lost actual Tree/Publication");
        entry.require_same_tree_without_callbacks(*prefix.tree);
        prefix.frame.require_actual_context_entry_without_callbacks(*prefix.publication,entry);
        const auto actual=prefix.publication->actual_root_context_entry_for_paint_without_callbacks();
        require(actual.has_value(),"Context prefix original Publication has no actual entry");
        entry.require_same_entry_without_callbacks(*actual);
        require(!prefix.publication->control_&&prefix.publication->tree_.get()==prefix.tree.get()&&
            !prefix.publication->tree_.owner_before(prefix.tree)&&
            !prefix.tree.owner_before(prefix.publication->tree_),
            "Context prefix original Publication/Tree allocation or receiver kind changed");
        entry.require_current_without_callbacks();
    }else{
        require(prefix.tree&&prefix.publication,"Indexed prefix lost actual Tree/Publication");
        const auto owner=prefix.tree->core_;owner->layout_origin(true);
        const auto& fresh=owner->source->current_operation_loan();fresh.require_origin_without_callbacks();
        require(&fresh==prefix.operation&&fresh.operation_serial()==prefix.serial,
            "Indexed prefix another current Source before actual Control borrow");
        const auto captured=prefix.indexed_control->owner_.lock();
        require(prefix.publication->control_&&!prefix.publication->root_context_entry_,
            "Indexed prefix original Publication changed receiver kind");
        const auto published=prefix.publication->control_->owner_.lock();
        require(captured&&published&&captured.get()==owner.get()&&published.get()==owner.get()&&
            !captured.owner_before(owner)&&!owner.owner_before(captured)&&
            !published.owner_before(owner)&&!owner.owner_before(published)&&
            prefix.indexed_control->id_==prefix.publication->control_->id_&&
            owner->checked(*prefix.indexed_control)==*prefix.control&&
            prefix.publication->actual_checked_control_id_for_paint_without_callbacks()==*prefix.control&&
            prefix.publication->tree_.get()==prefix.tree.get()&&
            !prefix.publication->tree_.owner_before(prefix.tree)&&
            !prefix.tree.owner_before(prefix.publication->tree_),
            "Indexed prefix requires SAME original Control/Tree physical ownership");
        prefix.frame.require_actual_paint_item_without_callbacks(*prefix.publication,*prefix.index);
        prefix.frame.require_actual_control_without_callbacks(*prefix.tree,*prefix.control);
    }
    prefix.frame.require_current_without_callbacks();
}

void BnwWorldViewControlTree::require_normal_root_history_context_without_callbacks(
    const detail::BnwNormalRootBatchHistory::Impl& history){
    //A captured historical RootScope is independent from the old ACTIVE
    //Frame/item. In particular, this function never calls Prefix::first,
    //Frame::first/current, Matrix::first/current or an indexed paint getter.
    history.root_scope.require_physical_current_without_callbacks();
    require(history.prefix&&history.prefix->context_entry&&!history.prefix->index&&
        !history.prefix->indexed_control&&!history.prefix->control&&
        std::holds_alternative<detail::BnwNormalRootBatchHistory::Impl::RootScope::Root>(history.root_scope.value),
        "Context history needs genuine generic RootScope and no indexed receiver surrogate");
    const auto& entry=*history.prefix->context_entry;entry.require_current_without_callbacks();
    require(history.tree&&history.publication&&history.adjusted_frame&&history.renderer,
        "Context history original strongly owned associations are absent");
    entry.require_same_tree_without_callbacks(*history.tree);
    const auto actual=history.publication->actual_root_context_entry_for_paint_without_callbacks();
    require(actual.has_value(),"Context history original Publication lost actual entry");
    entry.require_same_entry_without_callbacks(*actual);
    require(!history.publication->control_&&history.prefix->tree.get()==history.tree.get()&&
        !history.prefix->tree.owner_before(history.tree)&&!history.tree.owner_before(history.prefix->tree)&&
        history.publication->tree_.get()==history.tree.get()&&
        !history.publication->tree_.owner_before(history.tree)&&
        !history.tree.owner_before(history.publication->tree_)&&
        history.prefix->publication==history.publication.get(),
        "Context history original Entry/Prefix/Publication/Tree ownership changed");
    history.root_scope.require_actual_publication_without_callbacks(*history.publication);
    history.root_scope.require_actual_frame_owner_without_callbacks(history.adjusted_frame);
    history.root_scope.require_actual_renderer_without_callbacks(*history.renderer);
    entry.require_current_without_callbacks();
    history.root_scope.require_physical_current_without_callbacks();
}

std::unique_ptr<detail::BnwNormalGuiBatchPrefix>
BnwWorldViewControlRenderPublication::begin_normal_gui_batch_prefix_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalRootPaintCallFrame& actual_frame,
    std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> actual_matrix)const{
    const auto frame=actual_frame.capture_guard_without_callbacks();frame.require_current_without_callbacks();
    return begin_normal_gui_batch_prefix_from_frame_guard_owned(operation,frame,std::move(actual_matrix));
}

std::unique_ptr<detail::BnwNormalGuiBatchPrefix>
BnwWorldViewControlRenderPublication::begin_normal_gui_batch_prefix_from_frame_guard_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalRootPaintCallFrame::Guard& actual_guard,
    std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> actual_matrix)const{
    return begin_normal_gui_batch_prefix_from_frame_guard_impl_owned(
        std::nullopt,operation,actual_guard,std::move(actual_matrix));
}

std::unique_ptr<detail::BnwNormalRootBatchExecution>
BnwWorldViewControlTree::begin_normal_root_batch_execution_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const BnwWorldViewControlRenderPublication& actual_publication,
    const detail::BnwNormalRootPaintCallFrame& actual_frame)const{
    const auto frame=actual_frame.capture_guard_without_callbacks();frame.require_current_without_callbacks();
    return begin_normal_root_batch_execution_from_frame_guard_impl_owned(
        operation,actual_publication,std::nullopt,frame);
}
