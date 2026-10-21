// Distinct Source8 entered Run DATA; neither embedded C4 nor its factory lane.
detail::BnwStandalonePositionedLabelRunLoan::Guard
detail::BnwStandalonePositionedLabelRunLoan::capture_guard_for_paint_frame_guard_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    std::shared_ptr<const BnwWorldViewControlTree> actual_tree,std::uint64_t actual_id,
    const BnwLabelActualPaintCallFrame::Guard& actual_frame)const{
    // Value-capture existing typed Frame before callbacks; never revisit facade.
    const auto frame=actual_frame;frame.require_current_without_callbacks();
    const auto data=impl_;const auto tree=std::move(actual_tree);
    require(tree&&actual_id==data->id,"Standalone Paint guard another actual Control");
    frame.require_actual_control_without_callbacks(*tree,actual_id);
    data->first_for_paint(operation,tree,frame);
    const auto serial=operation.operation_serial();frame.require_current_without_callbacks();
    data->first_for_paint(operation,tree,frame);
    Guard out(data,operation,serial,tree,frame);out.require_source_owned(operation);
    out.require_current_without_callbacks();return out;
}
void detail::BnwStandalonePositionedLabelRunLoan::Guard::require_actual_control_without_callbacks(
    const BnwWorldViewControlTree& tree,std::uint64_t id)const{
    require_current_without_callbacks();require(tree_.get()==&tree&&impl_->id==id,
        "Standalone Paint another exact Tree/control");frame_.require_actual_control_without_callbacks(tree,id);
    require_current_without_callbacks();
}
void detail::BnwStandalonePositionedLabelRunLoan::Guard::require_system_font_borrower_without_callbacks(
    const BnwLabelSystemFontBorrower& system)const{
    require_current_without_callbacks();require(impl_->system&&impl_->system.get()==&system,
        "Standalone Paint another physical170 borrower");require_current_without_callbacks();
}
std::string_view detail::BnwStandalonePositionedLabelRunLoan::Guard::actual_c4_bytes_without_callbacks()const{
    require_current_without_callbacks();const auto& bytes=impl_->value.c4;
    require(!bytes.empty()&&bytes.back()=='\0',"Standalone Paint true C4 terminator missing");
    const std::string_view out(bytes.data(),bytes.size());require_current_without_callbacks();return out;
}
detail::BnwStandalonePositionedLabelRunLoan::Guard::RecordData
detail::BnwStandalonePositionedLabelRunLoan::Guard::actual_record_without_callbacks()const{
    require_current_without_callbacks();const auto& v=impl_->value;require(v.count<=1,
        "Standalone Paint actual multi-record owner unsupported");
    RecordData out{v.count,v.descriptor[0],v.descriptor[1],v.descriptor[5],v.descriptor[6],v.descriptor[7],
        {v.descriptor[2],v.descriptor[3],v.descriptor[4]},v.newline,v.leading,v.truncate,v.flags};
    require_current_without_callbacks();return out;
}
std::array<std::uint32_t,3>
detail::BnwStandalonePositionedLabelRunLoan::Guard::actual_style_pass_selectors_without_callbacks()const{
    require_current_without_callbacks();const auto out=impl_->font->style_layer_texture_indices_without_callbacks(
        impl_->value.descriptor[5]);require_current_without_callbacks();return out;
}
std::array<std::int32_t,3>
detail::BnwStandalonePositionedLabelRunLoan::Guard::actual_font_metrics_without_callbacks()const{
    require_current_without_callbacks();const auto out=impl_->font->metrics_without_callbacks();
    require_current_without_callbacks();return out;
}
