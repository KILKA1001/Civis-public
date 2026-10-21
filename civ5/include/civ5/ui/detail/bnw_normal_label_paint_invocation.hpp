#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include "civ5/ui/detail/bnw_label_actual_paint_call_frame.hpp"
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;class BnwWorldViewControlRenderPublication;class UiScreenGlRenderer;struct ComposedUiFrame;struct WorldViewPaint;}
namespace civ5::ui::detail {
class BnwStandalonePositionedLabelRunLoan;
class BnwNormalLabelPaintEmission;
// Closed port normal vslot18 invocation from a genuine standalone receiver.
// This is DATA; no Original runtime/draw/material/ancestor/default grant.
class BnwNormalLabelPaintInvocation final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::ui::BnwWorldViewControlRenderPublication;
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class BnwNormalLabelPaintEmission;
    struct Impl;const std::shared_ptr<Impl> impl_;
    explicit BnwNormalLabelPaintInvocation(std::shared_ptr<Impl>);
    // Only Ui's actual append producer can mint this lexical key. Its source
    // callsite, after f.paints.push_back, is a separate mandatory Graph splice.
    class EmissionIssuerKey final {
        friend class ::civ5::ui::UiScreenGlRenderer;
        friend class BnwNormalLabelPaintInvocation;
        const ComposedUiFrame*const frame_;
        const WorldViewPaint*const item_;
        const std::size_t index_,count_;
        EmissionIssuerKey(const ComposedUiFrame&,std::size_t);
        EmissionIssuerKey(const EmissionIssuerKey&)=delete;
        EmissionIssuerKey&operator=(const EmissionIssuerKey&)=delete;
    };
    std::shared_ptr<const BnwNormalLabelPaintEmission> bind_emitted_index_owned(
        const EmissionIssuerKey&,const runtime::BnwWorldViewSourceOperationLoan&)const;
public:
    ~BnwNormalLabelPaintInvocation();
    BnwNormalLabelPaintInvocation(const BnwNormalLabelPaintInvocation&)=delete;
    BnwNormalLabelPaintInvocation&operator=(const BnwNormalLabelPaintInvocation&)=delete;
};
class BnwNormalLabelPaintEmission final {
    friend class BnwNormalLabelPaintInvocation;
    friend class ::civ5::ui::BnwWorldViewControlRenderPublication;
    friend class ::civ5::ui::UiScreenGlRenderer;
    struct Impl;const std::shared_ptr<Impl> impl_;
    explicit BnwNormalLabelPaintEmission(std::shared_ptr<Impl>);
    const ComposedUiFrame& frame_origin_for_publication_owned()const;
    void require_publication_owned(const ::civ5::ui::BnwWorldViewControlRenderPublication&,
        const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::shared_ptr<const BnwStandalonePositionedLabelRunLoan> run_for_actual_item_owned(
        const ::civ5::ui::BnwWorldViewControlRenderPublication&,std::size_t,
        const runtime::BnwWorldViewSourceOperationLoan&,const BnwLabelActualPaintCallFrame&)const;
public:
    ~BnwNormalLabelPaintEmission();
    BnwNormalLabelPaintEmission(const BnwNormalLabelPaintEmission&)=delete;
    BnwNormalLabelPaintEmission&operator=(const BnwNormalLabelPaintEmission&)=delete;
};
}
