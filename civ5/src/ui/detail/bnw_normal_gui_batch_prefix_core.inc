// Inside civ5::ui AFTER complete Gui/Matrix/Frame and canonical Core.
// This actual producer executes the supported own normal-zero batch beginning;
// the unresolved Context RootRead->slot10->root18 is NOT silently dispatched.
struct detail::BnwNormalGuiBatchPrefix::Impl final {
    using Gui=NotificationContextGuiPort;using Batch=Gui::Impl::NormalGuiBatchState;
    const std::thread::id creator=std::this_thread::get_id();std::atomic<bool> alive{true};
    const std::weak_ptr<Gui> gui_facade;const std::weak_ptr<Gui::Impl> gui;
    const std::shared_ptr<Batch> batch;
    const std::shared_ptr<const BnwWindowedRendererMatrixDataLoan> matrix;
    // Only the two original renderer-issued controls can inhabit this value.
    // It forwards health to the actual alternative; it never manufactures a Label.
    struct FrameGuard final {
        using Label=BnwLabelActualPaintCallFrame::Guard;
        using Root=BnwNormalRootPaintCallFrame::Guard;
        const std::variant<Label,Root> value;
        explicit FrameGuard(Label actual):value(std::move(actual)){}
        explicit FrameGuard(Root actual):value(std::move(actual)){}
        void require_current_without_callbacks()const{
            std::visit([](const auto& actual){actual.require_current_without_callbacks();},value);
        }
        void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& op)const{
            std::visit([&](const auto& actual){actual.require_source_owned(op);},value);
        }
        void require_actual_paint_item_without_callbacks(const BnwWorldViewControlRenderPublication& pub,std::size_t index)const{
            std::visit([&](const auto& actual){actual.require_actual_paint_item_without_callbacks(pub,index);},value);
        }
        void require_actual_control_without_callbacks(const BnwWorldViewControlTree& tree,std::uint64_t control)const{
            std::visit([&](const auto& actual){actual.require_actual_control_without_callbacks(tree,control);},value);
        }
        void require_actual_context_entry_without_callbacks(const BnwWorldViewControlRenderPublication& pub,
            const BnwNormalRootContextEntry& entry)const{
            require_current_without_callbacks();
            const auto*const actual=std::get_if<Root>(&value);
            require(actual,"Context prefix requires the actual generic Frame, never a Label surrogate");
            actual->require_actual_context_entry_without_callbacks(pub,entry);
            require_current_without_callbacks();
        }
        std::array<std::uint32_t,2> actual_numeric_profile_without_callbacks()const{
            return std::visit([](const auto& actual){return actual.actual_numeric_profile_without_callbacks();},value);
        }
        template<class Actual>
        void require_same_actual_without_callbacks(const Actual& supplied)const{
            static_assert(std::is_same_v<Actual,Label>||std::is_same_v<Actual,Root>);
            require_current_without_callbacks();supplied.require_current_without_callbacks();
            const auto*const captured=std::get_if<Actual>(&value);
            require(captured&&captured->impl_==supplied.impl_&&
                !captured->impl_.owner_before(supplied.impl_)&&!supplied.impl_.owner_before(captured->impl_),
                "Normal GUI prefix has another actual Frame control or physical subtype");
            require_current_without_callbacks();
        }
    };
    const FrameGuard frame;
    const std::shared_ptr<BnwWorldViewControlTree> tree;
    const BnwWorldViewControlRenderPublication*const publication;
    const std::optional<std::size_t> index;
    const std::optional<BnwWorldViewControlTree::Control> indexed_control;
    const std::optional<BnwNormalRootContextEntry> context_entry;
    //Earned ordinary id for existing indexed comparisons only. Context has none.
    const std::optional<std::uint64_t> control;
    const std::uint64_t serial,revision;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    struct CommonRoot;std::shared_ptr<CommonRoot> common_root;
    std::weak_ptr<BnwWorldViewControlTree::NormalCommonTraversal> common_dispatch;
    void require_common_root_without_callbacks()const;
    bool sealed=false;
    template<class ActualFrameGuard>
    Impl(std::weak_ptr<Gui> facade,std::weak_ptr<Gui::Impl> owner,std::shared_ptr<Batch> physical,
        std::shared_ptr<const BnwWindowedRendererMatrixDataLoan> actual_matrix,
        ActualFrameGuard actual_frame,std::shared_ptr<BnwWorldViewControlTree> actual_tree,
        const BnwWorldViewControlRenderPublication* actual_publication,std::optional<std::size_t> actual_index,
        std::optional<BnwWorldViewControlTree::Control> actual_indexed_control,
        std::optional<BnwNormalRootContextEntry> actual_context_entry,
        std::optional<std::uint64_t> actual_control,std::uint64_t operation_serial,std::uint64_t batch_revision,
        const runtime::BnwWorldViewSourceOperationLoan* actual_operation)
        :gui_facade(std::move(facade)),gui(std::move(owner)),batch(std::move(physical)),
         matrix(std::move(actual_matrix)),frame(std::move(actual_frame)),tree(std::move(actual_tree)),
         publication(actual_publication),index(actual_index),indexed_control(std::move(actual_indexed_control)),
         context_entry(std::move(actual_context_entry)),control(actual_control),serial(operation_serial),
         revision(batch_revision),operation(actual_operation){}
    void first()const{
        require(creator==std::this_thread::get_id()&&alive.load()&&sealed&&batch&&batch->alive.load(),
            "Normal GUI batch prefix retired/foreign/unsealed FIRST");
        //Actual Frame and typed receiver FIRST precede borrowed owner state.
        frame.require_current_without_callbacks();
        BnwWorldViewControlTree::require_normal_gui_batch_receiver_without_callbacks(*this);
        const auto manager=gui.lock();const auto facade=gui_facade.lock();
        require(manager&&facade&&manager->facade_alive.load()&&!manager->failed.load(),
            "Normal GUI batch canonical manager/facade retired");
        const auto source=manager->source.lock();require(bool(source),"Normal GUI batch Source owner retired");
        const auto& current=source->current_operation_loan();
        require(&current==operation,"Normal GUI batch saved operation replaced before raw pointer");
        current.require_origin_without_callbacks();require(current.operation_serial()==serial,
            "Normal GUI batch immutable serial changed");manager->first(current);
        require(manager->canonical_gui.lock()==facade&&facade->impl_==manager&&
            tree&&tree->core_->context_gui_port==facade&&!manager->normal_gui_batch_beginning&&
            manager->normal_gui_batch_revision==revision&&manager->normal_gui_batch.lock()==batch&&
            manager->normal_gui_batch_prefix.lock().get()==this&&
            batch->creator==creator&&batch->revision==revision&&revision,
            "Normal GUI batch owner/control/revision replaced or recursive");
        tree->core_->layout_origin(true);
        require(&tree->core_->source->current_operation_loan()==operation,
            "Normal GUI batch Tree has another fresh actual Source");
        BnwWorldViewControlTree::require_normal_gui_batch_receiver_without_callbacks(*this);
        matrix->require_current_without_callbacks();
        require(matrix->impl_->gui.lock()==manager&&matrix->impl_->facade.lock()==facade&&
            batch->root_offset==matrix->arithmetic_words_without_callbacks()&&
            batch->dimensions==matrix->dimensions_without_callbacks(),
            "Normal GUI batch needs SAME current Matrix/Gui and actual mutable offset/dimensions");
        const auto a=frame.actual_numeric_profile_without_callbacks();
        const auto b=matrix->numeric_controls_without_callbacks();
        require(a[0]==b[0]&&(a[1]&~std::uint32_t{0x3f})==(b[1]&~std::uint32_t{0x3f}),
            "Normal GUI batch Frame/Matrix numeric controls differ");
        static constexpr std::array expected{Batch::Effect::viewport_zero,Batch::Effect::root_offset,
            Batch::Effect::root_alpha,Batch::Effect::root_arg4,Batch::Effect::root_arg5,
            Batch::Effect::viewport_dimensions,Batch::Effect::viewport_min_z,Batch::Effect::viewport_max_z,
            Batch::Effect::entered};
        require(batch->count==expected.size()&&batch->history==expected&&batch->viewport_selector==0&&
            batch->root_alpha==0x3f800000u&&batch->root_depth==std::array<std::uint32_t,2>{0x3f800000u,0}&&
            batch->viewport_depth==std::array<std::uint32_t,2>{0,0x3f800000u}&&
            batch->dimensions[0]>0&&batch->dimensions[1]>0,
            "Normal GUI batch actual ordered beginning effects changed");
        matrix->require_current_without_callbacks();frame.require_current_without_callbacks();
        BnwWorldViewControlTree::require_normal_gui_batch_receiver_without_callbacks(*this);
        if(common_root)require_common_root_without_callbacks();
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& supplied)const{
        first();require(&supplied==operation,"Normal GUI batch another Source operand");
        frame.require_source_owned(supplied);first();matrix->require_source_owned(supplied);first();
        frame.require_source_owned(supplied);first();
    }
};
detail::BnwNormalGuiBatchPrefix::BnwNormalGuiBatchPrefix(std::shared_ptr<Impl> state):impl_(std::move(state)){}
detail::BnwNormalGuiBatchPrefix::~BnwNormalGuiBatchPrefix(){
    impl_->alive.store(false);impl_->batch->alive.store(false); //No borrowed access/callback after retirement.
}
void detail::BnwNormalGuiBatchPrefix::require_current_without_callbacks()const{const auto saved=impl_;saved->first();}
void detail::BnwNormalGuiBatchPrefix::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& op)const{
    const auto saved=impl_;saved->full(op);saved->first();
}
std::unique_ptr<detail::BnwNormalGuiBatchPrefix>
BnwWorldViewControlRenderPublication::begin_normal_gui_batch_prefix_owned(std::size_t index,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwLabelActualPaintCallFrame& actual_frame,
    std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> actual_matrix)const{
    const auto frame=actual_frame.capture_guard_without_callbacks();frame.require_current_without_callbacks();
    return begin_normal_gui_batch_prefix_from_frame_guard_owned(index,operation,frame,std::move(actual_matrix));
}
std::unique_ptr<detail::BnwNormalGuiBatchPrefix>
BnwWorldViewControlRenderPublication::begin_normal_gui_batch_prefix_owned(std::size_t index,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalRootPaintCallFrame& actual_frame,
    std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> actual_matrix)const{
    const auto frame=actual_frame.capture_guard_without_callbacks();frame.require_current_without_callbacks();
    return begin_normal_gui_batch_prefix_from_frame_guard_owned(index,operation,frame,std::move(actual_matrix));
}
std::unique_ptr<detail::BnwNormalGuiBatchPrefix>
BnwWorldViewControlRenderPublication::begin_normal_gui_batch_prefix_from_frame_guard_owned(std::size_t index,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwLabelActualPaintCallFrame::Guard& actual_guard,
    std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> actual_matrix)const{
    return begin_normal_gui_batch_prefix_from_frame_guard_impl_owned(index,operation,actual_guard,std::move(actual_matrix));
}
std::unique_ptr<detail::BnwNormalGuiBatchPrefix>
BnwWorldViewControlRenderPublication::begin_normal_gui_batch_prefix_from_frame_guard_owned(std::size_t index,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalRootPaintCallFrame::Guard& actual_guard,
    std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> actual_matrix)const{
    return begin_normal_gui_batch_prefix_from_frame_guard_impl_owned(index,operation,actual_guard,std::move(actual_matrix));
}
template<class ActualFrameGuard>
std::unique_ptr<detail::BnwNormalGuiBatchPrefix>
BnwWorldViewControlRenderPublication::begin_normal_gui_batch_prefix_from_frame_guard_impl_owned(std::optional<std::size_t> index,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const ActualFrameGuard& actual_guard,
    std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> actual_matrix)const{
    static_assert(std::is_same_v<ActualFrameGuard,detail::BnwLabelActualPaintCallFrame::Guard>||
        std::is_same_v<ActualFrameGuard,detail::BnwNormalRootPaintCallFrame::Guard>);
    using Loan=detail::BnwNormalGuiBatchPrefix;using Gui=detail::NotificationContextGuiPort;
    using Batch=Gui::Impl::NormalGuiBatchState;
    const auto frame=actual_guard;frame.require_current_without_callbacks();
    const auto context_entry=index?std::optional<detail::BnwNormalRootContextEntry>{}:
        actual_root_context_entry_for_paint_without_callbacks();
    const auto indexed_control=index?control_:std::optional<BnwWorldViewControlTree::Control>{};
    const auto control=index?std::optional<std::uint64_t>{actual_checked_control_id_for_paint_without_callbacks()}:
        std::optional<std::uint64_t>{};
    require(index.has_value()==indexed_control.has_value()&&index.has_value()==control.has_value()&&
        index.has_value()!=context_entry.has_value(),
        "Normal GUI producer requires actual indexed Control/index XOR genuine Context entry");
    const auto receiver_first=[&]{
        frame.require_current_without_callbacks();
        if(context_entry){
            if constexpr(std::is_same_v<ActualFrameGuard,detail::BnwNormalRootPaintCallFrame::Guard>){
                context_entry->require_current_without_callbacks();
                const auto actual=actual_root_context_entry_for_paint_without_callbacks();
                require(actual.has_value(),"Context prefix supplier lost original entry");
                context_entry->require_same_entry_without_callbacks(*actual);
                context_entry->require_same_tree_without_callbacks(*tree_);
                frame.require_actual_context_entry_without_callbacks(*this,*context_entry);
                context_entry->require_current_without_callbacks();
            }else require(false,"Context prefix cannot be created through a Label Frame");
        }else frame.require_actual_paint_item_without_callbacks(*this,*index);
        frame.require_current_without_callbacks();
    };
    receiver_first();
    const auto tree=tree_;const auto owner=tree->core_;const auto gui=owner->context_gui_port;
    const auto matrix=std::move(actual_matrix);
    require(gui&&matrix,"Normal GUI batch actual canonical Gui/Matrix missing");
    const auto state=gui->impl_;
    const auto first=[&]{
        receiver_first();
        owner->layout_origin(true);const auto& current=owner->source->current_operation_loan();owner->layout_origin(true);
        require(&current==&operation,"Normal GUI batch current Source differs before supplied operation");
        current.require_origin_without_callbacks();state->first(current);
        require(owner->context_gui_port==gui&&gui->impl_==state&&state->canonical_gui.lock()==gui,
            "Normal GUI batch canonical owner replaced during prefix");
        matrix->require_current_without_callbacks();
        require(matrix->impl_->gui.lock()==state&&matrix->impl_->facade.lock()==gui,
            "Normal GUI batch current Matrix belongs to another manager");
        if(index){
            require(owner->checked(*indexed_control)==*control,
                "Normal GUI producer original Control belongs to another Tree");
            frame.require_actual_control_without_callbacks(*tree,*control);
        }
        receiver_first();
    };
    const auto full=[&]{first();frame.require_source_owned(operation);first();matrix->require_source_owned(operation);first();};
    first();full();
    require(!state->normal_gui_batch_beginning&&state->normal_gui_batch_revision<UINT64_MAX,
        "Normal GUI batch recursive begin/revision exhausted");
    {const auto previous=state->normal_gui_batch.lock();require(!previous||!previous->alive.load(),
        "Normal GUI batch already has an actual live lexical execution");}
    const auto serial=operation.operation_serial(),before=state->normal_gui_batch_revision;
    const auto offset=matrix->arithmetic_words_without_callbacks();first();
    const auto dimensions=matrix->dimensions_without_callbacks();first();
    const auto numeric=frame.actual_numeric_profile_without_callbacks();first();
    const auto matrix_numeric=matrix->numeric_controls_without_callbacks();first();
    require(numeric[0]==matrix_numeric[0]&&(numeric[1]&~std::uint32_t{0x3f})==
        (matrix_numeric[1]&~std::uint32_t{0x3f}),"Normal GUI batch Frame/Matrix raw CW/MXCSR differs");
    state->normal_gui_batch_beginning=true;
    struct End{std::shared_ptr<Gui::Impl> state;~End(){state->normal_gui_batch_beginning=false;}}end{state};
    const auto pending=[&]{first();require(state->normal_gui_batch_beginning&&
        state->normal_gui_batch_revision==before&&operation.operation_serial()==serial&&
        matrix->arithmetic_words_without_callbacks()==offset&&matrix->dimensions_without_callbacks()==dimensions,
        "Normal GUI batch allocating prefix POST changed source/mutable operands");};
    std::shared_ptr<Batch> batch;
    try{
        pending();full();pending();batch=std::make_shared<Batch>();pending();full();pending();
        const auto*const actual_publication=this;const auto*const op=&operation;
        //All constructor args below are OWN VALUES before allocator/new-handler.
        auto data=std::make_shared<Loan::Impl>(gui,state,batch,matrix,frame,tree,
            actual_publication,index,indexed_control,context_entry,control,serial,before+1,op);pending();full();pending();
        auto result=std::unique_ptr<Loan>(new Loan(data));pending();full();pending();
        //Actual OWN normal branch/8ECC40->8EC430 zero batch record prefix.
        //No caller selector/depth, guessed current Original mode or GL default.
        batch->viewport_selector=0;batch->record(Batch::Effect::viewport_zero);
        batch->root_offset=offset;batch->record(Batch::Effect::root_offset);
        batch->root_alpha=0x3f800000u;batch->record(Batch::Effect::root_alpha);
        batch->root_depth[0]=0x3f800000u;batch->record(Batch::Effect::root_arg4);
        batch->root_depth[1]=0;batch->record(Batch::Effect::root_arg5);
        //Ported zero-viewport policy stores. Native performs SetViewport in
        //later flush; this prefix never claims that physical call was performed.
        batch->dimensions=dimensions;batch->record(Batch::Effect::viewport_dimensions);
        batch->viewport_depth[0]=0;batch->record(Batch::Effect::viewport_min_z);
        batch->viewport_depth[1]=0x3f800000u;batch->record(Batch::Effect::viewport_max_z);
        batch->revision=before+1;batch->record(Batch::Effect::entered);
        pending();full();pending();
        //No allocations/callbacks between actual current-batch stores and seal.
        state->normal_gui_batch_revision=before+1;state->normal_gui_batch=batch;
        state->normal_gui_batch_prefix=data; //Closed weak control, original facade retires alive.
        state->normal_gui_batch_beginning=false;data->sealed=true;
        data->first();data->full(operation);data->first();return result;
    }catch(...){if(batch)batch->alive.store(false);state->failed.store(true);throw;}
}
