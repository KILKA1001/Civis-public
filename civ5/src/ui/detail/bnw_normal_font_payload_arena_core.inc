//Late inside civ5::ui, AFTER Source3 common kernel and full standalone Paint/DATA.
//This OWN preparation allocator is executable independently of the still-unissued
//normal root phase. It never claims that Original51D7F0 was reached for a plane.
struct detail::BnwNormalFontPayloadPrefix::Impl final {
    using Prefix=BnwNormalGuiBatchPrefix;using Gui=NotificationContextGuiPort;
    using Payload=Gui::Impl::NormalFontPayload144;
    const std::thread::id creator=std::this_thread::get_id();std::atomic<bool> alive{true};
    const std::shared_ptr<Prefix::Impl> batch;
    const BnwLabelPaintPositionedData::Guard data;
    const BnwLabelPaintTextLoan::Guard paint;
    const std::optional<BnwNormalButtonCaptionCall> caption;
    const std::optional<BnwNormalItemOccurrence::Guard> caption_item;
    const std::shared_ptr<Payload> payload;
    const std::uint64_t revision;const std::uint32_t used;
    bool sealed=false;
    Impl(std::shared_ptr<Prefix::Impl> b,BnwLabelPaintPositionedData::Guard d,
        BnwLabelPaintTextLoan::Guard p,std::shared_ptr<Payload> v,std::uint64_t r,std::uint32_t u)
        :batch(std::move(b)),data(std::move(d)),paint(std::move(p)),caption(),caption_item(),
         payload(std::move(v)),revision(r),used(u){}
    Impl(std::shared_ptr<Prefix::Impl> b,BnwLabelPaintPositionedData::Guard d,
        BnwLabelPaintTextLoan::Guard p,std::shared_ptr<Payload> v,std::uint64_t r,std::uint32_t u,
        BnwNormalButtonCaptionCall actual_caption,BnwNormalItemOccurrence::Guard actual_item)
        :batch(std::move(b)),data(std::move(d)),paint(std::move(p)),caption(std::move(actual_caption)),
         caption_item(std::move(actual_item)),payload(std::move(v)),revision(r),used(u){}
    void association()const{
        if(caption||caption_item){
            require(caption&&caption_item,"Embedded payload mandatory typed owner pair incomplete");
            BnwWorldViewControlTree::require_normal_payload_pending_caption_without_callbacks(*batch,data,paint,*caption,*caption_item);return;}
        BnwWorldViewControlTree::require_normal_root_epoch_prefix_without_callbacks(*batch);data.require_current_without_callbacks();paint.require_current_without_callbacks();
        data.require_same_paint_guard_without_callbacks(paint);
        const auto origin=paint.standalone_;require(origin&&!paint.impl_,
            "Normal payload actual selected standalone origin required, no embedded conversion");
        require(origin->tree.get()==batch->tree.get()&&!origin->tree.owner_before(batch->tree)&&
            !batch->tree.owner_before(origin->tree),
            "Normal payload needs SAME actual Frame9/Tree/Paint/DATA");
        BnwWorldViewControlTree::require_normal_payload_pending_label_without_callbacks(*batch,data,paint);
        //Standalone Paint.first checks real Run/CC/positive170 and Normal10,
        //but none of those controls grant original root/Font-plane membership.
        paint.require_current_without_callbacks();data.require_current_without_callbacks();BnwWorldViewControlTree::require_normal_root_epoch_prefix_without_callbacks(*batch);
    }
    void first()const{
        require(creator==std::this_thread::get_id()&&alive.load()&&sealed&&batch&&payload,
            "Normal payload retired/foreign/unsealed FIRST");
        association();const auto& queue=batch->batch->payload_queue;
        require(queue.creator==creator&&queue.alive.load()&&!queue.allocating&&
            queue.allocation_revision==revision&&revision&&queue.used_40==used&&queue.length_4c==0x90&&
            queue.current_loan.lock().get()==this&&
            queue.current_30.get()==payload.get()&&!queue.current_30.owner_before(payload)&&
            !payload.owner_before(queue.current_30)&&!queue.arena.empty()&&queue.arena.back()==payload&&
            payload->creator==creator&&payload->alive.load()&&payload->offset+0x90u==used,
            "Normal payload real current30/cursor40/length4C/arena control replaced");
        association();
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
        first();require(&operation==batch->operation,"Normal payload another canonical Source operand");
        BnwWorldViewControlTree::require_normal_root_epoch_prefix_source_owned(*batch,operation);first();data.require_source_owned(operation);first();
        paint.require_source_owned(operation);first();
        if(caption){BnwWorldViewControlTree::require_normal_payload_caption_source_owned(
            *batch,data,paint,*caption,*caption_item,operation);first();}
    }
};
detail::BnwNormalFontPayloadPrefix::BnwNormalFontPayloadPrefix(std::shared_ptr<Impl> value):impl_(std::move(value)){}
detail::BnwNormalFontPayloadPrefix::~BnwNormalFontPayloadPrefix(){impl_->alive.store(false);}
void detail::BnwNormalFontPayloadPrefix::require_current_without_callbacks()const{const auto v=impl_;v->first();}
void detail::BnwNormalFontPayloadPrefix::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto v=impl_;v->full(operation);v->first();
}

