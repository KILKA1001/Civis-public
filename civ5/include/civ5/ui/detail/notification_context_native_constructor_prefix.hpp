#pragma once
#include <memory>
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
// Reached40F9C0 prefix ONLY: Base/vtable/XML84/Lua90/Lua98/InstanceA0/XMLAC.
// No RETTHIS, B4/MD5/defaults/maps completion, Lua518, RootRead or children grant.
class NotificationContextNativeConstructorPrefix final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutContextOrdinaryState;
    friend class NotificationContextNativeConstructorReturn;
    friend class NotificationContextNativeLoadNameInputLoan;
    friend class NotificationContextRootReadB4Guard;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationContextNativeConstructorPrefix(std::shared_ptr<Impl>);
    void retire_owned() const noexcept;
public:
    ~NotificationContextNativeConstructorPrefix();
    NotificationContextNativeConstructorPrefix(const NotificationContextNativeConstructorPrefix&)=delete;
    NotificationContextNativeConstructorPrefix& operator=(const NotificationContextNativeConstructorPrefix&)=delete;
};
}
