#pragma once
#include <memory>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationLayoutOrdinaryReceiver;
class NotificationContextRootChildAttachReturn;
// Pending DATA slot captured during actual RootRead; NOT a completed return.
// Its required provider publishes a separate genuine Read-return stamp only
// AFTER every child, temporary XML/string/LuaRef cleanup and full SourcePOST.
// Capturing it before that return gives no initializer authority.
class NotificationContextRootReadCompletionSlot final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationContextRootChildAttachReturn;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationContextRootReadCompletionSlot(std::shared_ptr<Impl>);
    class Guard final {
        friend class NotificationContextRootReadCompletionSlot;
        friend class NotificationContextRootChildAttachReturn;
        friend class ::civ5::ui::BnwWorldViewControlTree;
        const std::shared_ptr<Impl> impl_;
        explicit Guard(std::shared_ptr<Impl>);
        void require_completed_current_without_callbacks()const;
        void require_actual_root_receiver_without_callbacks(const NotificationLayoutOrdinaryReceiver&)const;
        void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    };
    Guard capture_pending_data_guard_without_callbacks()const;
public:
    ~NotificationContextRootReadCompletionSlot();
    NotificationContextRootReadCompletionSlot(const NotificationContextRootReadCompletionSlot&)=delete;
    NotificationContextRootReadCompletionSlot&operator=(const NotificationContextRootReadCompletionSlot&)=delete;
};
}
