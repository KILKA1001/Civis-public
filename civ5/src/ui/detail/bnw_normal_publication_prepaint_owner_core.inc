// The ordinary factory runs before paint. Its canonical owner must be checked
// in that exact phase, rather than asking an active-paint accessor to accept it.
std::shared_ptr<const BnwWorldViewControlRenderPublication>
BnwWorldViewControlRenderPublication::original_owner_before_paint_without_callbacks()const{
    if(enclosing_context_entry_)enclosing_context_entry_->require_current_without_callbacks();
    require(control_&&!root_context_entry_,"Prepaint owner requires its genuine ordinary Control");
    require(creator_==std::this_thread::get_id()&&renderer_&&tree_&&!renderer_life_.expired()&&
        !adjusted_frame_painting_&&!adjusted_frame_owner_&&!adjusted_frame_operation_&&!adjusted_frame_source_serial_,
        "Ordinary original owner is outside its prepaint phase or has retired");
    if(enclosing_context_entry_)enclosing_context_entry_->require_same_tree_without_callbacks(*tree_);
    const bool entered=enclosing_context_entry_.has_value();
    const auto core=tree_->core_;core->layout_origin(entered);
    const auto& fresh=core->source->current_operation_loan();
    require(original_owner_operation_ && &fresh==original_owner_operation_,
        "Prepaint owner Source changed before saved operand");
    fresh.require_origin_without_callbacks();
    const auto canonical=original_owner_.lock();
    require(canonical&&canonical.get()==this&&original_owner_source_serial_&&
        fresh.operation_serial()==*original_owner_source_serial_&&
        core->topology_revision==topology_revision_&&core->effect_revision==effect_revision_,
        "Prepaint canonical original allocation or Source/revisions changed");
    require(core->checked(*control_)==control_->id_,"Prepaint owner has another actual Control");
    if(enclosing_context_entry_){enclosing_context_entry_->require_current_without_callbacks();
        enclosing_context_entry_->require_same_tree_without_callbacks(*tree_);}
    core->layout_origin(entered);
    require(&core->source->current_operation_loan()==original_owner_operation_,
        "Prepaint owner fresh Source changed before return");
    fresh.require_origin_without_callbacks();
    return canonical;
}
