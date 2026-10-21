// Inside civ5::ui, after complete genuine Call/Root/physical NULL return.
// Recursive ordinary Common owns its own native operands and actual cursor.
// An ancestor Call is suspended, never renewed as the current child permission.
struct BnwWorldViewControlTree::NormalCommonRecursiveReturn final {
private:
    friend class BnwWorldViewControlTree;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const detail::BnwNormalCommonChildCall::Impl> original_call;
    const detail::BnwNormalRootBatchHistory::Guard root;
    const std::shared_ptr<NormalCommonTraversal::Frame> frame;
    NormalCommonRecursiveReturn(std::weak_ptr<const detail::BnwNormalCommonChildCall::Impl> call,
        detail::BnwNormalRootBatchHistory::Guard actual_root,
        std::shared_ptr<NormalCommonTraversal::Frame> actual_frame)
        :original_call(std::move(call)),root(std::move(actual_root)),frame(std::move(actual_frame)){}
};

struct BnwWorldViewControlTree::NormalCommonChild18Return final {
private:
    friend class BnwWorldViewControlTree;
    const std::thread::id creator=std::this_thread::get_id();
    const std::shared_ptr<const detail::BnwNormalCommonChildCall::Impl> original_call;
    const std::shared_ptr<const NormalCommonRecursiveReturn> common;
    const std::shared_ptr<const NormalCommonNullChildReturn> null_return;
    const std::optional<detail::BnwNormalNonFontOwnReturn> own_return;
    NormalCommonChild18Return(std::shared_ptr<const detail::BnwNormalCommonChildCall::Impl> call,
        std::shared_ptr<const NormalCommonRecursiveReturn> actual_common,
        std::shared_ptr<const NormalCommonNullChildReturn> actual_null,
        std::optional<detail::BnwNormalNonFontOwnReturn> actual_own)
        :original_call(std::move(call)),common(std::move(actual_common)),
         null_return(std::move(actual_null)),own_return(std::move(actual_own)){}
};

std::uint32_t BnwWorldViewControlTree::require_normal_common_actual_child40_without_callbacks(
    const std::shared_ptr<Core>& actual,
    const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& parent,
    const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& child){
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require(actual&&parent&&child&&child->impl_,"Recursive Common40 original receiver/parent missing");
    const auto fields=child->impl_;const auto owner=fields->core.lock();
    const auto occurrence=fields->occurrence.lock();
    require(owner&&same(owner,actual)&&occurrence&&fields->id&&
        fields->creator==std::this_thread::get_id()&&same(fields->canonical.lock(),child),
        "Recursive Common40 cross-domain/retired receiver needs its actual native dispatcher");
    // Physical DATA only: a retained return may be checked after the active
    // paint scope returned. This does not issue CURRENT child/paint authority.
    actual->layout_origin(actual->busy);const auto& fresh=actual->source->current_operation_loan();
    fresh.require_origin_without_callbacks();
    const auto& node=Core::node(actual->nodes,fields->id);
    require(node.alive&&!node.dynamic&&node.layout_baseline&&node.full_xml_occurrence&&
        same(node.layout_receiver_occurrence,occurrence)&&fields->serial==occurrence->constructor_serial,
        "Recursive Common40 genuine canonical constructor/occurrence missing");
    const auto& base=*node.layout_baseline;const auto returned=base.base_.common_return_owned;
    require(returned&&returned->impl_&&
        same(base.base_.receiver_owned,child)&&same(base.base_.parent_38.lock(),parent),
        "Recursive Common40 physical Base/common return/paired38 missing");
    const auto done=returned->impl_;
    require(done->creator==fields->creator&&same(done->core.lock(),actual)&&
        same(done->occurrence.lock(),occurrence)&&same(done->selected.lock(),node.full_xml_occurrence)&&
        same(done->receiver.lock(),child)&&done->serial==fields->serial&&
        fields->serial==fresh.operation_serial()&&done->count==node.full_xml_occurrence->value.children.size()&&
        (base.base_.flags_40&8u),
        "Recursive Common40 physical stored Base/actual Common return replaced");
    require_native_ordinary_current_children_without_callbacks(child,returned,base.base_.children_78);
    // Type authority is the actual allocated subtype control, not XML/kind.
    const auto label=fields->standalone_label.lock();const auto button=fields->button.lock();
    const auto image=fields->image.lock();const auto box=fields->box.lock();const auto grid=fields->grid.lock();
    const unsigned count=unsigned(bool(label))+unsigned(bool(button))+unsigned(bool(image))+
        unsigned(bool(box))+unsigned(bool(grid));
    require(count==1&&fields->context.expired()&&fields->stack.expired(),
        "Recursive Common40 unissued real subtype native field supplier; no generic flags projection");
    if(label){
        require(same(label,base.standalone_label_),"Recursive Common40 actual Label storage replaced");
        label->impl_->first();const auto word=std::get_if<std::uint32_t>(&label->impl_->fields[0x40/4]);
        require(word&&*word==base.base_.flags_40,"Recursive Common40 actual Label native word differs");
    }else if(button){
        require(same(button,base.button_),"Recursive Common40 actual Button storage replaced");
        require_button_native_common_return_owned(actual,fields->id,fresh,actual->busy);
    }else if(image)require(same(image,base.image_),"Recursive Common40 actual Image storage replaced");
    else if(box)require(same(box,base.box_),"Recursive Common40 actual Box storage replaced");
    else require(same(grid,base.grid_),"Recursive Common40 actual Grid storage replaced");
    // The canonical Base is the real constructor/parser-owned word. No
    // presented alpha/hide/type DTO or reconstructed native32 address is read.
    const auto word=base.base_.flags_40;
    fresh.require_origin_without_callbacks();actual->layout_origin(actual->busy);
    const auto& post=Core::node(actual->nodes,fields->id);
    require(post.layout_baseline&&same(post.layout_receiver_occurrence,occurrence)&&
        same(post.layout_baseline->base_.receiver_owned,child)&&
        same(post.layout_baseline->base_.parent_38.lock(),parent)&&post.layout_baseline->base_.flags_40==word,
        "Recursive Common40 actual physical word changed at PURE POST");
    return word;
}

