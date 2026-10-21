// One genuine preappend FP32 rectangle owner. The issuer observes the already
// initialized original Run; it neither enters a Frame nor emits an item.
struct detail::BnwNormalCaptionFp32RectLoan::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const BnwNormalButtonCaptionCall caption;
    const std::shared_ptr<const BnwPositionedLabelRunLoan> run;
    const BnwNormalRootBatchHistory::Guard root;
    const std::array<std::uint32_t,4> own_geometry_28_34;
    const std::array<std::uint32_t,3> caption_arguments_xy_alpha;
    const std::array<std::uint32_t,2> numeric;
    const std::array<std::uint32_t,4> words;
    std::weak_ptr<const Impl> canonical_self;
    std::weak_ptr<const BnwNormalCaptionFp32RectLoan> canonical_facade;
    bool sealed=false;
    Impl(BnwNormalButtonCaptionCall actual_caption,
        std::shared_ptr<const BnwPositionedLabelRunLoan> actual_run,
        BnwNormalRootBatchHistory::Guard actual_root,
        std::array<std::uint32_t,4> actual_geometry,
        std::array<std::uint32_t,3> actual_arguments,
        std::array<std::uint32_t,2> actual_numeric,
        std::array<std::uint32_t,4> actual_words)
        :caption(std::move(actual_caption)),run(std::move(actual_run)),root(std::move(actual_root)),
         own_geometry_28_34(actual_geometry),caption_arguments_xy_alpha(actual_arguments),
         numeric(actual_numeric),words(actual_words){}
};

void BnwWorldViewControlTree::require_normal_button_caption_fp32_rect_current_without_callbacks(
    const detail::BnwNormalCaptionFp32RectLoan& offered){
    const auto saved=offered.impl_;
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require(saved&&saved->creator==std::this_thread::get_id()&&saved->sealed&&
        same(saved->canonical_self.lock(),saved),
        "Caption FP32 rect has foreign/unsealed/noncanonical original storage");
    // CURRENT Common/Root/Source FIRST precedes any saved Run/field access.
    require_normal_button_caption_call_current_without_callbacks(saved->caption);
    const auto caption=saved->caption.impl_;
    const auto facade=saved->canonical_facade.lock();
    require(facade&&facade.get()==&offered&&same(facade->impl_,saved)&&
        caption->original_fp32_rect_control&&same(caption->original_fp32_rect_control->lock(),saved)&&
        !caption->capturing_fp32_rect&&!caption->projecting&&caption->paint_arguments&&
        same(caption->run,saved->run),
        "Caption FP32 rect is not SAME original once-issued owner/initialized Run");
    saved->root.require_same_root_without_callbacks(caption->root);
    const auto actual=saved->run->impl_;
    require(actual&&actual->creator==saved->creator&&actual->alive.load(),
        "Caption FP32 rect original initialized Run retired/foreign");
    actual->require_published_data_without_callbacks();
    const std::array<std::uint32_t,4> own{
        actual->value.geometry[6],actual->value.geometry[7],
        actual->value.geometry[8],actual->value.geometry[9]};
    require(own==saved->own_geometry_28_34&&
        caption->paint_arguments->result==saved->caption_arguments_xy_alpha&&
        caption->paint_arguments->numeric==saved->numeric&&
        saved->words[2]==own[2]&&saved->words[3]==own[3],
        "Caption FP32 rect actual Label28/2C/30/34 or cached incoming operands changed");
    detail::BnwWindowedRendererMatrixDataLoan::Impl::same_controls(saved->numeric);
    require_normal_button_caption_call_current_without_callbacks(saved->caption);
    saved->root.require_physical_current_without_callbacks();
}

void BnwWorldViewControlTree::require_normal_button_caption_fp32_rect_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalCaptionFp32RectLoan& offered){
    // Own the original immutable physical control before callback cuts.
    const auto saved=offered.impl_;
    require_normal_button_caption_fp32_rect_current_without_callbacks(offered);
    const auto owner=saved->canonical_facade.lock();
    require(owner&&owner.get()==&offered,"Caption FP32 rect canonical facade unavailable before Source cut");
    require_normal_button_caption_call_source_owned(operation,saved->caption);
    require_normal_button_caption_fp32_rect_current_without_callbacks(*owner);
}

void BnwWorldViewControlTree::require_normal_button_caption_fp32_rect_same_caption_without_callbacks(
    const detail::BnwNormalCaptionFp32RectLoan& offered,
    const detail::BnwNormalButtonCaptionCall& supplied){
    const auto caption=supplied;
    require_normal_button_caption_fp32_rect_current_without_callbacks(offered);
    require_normal_button_caption_call_current_without_callbacks(caption);
    const auto actual=offered.impl_->caption.impl_;const auto candidate=caption.impl_;
    require(actual.get()==candidate.get()&&!actual.owner_before(candidate)&&!candidate.owner_before(actual),
        "Caption FP32 rect belongs to another actual Caption call control");
    require_normal_button_caption_call_current_without_callbacks(caption);
    require_normal_button_caption_fp32_rect_current_without_callbacks(offered);
}

