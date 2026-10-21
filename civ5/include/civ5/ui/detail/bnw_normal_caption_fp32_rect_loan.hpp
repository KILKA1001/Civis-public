#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include "civ5/ui/detail/bnw_positioned_label_run_loan.hpp"
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;class UiScreenGlRenderer;}
namespace civ5::ui::detail {
class BnwNormalButtonCaptionCall;
class BnwNormalButtonCaptionPaintEmission;
class BnwLabelPaintTextLoan;
class BnwLabelPaintPositionedData;
// Actual preappend FP32 screen words from SAME initialized embedded Run.
// No integer rectangle, clipping, frame, draw or completed-body authority.
class BnwNormalCaptionFp32RectLoan final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class BnwNormalButtonCaptionCall; //Only original weak-control association.
    friend class BnwNormalButtonCaptionPaintEmission;
    friend class BnwLabelPaintTextLoan;
    friend class BnwLabelPaintPositionedData;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    class ConstructionKey final {
        friend class ::civ5::ui::BnwWorldViewControlTree;
        ConstructionKey()=default;
        ConstructionKey(const ConstructionKey&)=delete;
    };
    explicit BnwNormalCaptionFp32RectLoan(const ConstructionKey&,std::shared_ptr<const Impl>);
    void require_current_without_callbacks()const;
    void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void require_same_caption_without_callbacks(const BnwNormalButtonCaptionCall&)const;
    void require_same_initialized_run_without_callbacks(
        const std::shared_ptr<const BnwPositionedLabelRunLoan>&)const;
    // The supplied Guard must be an actual entered Frame/Run capture. A cold
    // Run facade or renewed Guard cannot earn this SAME original association.
    void require_same_entered_run_without_callbacks(const BnwPositionedLabelRunLoan::Guard&)const;
    std::array<std::uint32_t,4> raw_words_without_callbacks()const;
public:
    BnwNormalCaptionFp32RectLoan(const BnwNormalCaptionFp32RectLoan&)=delete;
    BnwNormalCaptionFp32RectLoan&operator=(const BnwNormalCaptionFp32RectLoan&)=delete;
    ~BnwNormalCaptionFp32RectLoan()=default;
};
}
