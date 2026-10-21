#pragma once
#include <memory>
namespace civ5::runtime {class LuaScriptHost;}
namespace civ5::ui::detail {
// Genuine partial4EC473 GCSTOP->534B50 environment->selected config operand.
// Retained actual master debug namespace was initialized by protected luaopen_debug.
// Does NOT issue restricted copier/global library/full61/Controls/RootRead return.
class NotificationContextLuaDebugConfigPrefix final {
    friend class ::civ5::runtime::LuaScriptHost;
    struct Impl;const std::shared_ptr<const Impl> impl_;
    explicit NotificationContextLuaDebugConfigPrefix(std::shared_ptr<const Impl>);
public:
    ~NotificationContextLuaDebugConfigPrefix();
    NotificationContextLuaDebugConfigPrefix(const NotificationContextLuaDebugConfigPrefix&)=delete;
    NotificationContextLuaDebugConfigPrefix&operator=(const NotificationContextLuaDebugConfigPrefix&)=delete;
};
}