void BnwWorldViewControlTree::NormalCommonTraversal::require_recursive_frame_without_callbacks(
    const Frame& frame,
    const detail::BnwNormalRootBatchHistory::Guard& root){
    using Call=detail::BnwNormalCommonChildCall;
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require(frame.creator==std::this_thread::get_id()&&frame.alive&&frame.sealed&&frame.depth&&
        !frame.root_common&&frame.parent&&frame.children&&frame.children->impl_&&
        frame.ordered.size()==frame.count&&frame.filter_words.size()==frame.count&&
        frame.returned_children.size()==frame.count&&frame.cursor<=frame.count,
        "Recursive Common frame lacks real retained count/order/cursor");
    const auto parent_call=frame.original_parent_call.lock();
    require(parent_call&&parent_call->creator==frame.creator&&parent_call->sealed&&
        same(parent_call->canonical_self.lock(),parent_call)&&parent_call->kind==Call::Kind::button&&
        same(parent_call->receiver,frame.parent)&&parent_call->depth+1==frame.depth&&
        parent_call->base_sequence==frame.base_sequence&&parent_call->physical_flags==frame.physical_flags,
        "Recursive Common frame lost SAME original parent Call/physical Base");
    const auto history=root.impl_;
    require(history&&history->creator==frame.creator&&history->sealed&&history->tree&&history->serial==parent_call->serial,
        "Recursive Common frame original Root custody retired");
    history->root_scope.require_physical_current_without_callbacks();
    const auto actual=history->tree->core_;actual->layout_origin(actual->busy);
    const auto& fresh=actual->source->current_operation_loan();fresh.require_origin_without_callbacks();
    require(&fresh==history->operation&&fresh.operation_serial()==parent_call->serial,
        "Recursive Common frame current Source changed before saved physical operands");
    const auto& node=Core::node(actual->nodes,parent_call->control.id_);
    require(node.alive&&!node.dynamic&&node.layout_baseline&&node.layout_receiver_occurrence&&
        same(parent_call->control.owner_.lock(),actual)&&frame.parent->impl_&&
        frame.parent->impl_->id==parent_call->control.id_&&
        same(frame.parent->impl_->occurrence.lock(),node.layout_receiver_occurrence)&&
        same(node.layout_baseline->base_.receiver_owned,frame.parent)&&
        same(node.layout_baseline->button_,parent_call->original_button),
        "Recursive Common frame SAME parent constructor allocation changed");
    const auto& baseline=*node.layout_baseline;
    require(baseline.sequence_==frame.base_sequence&&baseline.base_.flags_40==frame.physical_flags&&
        same(baseline.base_.parent_38.lock(),parent_call->parent)&&
        parent_call->children&&parent_call->children->impl_&&parent_call->ordinal<parent_call->count&&
        parent_call->children->impl_->ordered.size()==parent_call->count&&
        same(parent_call->children->impl_->ordered[parent_call->ordinal],frame.parent)&&
        same(parent_call->children->impl_->parent.lock(),parent_call->parent)&&
        baseline.base_.alpha_word_44&&*baseline.base_.alpha_word_44==frame.physical_alpha&&
        frame.physical_offset==std::array<std::uint32_t,2>{
            std::bit_cast<std::uint32_t>(baseline.base_.offset_28_2c[0]),
            std::bit_cast<std::uint32_t>(baseline.base_.offset_28_2c[1])}&&
        same(baseline.base_.children_78,frame.children)&&
        same(frame.children->impl_->parent.lock(),frame.parent)&&
        frame.children->impl_->ordered.size()==frame.count,
        "Recursive Common frame physical28/2C/40/44/78 changed");
    for(std::size_t i=0;i<frame.count;++i)require(same(frame.ordered[i],frame.children->impl_->ordered[i]),
        "Recursive Common actual saved ordered78 member replaced");
    require((frame.count==0&&!frame.arithmetic_earned)||
        (frame.count>0&&frame.arithmetic_earned),"Recursive Common count/arithmetic Original branch changed");
    fresh.require_origin_without_callbacks();history->root_scope.require_physical_current_without_callbacks();
}

