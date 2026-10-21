#pragma once
#include <memory>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationLayoutOrdinaryReceiver;
class NotificationLayoutContextRealmRef;
// Genuine lexical673730 attachment STORE frame, not an initializer return.
// Sole Tree issuer after SAME RootRead/factory/v64 completion. The two actual
// stages44BF40 inherited3C and67373E ContextTHIS3C are internal, never caller
// enum/bool input. NULL DATA can occur only in the earned inherited stage.
class NotificationContextRootChildAttachStoreLoan final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationContextRootChildAttachStoreLoan(std::shared_ptr<Impl>);
    class Guard final {
        friend class NotificationContextRootChildAttachStoreLoan;
        friend class ::civ5::ui::BnwWorldViewControlTree;
        const std::shared_ptr<Impl> impl_;
        explicit Guard(std::shared_ptr<Impl>);
        void require_current_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
            const NotificationLayoutOrdinaryReceiver&)const;
        void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&,
            const NotificationLayoutOrdinaryReceiver&)const;
        std::shared_ptr<const NotificationLayoutContextRealmRef> value_3c_without_callbacks()const;
    };
    Guard capture_guard_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
        const NotificationLayoutOrdinaryReceiver&)const;
public:
    ~NotificationContextRootChildAttachStoreLoan();
    NotificationContextRootChildAttachStoreLoan(const NotificationContextRootChildAttachStoreLoan&)=delete;
    NotificationContextRootChildAttachStoreLoan&operator=(const NotificationContextRootChildAttachStoreLoan&)=delete;
};
}
