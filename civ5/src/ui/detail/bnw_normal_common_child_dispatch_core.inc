// Inside civ5::ui AFTER original Context Entry/Scope, Prefix/History/Execution
// and physical ordinary storage. Pending-call capability only in this packet;
// exact traversal/issuer bodies follow separately. No Frame/index/return mint.
struct BnwWorldViewControlTree::NormalCommonTraversal final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true};
    const std::weak_ptr<detail::BnwNormalRootBatchExecution::Impl> execution;
    const detail::BnwNormalRootContextEntry entry;
    const std::uint64_t serial;
    enum class Phase {captured,child18,recursive_common,whole_return,registered,completed};
    Phase phase=Phase::captured; //Private execution state, not an Original field.
    struct Frame final {
        const std::thread::id creator=std::this_thread::get_id();
        bool alive=true,sealed=false;
        const std::shared_ptr<detail::BnwNormalGuiBatchPrefix::Impl::CommonRoot> root_common;
        const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> parent;
        const std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> children;
        const std::size_t count,depth;
        const std::weak_ptr<const detail::BnwNormalCommonChildCall::Impl> original_parent_call;
        const std::uint64_t base_sequence=0;
        const std::uint32_t physical_flags=0;
        const std::array<std::uint32_t,2> physical_offset{},incoming_offset{};
        const std::uint32_t physical_alpha=0,incoming_alpha=0;
        std::array<std::uint32_t,2> child_offset{};
        std::uint32_t child_alpha=0;
        bool arithmetic_earned=false,popped=false;
        const std::vector<std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>> ordered;
        std::vector<std::optional<std::uint32_t>> filter_words;
        std::vector<std::shared_ptr<const NormalCommonChild18Return>> returned_children;
        enum class Stage {captured,children_tested,arithmetic,child18,returned};
        Stage stage=Stage::captured;
        std::size_t cursor=0; //Private loop cursor, advanced only after real RETURN.
        Frame(std::shared_ptr<detail::BnwNormalGuiBatchPrefix::Impl::CommonRoot> actual_root_common,
            std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> actual_parent,
            std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> actual_children,
            std::size_t actual_count,std::size_t actual_depth)
            :root_common(std::move(actual_root_common)),parent(std::move(actual_parent)),
             children(std::move(actual_children)),count(actual_count),depth(actual_depth),filter_words(actual_count),returned_children(actual_count){}
        Frame(std::weak_ptr<const detail::BnwNormalCommonChildCall::Impl> actual_call,
            std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> actual_parent,
            std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> actual_children,
            std::size_t actual_count,std::size_t actual_depth,std::uint64_t sequence,std::uint32_t flags,
            std::array<std::uint32_t,2> offset,std::uint32_t alpha,
            std::array<std::uint32_t,2> incoming,std::uint32_t incoming_scalar,
            std::vector<std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>> actual_order,
            std::vector<std::optional<std::uint32_t>> actual_filter_words,
            std::vector<std::shared_ptr<const NormalCommonChild18Return>> actual_child_returns)
            :parent(std::move(actual_parent)),children(std::move(actual_children)),count(actual_count),depth(actual_depth),
             original_parent_call(std::move(actual_call)),base_sequence(sequence),physical_flags(flags),
             physical_offset(offset),incoming_offset(incoming),physical_alpha(alpha),incoming_alpha(incoming_scalar),
             ordered(std::move(actual_order)),filter_words(std::move(actual_filter_words)),
             returned_children(std::move(actual_child_returns)){}
    };
    std::vector<std::shared_ptr<Frame>> stack;
    std::weak_ptr<const detail::BnwNormalCommonChildCall::Impl> current;
    std::vector<detail::BnwNormalNonFontOwnReturn> button_returns;
    std::vector<std::shared_ptr<const NormalCommonNullChildReturn>> button_null_returns;
    static void require_current_call_impl_without_callbacks(
        const std::shared_ptr<const detail::BnwNormalCommonChildCall::Impl>&);
    static void require_recursive_frame_without_callbacks(const Frame&,const detail::BnwNormalRootBatchHistory::Guard&);
    bool sealed=false;
    NormalCommonTraversal(std::weak_ptr<detail::BnwNormalRootBatchExecution::Impl> actual_execution,
        detail::BnwNormalRootContextEntry actual_entry,std::uint64_t actual_serial)
        :execution(std::move(actual_execution)),entry(std::move(actual_entry)),serial(actual_serial){}
};

