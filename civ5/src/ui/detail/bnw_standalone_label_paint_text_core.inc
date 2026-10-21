// Separate originating Source8 Guard. No generic DTO/embedded conversion.
void BnwWorldViewControlRenderPublication::require_normal_label_run_for_actual_item_without_callbacks(
    std::size_t index,const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwLabelActualPaintCallFrame::Guard& frame,
    const std::shared_ptr<const detail::BnwStandalonePositionedLabelRunLoan>& run)const{
    frame.require_current_without_callbacks();frame.require_actual_paint_item_without_callbacks(*this,index);
    const auto emission=normal_label_emission_;require(bool(emission),"Standalone Paint actual Normal10 emission missing");
    const auto data=emission->impl_;data->first(true);const auto call=data->invocation;
    const auto& current=call->first(true);require(&current==&operation,
        "Standalone Paint Normal10 requires canonical SAME Source before operand borrow");
    const auto tree=call->tree.lock();require(tree&&tree_.get()==tree.get()&&!tree_.owner_before(tree)&&!tree.owner_before(tree_),
        "Standalone Paint Normal10 another Tree");
    frame.require_actual_control_without_callbacks(*tree,call->id);
    require(normal_label_emission_.get()==emission.get()&&data->index==index&&data->index<data->count&&
        frame_.paints.size()==data->count&&topology_revision_==call->topology&&effect_revision_==call->effects&&
        actual_checked_control_id_for_paint_without_callbacks()==call->id&&call->selector==1&&
        std::find(paint_indices_.begin(),paint_indices_.end(),index)!=paint_indices_.end()&&run&&call->run&&
        run.get()==call->run.get()&&!run.owner_before(call->run)&&!call->run.owner_before(run),
        "Standalone Paint not SAME normal invocation/index/physical completed Run");
    data->first(true);frame.require_actual_paint_item_without_callbacks(*this,index);frame.require_current_without_callbacks();
}
struct detail::BnwLabelPaintTextLoan::StandaloneImpl {
    const std::thread::id creator=std::this_thread::get_id();std::atomic<bool> alive{true};
    const std::shared_ptr<const BnwStandalonePositionedLabelRunLoan> run_facade;
    const BnwStandalonePositionedLabelRunLoan::Guard run;
    const BnwLabelActualPaintCallFrame::Guard frame;
    const std::shared_ptr<const BnwWorldViewControlTree> tree;
    const std::uint64_t control;
    const BnwWorldViewControlRenderPublication*const publication;
    const std::size_t index;
    const std::shared_ptr<const BnwLabelSystemFontBorrower> system;
    StandaloneImpl(std::shared_ptr<const BnwStandalonePositionedLabelRunLoan> facade,
        BnwStandalonePositionedLabelRunLoan::Guard guard,BnwLabelActualPaintCallFrame::Guard actual_frame,
        std::shared_ptr<const BnwWorldViewControlTree> actual_tree,std::uint64_t actual_control,
        const BnwWorldViewControlRenderPublication& actual_publication,std::size_t actual_index,
        std::shared_ptr<const BnwLabelSystemFontBorrower> actual_system)
        :run_facade(std::move(facade)),run(std::move(guard)),frame(std::move(actual_frame)),
         tree(std::move(actual_tree)),control(actual_control),publication(&actual_publication),index(actual_index),
         system(std::move(actual_system)){first();}
    void first()const{
        require(creator==std::this_thread::get_id()&&alive.load(),"Standalone Paint lexical owner retired/foreign");
        // Independent Frame FIRST before borrowing the Publication pointer;
        // exact item association remains lexical, not renewed by Run storage.
        frame.require_current_without_callbacks();run.require_current_without_callbacks();
        const auto original=frame.original_publication_without_callbacks();
        require(original.get()==publication,"Standalone Paint lost actual Frame original Publication");
        original->require_no_caption_fp32_rect_for_legacy_paint_without_callbacks(frame);
        frame.require_actual_paint_item_without_callbacks(*publication,index);
        require(tree&&run_facade&&system,"Standalone Paint genuine owners absent");
        publication->require_normal_label_run_for_actual_item_without_callbacks(index,*run.operation_,frame,run_facade);
        run.require_actual_control_without_callbacks(*tree,control);
        frame.require_actual_control_without_callbacks(*tree,control);
        run.require_system_font_borrower_without_callbacks(*system);
        const auto bytes=run.actual_c4_bytes_without_callbacks();
        require(!bytes.empty()&&bytes.back()=='\0',"Standalone Paint actual C4 NUL absent");
        frame.require_current_without_callbacks();run.require_current_without_callbacks();
        frame.require_actual_paint_item_without_callbacks(*publication,index);
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
        // Run FIRST resolves canonical Source before any saved-op method.
        first();run.require_source_owned(operation);first();frame.require_source_owned(operation);first();
    }
};
detail::BnwLabelPaintTextLoan::BnwLabelPaintTextLoan(std::shared_ptr<StandaloneImpl> data)
    :standalone_(std::move(data)),impl_{}{}
