#pragma once
#include <memory>
namespace civ5::runtime {class LuaScriptHost;}
namespace civ5::ui::detail {
// Sole Host issuer after actual profile callback include->traceback->GCrestart,
// protected return, lexical NativeScope DTOR and SAME Source FULL POST.
// This is profile61 completion only, never API registrars/Controls/full Setup.
class NotificationContextLuaProfileReturn final {
    friend class ::civ5::runtime::LuaScriptHost;
    struct Impl;const std::shared_ptr<const Impl> impl_;
    explicit NotificationContextLuaProfileReturn(std::shared_ptr<const Impl>);
public:
    ~NotificationContextLuaProfileReturn();
    NotificationContextLuaProfileReturn(const NotificationContextLuaProfileReturn&)=delete;
    NotificationContextLuaProfileReturn&operator=(const NotificationContextLuaProfileReturn&)=delete;
};
}
