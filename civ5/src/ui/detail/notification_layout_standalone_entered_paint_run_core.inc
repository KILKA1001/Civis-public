//Inside civ5::ui after complete standalone Run/Realm/Module/ordinary storage.
void detail::BnwStandalonePositionedLabelRunLoan::Impl::first_for_paint(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const BnwWorldViewControlTree>& actual_tree,
    const BnwLabelActualPaintCallFrame::Guard& frame)const{
    frame.require_current_without_callbacks();
    require(creator==std::this_thread::get_id()&&alive.load()&&actual_tree,
        "Standalone Paint Run retired/foreign/actual Tree absent");
    frame.require_actual_control_without_callbacks(*actual_tree,id);
    const auto owner=core.lock();require(bool(owner),"Standalone Paint Run actual Core retired");
    const auto first=[&]{frame.require_current_without_callbacks();
        frame.require_actual_control_without_callbacks(*actual_tree,id);owner->layout_origin(true);
        require(actual_tree->core_.get()==owner.get()&&!actual_tree->core_.owner_before(owner)&&
            !owner.owner_before(actual_tree->core_),"Standalone Paint Run actual Tree/Core differs");
        frame.require_current_without_callbacks();};first();
    const auto& current=owner->source->current_operation_loan();first();
    require(&current==&operation,"Standalone Paint Run requires fresh SAME Source before borrowed operation");
    current.require_origin_without_callbacks();first();
    require(published&&published->source_serial&&published->receiver_return_sequence&&published->topology==topology,
        "Standalone Paint Run actual whole slot10/DTOR publication missing");
    first_fields(current,true,&frame);first();
    const auto object=child.lock();require(bool(object),"Standalone Paint Run actual initialized child retired");
    const auto& completed=object->impl_->completed_standalone_run;
    require(completed&&completed->impl_.get()==this,
        "Standalone Paint Run is not actual child's SAME current completed DATA");first();
}
detail::BnwStandalonePositionedLabelRunLoan::Guard::Guard(std::shared_ptr<Impl> data,
    const runtime::BnwWorldViewSourceOperationLoan& operation,std::uint64_t serial,
    std::shared_ptr<const BnwWorldViewControlTree> tree,BnwLabelActualPaintCallFrame::Guard frame)
    :impl_(std::move(data)),operation_(&operation),serial_(serial),tree_(std::move(tree)),frame_(std::move(frame)){}
