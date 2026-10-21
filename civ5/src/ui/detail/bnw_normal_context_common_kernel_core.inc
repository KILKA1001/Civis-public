// Inside civ5::ui, AFTER complete Context ctor/initializer, ordinary storage,
// standalone state and NormalGuiBatchPrefix::Impl. Private OWN normal policy.
// No root caller/Read/attach completion is created here; no material is issued.
struct detail::BnwNormalGuiBatchPrefix::Impl::CommonRoot final {
    enum class Stage {captured,children_tested,arithmetic,child18_required,returned_empty,returned_root};
    struct LabelTailReturn;std::shared_ptr<const LabelTailReturn> label_tail;
    std::vector<std::shared_ptr<const LabelTailReturn>> label_returns;
    struct WholeReturn final {
        const std::thread::id creator=std::this_thread::get_id();
        const std::weak_ptr<CommonRoot> root;
        const std::weak_ptr<BnwNormalGuiBatchPrefix::Impl> batch;
        const std::uint64_t serial;const std::size_t count,cursor,returned_labels;
        WholeReturn(std::weak_ptr<CommonRoot> r,std::weak_ptr<BnwNormalGuiBatchPrefix::Impl> b,
            std::uint64_t s,std::size_t n,std::size_t i,std::size_t labels)
            :root(std::move(r)),batch(std::move(b)),serial(s),count(n),cursor(i),returned_labels(labels){}
    };
    std::shared_ptr<const WholeReturn> whole_return;bool continuing=false;
    const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn> completion;
    const std::shared_ptr<detail::NotificationLayoutContextOrdinaryState> context;
    const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> root;
    const std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> children;
    const std::uint64_t topology,effects,layout,base_sequence;
    const std::array<std::uint32_t,2> physical_offset;
    const std::uint32_t physical_alpha,physical_flags;
    const std::size_t count;
    const std::vector<std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>> ordered;
    Stage stage=Stage::captured;
    std::array<std::uint32_t,2> child_offset{};
    std::uint32_t child_alpha=0;
    std::size_t cursor=0;
    CommonRoot(std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn> a,
        std::shared_ptr<detail::NotificationLayoutContextOrdinaryState> c,
        std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> r,
        std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> v,
        std::uint64_t t,std::uint64_t e,std::uint64_t l,std::uint64_t b,
        std::array<std::uint32_t,2> offset,std::uint32_t alpha,std::uint32_t flags,std::size_t n,
        std::vector<std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>> actual_order)
        :completion(std::move(a)),context(std::move(c)),root(std::move(r)),children(std::move(v)),
         topology(t),effects(e),layout(l),base_sequence(b),physical_offset(offset),physical_alpha(alpha),
         physical_flags(flags),count(n),ordered(std::move(actual_order)){}
};
void detail::BnwNormalGuiBatchPrefix::Impl::require_common_root_without_callbacks()const{
    BnwWorldViewControlTree::require_normal_common_root_prefix_without_callbacks(*this);
}

