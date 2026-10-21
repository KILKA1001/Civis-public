// Actual Source weak control is passed without a callback-spanning facade pin.
ui::detail::BnwLabelSystemFontLanguageData BnwWorldViewSourceOperationLoan::system_font_language_data_owned()const{
 require_origin_without_callbacks();require_render_operation_owned();const auto& p=*impl_;
 try{
  const auto serial=operation_serial();const auto source=p.owner;
  auto result=BnwSelectedWorldViewContextOwner::system_font_language_data_from_owned_weak(source,*this);
  require_origin_without_callbacks();require_operation(serial);require_origin_without_callbacks();return result;
 }catch(...){p.failed.store(true);p.lease.invalidate();throw;}
}
