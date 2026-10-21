// Added to the SAME canonical FIFO loan; never revive its source after pop.
namespace civ5::graphics {
std::int32_t BnwUnitSetupGraphicalQueue::CreatedOccurrenceLoan::read_pending_integer_owned(std::string_view method)const{
 require_current();
 try{
  setup_need(bool(impl_->batch->pending_read),"Created FIFO actual pending native read supplier absent");
  const auto&read=*impl_->batch->pending_read;read.require_same_world_without_callbacks(impl_->world,impl_->generation);
  require_origin_without_callbacks();const auto result=read.read_integer_without_callbacks(method);
  require_origin_without_callbacks();require_current();return result;
 }catch(...){impl_->poison();throw;}
}
bool BnwUnitSetupGraphicalQueue::CreatedOccurrenceLoan::read_pending_boolean_owned(std::string_view method)const{
 require_current();
 try{
  setup_need(bool(impl_->batch->pending_read),"Created FIFO actual pending native boolean supplier absent");
  const auto&read=*impl_->batch->pending_read;read.require_same_world_without_callbacks(impl_->world,impl_->generation);
  require_origin_without_callbacks();const auto result=read.read_boolean_without_callbacks(method);
  require_origin_without_callbacks();require_current();return result;
 }catch(...){impl_->poison();throw;}
}
}
