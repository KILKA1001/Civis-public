#pragma once
#include "civ5/ui/detail/bnw_windowed_renderer_matrix_data_loan.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include "civ5/ui/detail/bnw_label_paint_positioned_data.hpp"
#include "civ5/graphics/detail/bnw_normal_label_draw_record_binding.hpp"
#include "civ5/graphics/gl_bnw_system_font_atlas.hpp"

namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::graphics {class GlBnwFontShader;class GlBnwNonFontPipeline;}
namespace civ5::ui {
class BnwWorldViewControlTree;
class BnwWorldViewControlRenderPublication;
class UiScreenGlRenderer;
}
namespace civ5::ui::detail {
class NotificationContextGuiPort;
class BnwNormalLabelMaterialLoan;
class BnwNormalItemOccurrence;
class BnwNormalQueueDrainPermission;
class BnwNormalQueueRecordCompletion;
class BnwNormalRootBatchExecution;
class BnwNormalGuiBatchPrefix;
class BnwNormalVisibleLabelCall;

// One actual manager Matrix epoch captured BEFORE the genuine root traversal.
// Strong immutable control storage does not renew an active item stamp, Source,
// frame/context/renderer health, root owner membership or a retired batch.
class BnwNormalRootBatchHistory final {
    friend class BnwNormalItemOccurrence;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationContextGuiPort;
    struct Impl;
    class IssuerKey final {
        friend class ::civ5::ui::BnwWorldViewControlTree;
        friend class NotificationContextGuiPort;
        IssuerKey()=default;
        IssuerKey(const IssuerKey&)=delete;
    };
    BnwNormalRootBatchHistory()=delete;
public:
    class Guard final {
        friend class ::civ5::ui::BnwWorldViewControlTree;
        friend class NotificationContextGuiPort;
        friend class BnwNormalLabelMaterialLoan;
        friend class BnwNormalItemOccurrence;
        friend class BnwNormalQueueDrainPermission;
        friend class BnwNormalRootBatchExecution;
        friend class BnwNormalVisibleLabelCall;
        friend class ::civ5::graphics::GlBnwFontShader;
        friend class ::civ5::ui::UiScreenGlRenderer;
        friend class BnwNormalRootPaintCallFrame;
        friend class BnwNormalNonFontCurrentReceiverGuard;
        friend class BnwNormalNonFontPaintInvocation;
        friend class BnwNormalNonFontNativePool;
        friend class BnwNormalNonFontNativeSlots;
        friend class BnwNormalNonFontRecordLoan;
        friend class BnwNormalNonFontSlotLoan;
        friend class BnwNormalNonFontItemOccurrence;
        friend class BnwNormalNonFontMaterialLoan;
        friend class BnwNormalNonFontResourceLoan;
        friend class BnwNormalNonFontResourceBank;
        friend class BnwNormalNonFontOwnReturn;
        friend class BnwNormalNonFontDrainPermission;
        friend class BnwNormalNonFontButtonProducer;
        friend class ::civ5::graphics::GlBnwNonFontPipeline;
        const std::shared_ptr<const Impl> impl_;
        explicit Guard(std::shared_ptr<const Impl>,const IssuerKey&);
        // PURE: OWN control + root/manager membership + renderer/context/frame
        // health + immutable Source identity + Matrix words. Never ACTIVE
        // Matrix::first or Frame::first; no WGL/Source callbacks or allocation.
        void require_physical_current_without_callbacks()const;
        // FULL: fresh SAME Source PRE/POST with the independent PURE cuts.
        void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
        void require_same_root_without_callbacks(const Guard&)const;
        void require_actual_renderer_without_callbacks(const ::civ5::ui::UiScreenGlRenderer&)const;
        // Historical Root-bound SAME Matrix control, never Matrix ACTIVE FIRST.
        void require_actual_matrix_without_callbacks(const BnwWindowedRendererMatrixDataLoan&)const;
        BnwWindowedRendererSharedDescriptorIdentity captured_shared_descriptor_identity_without_callbacks()const;
        std::array<std::uint32_t,16> matrix_words_without_callbacks()const;
        std::array<std::int32_t,2> dimensions_without_callbacks()const;
        std::array<std::uint32_t,2> arithmetic_words_without_callbacks()const;
        std::array<std::uint32_t,2> numeric_controls_without_callbacks()const;
        std::array<std::uint32_t,2> numeric_return_controls_without_callbacks()const;
        std::uint64_t matrix_epoch_without_callbacks()const;
    public:
        Guard(const Guard&)=default;
        Guard&operator=(const Guard&)=delete;
        ~Guard()=default;
    };
};

// A distinct earned renderer/publication/tree/index/control/DATA occurrence.
// Its immutable CPU association survives retirement of its ACTIVE item stamp;
// current root/frame/context/renderer/Source health is checked independently.
class BnwNormalItemOccurrence final {
    friend class BnwNormalRootBatchHistory;
    friend class BnwNormalGuiBatchPrefix;
    friend class BnwLabelPaintPositionedData;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationContextGuiPort;
    struct Impl;
    class IssuerKey final {
        friend class ::civ5::ui::BnwWorldViewControlTree;
        friend class NotificationContextGuiPort;
        IssuerKey()=default;
        IssuerKey(const IssuerKey&)=delete;
    };
    BnwNormalItemOccurrence()=delete;
public:
    class Guard final {
        friend class ::civ5::ui::BnwWorldViewControlTree;
        friend class NotificationContextGuiPort;
        friend class BnwNormalLabelMaterialLoan;
        friend class BnwNormalQueueDrainPermission;
        friend class ::civ5::graphics::GlBnwFontShader;
        friend class ::civ5::ui::UiScreenGlRenderer;
        const std::shared_ptr<const Impl> impl_;
        explicit Guard(std::shared_ptr<const Impl>,const IssuerKey&);
        void require_physical_current_without_callbacks()const;
        void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
        void require_same_root_without_callbacks(const BnwNormalRootBatchHistory::Guard&)const;
        void require_same_item_without_callbacks(const Guard&)const;
        void require_actual_renderer_without_callbacks(const ::civ5::ui::UiScreenGlRenderer&)const;
        void require_actual_publication_without_callbacks(const ::civ5::ui::BnwWorldViewControlRenderPublication&)const;
        void require_actual_control_without_callbacks(const ::civ5::ui::BnwWorldViewControlTree&,std::uint64_t)const;
        // PURE association of OWN DATA control. Does not call DATA's ACTIVE
        // Guard::require_current_without_callbacks or dereference a facade.
        void require_actual_positioned_data_without_callbacks(const BnwLabelPaintPositionedData::Guard&)const;
        ::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::Guard
            captured_normal_draw_records_without_callbacks()const;
        ::civ5::graphics::GlBnwSystemFontAtlas::AcceptedFontPlanes::HistoricalGuard
            historical_font_planes_without_callbacks()const;
        std::shared_ptr<::civ5::graphics::GlBnwSystemFontAtlas>
            actual_font_atlas_owned_without_callbacks()const;
    public:
        Guard(const Guard&)=default;
        Guard&operator=(const Guard&)=delete;
        ~Guard()=default;
    };
};

// Consumer permission is a separate sealed effect: genuine complete Root18,
// actual queue registration, then actual owner manager-drain dispatch. Neither
// registration alone, first Label return, a caller bool nor dirty40 earns it.
class BnwNormalQueueDrainPermission final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationContextGuiPort;
    struct Impl;
    class IssuerKey final {
        friend class ::civ5::ui::BnwWorldViewControlTree;
        friend class NotificationContextGuiPort;
        IssuerKey()=default;
        IssuerKey(const IssuerKey&)=delete;
    };
    BnwNormalQueueDrainPermission()=delete;
public:
    class Guard final {
        friend class ::civ5::ui::BnwWorldViewControlTree;
        friend class NotificationContextGuiPort;
        friend class BnwNormalLabelMaterialLoan;
        friend class BnwNormalQueueRecordCompletion;
        friend class ::civ5::graphics::GlBnwFontShader;
        friend class ::civ5::ui::UiScreenGlRenderer;
        const std::shared_ptr<const Impl> impl_;
        explicit Guard(std::shared_ptr<const Impl>,const IssuerKey&);
        void require_physical_current_without_callbacks()const;
        void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
        void require_same_root_without_callbacks(const BnwNormalRootBatchHistory::Guard&)const;
        void require_actual_renderer_without_callbacks(const ::civ5::ui::UiScreenGlRenderer&)const;
        // Exact actual queued entry/CPU payload/Item association to genuine
        // Graph-produced opaque controls. No integer or generic record ID.
        // Terminal returned-native-failure metadata only; never GPU/stage authority.
        void require_terminal_postreturn_without_callbacks()const;
        void require_terminal_postreturn_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
        void require_terminal_queued_control_without_callbacks(const BnwNormalItemOccurrence::Guard&,
            const ::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity&,
            const ::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt&)const;
        void require_queued_record_without_callbacks(
            const BnwNormalItemOccurrence::Guard&,
            const ::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity&,
            const ::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt&)const;
    public:
        Guard(const Guard&)=default;
        Guard&operator=(const Guard&)=delete;
        ~Guard()=default;
    };
};
// Genuine Graph postreturn result. A consumer callback cannot mint this value.
// GlBnwFontShader creates it only after its actual selected transport/Draw/Rows
// restoration path. Native cursor traversal and Draw outcome stay separate.
class BnwNormalQueueRecordCompletion final {
    friend class ::civ5::graphics::GlBnwFontShader;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationContextGuiPort;
    class ConstructionKey final {
        friend class ::civ5::graphics::GlBnwFontShader;
        ConstructionKey()=default;
        ConstructionKey(const ConstructionKey&)=delete;
    };
    enum class Outcome {actual_draw_issued,zero_count_no_draw,native_failure}; // zero is RESERVED, never issued for this nonempty record.
    const BnwNormalQueueDrainPermission::Guard permission_;
    const BnwNormalItemOccurrence::Guard item_;
    const ::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity record_;
    const ::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt descriptor_;
    const Outcome outcome_;
    explicit BnwNormalQueueRecordCompletion(
        BnwNormalQueueDrainPermission::Guard,BnwNormalItemOccurrence::Guard,
        ::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity,
        ::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt,
        Outcome,const ConstructionKey&);
    void require_current_without_callbacks()const;
    void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void require_same_permission_without_callbacks(const BnwNormalQueueDrainPermission::Guard&)const;
    Outcome actual_outcome_without_callbacks()const;
public:
    BnwNormalQueueRecordCompletion(const BnwNormalQueueRecordCompletion&)=default;
    BnwNormalQueueRecordCompletion&operator=(const BnwNormalQueueRecordCompletion&)=delete;
    ~BnwNormalQueueRecordCompletion()=default;
};

