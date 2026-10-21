// Outside actual Host::Impl after child export API. The original lexical
// ExportLoan is retired. Only an earned genuine child4FD4D2 capability can
// authenticate this callback-free retained physical result check.
#include "civ5/ui/detail/notification_layout_native_child_controls_return.hpp"
namespace civ5::ui::detail {
void NotificationContextChildControlsExportReturn::require_current_after_child_v64_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan& op,const NotificationLayoutNativeChildControlsReturn& completed)const{
 // Genuine actual Root/currentSource+completed child return FIRST, before
 // dereferencing the old Host/facade/operand. Never call operand_guard here.
 completed.require_export_return_origin_without_callbacks(op,*this);
 const auto saved=impl_;
 if(!saved||saved->creator!=std::this_thread::get_id()||!saved->alive.load()||!saved->actual)
  throw runtime::ScriptError("returned childv64 Host export receipt retired/foreign");
 const auto owner=saved->host.lock();const auto actual=saved->actual;
 if(!owner||actual->host.lock()!=owner||actual->operation!=&op||actual->creator!=saved->creator||
  saved->creator!=owner->ui_thread||!owner->state||owner->failed||owner->calling||owner->end_turn_native_active||
  owner->worldview_native_host||owner->child_controls_export_frame||
  actual->serial!=op.operation_serial()||actual->epoch!=owner->failure_epoch||actual->writes!=owner->failed.writes_||
  actual->uncertainty!=owner->execution_uncertainty||
  actual->stage!=runtime::LuaScriptHost::Impl::NativeChildContextControlsResult::Stage::returned)
  throw runtime::ScriptError("returned childv64 actual Host export result replaced/failed or native scope active");
 const auto lifetime=owner->end_turn_lifetime;
 if(!saved->facade||!lifetime||!lifetime->alive||lifetime->failed||lifetime->host!=saved->facade||
  lifetime->vm!=owner->state||actual->slot>=owner->child_contexts.size()||
  !actual->cdata_creator||actual->cdata_creator!=owner->native_context_cdata_creator||
  !actual->cdata_creator->alive.load()||actual->cdata_creator->host!=owner.get()||
  actual->cdata_creator->creator!=saved->creator||std::strcmp(actual->cdata_creator->type_name,"LuaContext")||
  actual->cdata_creator->push_metatype!=&runtime::LuaScriptHost::Impl::native_child_context_push_existing_metatable||
  std::none_of(owner->child_controls_export_results.begin(),owner->child_controls_export_results.end(),
   [&](const auto& retained){return retained==actual;}))
  throw runtime::ScriptError("returned childv64 genuine Host lifetime/LuaContext creator/result custody changed");
 const auto physical=actual->child.lock();const auto tree=actual->child_tree.lock();
 if(!physical||!tree||physical.get()!=actual->child_cdata||!actual->parent||!actual->child_source||!actual->receiver)
  throw runtime::ScriptError("returned childv64 genuine physical Context/export operands retired");
 completed.require_export_return_operands_without_callbacks(op,*this,*actual->parent,*physical,*tree,
  *actual->child_source,*actual->receiver,std::string_view(actual->id.data(),std::strlen(actual->id.data())));
 const auto& current=owner->child_contexts[actual->slot];
 if(current.retired||!current.state_prepared||current.environment!=actual->environment||
  current.execution_thread!=actual->thread||current.source!=actual->parent_source||
  current.controls_table_pointer!=actual->parent_table||
  !runtime::LuaScriptHost::Impl::native_child_context_export_physical_without_callbacks(*owner,*actual))
  throw runtime::ScriptError("returned childv64 final actual parent Controls/Context CData/metatype changed");
 completed.require_export_return_origin_without_callbacks(op,*this);
}
}
