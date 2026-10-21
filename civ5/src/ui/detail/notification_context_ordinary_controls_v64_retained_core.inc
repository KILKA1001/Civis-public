// After the complete ordinary frame, closed operand fields and actual v64
// result payload. Only genuine returned capabilities can invoke these checks.
void BnwWorldViewControlTree::require_root_ordinary_child_v64_export_current_without_callbacks(
 const detail::NotificationLayoutNativeChildControlsReturn::Impl& returned){
 require_root_ordinary_child_v64_current_without_callbacks(returned);
 const auto& physical=*returned.ordinary;const auto frame=physical.frame;
 if(frame->id_outcome==RootOrdinaryControlsFrame::IdOutcome::positive_first_byte){
  const auto canonical=returned.canonical.lock();require(canonical&&physical.exported,
   "ordinary returned v64 authentic positive export absent");
  physical.exported->require_current_after_ordinary_v64_without_callbacks(*returned.operation,*canonical);
 }
 for(const auto& child:physical.completed_children){
  require(child&&child->impl_,"ordinary returned v64 actual completed child retired");
  require_root_ordinary_child_v64_export_current_without_callbacks(*child->impl_);
 }
 require_root_ordinary_child_v64_current_without_callbacks(returned);
}
void detail::NotificationLayoutNativeChildControlsReturn::require_ordinary_export_return_origin_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const NotificationContextRootOrdinaryControlsExportReturn& exported)const {
 BnwWorldViewControlTree::require_root_ordinary_v64_export_origin_without_callbacks(*this,operation,exported);
}
void BnwWorldViewControlTree::require_root_ordinary_v64_export_origin_without_callbacks(
 const detail::NotificationLayoutNativeChildControlsReturn& returned,
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const detail::NotificationContextRootOrdinaryControlsExportReturn& exported){
 const auto body=returned.impl_;require(body!=nullptr,"ordinary returned v64 sealed Host origin absent");
 require_root_ordinary_child_v64_current_without_callbacks(*body);
 require(body->canonical.lock().get()==&returned&&body->operation==&operation&&body->ordinary->exported.get()==&exported,
  "ordinary returned v64 offered different genuine export/Source/canonical result");
}
void detail::NotificationLayoutNativeChildControlsReturn::require_ordinary_export_return_operands_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const NotificationContextRootOrdinaryControlsExportReturn& exported,
 const NotificationContextLuaControlsNamedReference& controls,const void* concrete,
 const BnwWorldViewControlTree& tree,const NotificationLayoutOrdinaryReceiver& receiver,
 std::string_view id,std::string_view type_name)const {
 BnwWorldViewControlTree::require_root_ordinary_v64_export_operands_without_callbacks(*this,operation,exported,
  controls,concrete,tree,receiver,id,type_name);
}
void BnwWorldViewControlTree::require_root_ordinary_v64_export_operands_without_callbacks(
 const detail::NotificationLayoutNativeChildControlsReturn& returned,
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const detail::NotificationContextRootOrdinaryControlsExportReturn& exported,
 const detail::NotificationContextLuaControlsNamedReference& controls,const void* concrete,
 const BnwWorldViewControlTree& tree,const detail::NotificationLayoutOrdinaryReceiver& receiver,
 std::string_view id,std::string_view type_name){
 using Loan=detail::NotificationContextRootOrdinaryControlsExportLoan;
 require_root_ordinary_v64_export_origin_without_callbacks(returned,operation,exported);
 const auto body=returned.impl_;const auto& physical=*body->ordinary;
 const auto frame=physical.frame;const auto actual=frame->core.lock();const auto loan=physical.export_loan->impl_;
 require(actual&&tree.core_==actual&&loan&&loan->frame==frame&&loan->controls.get()==&controls&&
  loan->concrete.get()==concrete&&frame->receiver.get()==&receiver&&
  id==std::string_view(loan->terminated_id.data(),std::strlen(loan->terminated_id.data())),
  "ordinary returned v64 offered foreign physical CData/parent/receiver/ID");
 const char* expected=nullptr;
 switch(loan->subtype){
  case Loan::NativeSubtype::image:expected="ImageControl";break;
  case Loan::NativeSubtype::box:expected="ColorBoxControl";break;
  case Loan::NativeSubtype::button:expected="ButtonControl";break;
  case Loan::NativeSubtype::label:expected="TextControl";break;
  case Loan::NativeSubtype::stack:expected="StackControl";break;
  case Loan::NativeSubtype::grid:expected="GridControl";break;
 }
 require(expected&&type_name==expected,"ordinary returned v64 genuine native typename was substituted");
 require_root_ordinary_controls_export_operand_physical_without_callbacks(loan,operation,false);
 require_root_ordinary_v64_export_origin_without_callbacks(returned,operation,exported);
}