void detail::BnwStandalonePositionedLabelRunLoan::Guard::require_current_without_callbacks()const{
    //Frame's independent lexical alive/renderer FIRST precedes old Source pointer access.
    frame_.require_current_without_callbacks();require(bool(tree_),"Standalone Paint Guard actual Tree absent");
    frame_.require_actual_control_without_callbacks(*tree_,impl_->id);
    const auto owner=impl_->core.lock();require(bool(owner),"Standalone Paint Guard Core retired");
    owner->layout_origin(true);const auto& current=owner->source->current_operation_loan();
    frame_.require_current_without_callbacks();owner->layout_origin(true);
    require(&current==operation_,"Standalone Paint Guard Source replaced before old raw pointer");
    current.require_origin_without_callbacks();require(current.operation_serial()==serial_,
        "Standalone Paint Guard lexical Source serial changed");
    frame_.require_current_without_callbacks();impl_->first_for_paint(current,tree_,frame_);
    frame_.require_current_without_callbacks();
}
void detail::BnwStandalonePositionedLabelRunLoan::Guard::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& current)const{
    require_current_without_callbacks();require(&current==operation_,"Standalone Paint Guard another Source operand");
    frame_.require_source_owned(current);require_current_without_callbacks();
}
std::uint32_t detail::BnwStandalonePositionedLabelRunLoan::Guard::actual_field_44_bits_without_callbacks()const{
    require_current_without_callbacks();const auto out=impl_->value.field_44;
    require_current_without_callbacks();return out; //Physical word44 DATA, never color/style derived Alpha.
}
std::shared_ptr<const detail::BnwLabelSystemFontBorrower>
detail::BnwStandalonePositionedLabelRunLoan::Guard::actual_system_font_without_callbacks()const{
    require_current_without_callbacks();const auto out=impl_->system;
    //first_fields proved genuine same positive170 descriptor or actual typedNULL170.
    require_current_without_callbacks();return out;
}
const detail::BnwCompletedFontLoan& detail::BnwStandalonePositionedLabelRunLoan::Guard::actual_font_without_callbacks()const{
    require_current_without_callbacks();require(bool(impl_->font),"Standalone Paint physical CC absent");
    const auto& out=*impl_->font;require_current_without_callbacks();return out;
}
detail::BnwStandalonePositionedLabelRunLoan::Guard
detail::BnwStandalonePositionedLabelRunLoan::capture_guard_for_paint_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    std::shared_ptr<const BnwWorldViewControlTree> actual_tree,std::uint64_t actual_id,
    const BnwLabelActualPaintCallFrame& actual_frame)const{
    const auto frame=actual_frame.capture_guard_without_callbacks();
    frame.require_current_without_callbacks();const auto data=impl_;const auto tree=std::move(actual_tree);
    require(tree&&actual_id==data->id,"Standalone Paint capture another actual Control");
    frame.require_actual_control_without_callbacks(*tree,actual_id);
    data->first_for_paint(operation,tree,frame);
    const auto serial=operation.operation_serial();frame.require_current_without_callbacks();
    data->first_for_paint(operation,tree,frame);
    Guard out(data,operation,serial,tree,frame);out.require_source_owned(operation);
    out.require_current_without_callbacks();return out;
}
std::shared_ptr<const detail::BnwStandalonePositionedLabelRunLoan>
BnwWorldViewControlTree::standalone_positioned_label_run_for_paint_owned(
    std::shared_ptr<const BnwWorldViewControlTree> actual_tree,Control actual_control,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwLabelActualPaintCallFrame& actual_frame)const{
    const auto frame=actual_frame.capture_guard_without_callbacks();frame.require_current_without_callbacks();
    const auto tree=std::move(actual_tree);const auto actual=actual_control;
    require(tree&&tree.get()==this,"Standalone Paint getter actual Tree differs");
    const auto owner=tree->core_;owner->layout_origin(true);const auto id=owner->checked(actual);
    frame.require_actual_control_without_callbacks(*tree,id);
    const auto first=[&]{frame.require_current_without_callbacks();
        frame.require_actual_control_without_callbacks(*tree,id);owner->layout_origin(true);
        require(owner->checked(actual)==id,"Standalone Paint getter canonical Control changed");
        const auto& current=owner->source->current_operation_loan();frame.require_current_without_callbacks();
        require(&current==&operation,"Standalone Paint getter requires SAME fresh Source");
        current.require_origin_without_callbacks();frame.require_current_without_callbacks();};first();
    const auto& node=Core::node(owner->nodes,id);
    require(node.layout_baseline&&node.layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::label&&
        node.layout_baseline->actual_type_=="Label"&&node.layout_baseline->parsed_source_serial_&&
        *node.layout_baseline->parsed_source_serial_&&node.layout_baseline->standalone_label_&&
        !node.layout_baseline->button_,"Standalone Paint getter actual initialized standalone receiver required");
    const auto label=node.layout_baseline->standalone_label_;first();label->impl_->first();
    const auto run=label->impl_->completed_standalone_run;
    require(run&&run->impl_&&run->impl_->published,
        "Standalone Paint getter genuine whole slot10/published Run producer missing");
    const auto guard=run->capture_guard_for_paint_owned(operation,tree,id,actual_frame);
    const auto current=[&]{first();guard.require_current_without_callbacks();
        const auto& after=Core::node(owner->nodes,id);
        require(after.layout_baseline&&after.layout_baseline->standalone_label_.get()==label.get()&&
            !after.layout_baseline->standalone_label_.owner_before(label)&&!label.owner_before(after.layout_baseline->standalone_label_)&&
            label->impl_->completed_standalone_run.get()==run.get()&&
            !label->impl_->completed_standalone_run.owner_before(run)&&!run.owner_before(label->impl_->completed_standalone_run),
            "Standalone Paint getter actual child/published DATA changed");guard.require_current_without_callbacks();};
    current();frame.require_source_owned(operation);current();guard.require_source_owned(operation);current();return run;
}
