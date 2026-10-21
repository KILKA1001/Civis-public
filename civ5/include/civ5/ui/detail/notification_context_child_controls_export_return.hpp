#pragma once
#include <memory>
namespace civ5::runtime {class LuaScriptHost;class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationContextChildControlsExportLoan;
class NotificationLayoutNativeChildControlsReturn;
// Genuine Host664FB0/6768B0 return only. It does not complete515120, the78
// recursion, wholev64, parent attachment, child initialization or RootRead.
class NotificationContextChildControlsExportReturn final {
 friend class ::civ5::runtime::LuaScriptHost;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 struct Impl;
 const std::shared_ptr<Impl> impl_;
 explicit NotificationContextChildControlsExportReturn(std::shared_ptr<Impl>);
 class Guard final {
  friend class NotificationContextChildControlsExportReturn;
  friend class ::civ5::ui::BnwWorldViewControlTree;
  friend class ::civ5::runtime::LuaScriptHost;
  const std::shared_ptr<Impl> impl_;
  explicit Guard(std::shared_ptr<Impl>);
  void require_current_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
   const NotificationContextChildControlsExportLoan&)const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&,
   const NotificationContextChildControlsExportLoan&)const;
 };
 Guard capture_guard_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
  const NotificationContextChildControlsExportLoan&)const;
 void require_current_after_child_v64_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
  const NotificationLayoutNativeChildControlsReturn&)const;
public:
 ~NotificationContextChildControlsExportReturn();
 NotificationContextChildControlsExportReturn(const NotificationContextChildControlsExportReturn&)=delete;
 NotificationContextChildControlsExportReturn&operator=(const NotificationContextChildControlsExportReturn&)=delete;
};
}
