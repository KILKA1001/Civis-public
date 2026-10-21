// A completed ordinary v64 authenticates retained actual output. This path
// NEVER invokes the expired ordinary ExportLoan::Guard after lexical DTOR.
#include "civ5/ui/detail/notification_layout_native_child_controls_return.hpp"
namespace civ5::ui::detail {
void NotificationContextRootOrdinaryControlsExportReturn::require_current_after_ordinary_v64_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan& op,const NotificationLayoutNativeChildControlsReturn& completed)const {
 completed.require_ordinary_export_return_origin_without_callbacks(op,*this);
 const auto saved=impl_;
 if(!saved||saved->creator!=std::this_thread::get_id()||!saved->alive.load()||!saved->actual)
  throw runtime::ScriptError("ordinary returned v64 Host export receipt retired/foreign");
 const auto owner=saved->host.lock();const auto actual=saved->actual;
 if(!owner||actual->host.lock()!=owner||actual->operation!=&op||actual->creator!=saved->creator||
  saved->creator!=owner->ui_thread||!owner->state||owner->failed||owner->calling||owner->end_turn_native_active||
  owner->worldview_native_host||owner->root_ordinary_controls_export_frame||
  actual->serial!=op.operation_serial()||actual->epoch!=owner->failure_epoch||actual->writes!=owner->failed.writes_||
  actual->uncertainty!=owner->execution_uncertainty||
  actual->stage!=runtime::LuaScriptHost::Impl::NativeRootOrdinaryControlsResult::Stage::returned||!actual->cdata_creator)
  throw runtime::ScriptError("ordinary returned v64 actual Host result failed/replaced or protected scope active");
 const auto lifetime=owner->end_turn_lifetime;const auto index=static_cast<std::size_t>(actual->cdata_creator->subtype);
 if(!saved->facade||!lifetime||!lifetime->alive||lifetime->failed||lifetime->host!=saved->facade||
  lifetime->vm!=owner->state||actual->slot>=owner->child_contexts.size()||index>=6||
  actual->cdata_creator!=owner->native_ordinary_cdata_creators[index]||!actual->cdata_creator->alive.load()||
  actual->cdata_creator->host!=owner.get()||actual->cdata_creator->creator!=saved->creator||
  !actual->cdata_creator->push_metatype||
  std::none_of(owner->root_ordinary_controls_export_results.begin(),owner->root_ordinary_controls_export_results.end(),
   [&](const auto& retained){return retained==actual;}))
  throw runtime::ScriptError("ordinary returned v64 true subtype creator/result/facade lifetime changed");
 const auto tree=actual->tree.lock();
 if(!tree||!actual->concrete||actual->concrete.get()!=actual->cdata||!actual->parent||!actual->receiver)
  throw runtime::ScriptError("ordinary returned v64 genuine CData/receiver/Root tree retired");
 completed.require_ordinary_export_return_operands_without_callbacks(op,*this,*actual->parent,actual->cdata,
  *tree,*actual->receiver,std::string_view(actual->id.data(),std::strlen(actual->id.data())),actual->cdata_creator->type_name);
 const auto& current=owner->child_contexts[actual->slot];
 if(current.retired||!current.state_prepared||current.environment!=actual->environment||
  current.execution_thread!=actual->thread||current.source!=actual->parent_source||
  current.controls_table_pointer!=actual->parent_table||
  !runtime::LuaScriptHost::Impl::native_ordinary_export_physical_without_callbacks(*owner,*actual))
  throw runtime::ScriptError("ordinary returned v64 final actual table/CData/global types/metatype changed");
 completed.require_ordinary_export_return_origin_without_callbacks(op,*this);
}
}
