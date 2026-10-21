#pragma once
#include <memory>
#include <cstdint>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationLayoutOrdinaryReceiver;
class NotificationLayoutStandaloneLabelState;
// ROOT-OWNED REQUIRED DEFINITION. This declaration supplies no issuer. The
// genuine Root529520/Controls-v64/673730/44BF40/78append/67373E producer must
// finish before Root can construct it. No Common/Factory/metadata conversion.
class NotificationContextRootChildAttachReturn final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutStandaloneLabelState;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit NotificationContextRootChildAttachReturn(std::shared_ptr<const Impl>);
    class Guard final {
        friend class ::civ5::ui::BnwWorldViewControlTree; // actual closed lookup/publisher only
        friend class NotificationContextRootChildAttachReturn;
        friend class NotificationLayoutStandaloneLabelState;
        const std::shared_ptr<const Impl> impl_;
        explicit Guard(std::shared_ptr<const Impl>);
        void require_current_without_callbacks()const;
        void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
        void require_child_receiver_owned(const NotificationLayoutOrdinaryReceiver&)const;
    };
    Guard capture_current_initializer_parent_guard_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const NotificationLayoutOrdinaryReceiver&)const;
    static std::shared_ptr<const NotificationContextRootChildAttachReturn> require_actual_current_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const NotificationLayoutOrdinaryReceiver&);
public:
    ~NotificationContextRootChildAttachReturn();
    NotificationContextRootChildAttachReturn(const NotificationContextRootChildAttachReturn&)=delete;
    NotificationContextRootChildAttachReturn&operator=(const NotificationContextRootChildAttachReturn&)=delete;
};
}
