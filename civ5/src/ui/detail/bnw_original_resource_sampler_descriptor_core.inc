// Private leaf inside civ5::ui, after the Root-owned complete Init declaration.
// The two NEW Init gates below are exact proposals, not supplied issuers.
namespace detail {
struct BnwOriginalResourceSamplerDescriptorLoan::Impl final {
    using Initializer=BnwOriginalNonFontDeviceInitializer;
    const std::shared_ptr<const Initializer> initializer;
    const std::uint64_t epoch,creation_serial;
    const std::thread::id creator=std::this_thread::get_id();
    const ConstructorFieldSnapshot fields;
    std::weak_ptr<const BnwOriginalResourceSamplerDescriptorLoan> canonical;
    mutable std::atomic<bool> failed{false};
    mutable std::atomic<bool> checking{false};

    // Complete original8F6D20..8F6D57: seven DWORD stores and one byte store.
    // Actual arg2 is never read; arg3 is written to both+4 and+8.
    static ConstructorFieldSnapshot copy_actual_8f6d20(
        const std::array<std::uint32_t,8>& actual_arguments)noexcept {
        return {{actual_arguments[0],actual_arguments[2],actual_arguments[2],
                 actual_arguments[3],actual_arguments[4],actual_arguments[5],
                 actual_arguments[6]},static_cast<std::uint8_t>(actual_arguments[7])};
    }

    Impl(std::shared_ptr<const Initializer> actual,std::uint64_t actual_epoch,
        std::uint64_t actual_serial,const std::array<std::uint32_t,8>& arguments)
        :initializer(std::move(actual)),epoch(actual_epoch),creation_serial(actual_serial),
         fields(copy_actual_8f6d20(arguments)){}

    void first(const BnwOriginalResourceSamplerDescriptorLoan* receiver)const {
        if(creator!=std::this_thread::get_id()||failed.load()||!initializer)
            throw std::runtime_error("Original sampler descriptor creator/owner retired");
        const auto self=canonical.lock();
        if(!self||self.get()!=receiver||self->impl_.get()!=this)
            throw std::runtime_error("Original sampler descriptor canonical instance differs");
        initializer->require_physical_without_callbacks();
        const auto actual=initializer->retain_owner_owned_without_callbacks();
        if(actual.get()!=initializer.get()||actual.owner_before(initializer)||initializer.owner_before(actual)||
           initializer->initialization_epoch_without_callbacks()!=epoch)
            throw std::runtime_error("Original sampler descriptor canonical Init/epoch differs");
    }

