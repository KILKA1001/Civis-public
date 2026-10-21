// Inside civ5::ui after NotificationContextGuiPort::Impl and scalar2; before
// the accepted CPU image implementation. The Ui facade ABI is Graph owned.
namespace detail {
struct BnwOriginalNonFontDeviceInitializer::Impl final {
    using Lifetime=BnwWorldViewRendererProjectionLoan::Lifetime;
    enum class Stage {cpu_creation,cpu_complete,gpu_creation,gpu_complete,failed,retired};
    enum class VertexStage {unstarted,creating,completed,failed,retired};
    const std::thread::id creator=std::this_thread::get_id();
    UiScreenGlRenderer* const renderer;
    const std::shared_ptr<Lifetime> renderer_life;
    const std::shared_ptr<NotificationContextGuiPort> gui;
    const std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window;
    const std::shared_ptr<const graphics::detail::BnwRendererContextPin> pin;
    const std::shared_ptr<const assets::CivisResourceBankOwner> selected_bank;
    const std::uint64_t epoch;
    const std::uint64_t cpu_creation_serial;
    const runtime::BnwWorldViewSourceOperationLoan* const cpu_creation_operation;
    Stage stage=Stage::cpu_creation;
    std::uint64_t gpu_creation_serial=0;
    const runtime::BnwWorldViewSourceOperationLoan* gpu_creation_operation=nullptr;
    std::weak_ptr<const BnwOriginalNonFontDeviceInitializer> canonical;
    // CPU/GPU retain the initializer. Its reverse associations stay weak.
    std::weak_ptr<const BnwOriginalGeneratedResourceImageLoan> cpu;
    std::weak_ptr<const graphics::detail::BnwNormalResourceGpuImageLoan> gpu;
    VertexStage vertex_stage=VertexStage::unstarted;
    std::shared_ptr<const VertexDeclarationFixedTable> vertex_table;
    std::weak_ptr<VertexDeclarationCreationPhase> vertex_phase;
    std::weak_ptr<const BnwOriginalVertexDeclarationSetupLoan> vertex_setup;

    Impl(UiScreenGlRenderer& actual_renderer,std::shared_ptr<Lifetime> life,
        std::shared_ptr<NotificationContextGuiPort> actual_gui,
        std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> actual_window,
        std::shared_ptr<const graphics::detail::BnwRendererContextPin> actual_pin,
        std::shared_ptr<const assets::CivisResourceBankOwner> bank,std::uint64_t actual_epoch,
        const runtime::BnwWorldViewSourceOperationLoan& operation,std::uint64_t serial)
        :renderer(&actual_renderer),renderer_life(std::move(life)),gui(std::move(actual_gui)),
         window(std::move(actual_window)),pin(std::move(actual_pin)),selected_bank(std::move(bank)),
         epoch(actual_epoch),cpu_creation_serial(serial),cpu_creation_operation(&operation){}

