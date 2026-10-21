// After the complete tagged v64 payload and ordinary frame. A Root parent
// attach consumes the genuine returned ROOT factory, never ancestor custody
// from a recursive78 child or a plain common parser result.
void BnwWorldViewControlTree::require_context_root_attach_returned_factory_identity_without_callbacks(
 const detail::NotificationContextRootReadChildrenLoan::Guard& read,
 const detail::NotificationLayoutNativeChildControlsReturn::Guard& controls){
 using Return=detail::NotificationLayoutNativeChildControlsReturn;
 read.require_current_without_callbacks();controls.require_current_without_callbacks();
 const auto body=controls.impl_;require(body!=nullptr,"Root parent38 actual tagged v64 return missing");
 if(body->origin==Return::Origin::distinct_context){
  read.require_child_factory_return_identity_without_callbacks(controls.actual_factory_return_without_callbacks());
 }else{
  require(body->origin==Return::Origin::ordinary_control&&body->ordinary&&body->ordinary->frame,
   "Root parent38 cannot obtain ordinary origin from a cold declaration");
  const auto frame=body->ordinary->frame;
  require(!frame->recursive_origin&&frame->read.impl_==read.impl_&&body->read.impl_==read.impl_,
   "Root parent38 rejects recursive ancestor custody or another lexical RootRead");
  const auto& factory=controls.actual_ordinary_factory_return_without_callbacks();
  require(frame->factory.get()==&factory,"Root parent38 offered a different actual root factory return");
  frame->factory_guard.require_current_without_callbacks();
  frame->factory_guard.require_same_root_read_without_callbacks(*frame->root);
  require(&frame->factory_guard.actual_declaration_without_callbacks()==&read.actual_declaration_without_callbacks()&&
   frame->factory_guard.actual_receiver_without_callbacks()==controls.actual_receiver_without_callbacks()&&
   frame->factory_guard.actual_context_realm_without_callbacks()==read.actual_context_realm_without_callbacks(),
   "Root parent38 actual returned root XML/receiver/realm changed");
 }
 read.require_current_without_callbacks();controls.require_current_without_callbacks();
}
