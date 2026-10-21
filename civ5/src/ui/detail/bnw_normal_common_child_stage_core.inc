// Inside civ5::ui AFTER complete normal Material, queue and child-call owners.
// Genuine no-index Context Common entry and current native child18 execution.
// Required unported children/caption/drain suppliers remain exact STOPs.
void BnwWorldViewControlTree::require_normal_common_traversal_capture_without_callbacks(
    const detail::BnwNormalGuiBatchPrefix::Impl& batch){
    const auto traversal=batch.common_dispatch.lock();
    require(traversal&&traversal->creator==std::this_thread::get_id()&&traversal->alive.load()&&traversal->sealed,
        "Mixed Common capture original dispatcher retired/foreign/unsealed");
    const auto execution=traversal->execution.lock();
    require(bool(execution),"Mixed Common capture original Execution retired");execution->first();
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto history=execution->root.impl_;
    require(history&&history->prefix.get()==&batch&&same(execution->common_traversal,traversal)&&
        batch.context_entry&&!batch.index&&!batch.control&&!batch.indexed_control&&
        traversal->phase==NormalCommonTraversal::Phase::captured&&traversal->stack.empty()&&
        traversal->current.expired()&&traversal->serial==batch.serial,
        "Mixed Common capture is not the SAME once-only Context entry");
    execution->root.require_physical_current_without_callbacks();
    traversal->entry.require_current_without_callbacks();
    traversal->entry.require_same_tree_without_callbacks(*history->tree);
    traversal->entry.require_same_entry_without_callbacks(*batch.context_entry);
    const auto actual=history->tree->core_;actual->layout_origin(true);
    const auto& current=actual->source->current_operation_loan();current.require_origin_without_callbacks();
    require(&current==batch.operation&&current.operation_serial()==batch.serial,
        "Mixed Common capture canonical Source changed before saved operation");
    execution->first();require(traversal->alive.load()&&same(batch.common_dispatch.lock(),traversal),
        "Mixed Common capture current original dispatcher changed at PURE POST");
}

void BnwWorldViewControlTree::attach_normal_common_button_invocation_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwNormalCommonChildCall& call,
    const detail::BnwNormalNonFontPaintInvocation& invocation,
    const BnwWorldViewControlRenderPublication& publication,const detail::BnwNormalRootPaintCallFrame::Guard& frame)const{
    call.require_current_without_callbacks();frame.require_current_without_callbacks();invocation.require_current_without_callbacks();
    const auto offered=call.impl_;const auto traversal=offered->traversal.lock();
    const auto execution=traversal->execution.lock();const auto root=execution->root;
    const auto captured_invocation=std::static_pointer_cast<const void>(invocation.impl_);
    require(bool(captured_invocation),"Common Button attachment genuine Invocation control absent");
    const auto pure=[&]{
        call.require_current_without_callbacks();frame.require_current_without_callbacks();invocation.require_current_without_callbacks();
        require(offered->kind==detail::BnwNormalCommonChildCall::Kind::button&&
            offered->phase==detail::BnwNormalCommonChildCall::Phase::button_own118&&
            root.impl_->tree.get()==this&&publication.tree_.get()==this&&publication.control_&&
            !publication.root_context_entry_&&publication.enclosing_context_entry_&&publication.adjusted_frame_painting_,
            "Common Button attachment needs SAME current ordinary child Publication under Context");
        publication.enclosing_context_entry_->require_same_entry_without_callbacks(traversal->entry);
        require(publication.actual_checked_control_id_for_paint_without_callbacks()==offered->control.id_&&
            publication.tree_->core_->checked(*publication.control_)==offered->control.id_,
            "Common Button attachment actual Publication Control changed");
        frame.require_actual_publication_without_callbacks(publication);
        frame.require_actual_control_without_callbacks(*this,offered->control.id_);
        frame.require_actual_renderer_without_callbacks(*publication.renderer_);
        frame.require_actual_frame_owner_without_callbacks(publication.adjusted_frame_owner_);
        invocation.require_same_root_without_callbacks(root);
        invocation.require_actual_publication_without_callbacks(publication);
        invocation.require_actual_frame_without_callbacks(frame);
        invocation.require_actual_renderer_without_callbacks(*publication.renderer_);
        require(invocation.actual_control_without_callbacks()==offered->control.id_&&
            invocation.actual_ordered_ordinal_without_callbacks()==offered->ordinal,
            "Common Button genuine Invocation actual child/ordinal differs");
        const auto actual_control=std::static_pointer_cast<const void>(invocation.impl_);
        require(actual_control.get()==captured_invocation.get()&&
            !actual_control.owner_before(captured_invocation)&&!captured_invocation.owner_before(actual_control),
            "Common Button genuine Invocation shared control substituted");
    };
    const auto full=[&]{pure();call.require_source_owned(operation);pure();
        frame.require_source_owned(operation);pure();invocation.require_source_owned(operation);pure();};
    pure();full();pure();
    require(!offered->original_button_invocation_control,
        "Common Button Invocation attachment is once-only, before own118 effects");
    // Nonthrowing shared-control custody; no allocation or callback between the
    // retirement-sensitive check and this store. No ACTIVE facade is copied.
    offered->original_button_invocation_control=captured_invocation;
    pure();full();pure();
    require(offered->original_button_invocation_control.get()==captured_invocation.get()&&
        !offered->original_button_invocation_control.owner_before(captured_invocation)&&
        !captured_invocation.owner_before(offered->original_button_invocation_control),
        "Common Button attachment SAME original shared control changed at POST");
}

