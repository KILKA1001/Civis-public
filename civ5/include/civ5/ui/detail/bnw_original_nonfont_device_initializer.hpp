#pragma once
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include "civ5/ui/bnw_worldview_renderer_projection_loan.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include "civ5/ui/detail/bnw_original_generated_resource_image_loan.hpp"
#include "civ5/ui/detail/bnw_original_vertex_declaration_setup_loan.hpp"
#include "civ5/ui/detail/bnw_original_vertex_declaration_device_result_loan.hpp"
#include "civ5/ui/detail/bnw_original_resource_sampler_descriptor_loan.hpp"
#include <span>
#include <cstdint>
#include <memory>

namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::graphics::detail {class BnwNormalResourceGpuImageLoan;class BnwRendererContextPin;}
namespace civ5::ui {class UiScreenGlRenderer;}
namespace civ5::ui::detail {
class NotificationContextGuiPort;

// Resource0 creation transition of the Civis backend. Count/slot reservation,
// completed Original manager publication and draw-time sampler state remain
// separate actors. This owner never returns a whole-device readiness flag.
class BnwOriginalNonFontDeviceInitializer final {
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class BnwOriginalVertexDeclarationSetupLoan;
    friend class BnwOriginalVertexDeclarationCreateLoan;
    friend class BnwOriginalResourceSamplerDescriptorLoan;
    struct VertexDeclarationFixedTable;
    struct VertexDeclarationCreationPhase;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit BnwOriginalNonFontDeviceInitializer(std::shared_ptr<Impl>);

    class CpuPreparation final {
        friend class BnwOriginalNonFontDeviceInitializer;
        friend class ::civ5::ui::UiScreenGlRenderer;
        const std::shared_ptr<const BnwOriginalNonFontDeviceInitializer> initializer_;
        const std::shared_ptr<const BnwOriginalGeneratedResourceImageLoan> image_;
        CpuPreparation(std::shared_ptr<const BnwOriginalNonFontDeviceInitializer>,
            std::shared_ptr<const BnwOriginalGeneratedResourceImageLoan>);
    };
    // The real renderer alone enters this producer with its lexical projection,
    // GUI and window loans. An XML/viewport/source serial cannot construct it.
    static CpuPreparation prepare_cpu_resource0_owned(
        ::civ5::ui::UiScreenGlRenderer&,const ::civ5::ui::BnwWorldViewRendererProjectionLoan&,
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<NotificationContextGuiPort>&,
        const ::civ5::ui::BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&);
    std::shared_ptr<const graphics::detail::BnwNormalResourceGpuImageLoan>
        upload_resource0_storage_owned(::civ5::ui::UiScreenGlRenderer&,
            const runtime::BnwWorldViewSourceOperationLoan&,
            const std::shared_ptr<const BnwOriginalGeneratedResourceImageLoan>&)const;
    void retire_owned()const noexcept;

    // Closed executable declaration subset, independent of whole manager readiness.
    std::shared_ptr<const BnwOriginalVertexDeclarationSetupLoan>
        initialize_vertex_declarations_subset_owned(::civ5::ui::UiScreenGlRenderer&,
            const runtime::BnwWorldViewSourceOperationLoan&,
            const std::shared_ptr<const BnwOriginalGeneratedResourceImageLoan>&,
            const std::shared_ptr<const graphics::detail::BnwNormalResourceGpuImageLoan>&)const;
    void retire_vertex_declarations_owned()const noexcept;
    void require_vertex_declaration_source_identity_without_callbacks(
        const runtime::BnwWorldViewSourceOperationLoan&)const;
    // Called immediately after actual Source FULL; genuine native POST, no FULL recursion.
    void require_vertex_declaration_devices_after_source_full_owned(
        const runtime::BnwWorldViewSourceOperationLoan&)const;
    void bind_vertex_declaration_actor_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<BnwOriginalVertexDeclarationSetupLoan::Impl>&)const;
    void require_vertex_declaration_setup_creation_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::shared_ptr<const BnwOriginalVertexDeclarationDeviceResultLoan>
        create_original_vertex_declaration_owned(const runtime::BnwWorldViewSourceOperationLoan&,
            const BnwOriginalVertexDeclarationCreateLoan&)const;
    void require_original_vertex_declaration_result_creation_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,std::uint64_t,
        const BnwOriginalVertexDeclarationCreateLoan&,const BnwOriginalVertexDeclarationDeviceResultLoan&)const;
    void require_vertex_declaration_setup_current_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        std::uint64_t,const BnwOriginalVertexDeclarationSetupLoan&)const;
    void require_original_vertex_declaration_result_current_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,std::uint64_t,
        const BnwOriginalVertexDeclarationSetupLoan&,std::uint32_t,
        const BnwOriginalVertexDeclarationDeviceResultLoan&)const;
    std::span<const BnwOriginalVertexDeclarationElement> vertex_declaration_elements_without_callbacks(
        const runtime::BnwWorldViewSourceOperationLoan&,const BnwOriginalVertexDeclarationCreateLoan&)const;
    std::uint32_t vertex_declaration_slot_without_callbacks(
        const runtime::BnwWorldViewSourceOperationLoan&,const BnwOriginalVertexDeclarationCreateLoan&)const;
    std::uint32_t vertex_declaration_stride_without_callbacks(
        const runtime::BnwWorldViewSourceOperationLoan&,const BnwOriginalVertexDeclarationCreateLoan&)const;
    std::shared_ptr<const graphics::detail::BnwRendererContextPin> vertex_declaration_context_pin_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const BnwOriginalVertexDeclarationCreateLoan&)const;
    // These mandatory actors are NOT earned by the declaration subset.
    void require_manager20da8_sampler_creation_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void require_manager20da8_sampler_descriptor_current_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        std::uint64_t,const BnwOriginalResourceSamplerDescriptorLoan&)const;

public:
    ~BnwOriginalNonFontDeviceInitializer();
    BnwOriginalNonFontDeviceInitializer(const BnwOriginalNonFontDeviceInitializer&)=delete;
    BnwOriginalNonFontDeviceInitializer& operator=(const BnwOriginalNonFontDeviceInitializer&)=delete;
    void require_physical_without_callbacks()const;
    std::shared_ptr<const BnwOriginalNonFontDeviceInitializer> retain_owner_owned_without_callbacks()const;
    std::uint64_t initialization_epoch_without_callbacks()const;
    void require_cpu_resource0_creation_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void require_cpu_resource0_image_current_owned(const runtime::BnwWorldViewSourceOperationLoan&,std::uint64_t)const;

    // These are read-only fences, not phase setters. Only the private upload
    // member admits GPU creation and invokes the Ui upload facade.
    void require_gpu_resource0_creation_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const BnwOriginalGeneratedResourceImageLoan>&)const;
    void require_gpu_resource0_image_current_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const BnwOriginalGeneratedResourceImageLoan>&,std::uint64_t)const;
    std::shared_ptr<const graphics::detail::BnwRendererContextPin> renderer_context_pin_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const BnwOriginalGeneratedResourceImageLoan>&)const;
};
}