void BnwWorldViewControlTree::require_normal_common_root_prefix_without_callbacks(
    const detail::BnwNormalGuiBatchPrefix::Impl& batch){
    // Earned Root epoch is checked before borrowed Tree/Context fields. The
    // old ACTIVE item/Matrix guard is neither called nor renamed here.
    require_normal_root_epoch_prefix_without_callbacks(batch);
    const auto actual=batch.tree->core_;actual->layout_origin(true);
    const auto& current=actual->source->current_operation_loan();
    require(&current==batch.operation,"Normal root fresh Source differs before saved operation");
    current.require_origin_without_callbacks();require(current.operation_serial()==batch.serial,
        "Normal root immutable paint serial changed");
    require_normal_root_receiver_before_common_without_callbacks(actual,*batch.operation,batch.serial);
    const auto seal=actual->layout_initialization_return;
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto state=actual->context_ordinary_state;
    const auto& base=*actual->context_layout_baseline;
    const auto receiver=base.base_.receiver_owned;const auto children=base.base_.children_78;
    if(const auto captured=batch.common_root){
        require(same(captured->completion,seal->native_return_)&&same(captured->context,state)&&
            same(captured->root,receiver)&&same(captured->children,children)&&
            captured->topology==actual->topology_revision&&captured->effects==actual->effect_revision&&
            captured->layout==actual->layout_sequence&&captured->base_sequence==base.sequence_&&
            captured->physical_offset==std::array<std::uint32_t,2>{std::bit_cast<std::uint32_t>(base.base_.offset_28_2c[0]),
                std::bit_cast<std::uint32_t>(base.base_.offset_28_2c[1])}&&
            captured->physical_alpha==*base.base_.alpha_word_44&&captured->physical_flags==base.base_.flags_40&&
            captured->count==(children?children->impl_->ordered.size():0),
            "Normal root physical common fields/vector changed after executed prefix");
        require(captured->ordered.size()==captured->count,"Normal root captured78 count differs");
        for(std::size_t i=0;i<captured->count;++i)require(same(captured->ordered[i],children->impl_->ordered[i]),
            "Normal root actual ordered78 member/control replaced after capture");
        if(captured->label_tail||!captured->label_returns.empty())require_normal_label_tail_return_without_callbacks(batch);
        if(captured->whole_return){
            const auto done=captured->whole_return;
            require(!captured->continuing&&captured->cursor==captured->count&&
                (captured->stage==decltype(captured)::element_type::Stage::returned_root||
                 captured->stage==decltype(captured)::element_type::Stage::returned_empty)&&
                done->creator==batch.creator&&done->batch.lock().get()==&batch&&same(done->root.lock(),captured)&&
                done->serial==batch.serial&&done->count==captured->count&&done->cursor==captured->cursor&&
                done->returned_labels==captured->label_returns.size()&&!captured->label_tail,
                "Normal root18 sealed completed traversal changed after return");
        }
    }
    actual->layout_origin(true);require(&actual->source->current_operation_loan()==batch.operation,
        "Normal root current Source changed before final pure return");
    current.require_origin_without_callbacks();
}

