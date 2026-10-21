// Inside civ5::ui after Root/Paint/DATA and one actual original Prefix producer.
struct detail::BnwNormalRootBatchExecution::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true};
    std::unique_ptr<BnwNormalGuiBatchPrefix> original_prefix;
    const std::shared_ptr<const BnwWindowedRendererMatrixDataLoan> original_matrix;
    const BnwNormalRootBatchHistory::Guard root;
    std::vector<std::unique_ptr<BnwNormalLabelMaterialLoan>> original_materials;
    std::shared_ptr<BnwWorldViewControlTree::NormalCommonTraversal> common_traversal;
    enum class LabelPhase {unstarted,bootstrap_active,bootstrap_returned,continuation_active,completed};
    LabelPhase label_phase=LabelPhase::unstarted; //Only actual Tree effects advance this state.
    bool sealed=false;
    Impl(std::unique_ptr<BnwNormalGuiBatchPrefix> prefix,
        std::shared_ptr<const BnwWindowedRendererMatrixDataLoan> matrix,BnwNormalRootBatchHistory::Guard actual_root)
        :original_prefix(std::move(prefix)),original_matrix(std::move(matrix)),root(std::move(actual_root)){}
    void first()const{
        require(creator==std::this_thread::get_id()&&alive.load()&&sealed&&original_prefix&&original_matrix,
            "Original Root execution retired/foreign/unsealed");
        // Historical Root FIRST is separate from old active Frame and Matrix.
        // Root also verifies original Prefix facade's unreplaced alive control.
        root.require_physical_current_without_callbacks();
        require(alive.load()&&bool(original_prefix),"Original Root execution retired during physical cut");
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& op)const{
        first();root.require_source_owned(op);first();
    }
};
detail::BnwNormalRootBatchExecution::BnwNormalRootBatchExecution(std::shared_ptr<Impl> actual,const IssuerKey&):impl_(std::move(actual)){}
detail::BnwNormalRootBatchExecution::~BnwNormalRootBatchExecution(){
    impl_->alive.store(false);impl_->original_prefix.reset(); //No callback or borrowed Source/Scope use.
}
detail::BnwNormalRootBatchHistory::Guard
detail::BnwNormalRootBatchExecution::captured_root_without_callbacks()const{
    const auto saved=impl_;saved->first();const auto root=saved->root;saved->first();return root;
}
std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>
detail::BnwNormalRootBatchExecution::actual_matrix_owned_without_callbacks()const{
    const auto saved=impl_;saved->first();const auto matrix=saved->original_matrix;saved->first();return matrix;
}
void detail::BnwNormalRootBatchExecution::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& op)const{
    const auto saved=impl_;saved->full(op);saved->first();
}

