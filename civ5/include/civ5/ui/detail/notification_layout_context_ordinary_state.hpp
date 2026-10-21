#pragma once
#include <memory>
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationLayoutParentReceiver;
// OWN normalized constructor/Base projection, never full40F9C0/read/init/vector.
class NotificationLayoutContextOrdinaryState final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutParentReceiver;
    friend class NotificationContextNativeConstructorPrefix;
    friend class NotificationContextNativeConstructorReturn;
    friend class NotificationContextNativeLoadNameInputLoan;
    friend class NotificationContextLoadNameLoan;
    friend class NotificationContextRootReadB4Guard;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationLayoutContextOrdinaryState(std::shared_ptr<Impl>);
    void retire_owned()const noexcept;
public:
    ~NotificationLayoutContextOrdinaryState();
    NotificationLayoutContextOrdinaryState(const NotificationLayoutContextOrdinaryState&)=delete;
    NotificationLayoutContextOrdinaryState& operator=(const NotificationLayoutContextOrdinaryState&)=delete;
};
}
