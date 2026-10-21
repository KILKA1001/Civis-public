// Closed bridge: the existing Material friendship captures genuine Matrix
// ShaderAccess while ACTIVE. This does not grant a later active-loan renewal.
detail::BnwWindowedRendererSharedDescriptorIdentity
detail::BnwNormalLabelMaterialLoan::capture_live_root_shared_descriptor_without_callbacks(
    const BnwWindowedRendererMatrixDataLoan& actual_matrix){
    const auto matrix=actual_matrix.impl_;require(bool(matrix),"Root descriptor capture lacks original Matrix");
    matrix->first();const BnwWindowedRendererMatrixDataLoan::ShaderAccess access(matrix);
    const auto identity=access.shared_descriptor_identity_without_callbacks();matrix->first();
    identity.require_current_without_callbacks();matrix->first();return identity;
}
void detail::BnwNormalLabelMaterialLoan::require_root_shared_descriptor_without_callbacks(
    const BnwWindowedRendererSharedDescriptorIdentity& identity){
    identity.require_current_without_callbacks();
}

// Inside civ5::ui AFTER canonical Core/Gui/Matrix/Prefix definitions.
// The Graph-owned FrameCore provides the separate RootScopeHealthGuard bodies.
struct detail::BnwNormalRootBatchHistory::Impl final {
    using Gui=NotificationContextGuiPort;
    using Prefix=BnwNormalGuiBatchPrefix;
    using Matrix=BnwWindowedRendererMatrixDataLoan;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<Gui> gui_facade;
    const std::weak_ptr<Gui::Impl> gui;
    const std::shared_ptr<Prefix::Impl> prefix;
    const std::shared_ptr<const Matrix> matrix_facade;
    const std::shared_ptr<Gui::Impl::WindowedMatrixState> matrix_storage;
    const BnwWindowedRendererSharedDescriptorIdentity shared_descriptor;
    const std::shared_ptr<const BnwWorldViewControlRenderPublication> publication;
    const std::shared_ptr<const ComposedUiFrame> adjusted_frame;
    // Historical health remains distinct from the retired active item.
    struct RootScope final {
        using Label=BnwLabelActualPaintCallFrame::RootScopeHealthGuard;
        using Root=BnwNormalRootPaintCallFrame::RootScopeHealthGuard;
        const std::variant<Label,Root> value;
        explicit RootScope(Label actual):value(std::move(actual)){}
        explicit RootScope(Root actual):value(std::move(actual)){}
        void require_physical_current_without_callbacks()const{
            std::visit([](const auto& actual){actual.require_physical_current_without_callbacks();},value);
        }
        void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& op)const{
            std::visit([&](const auto& actual){actual.require_source_owned(op);},value);
        }
        void require_actual_renderer_without_callbacks(const UiScreenGlRenderer& renderer)const{
            std::visit([&](const auto& actual){actual.require_actual_renderer_without_callbacks(renderer);},value);
        }
        void require_actual_publication_without_callbacks(const BnwWorldViewControlRenderPublication& publication)const{
            std::visit([&](const auto& actual){actual.require_actual_publication_without_callbacks(publication);},value);
        }
        void require_actual_frame_owner_without_callbacks(const std::shared_ptr<const ComposedUiFrame>& frame)const{
            std::visit([&](const auto& actual){actual.require_actual_frame_owner_without_callbacks(frame);},value);
        }
        std::array<std::uint32_t,2> numeric_controls_without_callbacks()const{
            return std::visit([](const auto& actual){return actual.numeric_controls_without_callbacks();},value);
        }
    };
    const RootScope root_scope;
    const std::shared_ptr<BnwWorldViewControlTree> tree;
    const UiScreenGlRenderer*const renderer;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t serial,batch_revision,matrix_epoch,matrix_gui_revision;
    const std::array<std::uint32_t,16> matrix_words;
    const std::array<std::int32_t,2> dimensions;
    const std::array<std::uint32_t,2> arithmetic_words,numeric_before,numeric_after;
    //Reserved weak per-item journal avoids Root/Item ownership cycles.
    mutable std::vector<std::weak_ptr<const BnwNormalItemOccurrence::Impl>> item_occurrences;
    mutable bool item_capturing=false;
    mutable bool label_traversal_active=false,label_traversal_completed=false;
    mutable std::weak_ptr<const BnwNormalVisibleLabelCall::Impl> current_visible_label;
    mutable std::shared_ptr<Gui::Impl::NormalGuiOuterWindow> outer_window;
    mutable std::weak_ptr<Gui::Impl::NormalGuiManagerRegistration> outer_registration;
    mutable bool outer_opening=false,outer_registering=false;
    // SAME consumed registration controls, moved at genuine manager table
    // retirement. Strong custody only; registrations remain drained and grant
    // no CURRENT entry/dispatch permission after the canonical table clears.
    mutable std::vector<std::shared_ptr<Gui::Impl::NormalGuiManagerRegistration>> completed_registrations;
    mutable bool queue_drained=false;
    mutable std::array<std::uint64_t,4> queue_observations{};
    bool sealed=false;
    template<class ActualScopeGuard>
    Impl(std::weak_ptr<Gui> actual_facade,std::weak_ptr<Gui::Impl> actual_gui,
        std::shared_ptr<Prefix::Impl> actual_prefix,std::shared_ptr<const Matrix> actual_matrix,
        std::shared_ptr<Gui::Impl::WindowedMatrixState> actual_matrix_storage,
        BnwWindowedRendererSharedDescriptorIdentity actual_shared_descriptor,
        std::shared_ptr<const BnwWorldViewControlRenderPublication> actual_publication,
        std::shared_ptr<const ComposedUiFrame> actual_adjusted_frame,
        ActualScopeGuard actual_scope,
        std::shared_ptr<BnwWorldViewControlTree> actual_tree,const UiScreenGlRenderer* actual_renderer,
        const runtime::BnwWorldViewSourceOperationLoan* actual_operation,
        std::uint64_t actual_serial,std::uint64_t actual_batch_revision,std::uint64_t actual_matrix_epoch,
        std::uint64_t actual_matrix_gui_revision,std::array<std::uint32_t,16> actual_words,
        std::array<std::int32_t,2> actual_dimensions,std::array<std::uint32_t,2> actual_arithmetic,
        std::array<std::uint32_t,2> actual_numeric_before,std::array<std::uint32_t,2> actual_numeric_after)
        :gui_facade(std::move(actual_facade)),gui(std::move(actual_gui)),prefix(std::move(actual_prefix)),
         matrix_facade(std::move(actual_matrix)),matrix_storage(std::move(actual_matrix_storage)),
         shared_descriptor(std::move(actual_shared_descriptor)),
         publication(std::move(actual_publication)),adjusted_frame(std::move(actual_adjusted_frame)),
         root_scope(std::move(actual_scope)),tree(std::move(actual_tree)),renderer(actual_renderer),
         operation(actual_operation),serial(actual_serial),batch_revision(actual_batch_revision),
         matrix_epoch(actual_matrix_epoch),matrix_gui_revision(actual_matrix_gui_revision),matrix_words(actual_words),
         dimensions(actual_dimensions),arithmetic_words(actual_arithmetic),numeric_before(actual_numeric_before),
         numeric_after(actual_numeric_after){}
    void first()const{
        require(creator==std::this_thread::get_id()&&sealed,
            "Root history foreign creator/unsealed before physical borrow");
        // This health cut checks live life/active_scope BEFORE raw Scope use.
        // It never borrows the original ACTIVE item stamp or Frame facade.
        root_scope.require_physical_current_without_callbacks();
        require(prefix&&prefix->index.has_value()==prefix->indexed_control.has_value()&&
            prefix->index.has_value()==prefix->control.has_value()&&
            prefix->index.has_value()!=prefix->context_entry.has_value(),
            "Root history lost genuine indexed receiver XOR actual Context entry");
        if(prefix->context_entry)
            BnwWorldViewControlTree::require_normal_root_history_context_without_callbacks(*this);
        const auto state=gui.lock();const auto facade=gui_facade.lock();
        require(state&&facade&&state->facade_alive.load()&&!state->failed.load()&&
            tree&&publication&&adjusted_frame&&renderer&&prefix&&matrix_facade&&matrix_storage,
            "Root history actual owners retired");
        const auto source=state->source.lock();require(bool(source),"Root history actual Source owner retired");
        const auto& fresh=source->current_operation_loan();
        require(&fresh==operation,"Root history fresh Source differs before saved operand");
        fresh.require_origin_without_callbacks();
        require(fresh.operation_serial()==serial,"Root history immutable Source serial changed");
        state->first(fresh);
        const auto canonical=state->canonical_gui.lock();
        require(canonical&&canonical.get()==facade.get()&&!canonical.owner_before(facade)&&
            !facade.owner_before(canonical)&&facade->impl_==state&&tree->core_->context_gui_port==facade&&
            !state->normal_root_history_capturing&&state->normal_root_history.lock().get()==this&&
            !state->normal_gui_batch_beginning&&state->normal_gui_batch_revision==batch_revision&&
            state->normal_gui_batch_prefix.lock()==prefix&&state->normal_gui_batch.lock()==prefix->batch&&
            prefix->creator==creator&&prefix->alive.load()&&prefix->sealed&&prefix->batch&&
            prefix->batch->alive.load()&&prefix->batch->creator==creator&&
            prefix->batch->revision==batch_revision&&prefix->revision==batch_revision&&batch_revision&&
            prefix->gui.lock()==state&&prefix->gui_facade.lock()==facade&&prefix->tree==tree&&
            prefix->publication==publication.get()&&prefix->operation==operation&&prefix->serial==serial&&
            prefix->matrix==matrix_facade,
            "Root history genuine manager/Root/prefix membership changed or original Prefix retired");
        // The SAME original Publication is strongly owned BEFORE dereferencing
        // it here. An arbitrary shared-pointer alias/copy was never accepted.
        const auto original=publication->original_owner_without_callbacks();
        require(original.get()==publication.get()&&!original.owner_before(publication)&&
            !publication.owner_before(original),"Root history original Publication control replaced");
        publication->require_adjusted_frame_owner_without_callbacks(adjusted_frame);
        root_scope.require_actual_renderer_without_callbacks(*renderer);
        root_scope.require_actual_publication_without_callbacks(*publication);
        root_scope.require_actual_frame_owner_without_callbacks(adjusted_frame);
        tree->core_->layout_origin(true);
        require(&tree->core_->source->current_operation_loan()==operation,
            "Root history Tree has another fresh actual Source");
        // Historical storage validation deliberately does not call Matrix's
        // ACTIVE first, compare current manager matrix/revision, or require its
        // mutable active alive bit. These OWN snapshots were extracted while
        // that original epoch was ACTIVE and retain its exact physical storage.
        const auto actual_matrix=matrix_facade->impl_;
        require(actual_matrix&&actual_matrix->creator==creator&&actual_matrix->sealed&&
            actual_matrix->gui.lock()==state&&actual_matrix->facade.lock()==facade&&
            actual_matrix->matrix==matrix_storage&&actual_matrix->operation==operation&&
            actual_matrix->serial==serial&&matrix_storage->creator==creator&&matrix_storage->completed&&
            matrix_storage->source_serial==serial&&matrix_storage->update==matrix_epoch&&matrix_epoch&&
            matrix_storage->gui_revision==matrix_gui_revision&&matrix_storage->words==matrix_words&&
            matrix_storage->dimensions==dimensions&&matrix_storage->arithmetic_words==arithmetic_words&&
            matrix_storage->numeric_before==numeric_before&&matrix_storage->numeric_after==numeric_after,
            "Root history original captured Matrix storage/epoch/words changed");
        BnwNormalLabelMaterialLoan::require_root_shared_descriptor_without_callbacks(shared_descriptor);
        require(prefix->batch->root_offset==arithmetic_words&&prefix->batch->dimensions==dimensions,
            "Root history actual batch dimensions/arithmetic differ from captured Matrix");
        const auto scope_numeric=root_scope.numeric_controls_without_callbacks();
        require(scope_numeric[0]==numeric_before[0]&&
            (scope_numeric[1]&~std::uint32_t{0x3f})==(numeric_before[1]&~std::uint32_t{0x3f}),
            "Root history actual root numeric controls differ from captured Matrix");
        Matrix::Impl::same_controls(numeric_before);
        root_scope.require_physical_current_without_callbacks();
        fresh.require_origin_without_callbacks();
        require(&source->current_operation_loan()==operation&&fresh.operation_serial()==serial&&
            state->normal_root_history.lock().get()==this&&state->normal_gui_batch_prefix.lock()==prefix,
            "Root history terminal physical Source/owner cut changed");
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& supplied)const{
        first();require(&supplied==operation,"Root history supplied another Source operand");
        supplied.require_operation(serial);first();root_scope.require_source_owned(supplied);first();
        supplied.require_operation(serial);first();
    }
};

