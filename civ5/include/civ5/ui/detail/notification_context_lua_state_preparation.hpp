#pragma once
#include <memory>
namespace civ5::runtime {class LuaScriptHost;}
namespace civ5::ui {class BnwWorldViewControlTree;namespace detail {
// Earned only by the protected actual Host coroutine/environment constructor.
// This is neither a loaded-script receipt nor a complete Context return.
class NotificationContextLuaStatePreparation final {
    friend class ::civ5::runtime::LuaScriptHost;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit NotificationContextLuaStatePreparation(std::shared_ptr<const Impl>);
public:
    ~NotificationContextLuaStatePreparation();
    NotificationContextLuaStatePreparation(const NotificationContextLuaStatePreparation&)=delete;
    NotificationContextLuaStatePreparation& operator=(const NotificationContextLuaStatePreparation&)=delete;
};
}}
