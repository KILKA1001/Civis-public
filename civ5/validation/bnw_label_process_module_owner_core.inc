// Included inside civ5::ui by the ONE new runtime TU after actual UiH completes
// the real renderer alias. No UiRenderer out-of-line calls or GL calls are
// emitted. Root supplies literal friends/seams; no lifetime type is redefined.
namespace detail {
struct BnwLabelProcessModuleOwner::Impl {
    using Life=UiScreenGlRenderer::WorldViewPaintLifetime;
    const std::thread::id creator=std::this_thread::get_id();
    const UiScreenGlRenderer* const renderer;
    const std::weak_ptr<Life> renderer_life;
    std::weak_ptr<BnwLabelProcessModuleOwner> self;
    std::atomic<bool> alive{true},failed{false};
    bool busy=false;
    std::unique_ptr<std::uint8_t[]> bytes;
    // Original loader .data virtual-tail12zero proof plus actual private
    // fresh module creation. These are not caller-supplied live values.
    std::uint32_t count=0,capacity=0;
    // Native image fields start at zero. OWN fresh allocation uses real x64
    // storage, never unearned pool recycling or Original diagnostic total.
    std::uint32_t label_fresh_total=0,label_active_attempts=0,owned_control_allocation_delta=0;
    bool label_allocating=false;
    std::uint32_t owned_label_reclamations=0; //qualified fresh OWN backend only
    std::vector<std::shared_ptr<LabelAllocationAttempt>> label_attempts;
    std::uint64_t sequence=0;
    enum class Effect {image_zero_fields,reserve_begin,allocation_returned,
        pointer_published,space_written,count_incremented,count_nonzero_skip};
    struct Entry {std::uint64_t sequence,source_serial;Effect effect;};
    std::vector<Entry> history;
    Impl(UiScreenGlRenderer& actual,const std::shared_ptr<Life>& life)
        :renderer(&actual),renderer_life(life){record(0,Effect::image_zero_fields);}
    void first()const {
        if(creator!=std::this_thread::get_id())
            throw std::runtime_error("Label module foreign creator");
        if(!alive.load()||failed.load())
            throw std::runtime_error("Label module retired/failed");
        const auto life=renderer_life.lock();
        if(!life||life->creator!=creator||!life->alive.load()||life->failed.load()||
           life->renderer!=renderer)
            throw std::runtime_error("Label module actual stable renderer origin changed");
    }
    void record(std::uint64_t serial,Effect effect){
        if(sequence==UINT64_MAX)throw std::runtime_error("Label module prefix history exhausted");
        // This allocation may enter a new_handler. Keep the module cell retained
        // and validate creator/real renderer again before further state access.
        history.push_back({sequence+1,serial,effect});
        if(effect!=Effect::image_zero_fields)first();
        ++sequence;
    }
    void cleanup_if_quiescent()noexcept {
        if(!busy&&!alive.load())bytes.reset();
    }
};
// Private SAME-module fresh x64 backend accounting. No pooled allocation is
// advertised: real subtype destruction/pool insertion is not yet supported.
struct BnwLabelProcessModuleOwner::LabelAllocationAttempt {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<Impl> module;
    const std::uint64_t ordinal,source_serial;
    const std::uint32_t fresh_total,owned_control_allocation_delta;
    enum class Stage { allocation_entered,allocation_returned,constructor_returned,failed,
        cleanup_entered,subtype_cleanup_returned,storage_reclaimed };
    Stage stage=Stage::allocation_entered;
    enum class Role{embedded,standalone};
    const Role role;
    std::weak_ptr<NotificationLayoutEmbeddedLabelState> actual_child;
    std::weak_ptr<NotificationLayoutStandaloneLabelState> actual_standalone_child;
    LabelAllocationAttempt(const std::shared_ptr<Impl>& saved,std::uint64_t serial,Role family=Role::embedded)
        :module(saved),ordinal(saved->label_attempts.size()+1),source_serial(serial),
         fresh_total(saved->label_fresh_total),owned_control_allocation_delta(saved->owned_control_allocation_delta),role(family){}
};
std::shared_ptr<BnwLabelProcessModuleOwner::LabelAllocationAttempt>
BnwLabelProcessModuleOwner::begin_label_fresh_allocation_owned(
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
        auto ticket=std::shared_ptr<LabelAllocationAttempt>(new LabelAllocationAttempt(saved,operation.operation_serial()));
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
    const std::shared_ptr<NotificationLayoutEmbeddedLabelState>& child){
    const auto saved=impl_;saved->first();operation.require_origin_without_callbacks();saved->first();
    const auto owner=ticket?ticket->module.lock():nullptr;
    if(!ticket||ticket->creator!=std::this_thread::get_id()||!owner||owner.get()!=saved.get()||
       owner.owner_before(saved)||saved.owner_before(owner)||!saved->label_allocating||
       ticket->stage!=LabelAllocationAttempt::Stage::allocation_entered||
       ticket->source_serial!=operation.operation_serial()||saved->label_attempts.empty()||
       saved->label_attempts.back()!=ticket||ticket->ordinal!=saved->label_attempts.size())
        throw std::runtime_error("Label physical allocation return has no actual module attempt");
    ++saved->label_active_attempts; //703157 AFTER allocator returns, BEFORE NULL test
    if(!child){ticket->stage=LabelAllocationAttempt::Stage::failed;saved->label_allocating=false;
        saved->failed.store(true);throw std::runtime_error("Actual Label backend allocation returned NULL");}
    ticket->actual_child=child;ticket->stage=LabelAllocationAttempt::Stage::allocation_returned;
    require_label_allocation_owned(operation,ticket,child,false);
}
void BnwLabelProcessModuleOwner::require_label_allocation_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<LabelAllocationAttempt>& ticket,
    const std::shared_ptr<NotificationLayoutEmbeddedLabelState>& child,bool returned)const{
    const auto saved=impl_;saved->first();operation.require_origin_without_callbacks();saved->first();
    const auto owner=ticket?ticket->module.lock():nullptr;
    const auto object=ticket?ticket->actual_child.lock():nullptr;
    const auto actual=operation.label_module_owned();saved->first();
    const auto self=saved->self.lock();
    if(!ticket||ticket->creator!=std::this_thread::get_id()||!owner||owner.get()!=saved.get()||
       owner.owner_before(saved)||saved.owner_before(owner)||ticket->source_serial!=operation.operation_serial()||
       !ticket->ordinal||ticket->ordinal>saved->label_attempts.size()||saved->label_attempts[ticket->ordinal-1]!=ticket||
       !object||!child||object.get()!=child.get()||object.owner_before(child)||child.owner_before(object)||
       !actual||!self||actual.get()!=self.get()||actual.owner_before(self)||self.owner_before(actual)||
       ticket->stage!=(returned?LabelAllocationAttempt::Stage::constructor_returned:LabelAllocationAttempt::Stage::allocation_returned))
        throw std::runtime_error("Label allocation/constructor return identity retired or changed");
    saved->first();operation.require_origin_without_callbacks();saved->first();
}
// Ordinary live allocation lookup, distinct from constructor lexical Source
// serial and from retiring cleanup. The registered constructor serial remains
// immutable and is checked by the actual Child initialization borrower.
void BnwLabelProcessModuleOwner::require_label_live_allocation_without_callbacks(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<LabelAllocationAttempt>& ticket,
    const std::shared_ptr<NotificationLayoutEmbeddedLabelState>& child,
    std::uint64_t constructor_serial)const{
    const auto saved=impl_;
    const auto pure=[&]{
        saved->first();operation.require_origin_without_callbacks();saved->first();
        const auto owner=ticket?ticket->module.lock():nullptr;
        const auto object=ticket?ticket->actual_child.lock():nullptr;
        const auto current=operation.label_module_owned();saved->first();
        const auto self=saved->self.lock();
        if(!(ticket&&ticket->creator==std::this_thread::get_id()&&owner&&
            owner.get()==saved.get()&&!owner.owner_before(saved)&&!saved.owner_before(owner)&&
            constructor_serial&&ticket->source_serial==constructor_serial&&
            ticket->ordinal&&ticket->ordinal<=saved->label_attempts.size()&&
            saved->label_attempts[ticket->ordinal-1]==ticket&&object&&child&&object.get()==child.get()&&
            !object.owner_before(child)&&!child.owner_before(object)&&current&&self&&
            current.get()==self.get()&&!current.owner_before(self)&&!self.owner_before(current)&&
            ticket->stage==LabelAllocationAttempt::Stage::constructor_returned))
            throw std::runtime_error("Live Label allocation/returned constructor/module origin retired or changed");
        saved->first();operation.require_origin_without_callbacks();saved->first();
    };
    pure(); //Pure registration checks; InitLoan::full supplies full Source FIRST/POST.
    // No retired constructor/Runtime/String borrower is reactivated. Neither
    // cleanup entry, subtype-return, active decrement nor pool reuse is issued.
}

