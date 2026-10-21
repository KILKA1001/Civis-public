#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <optional>
#include "civ5/ui/detail/bnw_label_actual_paint_call_frame.hpp"
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::graphics {class GlBnwSystemFontAtlas;class GlBnwFontShader;}
namespace civ5::ui {class BnwWorldViewControlTree;class UiScreenGlRenderer;}
namespace civ5::ui::detail {
class BnwNormalCaptionFp32RectLoan;
class NotificationLayoutEmbeddedLabelState;
class BnwCompletedFontLoan;
class BnwLabelSystemFontBorrower;
class BnwLabelSystemFontPreparedRun;
class BnwLabelPaintTextLoan;
// Actual completed selected plain Label DATA. This is not a paint callframe,
// an InitLoan extension, a retained7505 preparation or a public readiness flag.
class BnwPositionedLabelRunLoan final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutEmbeddedLabelState;
    friend class BnwNormalCaptionFp32RectLoan; //Name Guard only; reads stay Tree-owned.
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class ::civ5::graphics::GlBnwSystemFontAtlas;
    friend class ::civ5::graphics::GlBnwFontShader;
    friend class BnwLabelPaintTextLoan;
    friend class BnwLabelSystemFontBorrower;
    friend class BnwLabelSystemFontPreparedRun;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit BnwPositionedLabelRunLoan(std::shared_ptr<Impl>);
    struct GlyphData final {
        std::uint32_t byte_begin,byte_count,codepoint,lookup_codepoint,cursor;
        std::array<std::uint32_t,9> bits;
    };
    struct RecordData final {
        std::uint32_t count,begin,byte_count,style,source_discriminator,inline_payload;
        std::array<std::uint32_t,3> packed_colors;
        std::uint8_t newline,style_c8;
        std::int16_t wrap,leading,truncate;
        std::uint32_t flags;
    };
    // Copies only strong physical immutable storage and fresh operation
    // identity before callbacks. Loan destruction retires this guard FIRST.
    class Guard final {
        friend class BnwPositionedLabelRunLoan;
        friend class ::civ5::ui::BnwWorldViewControlTree;
        friend class BnwLabelPaintTextLoan;
        friend class BnwLabelSystemFontBorrower;
        friend class BnwLabelSystemFontPreparedRun;
        friend class ::civ5::ui::UiScreenGlRenderer;
        friend class ::civ5::graphics::GlBnwSystemFontAtlas;
        friend class ::civ5::graphics::GlBnwFontShader;
        const std::shared_ptr<Impl> impl_;
        const runtime::BnwWorldViewSourceOperationLoan*const operation_;
        const std::uint64_t serial_;
        const std::shared_ptr<const ::civ5::ui::BnwWorldViewControlTree> paint_tree_;
        const std::optional<BnwLabelActualPaintCallFrame::Guard> paint_frame_;
        Guard(std::shared_ptr<Impl>,const runtime::BnwWorldViewSourceOperationLoan&,std::uint64_t);
        Guard(std::shared_ptr<Impl>,const runtime::BnwWorldViewSourceOperationLoan&,std::uint64_t,
            std::shared_ptr<const ::civ5::ui::BnwWorldViewControlTree>,BnwLabelActualPaintCallFrame::Guard);
        void require_current_without_callbacks()const;
        void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
        void require_system_font_borrower_without_callbacks(const BnwLabelSystemFontBorrower&)const;
        void require_font_without_callbacks(const BnwCompletedFontLoan&)const;
        //Comparison of an already sealed actual renderer Control target.
        //No lookup/authority can be issued from a raw id.
        void require_actual_control_without_callbacks(const ::civ5::ui::BnwWorldViewControlTree&,std::uint64_t)const;
        std::string_view actual_c4_bytes_without_callbacks()const;
        //Actual initialized7505 positions only. NEVER591020 emitted membership.
        std::span<const GlyphData> positioned_glyphs_without_callbacks()const;
        std::array<std::uint32_t,10> actual_geometry_bits_without_callbacks()const; //10..34 inclusive.
        // Captured physical Label44 Float32 bits, not composited UI alpha.
        std::uint32_t actual_alpha_bits_without_callbacks()const;
        RecordData actual_record_without_callbacks()const;
        std::array<std::uint32_t,3> actual_style_pass_selectors_without_callbacks()const;
        std::array<std::int32_t,3> actual_font_metrics_without_callbacks()const;
        const BnwCompletedFontLoan& actual_font_without_callbacks()const;
        std::shared_ptr<const BnwLabelSystemFontBorrower> actual_system_font_without_callbacks()const;
    };
    Guard capture_guard_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&)const;
    Guard capture_guard_for_paint_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        std::shared_ptr<const ::civ5::ui::BnwWorldViewControlTree>,std::uint64_t,
        const BnwLabelActualPaintCallFrame::Guard&)const;
public:
    ~BnwPositionedLabelRunLoan();
    BnwPositionedLabelRunLoan(const BnwPositionedLabelRunLoan&)=delete;
    BnwPositionedLabelRunLoan& operator=(const BnwPositionedLabelRunLoan&)=delete;
};
}
