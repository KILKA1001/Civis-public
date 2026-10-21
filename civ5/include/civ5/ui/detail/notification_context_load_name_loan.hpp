#pragma once
#include <memory>
#include <string>
#include "civ5/ui/detail/notification_context_resize_registration_return.hpp"
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;class LuaScriptHost;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationContextLuaStatePreparation;
class NotificationContextLoadNameLoan;
// The real source-issued argument of Context731370. No producer is supplied
// until the actual Gui loader input/lexical frame has been qualified. A source
// stem, XML path or metadata name cannot construct this closed operand.
class NotificationContextNativeLoadNameInputLoan final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationContextLoadNameLoan;
    friend class NotificationContextRootReadB4Guard;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationContextNativeLoadNameInputLoan(std::shared_ptr<Impl>);
    void retire_owned()const noexcept;
public:
    ~NotificationContextNativeLoadNameInputLoan();
    NotificationContextNativeLoadNameInputLoan(const NotificationContextNativeLoadNameInputLoan&)=delete;
    NotificationContextNativeLoadNameInputLoan&operator=(const NotificationContextNativeLoadNameInputLoan&)=delete;
};
// Borrower of the actual B4 field AFTER an executed81FEC0-equivalent store.
// It retains physical DATA and a live exact input/load lexical frame. It is
// neither full slot10 completion nor a RootRead, LuaRef or Controls authority.
class NotificationContextLoadNameLoan final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::runtime::LuaScriptHost;
    friend class NotificationContextRootReadB4Guard;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit NotificationContextLoadNameLoan(std::shared_ptr<Impl>);
    void retire_owned()const noexcept;
    void require_current_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const NotificationContextLuaStatePreparation>&)const;
    void require_current_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const NotificationContextLuaStatePreparation>&)const;
    std::shared_ptr<const NotificationContextResizeRegistrationReturn> register_context_resize_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const std::shared_ptr<const NotificationContextLuaStatePreparation>&)const;
    void require_context_resize_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const NotificationContextLuaStatePreparation>&,
        const std::shared_ptr<const NotificationContextResizeRegistrationReturn>&)const;
    std::u16string b4_utf16_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const NotificationContextLuaStatePreparation>&)const;
public:
    ~NotificationContextLoadNameLoan();
    NotificationContextLoadNameLoan(const NotificationContextLoadNameLoan&)=delete;
    NotificationContextLoadNameLoan&operator=(const NotificationContextLoadNameLoan&)=delete;
};
}