std::array<std::uint32_t,2> detail::BnwNormalCommonChildCall::actual_child_offset_words_without_callbacks()const{
    require_current_without_callbacks();const auto loop=impl_->parent_loop.lock();
    require(loop&&loop->count&&((loop->depth==0&&loop->root_common)||
        (loop->depth>0&&!loop->root_common&&loop->arithmetic_earned)),
        "Common child offset requires real local-depth arithmetic return");
    const auto words=loop->depth?loop->child_offset:loop->root_common->child_offset;
    require_current_without_callbacks();return words;
}
std::uint32_t detail::BnwNormalCommonChildCall::actual_child_alpha_word_without_callbacks()const{
    require_current_without_callbacks();const auto loop=impl_->parent_loop.lock();
    require(loop&&loop->count&&((loop->depth==0&&loop->root_common)||
        (loop->depth>0&&!loop->root_common&&loop->arithmetic_earned)),
        "Common child alpha requires real local-depth arithmetic return");
    const auto word=loop->depth?loop->child_alpha:loop->root_common->child_alpha;
    require_current_without_callbacks();return word;
}

std::shared_ptr<const BnwWorldViewControlTree::NormalCommonChild18Return>
BnwWorldViewControlTree::execute_normal_common_child18_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalCommonChildCall& actual_call,
    std::shared_ptr<const NormalCommonChildConsumers> consumers)const{
    using Call=detail::BnwNormalCommonChildCall;
    actual_call.require_current_without_callbacks();const Call call=actual_call;
    call.require_current_without_callbacks();const auto owned=call.impl_;
    const auto traversal=owned->traversal.lock();const auto execution=traversal->execution.lock();
    const auto root=execution->root;const auto matrix=execution->original_matrix;
    const auto receiver=owned->receiver;const auto control_id=owned->control.id_;
    const auto ordinal=owned->ordinal;const auto kind=owned->kind;
    require(consumers&&owned->phase==Call::Phase::child18&&!owned->common_return,
        "Common actual child18 suffix replay/another phase");
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto first=[&]{call.require_current_without_callbacks();};
    const auto full=[&]{first();call.require_source_owned(operation);first();};
    first();full();first();
    if(kind==Call::Kind::standalone_label)
        require(false,"Mixed standalone Label child18 body/return needs token-linked genuine supplier; indexed bootstrap is forbidden");
    owned->phase=Call::Phase::button_own118;call.require_current_without_callbacks();
    if(!call.actual_button_texture_is_nonnull_without_callbacks()){
        // Genuine covered NULL44 + actual init OR40 skips own118.
        // No UI draw callback, CURRENT Invocation, attachment or fake
        // zero-index OwnReturn is created for this Original branch.
        require(!owned->original_button_invocation_control,
            "Mixed NULL Button unexpectedly owns an own118 Invocation");
        owned->phase=Call::Phase::common56f630;call.require_current_without_callbacks();
        const auto button=owned->original_button;
        const auto& actual_base=*Core::node(core_->nodes,control_id).layout_baseline;
        const auto children=actual_base.base_.children_78;
        require(button&&same(actual_base.button_,button)&&children&&children->impl_&&
            same(children->impl_->parent.lock(),receiver),
            "Mixed NULL Button Common genuine children78 unavailable");
        const auto common_return=execute_normal_common_button_recursive78_owned(operation,call,consumers);
        call.require_current_without_callbacks();require_normal_common_recursive_return_physical_without_callbacks(*common_return);
        owned->phase=Call::Phase::button_caption178;call.require_current_without_callbacks();
        if(button->label_178_)
            execute_normal_common_button_caption178_owned(operation,call,consumers);
        require(button->child_178_observation&&
            *button->child_178_observation==detail::NotificationLayoutButtonState::Child178Observation::constructor_null&&
            !button->label_178_factory_return_&&!button->pending_label_178_&&!button->label_retirement_,
            "Mixed NULL Button NULL178 lacks actual constructor/MISS association");
        call.require_source_owned(operation);call.require_current_without_callbacks();
        const auto null_return=capture_normal_common_null_child_return_owned(operation,call);
        null_return->require_physical_original_without_callbacks();
        const auto before=traversal->button_null_returns.size();
        require(before<traversal->button_null_returns.max_size(),
            "Mixed NULL child return journal exhausted");
        traversal->button_null_returns.reserve(before+1);
        call.require_current_without_callbacks();full();call.require_current_without_callbacks();
        null_return->require_physical_original_without_callbacks();
        require(traversal->button_null_returns.size()==before&&traversal->button_null_returns.capacity()>before,
            "Mixed genuine NULL child return journal allocation changed");
        // Reserved append and actual caller retirement/advance contain
        // no allocations or callbacks. The retained return checker
        // after these stores deliberately does not require Call alive.
        traversal->button_null_returns.push_back(null_return);
        auto returned=std::shared_ptr<const NormalCommonChild18Return>(
            new NormalCommonChild18Return(owned,common_return,null_return,std::nullopt));
        call.require_current_without_callbacks();full();call.require_current_without_callbacks();
        require_normal_common_child18_return_physical_without_callbacks(*returned);return returned;
    }
    require((owned->physical_flags&0x10000000u)==0,
        "Mixed nonNULL Button native no-own flag requires a distinct genuine suffix supplier");
    require(bool(consumers->button_own118),"Mixed Button own118 typed consumer unavailable");
    auto own_return=consumers->button_own118(operation,root,matrix,call);
    // The original child paint may have returned. Never call ACTIVE
    // Invocation/Frame/Publication here; use genuine OwnReturn health.
    call.require_current_without_callbacks();full();call.require_current_without_callbacks();
    own_return.require_physical_current_without_callbacks();
    require(owned->original_button_invocation_control&&
        same(owned->original_button_invocation_control,own_return.invocation_control_),
        "Mixed Button own118 returned another Invocation or missing once-before-effects attachment");
    require(own_return.actual_control_without_callbacks()==control_id&&
        own_return.actual_ordered_ordinal_without_callbacks()==ordinal,
        "Mixed Button own118 returned another actual child");
    own_return.captured_root_without_callbacks().require_same_root_without_callbacks(root);
    own_return.require_source_owned(operation);call.require_current_without_callbacks();
    owned->phase=Call::Phase::common56f630;call.require_current_without_callbacks();
    const auto button=owned->original_button;const auto& actual_base=*Core::node(core_->nodes,control_id).layout_baseline;
    const auto children=actual_base.base_.children_78;
    require(button&&same(actual_base.button_,button)&&children&&children->impl_&&
        same(children->impl_->parent.lock(),receiver),"Mixed Button Common genuine children78 unavailable");
    // Actual Common always follows own118. Local physical78/count and
    // ordered native children return before optional embedded caption178.
    const auto common_return=execute_normal_common_button_recursive78_owned(operation,call,consumers);
    call.require_current_without_callbacks();require_normal_common_recursive_return_physical_without_callbacks(*common_return);
    owned->phase=Call::Phase::button_caption178;call.require_current_without_callbacks();
    if(button->label_178_)
        execute_normal_common_button_caption178_owned(operation,call,consumers);
    require(button->child_178_observation&&
        *button->child_178_observation==detail::NotificationLayoutButtonState::Child178Observation::constructor_null&&
        !button->label_178_factory_return_&&!button->pending_label_178_&&!button->label_retirement_,
        "Mixed Button NULL178 does not have genuine original constructor/MISS association");
    call.require_source_owned(operation);call.require_current_without_callbacks();
    // A registered nonfont CPU record needs its actual outer-window
    // association and typed selected-entry consumer. Label drain is
    // not a substitute for that mandatory manager path.
    if(own_return.outcome_without_callbacks()==detail::BnwNormalNonFontOwnReturn::Outcome::registered)
        require(false,"Mixed Button registered nonfont record needs genuine manager registration/selected-entry drain");
    own_return.require_physical_current_without_callbacks();own_return.require_source_owned(operation);
    call.require_current_without_callbacks();
    // These two Original own118 outcomes really return without a draw
    // record. Retain their SAME immutable return before caller advance.
    const auto before=traversal->button_returns.size();
    traversal->button_returns.reserve(before+1);
    call.require_current_without_callbacks();full();own_return.require_physical_current_without_callbacks();
    require(traversal->button_returns.size()==before,"Mixed Button return journal allocation changed");
    traversal->button_returns.push_back(own_return);
    auto returned=std::shared_ptr<const NormalCommonChild18Return>(
        new NormalCommonChild18Return(owned,common_return,nullptr,own_return));
    call.require_current_without_callbacks();full();call.require_current_without_callbacks();
    require_normal_common_child18_return_physical_without_callbacks(*returned);return returned;
}

