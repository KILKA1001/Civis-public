void detail::NotificationLayoutStandaloneLabelState::initialize_slot10_body_owned(
    const std::shared_ptr<InitializationLoan>& loan){
    require(loan&&loan->label.get()==this,"Standalone710270 body no genuine fresh issuer");
    try{
        initialize_slot10_prefix_owned(loan);loan->full();impl_->init_stage=Impl::InitStage::defaults_returned;
        require(impl_->history.max_size()-impl_->history.size()>=256&&impl_->sequence<=UINT64_MAX-256&&
            loan->baseline->history_.max_size()-loan->baseline->history_.size()>=256&&loan->baseline->sequence_<=UINT64_MAX-256,
            "Standalone full initializer journals exhausted");
        impl_->history.reserve(impl_->history.size()+256);loan->full();
        loan->baseline->history_.reserve(loan->baseline->history_.size()+256);loan->full();
        initialize_common_5761_owned(loan);loan->full();
        //Genuine71035F→6A2EA0 with real bit1 from5761, not a metadata setter.
        require(std::get<std::uint32_t>(impl_->fields[0x40/4])&1u,"Standalone6A no genuine5761 initialized bit");
        generate_text_segments_5a_owned(loan);loan->full();position_plain_segments_7505_owned(loan);loan->full();
        loan->store(0x16e);impl_->truncated_16e=std::uint8_t(0);loan->first();
        calculate_plain_size_5077_owned(loan);loan->full();
        anchor_slot4c_owned(loan);loan->full();bounds_slot94_owned(loan);loan->full();
        loan->require_actual_empty_children_owned();loan->full(); //Real positive78/zero loop, not NULL substitution.
        impl_->init_stage=Impl::InitStage::bounds_returned;loan->full();
    }catch(...){impl_->failed.store(true);if(const auto binding=loan->binding.lock())binding->poison();throw;}
}
