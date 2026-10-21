#pragma once
#include <memory>
namespace civ5::runtime {class LuaScriptHost;}
namespace civ5::ui {class BnwWorldViewControlTree;namespace detail {
// The actual7B15C0 StateName effect and temporary-ref DTOR only.
// No61EDD0/profile, API registration, Controls table/ref or RootRead grant.
class NotificationContextLuaStateNamePrefixReturn final {
    friend class ::civ5::runtime::LuaScriptHost;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    struct Impl;
    const std::shared_ptr<const Impl> impl_;
    explicit NotificationContextLuaStateNamePrefixReturn(std::shared_ptr<const Impl>);
public:
    ~NotificationContextLuaStateNamePrefixReturn();
    NotificationContextLuaStateNamePrefixReturn(const NotificationContextLuaStateNamePrefixReturn&)=delete;
    NotificationContextLuaStateNamePrefixReturn&operator=(const NotificationContextLuaStateNamePrefixReturn&)=delete;
};
}}
