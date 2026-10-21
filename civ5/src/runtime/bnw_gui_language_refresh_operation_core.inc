// Inside runtime after complete actual Source operation Impl.
std::pair<std::string,std::uint8_t> BnwWorldViewSourceOperationLoan::selected_language_extended_font_owned()const{
 require_origin_without_callbacks();require_render_operation_owned();const auto& p=*impl_;
 try{
  const auto serial=operation_serial();std::pair<std::string,std::uint8_t> result;
  {const auto source=p.owner.lock();need(bool(source),"Gui980A actual Source facade retired");
   result=source->selected_language_extended_font_owned(*this);}
  require_origin_without_callbacks();require_operation(serial);require_origin_without_callbacks();return result;
 }catch(...){p.failed.store(true);p.lease.invalidate();throw;}
}