void BnwWorldViewControlTree::require_normal_common_recursive_return_physical_without_callbacks(
    const NormalCommonRecursiveReturn& returned){
    const auto call=returned.original_call.lock();const auto frame=returned.frame;
    require(returned.creator==std::this_thread::get_id()&&call&&frame&&
        call->common_return.get()==&returned&&frame->popped&&
        frame->stage==NormalCommonTraversal::Frame::Stage::returned&&frame->cursor==frame->count,
        "Recursive Common return was not earned by real ordered78 exhaustion/pop");
    NormalCommonTraversal::require_recursive_frame_without_callbacks(*frame,returned.root);
    const auto actual=returned.root.impl_->tree->core_;
    for(std::size_t i=0;i<frame->count;++i){
        const auto filtered=frame->filter_words[i];const auto child=frame->returned_children[i];
        require(bool(filtered)!=bool(child),"Recursive Common member has missing/duplicate real return");
        if(filtered){
            require((*filtered&0x2002u)&&require_normal_common_actual_child40_without_callbacks(
                actual,frame->parent,frame->ordered[i])==*filtered,
                "Recursive Common native2002 filtered word changed after RETURN");
        }else{
            require(child->original_call&&child->original_call->receiver==frame->ordered[i]&&
                child->original_call->ordinal==i&&child->original_call->depth==frame->depth,
                "Recursive Common child18 return belongs to another real ordered member");
            require_normal_common_child18_return_physical_without_callbacks(*child);
        }
    }
    NormalCommonTraversal::require_recursive_frame_without_callbacks(*frame,returned.root);
}