detail::BnwNormalRootBatchHistory::Guard::Guard(std::shared_ptr<const Impl> actual,const IssuerKey&)
    :impl_(std::move(actual)){}
void detail::BnwNormalRootBatchHistory::Guard::require_physical_current_without_callbacks()const{
    const auto saved=impl_;require(bool(saved),"Root history Guard missing own control");saved->first();
}
void detail::BnwNormalRootBatchHistory::Guard::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto saved=impl_;require(bool(saved),"Root history Guard missing own control FULL");saved->full(operation);saved->first();
}
void detail::BnwNormalRootBatchHistory::Guard::require_same_root_without_callbacks(const Guard& supplied)const{
    const auto saved=impl_;const auto other=supplied.impl_;require(bool(saved)&&bool(other),"Root history SAME operand absent");
    saved->first();other->first();
    require(saved.get()==other.get()&&!saved.owner_before(other)&&!other.owner_before(saved),
        "Root history another earned Root control");saved->first();
}
void detail::BnwNormalRootBatchHistory::Guard::require_actual_renderer_without_callbacks(
    const UiScreenGlRenderer& renderer)const{
    const auto saved=impl_;saved->first();require(saved->renderer==&renderer,"Root history another actual renderer");
    saved->root_scope.require_actual_renderer_without_callbacks(renderer);saved->first();
}
void detail::BnwNormalRootBatchHistory::Guard::require_actual_matrix_without_callbacks(
    const BnwWindowedRendererMatrixDataLoan& supplied)const{
    // Capture its SAME control before PURE; do not call the supplied ACTIVE API.
    const auto input=supplied.impl_;const auto saved=impl_;saved->first();
    const auto original=saved->matrix_facade->impl_;
    require(input&&input.get()==original.get()&&!input.owner_before(original)&&!original.owner_before(input),
        "Root-bound Matrix bridge received another original Matrix control");
    require(input->matrix==saved->matrix_storage&&input->operation==saved->operation&&input->serial==saved->serial,
        "Root-bound Matrix immutable captured storage/Source changed");saved->first();
}
detail::BnwWindowedRendererSharedDescriptorIdentity
detail::BnwNormalRootBatchHistory::Guard::captured_shared_descriptor_identity_without_callbacks()const{
    const auto saved=impl_;saved->first();const auto identity=saved->shared_descriptor;saved->first();return identity;
}
std::array<std::uint32_t,16> detail::BnwNormalRootBatchHistory::Guard::matrix_words_without_callbacks()const{
    const auto saved=impl_;saved->first();const auto value=saved->matrix_words;saved->first();return value;
}
std::array<std::int32_t,2> detail::BnwNormalRootBatchHistory::Guard::dimensions_without_callbacks()const{
    const auto saved=impl_;saved->first();const auto value=saved->dimensions;saved->first();return value;
}
std::array<std::uint32_t,2> detail::BnwNormalRootBatchHistory::Guard::arithmetic_words_without_callbacks()const{
    const auto saved=impl_;saved->first();const auto value=saved->arithmetic_words;saved->first();return value;
}
std::array<std::uint32_t,2> detail::BnwNormalRootBatchHistory::Guard::numeric_controls_without_callbacks()const{
    const auto saved=impl_;saved->first();const auto value=saved->numeric_before;saved->first();return value;
}
std::array<std::uint32_t,2> detail::BnwNormalRootBatchHistory::Guard::numeric_return_controls_without_callbacks()const{
    const auto saved=impl_;saved->first();const auto value=saved->numeric_after;saved->first();return value;
}
std::uint64_t detail::BnwNormalRootBatchHistory::Guard::matrix_epoch_without_callbacks()const{
    const auto saved=impl_;saved->first();const auto value=saved->matrix_epoch;saved->first();return value;
}

