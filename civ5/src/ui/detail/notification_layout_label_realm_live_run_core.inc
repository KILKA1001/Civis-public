// Distinct post-DTOR read owner. Existing parser/init Impl::first still
// requires busy=true; this never revives Runtime/Argument/Constructor loans.
void detail::NotificationLayoutContextRealmRef::require_live_run_origin_without_callbacks(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto saved=impl_;
    require(saved->creator==std::this_thread::get_id()&&saved->alive.load()&&!saved->failed.load(),
        "Live Run Context identity retired/foreign/failed");
    const auto actual=saved->context.lock();const auto original_source=saved->source.lock();
    require(actual&&original_source,"Live Run actual Context/Source retired");actual->layout_origin();
    const auto& current=actual->source->current_operation_loan();actual->layout_origin();
    require(&current==&operation,"Live Run Context requires SAME fresh actual operation");
    operation.require_origin_without_callbacks();actual->layout_origin();
    require(saved->constructor&&actual->context_construction_return.get()==saved->constructor.get()&&
        !saved->constructor.owner_before(actual->context_construction_return)&&
        !actual->context_construction_return.owner_before(saved->constructor)&&
        actual->context_realm_identity.get()==this&&
        actual->source.get()==original_source.get()&&!actual->source.owner_before(original_source)&&
        !original_source.owner_before(actual->source),
        "Live Run Context actual constructor/identity/Source changed");
    actual->layout_origin();operation.require_origin_without_callbacks();actual->layout_origin();
}