std::unique_ptr<detail::BnwNormalFontPayloadPrefix>
BnwWorldViewControlTree::allocate_normal_font_payload_prefix_owned(const detail::BnwNormalGuiBatchPrefix& actual_batch,
    const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwLabelPaintPositionedData& actual_data)const{
    using Loan=detail::BnwNormalFontPayloadPrefix;
    //Capture every facade control before FIRST (which may observe the window).
    const auto batch=actual_batch.impl_;const auto data=actual_data.capture_guard_without_callbacks();
    const auto paint=data.captured_paint_guard_without_callbacks();
    auto state=allocate_normal_font_payload_control_owned(batch,data,paint,operation);
    try{
        state->first();state->full(operation);state->first();
        auto result=std::unique_ptr<Loan>(new Loan(state));
        state->first();state->full(operation);state->first();return result;
    }catch(...){state->alive.store(false);state->payload->alive.store(false);
        state->batch->alive.store(false);state->batch->batch->alive.store(false);core_->failed.store(true);throw;}
}

std::shared_ptr<detail::BnwNormalFontPayloadPrefix::Impl>
BnwWorldViewControlTree::allocate_normal_font_payload_control_owned(
    std::shared_ptr<detail::BnwNormalGuiBatchPrefix::Impl> batch,
    detail::BnwLabelPaintPositionedData::Guard data,detail::BnwLabelPaintTextLoan::Guard paint,
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    using Loan=detail::BnwNormalFontPayloadPrefix;using Payload=Loan::Impl::Payload;
    BnwWorldViewControlTree::require_normal_root_epoch_prefix_without_callbacks(*batch);require(batch->tree.get()==this,"Normal payload another actual control Tree");
    const auto associate=[&]{BnwWorldViewControlTree::require_normal_root_epoch_prefix_without_callbacks(*batch);data.require_current_without_callbacks();paint.require_current_without_callbacks();
        data.require_same_paint_guard_without_callbacks(paint);
        const auto origin=paint.standalone_;require(origin&&!paint.impl_&&origin->tree.get()==this&&
            !origin->tree.owner_before(batch->tree)&&!batch->tree.owner_before(origin->tree)&&
            origin->publication&&origin->control,
            "Normal payload needs actual standalone SAME Frame/Paint/DATA item");
        require_normal_payload_pending_label_without_callbacks(*batch,data,paint);
        require(&operation==batch->operation,"Normal payload current Source differs before old operand access");
        BnwWorldViewControlTree::require_normal_root_epoch_prefix_without_callbacks(*batch);paint.require_current_without_callbacks();data.require_current_without_callbacks();};
    const auto full=[&]{associate();BnwWorldViewControlTree::require_normal_root_epoch_prefix_source_owned(*batch,operation);associate();data.require_source_owned(operation);associate();
        paint.require_source_owned(operation);associate();};
    associate();full();associate();
    auto& queue=batch->batch->payload_queue;
    require(queue.creator==std::this_thread::get_id()&&queue.alive.load()&&!queue.allocating&&!queue.appending&&
        queue.allocation_revision<UINT64_MAX&&queue.used_40<=UINT32_MAX-0x90u&&queue.arena.size()<queue.arena.max_size(),
        "Normal payload actual OWN arena reentered/retired/overflow");
    const auto before=queue.used_40;const auto before_revision=queue.allocation_revision;const auto count=queue.arena.size();
    //A replacement allocation NEVER renews the preceding current Loan.
    //Retire its authority FIRST, retaining historical OWN arena storage.
    if(const auto preceding=queue.current_loan.lock()){
        require(preceding->creator==queue.creator&&preceding->batch.get()==batch.get(),
            "Normal payload preceding current Loan has another creator/batch");
        if(preceding->alive.load())preceding->first();
        preceding->alive.store(false);
    }
    queue.allocating=true;
    struct End{decltype(queue)& q;~End(){q.allocating=false;}}end{queue};
    const auto pending=[&]{associate();require(queue.alive.load()&&queue.allocating&&queue.used_40==before&&
        queue.allocation_revision==before_revision&&queue.arena.size()==count,
        "Normal payload allocating arena revision/cursor/control changed");};
    std::shared_ptr<Payload> payload;
    try{
        pending();full();pending();queue.arena.reserve(count+1);pending();full();pending();
        payload=std::make_shared<Payload>(before);pending();full();pending();
        auto state=std::make_shared<Loan::Impl>(batch,data,paint,payload,before_revision+1,before+0x90u);
        pending();full();pending();
        //A new allocation control is issued once; this is not a renewed facade.
        //Real OWN storage retained before portable8EF140 stores. Pointer+offset
        //maps to SAME storage control+offset, not a guessed PE/D3D pointer.
        queue.arena.push_back(payload); //reserved, shared VALUE, no allocation
        queue.current_30=payload;queue.used_40=before+0x90u;queue.length_4c=0x90;
        queue.allocation_revision=before_revision+1;queue.current_loan=state;
        queue.allocating=false;state->sealed=true;
        state->first();state->full(operation);state->first();return state;
    }catch(...){
        if(payload)payload->alive.store(false);queue.alive.store(false);batch->alive.store(false);
        batch->batch->alive.store(false);core_->failed.store(true);throw; //retire FIRST
    }
}

