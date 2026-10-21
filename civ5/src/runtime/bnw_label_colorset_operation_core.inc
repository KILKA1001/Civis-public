std::shared_ptr<const assets::CivisResourceBankOwner> BnwWorldViewSourceOperationLoan::selected_provider_startup_bank_owned()const{
 require_origin_without_callbacks();require_render_operation_owned();const auto& p=*impl_;
 try{
  const auto serial=operation_serial();std::shared_ptr<const assets::CivisResourceBankOwner> result;
  {const auto source=p.owner.lock();need(bool(source),"Selected startup actual Source facade retired");
   result=source->selected_provider_startup_bank_owned(*this);}
  require_origin_without_callbacks();require_operation(serial);require_origin_without_callbacks();return result;
 }catch(...){p.failed.store(true);p.lease.invalidate();throw;}
}
// Inside runtime after complete Source operation Impl. Metadata quote only;
// actual Gui/Context/Realm must separately admit every ColorSet lookup.
std::array<assets::CivisLoadedResource,2> BnwWorldViewSourceOperationLoan::color_atlas_resources_owned()const{
 require_origin_without_callbacks();require_render_operation_owned();const auto& p=*impl_;
 try{
  const auto serial=operation_serial();std::array<assets::CivisLoadedResource,2> result;
  {const auto source=p.owner.lock();need(bool(source),"ColorSet actual Source facade retired");
   result=source->color_atlas_resources_owned(*this);}
  require_origin_without_callbacks();require_operation(serial);require_origin_without_callbacks();return result;
 }catch(...){p.failed.store(true);p.lease.invalidate();throw;}
}
