// Distinct sealed standalone registration on the SAME actual module counters.
// Existing embedded/FIRST methods remain byte-exact; typed pointer families cannot cross.
std::shared_ptr<BnwLabelProcessModuleOwner::LabelAllocationAttempt>
BnwLabelProcessModuleOwner::begin_standalone_label_fresh_allocation_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    const auto saved=impl_;saved->first();operation.require_origin_without_callbacks();saved->first();
    const auto actual=operation.label_module_owned();saved->first();
    const auto self=saved->self.lock();
    if(!actual||!self||actual.get()!=self.get()||actual.owner_before(self)||self.owner_before(actual)||
       saved->label_allocating){saved->failed.store(true);throw std::runtime_error("Label fresh allocation foreign/reentered module");}
    saved->label_allocating=true;
    try{
        // Native fresh branch70313C/703142, BEFORE allocation. The common
        // delta counts actual OWN allocations only, never Original global total.
        ++saved->owned_control_allocation_delta;++saved->label_fresh_total;
        auto ticket=std::shared_ptr<LabelAllocationAttempt>(new LabelAllocationAttempt(saved,operation.operation_serial(),LabelAllocationAttempt::Role::standalone));
        saved->first();operation.require_origin_without_callbacks();saved->first();
        saved->label_attempts.push_back(ticket);
        saved->first();operation.require_origin_without_callbacks();saved->first();
        if(ticket->ordinal!=saved->label_attempts.size()||saved->label_attempts.back()!=ticket)
            throw std::runtime_error("Label allocation attempt superseded before physical child allocation");
        return ticket;
    }catch(...){saved->label_allocating=false;saved->failed.store(true);throw;}
}
void BnwLabelProcessModuleOwner::finish_label_fresh_allocation_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<LabelAllocationAttempt>& ticket,
    const std::shared_ptr<NotificationLayoutStandaloneLabelState>& child){
    const auto saved=impl_;saved->first();operation.require_origin_without_callbacks();saved->first();
    const auto owner=ticket?ticket->module.lock():nullptr;
    if(!ticket||ticket->role!=LabelAllocationAttempt::Role::standalone||!ticket->actual_child.expired()||ticket->creator!=std::this_thread::get_id()||!owner||owner.get()!=saved.get()||
       owner.owner_before(saved)||saved.owner_before(owner)||!saved->label_allocating||
       ticket->stage!=LabelAllocationAttempt::Stage::allocation_entered||
       ticket->source_serial!=operation.operation_serial()||saved->label_attempts.empty()||
       saved->label_attempts.back()!=ticket||ticket->ordinal!=saved->label_attempts.size())
        throw std::runtime_error("Label physical allocation return has no actual module attempt");
    ++saved->label_active_attempts; //703157 AFTER allocator returns, BEFORE NULL test
    if(!child){ticket->stage=LabelAllocationAttempt::Stage::failed;saved->label_allocating=false;
        saved->failed.store(true);throw std::runtime_error("Actual Label backend allocation returned NULL");}
    ticket->actual_standalone_child=child;ticket->stage=LabelAllocationAttempt::Stage::allocation_returned;
    require_label_allocation_owned(operation,ticket,child,false);
}
void BnwLabelProcessModuleOwner::require_label_allocation_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<LabelAllocationAttempt>& ticket,
    const std::shared_ptr<NotificationLayoutStandaloneLabelState>& child,bool returned)const{
    const auto saved=impl_;saved->first();operation.require_origin_without_callbacks();saved->first();
    const auto owner=ticket?ticket->module.lock():nullptr;
    const auto object=ticket?ticket->actual_standalone_child.lock():nullptr;
    const auto actual=operation.label_module_owned();saved->first();
    const auto self=saved->self.lock();
    if(!ticket||ticket->role!=LabelAllocationAttempt::Role::standalone||!ticket->actual_child.expired()||ticket->creator!=std::this_thread::get_id()||!owner||owner.get()!=saved.get()||
       owner.owner_before(saved)||saved.owner_before(owner)||ticket->source_serial!=operation.operation_serial()||
       !ticket->ordinal||ticket->ordinal>saved->label_attempts.size()||saved->label_attempts[ticket->ordinal-1]!=ticket||
       !object||!child||object.get()!=child.get()||object.owner_before(child)||child.owner_before(object)||
       !actual||!self||actual.get()!=self.get()||actual.owner_before(self)||self.owner_before(actual)||
       ticket->stage!=(returned?LabelAllocationAttempt::Stage::constructor_returned:LabelAllocationAttempt::Stage::allocation_returned))
        throw std::runtime_error("Label allocation/constructor return identity retired or changed");
    saved->first();operation.require_origin_without_callbacks();saved->first();
}
void BnwLabelProcessModuleOwner::finish_label_constructor_return_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<LabelAllocationAttempt>& ticket,
    const std::shared_ptr<NotificationLayoutStandaloneLabelState>& child){
    require_label_allocation_owned(operation,ticket,child,false);
    const auto saved=impl_;saved->first();
    if(!saved->label_allocating)throw std::runtime_error("Label constructor return without actual active allocation");
    ticket->stage=LabelAllocationAttempt::Stage::constructor_returned;saved->label_allocating=false;
    require_label_allocation_owned(operation,ticket,child,true);
}
