// Control-only child body under ONE genuine entered Root. The indexed Root22
// constructor/factory is not used or weakened. This body has no item/index.
struct BnwWorldViewControlTree::NormalContextChildBodyPublicationState final {
    using Call=detail::BnwNormalCommonChildCall;
    using Execution=detail::BnwNormalRootBatchExecution;
    using History=detail::BnwNormalRootBatchHistory;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const Call::Impl> original_call;
    const std::weak_ptr<Execution::Impl> original_execution;
    const std::weak_ptr<const History::Impl> original_history;
    const std::weak_ptr<BnwWorldViewControlTree> original_tree;
    const Control control;
    const detail::BnwNormalRootContextEntry entry;
    const std::shared_ptr<const BnwWorldViewControlRenderPublication> parent_publication;
    const std::shared_ptr<const ComposedUiFrame> whole_frame;
    const UiScreenGlRenderer*const renderer;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t serial,topology,effects;
    const Call::Phase phase;
    mutable std::weak_ptr<const NormalContextChildBodyPublicationState> canonical;
    bool sealed=false;
private:
    friend class BnwWorldViewControlTree;
    NormalContextChildBodyPublicationState(std::weak_ptr<const Call::Impl> call,
        std::weak_ptr<Execution::Impl> execution,std::weak_ptr<const History::Impl> history,
        std::weak_ptr<BnwWorldViewControlTree> tree,Control actual_control,
        detail::BnwNormalRootContextEntry actual_entry,
        std::shared_ptr<const BnwWorldViewControlRenderPublication> actual_parent,
        std::shared_ptr<const ComposedUiFrame> actual_frame,const UiScreenGlRenderer* actual_renderer,
        const runtime::BnwWorldViewSourceOperationLoan* actual_operation,std::uint64_t actual_serial,
        std::uint64_t actual_topology,std::uint64_t actual_effects,Call::Phase actual_phase)
        :original_call(std::move(call)),original_execution(std::move(execution)),
         original_history(std::move(history)),original_tree(std::move(tree)),control(std::move(actual_control)),
         entry(std::move(actual_entry)),parent_publication(std::move(actual_parent)),whole_frame(std::move(actual_frame)),
         renderer(actual_renderer),operation(actual_operation),serial(actual_serial),topology(actual_topology),
         effects(actual_effects),phase(actual_phase){}
public:
    void first()const{
        const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
        const auto actual_owner=canonical.lock();
        require(creator==std::this_thread::get_id()&&sealed&&actual_owner&&actual_owner.get()==this,
            "Child body original state is unsealed/retired/foreign before Call borrow");
        const auto call=original_call.lock();
        require(bool(call),"Child body original CURRENT CommonCall retired");
        // Read the SAME actual original impl; never construct a Call facade/key.
        NormalCommonTraversal::require_current_call_impl_without_callbacks(call);
        require(call->kind==Call::Kind::button&&call->phase==phase&&phase==Call::Phase::button_own118,
            "Control-only body is not the genuine CURRENT Button own118 call");
        const auto traversal=call->traversal.lock();
        const auto execution=original_execution.lock();const auto history=original_history.lock();
        require(traversal&&execution&&history&&same(traversal->execution.lock(),execution)&&
            same(execution->root.impl_,history),"Child body original Root/Execution association changed");
        execution->first();history->first();
        const auto tree=original_tree.lock();
        require(tree&&same(history->tree,tree)&&history->renderer==renderer&&history->operation==operation&&
            history->serial==serial&&history->prefix->context_entry&&
            same(history->publication,parent_publication)&&same(history->adjusted_frame,whole_frame),
            "Child body lost SAME actual Root Tree/renderer/Source/whole frame");
        entry.require_current_without_callbacks();entry.require_same_tree_without_callbacks(*tree);
        entry.require_same_entry_without_callbacks(traversal->entry);
        entry.require_same_entry_without_callbacks(*history->prefix->context_entry);
        const auto core=tree->core_;core->layout_origin(true);
        require(core->topology_revision==topology&&core->effect_revision==effects,
            "Child body original receiver revisions changed");
        const auto candidate_owner=control.owner_.lock();const auto call_owner=call->control.owner_.lock();
        require(candidate_owner&&call_owner&&same(candidate_owner,core)&&same(call_owner,core)&&
            core->checked(control)==control.id_&&control.id_==call->control.id_,
            "Child body does not hold SAME actual CURRENT Control");
        // Strong originals precede these reads. Root Entry owns the inherited
        // lexical Gui/window lifetime; these owners never renew that Scope.
        require(parent_publication&&whole_frame&&parent_publication->renderer_==renderer&&
            parent_publication->tree_==tree&&parent_publication->root_context_entry_&&
            !parent_publication->control_&&parent_publication->paint_indices_.empty()&&
            !parent_publication->normal_label_emission_,"Child body original parent is not genuine no-index Root");
        parent_publication->root_context_entry_->require_same_entry_without_callbacks(entry);
        const auto original=parent_publication->original_owner_without_callbacks();
        require(same(original,parent_publication),"Child body parent original shared control substituted");
        parent_publication->require_adjusted_frame_owner_without_callbacks(whole_frame);
        const auto& fresh=core->source->current_operation_loan();
        require(&fresh==operation,"Child body Source replaced before saved operation borrower");
        fresh.require_origin_without_callbacks();
        require(fresh.operation_serial()==serial,"Child body original Source serial changed");
        history->first();execution->first();
        NormalCommonTraversal::require_current_call_impl_without_callbacks(call);
        require(call->phase==phase&&same(original_call.lock(),call)&&
            same(original_execution.lock(),execution)&&same(original_history.lock(),history)&&
            same(history->publication,parent_publication)&&same(history->adjusted_frame,whole_frame),
            "Child body original CURRENT/Root/whole owner changed at PURE POST");
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& supplied)const{
        first();require(&supplied==operation,"Child body supplied another actual Source");
        const auto execution=original_execution.lock();
        require(bool(execution),"Child body original Execution retired before Source FULL");
        execution->full(supplied);first();entry.require_source_owned(supplied);first();
    }
};

