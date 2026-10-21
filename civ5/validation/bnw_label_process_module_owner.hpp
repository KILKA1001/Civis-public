#pragma once
#include <memory>
#include <cstdint>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class UiScreenGlRenderer;class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class BnwPositionedLabelRunLoan;
class NotificationContextGuiPort;
class NotificationLayoutEmbeddedLabelState;
class NotificationLayoutStandaloneLabelState;
class BnwCompletedFontLoan;
class NotificationLayoutRuntimeDimensions;
class NotificationLayoutEmbeddedLabelFactoryReturn;
// Scoped Civis UI module state. NOT an Original owning-class attribution or
// an observation/import of Original postCRT globals. No public factory,
// default constructor, caller-current callback, mutable DTO or readiness bit.
class BnwLabelProcessModuleOwner final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
    friend class NotificationContextGuiPort;
    friend class NotificationLayoutEmbeddedLabelState;
    friend class NotificationLayoutStandaloneLabelState;
    friend class BnwStandalonePositionedLabelRunLoan;
    friend class BnwNormalLabelPaintInvocation;
    friend class BnwPositionedLabelRunLoan;
    friend class BnwCompletedFontLoan;
    friend class NotificationLayoutRuntimeDimensions;
    friend class NotificationLayoutEmbeddedLabelFactoryReturn;
    struct Impl;
    struct LabelAllocationAttempt;
    const std::shared_ptr<Impl> impl_;
    explicit BnwLabelProcessModuleOwner(std::shared_ptr<Impl>);
    static std::shared_ptr<BnwLabelProcessModuleOwner> create_fresh_owned(
        ::civ5::ui::UiScreenGlRenderer&);
    void require_renderer_origin_owned(const ::civ5::ui::UiScreenGlRenderer&)const;
    void require_current_without_callbacks()const;
    void retire_owned()noexcept;
    // Only the future genuine private child factory may enter this effect.
    // It is NOT an allocated/constructed/parsed/initialized Label receipt.
    // The Source lender must name this actual module through the real Grid
    // renderer provider. No ordinary outer-baseline alias is accepted here.
    void constructor_space_prefix_owned(const runtime::BnwWorldViewSourceOperationLoan&);
    std::shared_ptr<LabelAllocationAttempt> begin_label_fresh_allocation_owned(const runtime::BnwWorldViewSourceOperationLoan&);
    void finish_label_fresh_allocation_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<LabelAllocationAttempt>&,const std::shared_ptr<NotificationLayoutEmbeddedLabelState>&);
    void finish_label_constructor_return_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<LabelAllocationAttempt>&,const std::shared_ptr<NotificationLayoutEmbeddedLabelState>&);
    void require_label_allocation_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<LabelAllocationAttempt>&,const std::shared_ptr<NotificationLayoutEmbeddedLabelState>&,bool)const;
    void require_label_live_allocation_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<LabelAllocationAttempt>&,const std::shared_ptr<NotificationLayoutEmbeddedLabelState>&,
        std::uint64_t actual_child_constructor_serial)const;
    std::shared_ptr<LabelAllocationAttempt> begin_standalone_label_fresh_allocation_owned(const runtime::BnwWorldViewSourceOperationLoan&);
    void finish_label_fresh_allocation_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<LabelAllocationAttempt>&,const std::shared_ptr<NotificationLayoutStandaloneLabelState>&);
    void finish_label_constructor_return_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<LabelAllocationAttempt>&,const std::shared_ptr<NotificationLayoutStandaloneLabelState>&);
    void require_label_allocation_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<LabelAllocationAttempt>&,const std::shared_ptr<NotificationLayoutStandaloneLabelState>&,bool)const;
    void require_standalone_label_live_allocation_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<LabelAllocationAttempt>&,const std::shared_ptr<NotificationLayoutStandaloneLabelState>&,
        std::uint64_t actual_constructor_serial)const;
    void fail_label_allocation_owned(const std::shared_ptr<LabelAllocationAttempt>&)noexcept;
    void require_label_cleanup_identity_owned(const std::shared_ptr<LabelAllocationAttempt>&,
        const std::shared_ptr<NotificationLayoutEmbeddedLabelState>&)const;
    void begin_label_cleanup_owned(const std::shared_ptr<LabelAllocationAttempt>&,
        const std::shared_ptr<NotificationLayoutEmbeddedLabelState>&);
    void finish_label_subtype_cleanup_owned(const std::shared_ptr<LabelAllocationAttempt>&,
        const std::shared_ptr<NotificationLayoutEmbeddedLabelState>&);
    void finish_label_reclamation_owned(const std::shared_ptr<LabelAllocationAttempt>&,
        const std::shared_ptr<NotificationLayoutEmbeddedLabelState>&);
public:
    ~BnwLabelProcessModuleOwner();
    BnwLabelProcessModuleOwner(const BnwLabelProcessModuleOwner&)=delete;
    BnwLabelProcessModuleOwner& operator=(const BnwLabelProcessModuleOwner&)=delete;
};
}
