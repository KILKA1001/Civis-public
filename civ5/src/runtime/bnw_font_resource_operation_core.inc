std::array<assets::CivisLoadedResource,3> BnwWorldViewSourceOperationLoan::selected_style_resources_owned()const{
 require_render_operation_owned();require_origin_without_callbacks();const auto& p=*impl_;
 try{
  const auto source=p.owner.lock();need(bool(source),"Style resource current Source missing");
  const auto serial=operation_serial();const auto result=source->selected_style_resources_owned(*this);
  require_operation(serial);require_origin_without_callbacks();return result;
 }catch(...){p.failed.store(true);p.lease.invalidate();throw;}
}
std::unique_ptr<detail::BnwFontResourceLoan> BnwWorldViewSourceOperationLoan::font_resource_loan_owned(std::string request)const{
 require_render_operation_owned();require_origin_without_callbacks();const auto& p=*impl_;
 try{
  const auto source=p.owner.lock();need(bool(source),"Font resource current Source missing");
  const auto serial=operation_serial();
  auto loan=source->font_resource_loan_owned(*this,std::move(request));
  require_operation(serial);require_origin_without_callbacks();return loan;
 }catch(...){p.failed.store(true);p.lease.invalidate();throw;}
}