void BnwWorldViewControlTree::write_normal_font_scalar_prefix_owned(const detail::BnwNormalFontPayloadPrefix& allocation)const{
    const auto loan=allocation.impl_;write_normal_font_scalar_control_owned(loan);
}
void BnwWorldViewControlTree::write_normal_font_scalar_control_owned(
    std::shared_ptr<detail::BnwNormalFontPayloadPrefix::Impl> loan)const{
    if(loan&&loan->caption){write_normal_font_caption_scalar_control_owned(std::move(loan));return;}
    loan->first();require(loan->batch->tree.get()==this,
        "Normal scalar writer another actual Tree");
    const auto batch=loan->batch;const auto root=batch->common_root;
    require(root&&root->stage==decltype(root)::element_type::Stage::child18_required&&root->children&&
        root->cursor<root->count,"Normal scalar writer requires earned visiblechild18 entry, not arena/Normal10/Matrix");
    require_normal_common_root_prefix_without_callbacks(*batch);
    const auto owner=core_;const auto actual_control=loan->paint.standalone_->control;
    const auto label=[&]{const auto& node=Core::node(owner->nodes,actual_control);
        require(node.layout_baseline&&node.layout_baseline->standalone_label_&&
            root->ordered[root->cursor]==node.layout_baseline->base_.receiver_owned,
            "Normal scalar writer root actual78 cursor is not SAME prepared Label");
        return node.layout_baseline->standalone_label_;}();
    const auto fields=label->impl_;fields->first();
    const auto word=[&](std::size_t offset){const auto* v=std::get_if<std::uint32_t>(&fields->fields[offset/4]);
        require(v!=nullptr,"Normal scalar writer physical Label word missing");return *v;};
    const std::array<std::uint32_t,3> own{word(0x28),word(0x2c),word(0x44)};
    const auto incoming=root->child_offset;const auto alpha=root->child_alpha;const auto depth=batch->batch->root_depth;
    const auto first=[&]{loan->first();require_normal_common_root_prefix_without_callbacks(*batch);fields->first();
        const auto& node=Core::node(owner->nodes,actual_control);
        require(root==batch->common_root&&root->stage==decltype(root)::element_type::Stage::child18_required&&
            root->cursor<root->count&&root->ordered[root->cursor]==node.layout_baseline->base_.receiver_owned&&
            node.layout_baseline->standalone_label_==label&&word(0x28)==own[0]&&word(0x2c)==own[1]&&word(0x44)==own[2]&&
            root->child_offset==incoming&&root->child_alpha==alpha&&batch->batch->root_depth==depth&&
            !loan->payload->scalar_stores,"Normal scalar writer actual Label/ancestor words replaced or replay");};
    try{
    first();loan->full(*batch->operation);first();
    //Scalar arithmetic never grants plane/resource/material association. Raw
    //SSE order matches54075F/774/782, including Label-own operand FIRST.
    std::array<std::uint32_t,5> result;
#if defined(__GNUC__) && defined(__x86_64__)
    asm volatile("movss %[x], %%xmm0\n\taddss %[parentx], %%xmm0\n\tmovss %%xmm0, %[outx]\n\t"
        "movss %[y], %%xmm0\n\taddss %[parenty], %%xmm0\n\tmovss %%xmm0, %[outy]\n\t"
        "movss %[ownalpha], %%xmm0\n\tmulss %[parentalpha], %%xmm0\n\tmovss %%xmm0, %[outalpha]\n\t"
        "flds %[arg5]\n\tfstps %[out5]\n\tflds %[arg4]\n\tfstps %[out4]\n\t"
        :[outx]"=m"(result[0]),[outy]"=m"(result[1]),[outalpha]"=m"(result[2]),[out5]"=m"(result[3]),[out4]"=m"(result[4])
        :[x]"m"(own[0]),[y]"m"(own[1]),[ownalpha]"m"(own[2]),[parentx]"m"(incoming[0]),
         [parenty]"m"(incoming[1]),[parentalpha]"m"(alpha),[arg4]"m"(depth[0]),[arg5]"m"(depth[1])
        :"xmm0","st","memory");
#else
    require(false,"Normal scalar writer requires proved x64 GNU SSE/x87 profile");
#endif
    first();auto& payload=*loan->payload;
    payload.store_word(8,result[0]);payload.store_word(0,0);payload.store_word(4,0);payload.store_word(0xc,result[1]);
    payload.store_word(0x1c,result[2]);payload.store_word(0x20,result[3]);payload.store_word(0x24,result[4]);
    payload.store_word(0x70,0);payload.store_word(0x74,0);payload.store_word(0x40,0x3f800000u);
    payload.store_word(0x50,0);payload.store_word(0x60,0);payload.store_word(0x80,0);payload.store_word(0x84,0);
    payload.scalar_stores=true;loan->first();loan->full(*batch->operation);loan->first();
    //This is NOT8EEFE0: byte2C belongs to renderer/material descriptor owner,
    //not Queue. Its finalizer and actual pass118/11C/120→8EC210 are UNISSUED.
    }catch(...){loan->alive.store(false);loan->payload->alive.store(false);
        batch->alive.store(false);batch->batch->alive.store(false);owner->failed.store(true);throw;}
}