void BnwWorldViewControlTree::require_normal_button_caption_fp32_rect_same_initialized_run_without_callbacks(
    const detail::BnwNormalCaptionFp32RectLoan& offered,
    const std::shared_ptr<const detail::BnwPositionedLabelRunLoan>& supplied){
    const auto run=supplied;
    require_normal_button_caption_fp32_rect_current_without_callbacks(offered);
    const auto actual=offered.impl_->run;
    require(run&&actual.get()==run.get()&&!actual.owner_before(run)&&!run.owner_before(actual)&&
        actual->impl_.get()==run->impl_.get()&&!actual->impl_.owner_before(run->impl_)&&
        !run->impl_.owner_before(actual->impl_),
        "Caption FP32 rect initialized Run facade/control is not SAME physical original");
    require_normal_button_caption_fp32_rect_current_without_callbacks(offered);
}

void BnwWorldViewControlTree::require_normal_button_caption_fp32_rect_same_entered_run_without_callbacks(
    const detail::BnwNormalCaptionFp32RectLoan& offered,
    const detail::BnwPositionedLabelRunLoan::Guard& supplied){
    // Guard VALUE retains its genuine original entered Frame/Run before FIRST.
    // No new Guard, frame, paint index, publication or epoch is created here.
    const auto entered=supplied;
    entered.require_current_without_callbacks();
    require_normal_button_caption_fp32_rect_current_without_callbacks(offered);
    const auto saved=offered.impl_;const auto caption=saved->caption.impl_;
    const auto actual=saved->run->impl_;const auto tree=caption->root.impl_->tree;
    require(entered.paint_frame_&&entered.paint_tree_&&
        entered.paint_tree_.get()==tree.get()&&!entered.paint_tree_.owner_before(tree)&&
        !tree.owner_before(entered.paint_tree_)&&
        entered.impl_.get()==actual.get()&&!entered.impl_.owner_before(actual)&&
        !actual.owner_before(entered.impl_)&&
        entered.operation_==caption->root.impl_->operation&&entered.serial_==caption->serial,
        "Caption FP32 rect association requires SAME actual entered Frame/Run/Tree/Source");
    entered.require_actual_control_without_callbacks(*tree,caption->control.id_);
    const auto geometry=entered.actual_geometry_bits_without_callbacks();
    require(std::array<std::uint32_t,4>{geometry[6],geometry[7],geometry[8],geometry[9]}==
        saved->own_geometry_28_34,
        "Caption FP32 rect entered Run changed its original geometry words");
    entered.require_current_without_callbacks();
    require_normal_button_caption_fp32_rect_current_without_callbacks(offered);
}

