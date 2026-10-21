// Inside civ5::ui, after the actual Context/ordinary storage and OWN40F map588.
// Original5803E0 reads the live numeric receiver map; it is not an XML-name scan.
void BnwWorldViewControlTree::require_lookup_context_current_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<detail::NotificationLayoutContextOrdinaryState>& context) {
    operation.require_origin_without_callbacks();
    require(context&&context->impl_,"LookUpControl actual Context receiver absent");
    const auto state=context->impl_;const auto actual=state->core.lock();const auto source=state->source.lock();
    require(state->creator==std::this_thread::get_id()&&state->alive.load()&&!state->failed.load()&&
        actual&&source&&actual->source==source&&actual->context_ordinary_state==context,
        "LookUpControl actual Context/source/allocation retired");
    actual->layout_origin();
    require(&source->current_operation_loan()==&operation,
        "LookUpControl actual current Source operation differs");
    const auto constructor=state->native_constructor_return;
    require(constructor&&constructor->impl_&&constructor->impl_->creator==state->creator&&
        constructor->impl_->alive.load()&&!constructor->impl_->failed.load()&&
        constructor->impl_->core.lock()==actual&&constructor->impl_->source.lock()==source&&
        constructor->impl_->projection.lock()==context&&constructor->impl_->port&&
        constructor->impl_->port->map588,"LookUpControl actual completed map588 absent");
    constructor->impl_->port->require_completed();
    operation.require_current();actual->layout_origin();
    require(actual->context_ordinary_state==context&&context->impl_==state&&
        state->native_constructor_return==constructor&&state->alive.load()&&!state->failed.load()&&
        &source->current_operation_loan()==&operation,
        "LookUpControl Context/map588 replaced during current Source validation");
}

std::shared_ptr<const void> BnwWorldViewControlTree::lookup_receiver_physical_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver) {
    operation.require_origin_without_callbacks();
    require(receiver&&receiver->impl_,"LookUpControl nonNULL native receiver has no physical issuer");
    const auto r=receiver->impl_;const auto actual=r->core.lock();
    require(r->creator==std::this_thread::get_id()&&actual&&r->canonical.lock()==receiver,
        "LookUpControl nonNULL receiver/canonical storage retired");
    actual->layout_origin();require(&actual->source->current_operation_loan()==&operation,
        "LookUpControl receiver belongs to another current Source");
    const std::weak_ptr<detail::NotificationLayoutContextOrdinaryState> empty_context;
    const bool is_context=r->context.owner_before(empty_context)||empty_context.owner_before(r->context);
    if(is_context){
        const auto context=r->context.lock();
        require(context!=nullptr,"LookUpControl nonNULL Context receiver retired; it is not native NULL");
        require_lookup_context_current_owned(operation,context);return context;
    }
    // The existing closed constructors own exactly one physical subtype.
    // An expired subtype never turns a nonNULL map cell into a NULL result.
    std::shared_ptr<const void> result;std::size_t count=0;
    const auto keep=[&](const auto& weak){if(const auto value=weak.lock()){result=value;++count;}};
    keep(r->image);keep(r->grid);keep(r->box);keep(r->standalone_label);keep(r->button);keep(r->stack);
    require(count==1&&result,"LookUpControl nonNULL ordinary physical receiver retired/ambiguous");
    const auto& node=Core::node(actual->nodes,r->id);
    require(node.alive&&r->occurrence.lock()&&node.layout_baseline,
        "LookUpControl nonNULL ordinary receiver has no current canonical node");
    actual->layout_origin();return result;
}

std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>
BnwWorldViewControlTree::lookup_context_relative_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<detail::NotificationLayoutContextOrdinaryState>& context,const char* path) {
    require_lookup_context_current_owned(operation,context);
    if(!path)return {}; // Actual5803F1 NULL input returns native NULL.
    require(path[0]!='/',"LookUpControl absolute562D60 requires actual UIManager ordered roots9700 supplier");
    // Native strcspn -> min(length,255) -> memcpy -> zero terminator. For an
    // overlong segment the remainder starts at byte255, before its slash.
    const auto length=std::min<std::size_t>(std::strcspn(path,"/"),255);
    if(!length)return {}; // Empty segment never hashes or mutates map588.
    std::array<char,256> segment{};std::memcpy(segment.data(),path,length);
    const auto key=content::saved_info_type_hash(std::string_view(segment.data(),length));
    auto& map=*context->impl_->native_constructor_return->impl_->port->map588;
    auto at=map.cells.find(key);
    if(at==map.cells.end()){
        // Original59EBD0 is operator[]: a miss creates an actual NULL cell.
        require(map.revision!=UINT64_MAX,"LookUpControl map588 revision exhausted");
        at=map.cells.emplace(key,std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>{}).first;
        ++map.revision;
    }
    const auto receiver=at->second;if(!receiver)return {};
    (void)lookup_receiver_physical_owned(operation,receiver);
    const char* remaining=path+length;remaining+=std::strspn(remaining,"/");
    if(*remaining){
        const auto child=receiver->impl_->context.lock();
        if(child)return lookup_context_relative_owned(operation,child,remaining); // Actual virtual type8 only.
    }
    // A real ordinary receiver is returned even with unconsumed path suffix.
    return receiver;
}
