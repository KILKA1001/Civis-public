#pragma once
#include <memory>
#include <cstdint>
#include <array>
#include <string_view>
#include "civ5/ui/detail/bnw_label_actual_paint_call_frame.hpp"
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;class UiScreenGlRenderer;}
namespace civ5::ui::detail {
class BnwLabelPaintTextLoan;
class BnwNormalLabelPaintInvocation;class BnwNormalLabelPaintEmission;
class NotificationLayoutStandaloneLabelState;class BnwCompletedFontLoan;class BnwLabelSystemFontBorrower;
//Distinct completed standalone DATA, never Embedded Button178 or an actual
//paint frame/GL grant. The Tree publisher alone seals after real View DTOR.
class BnwStandalonePositionedLabelRunLoan final {
    friend class NotificationLayoutStandaloneLabelState;
    friend class BnwNormalLabelPaintInvocation;
    friend class BnwLabelPaintTextLoan;
    friend class BnwNormalLabelPaintEmission;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit BnwStandalonePositionedLabelRunLoan(std::shared_ptr<Impl>);
    void require_current_data_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    friend class ::civ5::ui::UiScreenGlRenderer;
    class Guard final {
        friend class BnwStandalonePositionedLabelRunLoan;
        friend class BnwNormalLabelPaintEmission;
        friend class BnwLabelPaintTextLoan;
        friend class ::civ5::ui::BnwWorldViewControlTree;
        friend class ::civ5::ui::UiScreenGlRenderer;
        const std::shared_ptr<Impl> impl_;
        const runtime::BnwWorldViewSourceOperationLoan*const operation_;
        const std::uint64_t serial_;
        const std::shared_ptr<const ::civ5::ui::BnwWorldViewControlTree> tree_;
        const BnwLabelActualPaintCallFrame::Guard frame_;
        Guard(std::shared_ptr<Impl>,const runtime::BnwWorldViewSourceOperationLoan&,std::uint64_t,
            std::shared_ptr<const ::civ5::ui::BnwWorldViewControlTree>,BnwLabelActualPaintCallFrame::Guard);
        void require_current_without_callbacks()const;
        void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
        void require_actual_control_without_callbacks(const ::civ5::ui::BnwWorldViewControlTree&,std::uint64_t)const;
        void require_system_font_borrower_without_callbacks(const BnwLabelSystemFontBorrower&)const;
        std::string_view actual_c4_bytes_without_callbacks()const;
        struct RecordData final {
            std::uint32_t count,begin,byte_count,style,source_discriminator,inline_payload;
            std::array<std::uint32_t,3> packed_colors;
            std::uint8_t newline;std::int16_t leading,truncate;std::uint32_t flags;
        };
        RecordData actual_record_without_callbacks()const;
        std::array<std::uint32_t,3> actual_style_pass_selectors_without_callbacks()const;
        std::array<std::int32_t,3> actual_font_metrics_without_callbacks()const;
        std::uint32_t actual_field_44_bits_without_callbacks()const;
        std::shared_ptr<const BnwLabelSystemFontBorrower> actual_system_font_without_callbacks()const;
        const BnwCompletedFontLoan& actual_font_without_callbacks()const;
    };
    Guard capture_guard_for_paint_frame_guard_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        std::shared_ptr<const ::civ5::ui::BnwWorldViewControlTree>,std::uint64_t,
        const BnwLabelActualPaintCallFrame::Guard&)const;
    Guard capture_guard_for_paint_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        std::shared_ptr<const ::civ5::ui::BnwWorldViewControlTree>,std::uint64_t,
        const BnwLabelActualPaintCallFrame&)const;

public:
    ~BnwStandalonePositionedLabelRunLoan();
    BnwStandalonePositionedLabelRunLoan(const BnwStandalonePositionedLabelRunLoan&)=delete;
    BnwStandalonePositionedLabelRunLoan&operator=(const BnwStandalonePositionedLabelRunLoan&)=delete;
};
}