std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan>
BnwWorldViewControlTree::capture_normal_button_caption_fp32_rect_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalButtonCaptionCall& supplied)const{
    using Rect=detail::BnwNormalCaptionFp32RectLoan;
    using Caption=detail::BnwNormalButtonCaptionCall;
    const auto caption=supplied;
    const auto first=[&]{require_normal_button_caption_call_current_without_callbacks(caption);
        const auto original=caption.impl_;
        require(original->root.impl_->tree.get()==this&&original->paint_arguments&&
            !original->projecting&&!original->original_emission_control&&!original->original_fp32_rect_control,
            "Caption FP32 rect issuer requires actual preappend cached arguments and no previous issuance");};
    const auto full=[&]{first();require_normal_button_caption_call_source_owned(operation,caption);first();};
    first();const auto original=caption.impl_;
    require(!original->capturing_fp32_rect,"Caption FP32 rect issuer reentered before original allocation");
    original->capturing_fp32_rect=true;
    struct CaptureExit final {const std::shared_ptr<const Caption::Impl> owner;
        ~CaptureExit(){owner->capturing_fp32_rect=false;}}capture_exit{original};
    try{
        full();first();
        const auto run=original->run;const auto root=original->root;
        const auto actual=run->impl_;
        const std::array<std::uint32_t,4> own{
            actual->value.geometry[6],actual->value.geometry[7],
            actual->value.geometry[8],actual->value.geometry[9]};
        const auto arguments=*original->paint_arguments;
        const auto numeric=arguments.numeric;
        const auto pending_first=[&]{first();
            const auto current=original->run->impl_;
            require(original->capturing_fp32_rect&&original->run.get()==run.get()&&
                !original->run.owner_before(run)&&!run.owner_before(original->run)&&
                current.get()==actual.get()&&!current.owner_before(actual)&&!actual.owner_before(current)&&
                std::array<std::uint32_t,4>{current->value.geometry[6],current->value.geometry[7],
                    current->value.geometry[8],current->value.geometry[9]}==own&&
                original->paint_arguments->result==arguments.result&&
                original->paint_arguments->numeric==numeric,
                "Caption FP32 rect allocator/Source cut changed SAME original owned operands");
            root.require_same_root_without_callbacks(original->root);
            detail::BnwWindowedRendererMatrixDataLoan::Impl::same_controls(numeric);first();};
        const auto pending_full=[&]{pending_first();full();pending_first();};
        pending_first();pending_full();pending_first();
        // FP32 ADDSS uses own Label28/2C as its first operands, exactly as the
        // real Label position actor. Width30/height34 are copied, never guessed.
        std::array<std::uint32_t,2> xy;
        asm volatile("movss %[ownx], %%xmm0\n\t"
                     "addss %[incomingx], %%xmm0\n\t"
                     "movss %%xmm0, %[outx]\n\t"
                     "movss %[owny], %%xmm1\n\t"
                     "addss %[incomingy], %%xmm1\n\t"
                     "movss %%xmm1, %[outy]\n\t"
                     :[outx]"=m"(xy[0]),[outy]"=m"(xy[1])
                     :[ownx]"m"(own[0]),[owny]"m"(own[1]),
                      [incomingx]"m"(arguments.result[0]),[incomingy]"m"(arguments.result[1])
                     :"xmm0","xmm1","memory");
        pending_first();
        const std::array<std::uint32_t,4> words{xy[0],xy[1],own[2],own[3]};
        auto data=std::make_shared<Rect::Impl>(caption,run,root,own,arguments.result,numeric,words);
        pending_first();pending_full();pending_first();
        const Rect::ConstructionKey key;
        auto result=std::shared_ptr<Rect>(new Rect(key,data));
        pending_first();pending_full();pending_first();
        require(data->words==words&&data->own_geometry_28_34==own&&
            data->run.get()==run.get()&&!data->run.owner_before(run)&&!run.owner_before(data->run)&&
            data->caption.impl_.get()==original.get()&&!data->caption.impl_.owner_before(original)&&
            !original.owner_before(data->caption.impl_),
            "Caption FP32 rect allocated storage lost SAME original Caption/Run/word association");
        // No callback/allocation between these once-only weak publications.
        data->canonical_self=data;data->canonical_facade=result;data->sealed=true;
        original->original_fp32_rect_control=std::weak_ptr<const Rect::Impl>(data);
        original->capturing_fp32_rect=false;
        require_normal_button_caption_fp32_rect_current_without_callbacks(*result);
        require_normal_button_caption_fp32_rect_source_owned(operation,*result);
        require_normal_button_caption_fp32_rect_current_without_callbacks(*result);return result;
    }catch(...){const auto root=original->root.impl_;
        if(root&&root->tree)root->tree->core_->failed.store(true);throw;}
}

detail::BnwNormalCaptionFp32RectLoan::BnwNormalCaptionFp32RectLoan(
    const ConstructionKey&,std::shared_ptr<const Impl> actual):impl_(std::move(actual)){}
void detail::BnwNormalCaptionFp32RectLoan::require_current_without_callbacks()const{
    BnwWorldViewControlTree::require_normal_button_caption_fp32_rect_current_without_callbacks(*this);
}
void detail::BnwNormalCaptionFp32RectLoan::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    BnwWorldViewControlTree::require_normal_button_caption_fp32_rect_source_owned(operation,*this);
}
void detail::BnwNormalCaptionFp32RectLoan::require_same_caption_without_callbacks(
    const BnwNormalButtonCaptionCall& caption)const{
    BnwWorldViewControlTree::require_normal_button_caption_fp32_rect_same_caption_without_callbacks(*this,caption);
}
void detail::BnwNormalCaptionFp32RectLoan::require_same_initialized_run_without_callbacks(
    const std::shared_ptr<const BnwPositionedLabelRunLoan>& run)const{
    BnwWorldViewControlTree::require_normal_button_caption_fp32_rect_same_initialized_run_without_callbacks(*this,run);
}
void detail::BnwNormalCaptionFp32RectLoan::require_same_entered_run_without_callbacks(
    const BnwPositionedLabelRunLoan::Guard& entered)const{
    BnwWorldViewControlTree::require_normal_button_caption_fp32_rect_same_entered_run_without_callbacks(*this,entered);
}
std::array<std::uint32_t,4> detail::BnwNormalCaptionFp32RectLoan::raw_words_without_callbacks()const{
    require_current_without_callbacks();const auto words=impl_->words;
    require_current_without_callbacks();return words;
}
