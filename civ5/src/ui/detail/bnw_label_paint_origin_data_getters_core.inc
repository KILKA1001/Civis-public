// These closed DATA accessors retain the actual originating Run lane.
detail::BnwLabelPaintTextLoan::Guard::PositionedRecordData
detail::BnwLabelPaintTextLoan::Guard::actual_positioned_record_without_callbacks()const{
    require_current_without_callbacks();
    if(standalone_){const auto r=standalone_->run.actual_record_without_callbacks();require_current_without_callbacks();
        return {r.count,r.begin,r.byte_count,r.style,r.source_discriminator,r.inline_payload,r.packed_colors,
            r.newline,r.leading,r.truncate,r.flags};}
    const auto r=impl_->run.actual_record_without_callbacks();require_current_without_callbacks();
    return {r.count,r.begin,r.byte_count,r.style,r.source_discriminator,r.inline_payload,r.packed_colors,
        r.newline,r.leading,r.truncate,r.flags};
}
std::array<std::uint32_t,3>
detail::BnwLabelPaintTextLoan::Guard::actual_positioned_style_passes_without_callbacks()const{
    require_current_without_callbacks();const auto out=standalone_?
        standalone_->run.actual_style_pass_selectors_without_callbacks():impl_->run.actual_style_pass_selectors_without_callbacks();
    require_current_without_callbacks();return out;
}
std::array<std::int32_t,3>
detail::BnwLabelPaintTextLoan::Guard::actual_positioned_font_metrics_without_callbacks()const{
    require_current_without_callbacks();const auto out=standalone_?
        standalone_->run.actual_font_metrics_without_callbacks():impl_->run.actual_font_metrics_without_callbacks();
    require_current_without_callbacks();return out;
}
