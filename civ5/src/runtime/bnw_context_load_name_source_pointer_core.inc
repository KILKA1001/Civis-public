// Inside civ5::runtime AFTER the actual Source State definition. No callback
// or raw Loan method occurs before the physical active-pointer comparison.
std::uint64_t BnwSelectedWorldViewContextOwner::capture_active_operation_scope_serial_without_callbacks(
    const BnwWorldViewSourceOperationLoan* expected)const{
    const auto saved=state_;
    require(saved->creator==std::this_thread::get_id()&&saved->alive.load()&&!saved->failed.load(),
        "ContextB4 Source pointer capture retired/foreign/failed FIRST");
    const auto canonical=saved->canonical_owner.lock();
    require(canonical&&canonical.get()==this&&canonical->state_.get()==saved.get()&&
        !canonical->state_.owner_before(saved)&&!saved.owner_before(canonical->state_),
        "ContextB4 Source pointer capture substituted canonical allocation/control");
    require(expected&&saved->active==expected&&saved->scope_serial,
        "ContextB4 Source pointer capture active frame absent/substituted");
    return saved->scope_serial; //Closed captured DATA, no VM/GUI/RootRead grant.
}
