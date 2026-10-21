// Closed original Publication for an actual initialized Context. No ordinary
// Control or indexed paint item is manufactured for the root receiver.
std::uint64_t BnwWorldViewControlRenderPublication::context_receiver_revision_without_callbacks(
    const std::shared_ptr<BnwWorldViewControlTree>& tree,
    const std::optional<detail::BnwNormalRootContextEntry>& entry,bool effect){
    require(bool(entry),"Context revision requires genuine closed Entry");
    entry->require_current_without_callbacks();
    require(bool(tree),"Context revision requires actual Tree");
    entry->require_same_tree_without_callbacks(*tree);
    const auto core=tree->core_;core->layout_origin(true);
    const auto revision=effect?core->effect_revision:core->topology_revision;
    entry->require_current_without_callbacks();entry->require_same_tree_without_callbacks(*tree);
    core->layout_origin(true);return revision;
}

BnwWorldViewControlRenderPublication::BnwWorldViewControlRenderPublication(
    const UiScreenGlRenderer& renderer,std::weak_ptr<const void> life,
    std::shared_ptr<BnwWorldViewControlTree> tree,detail::BnwNormalRootContextEntry entry,
    const BnwOwnedCameraGuiScreen::Loan& gui,
    const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,ComposedUiFrame frame):
    renderer_(&renderer),renderer_life_(std::move(life)),tree_(std::move(tree)),
    root_context_entry_(std::move(entry)),gui_(gui),window_(window),frame_(std::move(frame)),
    topology_revision_(context_receiver_revision_without_callbacks(tree_,root_context_entry_,false)),
    effect_revision_(context_receiver_revision_without_callbacks(tree_,root_context_entry_,true)){
    require(tree_&&!renderer_life_.expired()&&!control_&&root_context_entry_&&
        paint_indices_.empty()&&!normal_label_emission_&&frame_.paints.empty(),
        "Context Publication requires genuine Root entry and no indexed control paints");
    root_context_entry_->require_current_without_callbacks();
    root_context_entry_->require_same_tree_without_callbacks(*tree_);
    const auto& current=tree_->core_->source->current_operation_loan();
    const auto pure=[&]{root_context_entry_->require_current_without_callbacks();
        root_context_entry_->require_same_tree_without_callbacks(*tree_);
        tree_->core_->layout_origin(true);
        require(&tree_->core_->source->current_operation_loan()==&current,
            "Context Publication constructor Source changed before saved operand");};
    pure();root_context_entry_->require_source_owned(current);pure();
    gui_.require_current();pure();window_.require_current();pure();
    root_context_entry_->require_source_owned(current);pure();
}

void BnwWorldViewControlRenderPublication::require_actual_receiver_for_paint_without_callbacks()const{
    require(control_.has_value()!=root_context_entry_.has_value(),
        "Original Publication requires exactly one genuine receiver kind");
    if(root_context_entry_){
        root_context_entry_->require_current_without_callbacks();
        require(creator_==std::this_thread::get_id()&&renderer_&&tree_&&!renderer_life_.expired()&&
            paint_indices_.empty()&&!normal_label_emission_,
            "Context Publication physical renderer/Tree owner retired or substituted");
        root_context_entry_->require_same_tree_without_callbacks(*tree_);
        tree_->core_->layout_origin(true);
        require(tree_->core_->topology_revision==topology_revision_&&
            tree_->core_->effect_revision==effect_revision_,
            "Context Publication original receiver revisions changed");
        root_context_entry_->require_current_without_callbacks();
    }else{
        (void)actual_checked_control_id_for_paint_without_callbacks();
    }
}

std::optional<detail::BnwNormalRootContextEntry>
BnwWorldViewControlRenderPublication::actual_root_context_entry_for_paint_without_callbacks()const{
    require_actual_receiver_for_paint_without_callbacks();
    const auto actual=root_context_entry_;
    require_actual_receiver_for_paint_without_callbacks();
    return actual; //Copy SAME closed allocation, never issue a new entry.
}

