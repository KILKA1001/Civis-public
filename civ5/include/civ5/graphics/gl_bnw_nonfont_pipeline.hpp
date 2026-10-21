#pragma once

#include <memory>
#include "civ5/ui/detail/bnw_normal_nonfont_pipeline.hpp"

namespace civ5::graphics {

// Dedicated nonFont consumer. This source exposes a selected native projection
// only; the native codec, actual GL transfer/draw and restoration are not issued.
class GlBnwNonFontPipeline final {
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    GlBnwNonFontPipeline() = delete;

    class PreparedNativeRecord final {
        friend class GlBnwNonFontPipeline;
        struct Impl;
        const std::shared_ptr<const Impl> impl_;
        explicit PreparedNativeRecord(std::shared_ptr<const Impl>);
        void require_selected_current_owned(
            const runtime::BnwWorldViewSourceOperationLoan&) const;
    public:
        ~PreparedNativeRecord();
        PreparedNativeRecord(const PreparedNativeRecord&) = delete;
        PreparedNativeRecord& operator=(const PreparedNativeRecord&) = delete;
    };

    static std::unique_ptr<const PreparedNativeRecord> prepare_selected_native_record_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const ui::detail::BnwNormalNonFontItemOccurrence::Guard&,
        const ui::detail::BnwNormalNonFontDrainPermission::Guard&,
        const std::shared_ptr<const ui::detail::BnwNormalNonFontMaterialLoan>&);
};

} // namespace civ5::graphics
