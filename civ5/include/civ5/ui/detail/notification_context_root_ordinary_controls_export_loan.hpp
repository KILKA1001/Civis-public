#pragma once
#include <memory>
#include <string_view>
namespace civ5::runtime {class LuaScriptHost;class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationContextLuaControlsNamedReference;
class NotificationLayoutOrdinaryReceiver;
class NotificationContextRootOrdinaryControlsExportReturn;
// Issued only after the actual Root ordinary factory returned. CData custody
// is derived from its concrete owned native state, never a caller void pointer.
class NotificationContextRootOrdinaryControlsExportLoan final {
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class ::civ5::runtime::LuaScriptHost;
 friend class NotificationContextRootOrdinaryControlsExportReturn;
 enum class NativeSubtype {image,box,button,label,stack,grid};
 struct Impl;
 const std::shared_ptr<Impl> impl_;
 explicit NotificationContextRootOrdinaryControlsExportLoan(std::shared_ptr<Impl>);
 class Guard final {
  friend class NotificationContextRootOrdinaryControlsExportLoan;
  friend class NotificationContextRootOrdinaryControlsExportReturn;
  friend class ::civ5::ui::BnwWorldViewControlTree;
  friend class ::civ5::runtime::LuaScriptHost;
  const std::shared_ptr<Impl> impl_;
  explicit Guard(std::shared_ptr<Impl>);
  void require_current_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  std::shared_ptr<const NotificationContextLuaControlsNamedReference> parent_controls_without_callbacks()const;
  std::shared_ptr<const NotificationLayoutOrdinaryReceiver> receiver_without_callbacks()const;
  std::shared_ptr<const void> concrete_cdata_owner_without_callbacks()const;
  std::shared_ptr<::civ5::ui::BnwWorldViewControlTree> tree_without_callbacks()const;
  NativeSubtype subtype_without_callbacks()const;
  const char* terminated_id_without_callbacks()const;
  std::u16string_view parent_screen_b4_without_callbacks()const;
 };
 Guard capture_guard_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&)const;
public:
 ~NotificationContextRootOrdinaryControlsExportLoan();
 NotificationContextRootOrdinaryControlsExportLoan(const NotificationContextRootOrdinaryControlsExportLoan&)=delete;
 NotificationContextRootOrdinaryControlsExportLoan&operator=(const NotificationContextRootOrdinaryControlsExportLoan&)=delete;
};
}
