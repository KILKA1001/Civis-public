#pragma once
#include <memory>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;class BnwWorldViewControlRenderPublication;}
namespace civ5::ui::detail {
class BnwNormalRootPaintCallFrame;
class BnwNormalLabelPaintInvocation;
class BnwLabelActualPaintCallFrame;
// Current custody of the actual initialized Context receiver. This value has
// no ordinary Control, index, paint item or Frame/Prefix/draw authority.
class BnwNormalRootContextEntry final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::ui::BnwWorldViewControlRenderPublication;
    friend class BnwNormalRootPaintCallFrame;
    friend class BnwNormalLabelPaintInvocation;
    friend class BnwLabelActualPaintCallFrame;
    struct Impl;
    struct Scope;
    const std::shared_ptr<const Impl> impl_;
    class IssuerKey final {
        friend class ::civ5::ui::BnwWorldViewControlTree;
        IssuerKey()=default;
        IssuerKey(const IssuerKey&)=delete;
    };
    explicit BnwNormalRootContextEntry(std::shared_ptr<const Impl>,const IssuerKey&);
    void require_current_without_callbacks()const;
    void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void require_same_tree_without_callbacks(const ::civ5::ui::BnwWorldViewControlTree&)const;
    void require_same_entry_without_callbacks(const BnwNormalRootContextEntry&)const;
public:
    BnwNormalRootContextEntry(const BnwNormalRootContextEntry&)=default;
    BnwNormalRootContextEntry&operator=(const BnwNormalRootContextEntry&)=delete;
    ~BnwNormalRootContextEntry()=default;
};
}