std::shared_ptr<const BnwWorldViewControlRenderPublication>
BnwWorldViewControlRenderPublication::capture_context_original_owned(const OriginalOwnerIssuerKey&,
    const UiScreenGlRenderer& renderer,std::weak_ptr<const void> renderer_life,
    std::shared_ptr<BnwWorldViewControlTree> actual_tree,detail::BnwNormalRootContextEntry actual_entry,
    const BnwOwnedCameraGuiScreen::Loan& gui,const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,
    ComposedUiFrame frame){
    const auto tree=std::move(actual_tree);
    const auto entry=std::move(actual_entry);
    entry.require_current_without_callbacks();
    require(tree&&!renderer_life.expired(),"Context original factory requires actual Tree/renderer");
    entry.require_same_tree_without_callbacks(*tree);
    const auto core=tree->core_;const auto source=core->source;
    const auto& operation=source->current_operation_loan();
    operation.require_origin_without_callbacks();const auto serial=operation.operation_serial();
    const auto topology=core->topology_revision,effects=core->effect_revision;
    const auto pure=[&]{
        entry.require_current_without_callbacks();entry.require_same_tree_without_callbacks(*tree);
        require(tree->core_==core&&core->source==source&&!renderer_life.expired(),
            "Context original factory lost SAME Tree/Core/Source/renderer");
        core->layout_origin(true);const auto& current=source->current_operation_loan();
        require(&current==&operation,"Context original factory Source replaced before saved operand");
        current.require_origin_without_callbacks();
        require(current.operation_serial()==serial&&core->topology_revision==topology&&core->effect_revision==effects,
            "Context original factory immutable Source/receiver revisions changed");
    };
    const auto full=[&]{pure();entry.require_source_owned(operation);pure();
        gui.require_current();pure();window.require_current();pure();};
    try{
        pure();full();pure();
        auto owned=std::shared_ptr<BnwWorldViewControlRenderPublication>(
            new BnwWorldViewControlRenderPublication(renderer,renderer_life,tree,entry,gui,window,std::move(frame)));
        pure();full();pure();
        require(owned&&owned->creator_==std::this_thread::get_id()&&owned->renderer_==&renderer&&
            owned->tree_==tree&&!owned->control_&&owned->root_context_entry_&&
            owned->topology_revision_==topology&&owned->effect_revision_==effects&&
            owned->original_owner_.expired()&&!owned->original_owner_operation_&&!owned->original_owner_source_serial_,
            "Context original factory newly created allocation/control differs");
        owned->root_context_entry_->require_same_entry_without_callbacks(entry);
        pure();
        owned->original_owner_operation_=&operation;owned->original_owner_source_serial_=serial;
        owned->original_owner_=owned;
        pure();full();pure();
        const auto canonical=owned->original_owner_without_callbacks();
        require(canonical.get()==owned.get()&&!canonical.owner_before(owned)&&!owned.owner_before(canonical),
            "Context original factory canonical shared owner differs");
        pure();return canonical;
    }catch(...){core->failed.store(true);throw;}
}

UiGlPaintReport BnwWorldViewControlRenderPublication::paint_context_root_owned(UiScreenGlRenderer& renderer)const{
    const auto canonical_owner=original_owner_without_callbacks();
    require(canonical_owner&&canonical_owner.get()==this,
        "Context paint requires SAME canonical original Publication control");
    require_actual_receiver_for_paint_without_callbacks();
    require(root_context_entry_&&!control_&&renderer_==&renderer&&
        !adjusted_frame_painting_&&!adjusted_frame_owner_&&!adjusted_frame_operation_&&!adjusted_frame_source_serial_,
        "Context original paint is replayed/reentered or uses another renderer");
    const auto entry=*root_context_entry_;
    const auto tree=tree_;const auto core=tree->core_;const auto source=core->source;
    const auto& operation=source->current_operation_loan();
    operation.require_origin_without_callbacks();const auto serial=operation.operation_serial();
    adjusted_frame_painting_=true;
    struct Exit final {
        std::shared_ptr<const BnwWorldViewControlRenderPublication> owner;
        bool& painting;~Exit(){painting=false;}
    }exit{canonical_owner,adjusted_frame_painting_};
    const auto pure=[&,canonical_owner]{
        entry.require_current_without_callbacks();entry.require_same_tree_without_callbacks(*tree);
        const auto current_owner=original_owner_without_callbacks();
        require(current_owner.get()==canonical_owner.get()&&!current_owner.owner_before(canonical_owner)&&
            !canonical_owner.owner_before(current_owner),"Context paint original control was substituted");
        require(root_context_entry_&&!control_&&tree_==tree&&tree->core_==core&&core->source==source&&
            adjusted_frame_painting_&&renderer_==&renderer&&!renderer_life_.expired(),
            "Context original adjusted-frame paint custody changed");
        entry.require_same_entry_without_callbacks(*root_context_entry_);
        const auto& current=source->current_operation_loan();
        require(&current==&operation,"Context original paint Source replaced before saved operand");
        current.require_origin_without_callbacks();
        require(current.operation_serial()==serial,"Context original paint Source serial changed");
        require_actual_receiver_for_paint_without_callbacks();
        if(adjusted_frame_owner_)require_adjusted_frame_owner_without_callbacks(adjusted_frame_owner_);
    };
    const auto full=[&]{pure();entry.require_source_owned(operation);pure();
        gui_.require_current();pure();window_.require_current();pure();};
    try{
        pure();full();pure();
        auto adjusted=std::make_shared<ComposedUiFrame>(frame_);
        pure();full();pure();
        require(adjusted&&adjusted->paints.empty(),"Context Root must not substitute an indexed paint span");
        std::shared_ptr<const ComposedUiFrame> immutable=std::move(adjusted);
        require(immutable&&!adjusted,"Context adjusted-frame immutable transfer was not earned");
        adjusted_frame_operation_=&operation;adjusted_frame_source_serial_=serial;
        adjusted_frame_owner_=std::move(immutable);
        pure();full();pure();
        const auto width=gui_.width();pure();const auto height=gui_.height();pure();
        const auto actual=adjusted_frame_owner_;
        require_adjusted_frame_owner_without_callbacks(actual);
        auto result=renderer.paint_worldview_control_publication(*this,*actual,width,height);
        pure();full();pure();
        return result;
    }catch(...){core->failed.store(true);throw;}
}