void BnwLabelProcessModuleOwner::finish_label_constructor_return_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<LabelAllocationAttempt>& ticket,
    const std::shared_ptr<NotificationLayoutEmbeddedLabelState>& child){
    require_label_allocation_owned(operation,ticket,child,false);
    const auto saved=impl_;saved->first();
    if(!saved->label_allocating)throw std::runtime_error("Label constructor return without actual active allocation");
    ticket->stage=LabelAllocationAttempt::Stage::constructor_returned;saved->label_allocating=false;
    require_label_allocation_owned(operation,ticket,child,true);
}
void BnwLabelProcessModuleOwner::fail_label_allocation_owned(
    const std::shared_ptr<LabelAllocationAttempt>& ticket)noexcept{
    const auto saved=impl_;saved->failed.store(true);
    if(saved->creator!=std::this_thread::get_id())return;
    saved->label_allocating=false;
    if(ticket&&ticket->creator==std::this_thread::get_id())ticket->stage=LabelAllocationAttempt::Stage::failed;
    // Retain actual counts and attempted/returned identity. No speculative
    // decrement, subtype destruction, successful pool insertion or rollback.
}

#include "bnw_standalone_label_module_core.inc"
#include "bnw_standalone_label_live_allocation_core.inc"

// Retiring bookkeeping uses exact actual module/ticket/child storage and creator.
// It deliberately does not borrow Source/window/GL for memory-only teardown.
void BnwLabelProcessModuleOwner::require_label_cleanup_identity_owned(
    const std::shared_ptr<LabelAllocationAttempt>& ticket,
    const std::shared_ptr<NotificationLayoutEmbeddedLabelState>& child)const{
    const auto saved=impl_;
    if(saved->creator!=std::this_thread::get_id())
        throw std::runtime_error("Label cleanup module foreign creator");
    const auto owner=ticket?ticket->module.lock():nullptr;
    const auto object=ticket?ticket->actual_child.lock():nullptr;
    if(!ticket||ticket->creator!=saved->creator||!owner||owner.get()!=saved.get()||
       owner.owner_before(saved)||saved.owner_before(owner)||!object||!child||
       object.get()!=child.get()||object.owner_before(child)||child.owner_before(object)||
       !ticket->source_serial||!ticket->ordinal||ticket->ordinal>saved->label_attempts.size()||
       saved->label_attempts[ticket->ordinal-1]!=ticket)
        throw std::runtime_error("Label cleanup actual allocation/module membership changed");
}
void BnwLabelProcessModuleOwner::begin_label_cleanup_owned(
    const std::shared_ptr<LabelAllocationAttempt>& ticket,
    const std::shared_ptr<NotificationLayoutEmbeddedLabelState>& child){
    require_label_cleanup_identity_owned(ticket,child);
    if(ticket->stage!=LabelAllocationAttempt::Stage::constructor_returned)
        throw std::runtime_error("Label cleanup lacks actual completed constructor or was replayed");
    ticket->stage=LabelAllocationAttempt::Stage::cleanup_entered;
}
void BnwLabelProcessModuleOwner::finish_label_subtype_cleanup_owned(
    const std::shared_ptr<LabelAllocationAttempt>& ticket,
    const std::shared_ptr<NotificationLayoutEmbeddedLabelState>& child){
    require_label_cleanup_identity_owned(ticket,child);
    if(ticket->stage!=LabelAllocationAttempt::Stage::cleanup_entered)
        throw std::runtime_error("Label actual subtype cleanup return has no retiring call");
    // Sole caller follows real closed64C8C0/5126B0/5BC660 effects. No public
    // boolean, metadata importer or caller-supplied callback grants this state.
    ticket->stage=LabelAllocationAttempt::Stage::subtype_cleanup_returned;
}
void BnwLabelProcessModuleOwner::finish_label_reclamation_owned(
    const std::shared_ptr<LabelAllocationAttempt>& ticket,
    const std::shared_ptr<NotificationLayoutEmbeddedLabelState>& child){
    require_label_cleanup_identity_owned(ticket,child);
    const auto saved=impl_;
    if(ticket->stage!=LabelAllocationAttempt::Stage::subtype_cleanup_returned||
       !saved->label_active_attempts)
        throw std::runtime_error("Label reclamation cannot skip actual subtype return/active allocation");
    // The qualified OWN backend reclaims fresh x64 storage rather than storing
    // Original pooled addresses. No pool reuse is admitted. Physical storage
    // lasts until retiring parent/local/read-only references really drop.
    ++saved->owned_label_reclamations;
    --saved->label_active_attempts;
    ticket->stage=LabelAllocationAttempt::Stage::storage_reclaimed;
}

