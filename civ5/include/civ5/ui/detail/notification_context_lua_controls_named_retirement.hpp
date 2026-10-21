#pragma once
#include <memory>
namespace civ5::runtime {class LuaScriptHost;class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;namespace detail {
class NotificationContextLuaStatePreparation;class NotificationContextRootDefaultFieldsReturn;
class NotificationContextLuaControlsNamedReference;
// Actual local RootRead LuaRef8F8820 return, not wholeRead/Setup authority.
class NotificationContextLuaControlsNamedRetirement final {
 friend class ::civ5::runtime::LuaScriptHost;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 struct Impl;
 const std::shared_ptr<const Impl> impl_;
 explicit NotificationContextLuaControlsNamedRetirement(std::shared_ptr<const Impl>);
 void require_actual_nil_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&,
  const NotificationContextLuaControlsNamedReference&)const;
 void require_actual_nil_owned(const runtime::BnwWorldViewSourceOperationLoan&,
  const NotificationContextLuaControlsNamedReference&)const;
public:
 ~NotificationContextLuaControlsNamedRetirement();
 NotificationContextLuaControlsNamedRetirement(const NotificationContextLuaControlsNamedRetirement&)=delete;
 NotificationContextLuaControlsNamedRetirement&operator=(const NotificationContextLuaControlsNamedRetirement&)=delete;
};
}}
