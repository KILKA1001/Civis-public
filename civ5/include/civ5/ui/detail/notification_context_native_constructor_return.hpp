#pragma once
#include <memory>
#include "civ5/ui/detail/notification_context_native_wide_string_value.hpp"
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationLayoutContextOrdinaryState;
class NotificationContextLoadNameLoan;
// OWN completed portable40F9C0 constructor fields, never518/setup/RootRead.
// Native pooled cleanup remains HOLD and quarantines its physical field storage.
class NotificationContextNativeConstructorReturn final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutContextOrdinaryState;
    friend class NotificationContextLoadNameLoan;
    friend class NotificationContextNativeLoadNameInputLoan;
    friend class NotificationContextRootReadB4Guard;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationContextNativeConstructorReturn(std::shared_ptr<Impl>);
    void retire_owned(std::shared_ptr<const NotificationContextNativeWideStringValue>)const noexcept;
public:
    ~NotificationContextNativeConstructorReturn();
    NotificationContextNativeConstructorReturn(const NotificationContextNativeConstructorReturn&)=delete;
    NotificationContextNativeConstructorReturn&operator=(const NotificationContextNativeConstructorReturn&)=delete;
};
}