void BnwWorldViewControlTree::require_normal_common_child18_return_physical_without_callbacks(
    const NormalCommonChild18Return& returned){
    using Call=detail::BnwNormalCommonChildCall;
    const auto call=returned.original_call;
    require(returned.creator==std::this_thread::get_id()&&call&&call->creator==returned.creator&&
        call->sealed&&call->canonical_self.lock()==call&&call->kind==Call::Kind::button&&
        call->phase==Call::Phase::button_caption178&&returned.common&&
        call->common_return==returned.common&&bool(returned.null_return)!=bool(returned.own_return),
        "Recursive child18 return lacks genuine same Button/Common/caption branch");
    require_normal_common_recursive_return_physical_without_callbacks(*returned.common);
    if(returned.null_return){
        require(returned.null_return->original_call==call,"Recursive child18 NULL return changed actual Call");
        returned.null_return->require_physical_original_without_callbacks();
    }else{
        const auto& own=*returned.own_return;
        own.require_physical_current_without_callbacks();
        require(call->original_button_invocation_control&&
            call->original_button_invocation_control==own.invocation_control_&&
            own.actual_control_without_callbacks()==call->control.id_&&
            own.actual_ordered_ordinal_without_callbacks()==call->ordinal&&
            own.outcome_without_callbacks()!=detail::BnwNormalNonFontOwnReturn::Outcome::registered,
            "Recursive child18 genuine own118 returned another actual control/mandatory queue path");
        own.captured_root_without_callbacks().require_same_root_without_callbacks(returned.common->root);
        const auto history=returned.common->root.impl_;
        history->root_scope.require_physical_current_without_callbacks();
        const auto actual=history->tree->core_;
        const auto& fresh=actual->source->current_operation_loan();fresh.require_origin_without_callbacks();
        require(&fresh==history->operation&&fresh.operation_serial()==call->serial,
            "Recursive child18 retained original Source changed before caption storage");
        const auto button=call->original_button;
        require(button&&button->child_178_observation&&
            *button->child_178_observation==detail::NotificationLayoutButtonState::Child178Observation::constructor_null&&
            !button->label_178_&&!button->label_178_factory_return_&&!button->pending_label_178_&&!button->label_retirement_,
            "Recursive child18 actual NULL caption178 changed after returned suffix");
        require_button_native_common_return_owned(actual,call->control.id_,fresh,actual->busy);
        own.require_physical_current_without_callbacks();
        history->root_scope.require_physical_current_without_callbacks();fresh.require_origin_without_callbacks();
    }
    // Positive caption and registered CPU queue returns deliberately remain
    // unissued until their genuine ordered suffixes are connected.
}

void BnwWorldViewControlTree::execute_normal_common56_required_manager_prefix_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalCommonChildCall& offered)const{
    const auto call=offered; // Actual VALUE before FIRST/FULL/allocator callbacks.
    const auto origin=capture_normal_common_manager_receiver_owned(operation,call);
    execute_normal_common56_required_manager_prefix_owned(operation,*origin);
}

