#include "bnw_standalone_label_paint_text_core.inc"
// Private DATA lender after the complete Run implementation. Renderer owns
// the actual591020 frame implementation; no Impl crosses a TU boundary.
struct detail::BnwLabelPaintTextLoan::Impl {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true};
    const BnwPositionedLabelRunLoan::Guard run;
    const BnwLabelActualPaintCallFrame::Guard frame;
    const std::weak_ptr<const BnwWorldViewControlTree> tree;
    const std::uint64_t actual_control_id;
    const std::shared_ptr<const BnwLabelSystemFontBorrower> system;
    const std::shared_ptr<const BnwNormalCaptionFp32RectLoan> caption_rect{};
    const std::shared_ptr<const BnwWorldViewControlRenderPublication> caption_publication{};
    const std::optional<std::size_t> caption_index{};
    Impl(BnwPositionedLabelRunLoan::Guard actual_run,BnwLabelActualPaintCallFrame::Guard actual_frame,
        std::shared_ptr<const BnwWorldViewControlTree> actual_tree,std::uint64_t actual_id,
        std::shared_ptr<const BnwLabelSystemFontBorrower> actual_system,
        const runtime::BnwWorldViewSourceOperationLoan& operation)
        :run(std::move(actual_run)),frame(std::move(actual_frame)),tree(actual_tree),
         actual_control_id(actual_id),system(std::move(actual_system)){
        first();full(operation);first();
    }
    Impl(BnwPositionedLabelRunLoan::Guard actual_run,BnwLabelActualPaintCallFrame::Guard actual_frame,
        std::shared_ptr<const BnwWorldViewControlTree> actual_tree,std::uint64_t actual_id,
        std::shared_ptr<const BnwLabelSystemFontBorrower> actual_system,
        const runtime::BnwWorldViewSourceOperationLoan& operation,
        std::shared_ptr<const BnwNormalCaptionFp32RectLoan> actual_rect,
        std::shared_ptr<const BnwWorldViewControlRenderPublication> actual_publication,std::size_t index)
        :run(std::move(actual_run)),frame(std::move(actual_frame)),tree(actual_tree),
         actual_control_id(actual_id),system(std::move(actual_system)),caption_rect(std::move(actual_rect)),
         caption_publication(std::move(actual_publication)),caption_index(index){
        require(caption_rect&&caption_publication,"Caption Paint original owners absent");
        first();full(operation);first();
    }
    void first()const{
        require(creator==std::this_thread::get_id()&&alive.load(),"Paint text lexical loan retired/foreign");
        //Each sealed producer resolves its live canonical owner before any
        //borrowed operation/scope access. Physical retention renews neither.
        run.require_current_without_callbacks();frame.require_current_without_callbacks();
        run.require_current_without_callbacks();
        const auto owner=tree.lock();require(bool(owner),"Paint text actual renderer Tree retired");
        run.require_actual_control_without_callbacks(*owner,actual_control_id);
        frame.require_actual_control_without_callbacks(*owner,actual_control_id);
        run.require_current_without_callbacks();frame.require_current_without_callbacks();
        const auto original=frame.original_publication_without_callbacks();
        if(caption_rect){
            require(caption_publication&&caption_index&&original.get()==caption_publication.get()&&
                !original.owner_before(caption_publication)&&!caption_publication.owner_before(original),
                "Caption Paint changed original Publication control");
            const auto actual=original->caption_fp32_rect_for_actual_item_without_callbacks(*caption_index,frame);
            require(actual.get()==caption_rect.get()&&!actual.owner_before(caption_rect)&&!caption_rect.owner_before(actual),
                "Caption Paint lost SAME immutable indexed FP32 loan");
            caption_rect->require_same_entered_run_without_callbacks(run);
        }else{
            require(!caption_publication&&!caption_index,"Legacy Paint has partial caption originals");
            original->require_no_caption_fp32_rect_for_legacy_paint_without_callbacks(frame);
        }
        require(bool(system),"Paint text actual170 system-font path is unavailable");
        run.require_system_font_borrower_without_callbacks(*system);
        frame.require_current_without_callbacks();
        const auto bytes=run.actual_c4_bytes_without_callbacks();
        require(!bytes.empty()&&bytes.back()=='\0',"Paint text actual full C4 terminator unavailable");
        frame.require_current_without_callbacks();run.require_current_without_callbacks();
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
        first();frame.require_source_owned(operation);first();
        run.require_source_owned(operation);first();
        frame.require_source_owned(operation);first();
        if(caption_rect){caption_rect->require_source_owned(operation);first();}
    }
};

