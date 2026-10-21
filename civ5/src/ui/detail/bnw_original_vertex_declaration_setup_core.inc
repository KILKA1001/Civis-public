// Inside civ5::ui after the complete Root Init declaration. All NEW Root
// phase/backend/result methods are mandatory external producers, not stubs.
namespace detail {
#include "bnw_original_vertex_declaration_fixed_data.inc"

struct BnwOriginalVertexDeclarationSetupLoan::Impl final {
    using Initializer=BnwOriginalNonFontDeviceInitializer;
    const std::shared_ptr<const Initializer> initializer;
    const std::uint64_t epoch,creation_serial;
    const std::thread::id creator=std::this_thread::get_id();
    std::array<std::uint32_t,29> strides;
    std::array<std::shared_ptr<const BnwOriginalVertexDeclarationDeviceResultLoan>,29> results;
    std::array<std::uint8_t,64> local_cleared;
    std::uint32_t next_slot=0;
    std::weak_ptr<const BnwOriginalVertexDeclarationCreateLoan> current_create;
    std::weak_ptr<const BnwOriginalVertexDeclarationSetupLoan> canonical;
    mutable std::atomic<bool> failed{false},checking{false};

    Impl(std::shared_ptr<const Initializer> actual,std::uint64_t e,std::uint64_t serial)
        :initializer(std::move(actual)),epoch(e),creation_serial(serial){}

    void first_creation()const {
        if(creator!=std::this_thread::get_id()||failed.load()||!initializer||next_slot>29)
            throw std::runtime_error("Original vertex declaration actor retired/reentered");
        initializer->require_physical_without_callbacks();
        const auto actual=initializer->retain_owner_owned_without_callbacks();
        if(actual.get()!=initializer.get()||actual.owner_before(initializer)||initializer.owner_before(actual)||
           initializer->initialization_epoch_without_callbacks()!=epoch)
            throw std::runtime_error("Original vertex declaration Init/epoch differs");
    }
    void full_creation(const runtime::BnwWorldViewSourceOperationLoan& op)const {
        first_creation();
        if(checking.exchange(true)){
            failed.store(true);
            throw std::runtime_error("Original vertex declaration Source check reentered");
        }
        struct End {const Impl* p;~End(){p->checking.store(false);}}end{this};
        try{
            op.require_origin_without_callbacks();op.require_render_operation_owned();first_creation();
            if(op.operation_serial()!=creation_serial)
                throw std::runtime_error("Original vertex declaration creation Source changed");
            op.require_operation(creation_serial);first_creation();op.require_origin_without_callbacks();
            initializer->require_vertex_declaration_devices_after_source_full_owned(op);
            // Callback-free final gate: genuine fresh8E1060 phase, selected
            // current fixed-table profile and SAME canonical device owners.
            initializer->require_vertex_declaration_setup_creation_owned(op);
            first_creation();op.require_origin_without_callbacks();
        }catch(...){failed.store(true);throw;}
    }
    void first(const BnwOriginalVertexDeclarationSetupLoan* receiver)const {
        first_creation();
        const auto self=canonical.lock();
        if(!self||self.get()!=receiver||self->impl_.get()!=this||next_slot!=29||!current_create.expired())
            throw std::runtime_error("Original vertex declaration completed actor differs");
    }
    void full(const BnwOriginalVertexDeclarationSetupLoan* receiver,
        const runtime::BnwWorldViewSourceOperationLoan& op)const {
        first(receiver);
        if(checking.exchange(true)){
            failed.store(true);
            throw std::runtime_error("Original vertex declaration current actor reentered");
        }
        struct End {const Impl* p;~End(){p->checking.store(false);}}end{this};
        try{
            const auto self=canonical.lock();first(receiver);
            op.require_origin_without_callbacks();op.require_render_operation_owned();first(receiver);
            const auto serial=op.operation_serial();first(receiver);
            op.require_operation(serial);first(receiver);op.require_origin_without_callbacks();
            initializer->require_vertex_declaration_devices_after_source_full_owned(op);
            // A retained getter uses the published SAME actual Root member,
            // never renews the old constructor phase or create operand.
            initializer->require_vertex_declaration_setup_current_owned(op,epoch,*self);
            first(receiver);op.require_origin_without_callbacks();
        }catch(...){failed.store(true);throw;}
    }
};

struct BnwOriginalVertexDeclarationCreateLoan::Impl final {
    const std::shared_ptr<BnwOriginalVertexDeclarationSetupLoan::Impl> actor;
    const std::uint32_t slot;
    const std::vector<BnwOriginalVertexDeclarationElement> elements;
    std::weak_ptr<const BnwOriginalVertexDeclarationCreateLoan> canonical;
    bool active=true;
    Impl(std::shared_ptr<BnwOriginalVertexDeclarationSetupLoan::Impl> actual,
        std::uint32_t original_slot,std::vector<BnwOriginalVertexDeclarationElement> actual_elements)
        :actor(std::move(actual)),slot(original_slot),elements(std::move(actual_elements)){}
    void first(const BnwOriginalVertexDeclarationCreateLoan* receiver)const {
        actor->first_creation();
        const auto self=canonical.lock(),current=actor->current_create.lock();
        if(!active||!self||self.get()!=receiver||self->impl_.get()!=this||
           !current||current.get()!=receiver||current.owner_before(self)||self.owner_before(current)||
           actor->next_slot!=slot||slot>=29||elements.empty())
            throw std::runtime_error("Original vertex declaration lexical create operand retired");
    }
    void full(const BnwOriginalVertexDeclarationCreateLoan* receiver,
        const runtime::BnwWorldViewSourceOperationLoan& op)const {
        first(receiver);actor->full_creation(op);first(receiver);
    }
};

BnwOriginalVertexDeclarationCreateLoan::BnwOriginalVertexDeclarationCreateLoan(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwOriginalVertexDeclarationCreateLoan::~BnwOriginalVertexDeclarationCreateLoan()=default;
void BnwOriginalVertexDeclarationCreateLoan::require_physical_without_callbacks()const{
    const auto p=impl_;p->first(this);
}
void BnwOriginalVertexDeclarationCreateLoan::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op)const{
    const auto self=retain_owner_owned_without_callbacks();self->impl_->full(self.get(),op);
}
std::shared_ptr<const BnwOriginalVertexDeclarationCreateLoan>
BnwOriginalVertexDeclarationCreateLoan::retain_owner_owned_without_callbacks()const{
    const auto p=impl_;p->first(this);const auto self=p->canonical.lock();p->first(this);return self;
}
std::uint32_t BnwOriginalVertexDeclarationCreateLoan::slot_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op)const{
    const auto self=retain_owner_owned_without_callbacks();self->impl_->full(self.get(),op);return self->impl_->slot;
}
std::span<const BnwOriginalVertexDeclarationElement>
BnwOriginalVertexDeclarationCreateLoan::elements_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op)const{
    const auto self=retain_owner_owned_without_callbacks();self->impl_->full(self.get(),op);return self->impl_->elements;
}