    void first()const {
        require(creator==std::this_thread::get_id()&&epoch&&cpu_creation_serial,
            "Resource0 initializer foreign creator or missing actual transition");
        require(stage!=Stage::failed&&stage!=Stage::retired,
            "Resource0 initializer failed/retired");
        require(renderer&&renderer_life&&renderer_life->creator==creator&&
            renderer_life->renderer==renderer&&renderer_life->alive.load()&&!renderer_life->failed.load(),
            "Resource0 initializer actual renderer retired/failed");
        require(gui&&gui->impl_&&!window.expired()&&pin&&selected_bank,
            "Resource0 initializer missing canonical owners");
        const auto& body=*gui->impl_;
        require(body.creator==creator&&body.facade_alive.load()&&!body.failed.load()&&
            !body.source.expired()&&!body.host.expired()&&!body.world.expired()&&!body.scene.expired(),
            "Resource0 initializer canonical GUI realm retired/failed");
        const auto canonical_gui=body.canonical_gui.lock();
        require(canonical_gui&&canonical_gui.get()==gui.get()&&
            !canonical_gui.owner_before(gui)&&!gui.owner_before(canonical_gui),
            "Resource0 initializer GUI shared control changed");
        require(!body.window.expired()&&!window.owner_before(body.window)&&
            !body.window.owner_before(window),
            "Resource0 initializer window differs from actual GUI Source origin");
        graphics::BnwOwnedCameraRendererWindowLease::require_retained_context_pin_origin_without_callbacks(window,pin);
        pin->first_without_callbacks();
        require(pin->identity_.window==body.hwnd&&pin->identity_.owner_thread,
            "Resource0 initializer captured context differs from GUI native origin");
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const {
        first();operation.require_render_operation_owned();first();
        gui->require_current_owned(operation);first();
        operation.require_current();first();
        const auto current_bank=operation.selected_provider_startup_bank_owned();first();
        require(current_bank&&current_bank.get()==selected_bank.get()&&
            !current_bank.owner_before(selected_bank)&&!selected_bank.owner_before(current_bank),
            "Resource0 initializer selected Bank changed");
        gui->require_current_owned(operation);first();operation.require_current();first();
    }
    void same_cpu(const std::shared_ptr<const BnwOriginalGeneratedResourceImageLoan>& offered)const {
        first();const auto actual=cpu.lock();
        require(offered&&actual&&offered.get()==actual.get()&&
            !offered.owner_before(actual)&&!actual.owner_before(offered),
            "Resource0 GPU producer requires SAME canonical CPU image");
        offered->require_physical_without_callbacks();first();
        const auto retained=offered->retain_owner_owned_without_callbacks();first();
        require(retained&&retained.get()==offered.get()&&
            !retained.owner_before(offered)&&!offered.owner_before(retained),
            "Resource0 GPU producer CPU shared control mismatch");
    }
};

BnwOriginalNonFontDeviceInitializer::BnwOriginalNonFontDeviceInitializer(std::shared_ptr<Impl> body)
    :impl_(std::move(body)){}
BnwOriginalNonFontDeviceInitializer::~BnwOriginalNonFontDeviceInitializer(){retire_owned();}
void BnwOriginalNonFontDeviceInitializer::retire_owned()const noexcept {
    if(impl_){retire_vertex_declarations_owned();impl_->stage=Impl::Stage::retired;impl_->cpu.reset();impl_->gpu.reset();
        impl_->gpu_creation_operation=nullptr;impl_->gpu_creation_serial=0;}
}
void BnwOriginalNonFontDeviceInitializer::require_physical_without_callbacks()const {
    require(bool(impl_),"Resource0 initializer body absent");impl_->first();
    const auto actual=impl_->canonical.lock();
    require(actual&&actual.get()==this,"Resource0 initializer canonical owner retired");impl_->first();
}
std::shared_ptr<const BnwOriginalNonFontDeviceInitializer>
BnwOriginalNonFontDeviceInitializer::retain_owner_owned_without_callbacks()const {
    require_physical_without_callbacks();const auto actual=impl_->canonical.lock();
    require(actual&&actual.get()==this,"Resource0 initializer canonical retention failed");
    require_physical_without_callbacks();return actual;
}
std::uint64_t BnwOriginalNonFontDeviceInitializer::initialization_epoch_without_callbacks()const {
    require_physical_without_callbacks();return impl_->epoch;
}
void BnwOriginalNonFontDeviceInitializer::require_cpu_resource0_creation_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const {
    const auto held=retain_owner_owned_without_callbacks();
    const auto first=[&]{held->require_physical_without_callbacks();
        require(held->impl_->stage==Impl::Stage::cpu_creation&&
            held->impl_->cpu_creation_operation==&operation,
            "Resource0 CPU creation outside executed initializer phase");};
    first();operation.require_operation(held->impl_->cpu_creation_serial);first();
    held->impl_->full(operation);first();operation.require_operation(held->impl_->cpu_creation_serial);first();
}
void BnwOriginalNonFontDeviceInitializer::require_cpu_resource0_image_current_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,std::uint64_t epoch)const {
    const auto held=retain_owner_owned_without_callbacks();
    const auto first=[&]{held->require_physical_without_callbacks();
        require(epoch==held->impl_->epoch,"Resource0 CPU initializer epoch mismatch");};
    first();held->impl_->full(operation);first();
}
void BnwOriginalNonFontDeviceInitializer::require_gpu_resource0_creation_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const BnwOriginalGeneratedResourceImageLoan>& cpu)const {
    const auto held=retain_owner_owned_without_callbacks();
    held->impl_->same_cpu(cpu);const auto owned_cpu=cpu;
    const auto first=[&]{held->require_physical_without_callbacks();held->impl_->same_cpu(owned_cpu);
        require(held->impl_->stage==Impl::Stage::gpu_creation&&
            held->impl_->gpu_creation_operation==&operation&&held->impl_->gpu_creation_serial,
            "Resource0 GPU creation outside executed initializer upload");};
    first();held->impl_->full(operation);first();owned_cpu->require_source_owned(operation);first();
    operation.require_operation(held->impl_->gpu_creation_serial);first();
}
void BnwOriginalNonFontDeviceInitializer::require_gpu_resource0_image_current_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const BnwOriginalGeneratedResourceImageLoan>& cpu,std::uint64_t epoch)const {
    const auto held=retain_owner_owned_without_callbacks();
    held->impl_->same_cpu(cpu);const auto owned_cpu=cpu;
    const auto first=[&]{held->require_physical_without_callbacks();held->impl_->same_cpu(owned_cpu);
        require(epoch==held->impl_->epoch&&
            (held->impl_->stage==Impl::Stage::gpu_creation||held->impl_->stage==Impl::Stage::gpu_complete),
            "Resource0 GPU image outside current raw-storage phase/epoch");};
    first();held->impl_->full(operation);first();owned_cpu->require_source_owned(operation);first();
}
std::shared_ptr<const graphics::detail::BnwRendererContextPin>
BnwOriginalNonFontDeviceInitializer::renderer_context_pin_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const BnwOriginalGeneratedResourceImageLoan>& cpu)const {
    const auto held=retain_owner_owned_without_callbacks();
    held->require_gpu_resource0_image_current_owned(operation,cpu,held->impl_->epoch);
    const auto pin=held->impl_->pin;held->require_physical_without_callbacks();return pin;
}
BnwOriginalNonFontDeviceInitializer::CpuPreparation::CpuPreparation(
    std::shared_ptr<const BnwOriginalNonFontDeviceInitializer> initializer,
    std::shared_ptr<const BnwOriginalGeneratedResourceImageLoan> image)
    :initializer_(std::move(initializer)),image_(std::move(image)){}