void BnwWorldViewControlTree::stage_normal_root_common_children_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalRootBatchExecution& original_execution,
    std::shared_ptr<const NormalCommonChildConsumers> actual_consumers)const{
    using Execution=detail::BnwNormalRootBatchExecution;using Prefix=detail::BnwNormalGuiBatchPrefix::Impl;
    using Common=Prefix::CommonRoot;using Call=detail::BnwNormalCommonChildCall;
    const auto execution=original_execution.impl_;const auto consumers=std::move(actual_consumers);
    require(execution&&consumers,"Mixed Common entry needs original Execution and owned typed consumers");
    execution->first();const auto root=execution->root;const auto history=root.impl_;
    const auto prefix=history->prefix;const auto matrix=execution->original_matrix;
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto first=[&]{execution->first();root.require_physical_current_without_callbacks();
        require(history->tree.get()==this&&core_->busy&&prefix->context_entry&&
            !prefix->index&&!prefix->control&&!prefix->indexed_control&&execution->original_prefix&&
            same(execution->original_prefix->impl_,prefix)&&same(history->matrix_facade,matrix),
            "Mixed Common entry lacks SAME whole Context scope/original Prefix/Matrix");
        prefix->context_entry->require_current_without_callbacks();
        prefix->context_entry->require_same_tree_without_callbacks(*this);};
    const auto full=[&]{first();execution->full(operation);first();};
    first();full();first();
    require(!execution->common_traversal&&prefix->common_dispatch.expired()&&!prefix->common_root&&
        execution->original_materials.empty()&&!history->label_traversal_active&&!history->label_traversal_completed&&
        history->current_visible_label.expired()&&execution->label_phase==Execution::Impl::LabelPhase::unstarted,
        "Mixed Common entry replay/another child dispatcher/Common already executed");
    const auto entry=*prefix->context_entry;const auto serial=prefix->serial;
    std::shared_ptr<NormalCommonTraversal> traversal;
    try{
        traversal=std::make_shared<NormalCommonTraversal>(std::weak_ptr<Execution::Impl>(execution),entry,serial);
        first();full();first();
        require(!execution->common_traversal&&prefix->common_dispatch.expired()&&!prefix->common_root&&
            traversal->creator==std::this_thread::get_id()&&traversal->serial==serial&&traversal->alive.load()&&
            traversal->phase==NormalCommonTraversal::Phase::captured&&traversal->stack.empty()&&!traversal->sealed,
            "Mixed Common dispatcher allocation POST changed original entry");
        // Exact enclosing owner; weak backedge never owns Execution or Prefix.
        execution->common_traversal=traversal;prefix->common_dispatch=traversal;traversal->sealed=true;
        require_normal_common_traversal_capture_without_callbacks(*prefix);full();
        // ONE actual Common entry executes physical78/count and the raw SSE
        // arithmetic. Its additive deferred branch leaves the real loop to us.
        execute_normal_context_selected_label_entry_owned(prefix,nullptr);
        first();full();first();const auto common=prefix->common_root;
        require(common&&!common->continuing&&!common->whole_return&&!common->label_tail&&
            (common->stage==Common::Stage::arithmetic||
             (common->stage==Common::Stage::returned_empty&&!common->count)),
            "Mixed Common did not return actual captured/count-tested entry");
        auto loop=std::make_shared<NormalCommonTraversal::Frame>(common,common->root,common->children,common->count,0);
        first();full();first();require_normal_common_traversal_capture_without_callbacks(*prefix);
        require(common==prefix->common_root&&!common->whole_return&&!common->continuing&&common->cursor==0,
            "Mixed Common frame allocation POST changed actual first cursor");
        traversal->stack.reserve(1);first();full();first();
        require_normal_common_traversal_capture_without_callbacks(*prefix);
        loop->stage=common->count?NormalCommonTraversal::Frame::Stage::arithmetic:NormalCommonTraversal::Frame::Stage::returned;
        loop->sealed=true;traversal->stack.push_back(loop);
        const auto loop_first=[&]{first();require(traversal->alive.load()&&traversal->sealed&&
            same(execution->common_traversal,traversal)&&same(prefix->common_dispatch.lock(),traversal)&&
            traversal->stack.size()==1&&same(traversal->stack.back(),loop)&&loop->alive&&loop->sealed&&
            loop->creator==std::this_thread::get_id()&&same(loop->root_common,common)&&
            same(loop->parent,common->root)&&same(loop->children,common->children)&&
            loop->count==common->count&&loop->depth==0&&loop->cursor==common->cursor&&
            common->cursor<=common->count&&!common->continuing&&!common->whole_return,
            "Mixed Common actual loop/cursor/current owner changed");};
        while(loop->cursor<loop->count){
            loop_first();full();loop_first();
            require(traversal->current.expired()&&loop->stage==NormalCommonTraversal::Frame::Stage::arithmetic&&
                common->stage==Common::Stage::arithmetic,"Mixed Common previous native child has not returned");
            const auto ordinal=loop->cursor;const auto receiver=common->ordered[ordinal];
            require(receiver&&receiver->impl_,"Mixed Common ordered78 contains absent actual receiver");
            const auto fields=receiver->impl_;const auto owner=fields->core.lock();
            require(owner&&same(owner,core_)&&fields->id&&same(fields->canonical.lock(),receiver),
                "Mixed Common actual cross-Tree/Context receiver requires separate native dispatcher");
            const auto control_id=fields->id;const auto& node=Core::node(core_->nodes,control_id);
            require(node.alive&&!node.dynamic&&node.layout_baseline&&node.layout_receiver_occurrence&&
                same(node.layout_receiver_occurrence,fields->occurrence.lock()),
                "Mixed Common child actual constructor occurrence unavailable");
            const auto& baseline=*node.layout_baseline;
            require(same(baseline.base_.receiver_owned,receiver)&&same(baseline.base_.parent_38.lock(),common->root)&&
                same(common->children->impl_->ordered[ordinal],receiver),
                "Mixed Common child SAME Base/paired38/ordered78 changed");
            const auto actual_label=fields->standalone_label.lock();const auto actual_button=fields->button.lock();
            const auto flags=require_normal_common_actual_child40_without_callbacks(core_,common->root,receiver);
            loop_first();
            if(flags&0x2002u){ //Genuine physical56F706/56F70D, before child18.
                require(loop->filter_words.size()==loop->count&&!loop->filter_words[loop->cursor]&&
                    !loop->returned_children[loop->cursor],"Mixed Root native40 filter journal replay/absent");
                loop->filter_words[loop->cursor]=flags;
                ++common->cursor;++loop->cursor;loop_first();continue;
            }
            require(bool(actual_label)!=bool(actual_button),
                "Mixed Common reached real Image/Box/Grid child18 supplier unissued; native2002 does not skip it");
            const auto kind=actual_label?Call::Kind::standalone_label:Call::Kind::button;
            const auto sequence=baseline.sequence_;
            const Control actual_control(core_,control_id);
            auto owned=std::make_shared<Call::Impl>(std::weak_ptr<NormalCommonTraversal>(traversal),
                std::weak_ptr<NormalCommonTraversal::Frame>(loop),actual_control,common->root,receiver,common->children,
                serial,sequence,ordinal,loop->count,0,flags,kind,Call::Phase::child18,actual_label,actual_button);
            loop_first();full();loop_first();
            const auto& now=*Core::node(core_->nodes,control_id).layout_baseline;
            require(common->cursor==ordinal&&loop->cursor==ordinal&&now.sequence_==sequence&&
                same(now.base_.receiver_owned,receiver)&&now.base_.flags_40==flags&&
                traversal->current.expired(),"Mixed Common pending child allocation POST changed genuine call");
            common->stage=Common::Stage::child18_required;loop->stage=NormalCommonTraversal::Frame::Stage::child18;
            traversal->phase=NormalCommonTraversal::Phase::child18;traversal->current=owned;
            owned->canonical_self=owned;owned->sealed=true;
            const Call::IssuerKey key;const Call call(owned,key);
            call.require_current_without_callbacks();call.require_source_owned(operation);call.require_current_without_callbacks();
            const auto child_return=execute_normal_common_child18_owned(operation,call,consumers);
            call.require_current_without_callbacks();full();call.require_current_without_callbacks();
            require(child_return&&same(child_return->original_call,owned),
                "Mixed Common child18 returned another actual closed Call");
            require_normal_common_child18_return_physical_without_callbacks(*child_return);
            require(loop->returned_children.size()==loop->count&&!loop->returned_children[ordinal]&&
                !loop->filter_words[ordinal],"Mixed Root actual returned child journal unavailable/replayed");
            loop->returned_children[ordinal]=child_return;
            owned->alive.store(false);traversal->current.reset();
            ++common->cursor;++loop->cursor;common->stage=Common::Stage::arithmetic;
            loop->stage=NormalCommonTraversal::Frame::Stage::arithmetic;
            traversal->phase=NormalCommonTraversal::Phase::recursive_common;
            loop_first();full();loop_first();
        }
        loop->stage=NormalCommonTraversal::Frame::Stage::returned;
        // No completed Common/Root/queue seal is minted from the pending ABI.
        // The mixed return journal/current registered queue publisher is a
        // separate Root-owned dependency, including the genuine empty case.
        require(false,"Mixed whole Root completed-return/registration/drain publisher still unissued");
    }catch(...){
        if(traversal){if(const auto current=traversal->current.lock())current->alive.store(false);
            traversal->current.reset();traversal->alive.store(false);}
        prefix->alive.store(false);prefix->batch->alive.store(false);
        if(const auto manager=history->gui.lock())manager->failed.store(true);
        core_->failed.store(true);throw;
    }
}