detail::BnwLabelPaintTextLoan::BnwLabelPaintTextLoan(std::shared_ptr<Impl> data):impl_(std::move(data)){}
detail::BnwLabelPaintTextLoan::~BnwLabelPaintTextLoan(){if(standalone_){standalone_->alive.store(false);return;}impl_->alive.store(false);}
detail::BnwLabelPaintTextLoan::Guard::Guard(std::shared_ptr<Impl> data):impl_(std::move(data)){}
detail::BnwLabelPaintTextLoan::Guard detail::BnwLabelPaintTextLoan::capture_guard_without_callbacks()const{
    if(standalone_){const auto data=standalone_;data->first();return Guard(data);}
    const auto data=impl_;data->first();return Guard(data);
}
void detail::BnwLabelPaintTextLoan::Guard::require_current_without_callbacks()const{if(standalone_){standalone_->first();return;}impl_->first();}
void detail::BnwLabelPaintTextLoan::Guard::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{if(standalone_){standalone_->full(operation);return;}impl_->full(operation);}
void detail::BnwLabelPaintTextLoan::Guard::require_system_font_borrower_without_callbacks(
    const BnwLabelSystemFontBorrower& actual)const{
    if(standalone_){standalone_->first();standalone_->run.require_system_font_borrower_without_callbacks(actual);standalone_->first();return;}
    impl_->first();require(impl_->system.get()==&actual,"Paint text is for another actual170 borrower");
    impl_->run.require_system_font_borrower_without_callbacks(actual);impl_->first();
}
void detail::BnwLabelPaintTextLoan::Guard::require_actual_renderer_without_callbacks(
    const UiScreenGlRenderer& actual)const{
    if(standalone_){standalone_->first();standalone_->frame.require_actual_renderer_without_callbacks(actual);standalone_->first();return;}
    impl_->first();impl_->frame.require_actual_renderer_without_callbacks(actual);impl_->first();
}
std::string_view detail::BnwLabelPaintTextLoan::Guard::actual_utf8_without_callbacks()const{
    if(standalone_){standalone_->first();const auto bytes=standalone_->run.actual_c4_bytes_without_callbacks();
        const auto end=std::find(bytes.begin(),bytes.end(),'\0');require(end!=bytes.end(),"Standalone Paint true C4 NUL missing");
        const std::string_view out(bytes.data(),static_cast<std::size_t>(end-bytes.begin()));standalone_->first();return out;}
    impl_->first();const auto bytes=impl_->run.actual_c4_bytes_without_callbacks();
    const auto end=std::find(bytes.begin(),bytes.end(),'\0');
    require(end!=bytes.end(),"Paint text actual C4 NUL unavailable");
    const std::string_view result(bytes.data(),static_cast<std::size_t>(end-bytes.begin()));
    impl_->first();return result;
}
std::string_view detail::BnwLabelPaintTextLoan::Guard::actual_c4_bytes_without_callbacks()const{
    if(standalone_){standalone_->first();const auto out=standalone_->run.actual_c4_bytes_without_callbacks();standalone_->first();return out;}
    impl_->first();const auto result=impl_->run.actual_c4_bytes_without_callbacks();impl_->first();return result;
}
std::array<std::uint32_t,2> detail::BnwLabelPaintTextLoan::Guard::actual_numeric_profile_without_callbacks()const{
    if(standalone_){standalone_->first();const auto out=standalone_->frame.actual_numeric_profile_without_callbacks();standalone_->first();return out;}
    impl_->first();const auto result=impl_->frame.actual_numeric_profile_without_callbacks();impl_->first();return result;
}
std::unique_ptr<detail::BnwLabelPaintTextLoan> detail::BnwLabelPaintTextLoan::begin_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const BnwWorldViewControlTree>& actual_tree,std::uint64_t actual_control_id,
    const BnwPositionedLabelRunLoan& actual_run,const BnwLabelActualPaintCallFrame& actual_frame){
    //Snapshot ALL retained operands by VALUE before callbacks/allocators.
    //Do not reread either facade or an lvalue to a renderer registry entry.
    const auto tree_snapshot=actual_tree;
    const auto frame_snapshot=actual_frame.capture_guard_without_callbacks();
    const auto original_publication=frame_snapshot.original_publication_without_callbacks();
    original_publication->require_no_caption_fp32_rect_for_legacy_paint_without_callbacks(frame_snapshot);
    const auto run_snapshot=actual_run.capture_guard_for_paint_owned(operation,tree_snapshot,actual_control_id,frame_snapshot);
    const auto first=[&]{
        run_snapshot.require_current_without_callbacks();frame_snapshot.require_current_without_callbacks();
        require(bool(tree_snapshot),"Paint text entered actual Tree absent");
        run_snapshot.require_actual_control_without_callbacks(*tree_snapshot,actual_control_id);
        frame_snapshot.require_actual_control_without_callbacks(*tree_snapshot,actual_control_id);
        run_snapshot.require_current_without_callbacks();frame_snapshot.require_current_without_callbacks();
    };
    first();frame_snapshot.require_source_owned(operation);first();
    run_snapshot.require_source_owned(operation);first();
    const auto system_snapshot=run_snapshot.actual_system_font_without_callbacks();first();
    require(bool(system_snapshot),"Paint text real170 is NULL; system-font painter unavailable");
    auto data=std::make_shared<Impl>(run_snapshot,frame_snapshot,tree_snapshot,actual_control_id,
        system_snapshot,operation);
    data->first();data->full(operation);data->first();
    auto result=std::unique_ptr<BnwLabelPaintTextLoan>(new BnwLabelPaintTextLoan(data));
    data->first();data->full(operation);data->first();
    //No callback/allocation follows the final full POST. Destruction retires
    //alive FIRST; Guards retain only physical immutable C4 DATA thereafter.
    return result;
}