BnwLabelProcessModuleOwner::BnwLabelProcessModuleOwner(std::shared_ptr<Impl> state)
    :impl_(std::move(state)){}
std::shared_ptr<BnwLabelProcessModuleOwner>
BnwLabelProcessModuleOwner::create_fresh_owned(UiScreenGlRenderer& actual){
    // Only the real renderer constructor calls this, at body entry BEFORE
    // shader/VAO creation. It cannot publish an unreturned renderer/module.
    const auto life=actual.worldview_lifetime_;
    if(!life||life->creator!=std::this_thread::get_id()||!life->alive.load()||
       life->failed.load()||life->renderer!=&actual||actual.label_module_)
        throw std::runtime_error("Label module requires unique real renderer constructor");
    auto state=std::make_shared<Impl>(actual,life);
    state->first();
    auto result=std::shared_ptr<BnwLabelProcessModuleOwner>(
        new BnwLabelProcessModuleOwner(state));
    state->self=result;state->first();return result;
}
BnwLabelProcessModuleOwner::~BnwLabelProcessModuleOwner(){retire_owned();}
void BnwLabelProcessModuleOwner::require_current_without_callbacks()const{
    const auto saved=impl_;saved->first();
}
void BnwLabelProcessModuleOwner::require_renderer_origin_owned(
    const UiScreenGlRenderer& actual)const{
    const auto saved=impl_;saved->first();
    if(saved->renderer!=&actual)
        throw std::runtime_error("Label module belongs to another actual renderer");
    const auto retained=saved->renderer_life.lock();
    const auto current=actual.worldview_lifetime_;
    saved->first();
    if(!retained||retained.get()!=current.get()||retained.owner_before(current)||
       current.owner_before(retained))
        throw std::runtime_error("Label module actual renderer control replaced");
}
void BnwLabelProcessModuleOwner::retire_owned()noexcept{
    const auto saved=impl_;saved->alive.store(false);
    if(saved->creator!=std::this_thread::get_id()){
        // No foreign read/mutation of byte/count/history; authority still dies.
        saved->failed.store(true);return;
    }
    saved->cleanup_if_quiescent();
}
void BnwLabelProcessModuleOwner::constructor_space_prefix_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    const auto saved=impl_;saved->first();
    if(saved->busy){saved->failed.store(true);throw std::runtime_error("Label module constructor reentry");}
    const auto serial=operation.operation_serial();
    const auto pure=[&]{
        saved->first();operation.require_origin_without_callbacks();saved->first();
        if(operation.operation_serial()!=serial)
            throw std::runtime_error("Label module constructor Source occurrence replaced");
        const auto actual=operation.label_module_owned();saved->first();
        const auto self=saved->self.lock();saved->first();
        if(!actual||!self||actual.get()!=self.get()||actual.owner_before(self)||
           self.owner_before(actual)||actual->impl_.get()!=saved.get()||
           actual->impl_.owner_before(saved)||saved.owner_before(actual->impl_))
            throw std::runtime_error("Label constructor requires SAME issued renderer module");
        operation.require_origin_without_callbacks();saved->first();
    };
    struct Scope {std::shared_ptr<Impl> state;~Scope(){state->busy=false;state->cleanup_if_quiescent();}};
    saved->busy=true;Scope scope{saved};
    try{
        pure();
        if(saved->count){
            if(saved->count!=1||saved->capacity!=1||!saved->bytes)
                throw std::runtime_error("Label module nonzero buffer outside proved fresh-space domain");
            saved->record(serial,Impl::Effect::count_nonzero_skip);pure();return;
        }
        if(saved->capacity!=0||saved->bytes)
            throw std::runtime_error("Label fresh module buffer has unissued nonzero capacity/pointer");
        // Closed successful67A6E0(0) fresh-image branch. Original allocation
        // failure/unknown pointer outcome does not become a successful skip.
        saved->record(serial,Impl::Effect::reserve_begin);pure();
        saved->capacity=1;
        auto allocation=std::unique_ptr<std::uint8_t[]>(new std::uint8_t[1]);pure();
        saved->record(serial,Impl::Effect::allocation_returned);pure();
        if(saved->count!=0||saved->capacity!=1||saved->bytes)
            throw std::runtime_error("Label module allocation callback changed admitted prefix");
        saved->bytes=std::move(allocation);
        saved->record(serial,Impl::Effect::pointer_published);pure();
        saved->bytes[0]=0x20;
        saved->record(serial,Impl::Effect::space_written);pure();
        ++saved->count;
        saved->record(serial,Impl::Effect::count_incremented);pure();
    }catch(...){saved->failed.store(true);throw;}
}
}
