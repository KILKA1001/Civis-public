//PRIVATE entered-paint Run capture. Graphics alone mints actual Frame.
detail::BnwPositionedLabelRunLoan::Guard::Guard(std::shared_ptr<Impl> data,
    const runtime::BnwWorldViewSourceOperationLoan& op,std::uint64_t serial,
    std::shared_ptr<const BnwWorldViewControlTree> tree,BnwLabelActualPaintCallFrame::Guard frame)
    :impl_(std::move(data)),operation_(&op),serial_(serial),paint_tree_(std::move(tree)),paint_frame_(std::move(frame)){}
detail::BnwPositionedLabelRunLoan::Guard detail::BnwPositionedLabelRunLoan::capture_guard_for_paint_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,std::shared_ptr<const BnwWorldViewControlTree> actual_tree,
    std::uint64_t actual_id,const BnwLabelActualPaintCallFrame::Guard& actual_frame)const{
    //All physical owners/Guard VALUE before callbacks; no borrowed facade reread.
    const auto data=impl_;const auto tree=std::move(actual_tree);const auto frame=actual_frame;
    frame.require_current_without_callbacks();require(bool(tree),"Paint Run captured actual Tree absent");
    frame.require_actual_control_without_callbacks(*tree,actual_id);
    require(actual_id==data->node_id,"Paint Run another genuine entered Control");
    data->first_for_paint(op,tree,frame);const auto serial=op.operation_serial();
    frame.require_current_without_callbacks();data->first_for_paint(op,tree,frame);
    return Guard(data,op,serial,tree,frame);
}
std::shared_ptr<const detail::BnwPositionedLabelRunLoan>
BnwWorldViewControlTree::positioned_label_run_for_paint_owned(std::shared_ptr<const BnwWorldViewControlTree> actual_tree,
    Control actual_control,const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwLabelActualPaintCallFrame& actual_frame)const{
    const auto tree=std::move(actual_tree);const auto actual=actual_control;
    const auto frame=actual_frame.capture_guard_without_callbacks();
    frame.require_current_without_callbacks();require(tree&&tree.get()==this,"Paint completed Run wrong actual Tree");
    const auto owner=tree->core_;frame.require_current_without_callbacks();owner->layout_origin(true);
    const auto id=owner->checked(actual);frame.require_actual_control_without_callbacks(*tree,id);
    const auto first=[&]{frame.require_current_without_callbacks();
        frame.require_actual_control_without_callbacks(*tree,id);owner->layout_origin(true);
        require(owner->checked(actual)==id,"Paint actual checked Control changed");
        const auto& current=owner->source->current_operation_loan();frame.require_current_without_callbacks();
        require(&current==&operation,"Paint completed Run requires SAME current Source");
        operation.require_origin_without_callbacks();frame.require_current_without_callbacks();};
    first();const auto& node=Core::node(owner->nodes,id);
    require(node.layout_baseline&&node.layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::button&&
        node.layout_baseline->actual_type_=="Button"&&node.layout_baseline->parsed_source_serial_&&node.layout_baseline->button_,
        "Paint standalone Label/nonButton needs its genuine factory/initialized DATA owner");
    const auto button=node.layout_baseline->button_;first();
    require(button->child_178_observation.has_value(),"Paint actual178 observation unavailable");
    const auto same=[&]{first();const auto& after=Core::node(owner->nodes,id);
        require(after.layout_baseline&&after.layout_baseline->button_.get()==button.get()&&
            !after.layout_baseline->button_.owner_before(button)&&!button.owner_before(after.layout_baseline->button_),
            "Paint current actual Button changed");first();};
    if(*button->child_178_observation==detail::NotificationLayoutButtonState::Child178Observation::constructor_null){
        const auto null_first=[&]{same();require(!button->label_178_&&!button->label_178_factory_return_&&!button->pending_label_178_&&
            button->child_178_observation==detail::NotificationLayoutButtonState::Child178Observation::constructor_null,
            "Paint genuine constructorNULL178 changed");};
        null_first();frame.require_source_owned(operation);null_first();return nullptr;
    }
    require(*button->child_178_observation==detail::NotificationLayoutButtonState::Child178Observation::factory_child&&
        button->label_178_&&button->label_178_factory_return_&&!button->pending_label_178_,
        "Paint actual published178 child is unavailable");
    const auto child=button->label_178_;const auto run=child->impl_->completed_run;
    require(bool(run),"Paint actual completed Run producer missing");
    const auto guard=run->capture_guard_for_paint_owned(operation,tree,id,frame);
    const auto run_first=[&]{same();guard.require_current_without_callbacks();
        require(button->label_178_.get()==child.get()&&!button->label_178_.owner_before(child)&&
            !child.owner_before(button->label_178_)&&child->impl_->completed_run.get()==run.get()&&
            !child->impl_->completed_run.owner_before(run)&&!run.owner_before(child->impl_->completed_run),
            "Paint actual178/publishedRun changed");guard.require_current_without_callbacks();};
    run_first();frame.require_source_owned(operation);run_first();guard.require_source_owned(operation);run_first();return run;
}
