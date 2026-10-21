// Persistent attachment DATA, separate from the expired Read/Controls/Store
// lexical scopes. Pending wholeRead slot cannot grant parent initialization.
struct detail::NotificationContextRootChildAttachReturn::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    mutable std::atomic<bool> alive{true};
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::weak_ptr<NotificationLayoutContextOrdinaryState> context;
    const std::weak_ptr<const NotificationLayoutOrdinaryReceiver> root,child;
    const std::weak_ptr<NotificationLayoutOrdinaryChildren78> children;
    const std::weak_ptr<const NotificationLayoutContextRealmRef> realm;
    const NotificationContextRootReadCompletionSlot::Guard completed;
    enum class ReceiverKind {ordinary_control,distinct_context};
    const ReceiverKind receiver_kind;
    const std::optional<std::uint64_t> child_id;
    const std::weak_ptr<BnwWorldViewControlTree::Core> distinct_context_child;
    const std::weak_ptr<NotificationLayoutContextOrdinaryState> distinct_context_state;
    const std::size_t append_index;
    Impl(std::shared_ptr<BnwWorldViewControlTree::Core> c,
        std::shared_ptr<NotificationLayoutContextOrdinaryState> parent,
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> r,
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> actual_child,
        std::shared_ptr<NotificationLayoutOrdinaryChildren78> array,
        std::shared_ptr<const NotificationLayoutContextRealmRef> context_realm,
        NotificationContextRootReadCompletionSlot::Guard completion,std::uint64_t id,std::size_t index)
        :core(std::move(c)),context(std::move(parent)),root(std::move(r)),child(std::move(actual_child)),
         children(std::move(array)),realm(std::move(context_realm)),completed(std::move(completion)),
         receiver_kind(ReceiverKind::ordinary_control),child_id(id),append_index(index){}
    Impl(std::shared_ptr<BnwWorldViewControlTree::Core> c,
        std::shared_ptr<NotificationLayoutContextOrdinaryState> parent,
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> r,
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> actual_child,
        std::shared_ptr<NotificationLayoutOrdinaryChildren78> array,
        std::shared_ptr<const NotificationLayoutContextRealmRef> context_realm,
        NotificationContextRootReadCompletionSlot::Guard completion,std::size_t index,
        std::shared_ptr<BnwWorldViewControlTree::Core> child_context,
        std::shared_ptr<NotificationLayoutContextOrdinaryState> actual_state)
        :core(std::move(c)),context(std::move(parent)),root(std::move(r)),child(std::move(actual_child)),
         children(std::move(array)),realm(std::move(context_realm)),completed(std::move(completion)),
         receiver_kind(ReceiverKind::distinct_context),child_id(std::nullopt),
         distinct_context_child(std::move(child_context)),distinct_context_state(std::move(actual_state)),append_index(index){}
};
detail::NotificationContextRootChildAttachReturn::NotificationContextRootChildAttachReturn(std::shared_ptr<const Impl> data)
    :impl_(std::move(data)){}
detail::NotificationContextRootChildAttachReturn::~NotificationContextRootChildAttachReturn(){impl_->alive.store(false);}
detail::NotificationContextRootChildAttachReturn::Guard::Guard(std::shared_ptr<const Impl> data):impl_(std::move(data)){}
void detail::NotificationContextRootChildAttachReturn::Guard::require_current_without_callbacks()const{
    const auto saved=impl_;require(saved&&saved->creator==std::this_thread::get_id()&&saved->alive.load(),
        "RootAttach initializer borrower retired/foreign FIRST");
    // No expired Init/Binding/RootRead loan retained or renewed here.
    saved->completed.require_completed_current_without_callbacks();
    BnwWorldViewControlTree::require_context_root_attachment_physical_without_callbacks(saved);
}
void detail::NotificationContextRootChildAttachReturn::Guard::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto saved=impl_;require_current_without_callbacks();
    saved->completed.require_source_owned(operation);require_current_without_callbacks();
}
void detail::NotificationContextRootChildAttachReturn::Guard::require_child_receiver_owned(
    const NotificationLayoutOrdinaryReceiver& actual_child)const{
    const auto saved=impl_;require_current_without_callbacks();const auto child=saved->child.lock();
    require(child&&child.get()==&actual_child,"RootAttach initializer substituted genuine child control");
    require_current_without_callbacks();
}
detail::NotificationContextRootChildAttachReturn::Guard
detail::NotificationContextRootChildAttachReturn::capture_current_initializer_parent_guard_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const NotificationLayoutOrdinaryReceiver& child)const{
    const auto saved=impl_;Guard result(saved);result.require_child_receiver_owned(child);
    result.require_source_owned(operation);result.require_child_receiver_owned(child);return result;
}
std::shared_ptr<const detail::NotificationContextRootChildAttachReturn>
detail::NotificationContextRootChildAttachReturn::require_actual_current_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const NotificationLayoutOrdinaryReceiver& actual_child){
    return BnwWorldViewControlTree::require_actual_context_root_attachment_owned(operation,actual_child);
}

