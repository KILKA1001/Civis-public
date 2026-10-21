//After the Source11 actual canonical receiver borrower. Read-only OWN current
//getter: no layout/virtual4C94 update or cache/default/anchored28 substitution.
float BnwWorldViewControlTree::read_native_ordinary_offset_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,NativeOrdinaryOffsetRead read)const{
 const auto actual=core_;
 const auto pure=[&]{actual->layout_origin();
  const auto&current=actual->source->current_operation_loan();actual->layout_origin();
  require(&current==&operation,"Offset getter offered a retired/different Source operation");
  current.require_origin_without_callbacks();
  (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);actual->layout_origin();};
 try{
  pure();actual->layout_current_owned();pure();operation.require_current();pure();
  const auto id=receiver->impl_->id;
  const auto&state=*Core::node(actual->nodes,id).layout_baseline;
  //Only the SAME genuine six original subtype constructor families have
  //qualified B0/B4 slots. Private current borrower already authenticates
  //their closed actual allocation, subtype, common return and physical CData.
  //Image A43978; Box A3EA80; Button A3C9C0; Label A48DD0;
  //Stack A47A90; Grid A42E00: B0=5B5B50, B4=749A00 for every selected pair.
  //Stack CData allocation/export remains genuinely unavailable in Source11,
  //so it is never admitted here from XML/kind/metadata alone.
  float result;
  switch(read){
   case NativeOrdinaryOffsetRead::virtual_b0_5b5b50:
    //5AF940 JMP[SAME vtable+B0] ->5B5B50 FLD FP32[SAME+70],RET.
    result=state.base_.margin_70_74[0];break;
   case NativeOrdinaryOffsetRead::virtual_b4_749a00:
    //58D230 JMP[SAME vtable+B4] ->749A00 FLD FP32[SAME+74],RET.
    result=state.base_.margin_70_74[1];break;
   case NativeOrdinaryOffsetRead::direct_70_6486f0:
    result=state.base_.margin_70_74[0];break;
   case NativeOrdinaryOffsetRead::direct_74_6486f0:
    result=state.base_.margin_70_74[1];break;
   default:throw runtime::ScriptError("Required genuine current offset getter identity");
  }
  pure();return result; //Actual observed FP32, not a readiness/borrowed pointer.
 }catch(...){actual->failed.store(true);throw;}
}