detail::BnwLabelPaintTextLoan::Guard::Guard(std::shared_ptr<StandaloneImpl> data)
    :standalone_(std::move(data)),impl_{}{}
void detail::BnwLabelPaintTextLoan::Guard::require_same_paint_guard_without_callbacks(const Guard& other)const{
    require_current_without_callbacks();other.require_current_without_callbacks();
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require(same(impl_,other.impl_)&&same(standalone_,other.standalone_)&&bool(impl_)!=bool(standalone_),
        "Prepared receipt another originating typed lexical Paint");
    require_current_without_callbacks();other.require_current_without_callbacks();
}
std::unique_ptr<detail::BnwLabelPaintTextLoan> detail::BnwLabelPaintTextLoan::begin_standalone_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const BnwWorldViewControlTree>& actual_tree,std::uint64_t actual_id,
    std::shared_ptr<const BnwStandalonePositionedLabelRunLoan> actual_run,
    const BnwWorldViewControlRenderPublication& publication,std::size_t index,
    const BnwLabelActualPaintCallFrame& actual_frame){
    const auto tree=actual_tree;const auto facade=std::move(actual_run);
    const auto frame=actual_frame.capture_guard_without_callbacks();frame.require_current_without_callbacks();
    const auto original=frame.original_publication_without_callbacks();
    require(original.get()==&publication,"Standalone Paint supplied another original Publication");
    original->require_no_caption_fp32_rect_for_legacy_paint_without_callbacks(frame);
    frame.require_actual_paint_item_without_callbacks(publication,index);
    require(tree&&facade,"Standalone Paint genuine Tree/Run missing");
    const auto run=facade->capture_guard_for_paint_frame_guard_owned(operation,tree,actual_id,frame);
    const auto first=[&]{frame.require_current_without_callbacks();run.require_current_without_callbacks();
        frame.require_actual_paint_item_without_callbacks(publication,index);
        frame.require_actual_control_without_callbacks(*tree,actual_id);
        run.require_actual_control_without_callbacks(*tree,actual_id);run.require_current_without_callbacks();};
    const auto full=[&]{first();run.require_source_owned(operation);first();frame.require_source_owned(operation);first();};
    first();full();
    // Closed callback-free Normal10 association under captured Frame; no
    // reread of a raw Frame/Run facade after a callback or allocation.
    publication.require_normal_label_run_for_actual_item_without_callbacks(index,operation,frame,facade);first();
    const auto system=run.actual_system_font_without_callbacks();first();
    require(bool(system),"Standalone Paint genuine bitmap NULL170 unsupported; not missing-system success");
    full();auto data=std::make_shared<StandaloneImpl>(facade,run,frame,tree,actual_id,publication,index,system);
    first();data->first();data->full(operation);first();
    auto result=std::unique_ptr<BnwLabelPaintTextLoan>(new BnwLabelPaintTextLoan(data));
    data->first();data->full(operation);data->first();return result;
}
