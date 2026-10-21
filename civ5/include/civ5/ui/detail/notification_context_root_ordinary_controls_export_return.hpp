#pragma once
#include <memory>
namespace civ5::runtime {class LuaScriptHost;class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationContextRootOrdinaryControlsExportLoan;
class NotificationLayoutNativeChildControlsReturn;
// Genuine subtype +7C CData export only; no515120/78/wholev64/attachment grant.
class NotificationContextRootOrdinaryControlsExportReturn final {
 friend class ::civ5::runtime::LuaScriptHost;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 struct Impl;
 const std::shared_ptr<Impl> impl_;
 explicit NotificationContextRootOrdinaryControlsExportReturn(std::shared_ptr<Impl>);
 class Guard final {
  friend class NotificationContextRootOrdinaryControlsExportReturn;
  friend class ::civ5::runtime::LuaScriptHost;
  friend class ::civ5::ui::BnwWorldViewControlTree;
  const std::shared_ptr<Impl> impl_;
  explicit Guard(std::shared_ptr<Impl>);
  void require_current_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
   const NotificationContextRootOrdinaryControlsExportLoan&)const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&,
   const NotificationContextRootOrdinaryControlsExportLoan&)const;
 };
 Guard capture_guard_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
  const NotificationContextRootOrdinaryControlsExportLoan&)const;
 void require_current_after_ordinary_v64_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
  const NotificationLayoutNativeChildControlsReturn&)const;
public:
 ~NotificationContextRootOrdinaryControlsExportReturn();
 NotificationContextRootOrdinaryControlsExportReturn(const NotificationContextRootOrdinaryControlsExportReturn&)=delete;
 NotificationContextRootOrdinaryControlsExportReturn&operator=(const NotificationContextRootOrdinaryControlsExportReturn&)=delete;
};
}
