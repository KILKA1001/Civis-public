#pragma once
#include <memory>
namespace civ5::runtime {class LuaScriptHost;}
namespace civ5::ui::detail {
class NotificationContextLoadNameLoan;
// Completion of the real86AC20 resize-list insertion only. It grants neither
// event0/v98 execution nor completed Lua profile/setup/RootRead authority.
class NotificationContextResizeRegistrationReturn final {
    friend class NotificationContextLoadNameLoan;
    friend class ::civ5::runtime::LuaScriptHost;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit NotificationContextResizeRegistrationReturn(std::shared_ptr<const Impl>);
public:
    ~NotificationContextResizeRegistrationReturn();
    NotificationContextResizeRegistrationReturn(const NotificationContextResizeRegistrationReturn&)=delete;
    NotificationContextResizeRegistrationReturn&operator=(const NotificationContextResizeRegistrationReturn&)=delete;
};
}
