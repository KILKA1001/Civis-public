// Distinct indexed caption publication. Root22 standalone constructors and
// bodies remain byte-identical. Frame authority comes only from SAME actual
// Ui postappend shared carrier, owned before original Label Frame issuance.
ComposedUiFrame BnwWorldViewControlRenderPublication::context_caption_frame_before_constructor_owned(
    const std::shared_ptr<BnwWorldViewControlTree>& tree,const BnwWorldViewControlTree::Control& control,
    const std::optional<detail::BnwNormalRootContextEntry>& entry,ComposedUiFrame legacy_frame,
    const std::shared_ptr<const detail::BnwNormalButtonCaptionPaintEmission>& emission){
    require(entry&&emission,"Caption178 original Publication lacks genuine Entry/emission");
    BnwWorldViewControlTree::require_normal_button_caption_publication_inputs_without_callbacks(
        *emission,tree,control,*entry);
    const auto& actual=emission->frame_origin_for_publication_owned();
    // The caller value is legacy bookkeeping only. A frame copy is never an
    // issuer; actual adjusted authority below remains SAME retained origin.
    require(legacy_frame.frame_owner==actual.frame_owner&&legacy_frame.frame_sequence==actual.frame_sequence&&
        legacy_frame.snapshot_frame==actual.snapshot_frame&&legacy_frame.paints.size()==actual.paints.size(),
        "Caption178 legacy frame bookkeeping differs from actual owned append");
    ComposedUiFrame result=actual;
    BnwWorldViewControlTree::require_normal_button_caption_publication_inputs_without_callbacks(
        *emission,tree,control,*entry);
    return result;
}

BnwWorldViewControlRenderPublication::BnwWorldViewControlRenderPublication(
    const UiScreenGlRenderer& renderer,std::weak_ptr<const void> life,
    std::shared_ptr<BnwWorldViewControlTree> tree,BnwWorldViewControlTree::Control control,
    detail::BnwNormalRootContextEntry entry,const BnwOwnedCameraGuiScreen::Loan& gui,
    const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,ComposedUiFrame frame,
    std::vector<std::size_t> indices,std::shared_ptr<const detail::BnwNormalButtonCaptionPaintEmission> emission)
    :renderer_(&renderer),renderer_life_(std::move(life)),tree_(std::move(tree)),control_(std::move(control)),
     enclosing_context_entry_(std::move(entry)),gui_(gui),window_(window),
     frame_(context_caption_frame_before_constructor_owned(tree_,*control_,enclosing_context_entry_,std::move(frame),emission)),
     paint_indices_(std::move(indices)),normal_caption_emission_(std::move(emission)),
     topology_revision_(context_receiver_revision_without_callbacks(tree_,enclosing_context_entry_,false)),
     effect_revision_(context_receiver_revision_without_callbacks(tree_,enclosing_context_entry_,true)){
    require(control_&&!root_context_entry_&&enclosing_context_entry_&&normal_caption_emission_&&
        !normal_label_emission_&&!context_child_body_&&!renderer_life_.expired()&&paint_indices_.size()==1,
        "Caption178 original Publication requires real one-item span and distinct typed caption emission");
    const auto pure=[&]{BnwWorldViewControlTree::require_normal_button_caption_publication_inputs_without_callbacks(
        *normal_caption_emission_,tree_,*control_,*enclosing_context_entry_);};
    pure();const auto& op=tree_->core_->source->current_operation_loan();pure();
    BnwWorldViewControlTree::require_normal_button_caption_emission_source_owned(op,*normal_caption_emission_);pure();
    gui_.require_current();pure();window_.require_current();pure();
    require(paint_indices_[0]<frame_.paints.size(),"Caption178 actual append index is outside original carrier");
    normal_caption_emission_->require_publication_owned(*this,op);pure();
}

