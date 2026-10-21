// Inside civ5::ui after complete Caption/Emission/embedded Item bodies. The
// current embedded branch is distinct from the unchanged standalone allocator.
// The same real144 arena and fourteen physical scalar stores are used; this
// entry neither completes Label540B5F nor grants a whole Root or manager drain.
void BnwWorldViewControlTree::require_normal_payload_pending_caption_without_callbacks(
    const detail::BnwNormalGuiBatchPrefix::Impl& prefix,
    const detail::BnwLabelPaintPositionedData::Guard& data,
    const detail::BnwLabelPaintTextLoan::Guard& paint,
    const detail::BnwNormalButtonCaptionCall& caption,
    const detail::BnwNormalItemOccurrence::Guard& item){
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require_normal_button_caption_call_current_without_callbacks(caption);
    require_normal_root_epoch_prefix_without_callbacks(prefix);
    data.require_current_without_callbacks();paint.require_current_without_callbacks();
    data.require_same_paint_guard_without_callbacks(paint);
    const auto call=caption.impl_;const auto root=call->root.impl_;
    const auto actual=data.impl_->normal_item_occurrence.lock();
    require(actual&&data.impl_->normal_item_occurrence_issued&&actual->caption_history&&
        same(actual->caption_history->impl_,call)&&same(actual->root.impl_,root)&&
        root->prefix.get()==&prefix&&same(root->tree,prefix.tree)&&
        root->operation==prefix.operation&&root->serial==prefix.serial&&
        same(actual->tree,prefix.tree)&&actual->operation==prefix.operation&&actual->serial==prefix.serial,
        "Caption payload requires SAME already earned embedded Item/Root/Source, no legacy index admission");
    require(same(item.impl_,actual),
        "Caption payload original retained Item control replaced before use");
    item.require_physical_current_without_callbacks();item.require_same_root_without_callbacks(call->root);
    item.require_actual_positioned_data_without_callbacks(data);
    data.require_same_normal_draw_records_without_callbacks(actual->binding);
    const auto storage=paint.impl_;const auto emission=actual->caption_emission;
    require(storage&&!paint.standalone_&&same(actual->embedded_paint_storage,storage)&&
        !actual->paint_storage&&same(actual->paint.impl_,storage)&&!actual->paint.standalone_&&
        same(storage->run.impl_,call->run->impl_)&&same(storage->tree.lock(),prefix.tree)&&
        storage->actual_control_id==actual->control&&actual->control==call->control.id_&&
        emission&&same(emission->impl_->caption.impl_,call)&&
        same(actual->publication->normal_caption_emission_,emission)&&
        emission->impl_->index==actual->index&&same(emission->impl_->origin,actual->adjusted_frame),
        "Caption payload does not retain SAME entered embedded Run/Paint/DATA/emission controls");
    storage->frame.require_current_without_callbacks();
    storage->frame.require_actual_paint_item_without_callbacks(*actual->publication,actual->index);
    storage->frame.require_actual_control_without_callbacks(*prefix.tree,actual->control);
    require_normal_button_caption_emission_current_without_callbacks(*emission);
    require(call->paint_arguments&&!call->projecting,
        "Caption payload requires completed genuine Button child18 arguments before allocation");
    detail::BnwWindowedRendererMatrixDataLoan::Impl::same_controls(call->paint_arguments->numeric);
    item.require_physical_current_without_callbacks();
    data.require_same_normal_draw_records_without_callbacks(actual->binding);
    data.require_current_without_callbacks();paint.require_current_without_callbacks();
    require_normal_root_epoch_prefix_without_callbacks(prefix);
    require_normal_button_caption_call_current_without_callbacks(caption);
}

void BnwWorldViewControlTree::require_normal_payload_caption_source_owned(
    const detail::BnwNormalGuiBatchPrefix::Impl& prefix,
    const detail::BnwLabelPaintPositionedData::Guard& data,
    const detail::BnwLabelPaintTextLoan::Guard& paint,
    const detail::BnwNormalButtonCaptionCall& caption,
    const detail::BnwNormalItemOccurrence::Guard& item,
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    const auto first=[&]{require_normal_payload_pending_caption_without_callbacks(prefix,data,paint,caption,item);};
    first();require_normal_root_epoch_prefix_source_owned(prefix,operation);first();
    require_normal_button_caption_call_source_owned(operation,caption);first();
    data.require_source_owned(operation);first();paint.require_source_owned(operation);first();
    data.require_same_normal_draw_records_source_owned(operation,item.impl_->binding);first();
}

