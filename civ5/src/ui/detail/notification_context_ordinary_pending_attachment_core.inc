// Historical attachment DATA for the genuine ordinary Root child. The
// entered XML cursor may have advanced, or its E/frame may have ended. This
// checker never invokes an expired Read/Factory/NativeGuard or the completed
// Slot initializer gate. It only verifies actual already-issued identities.
void BnwWorldViewControlTree::require_context_root_ordinary_attachment_physical_without_callbacks(
 const std::shared_ptr<const detail::NotificationContextRootChildAttachReturn::Impl>& saved){
 using Attach=detail::NotificationContextRootChildAttachReturn::Impl;
 using Return=detail::NotificationLayoutNativeChildControlsReturn;
 using Factory=detail::NotificationContextRootOrdinaryFactoryReturn;
 using Frame=RootOrdinaryControlsFrame;
 using Base=detail::NotificationLayoutBaseline;
 const auto thread=std::this_thread::get_id();
 require(saved&&saved->creator==thread&&saved->alive.load()&&
  saved->receiver_kind==Attach::ReceiverKind::ordinary_control&&saved->child_id,
  "ordinary Root attachment historical DATA retired/foreign or wrong variant FIRST");
 const auto slot=saved->completed.impl_;
 require(slot&&slot->creator==thread&&slot->alive.load()&&slot->source&&slot->operation,
  "ordinary Root attachment genuine pending Slot DATA absent FIRST");
 // The strong canonical Source owns the current operation lifetime. It
 // compares its actual scope and operation identity BEFORE raw dispatch.
 const auto source=slot->source;
 source->require_operation_origin_without_callbacks(*slot->operation,slot->scope);
 const auto actual=saved->core.lock();const auto canonical_slot=slot->canonical.lock();
 const auto context=saved->context.lock();const auto root=saved->root.lock();
 const auto receiver=saved->child.lock();const auto array=saved->children.lock();
 const auto realm=saved->realm.lock();const auto cursor=slot->actual_cursor.lock();
 const auto read_frame=slot->actual_frame.lock();
 require(actual&&actual==slot->core.lock()&&actual->creator==thread&&actual->facade_alive.load()&&
  !actual->failed.load()&&actual->source==source&&canonical_slot&&canonical_slot->impl_==slot&&
  actual->context_root_read_pending_completion==canonical_slot&&context&&root&&receiver&&array&&realm&&
  cursor&&read_frame&&cursor->core.lock()==actual&&read_frame->core.lock()==actual&&
  cursor->frame.lock()==read_frame&&cursor->operation==slot->operation&&read_frame->operation==slot->operation&&
  cursor->pending_completion.lock()==canonical_slot&&cursor->receiver==root&&read_frame->receiver==root&&
  slot->state==context&&slot->receiver==root&&slot->operation->operation_serial()==slot->serial,
  "ordinary Root attachment SAME canonical Slot/Source/root/returned cursor DATA changed");
 require(context->impl_&&context->impl_->creator==thread&&context->impl_->alive.load()&&
  !context->impl_->failed.load()&&actual->context_ordinary_state==context&&
  context->impl_->native_constructor_return&&context->impl_->native_constructor_return->impl_&&
  context->impl_->native_constructor_return->impl_->alive.load()&&
  !context->impl_->native_constructor_return->impl_->failed.load()&&
  actual->context_layout_baseline&&actual->context_layout_baseline->base_.receiver_owned==root&&
  actual->context_layout_baseline->base_.children_78==array&&array->impl_&&array->impl_->creator==thread&&
  array->impl_->parent.lock()==root&&array->impl_->native_storage_byte20&&
  *array->impl_->native_storage_byte20==0&&saved->append_index<array->impl_->ordered.size()&&
  array->impl_->ordered[saved->append_index]==receiver&&actual->context_realm_identity==realm&&realm->impl_&&
  realm->impl_->creator==thread&&realm->impl_->alive.load()&&!realm->impl_->failed.load()&&
  realm->impl_->context.lock()==actual&&realm->impl_->source.lock()==source&&
  realm->impl_->constructor==actual->context_construction_return,
  "ordinary Root attachment actual Context/parent78/byte20/ContextTHIS identity changed");
 const auto& node=Core::node(actual->nodes,*saved->child_id);
 require(node.alive&&!node.dynamic&&node.layout_baseline&&node.layout_receiver_occurrence&&
  node.full_xml_occurrence&&node.root_context_attach_return&&node.root_context_attach_return->impl_==saved&&
  node.layout_baseline->base_.receiver_owned==receiver&&node.layout_baseline->base_.parent_38.lock()==root&&
  node.layout_baseline->base_.context_realm_3c==realm&&node.layout_baseline->base_.context_3c_observation==
   Base::Base::Context3CObservation::parsed_realm&&receiver->impl_&&receiver->impl_->creator==thread&&
  receiver->impl_->core.lock()==actual&&receiver->impl_->canonical.lock()==receiver&&
  receiver->impl_->occurrence.lock()==node.layout_receiver_occurrence&&receiver->impl_->id==*saved->child_id&&
  receiver->impl_->serial==slot->serial&&node.root_ordinary_controls_return,
  "ordinary Root attachment actual38/67373E3C/member/canonical tagged return changed");
 const auto returned=node.root_ordinary_controls_return;
 const auto body=returned->impl_;
 require(body&&body->origin==Return::Origin::ordinary_control&&body->ordinary&&body->ordinary->frame,
  "ordinary Root attachment cannot replace the genuine typed v64 return with node metadata");
 const auto top=body->ordinary->frame;
 const auto factory=top->factory?top->factory->impl_:nullptr;
 require(factory&&factory->creator==thread&&factory->core.lock()==actual&&factory->root==top->root&&
  factory->read.impl_==top->read.impl_&&factory->operation==slot->operation&&factory->receiver==receiver&&
  factory->realm==realm&&factory->id==*saved->child_id&&factory->subtype==top->subtype&&
  top->factory_guard.impl_==factory&&!top->recursive_origin&&factory->closed&&
  factory->closed->creator==thread&&factory->closed->frame_ended.load(),
  "ordinary Root attachment requires the genuine TOP root factory parser DTOR, not recursive ancestor custody");
 const auto closed=factory->closed;
 require(closed->runtime&&!closed->runtime->alive.load()&&!closed->runtime->failed.load()&&
  closed->runtime->id==factory->id&&closed->runtime->operation==slot->operation&&
  closed->runtime->baseline==&*node.layout_baseline&&closed->occurrence==node.layout_receiver_occurrence&&
  closed->xml==node.full_xml_occurrence&&closed->xml->source.lock()==source&&factory->root_xml&&
  closed->xml->parent.lock()==factory->root_xml&&actual->context_root_parsed_xml_occurrence==factory->root_xml&&
  factory->root_xml->source.lock()==source&&closed->common&&
  node.layout_baseline->base_.common_return_owned==closed->common&&cursor->document&&
  cursor->document->storage&&cursor->document->storage->document&&
  closed->xml->ordinal<cursor->document->storage->document->children.size()&&
  Core::same_layout_xml(closed->xml->value,
   cursor->document->storage->document->children[closed->xml->ordinal]),
  "ordinary Root attachment SAME fresh XML84/CommonReturn/Runtime and Binding DTOR changed");
 // Pure retained traversal. It checks the actual completed children, rather
 // than calling their former lexical Guard at a later root ordinal. The
 // parent backedge stays weak; local strong custody lasts through this read.
 const auto check=[&](const auto& self,const std::shared_ptr<const Return>& value,
  const std::shared_ptr<Frame>& parent,std::size_t ordinal)->void {
  require(value&&value->impl_,"ordinary Root attachment completed78 return absent");
  const auto data=value->impl_;const auto physical=data->ordinary;
  require(data->creator==thread&&data->alive.load()&&data->origin==Return::Origin::ordinary_control&&
   physical&&physical->frame&&data->operation==slot->operation&&data->canonical.lock()==value,
   "ordinary Root attachment retained v64 is not the genuine canonical ordinary return");
  const auto f=physical->frame;
  require(f->creator==thread&&!f->alive.load()&&!f->failed.load()&&f->core.lock()==actual&&
   f->operation==slot->operation&&f->root==top->root&&f->factory==top->factory&&f->realm==realm&&
   f->read.impl_==data->read.impl_&&f->read.impl_==top->read.impl_&&f->root&&
   f->root->impl_==f->read.impl_&&f->read.impl_&&f->read.impl_->evidence==cursor&&
   f->canonical.lock()==value&&f->staged.expired()&&f->traversed_count==f->selected_children.size()&&
   physical->completed_children.size()==f->selected_children.size(),
   "ordinary Root attachment actual returned v64/ordered78 cursor/lexical DTOR changed");
  const auto& current=Core::node(actual->nodes,f->id);
  require(current.alive&&!current.dynamic&&current.layout_baseline&&current.full_xml_occurrence&&
   current.layout_receiver_occurrence&&current.root_ordinary_controls_return==value&&f->receiver&&f->receiver->impl_&&
   f->receiver->impl_->creator==thread&&f->receiver->impl_->core.lock()==actual&&
   f->receiver->impl_->canonical.lock()==f->receiver&&f->receiver->impl_->id==f->id&&
   f->receiver->impl_->serial==slot->serial&&f->receiver->impl_->occurrence.lock()==current.layout_receiver_occurrence,
   "ordinary Root attachment retained v64 receiver is not the actual physical allocation");
  const auto& base=current.layout_baseline->base_;
  const auto common=base.common_return_owned?base.common_return_owned->impl_:nullptr;
  require(base.receiver_owned==f->receiver&&base.id_58==f->id58&&base.children_78==f->children78&&common&&
   common->creator==thread&&common->core.lock()==actual&&common->occurrence.lock()==current.layout_receiver_occurrence&&
   common->selected.lock()==current.full_xml_occurrence&&common->receiver.lock()==f->receiver&&
   common->serial==slot->serial&&common->children.lock()==f->children78&&
   common->count==f->selected_children.size()&&common->count==current.full_xml_occurrence->value.children.size()&&
   (f->children78?(f->children78->impl_&&f->children78->impl_->parent.lock()==f->receiver&&
    f->children78->impl_->ordered==f->selected_children):f->selected_children.empty()),
   "ordinary Root attachment actual returned common/immutable ordered78 member identities changed");
  if(parent){
   const auto origin=f->recursive_origin;
   require(origin&&origin->creator==thread&&origin->parent.lock()==parent&&origin->ordinal==ordinal&&
    ordinal<parent->selected_children.size()&&parent->selected_children[ordinal]==f->receiver&&
    origin->receiver==f->receiver&&origin->id==f->id&&origin->subtype==f->subtype&&origin->typed_state&&
    origin->xml==current.full_xml_occurrence&&origin->occurrence==current.layout_receiver_occurrence&&
    origin->common==base.common_return_owned&&origin->xml->parent.lock()==origin->parent_xml&&
    origin->xml->ordinal==ordinal&&current.native_initial_parent==parent->id&&
    base.parent_38.lock()==parent->receiver&&base.context_realm_3c==origin->inherited_3c&&
    base.context_3c_observation==origin->inherited_observation,
    "ordinary Root attachment recursive return lacks SAME actual completed common/78/parent38 provenance");
   const auto& p=Core::node(actual->nodes,parent->id);
   require(p.full_xml_occurrence==origin->parent_xml&&p.layout_baseline&&
    p.layout_baseline->base_.common_return_owned==origin->parent_common,
    "ordinary Root attachment recursive ancestor is not the actual ordered78 parent common return");
  }else require(f==top&&!f->recursive_origin&&f->id==*saved->child_id&&f->receiver==receiver,
   "ordinary Root attachment rejects recursive custody as top factory origin");
  if(f->subtype==Factory::Subtype::button){
   const std::shared_ptr<const void> button=current.layout_baseline->button_;
   require_root_ordinary_button_cdata_owner_without_callbacks(actual,f->id,f->receiver,button,slot->serial);
  }
  if(f->id_outcome==Frame::IdOutcome::positive_first_byte){
   const auto loan=physical->export_loan?physical->export_loan->impl_:nullptr;
   require(f->id58&&f->id58->length&&f->id58->bytes[0]&&f->id58->length<f->id58->bytes.size()&&
    f->id58->bytes[f->id58->length]=='\0'&&loan&&loan->frame==f&&loan->concrete&&
    loan->parent==context&&loan->controls==cursor->controls&&loan->terminated_id==f->id58->bytes&&
    physical->exported&&f->name588_registration&&f->name588_registration->creator==thread&&
    f->name588_registration->frame.lock()==f&&f->name588_registration->id58==f->id58,
    "ordinary Root attachment retained positiveID +7C/export/515120 returned DATA absent");
  }else require(!physical->export_loan&&!physical->exported&&!f->name588_registration&&
   ((f->id_outcome==Frame::IdOutcome::null_pointer&&!f->id58)||
    (f->id_outcome==Frame::IdOutcome::empty_first_byte&&f->id58&&!f->id58->bytes[0])),
   "ordinary Root attachment no-ID branch claims positive CData/export effects");
  for(std::size_t i=0;i<physical->completed_children.size();++i)self(self,physical->completed_children[i],f,i);
 };
 check(check,returned,std::shared_ptr<Frame>{},0);
 if(node.layout_baseline->kind_==Base::Kind::label){
  const auto label=node.layout_baseline->standalone_label_;
  require(label&&label->impl_&&receiver->impl_->standalone_label.lock()==label,
   "ordinary Root attachment standalone Label actual allocation changed");
  const auto fields=label->impl_;
  const auto native_parent=std::get_if<detail::NotificationLayoutStandaloneLabelState::Impl::ParentReceiverBorrow>(
   &fields->fields[0x38/4]);
  require(native_parent&&native_parent->value.lock()==root,
   "ordinary Root attachment Label native38 and physical Base38 are not the same actual parent");
 }
 source->require_operation_origin_without_callbacks(*slot->operation,slot->scope);
 require(saved->alive.load()&&slot->alive.load(),"ordinary Root attachment retired at final physical DATA boundary");
}
