// Ordinary children painted under the genuine whole Root Context exclusion.
// The actual receiver is a Control; the parent Entry is lexical custody only.
ComposedUiFrame BnwWorldViewControlRenderPublication::context_child_frame_before_constructor_owned(
    const std::shared_ptr<BnwWorldViewControlTree>& tree,
    const std::optional<detail::BnwNormalRootContextEntry>& entry,ComposedUiFrame frame,
    const std::shared_ptr<const detail::BnwNormalLabelPaintEmission>& emission){
    require(bool(entry),"Context child frame requires genuine enclosing Entry");
    entry->require_current_without_callbacks();
    require(bool(tree),"Context child frame requires actual Tree");
    entry->require_same_tree_without_callbacks(*tree);tree->core_->layout_origin(true);
    auto result=emission?emission->frame_origin_for_publication_owned():std::move(frame);
    entry->require_current_without_callbacks();entry->require_same_tree_without_callbacks(*tree);
    tree->core_->layout_origin(true);return result;
}

BnwWorldViewControlRenderPublication::BnwWorldViewControlRenderPublication(
    const UiScreenGlRenderer& renderer,std::weak_ptr<const void> life,
    std::shared_ptr<BnwWorldViewControlTree> tree,BnwWorldViewControlTree::Control control,
    detail::BnwNormalRootContextEntry entry,const BnwOwnedCameraGuiScreen::Loan& gui,
    const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,ComposedUiFrame frame,
    std::vector<std::size_t> indices,std::shared_ptr<const detail::BnwNormalLabelPaintEmission> emission):
    renderer_(&renderer),renderer_life_(std::move(life)),tree_(std::move(tree)),control_(std::move(control)),
    enclosing_context_entry_(std::move(entry)),gui_(gui),window_(window),
    frame_(context_child_frame_before_constructor_owned(tree_,enclosing_context_entry_,std::move(frame),emission)),
    paint_indices_(std::move(indices)),normal_label_emission_(std::move(emission)),
    topology_revision_(context_receiver_revision_without_callbacks(tree_,enclosing_context_entry_,false)),
    effect_revision_(context_receiver_revision_without_callbacks(tree_,enclosing_context_entry_,true)){
    require(control_&&!root_context_entry_&&enclosing_context_entry_&&!renderer_life_.expired(),
        "Context child Publication requires genuine ordinary Control and alive parent Entry");
    const auto pure=[&]{enclosing_context_entry_->require_current_without_callbacks();
        enclosing_context_entry_->require_same_tree_without_callbacks(*tree_);
        (void)actual_checked_control_id_for_paint_without_callbacks();};
    pure();const auto& operation=tree_->core_->source->current_operation_loan();pure();
    enclosing_context_entry_->require_source_owned(operation);pure();
    gui_.require_current();pure();window_.require_current();pure();
    std::set<std::size_t> seen;
    require(!paint_indices_.empty(),"Context child genuine paint span unavailable");
    for(auto index:paint_indices_)require(index<frame_.paints.size()&&seen.insert(index).second,
        "Context child genuine paint span invalid");
    pure();
    if(normal_label_emission_)normal_label_emission_->require_publication_owned(*this,operation);
    pure();
}

std::shared_ptr<const BnwWorldViewControlRenderPublication>
BnwWorldViewControlRenderPublication::capture_context_child_original_owned(const OriginalOwnerIssuerKey&,
    const UiScreenGlRenderer& renderer,std::weak_ptr<const void> renderer_life,
    std::shared_ptr<BnwWorldViewControlTree> actual_tree,BnwWorldViewControlTree::Control actual_control,
    const detail::BnwNormalRootContextEntry& actual_entry,const BnwOwnedCameraGuiScreen::Loan& gui,
    const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,ComposedUiFrame frame,
    std::vector<std::size_t> indices,std::shared_ptr<const detail::BnwNormalLabelPaintEmission> emission){
    actual_entry.require_current_without_callbacks();
    const auto entry=actual_entry;const auto tree=std::move(actual_tree);
    require(tree&&!renderer_life.expired(),"Context child factory requires actual Tree/renderer");
    entry.require_same_tree_without_callbacks(*tree);
    const auto core=tree->core_;core->layout_origin(true);
    const auto source=core->source;const auto& operation=source->current_operation_loan();
    operation.require_origin_without_callbacks();const auto serial=operation.operation_serial();
    const auto topology=core->topology_revision,effects=core->effect_revision;
    const auto id=core->checked(actual_control);
    const auto pure=[&]{entry.require_current_without_callbacks();entry.require_same_tree_without_callbacks(*tree);
        require(tree->core_==core&&core->source==source&&!renderer_life.expired(),
            "Context child factory lost SAME Tree/Core/Source/renderer");
        core->layout_origin(true);const auto& current=source->current_operation_loan();
        require(&current==&operation,"Context child factory Source replaced before saved operand");
        current.require_origin_without_callbacks();
        require(current.operation_serial()==serial&&core->topology_revision==topology&&
            core->effect_revision==effects&&core->checked(actual_control)==id,
            "Context child factory immutable actual Source/control tuple changed");};
    const auto full=[&]{pure();entry.require_source_owned(operation);pure();
        gui.require_current();pure();window.require_current();pure();};
    try{
        pure();full();pure();
        auto owned=std::shared_ptr<BnwWorldViewControlRenderPublication>(
            new BnwWorldViewControlRenderPublication(renderer,renderer_life,tree,actual_control,entry,
                gui,window,std::move(frame),std::move(indices),std::move(emission)));
        pure();full();pure();
        require(owned&&owned->creator_==std::this_thread::get_id()&&owned->renderer_==&renderer&&
            owned->tree_==tree&&owned->control_&&!owned->root_context_entry_&&owned->enclosing_context_entry_&&
            owned->topology_revision_==topology&&owned->effect_revision_==effects&&
            owned->original_owner_.expired()&&!owned->original_owner_operation_&&!owned->original_owner_source_serial_,
            "Context child newly created canonical allocation differs");
        owned->enclosing_context_entry_->require_same_entry_without_callbacks(entry);
        require(owned->actual_checked_control_id_for_paint_without_callbacks()==id,
            "Context child new original Publication Control substituted");
        owned->original_owner_operation_=&operation;owned->original_owner_source_serial_=serial;
        owned->original_owner_=owned;
        pure();full();pure();
        const auto canonical=owned->original_owner_before_paint_without_callbacks();
        require(canonical.get()==owned.get()&&!canonical.owner_before(owned)&&!owned.owner_before(canonical),
            "Context child canonical shared control differs at factory return");
        pure();return canonical;
    }catch(...){core->failed.store(true);throw;}
}

std::optional<detail::BnwNormalRootContextEntry>
BnwWorldViewControlRenderPublication::actual_enclosing_root_context_entry_without_callbacks()const{
    require_actual_receiver_for_paint_without_callbacks();
    const auto actual=enclosing_context_entry_;
    require(!actual||(!root_context_entry_&&control_),
        "Enclosing Context belongs only to a genuine ordinary child receiver");
    require_actual_receiver_for_paint_without_callbacks();
    return actual; // SAME immutable Entry value; never mint a Scope or receiver.
}