std::shared_ptr<const BnwWorldViewControlRenderPublication>
BnwWorldViewControlRenderPublication::capture_context_child_original_owned(const OriginalOwnerIssuerKey&,
    const UiScreenGlRenderer& renderer,std::weak_ptr<const void> renderer_life,
    std::shared_ptr<BnwWorldViewControlTree> actual_tree,BnwWorldViewControlTree::Control actual_control,
    const detail::BnwNormalRootContextEntry& actual_entry,const BnwOwnedCameraGuiScreen::Loan& gui,
    const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,ComposedUiFrame frame,
    std::vector<std::size_t> indices,std::shared_ptr<const detail::BnwNormalButtonCaptionPaintEmission> actual_emission){
    // Owned originals precede every callbackful Source/Gui/window observation.
    const auto entry=actual_entry;const auto control=actual_control;
    const auto tree=std::move(actual_tree);const auto emission=std::move(actual_emission);
    const auto life=renderer_life;
    require(emission&&tree&&!life.expired(),"Caption178 factory lacks actual owned append/Tree/renderer");
    BnwWorldViewControlTree::require_normal_button_caption_publication_inputs_without_callbacks(*emission,tree,control,entry);
    const auto core=tree->core_;const auto source=core->source;const auto& op=source->current_operation_loan();
    op.require_origin_without_callbacks();const auto serial=op.operation_serial();
    const auto topology=core->topology_revision,effects=core->effect_revision;
    const auto id=core->checked(control);
    const auto pure=[&]{BnwWorldViewControlTree::require_normal_button_caption_publication_inputs_without_callbacks(
        *emission,tree,control,entry);
        require(tree->core_==core&&core->source==source&&!life.expired(),
            "Caption178 factory lost SAME actual Tree/Core/Source/renderer");
        const auto& current=source->current_operation_loan();
        require(&current==&op,"Caption178 factory Source differs before saved operand");
        current.require_origin_without_callbacks();
        require(current.operation_serial()==serial&&core->topology_revision==topology&&core->effect_revision==effects&&
            core->checked(control)==id,"Caption178 factory immutable actual tuple changed");};
    const auto full=[&]{pure();BnwWorldViewControlTree::require_normal_button_caption_emission_source_owned(op,*emission);pure();
        gui.require_current();pure();window.require_current();pure();};
    try{
        pure();full();pure();
        auto owned=std::shared_ptr<BnwWorldViewControlRenderPublication>(new BnwWorldViewControlRenderPublication(
            renderer,life,tree,control,entry,gui,window,std::move(frame),std::move(indices),emission));
        pure();full();pure();
        require(owned&&owned->creator_==std::this_thread::get_id()&&owned->renderer_==&renderer&&owned->tree_==tree&&
            owned->control_&&!owned->root_context_entry_&&owned->enclosing_context_entry_&&
            owned->normal_caption_emission_==emission&&!owned->normal_label_emission_&&!owned->context_child_body_&&
            owned->topology_revision_==topology&&owned->effect_revision_==effects&&
            owned->original_owner_.expired()&&!owned->original_owner_operation_&&!owned->original_owner_source_serial_,
            "Caption178 new original Publication allocation/association differs");
        owned->enclosing_context_entry_->require_same_entry_without_callbacks(entry);
        require(owned->actual_checked_control_id_for_paint_without_callbacks()==id,
            "Caption178 original Publication parent Control substituted");
        owned->original_owner_operation_=&op;owned->original_owner_source_serial_=serial;owned->original_owner_=owned;
        pure();full();pure();
        const auto canonical=owned->original_owner_before_paint_without_callbacks();
        require(canonical.get()==owned.get()&&!canonical.owner_before(owned)&&!owned.owner_before(canonical),
            "Caption178 original Publication canonical shared control differs");
        emission->require_publication_owned(*owned,op);pure();return canonical;
    }catch(...){core->failed.store(true);throw;}
}

UiGlPaintReport BnwWorldViewControlRenderPublication::paint_context_caption_owned(UiScreenGlRenderer& renderer)const{
    const auto emission=normal_caption_emission_;
    require(bool(emission),"Caption178 paint lacks genuine original emission");
    BnwWorldViewControlTree::require_normal_button_caption_emission_current_without_callbacks(*emission);
    const auto canonical=original_owner_before_paint_without_callbacks();
    require(canonical&&canonical.get()==this&&renderer_==&renderer&&control_&&!root_context_entry_&&enclosing_context_entry_&&
        !context_child_body_&&!normal_label_emission_&&!adjusted_frame_painting_&&!adjusted_frame_owner_&&
        !adjusted_frame_operation_&&!adjusted_frame_source_serial_,
        "Caption178 original paint replay/reentry/receiver substitution");
    const auto tree=tree_;const auto core=tree->core_;const auto source=core->source;
    const auto& op=source->current_operation_loan();const auto serial=op.operation_serial();
    const auto frame=emission->impl_->origin;
    BnwWorldViewControlTree::require_normal_button_caption_emission_publication_owned(*emission,*this,op);
    adjusted_frame_painting_=true;
    struct Exit final {const std::shared_ptr<const BnwWorldViewControlRenderPublication> original;
        bool& painting;~Exit(){painting=false;}}exit{canonical,adjusted_frame_painting_};
    const auto pure=[&]{BnwWorldViewControlTree::require_normal_button_caption_emission_current_without_callbacks(*emission);
        const auto original=original_owner_without_callbacks();
        require(original.get()==canonical.get()&&!original.owner_before(canonical)&&!canonical.owner_before(original)&&
            normal_caption_emission_==emission&&tree_==tree&&tree->core_==core&&core->source==source&&
            core->busy&&!core->failed.load()&&renderer_==&renderer&&!renderer_life_.expired()&&adjusted_frame_painting_,
            "Caption178 SAME original child paint and enclosing exclusion changed");
        const auto& current=source->current_operation_loan();require(&current==&op,
            "Caption178 paint fresh Source differs before saved operand");
        current.require_origin_without_callbacks();require(current.operation_serial()==serial,
            "Caption178 paint immutable Source serial changed");
        if(adjusted_frame_owner_)require_adjusted_frame_owner_without_callbacks(frame);
        BnwWorldViewControlTree::require_normal_button_caption_emission_current_without_callbacks(*emission);};
    const auto full=[&]{pure();emission->require_publication_owned(*this,op);pure();
        gui_.require_current();pure();window_.require_current();pure();};
    try{
        pure();full();pure();
        // SAME original owned Ui postappend object, before any Label Frame.
        // No second carrier/new Frame, no per-caption Prefix/Matrix epoch.
        adjusted_frame_operation_=&op;adjusted_frame_source_serial_=serial;adjusted_frame_owner_=frame;
        pure();full();pure();require_adjusted_frame_owner_without_callbacks(frame);
        const auto width=gui_.width();pure();const auto height=gui_.height();pure();full();pure();
        auto report=renderer.paint_worldview_control_publication(*this,*frame,width,height);
        pure();full();pure();return report;
    }catch(...){core->failed.store(true);throw;}
}

std::shared_ptr<const detail::BnwPositionedLabelRunLoan>
BnwWorldViewControlRenderPublication::caption_run_for_actual_item_owned(std::size_t index,
    const runtime::BnwWorldViewSourceOperationLoan& op,const detail::BnwLabelActualPaintCallFrame& frame)const{
    require(bool(normal_caption_emission_),"Caption178 Run requires distinct genuine typed emission");
    return normal_caption_emission_->run_for_actual_item_owned(*this,index,op,frame);
}