std::shared_ptr<const BnwWorldViewControlTree::NormalContextChildBodyPublicationState>
BnwWorldViewControlTree::capture_normal_context_child_body_publication_state_owned(
    const detail::BnwNormalCommonChildCall& actual_call,const std::shared_ptr<BnwWorldViewControlTree>& actual_tree,
    const Control& offered_control,const detail::BnwNormalRootContextEntry& offered_entry,
    const UiScreenGlRenderer& renderer,const std::shared_ptr<const ComposedUiFrame>& actual_frame){
    actual_call.require_current_without_callbacks();
    require(actual_call.matches_actual_control_without_callbacks(offered_control),
        "Child body capture Control differs from genuine CURRENT Call");
    const auto call=actual_call.impl_;const auto control=offered_control;const auto entry=offered_entry;
    const auto tree=actual_tree;const auto frame=actual_frame;
    require(tree&&frame,"Child body capture requires actual Tree and strong original whole frame");
    entry.require_current_without_callbacks();entry.require_same_tree_without_callbacks(*tree);
    const auto traversal=call->traversal.lock();const auto execution=traversal->execution.lock();
    const auto history=execution->root.impl_;execution->first();history->first();
    const auto parent=history->publication;const auto*const operation=history->operation;
    const auto serial=history->serial,topology=tree->core_->topology_revision,effects=tree->core_->effect_revision;
    const auto phase=call->phase;
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto pure=[&]{
        NormalCommonTraversal::require_current_call_impl_without_callbacks(call);
        execution->first();history->first();entry.require_current_without_callbacks();
        entry.require_same_tree_without_callbacks(*tree);entry.require_same_entry_without_callbacks(traversal->entry);
        require(call->kind==detail::BnwNormalCommonChildCall::Kind::button&&
            phase==detail::BnwNormalCommonChildCall::Phase::button_own118&&call->phase==phase&&
            same(traversal->execution.lock(),execution)&&same(execution->root.impl_,history)&&same(history->tree,tree)&&
            history->renderer==&renderer&&history->operation==operation&&history->serial==serial&&
            same(history->publication,parent)&&same(history->adjusted_frame,frame),
            "Child body capture original CURRENT/Root/renderer/frame tuple changed");
        entry.require_same_entry_without_callbacks(*history->prefix->context_entry);
        const auto core=tree->core_;core->layout_origin(true);
        const auto candidate=control.owner_.lock();const auto actual=call->control.owner_.lock();
        require(candidate&&actual&&same(candidate,core)&&same(actual,core)&&
            core->checked(control)==call->control.id_&&core->topology_revision==topology&&core->effect_revision==effects,
            "Child body capture actual canonical Control or revisions changed");
        require(parent&&parent->root_context_entry_&&!parent->control_&&parent->renderer_==&renderer,
            "Child body capture parent must be genuine original Root Publication");
        parent->root_context_entry_->require_same_entry_without_callbacks(entry);
        const auto owner=parent->original_owner_without_callbacks();
        require(same(owner,parent),"Child body capture original parent control changed");
        parent->require_adjusted_frame_owner_without_callbacks(frame);
    };
    const auto full=[&]{pure();execution->full(*operation);pure();entry.require_source_owned(*operation);pure();};
    const auto core=tree->core_;
    try{
        pure();full();pure();
        auto owned=std::shared_ptr<NormalContextChildBodyPublicationState>(
            new NormalContextChildBodyPublicationState(call,execution,history,tree,control,entry,parent,frame,
                &renderer,operation,serial,topology,effects,phase));
        pure();full();pure();
        require(owned&&owned->canonical.expired()&&!owned->sealed,
            "Child body original closed state allocation was substituted");
        owned->canonical=owned;owned->sealed=true;
        owned->first();full();owned->first();
        std::shared_ptr<const NormalContextChildBodyPublicationState> immutable=std::move(owned);
        require(immutable&&!owned,"Child body original immutable state transfer unavailable");
        immutable->first();return immutable;
    }catch(...){core->failed.store(true);throw;}
}