detail::BnwNormalRootBatchHistory::Guard BnwWorldViewControlTree::capture_normal_root_batch_history_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const BnwWorldViewControlRenderPublication& actual_publication,
    const detail::BnwLabelActualPaintCallFrame& actual_frame,
    std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> actual_matrix,
    const detail::BnwNormalGuiBatchPrefix& actual_prefix)const{
    const auto frame=actual_frame.capture_guard_without_callbacks();frame.require_current_without_callbacks();
    return capture_normal_root_batch_history_from_frame_guard_owned(operation,actual_publication,frame,
        std::move(actual_matrix),actual_prefix);
}
detail::BnwNormalRootBatchHistory::Guard BnwWorldViewControlTree::capture_normal_root_batch_history_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const BnwWorldViewControlRenderPublication& actual_publication,
    const detail::BnwNormalRootPaintCallFrame& actual_frame,
    std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> actual_matrix,
    const detail::BnwNormalGuiBatchPrefix& actual_prefix)const{
    const auto frame=actual_frame.capture_guard_without_callbacks();frame.require_current_without_callbacks();
    return capture_normal_root_batch_history_from_frame_guard_owned(operation,actual_publication,frame,
        std::move(actual_matrix),actual_prefix);
}
detail::BnwNormalRootBatchHistory::Guard BnwWorldViewControlTree::capture_normal_root_batch_history_from_frame_guard_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const BnwWorldViewControlRenderPublication& actual_publication,
    const detail::BnwLabelActualPaintCallFrame::Guard& actual_guard,
    std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> actual_matrix,
    const detail::BnwNormalGuiBatchPrefix& actual_prefix)const{
    return capture_normal_root_batch_history_from_frame_guard_impl_owned(operation,actual_publication,actual_guard,
        std::move(actual_matrix),actual_prefix);
}
detail::BnwNormalRootBatchHistory::Guard BnwWorldViewControlTree::capture_normal_root_batch_history_from_frame_guard_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const BnwWorldViewControlRenderPublication& actual_publication,
    const detail::BnwNormalRootPaintCallFrame::Guard& actual_guard,
    std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> actual_matrix,
    const detail::BnwNormalGuiBatchPrefix& actual_prefix)const{
    return capture_normal_root_batch_history_from_frame_guard_impl_owned(operation,actual_publication,actual_guard,
        std::move(actual_matrix),actual_prefix);
}
template<class ActualFrameGuard>
detail::BnwNormalRootBatchHistory::Guard BnwWorldViewControlTree::capture_normal_root_batch_history_from_frame_guard_impl_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const BnwWorldViewControlRenderPublication& actual_publication,
    const ActualFrameGuard& actual_guard,
    std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> actual_matrix,
    const detail::BnwNormalGuiBatchPrefix& actual_prefix)const{
    static_assert(std::is_same_v<ActualFrameGuard,detail::BnwLabelActualPaintCallFrame::Guard>||
        std::is_same_v<ActualFrameGuard,detail::BnwNormalRootPaintCallFrame::Guard>);
    using Root=detail::BnwNormalRootBatchHistory;using Gui=detail::NotificationContextGuiPort;
    const auto frame=actual_guard;frame.require_current_without_callbacks();
    const auto prefix=actual_prefix.impl_;require(bool(prefix),"Root history missing original executed Prefix");
    const auto publication=actual_publication.original_owner_without_callbacks();
    const auto tree=publication->tree_;const auto adjusted=publication->adjusted_frame_owner_;
    require(tree&&tree.get()==this&&publication.get()==&actual_publication&&adjusted&&publication->renderer_,
        "Root history requires SAME original Publication/Tree/adjusted Frame");
    const auto*const renderer=publication->renderer_;
    const auto owner=tree->core_;const auto facade=owner->context_gui_port;
    require(bool(facade)&&bool(actual_matrix),"Root history actual canonical Gui/Matrix absent");
    const auto state=facade->impl_;const auto matrix=std::move(actual_matrix);
    const auto active_first=[&]{
        frame.require_current_without_callbacks();prefix->first();
        matrix->require_current_without_callbacks();owner->layout_origin(true);
        const auto& fresh=owner->source->current_operation_loan();
        require(&fresh==&operation,"Root history ACTIVE fresh Source differs before supplied operand");
        fresh.require_origin_without_callbacks();state->first(fresh);
        prefix->frame.require_same_actual_without_callbacks(frame);
        require(facade->impl_==state&&owner->context_gui_port==facade&&state->canonical_gui.lock()==facade&&
            prefix->tree==tree&&prefix->publication==publication.get()&&
            prefix->matrix.get()==matrix.get()&&!prefix->matrix.owner_before(matrix)&&
            !matrix.owner_before(prefix->matrix)&&prefix->operation==&operation&&
            matrix->impl_->gui.lock()==state&&matrix->impl_->facade.lock()==facade,
            "Root history ACTIVE inputs are not SAME actual prefix/frame/Matrix/manager");
        frame.require_actual_renderer_without_callbacks(*renderer);
        BnwWorldViewControlTree::require_normal_gui_batch_receiver_without_callbacks(*prefix);
        if(prefix->context_entry){
            if constexpr(std::is_same_v<ActualFrameGuard,detail::BnwNormalRootPaintCallFrame::Guard>)
                frame.require_actual_context_entry_without_callbacks(*publication,*prefix->context_entry);
            else require(false,"Context history cannot capture a Label Frame health control");
        }else{
            frame.require_actual_paint_item_without_callbacks(*publication,*prefix->index);
            frame.require_actual_control_without_callbacks(*tree,*prefix->control);
        }
        const auto original=publication->original_owner_without_callbacks();
        require(original.get()==publication.get()&&!original.owner_before(publication)&&
            !publication.owner_before(original),"Root history original Publication replaced during capture");
        publication->require_adjusted_frame_owner_without_callbacks(adjusted);
        frame.require_current_without_callbacks();
    };
    const auto active_full=[&]{active_first();frame.require_source_owned(operation);active_first();
        prefix->full(operation);active_first();matrix->require_source_owned(operation);active_first();};
    active_first();active_full();active_first();
    require(!prefix->common_root&&!state->normal_root_history_capturing&&state->normal_root_history.expired(),
        "Root history must be captured once BEFORE actual Common traversal");
    const auto serial=operation.operation_serial(),revision=prefix->revision;
    const auto matrix_storage=matrix->impl_->matrix;
    const auto words=matrix->matrix_words_without_callbacks();active_first();
    const auto dimensions=matrix->dimensions_without_callbacks();active_first();
    const auto arithmetic=matrix->arithmetic_words_without_callbacks();active_first();
    const auto before=matrix->numeric_controls_without_callbacks();active_first();
    const auto after=matrix->numeric_return_controls_without_callbacks();active_first();
    const auto epoch=matrix_storage->update,matrix_gui_revision=matrix_storage->gui_revision;
    // Mandatory genuine identity is captured before the original Matrix epoch retires.
    const auto shared_descriptor=detail::BnwNormalLabelMaterialLoan::capture_live_root_shared_descriptor_without_callbacks(*matrix);
    active_first();active_full();active_first();
    // This distinct historical capability is extracted while original Frame
    // FIRST/FULL and actual physical Matrix epoch are still live. It is never
    // reconstructed from an already retired active item or raw frame pointer.
    const auto scope=frame.capture_root_scope_health_owned(operation);active_first();active_full();active_first();
    scope.require_physical_current_without_callbacks();scope.require_actual_renderer_without_callbacks(*renderer);
    scope.require_actual_publication_without_callbacks(*publication);scope.require_actual_frame_owner_without_callbacks(adjusted);
    require(epoch&&serial&&revision,"Root history requires actual nonempty Matrix/Source/batch epochs");
    state->normal_root_history_capturing=true;
    struct End{std::shared_ptr<Gui::Impl> state;~End(){state->normal_root_history_capturing=false;}}end{state};
    const auto pending=[&]{
        active_first();scope.require_physical_current_without_callbacks();
        require(state->normal_root_history_capturing&&state->normal_root_history.expired()&&!prefix->common_root&&
            prefix->revision==revision&&operation.operation_serial()==serial&&matrix->impl_->matrix==matrix_storage&&
            matrix_storage->update==epoch&&matrix_storage->gui_revision==matrix_gui_revision&&
            matrix_storage->words==words&&matrix_storage->dimensions==dimensions&&
            matrix_storage->arithmetic_words==arithmetic&&matrix_storage->numeric_before==before&&
            matrix_storage->numeric_after==after,
            "Root history allocation/FULL POST changed actual original inputs");
        publication->require_adjusted_frame_owner_without_callbacks(adjusted);
        scope.require_actual_frame_owner_without_callbacks(adjusted);
        detail::BnwNormalLabelMaterialLoan::require_root_shared_descriptor_without_callbacks(shared_descriptor);
    };
    try{
        pending();active_full();pending();
        // Constructor operands below are OWN VALUES before allocator/new-handler.
        auto owned=std::make_shared<Root::Impl>(facade,state,prefix,matrix,matrix_storage,shared_descriptor,publication,adjusted,scope,
            tree,renderer,&operation,serial,revision,epoch,matrix_gui_revision,words,dimensions,arithmetic,before,after);
        pending();active_full();pending();
        require(owned&&owned->creator==std::this_thread::get_id()&&!owned->sealed,
            "Root history new original allocation/control invalid");
        // No allocator, Source FULL, callbacks or arithmetic between canonical
        // publication and seal. Guard read friends cannot construct IssuerKey.
        state->normal_root_history=owned;state->normal_root_history_capturing=false;owned->sealed=true;
        owned->first();owned->full(operation);owned->first();
        const Root::IssuerKey key;return Root::Guard(owned,key);
    }catch(...){state->failed.store(true);throw;}
}