    void full(const BnwOriginalResourceSamplerDescriptorLoan* receiver,
        const runtime::BnwWorldViewSourceOperationLoan& operation)const {
        first(receiver);
        if(checking.exchange(true)){
            failed.store(true);
            throw std::runtime_error("Original sampler descriptor currentness reentered");
        }
        struct End {const Impl* p;~End(){p->checking.store(false);}}end{this};
        try{
            const auto self=canonical.lock();
            first(receiver);
            operation.require_origin_without_callbacks();
            operation.require_render_operation_owned();first(receiver);
            const auto serial=operation.operation_serial();first(receiver);
            operation.require_operation(serial);
            first(receiver);operation.require_origin_without_callbacks();
            // Successful Root gate is callback-free and checks the actual
            // member AFTER Source callbacks, before immutable field access.
            // No old creation guard is renewed by this currentness boundary.
            initializer->require_manager20da8_sampler_descriptor_current_owned(
                operation,epoch,*self);
            first(receiver);operation.require_origin_without_callbacks();
        }catch(...){failed.store(true);throw;}
    }
};

BnwOriginalResourceSamplerDescriptorLoan::BnwOriginalResourceSamplerDescriptorLoan(
    std::shared_ptr<Impl> actual):impl_(std::move(actual)){}
BnwOriginalResourceSamplerDescriptorLoan::~BnwOriginalResourceSamplerDescriptorLoan()=default;

std::shared_ptr<const BnwOriginalResourceSamplerDescriptorLoan>
BnwOriginalResourceSamplerDescriptorLoan::create_manager20da8_8e3bd2_owned(
    const BnwOriginalNonFontDeviceInitializer& actual_initializer,
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    const auto initializer=actual_initializer.retain_owner_owned_without_callbacks();
    if(!initializer||initializer.get()!=&actual_initializer)
        throw std::runtime_error("Original sampler descriptor needs actual canonical Init");
    initializer->require_physical_without_callbacks();
    operation.require_origin_without_callbacks();operation.require_render_operation_owned();
    const auto epoch=initializer->initialization_epoch_without_callbacks();
    const auto serial=operation.operation_serial();
    const auto first=[&]{
        initializer->require_physical_without_callbacks();
        if(initializer->initialization_epoch_without_callbacks()!=epoch)
            throw std::runtime_error("Original sampler descriptor creation Init epoch changed");
        operation.require_origin_without_callbacks();
        if(operation.operation_serial()!=serial)
            throw std::runtime_error("Original sampler descriptor creation Source changed");
    };
    const auto full=[&]{
        first();
        // This is NOT the CPU or post-upload gate. Original intervening stores,
        // record-flag loop and actual8E1060 actor must have completed first.
        operation.require_operation(serial);first();
        // Successful Root phase check is callback-free. It follows FULL so a
        // caught callback cannot advance/retire the phase before the copy.
        initializer->require_manager20da8_sampler_creation_owned(operation);
        first();
    };
    full();
    // Only this one actual8E3BD2 call is implemented. No default catalogue or
    // arbitrary caller argument DTO can publish another descriptor instance.
    constexpr std::array<std::uint32_t,8> actual_8e3bd2_arguments{1,1,1,2,2,2,1,0};
    const auto storage=std::make_shared<Impl>(initializer,epoch,serial,actual_8e3bd2_arguments);
    full();
    const auto result=std::shared_ptr<const BnwOriginalResourceSamplerDescriptorLoan>(
        new BnwOriginalResourceSamplerDescriptorLoan(storage));
    full();storage->canonical=result;first();storage->first(result.get());
    // Root publishes its actual descriptor member only after this constructor
    // return. This return itself does not publish the complete native manager.
    return result;
}

void BnwOriginalResourceSamplerDescriptorLoan::require_physical_without_callbacks()const{
    const auto storage=impl_;storage->first(this);
}
std::shared_ptr<const BnwOriginalResourceSamplerDescriptorLoan>
BnwOriginalResourceSamplerDescriptorLoan::retain_owner_owned_without_callbacks()const{
    const auto storage=impl_;storage->first(this);
    const auto result=storage->canonical.lock();storage->first(this);return result;
}
void BnwOriginalResourceSamplerDescriptorLoan::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto self=retain_owner_owned_without_callbacks();
    const auto storage=self->impl_;storage->full(self.get(),operation);
}
BnwOriginalResourceSamplerDescriptorLoan::ConstructorFieldSnapshot
BnwOriginalResourceSamplerDescriptorLoan::constructor_fields_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto self=retain_owner_owned_without_callbacks();
    const auto storage=self->impl_;storage->full(self.get(),operation);
    return storage->fields;
}

BnwOriginalResourceSamplerDescriptorLoan::RawBgra8UnormStoragePolicy::RawBgra8UnormStoragePolicy(
    std::shared_ptr<const BnwOriginalResourceSamplerDescriptorLoan> actual):descriptor_(std::move(actual)){}
void BnwOriginalResourceSamplerDescriptorLoan::RawBgra8UnormStoragePolicy::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    if(!descriptor_)throw std::runtime_error("Raw BGRA storage policy lost actual descriptor");
    const auto actual=descriptor_->retain_owner_owned_without_callbacks();
    if(actual.get()!=descriptor_.get()||actual.owner_before(descriptor_)||descriptor_.owner_before(actual))
        throw std::runtime_error("Raw BGRA storage policy canonical descriptor differs");
    actual->require_source_owned(operation);actual->require_physical_without_callbacks();
}
std::uint32_t BnwOriginalResourceSamplerDescriptorLoan::RawBgra8UnormStoragePolicy::original_d3d_storage_format_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    require_source_owned(operation);return 21; // Actual8E3AF7 storage argument, not a sampler decision.
}
std::array<std::uint8_t,4> BnwOriginalResourceSamplerDescriptorLoan::RawBgra8UnormStoragePolicy::component_bits_in_native_bgra_order_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    require_source_owned(operation);return {8,8,8,8}; // Exact accepted native BGRA bytes only.
}
BnwOriginalResourceSamplerDescriptorLoan::RawBgra8UnormStoragePolicy
BnwOriginalResourceSamplerDescriptorLoan::raw_bgra8_unorm_storage_policy_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto self=retain_owner_owned_without_callbacks();
    const auto storage=self->impl_;storage->full(self.get(),operation);
    return RawBgra8UnormStoragePolicy(self);
}
} // namespace detail