ComposedUiFrame BnwWorldViewControlRenderPublication::context_child_body_frame_before_constructor_owned(
    const std::shared_ptr<const BnwWorldViewControlTree::NormalContextChildBodyPublicationState>& actual){
    require(bool(actual),"Child body constructor needs genuine closed admission");
    actual->first();actual->full(*actual->operation);actual->first();
    const auto frame=actual->whole_frame;
    require(bool(frame),"Child body constructor original whole frame unavailable");
    auto bookkeeping=*frame;
    actual->first();actual->full(*actual->operation);actual->first();
    return bookkeeping; // Not issued to renderer/Frame; SAME strong owner is authority.
}

BnwWorldViewControlRenderPublication::BnwWorldViewControlRenderPublication(const ContextChildBodyIssuerKey&,
    const UiScreenGlRenderer& renderer,std::weak_ptr<const void> life,
    std::shared_ptr<BnwWorldViewControlTree> tree,BnwWorldViewControlTree::Control control,
    detail::BnwNormalRootContextEntry entry,const BnwOwnedCameraGuiScreen::Loan& gui,
    const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,
    std::shared_ptr<const BnwWorldViewControlTree::NormalContextChildBodyPublicationState> body):
    renderer_(&renderer),renderer_life_(std::move(life)),tree_(std::move(tree)),control_(std::move(control)),
    enclosing_context_entry_(std::move(entry)),context_child_body_(std::move(body)),gui_(gui),window_(window),
    frame_(context_child_body_frame_before_constructor_owned(context_child_body_)),
    topology_revision_(context_receiver_revision_without_callbacks(tree_,enclosing_context_entry_,false)),
    effect_revision_(context_receiver_revision_without_callbacks(tree_,enclosing_context_entry_,true)){
    context_child_body_->first();
    require(control_&&!root_context_entry_&&enclosing_context_entry_&&!renderer_life_.expired()&&
        paint_indices_.empty()&&!normal_label_emission_&&renderer_==context_child_body_->renderer&&
        &gui_==&context_child_body_->parent_publication->gui_&&
        &window_==&context_child_body_->parent_publication->window_,
        "Child body Publication requires actual Control/Entry and SAME original Gui/window lender");
    enclosing_context_entry_->require_same_entry_without_callbacks(context_child_body_->entry);
    require(actual_checked_control_id_for_paint_without_callbacks()==context_child_body_->control.id_,
        "Child body Publication has another actual Control");
    context_child_body_->first();
}

