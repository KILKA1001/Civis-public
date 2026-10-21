#pragma once
#include <memory>
namespace civ5::runtime {class LuaScriptHost;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationContextPostreadRootCDataReturn;
// Completed destruction of genuine selected-Civis loader temporaries, not
// an Original local-stack/control-block or general Context readiness token.
class NotificationContextLoaderCleanupReturn final {
 friend class ::civ5::runtime::LuaScriptHost;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class NotificationContextPostreadRootCDataReturn;
 struct Impl;
 const std::shared_ptr<const Impl> impl_;
 explicit NotificationContextLoaderCleanupReturn(std::shared_ptr<const Impl>);
public:
 ~NotificationContextLoaderCleanupReturn();
 NotificationContextLoaderCleanupReturn(const NotificationContextLoaderCleanupReturn&)=delete;
 NotificationContextLoaderCleanupReturn& operator=(const NotificationContextLoaderCleanupReturn&)=delete;
};
}