BnwOriginalVertexDeclarationSetupLoan::BnwOriginalVertexDeclarationSetupLoan(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwOriginalVertexDeclarationSetupLoan::~BnwOriginalVertexDeclarationSetupLoan()=default;

std::shared_ptr<const BnwOriginalVertexDeclarationSetupLoan>
BnwOriginalVertexDeclarationSetupLoan::execute_manager8e1060_owned(
    const BnwOriginalNonFontDeviceInitializer& actual_initializer,
    const runtime::BnwWorldViewSourceOperationLoan& op){
    const auto initializer=actual_initializer.retain_owner_owned_without_callbacks();
    if(!initializer||initializer.get()!=&actual_initializer)
        throw std::runtime_error("Original vertex declaration actor needs canonical Init");
    initializer->require_physical_without_callbacks();
    op.require_origin_without_callbacks();op.require_render_operation_owned();
    const auto epoch=initializer->initialization_epoch_without_callbacks();
    const auto serial=op.operation_serial();
    const auto state=std::make_shared<Impl>(initializer,epoch,serial);
    initializer->bind_vertex_declaration_actor_owned(op,state);
    state->full_creation(op);
    try{
        // Original order:20CBC stride range,434 pointer range, local maps,
        // local64B memset, then29 rows. shared_ptr reset represents null native
        // slots safely; no memset of C++ smart-pointer object representations.
        static_assert(sizeof(state->strides)==0x74);
        std::memset(state->strides.data(),0,0x74);
        for(auto& p:state->results)p.reset();
        const auto type_map=original8e1060_types;
        const auto usage_map=original8e1060_usages;
        std::memset(state->local_cleared.data(),0,0x40);
        while(state->next_slot<29){
            state->full_creation(op);
            const auto slot=state->next_slot;
            const auto definition=original8e1060_rows[slot];
            if(definition.count==0){
                // This branch exists in fourteen actual qualified PE rows.
                state->strides[slot]=0;
                ++state->next_slot;
                continue;
            }
            if(definition.first>original8e1060_records.size()||
               definition.count>original8e1060_records.size()-definition.first)
                throw std::runtime_error("Original vertex declaration fixed record span differs");
            std::vector<BnwOriginalVertexDeclarationElement> elements;
            elements.reserve(static_cast<std::size_t>(definition.count)+1);
            state->full_creation(op);
            std::uint32_t stride=0;
            for(std::uint32_t j=0;j<definition.count;++j){
                const auto record=original8e1060_records[definition.first+j];
                if(record.type_key>=type_map.size()||record.usage_key>=usage_map.size())
                    throw std::runtime_error("Original vertex declaration fixed map key differs");
                const auto type=type_map[record.type_key];
                const auto usage=usage_map[record.usage_key];
                elements.push_back({0,record.offset,type.type,0,usage.usage,usage.index});
                stride+=type.size; // Original unsigned DWORD accumulation.
            }
            elements.push_back({255,0,17,0,0,0}); // Genuine D3DDECL_END8B.
            state->full_creation(op);
            const auto frame=std::make_shared<BnwOriginalVertexDeclarationCreateLoan::Impl>(state,slot,std::move(elements));
            state->full_creation(op);
            const auto operand=std::shared_ptr<const BnwOriginalVertexDeclarationCreateLoan>(
                new BnwOriginalVertexDeclarationCreateLoan(frame));
            state->full_creation(op);
            frame->canonical=operand;state->current_create=operand;
            struct Retire {
                BnwOriginalVertexDeclarationCreateLoan::Impl* f;
                Impl* p;
                ~Retire(){f->active=false;p->current_create.reset();}
            }retire{frame.get(),state.get()};
            frame->full(operand.get(),op);
            const auto result=initializer->create_original_vertex_declaration_owned(op,*operand);
            // A caught native/Lua failure or reentry must not reach the next
            // backend call. Fresh FULL before callback-free result identity.
            frame->full(operand.get(),op);
            if(!result)
                throw std::runtime_error("Original vertex declaration backend return is unissued");
            initializer->require_original_vertex_declaration_result_creation_owned(op,epoch,*operand,*result);
            frame->first(operand.get());op.require_origin_without_callbacks();
            // The real result may describe an actual failed HRESULT/null
            // native object: Original ignores HRESULT. Result authenticity,
            // rather than fabricated success or non-null GPU name, is needed.
            state->results[slot]=result;
            state->strides[slot]=stride;
            frame->active=false;state->current_create.reset();
            ++state->next_slot;
        }
        state->full_creation(op);
        if(!state->current_create.expired()||state->next_slot!=29)
            throw std::runtime_error("Original vertex declaration ordered actor incomplete");
        const auto result=std::shared_ptr<const BnwOriginalVertexDeclarationSetupLoan>(
            new BnwOriginalVertexDeclarationSetupLoan(state));
        state->full_creation(op);state->canonical=result;state->first(result.get());
        // Root publishes this SAME actual manager state only after this
        // genuine whole8E1060 return; no sampler/manager/Draw readiness here.
        return result;
    }catch(...){state->failed.store(true);throw;}
}

void BnwOriginalVertexDeclarationSetupLoan::require_physical_without_callbacks()const{
    const auto p=impl_;p->first(this);
}
std::shared_ptr<const BnwOriginalVertexDeclarationSetupLoan>
BnwOriginalVertexDeclarationSetupLoan::retain_owner_owned_without_callbacks()const{
    const auto p=impl_;p->first(this);const auto self=p->canonical.lock();p->first(this);return self;
}
void BnwOriginalVertexDeclarationSetupLoan::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op)const{
    const auto self=retain_owner_owned_without_callbacks();self->impl_->full(self.get(),op);
}
std::uint32_t BnwOriginalVertexDeclarationSetupLoan::stride_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,std::uint32_t slot)const{
    const auto self=retain_owner_owned_without_callbacks();const auto p=self->impl_;p->full(self.get(),op);
    if(slot>=29)throw std::runtime_error("Original vertex declaration slot out of range");
    return p->strides[slot];
}
std::shared_ptr<const BnwOriginalVertexDeclarationDeviceResultLoan>
BnwOriginalVertexDeclarationSetupLoan::declaration_result_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,std::uint32_t slot)const{
    const auto self=retain_owner_owned_without_callbacks();const auto p=self->impl_;p->full(self.get(),op);
    if(slot>=29)throw std::runtime_error("Original vertex declaration slot out of range");
    const auto result=p->results[slot];
    if(original8e1060_rows[slot].count==0){
        if(result||p->strides[slot]!=0)throw std::runtime_error("Original empty declaration row differs");
        return {};
    }
    if(!result)throw std::runtime_error("Original declaration invocation return retired");
    // Retained result check uses the completed actor, not an expired lexical
    // CreateLoan. It must be callback-free after FULL before the return.
    p->initializer->require_original_vertex_declaration_result_current_owned(op,p->epoch,*self,slot,*result);
    p->first(self.get());op.require_origin_without_callbacks();return result;
}
} // namespace detail