std::shared_ptr<const BnwWorldViewControlTree::NormalCommonRecursiveReturn>
BnwWorldViewControlTree::execute_normal_common_button_recursive78_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalCommonChildCall& actual_call,
    std::shared_ptr<const NormalCommonChildConsumers> consumers)const{
    using Call=detail::BnwNormalCommonChildCall;using Frame=NormalCommonTraversal::Frame;
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    actual_call.require_current_without_callbacks();const Call call=actual_call;
    call.require_current_without_callbacks();const auto parent_call=call.impl_;
    const auto traversal=parent_call->traversal.lock();const auto execution=traversal->execution.lock();
    const auto root=execution->root;const auto previous=parent_call->parent_loop.lock();
    const auto parent=parent_call->receiver;const auto button=parent_call->original_button;
    require(consumers&&parent_call->kind==Call::Kind::button&&button&&previous&&
        parent_call->phase==Call::Phase::common56f630&&!parent_call->common_return,
        "Recursive Common entry replay/not actual Button56F630");
    const auto first=[&]{call.require_current_without_callbacks();
        require(parent_call->phase==Call::Phase::common56f630&&!parent_call->common_return&&
            traversal->stack.size()==parent_call->depth+1&&same(traversal->stack.back(),previous),
            "Recursive Common original parent continuation changed");};
    const auto full=[&]{first();call.require_source_owned(operation);first();};
    first();full();first();
    execute_normal_common56_required_manager_prefix_owned(operation,call);
    first();full();first();
    const auto& base=*Core::node(core_->nodes,parent_call->control.id_).layout_baseline;
    const auto children=base.base_.children_78;
    require(children&&children->impl_&&same(children->impl_->parent.lock(),parent)&&
        same(base.button_,button)&&base.base_.alpha_word_44,
        "Recursive Common actual Button physical children78/44 absent");
    const auto count=children->impl_->ordered.size(),depth=parent_call->depth+1;
    const auto sequence=base.sequence_;const auto flags=base.base_.flags_40,alpha=*base.base_.alpha_word_44;
    const std::array<std::uint32_t,2> offset{std::bit_cast<std::uint32_t>(base.base_.offset_28_2c[0]),
        std::bit_cast<std::uint32_t>(base.base_.offset_28_2c[1])};
    const auto incoming=call.actual_child_offset_words_without_callbacks();
    const auto incoming_alpha=call.actual_child_alpha_word_without_callbacks();
    std::vector<std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>> ordered;
    std::vector<std::optional<std::uint32_t>> filters;
    std::vector<std::shared_ptr<const NormalCommonChild18Return>> returns;
    const auto capture_first=[&]{first();const auto& current=*Core::node(core_->nodes,parent_call->control.id_).layout_baseline;
        require(current.sequence_==sequence&&current.base_.flags_40==flags&&current.base_.alpha_word_44&&
            *current.base_.alpha_word_44==alpha&&same(current.button_,button)&&same(current.base_.children_78,children)&&
            children->impl_->ordered.size()==count&&offset==std::array<std::uint32_t,2>{
                std::bit_cast<std::uint32_t>(current.base_.offset_28_2c[0]),
                std::bit_cast<std::uint32_t>(current.base_.offset_28_2c[1])},
            "Recursive Common capture allocation POST changed real Base/count/SSE operands");
        for(std::size_t i=0;i<ordered.size();++i)require(same(ordered[i],children->impl_->ordered[i]),
            "Recursive Common captured ordered78 member replaced");};
    ordered.reserve(count);capture_first();full();capture_first();
    filters.resize(count);capture_first();full();capture_first();
    returns.resize(count);capture_first();full();capture_first();
    for(std::size_t i=0;i<count;++i)ordered.push_back(children->impl_->ordered[i]);
    auto frame=std::make_shared<Frame>(std::weak_ptr<const Call::Impl>(parent_call),parent,children,count,depth,
        sequence,flags,offset,alpha,incoming,incoming_alpha,std::move(ordered),std::move(filters),std::move(returns));
    capture_first();full();capture_first();
    require(frame->ordered.size()==count&&same(frame->parent,parent)&&same(frame->children,children),
        "Recursive Common frame allocation changed owned actual order");
    for(std::size_t i=0;i<count;++i)require(same(frame->ordered[i],children->impl_->ordered[i]),
        "Recursive Common frame allocation POST changed actual member");
    traversal->stack.reserve(depth+1);capture_first();full();capture_first();
    frame->stage=Frame::Stage::children_tested;
    // Actual NULL/count branch returns BEFORE arithmetic. A genuine ctor has
    // already issued non-NULL78 here; its count may independently be zero.
    if(count){
#if defined(__GNUC__) && defined(__x86_64__)
        asm volatile("movss %[ownalpha], %%xmm0\n\tmulss %[parentalpha], %%xmm0\n\t"
            "movss %%xmm0, %[outalpha]\n\tmovss %[parentx], %%xmm0\n\taddss %[ownx], %%xmm0\n\t"
            "movss %%xmm0, %[outx]\n\tmovss %[parenty], %%xmm0\n\taddss %[owny], %%xmm0\n\t"
            "movss %%xmm0, %[outy]\n\t"
            :[outalpha]"=m"(frame->child_alpha),[outx]"=m"(frame->child_offset[0]),[outy]"=m"(frame->child_offset[1])
            :[ownalpha]"m"(alpha),[parentalpha]"m"(incoming_alpha),[parentx]"m"(incoming[0]),
             [parenty]"m"(incoming[1]),[ownx]"m"(offset[0]),[owny]"m"(offset[1]):"xmm0","memory");
#else
        require(false,"Recursive Common56F630 requires proved GNU x64 raw SSE kernel");
#endif
        frame->arithmetic_earned=true;
    }
    frame->stage=count?Frame::Stage::arithmetic:Frame::Stage::returned;frame->sealed=true;
    traversal->current.reset();traversal->phase=NormalCommonTraversal::Phase::recursive_common;
    traversal->stack.push_back(frame); //capacity earned; persistent backedges are weak
    const auto frame_first=[&]{execution->first();root.require_physical_current_without_callbacks();
        require(traversal->alive.load()&&traversal->sealed&&same(execution->common_traversal,traversal)&&
            traversal->stack.size()==depth+1&&same(traversal->stack.back(),frame)&&
            parent_call->alive.load()&&parent_call->phase==Call::Phase::common56f630&&
            previous->alive&&previous->sealed&&previous->stage==Frame::Stage::child18&&
            previous->cursor==parent_call->ordinal&&!parent_call->common_return,
            "Recursive Common suspended parent/current frame changed");
        NormalCommonTraversal::require_recursive_frame_without_callbacks(*frame,root);};
    const auto frame_full=[&]{frame_first();execution->full(operation);frame_first();};
    frame_first();frame_full();frame_first();
    while(frame->cursor<count){
        frame_first();frame_full();frame_first();
        require(traversal->current.expired()&&frame->stage==Frame::Stage::arithmetic,
            "Recursive Common previous child18 has not actually returned");
        const auto ordinal=frame->cursor;const auto receiver=frame->ordered[ordinal];
        const auto flags40=require_normal_common_actual_child40_without_callbacks(core_,parent,receiver);
        frame_first();
        if(flags40&0x2002u){frame->filter_words[ordinal]=flags40;++frame->cursor;continue;}
        const auto fields=receiver->impl_;const auto label=fields->standalone_label.lock();
        const auto child_button=fields->button.lock();
        require(bool(label)!=bool(child_button),
            "Recursive56F725 reached real Image/Box/Grid child18 supplier unissued; native2002 does not skip it");
        const auto kind=label?Call::Kind::standalone_label:Call::Kind::button;
        const auto& physical=*Core::node(core_->nodes,fields->id).layout_baseline;
        const auto child_sequence=physical.sequence_;const Control actual_control(core_,fields->id);
        auto owned=std::make_shared<Call::Impl>(std::weak_ptr<NormalCommonTraversal>(traversal),
            std::weak_ptr<Frame>(frame),actual_control,parent,receiver,children,parent_call->serial,
            child_sequence,ordinal,count,depth,flags40,kind,Call::Phase::child18,label,child_button);
        frame_first();frame_full();frame_first();
        require(frame->cursor==ordinal&&traversal->current.expired()&&
            Core::node(core_->nodes,fields->id).layout_baseline->sequence_==child_sequence&&
            require_normal_common_actual_child40_without_callbacks(core_,parent,receiver)==flags40,
            "Recursive child18 allocation POST changed actual receiver/word/cursor");
        frame->stage=Frame::Stage::child18;traversal->phase=NormalCommonTraversal::Phase::child18;
        traversal->current=owned;owned->canonical_self=owned;owned->sealed=true;
        const Call::IssuerKey key;const Call child(owned,key);
        child.require_current_without_callbacks();child.require_source_owned(operation);child.require_current_without_callbacks();
        const auto returned=execute_normal_common_child18_owned(operation,child,consumers);
        // Producer returned a CLOSED actual full child18 receipt, not own118
        // alone, a caller bool, fake nil caption or a mutable control facade.
        child.require_current_without_callbacks();frame_full();child.require_current_without_callbacks();
        require(returned&&same(returned->original_call,owned)&&!frame->returned_children[ordinal]&&
            !frame->filter_words[ordinal],"Recursive child18 returned another actual Call/member");
        require_normal_common_child18_return_physical_without_callbacks(*returned);
        frame->returned_children[ordinal]=returned;
        owned->alive.store(false);traversal->current.reset();
        ++frame->cursor;frame->stage=Frame::Stage::arithmetic;
        traversal->phase=NormalCommonTraversal::Phase::recursive_common;
        frame_first();frame_full();frame_first();
    }
    frame->stage=Frame::Stage::returned;frame_first();frame_full();frame_first();
    require(frame->cursor==frame->count&&traversal->current.expired(),
        "Recursive Common actual local loop has not fully returned");
    auto returned=std::shared_ptr<const NormalCommonRecursiveReturn>(
        new NormalCommonRecursiveReturn(std::weak_ptr<const Call::Impl>(parent_call),root,frame));
    frame_first();frame_full();frame_first();
    require(!parent_call->common_return&&frame->cursor==count&&traversal->current.expired(),
        "Recursive Common actual returned journal changed through return allocation");
    // Pop only the genuine current frame. The ancestor's saved cursor was not
    // advanced or reconstructed; its SAME still-live Call becomes current.
    traversal->stack.pop_back();frame->popped=true;parent_call->common_return=returned;
    traversal->current=parent_call;traversal->phase=NormalCommonTraversal::Phase::child18;
    call.require_current_without_callbacks();require_normal_common_recursive_return_physical_without_callbacks(*returned);
    call.require_source_owned(operation);call.require_current_without_callbacks();return returned;
}