std::shared_ptr<const BnwWorldViewControlRenderPublication>
BnwWorldViewControlRenderPublication::capture_context_child_body_original_owned(const OriginalOwnerIssuerKey&,
    const UiScreenGlRenderer& renderer,std::weak_ptr<const void> renderer_life,
    std::shared_ptr<BnwWorldViewControlTree> actual_tree,const detail::BnwNormalCommonChildCall& actual_call,
    BnwWorldViewControlTree::Control actual_control,const detail::BnwNormalRootContextEntry& actual_entry,
    const BnwOwnedCameraGuiScreen::Loan& gui,const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,
    std::shared_ptr<const ComposedUiFrame> actual_whole_frame){
    actual_entry.require_current_without_callbacks();
    const auto entry=actual_entry;
    entry.require_current_without_callbacks();
    const auto state=BnwWorldViewControlTree::capture_normal_context_child_body_publication_state_owned(
        actual_call,actual_tree,actual_control,entry,renderer,actual_whole_frame);
    state->first();const auto tree=std::move(actual_tree);const auto core=tree->core_;
    const auto control=std::move(actual_control);
    const auto*const operation=state->operation;const auto serial=state->serial;
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto pure=[&]{
        state->first();require(!renderer_life.expired()&&tree->core_==core&&state->renderer==&renderer&&
            &gui==&state->parent_publication->gui_&&&window==&state->parent_publication->window_,
            "Child body factory renderer/Tree/original Gui/window lender changed");
        entry.require_current_without_callbacks();entry.require_same_tree_without_callbacks(*tree);
        entry.require_same_entry_without_callbacks(state->entry);
        require(core->checked(control)==state->control.id_,"Child body factory real Control changed");
        state->first();
    };
    const auto full=[&]{pure();state->full(*operation);pure();gui.require_current();pure();window.require_current();pure();};
    try{
        pure();full();pure();
        const ContextChildBodyIssuerKey body_key;
        auto owned=std::shared_ptr<BnwWorldViewControlRenderPublication>(
            new BnwWorldViewControlRenderPublication(body_key,renderer,renderer_life,tree,control,entry,gui,window,state));
        pure();full();pure();
        require(owned&&owned->creator_==std::this_thread::get_id()&&owned->renderer_==&renderer&&
            owned->tree_==tree&&owned->control_&&!owned->root_context_entry_&&owned->enclosing_context_entry_&&
            same(owned->context_child_body_,state)&&owned->paint_indices_.empty()&&!owned->normal_label_emission_&&
            owned->topology_revision_==state->topology&&owned->effect_revision_==state->effects&&
            owned->original_owner_.expired()&&!owned->original_owner_operation_&&!owned->original_owner_source_serial_,
            "Child body new genuine Publication allocation/receiver differs");
        owned->enclosing_context_entry_->require_same_entry_without_callbacks(entry);
        require(owned->actual_checked_control_id_for_paint_without_callbacks()==state->control.id_,
            "Child body new Publication actual Control was substituted");
        // Publish only original allocation identity; no cursor, return or epoch stores.
        owned->original_owner_operation_=operation;owned->original_owner_source_serial_=serial;
        owned->original_owner_=owned;
        pure();full();pure();
        const auto canonical=owned->original_owner_before_paint_without_callbacks();
        require(same(canonical,owned),"Child body canonical original Publication shared control differs");
        pure();return canonical;
    }catch(...){core->failed.store(true);throw;}
}

UiGlPaintReport BnwWorldViewControlRenderPublication::paint_context_child_body_owned(UiScreenGlRenderer& renderer)const{
    const auto state=context_child_body_;require(bool(state),"Child body paint lacks genuine closed admission");
    state->first();
    const auto canonical_owner=original_owner_before_paint_without_callbacks();
    require(canonical_owner&&canonical_owner.get()==this&&control_&&!root_context_entry_&&enclosing_context_entry_&&
        renderer_==&renderer&&state->renderer==&renderer&&paint_indices_.empty()&&!normal_label_emission_&&
        !adjusted_frame_painting_&&!adjusted_frame_owner_&&!adjusted_frame_operation_&&!adjusted_frame_source_serial_,
        "Child body original paint replay/reentry/receiver substitution");
    const auto tree=tree_;const auto core=tree->core_;const auto*const operation=state->operation;
    const auto serial=state->serial;const auto frame=state->whole_frame;
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    adjusted_frame_painting_=true;
    struct Exit final {
        const std::shared_ptr<const BnwWorldViewControlRenderPublication> owner;
        bool& painting;~Exit(){painting=false;}
    }exit{canonical_owner,adjusted_frame_painting_};
    const auto pure=[&]{
        state->first();
        const auto owner=original_owner_without_callbacks();
        require(same(owner,canonical_owner)&&same(context_child_body_,state)&&tree_==tree&&tree->core_==core&&
            adjusted_frame_painting_&&core->busy&&!core->failed.load()&&renderer_==&renderer&&!renderer_life_.expired()&&
            control_&&!root_context_entry_&&enclosing_context_entry_&&paint_indices_.empty()&&!normal_label_emission_,
            "Child body SAME original paint/Control/Root exclusion changed");
        enclosing_context_entry_->require_same_entry_without_callbacks(state->entry);
        require(actual_checked_control_id_for_paint_without_callbacks()==state->control.id_,
            "Child body original actual Control changed during paint");
        if(adjusted_frame_owner_)require_adjusted_frame_owner_without_callbacks(frame);
        state->first();
    };
    const auto full=[&]{pure();state->full(*operation);pure();gui_.require_current();pure();window_.require_current();pure();};
    try{
        pure();full();pure();
        // SAME original Root-adjusted object, produced BEFORE original Frame
        // issuance. No empty carrier/copy/clone/item0 or per-child Matrix update.
        adjusted_frame_operation_=operation;adjusted_frame_source_serial_=serial;
        adjusted_frame_owner_=frame;
        pure();full();pure();require_adjusted_frame_owner_without_callbacks(frame);
        const auto width=gui_.width();pure();const auto height=gui_.height();pure();
        full();pure();
        auto report=renderer.paint_worldview_control_publication(*this,*frame,width,height);
        pure();full();pure();
        return report; // Actual caller report only; no Root/Draw/drain completion.
    }catch(...){core->failed.store(true);throw;}
}
