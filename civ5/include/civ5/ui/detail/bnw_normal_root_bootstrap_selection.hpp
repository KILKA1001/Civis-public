#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;class BnwWorldViewControlRenderPublication;class UiScreenGlRenderer;}
namespace civ5::ui::detail {
class BnwNormalRootBatchHistory;
class BnwNormalVisibleLabelCall;
// Actual first native ordered78 receiver selection, captured before any Root
// epoch or paint. It is DATA custody only: no Frame/Common/Material/Draw grant.
class BnwNormalRootBootstrapSelection final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::ui::UiScreenGlRenderer;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    class IssuerKey final {
        friend class ::civ5::ui::BnwWorldViewControlTree;
        IssuerKey()=default;
        IssuerKey(const IssuerKey&)=delete;
    };
    explicit BnwNormalRootBootstrapSelection(std::shared_ptr<const Impl>,const IssuerKey&);
    void require_current_without_callbacks()const;
    void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::uint64_t actual_control_without_callbacks()const;
    void require_actual_publication_without_callbacks(const ::civ5::ui::BnwWorldViewControlRenderPublication&)const;
    // Root and call guards are checked by the Tree-owned bridge definition.
    // This overload is declared after complete RootHistory in the Tree API.
public:
    BnwNormalRootBootstrapSelection(const BnwNormalRootBootstrapSelection&)=default;
    BnwNormalRootBootstrapSelection&operator=(const BnwNormalRootBootstrapSelection&)=delete;
    ~BnwNormalRootBootstrapSelection()=default;
};
}