bool BnwWorldViewControlTree::normal_common_child_call_matches_actual_control_without_callbacks(
    const detail::BnwNormalCommonChildCall& call,const Control& candidate){
    // Genuine pending Call/Root/phase FIRST before any candidate Tree borrow.
    require_normal_common_child_call_current_without_callbacks(call);
    const auto offered=call.impl_;const auto phase=offered->phase;
    const auto actual=offered->control.owner_.lock();const auto owner=candidate.owner_.lock();
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require(actual&&owner&&same(actual,owner),
        "Common candidate Control does not belong to SAME live canonical Tree");
    // A retired/unknown Control is an error even when its id differs. The
    // generic checked() bridge preserves canonical node lifetime; no XML kind
    // or ordered ordinal is interpreted as a paint index.
    const auto id=actual->checked(candidate);
    const bool matches=id==offered->control.id_;
    require_normal_common_child_call_current_without_callbacks(call);
    const auto current_owner=candidate.owner_.lock();
    require(current_owner&&same(current_owner,owner)&&same(offered->control.owner_.lock(),actual)&&
        offered->phase==phase&&actual->checked(candidate)==id,
        "Common candidate Control owner/life/phase changed at PURE POST");
    return matches; // false is only another live Control of this SAME Tree.
}

bool detail::BnwNormalCommonChildCall::matches_actual_control_without_callbacks(
    const BnwWorldViewControlTree::Control& candidate)const{
    return BnwWorldViewControlTree::normal_common_child_call_matches_actual_control_without_callbacks(*this,candidate);
}