std::shared_ptr<detail::BnwNormalFontPayloadPrefix::Impl>
BnwWorldViewControlTree::allocate_normal_font_payload_control_owned(
    std::shared_ptr<detail::BnwNormalGuiBatchPrefix::Impl> batch,
    detail::BnwLabelPaintPositionedData::Guard data,detail::BnwLabelPaintTextLoan::Guard paint,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalButtonCaptionCall& supplied_caption)const{
    using Loan=detail::BnwNormalFontPayloadPrefix;using Payload=Loan::Impl::Payload;
    const auto caption=supplied_caption;
    // Capture the original VALUE before any source/window/new-handler cut.
    require_normal_button_caption_call_current_without_callbacks(caption);data.require_current_without_callbacks();
    const auto item_control=data.impl_->normal_item_occurrence.lock();
    require(item_control&&data.impl_->normal_item_occurrence_issued,
        "Caption payload allocator original Item producer has not returned");
    const detail::BnwNormalItemOccurrence::IssuerKey item_key;
    const detail::BnwNormalItemOccurrence::Guard item(item_control,item_key);
    const auto associate=[&]{require_normal_button_caption_call_current_without_callbacks(caption);
        require(batch&&batch->tree.get()==this,
            "Caption payload allocator requires SAME canonical actual Tree");
        require_normal_payload_pending_caption_without_callbacks(*batch,data,paint,caption,item);
        require(&operation==batch->operation,
            "Caption payload allocator canonical Source changed before saved operand");};
    const auto full=[&]{associate();require_normal_payload_caption_source_owned(*batch,data,paint,caption,item,operation);associate();};
    associate();full();associate();
    auto& queue=batch->batch->payload_queue;
    require(queue.creator==std::this_thread::get_id()&&queue.alive.load()&&!queue.allocating&&!queue.appending&&
        queue.allocation_revision<UINT64_MAX&&queue.used_40<=UINT32_MAX-0x90u&&queue.arena.size()<queue.arena.max_size(),
        "Caption payload actual OWN arena reentered/retired/overflow");
    const auto before=queue.used_40;const auto before_revision=queue.allocation_revision;const auto count=queue.arena.size();
    if(const auto preceding=queue.current_loan.lock()){
        require(preceding->creator==queue.creator&&preceding->batch.get()==batch.get(),
            "Caption payload preceding current Loan has another actual creator/batch");
        if(preceding->alive.load())preceding->first();
        preceding->alive.store(false);
    }
    queue.allocating=true;
    struct End{decltype(queue)& q;~End(){q.allocating=false;}}end{queue};
    const auto pending=[&]{associate();require(queue.alive.load()&&queue.allocating&&queue.used_40==before&&
        queue.allocation_revision==before_revision&&queue.arena.size()==count,
        "Caption payload allocating arena revision/cursor/control changed");};
    std::shared_ptr<Payload> payload;
    try{
        pending();full();pending();queue.arena.reserve(count+1);pending();full();pending();
        payload=std::make_shared<Payload>(before);pending();full();pending();
        auto state=std::make_shared<Loan::Impl>(batch,data,paint,payload,before_revision+1,before+0x90u,caption,item);
        pending();full();pending();
        // Captured owners only, reserved append; no callback/allocation between
        // the real portable8EF140 current30/cursor40/length4C stores.
        queue.arena.push_back(payload);
        queue.current_30=payload;queue.used_40=before+0x90u;queue.length_4c=0x90;
        queue.allocation_revision=before_revision+1;queue.current_loan=state;
        queue.allocating=false;state->sealed=true;
        state->first();state->full(operation);state->first();return state;
    }catch(...){
        if(payload)payload->alive.store(false);queue.alive.store(false);batch->alive.store(false);
        batch->batch->alive.store(false);core_->failed.store(true);throw;
    }
}

