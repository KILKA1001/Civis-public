#pragma once
#include <memory>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
// Captured under the existing COLD NameLoan guard BEFORE RootRead entry.
// Own lifetime/current physical DATA only. RootRead's distinct lexical guard
// must separately earn SAME Core creator/facade/failed/busy/rootrole and FULL
// current Source. This guard neither enters RootRead nor accepts a busy bool.
class NotificationContextRootReadB4Guard final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationContextRootReadB4Guard(std::shared_ptr<Impl>);
    void require_data_current_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void retire_owned()const noexcept;
public:
    ~NotificationContextRootReadB4Guard();
    NotificationContextRootReadB4Guard(const NotificationContextRootReadB4Guard&)=delete;
    NotificationContextRootReadB4Guard&operator=(const NotificationContextRootReadB4Guard&)=delete;
};
}