struct detail::BnwNormalCommonChildCall::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    mutable std::atomic<bool> alive{true};
    const std::weak_ptr<BnwWorldViewControlTree::NormalCommonTraversal> traversal;
    const std::weak_ptr<BnwWorldViewControlTree::NormalCommonTraversal::Frame> parent_loop;
    const BnwWorldViewControlTree::Control control;
    const std::shared_ptr<const NotificationLayoutOrdinaryReceiver> parent,receiver;
    const std::shared_ptr<NotificationLayoutOrdinaryChildren78> children;
    const std::uint64_t serial,base_sequence;
    const std::size_t ordinal,count,depth;
    const std::uint32_t physical_flags;
    const Kind kind;
    const std::shared_ptr<NotificationLayoutStandaloneLabelState> original_label;
    const std::shared_ptr<NotificationLayoutButtonState> original_button;
    mutable std::shared_ptr<const void> original_button_invocation_control;
    mutable std::optional<std::weak_ptr<const BnwNormalButtonCaptionCall::Impl>> original_caption_call_control;
    mutable std::shared_ptr<const BnwWorldViewControlTree::NormalCommonRecursiveReturn> common_return;
    mutable Phase phase;
    std::weak_ptr<const Impl> canonical_self;
    bool sealed=false;
    Impl(std::weak_ptr<BnwWorldViewControlTree::NormalCommonTraversal> actual_traversal,
        std::weak_ptr<BnwWorldViewControlTree::NormalCommonTraversal::Frame> actual_parent_loop,
        BnwWorldViewControlTree::Control actual_control,
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> actual_parent,
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> actual_receiver,
        std::shared_ptr<NotificationLayoutOrdinaryChildren78> actual_children,
        std::uint64_t actual_serial,std::uint64_t actual_sequence,std::size_t actual_ordinal,
        std::size_t actual_count,std::size_t actual_depth,std::uint32_t actual_flags,Kind actual_kind,Phase actual_phase,
        std::shared_ptr<NotificationLayoutStandaloneLabelState> actual_label,
        std::shared_ptr<NotificationLayoutButtonState> actual_button)
        :traversal(std::move(actual_traversal)),parent_loop(std::move(actual_parent_loop)),control(std::move(actual_control)),
         parent(std::move(actual_parent)),receiver(std::move(actual_receiver)),children(std::move(actual_children)),
         serial(actual_serial),base_sequence(actual_sequence),ordinal(actual_ordinal),count(actual_count),
         depth(actual_depth),physical_flags(actual_flags),kind(actual_kind),original_label(std::move(actual_label)),
         original_button(std::move(actual_button)),phase(actual_phase){}
};