void BnwWorldViewControlTree::write_normal_font_caption_scalar_control_owned(
    std::shared_ptr<detail::BnwNormalFontPayloadPrefix::Impl> loan)const{
    require(loan&&loan->caption,"Caption scalar writer requires distinct captured Caption VALUE");
    loan->first();const auto caption=*loan->caption;const auto call=caption.impl_;const auto batch=loan->batch;
    require(batch->tree.get()==this&&call->paint_arguments&&!call->projecting,
        "Caption scalar writer lacks SAME actual Tree/completed incoming arguments");
    const auto fields=call->label->impl_;fields->first();
    const auto word=[&](std::size_t offset){const auto* v=std::get_if<std::uint32_t>(&fields->fields[offset/4]);
        require(v,"Caption scalar writer physical Label word not produced");return *v;};
    const std::array<std::uint32_t,3> own{word(0x28),word(0x2c),word(0x44)};
    const auto arguments=*call->paint_arguments;const auto incoming=arguments.result;
    const auto depth=batch->batch->root_depth;
    const auto first=[&]{loan->first();require_normal_button_caption_call_current_without_callbacks(caption);
        fields->first();require(batch->tree.get()==this&&loan->caption&&loan->caption->impl_==call&&
            call->label->impl_==fields&&call->paint_arguments&&!call->projecting&&
            call->paint_arguments->incoming_position==arguments.incoming_position&&
            call->paint_arguments->incoming_alpha==arguments.incoming_alpha&&
            call->paint_arguments->button_position==arguments.button_position&&
            call->paint_arguments->button_alpha==arguments.button_alpha&&
            call->paint_arguments->result==incoming&&call->paint_arguments->numeric==arguments.numeric&&
            word(0x28)==own[0]&&word(0x2c)==own[1]&&word(0x44)==own[2]&&
            own[0]==call->run->impl_->value.geometry[6]&&own[1]==call->run->impl_->value.geometry[7]&&
            own[2]==call->run->impl_->value.alpha_44&&batch->batch->root_depth==depth&&
            !loan->payload->scalar_stores,
            "Caption scalar writer actual Label/incoming/depth operands changed or physical stores replayed");
        detail::BnwWindowedRendererMatrixDataLoan::Impl::same_controls(arguments.numeric);
        require_normal_button_caption_call_current_without_callbacks(caption);};
    try{
        first();loan->full(*batch->operation);first();
        std::array<std::uint32_t,5> result;
#if defined(__GNUC__) && defined(__x86_64__)
        // Actual Label-own operands FIRST in54075F/774/782. Input words0..2
        // are the original Button6DA5A0/5C5/5D2 cached results, not re-execution.
        asm volatile("movss %[x], %%xmm0\n\taddss %[parentx], %%xmm0\n\tmovss %%xmm0, %[outx]\n\t"
            "movss %[y], %%xmm0\n\taddss %[parenty], %%xmm0\n\tmovss %%xmm0, %[outy]\n\t"
            "movss %[ownalpha], %%xmm0\n\tmulss %[parentalpha], %%xmm0\n\tmovss %%xmm0, %[outalpha]\n\t"
            "flds %[arg5]\n\tfstps %[out5]\n\tflds %[arg4]\n\tfstps %[out4]\n\t"
            :[outx]"=m"(result[0]),[outy]"=m"(result[1]),[outalpha]"=m"(result[2]),[out5]"=m"(result[3]),[out4]"=m"(result[4])
            :[x]"m"(own[0]),[y]"m"(own[1]),[ownalpha]"m"(own[2]),[parentx]"m"(incoming[0]),
             [parenty]"m"(incoming[1]),[parentalpha]"m"(incoming[2]),[arg4]"m"(depth[0]),[arg5]"m"(depth[1])
            :"xmm0","st","memory");
#else
        require(false,"Caption scalar writer requires proved x64 GNU SSE/x87 profile");
#endif
        first();auto& payload=*loan->payload;
        payload.store_word(8,result[0]);payload.store_word(0,0);payload.store_word(4,0);payload.store_word(0xc,result[1]);
        payload.store_word(0x1c,result[2]);payload.store_word(0x20,result[3]);payload.store_word(0x24,result[4]);
        payload.store_word(0x70,0);payload.store_word(0x74,0);payload.store_word(0x40,0x3f800000u);
        payload.store_word(0x50,0);payload.store_word(0x60,0);payload.store_word(0x80,0);payload.store_word(0x84,0);
        payload.scalar_stores=true;loan->first();loan->full(*batch->operation);loan->first();
    }catch(...){
        loan->alive.store(false);loan->payload->alive.store(false);batch->alive.store(false);
        batch->batch->alive.store(false);core_->failed.store(true);throw;
    }
}