void BnwWorldViewControlTree::continue_normal_context_common56_owned(
    std::shared_ptr<detail::BnwNormalGuiBatchPrefix::Impl> batch,
    detail::BnwNormalLabelMaterialLoan::Impl* selected)const{
    using Root=detail::BnwNormalGuiBatchPrefix::Impl::CommonRoot;
    require_normal_root_epoch_prefix_without_callbacks(*batch);require(batch->tree.get()==this&&batch->common_root,
        "Normal common continuation has another actual root/batch");
    const auto reached=batch->common_root;const auto actual=core_;
    const auto children=reached->children;const auto root=reached->root;
    const auto seal=reached->completion;const auto count=reached->count;
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto first=[&]{require_normal_root_epoch_prefix_without_callbacks(*batch);require_normal_common_root_prefix_without_callbacks(*batch);};
    const auto full=[&]{first();require_normal_root_epoch_prefix_source_owned(*batch,*batch->operation);first();};
    first();require(!reached->continuing&&(reached->stage==Root::Stage::arithmetic||
        (reached->stage==Root::Stage::returned_empty&&!reached->count))&&
        reached->cursor<=count&&!reached->whole_return,
        "Normal common traversal replay/reentry/another reached phase");
    try{
      {
        reached->continuing=true;
        struct End{std::shared_ptr<Root> r;~End(){r->continuing=false;}}end{reached};
        for(;reached->cursor<count;++reached->cursor){
            first();const auto child=children->impl_->ordered[reached->cursor];
            require(child&&child->impl_,"Normal56F630 physical78 contains retired/NULL receiver");
            const auto native_flags=require_normal_root_ordered_child_flags_without_callbacks(actual,seal,root,child);
            if(native_flags&0x2002u)continue; //Actual56F706 TEST/56F70D only.
            reached->stage=Root::Stage::child18_required;first();
            // An enclosing Tree-owned traversal pauses at the actual call.
            // No Item/CPU data is fabricated for this receiver.
            if(!selected){
                const auto manager=batch->gui.lock();const auto actual_root=manager->normal_root_history.lock();
                require(actual_root&&actual_root->prefix==batch&&actual_root->label_traversal_active&&
                    !actual_root->label_traversal_completed&&actual_root->current_visible_label.expired()&&
                    !reached->label_tail,"Normal56F725 another genuine visible child18 dispatcher required");
                first();full();first();return;
            }
            enter_normal_selected_label591020_owned(*selected,*batch);
            first();full();first();return;
        }
      } //Actual continuation scope has returned before a WholeReturn is allocated.
        //Every real ordered member was visited, skipped by its physical40,
        //or returned from the one authentic selected child at its saved cursor.
        require(!reached->continuing&&reached->cursor==count,
            "Normal root18 traversal scope has not completed");
        reached->stage=!reached->label_returns.empty()?Root::Stage::returned_root:Root::Stage::returned_empty;
        first();full();first();
        const auto expected_cursor=reached->cursor;
        auto done=std::make_shared<const Root::WholeReturn>(std::weak_ptr<Root>(reached),
            std::weak_ptr<detail::BnwNormalGuiBatchPrefix::Impl>(batch),batch->serial,count,expected_cursor,
            reached->label_returns.size());
        first();full();first();
        require(reached->cursor==expected_cursor&&expected_cursor==count&&!reached->whole_return,
            "Normal root18 return allocation POST changed actual traversal");
        reached->whole_return=done;
        first();full();first();require_normal_root18_return_without_callbacks(*batch);
    }catch(...){batch->alive.store(false);batch->batch->alive.store(false);actual->failed.store(true);throw;}
}

void BnwWorldViewControlTree::resume_normal_context_common56_after_label_owned(
    std::shared_ptr<detail::BnwNormalGuiBatchPrefix::Impl> batch)const{
    using Root=detail::BnwNormalGuiBatchPrefix::Impl::CommonRoot;
    require_normal_root_epoch_prefix_without_callbacks(*batch);require(batch->tree.get()==this&&batch->common_root,
        "Normal root18 resume has another actual root/batch");
    const auto reached=batch->common_root;
    try{
        require(!reached->continuing&&reached->stage==Root::Stage::child18_required&&
            reached->cursor<reached->count&&reached->label_tail&&!reached->whole_return,
            "Normal root18 resume lacks the genuine current child return");
        require_normal_label_tail_return_without_callbacks(*batch);
        require_normal_root_epoch_prefix_source_owned(*batch,*batch->operation);require_normal_root_epoch_prefix_without_callbacks(*batch);
        require_normal_label_tail_return_without_callbacks(*batch);
        // Archive the SAME actual return before the executed caller advance.
        const auto tail=reached->label_tail;const auto before=reached->label_returns.size();
        reached->label_returns.reserve(before+1);
        require_normal_root_epoch_prefix_source_owned(*batch,*batch->operation);
        require_normal_label_tail_return_without_callbacks(*batch);
        require(reached->label_tail==tail&&reached->label_returns.size()==before,
            "Normal root caller child-return journal changed during allocation");
        reached->label_returns.push_back(tail);reached->label_tail.reset();
        //This is the executed caller56F727 ADD ESI,4, not a second prefix.
        ++reached->cursor;reached->stage=Root::Stage::arithmetic;
        continue_normal_context_common56_owned(batch,nullptr);
        require_normal_root_epoch_prefix_without_callbacks(*batch);require_normal_root_epoch_prefix_source_owned(*batch,*batch->operation);require_normal_root_epoch_prefix_without_callbacks(*batch);
        require_normal_root18_return_without_callbacks(*batch);
    }catch(...){batch->alive.store(false);batch->batch->alive.store(false);core_->failed.store(true);throw;}
}