void BnwWorldViewControlTree::NormalCommonTraversal::require_current_call_impl_without_callbacks(
    const std::shared_ptr<const detail::BnwNormalCommonChildCall::Impl>& call){
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require(call&&call->creator==std::this_thread::get_id()&&call->alive.load()&&call->sealed&&
        same(call->canonical_self.lock(),call),"Common child18 offered control retired/foreign/unsealed");
    //Own C++ lifetime and genuine Execution FIRST before any borrowed Tree,
    //Source facade, ordinary receiver or saved operation is dereferenced.
    const auto traversal=call->traversal.lock();
    require(traversal&&traversal->creator==call->creator&&traversal->alive.load()&&traversal->sealed&&
        traversal->phase==NormalCommonTraversal::Phase::child18&&same(traversal->current.lock(),call),
        "Common child18 is not the SAME active dispatcher call");
    const auto execution=traversal->execution.lock();
    require(bool(execution),"Common child18 original Execution retired");execution->first();
    const auto root=execution->root;root.require_physical_current_without_callbacks();
    const auto history=root.impl_;const auto prefix=history->prefix;
    require(history->tree&&history->serial==call->serial&&traversal->serial==call->serial&&
        same(execution->common_traversal,traversal)&&prefix&&prefix->context_entry&&
        !prefix->index&&!prefix->control&&!prefix->indexed_control,
        "Common child18 needs SAME genuine no-index Context Root execution");
    const auto actual=history->tree->core_;actual->layout_origin(true);
    require(actual->busy,"Common child18 needs the genuine enclosing Context scope");
    const auto& fresh=actual->source->current_operation_loan();fresh.require_origin_without_callbacks();
    require(&fresh==prefix->operation&&fresh.operation_serial()==call->serial,
        "Common child18 canonical Source changed before saved borrower");
    traversal->entry.require_current_without_callbacks();
    traversal->entry.require_same_tree_without_callbacks(*history->tree);
    traversal->entry.require_same_entry_without_callbacks(*prefix->context_entry);
    const auto loop=call->parent_loop.lock();
    require(loop&&loop->creator==call->creator&&loop->alive&&loop->sealed&&
        loop->stage==NormalCommonTraversal::Frame::Stage::child18&&loop->cursor==call->ordinal&&
        loop->count==call->count&&loop->depth==call->depth&&same(loop->parent,call->parent)&&
        same(loop->children,call->children)&&traversal->stack.size()==call->depth+1&&
        same(traversal->stack.back(),loop),
        "Common child18 is not at the real active parent-loop cursor");
    if(call->depth==0){
        const auto common=prefix->common_root;
        require(common&&same(loop->root_common,common)&&same(common->root,call->parent)&&
            same(common->children,call->children)&&common->count==call->count&&
            common->cursor==call->ordinal&&!common->whole_return&&!common->continuing&&
            common->stage==detail::BnwNormalGuiBatchPrefix::Impl::CommonRoot::Stage::child18_required,
            "Common child18 Root cursor does not match actual56F725");
    }else{
        require(!loop->root_common,"Recursive child18 cannot alias the Root Common cursor");
        for(std::size_t d=1;d<=call->depth;++d){
            const auto active_frame=traversal->stack[d];
            require(active_frame&&active_frame->depth==d,"Recursive child18 original ancestor depth changed");
            NormalCommonTraversal::require_recursive_frame_without_callbacks(*active_frame,root);
            const auto suspended=active_frame->original_parent_call.lock();
            const auto previous=traversal->stack[d-1];
            require(suspended&&suspended->alive.load()&&suspended->phase==detail::BnwNormalCommonChildCall::Phase::common56f630&&
                !suspended->common_return&&same(suspended->traversal.lock(),traversal)&&
                suspended->depth+1==d&&same(previous,suspended->parent_loop.lock())&&
                previous->alive&&previous->sealed&&previous->stage==NormalCommonTraversal::Frame::Stage::child18&&
                previous->cursor==suspended->ordinal&&previous->count==suspended->count,
                "Recursive child18 lacks SAME actually suspended ancestor continuation");
        }
    }
    require(call->children&&call->children->impl_&&call->parent&&call->receiver&&call->receiver->impl_&&
        call->ordinal<call->count&&call->children->impl_->ordered.size()==call->count&&
        same(call->children->impl_->ordered[call->ordinal],call->receiver)&&
        same(call->children->impl_->parent.lock(),call->parent),
        "Common child18 ordered78/parent/current cursor association changed");
    const auto receiver=call->receiver->impl_;const auto owner=receiver->core.lock();
    const auto occurrence=receiver->occurrence.lock();
    const auto control_owner=call->control.owner_.lock();
    require(owner&&control_owner&&owner.get()==actual.get()&&control_owner.get()==actual.get()&&
        !owner.owner_before(actual)&&!actual.owner_before(owner)&&
        !control_owner.owner_before(actual)&&!actual.owner_before(control_owner)&&
        receiver->creator==call->creator&&receiver->id&&receiver->id==call->control.id_&&
        same(receiver->canonical.lock(),call->receiver)&&occurrence&&occurrence->id==receiver->id&&
        occurrence->constructor_serial==receiver->serial,
        "Common child18 SAME actual Control/receiver allocation changed");
    const auto& node=Core::node(actual->nodes,receiver->id);
    require(node.alive&&!node.dynamic&&node.layout_baseline&&same(node.layout_receiver_occurrence,occurrence),
        "Common child18 canonical physical Base/occurrence absent");
    const auto& baseline=*node.layout_baseline;
    require(baseline.sequence_==call->base_sequence&&same(baseline.base_.receiver_owned,call->receiver)&&
        same(baseline.base_.parent_38.lock(),call->parent)&&baseline.base_.flags_40==call->physical_flags&&
        !(call->physical_flags&0x2002u),"Common child18 native40/paired38/canonical Base changed");
    require(call->kind==detail::BnwNormalCommonChildCall::Kind::standalone_label||
        call->kind==detail::BnwNormalCommonChildCall::Kind::button,"Common child18 actual subtype capability invalid");
    require(call->phase==detail::BnwNormalCommonChildCall::Phase::child18||
        call->phase==detail::BnwNormalCommonChildCall::Phase::button_own118||
        call->phase==detail::BnwNormalCommonChildCall::Phase::common56f630||
        call->phase==detail::BnwNormalCommonChildCall::Phase::button_caption178,
        "Common child18 phase is not an actual pending native call");
    if(call->kind==detail::BnwNormalCommonChildCall::Kind::standalone_label){
        require(call->phase==detail::BnwNormalCommonChildCall::Phase::child18,
            "Standalone Label cannot acquire Button continuation phases");
        const auto label=receiver->standalone_label.lock();
        require(label&&same(baseline.standalone_label_,label)&&same(call->original_label,label)&&
            !call->original_button&&receiver->button.expired(),
            "Common child18 actual standalone Label allocation changed");
        const auto fields=label->impl_;fields->first();
        const auto word=std::get_if<std::uint32_t>(&fields->fields[0x40/4]);
        require(word&&*word==call->physical_flags,"Common Label physical40 differs from Common Base");
    }else{
        const auto button=receiver->button.lock();
        require(button&&same(baseline.button_,button)&&same(call->original_button,button)&&
            !call->original_label&&receiver->standalone_label.expired()&&
            receiver->button_parser_progress==detail::NotificationLayoutOrdinaryReceiver::Impl::ButtonParserProgress::common_returned&&
            same(receiver->button_id58_return.lock(),baseline.base_.id_58),
            "Common child18 actual Button constructor/Common/id58 return changed");
        require_button_native_common_return_owned(actual,receiver->id,fresh,true);
    }
    root.require_physical_current_without_callbacks();execution->first();
    require(call->alive.load()&&traversal->alive.load()&&same(traversal->current.lock(),call)&&
        actual->source->current_operation_loan().operation_serial()==call->serial,
        "Common child18 original owner/current phase changed at PURE POST");
}

