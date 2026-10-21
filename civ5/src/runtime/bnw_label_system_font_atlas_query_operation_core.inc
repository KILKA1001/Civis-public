// Inside runtime after complete SourceOp::Impl. The original Request is
// accessed once, callback-free, to capture its sealed independent Guard.
std::shared_ptr<const graphics::detail::BnwLabelSystemFontAtlasMetadata>
BnwWorldViewSourceOperationLoan::system_font_atlas_query_owned(
 const ui::detail::BnwLabelSystemFontRequest& request)const{
 const auto request_guard=request.capture_guard_without_callbacks();
 require_render_operation_owned();require_origin_without_callbacks();const auto& p=*impl_;
 try{
  const auto serial=operation_serial();
  const auto* const grid=p.grid;const auto* const gui=p.gui;const auto* const window=p.window;
  const auto first=[&]{require_origin_without_callbacks();request_guard.first_without_callbacks();};
  first();
  if(!grid||!gui||!window)throw ScriptError("System font atlas actual render lenders missing");
  if(request_guard.serial_without_callbacks()!=serial)
   throw ScriptError("System font atlas Request belongs to another actual Source operation");
  request_guard.require_source_owned(*this);first();require_operation(serial);first();
  auto result=grid->query_system_font_atlas_owned(*this,*gui,*window,request_guard);first();
  if(!result)throw ScriptError("System font actual renderer returned no atlas metadata");
  request_guard.require_source_owned(*this);first();require_operation(serial);first();
  result->require_pending_source_owned(*this,request_guard);first();
  require_operation(serial);first();return result;
 }catch(...){p.failed.store(true);p.lease.invalidate();throw;}
}