// A genuine pending native56F725 call at one actual ordered78 cursor. This
// grants only staging of that SAME Label; no Root return, queue or Draw grant.
class BnwNormalVisibleLabelCall final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class BnwNormalRootBatchHistory;
    friend class BnwNormalGuiBatchPrefix;
    friend class BnwNormalLabelMaterialLoan;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    class IssuerKey final {
        friend class ::civ5::ui::BnwWorldViewControlTree;
        IssuerKey()=default;
        IssuerKey(const IssuerKey&)=delete;
    };
    explicit BnwNormalVisibleLabelCall(std::shared_ptr<const Impl>,const IssuerKey&);
    void require_current_without_callbacks()const;
    void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void require_same_root_without_callbacks(const BnwNormalRootBatchHistory::Guard&)const;
    void require_actual_publication_without_callbacks(
        const ::civ5::ui::BnwWorldViewControlRenderPublication&)const;
    std::uint64_t actual_control_without_callbacks()const;
    std::size_t actual_ordered_ordinal_without_callbacks()const;
public:
    BnwNormalVisibleLabelCall(const BnwNormalVisibleLabelCall&)=default;
    BnwNormalVisibleLabelCall&operator=(const BnwNormalVisibleLabelCall&)=delete;
    ~BnwNormalVisibleLabelCall()=default;
};

}