void BnwWorldViewControlTree::require_normal_root18_return_without_callbacks(
    const detail::BnwNormalGuiBatchPrefix::Impl& batch){
    require_normal_common_root_prefix_without_callbacks(batch);
    const auto root=batch.common_root;
    require(root&&root->whole_return&&!root->continuing&&root->cursor==root->count&&
        (root->stage==decltype(root)::element_type::Stage::returned_root||
         root->stage==decltype(root)::element_type::Stage::returned_empty),
        "Normal root18 actual complete traversal return unissued");
    const auto done=root->whole_return;const auto b=done->batch.lock();const auto r=done->root.lock();
    require(done->creator==std::this_thread::get_id()&&b.get()==&batch&&r==root&&
        done->serial==batch.serial&&done->count==root->count&&done->cursor==root->cursor&&
        done->cursor==done->count&&done->returned_labels==root->label_returns.size()&&!root->label_tail,
        "Normal root18 sealed actual batch/root/serial/cursor changed");
    if(!root->label_returns.empty())require_normal_label_tail_return_without_callbacks(batch);
    require_normal_common_root_prefix_without_callbacks(batch);
}

void BnwWorldViewControlTree::execute_normal_context_common56_prefix_owned(
    const detail::BnwNormalGuiBatchPrefix& prefix)const{
    const auto batch=prefix.impl_;execute_normal_context_selected_label_entry_owned(batch,nullptr);
}
void BnwWorldViewControlTree::execute_normal_context_selected_label_entry_owned(
    std::shared_ptr<detail::BnwNormalGuiBatchPrefix::Impl> batch,
    detail::BnwNormalLabelMaterialLoan::Impl* selected)const{
    using Loan=detail::BnwNormalGuiBatchPrefix;using Root=Loan::Impl::CommonRoot;
    require_normal_root_epoch_prefix_without_callbacks(*batch);require(batch->tree.get()==this&&!batch->common_root,
        "Normal common root prefix another Tree/replay");
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto first=[&]{require_normal_root_epoch_prefix_without_callbacks(*batch);require_normal_common_root_prefix_without_callbacks(*batch);};
    const auto full=[&]{first();require_normal_root_epoch_prefix_source_owned(*batch,*batch->operation);first();};
    first();full();first();
    const auto actual=core_;const auto seal=actual->layout_initialization_return->native_return_;
    const auto context=actual->context_ordinary_state;const auto& base=*actual->context_layout_baseline;
    const auto root=base.base_.receiver_owned;const auto children=base.base_.children_78;
    const auto topology=actual->topology_revision,effects=actual->effect_revision,layout=actual->layout_sequence,
        sequence=base.sequence_;
    const std::array<std::uint32_t,2> offset{std::bit_cast<std::uint32_t>(base.base_.offset_28_2c[0]),
        std::bit_cast<std::uint32_t>(base.base_.offset_28_2c[1])};
    const auto alpha=*base.base_.alpha_word_44,flags=base.base_.flags_40;
    const auto count=children?children->impl_->ordered.size():0;
    const auto incoming=batch->batch->root_offset;const auto incoming_alpha=batch->batch->root_alpha;
    try{
        //Do not copy a mutable borrowed vector through operator new/new_handler.
        std::vector<std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>> ordered;
        ordered.reserve(count);first();full();first();
        require((children?children->impl_->ordered.size():0)==count,"Normal root78 reserve POST changed count");
        for(std::size_t i=0;i<count;++i)ordered.push_back(children->impl_->ordered[i]); //capacity earned, shared VALUE copy
        const auto order_current=[&]{first();require(ordered.size()==count&&
            (children?children->impl_->ordered.size():0)==count,"Normal root78 staged count changed");
            for(std::size_t i=0;i<count;++i)require(ordered[i]==children->impl_->ordered[i]&&
                !ordered[i].owner_before(children->impl_->ordered[i])&&!children->impl_->ordered[i].owner_before(ordered[i]),
                "Normal root78 staged actual member/control replaced");};
        order_current();
        //Constructor receives OWN values, no reference to Base across allocation.
        auto reached=std::make_shared<Root>(seal,context,root,children,topology,effects,layout,sequence,offset,alpha,flags,count,
            std::move(ordered));
        first();full();first();
        //Same actual fields BEFORE publication, including an allocator that
        //changed an otherwise unrelated Base without incrementing epochs.
        const auto& now=*actual->context_layout_baseline;
        require(same(actual->layout_initialization_return->native_return_,seal)&&same(actual->context_ordinary_state,context)&&
            now.sequence_==sequence&&same(now.base_.receiver_owned,root)&&same(now.base_.children_78,children)&&
            now.base_.alpha_word_44&&*now.base_.alpha_word_44==alpha&&now.base_.flags_40==flags&&
            std::bit_cast<std::uint32_t>(now.base_.offset_28_2c[0])==offset[0]&&
            std::bit_cast<std::uint32_t>(now.base_.offset_28_2c[1])==offset[1]&&
            (!children||children->impl_->ordered.size()==count),"Normal common root allocation POST changed operands");
        for(std::size_t i=0;i<count;++i)require(reached->ordered[i]==children->impl_->ordered[i]&&
            !reached->ordered[i].owner_before(children->impl_->ordered[i])&&
            !children->impl_->ordered[i].owner_before(reached->ordered[i]),"Normal root78 allocation POST changed member");
        batch->common_root=reached;first();
        //56F6A0..6AF tests genuine physical78/count BEFORE numerical effects.
        reached->stage=Root::Stage::children_tested;
        if(!children||!count){reached->stage=Root::Stage::returned_empty;first();full();first();return;}
#if defined(__GNUC__) && defined(__x86_64__)
        //Raw SSE preserves the exact operand order and caller's MXCSR profile:
        //MULSS(this44,arg3), ADDSS(arg2.x,this28), ADDSS(arg2.y,this2C).
        //No FMA, FP64 accumulation, canonical NaN/negative-zero sanitization.
        asm volatile("movss %[ownalpha], %%xmm0\n\t"
            "mulss %[parentalpha], %%xmm0\n\t"
            "movss %%xmm0, %[outalpha]\n\t"
            "movss %[parentx], %%xmm0\n\taddss %[ownx], %%xmm0\n\t"
            "movss %%xmm0, %[outx]\n\t"
            "movss %[parenty], %%xmm0\n\taddss %[owny], %%xmm0\n\t"
            "movss %%xmm0, %[outy]\n\t"
            :[outalpha]"=m"(reached->child_alpha),[outx]"=m"(reached->child_offset[0]),
             [outy]"=m"(reached->child_offset[1])
            :[ownalpha]"m"(alpha),[parentalpha]"m"(incoming_alpha),[parentx]"m"(incoming[0]),
             [parenty]"m"(incoming[1]),[ownx]"m"(offset[0]),[owny]"m"(offset[1])
            :"xmm0","memory");
#else
        require(false,"Normal56F630 requires proved x64 GNU raw SSE kernel");
#endif
        reached->stage=Root::Stage::arithmetic;first();
        if(!batch->common_dispatch.expired()){
            require(!selected,"Mixed Common cannot enter indexed/selected Label bootstrap");
            require_normal_common_traversal_capture_without_callbacks(*batch);
            first();full();first();return;
        }
        continue_normal_context_common56_owned(batch,selected);
    }catch(...){
        batch->alive.store(false);batch->batch->alive.store(false); //retire FIRST
        actual->failed.store(true);throw;
    }
}