BnwOriginalNonFontDeviceInitializer::CpuPreparation
BnwOriginalNonFontDeviceInitializer::prepare_cpu_resource0_owned(
    UiScreenGlRenderer& renderer,const BnwWorldViewRendererProjectionLoan& projection,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<NotificationContextGuiPort>& gui,
    const BnwOwnedCameraGuiScreen::Loan& gui_loan,
    const graphics::BnwOwnedCameraRendererWindowLease::Loan& window_loan){
    projection.require_current_without_callbacks();operation.require_render_operation_owned();
    operation.require_same_render_loans_without_callbacks(gui_loan,window_loan);
    operation.require_grid_scene_origin_without_callbacks(projection.scene_);
    require(projection.renderer_==&renderer,"Resource0 initializer wrong actual renderer");
    require(bool(gui),"Resource0 initializer actual GUI owner missing");
    const auto owned_gui=gui;
    gui_loan.require_current();window_loan.require_current();
    owned_gui->require_current_owned(operation);
    const std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window=window_loan.retain_owner();
    const auto pin=window_loan.retain_context_pin_owned();
    require(pin&&pin->identity_.window==window_loan.window()&&pin->identity_.dc==window_loan.dc()&&
        pin->identity_.context==window_loan.wgl_context()&&pin->identity_.owner_thread,
        "Resource0 initializer pin differs from actual lexical Window loan");
    operation.require_same_render_loans_without_callbacks(gui_loan,window_loan);
    const auto bank=operation.selected_provider_startup_bank_owned();
    projection.require_current_without_callbacks();gui_loan.require_current();window_loan.require_current();
    operation.require_current();owned_gui->require_current_owned(operation);
    // Own monotonic init occurrence, minted only by this actual transition.
    // It is deliberately separate from Source serial/world/viewport revisions.
    static std::atomic<std::uint64_t> last_epoch{0};
    auto epoch=last_epoch.load(std::memory_order_relaxed);
    do{require(epoch<UINT64_MAX,"Resource0 initializer epoch exhausted");}
    while(!last_epoch.compare_exchange_weak(epoch,epoch+1,std::memory_order_relaxed));
    ++epoch;
    auto body=std::make_shared<Impl>(renderer,projection.lifetime_,owned_gui,window,pin,bank,
        epoch,operation,operation.operation_serial());
    auto owner=std::shared_ptr<const BnwOriginalNonFontDeviceInitializer>(
        new BnwOriginalNonFontDeviceInitializer(body));body->canonical=owner;
    try{
        owner->require_cpu_resource0_creation_owned(operation);
        const auto image=BnwOriginalGeneratedResourceImageLoan::create_resource0_owned(*owner,operation);
        owner->require_cpu_resource0_creation_owned(operation);image->require_source_owned(operation);
        owner->require_cpu_resource0_creation_owned(operation);
        body->cpu=image;body->stage=Impl::Stage::cpu_complete;
        owner->require_cpu_resource0_image_current_owned(operation,body->epoch);image->require_source_owned(operation);
        projection.require_current_without_callbacks();gui_loan.require_current();window_loan.require_current();
        operation.require_same_render_loans_without_callbacks(gui_loan,window_loan);
        operation.require_grid_scene_origin_without_callbacks(projection.scene_);
        return CpuPreparation(owner,image);
    }catch(...){body->stage=Impl::Stage::failed;throw;}
}
std::shared_ptr<const graphics::detail::BnwNormalResourceGpuImageLoan>
BnwOriginalNonFontDeviceInitializer::upload_resource0_storage_owned(UiScreenGlRenderer& renderer,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const BnwOriginalGeneratedResourceImageLoan>& cpu)const {
    const auto held=retain_owner_owned_without_callbacks();
    held->impl_->same_cpu(cpu);const auto owned_cpu=cpu;
    try{
        held->require_physical_without_callbacks();held->impl_->same_cpu(owned_cpu);
        require(held->impl_->renderer==&renderer&&held->impl_->stage==Impl::Stage::cpu_complete,
            "Resource0 raw storage upload reentered or foreign renderer");
        operation.require_render_operation_owned();
        held->impl_->gpu_creation_serial=operation.operation_serial();
        held->impl_->gpu_creation_operation=&operation;held->impl_->stage=Impl::Stage::gpu_creation;
        held->require_gpu_resource0_creation_owned(operation,owned_cpu);
        const auto image=renderer.upload_original_resource0_storage_owned(held,operation,owned_cpu);
        held->require_gpu_resource0_creation_owned(operation,owned_cpu);
        require(bool(image),"Resource0 genuine upload returned no typed storage");
        renderer.require_original_resource0_storage_current_owned(held,operation,owned_cpu,image);
        held->require_gpu_resource0_creation_owned(operation,owned_cpu);
        held->impl_->gpu=image;held->impl_->stage=Impl::Stage::gpu_complete;
        held->impl_->gpu_creation_operation=nullptr;held->impl_->gpu_creation_serial=0;
        held->require_gpu_resource0_image_current_owned(operation,owned_cpu,held->impl_->epoch);
        renderer.require_original_resource0_storage_current_owned(held,operation,owned_cpu,image);
        held->require_gpu_resource0_image_current_owned(operation,owned_cpu,held->impl_->epoch);
        return image;
    }catch(...){held->impl_->stage=Impl::Stage::failed;held->impl_->gpu.reset();
        held->impl_->gpu_creation_operation=nullptr;held->impl_->gpu_creation_serial=0;throw;}
}
}