void BnwWorldViewControlTree::require_normal_common_child_call_current_without_callbacks(
    const detail::BnwNormalCommonChildCall& offered){
    NormalCommonTraversal::require_current_call_impl_without_callbacks(offered.impl_);
}

void BnwWorldViewControlTree::require_normal_common_child_call_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwNormalCommonChildCall& call){
    require_normal_common_child_call_current_without_callbacks(call);
    const auto traversal=call.impl_->traversal.lock();const auto execution=traversal->execution.lock();
    execution->full(operation);
    require_normal_common_child_call_current_without_callbacks(call);
}

detail::BnwNormalCommonChildCall::BnwNormalCommonChildCall(std::shared_ptr<const Impl> actual,const IssuerKey&)
    :impl_(std::move(actual)){}
void detail::BnwNormalCommonChildCall::require_current_without_callbacks()const{
    BnwWorldViewControlTree::require_normal_common_child_call_current_without_callbacks(*this);
}
void detail::BnwNormalCommonChildCall::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& op)const{
    BnwWorldViewControlTree::require_normal_common_child_call_source_owned(op,*this);
}
BnwWorldViewControlTree::Control detail::BnwNormalCommonChildCall::actual_control_without_callbacks()const{
    require_current_without_callbacks();const auto control=impl_->control;require_current_without_callbacks();return control;
}
detail::BnwNormalRootContextEntry detail::BnwNormalCommonChildCall::captured_root_context_entry_without_callbacks()const{
    require_current_without_callbacks();const auto entry=impl_->traversal.lock()->entry;
    require_current_without_callbacks();return entry;
}
detail::BnwNormalRootBatchHistory::Guard detail::BnwNormalCommonChildCall::captured_root_without_callbacks()const{
    require_current_without_callbacks();const auto root=impl_->traversal.lock()->execution.lock()->root;
    require_current_without_callbacks();return root;
}
std::size_t detail::BnwNormalCommonChildCall::actual_ordered_ordinal_without_callbacks()const{
    require_current_without_callbacks();const auto ordinal=impl_->ordinal;require_current_without_callbacks();return ordinal;
}
std::size_t detail::BnwNormalCommonChildCall::actual_common_depth_without_callbacks()const{
    require_current_without_callbacks();const auto depth=impl_->depth;require_current_without_callbacks();return depth;
}
detail::BnwNormalCommonChildCall::Kind detail::BnwNormalCommonChildCall::actual_kind_without_callbacks()const{
    require_current_without_callbacks();const auto kind=impl_->kind;require_current_without_callbacks();return kind;
}
detail::BnwNormalCommonChildCall::Phase detail::BnwNormalCommonChildCall::actual_phase_without_callbacks()const{
    require_current_without_callbacks();const auto phase=impl_->phase;require_current_without_callbacks();return phase;
}