void BnwWorldViewControlTree::require_context_root_attachment_physical_without_callbacks(
    const std::shared_ptr<const detail::NotificationContextRootChildAttachReturn::Impl>& saved){
    require(saved&&saved->creator==std::this_thread::get_id()&&saved->alive.load(),"RootAttach physical receipt FIRST");
    const auto actual=saved->core.lock();const auto context=saved->context.lock();
    const auto root=saved->root.lock(),child=saved->child.lock();const auto array=saved->children.lock();
    const auto realm=saved->realm.lock();
    require(actual&&context&&root&&child&&array&&realm,"RootAttach physical allocation/78/realm retired");
    actual->layout_origin(true); // entered actual initializer has its own sealed Binding
    require(context->impl_&&context->impl_->creator==std::this_thread::get_id()&&
        context->impl_->alive.load()&&!context->impl_->failed.load()&&
        context->impl_->native_constructor_return&&context->impl_->native_constructor_return->impl_&&
        context->impl_->native_constructor_return->impl_->alive.load()&&
        actual->context_ordinary_state==context&&actual->context_layout_baseline&&
        actual->context_layout_baseline->base_.receiver_owned==root&&
        actual->context_layout_baseline->base_.children_78==array&&array->impl_&&array->impl_->parent.lock()==root&&
        saved->append_index<array->impl_->ordered.size()&&array->impl_->ordered[saved->append_index]==child&&
        actual->context_realm_identity==realm,"RootAttach SAME real Context/78/3C origin changed");
    saved->completed.require_actual_root_receiver_without_callbacks(*root);
    if(saved->receiver_kind==detail::NotificationContextRootChildAttachReturn::Impl::ReceiverKind::distinct_context){
        require_context_distinct_attachment_physical_without_callbacks(saved);return;
    }
    require(saved->child_id.has_value(),"RootAttach ordinary child numeric domain absent");
    const auto& node=Core::node(actual->nodes,*saved->child_id);
    require(node.alive&&!node.dynamic&&node.layout_baseline&&node.layout_receiver_occurrence&&
        node.root_context_attach_return&&node.root_context_attach_return->impl_==saved&&
        node.layout_baseline->base_.receiver_owned==child&&node.layout_baseline->base_.parent_38.lock()==root&&
        node.layout_baseline->base_.context_realm_3c==realm&&child->impl_&&
        child->impl_->creator==std::this_thread::get_id()&&child->impl_->canonical.lock()==child&&
        child->impl_->occurrence.lock()==node.layout_receiver_occurrence&&child->impl_->core.lock()==actual,
        "RootAttach child current actual38/3C/78/member receipt changed");
    // Child initializer legitimately writes C4/CC/170/style/geometry/sequence.
    // None of those future fields are frozen by parent attachment DATA.
}
std::shared_ptr<const detail::NotificationContextRootChildAttachReturn>
BnwWorldViewControlTree::require_actual_context_root_attachment_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::NotificationLayoutOrdinaryReceiver& actual_child){
    const auto data=actual_child.impl_;
    require(data&&data->creator==std::this_thread::get_id(),"RootAttach lookup actual child FIRST");
    const auto actual=data->core.lock();const auto child=data->canonical.lock();const auto occurrence=data->occurrence.lock();
    require(actual&&child&&child.get()==&actual_child&&occurrence,"RootAttach lookup actual control retired");
    actual->layout_origin(true);require(&actual->source->current_operation_loan()==&operation,
        "RootAttach lookup canonical Source replaced before raw operand");
    if(data->context.lock()&&actual->context_layout_baseline&&actual->context_layout_baseline->base_.receiver_owned==child){
        require(actual->context_root_distinct_attach_return!=nullptr,"distinct Context initializer missing genuine parent673730 DATA");
        const auto result=actual->context_root_distinct_attach_return;
        auto guard=result->capture_current_initializer_parent_guard_owned(operation,actual_child);
        guard.require_child_receiver_owned(actual_child);return result;
    }
    const auto& node=Core::node(actual->nodes,data->id);
    require(node.alive&&node.layout_receiver_occurrence==occurrence&&node.layout_baseline&&
        node.layout_baseline->base_.receiver_owned==child&&node.root_context_attach_return,
        "RootAttach missing genuine673730 return; common/tag/ID cannot convert");
    const auto result=node.root_context_attach_return;
    auto guard=result->capture_current_initializer_parent_guard_owned(operation,actual_child);
    guard.require_child_receiver_owned(actual_child);return result;
}
std::shared_ptr<const detail::NotificationContextRootChildAttachReturn>
BnwWorldViewControlTree::publish_context_root_child_attach_return_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const std::shared_ptr<ContextRootAttachEvidence>& evidence){
    using Return=detail::NotificationContextRootChildAttachReturn;
    evidence->full();require(evidence->operation==&operation&&evidence->stage==ContextRootAttachEvidence::Stage::returned,
        "RootAttach DATA publisher before true38/3C/78/67373E completion");
    const auto actual=evidence->core.lock();const auto context=evidence->context.lock();
    require(actual&&actual==core_,"RootAttach publication requires SAME owning Tree Core");
    const auto root=evidence->root,child=evidence->child;const auto array=evidence->ordered;const auto realm=evidence->context_this;
    const auto child_id=evidence->child_id;const auto index=evidence->append_index;
    const auto completion=evidence->completion->capture_pending_data_guard_without_callbacks();
    evidence->full();require(!Core::node(actual->nodes,child_id).root_context_attach_return,"RootAttach replayed publication");
    auto data=std::make_shared<const Return::Impl>(actual,context,root,child,array,realm,completion,child_id,index);evidence->full();
    auto result=std::shared_ptr<const Return>(new Return(data));evidence->full();
    require(!Core::node(actual->nodes,child_id).root_context_attach_return,"RootAttach publication callback replaced node");
    Core::mutable_node_owned(actual->nodes,child_id).root_context_attach_return=result;evidence->full();
    require(Core::node(actual->nodes,child_id).root_context_attach_return==result,
        "RootAttach exact pending DATA publication changed");
    return result; // PendingRead slot is NOT yet an initializer completion
}

