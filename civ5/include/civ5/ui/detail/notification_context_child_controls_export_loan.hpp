#pragma once
#include <memory>
#include <string_view>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;class LuaScriptHost;class ScriptUiChildSource;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationContextLuaControlsNamedReference;
class NotificationContextChildControlsExportReturn;
class NotificationLayoutOrdinaryReceiver;
class NotificationLayoutContextOrdinaryState;
// Sole Tree issue from real child4FD460 entry and returned529520/v44. A loan
// is an operand for Host's actual v7C export, NEVER proof it has executed.
// The historical owner cannot renew the lexical child-v64 frame after exit.
class NotificationContextChildControlsExportLoan final {
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class ::civ5::runtime::LuaScriptHost;
 friend class NotificationContextChildControlsExportReturn;
 struct Impl;
 const std::shared_ptr<Impl> impl_;
 explicit NotificationContextChildControlsExportLoan(std::shared_ptr<Impl>);
 class Guard final {
  friend class NotificationContextChildControlsExportLoan;
  friend class ::civ5::ui::BnwWorldViewControlTree;
  friend class ::civ5::runtime::LuaScriptHost;
  friend class NotificationContextChildControlsExportReturn;
  const std::shared_ptr<Impl> impl_;
  explicit Guard(std::shared_ptr<Impl>);
  void require_current_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  std::shared_ptr<const NotificationContextLuaControlsNamedReference> parent_controls_without_callbacks()const;
  std::shared_ptr<const runtime::ScriptUiChildSource> child_source_without_callbacks()const;
  std::shared_ptr<const NotificationLayoutOrdinaryReceiver> child_receiver_without_callbacks()const;
  std::shared_ptr<NotificationLayoutContextOrdinaryState> child_context_without_callbacks()const;
  std::shared_ptr<::civ5::ui::BnwWorldViewControlTree> child_tree_without_callbacks()const;
  const char* terminated_id_without_callbacks()const;
  std::u16string_view parent_screen_b4_without_callbacks()const;
 };
 Guard capture_guard_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&)const;
public:
 ~NotificationContextChildControlsExportLoan();
 NotificationContextChildControlsExportLoan(const NotificationContextChildControlsExportLoan&)=delete;
 NotificationContextChildControlsExportLoan&operator=(const NotificationContextChildControlsExportLoan&)=delete;
};
}