std::unique_ptr<detail::BnwNormalRootBatchExecution>
BnwWorldViewControlTree::begin_normal_root_batch_execution_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const BnwWorldViewControlRenderPublication& actual_publication,std::size_t index,
    const detail::BnwLabelActualPaintCallFrame& actual_frame)const{
    const auto guard=actual_frame.capture_guard_without_callbacks();
    return begin_normal_root_batch_execution_from_frame_guard_impl_owned(operation,actual_publication,index,guard);
}
std::unique_ptr<detail::BnwNormalRootBatchExecution>
BnwWorldViewControlTree::begin_normal_root_batch_execution_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const BnwWorldViewControlRenderPublication& actual_publication,std::size_t index,
    const detail::BnwNormalRootPaintCallFrame& actual_frame)const{
    const auto guard=actual_frame.capture_guard_without_callbacks();
    return begin_normal_root_batch_execution_from_frame_guard_impl_owned(operation,actual_publication,index,guard);
}
template<class ActualFrameGuard>
std::unique_ptr<detail::BnwNormalRootBatchExecution>
BnwWorldViewControlTree::begin_normal_root_batch_execution_from_frame_guard_impl_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const BnwWorldViewControlRenderPublication& actual_publication,std::optional<std::size_t> index,
    const ActualFrameGuard& actual_guard)const{
    static_assert(std::is_same_v<ActualFrameGuard,detail::BnwLabelActualPaintCallFrame::Guard>||
        std::is_same_v<ActualFrameGuard,detail::BnwNormalRootPaintCallFrame::Guard>);
    using Execution=detail::BnwNormalRootBatchExecution;
    // Capture all original facade controls before any Source FULL/allocation.
    // Captured Guard delegates below keep SAME original Frame, not a clone.
    const auto frame=actual_guard;frame.require_current_without_callbacks();
    const auto context_entry=index?std::optional<detail::BnwNormalRootContextEntry>{}:
        actual_publication.actual_root_context_entry_for_paint_without_callbacks();
    require(index.has_value()!=context_entry.has_value(),
        "Root boundary requires a genuine indexed receiver OR actual Context entry");
    const auto receiver_first=[&]{
        frame.require_current_without_callbacks();
        if(context_entry){
            if constexpr(std::is_same_v<ActualFrameGuard,detail::BnwNormalRootPaintCallFrame::Guard>){
                context_entry->require_current_without_callbacks();
                const auto actual=actual_publication.actual_root_context_entry_for_paint_without_callbacks();
                require(actual.has_value(),"Root boundary original Context entry was removed");
                context_entry->require_same_entry_without_callbacks(*actual);
                context_entry->require_same_tree_without_callbacks(*this);
                frame.require_actual_context_entry_without_callbacks(actual_publication,*context_entry);
                context_entry->require_current_without_callbacks();
            }else require(false,"Context root boundary cannot enter through a Label Frame");
        }else frame.require_actual_paint_item_without_callbacks(actual_publication,*index);
        frame.require_current_without_callbacks();
    };
    receiver_first();
    const auto publication=actual_publication.original_owner_without_callbacks();
    const auto tree=publication->tree_;require(tree&&tree.get()==this,"Root boundary has another original Tree");
    const auto owner=tree->core_;
    const auto active_first=[&]{
        receiver_first();
        owner->layout_origin(true);const auto& fresh=owner->source->current_operation_loan();
        require(&fresh==&operation,"Root boundary fresh Source differs before supplied operand");
        fresh.require_origin_without_callbacks();
        const auto original=publication->original_owner_without_callbacks();
        require(original.get()==publication.get()&&!original.owner_before(publication)&&
            !publication.owner_before(original),"Root boundary original Publication control changed");
        frame.require_current_without_callbacks();
    };
    const auto active_full=[&]{active_first();frame.require_source_owned(operation);active_first();};
    try{
        active_first();active_full();active_first();
        const auto gui=owner->context_gui_port;require(bool(gui),"Root boundary canonical Gui absent");
        const auto manager=gui->impl_;manager->first(operation);
        const auto preceding=manager->normal_gui_batch_prefix.lock();
        require(!manager->normal_gui_batch_beginning&&!manager->normal_root_history_capturing&&
            manager->normal_root_history.expired()&&(!preceding||!preceding->alive.load()),
            "Root boundary cannot begin another epoch while an original Root/Prefix remains live");
        // One actual manager epoch for the entire enclosing Root traversal.
        const auto matrix=tree->windowed_renderer_matrix_data_owned(tree,operation);
        active_first();active_full();active_first();
        require(bool(matrix),"Root boundary genuine Matrix producer absent");
        // This producer consumes the captured ORIGINAL Frame Guard, so no raw
        // Frame facade is dereferenced after the Matrix producer's callbacks.
        std::unique_ptr<detail::BnwNormalGuiBatchPrefix> prefix;
        if(index)prefix=publication->begin_normal_gui_batch_prefix_from_frame_guard_owned(*index,operation,frame,matrix);
        else{
            if constexpr(std::is_same_v<ActualFrameGuard,detail::BnwNormalRootPaintCallFrame::Guard>)
                prefix=publication->begin_normal_gui_batch_prefix_from_frame_guard_owned(operation,frame,matrix);
            else require(false,"Context execution has no Label-prefix producer");
        }
        active_first();active_full();active_first();
        require(bool(prefix),"Root boundary original executed Prefix facade absent");
        const auto root=tree->capture_normal_root_batch_history_from_frame_guard_owned(
            operation,*publication,frame,matrix,*prefix);
        active_first();root.require_physical_current_without_callbacks();
        root.require_source_owned(operation);active_first();root.require_physical_current_without_callbacks();
        const auto captured=root.impl_;
        require(captured&&captured->prefix==prefix->impl_&&captured->matrix_facade.get()==matrix.get()&&
            !captured->matrix_facade.owner_before(matrix)&&!matrix.owner_before(captured->matrix_facade)&&
            !captured->prefix->common_root,
            "Root boundary original Prefix/Matrix differs or Common already entered");
        tree->open_normal_root_outer_window_owned(operation,root);
        active_first();root.require_source_owned(operation);active_first();
        // Unique ownership is moved into the original execution control. The
        // prefix stays physically alive across allocator callbacks; Root POST
        // still checks its genuine alive and manager membership afterwards.
        active_first();root.require_source_owned(operation);active_first();
        auto owned=std::make_shared<Execution::Impl>(std::move(prefix),matrix,root);
        active_first();root.require_physical_current_without_callbacks();
        root.require_source_owned(operation);active_first();root.require_physical_current_without_callbacks();
        require(owned&&owned->original_prefix&&owned->original_prefix->impl_==captured->prefix&&
            owned->original_matrix==matrix&&!owned->sealed&&!captured->prefix->common_root,
            "Root boundary original execution allocation POST changed inputs");
        owned->sealed=true;owned->first();owned->full(operation);owned->first();
        const Execution::IssuerKey key;
        auto result=std::unique_ptr<Execution>(new Execution(owned,key));
        owned->first();owned->full(operation);owned->first();return result;
    }catch(...){owner->failed.store(true);throw;}
}