void BnwWorldViewControlTree::store_context_root_attach_3c_for_actual_receiver_owned(
    const detail::NotificationContextRootChildAttachStoreLoan& store,
    const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::NotificationLayoutOrdinaryReceiver& receiver){
    using Base=detail::NotificationLayoutBaseline;
    const auto guard=store.capture_guard_without_callbacks(operation,receiver);
    guard.require_current_without_callbacks(operation,receiver);const auto value=guard.value_3c_without_callbacks();
    const auto actual=core_;const auto data=receiver.impl_;
    require(data&&data->core.lock()==actual&&data->canonical.lock().get()==&receiver,
        "RootAttach3C actual registered receiver changed");
    const auto id=data->id;const auto& node=Core::node(actual->nodes,id);
    require(node.layout_baseline.has_value(),"RootAttach3C actual Base missing");
    if(node.layout_baseline->kind_==Base::Kind::label){
        store_standalone_root_attached_context_3c_owned(store,operation,receiver);return;
    }
    const bool button_kind=node.layout_baseline->kind_==Base::Kind::button;
    if(button_kind){
        // Original44BF52 and67373E are direct child Base3C writes. Their
        // genuine selected Button allocation was already earned at529520/v44.
        // This verifies retained typed constructor/Common DATA, without
        // renewing the old parser's child-context relation during reparenting.
        const auto button=node.layout_baseline->button_;
        const auto returned=node.layout_baseline->base_.common_return_owned;
        const auto fields=receiver.impl_;
        const auto common=returned?returned->impl_:nullptr;
        require(node.layout_baseline->actual_type_=="Button"&&button&&fields&&common&&
            node.root_ordinary_controls_return&&node.layout_receiver_occurrence&&node.full_xml_occurrence&&
            fields->creator==std::this_thread::get_id()&&fields->button.lock()==button&&
            fields->canonical.lock().get()==&receiver&&fields->core.lock()==actual&&
            fields->occurrence.lock()==node.layout_receiver_occurrence&&fields->id==id&&
            fields->serial==operation.operation_serial()&&
            fields->button_parser_progress==detail::NotificationLayoutOrdinaryReceiver::Impl::ButtonParserProgress::common_returned&&
            fields->button_void_current_matches_owned()&&
            fields->button_void_missing_read_mask==3u&&common->creator==fields->creator&&
            common->core.lock()==actual&&common->receiver.lock().get()==&receiver&&
            common->occurrence.lock()==node.layout_receiver_occurrence&&common->selected.lock()==node.full_xml_occurrence&&
            common->children.lock()==node.layout_baseline->base_.children_78&&common->serial==fields->serial&&
            (node.layout_baseline->base_.flags_40&8u),
            "RootAttach3C requires actual earned Button allocation/constructor/Common return and tagged v64");
    }
    require(node.layout_baseline->kind_==Base::Kind::image||node.layout_baseline->kind_==Base::Kind::box||
        node.layout_baseline->kind_==Base::Kind::grid||button_kind,
        "RootAttach3C concrete subtype physical Base setter required; no generic tag success");
    const auto old=node.layout_baseline->base_.context_realm_3c;const auto old_sequence=node.layout_baseline->sequence_;
    const auto old_history_size=node.layout_baseline->history_.size();
    const auto*const old_address=&*node.layout_baseline;
    guard.require_source_owned(operation,receiver);
    auto& fresh=Core::node(actual->nodes,id);require(fresh.layout_baseline&& &*fresh.layout_baseline==old_address&&
        fresh.layout_baseline->base_.receiver_owned.get()==&receiver&&fresh.layout_baseline->base_.context_realm_3c==old&&
        fresh.layout_baseline->sequence_==old_sequence,"RootAttach3C reserve physical preimage changed");
    require(old_sequence!=std::numeric_limits<std::uint64_t>::max(),"RootAttach3C history exhausted");
    // Allocate a LOCAL empty history buffer. Do not reserve/copy from a raw
    // mutable Node vector while new_handler may retire/reallocate that Node.
    std::vector<Base::Entry> staged_history;
    require(old_history_size<staged_history.max_size(),"RootAttach3C history capacity exhausted");
    staged_history.reserve(old_history_size+1);
    guard.require_source_owned(operation,receiver);
    auto& after=Core::mutable_node_owned(actual->nodes,id);require(after.layout_baseline&& &*after.layout_baseline==old_address&&
        after.layout_baseline->base_.receiver_owned.get()==&receiver&&after.layout_baseline->base_.context_realm_3c==old&&
        after.layout_baseline->sequence_==old_sequence&&after.layout_baseline->history_.size()==old_history_size,
        "RootAttach3C allocator changed target before store");
    // Entry is owned scalar DATA; capacity is already allocated, so this copy
    // and swap invoke no allocator/source callback and no stale source read.
    for(const auto& entry:after.layout_baseline->history_)staged_history.push_back(entry);
    after.layout_baseline->history_.swap(staged_history);
    after.layout_baseline->record_owned(Base::Mutation::context_realm_3c);
    after.layout_baseline->base_.context_realm_3c=value;
    after.layout_baseline->base_.context_3c_observation=value?Base::Base::Context3CObservation::parsed_realm:
        Base::Base::Context3CObservation::root_inherited_null;
    guard.require_source_owned(operation,receiver);
    const auto& final=Core::node(actual->nodes,id);require(final.layout_baseline&& &*final.layout_baseline==old_address&&
        final.layout_baseline->base_.context_realm_3c==value&&final.layout_baseline->sequence_==old_sequence+1,
        "RootAttach3C exact paired physical store changed");
}