detail::BnwLabelPaintTextLoan::Guard::RecordData
detail::BnwLabelPaintTextLoan::Guard::actual_record_without_callbacks()const{
    if(standalone_){standalone_->first();const auto record=standalone_->run.actual_record_without_callbacks();standalone_->first();
        const auto style=standalone_->run.actual_style_pass_selectors_without_callbacks();standalone_->first();
        const auto metrics=standalone_->run.actual_font_metrics_without_callbacks();standalone_->first();
        RecordData out{record.count,record.begin,record.byte_count,record.style,record.source_discriminator,record.inline_payload,
            record.packed_colors,style,metrics,record.newline,record.leading,record.truncate,record.flags};standalone_->first();return out;}
    impl_->first();const auto record=impl_->run.actual_record_without_callbacks();impl_->first();
    //No callback/allocation occurs in the Font DATA leaf accessors. Keep
    //canonical Paint+Run FIRST before/after each actual closed producer.
    const auto style=impl_->run.actual_style_pass_selectors_without_callbacks();impl_->first();
    const auto metrics=impl_->run.actual_font_metrics_without_callbacks();impl_->first();
    RecordData result{record.count,record.begin,record.byte_count,record.style,
        record.source_discriminator,record.inline_payload,record.packed_colors,
        style,metrics,record.newline,record.leading,record.truncate,record.flags};
    impl_->first();return result;
}
#include "bnw_label_paint_origin_data_getters_core.inc"
