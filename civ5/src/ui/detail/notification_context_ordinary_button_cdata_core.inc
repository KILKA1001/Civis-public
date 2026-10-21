// Decomposition of the genuine BankButton1 owned allocation/common return.
// Callers already hold the closed Root factory or actual ordered78 origin
// after Source FIRST. This validator creates no origin or readiness token.
void BnwWorldViewControlTree::require_root_ordinary_button_cdata_owner_without_callbacks(
 const std::shared_ptr<Core>& actual,std::uint64_t id,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,std::uint64_t serial){
 using Base=detail::NotificationLayoutBaseline;
 using Button=detail::NotificationLayoutButtonState;
 using Progress=detail::NotificationLayoutOrdinaryReceiver::Impl::ButtonParserProgress;
 const auto thread=std::this_thread::get_id();
 require(actual&&actual->creator==thread&&actual->facade_alive.load()&&!actual->failed.load()&&
  receiver&&receiver->impl_&&receiver->impl_->creator==thread&&concrete&&id,
  "ordinary Button60A4B0 authentic CData owner retired/foreign FIRST");
 const auto& node=Core::node(actual->nodes,id);
 require(node.alive&&!node.dynamic&&node.layout_baseline&&node.layout_receiver_occurrence&&
  node.full_xml_occurrence&&node.full_xml_occurrence->source.lock()==actual->source,
  "ordinary Button60A4B0 genuine selected native allocation/XML absent");
 const auto& state=*node.layout_baseline;const auto button=state.button_;
 const auto fields=receiver->impl_;const auto& base=state.base_;
 const auto returned=base.common_return_owned?base.common_return_owned->impl_:nullptr;
 require(state.kind_==Base::Kind::button&&state.actual_type_=="Button"&&node.full_xml_occurrence->value.name=="Button"&&
  button&&button.get()==concrete.get()&&fields->button.lock()==button&&base.receiver_owned==receiver&&
  fields->core.lock()==actual&&fields->canonical.lock()==receiver&&fields->occurrence.lock()==node.layout_receiver_occurrence&&
  fields->id==id&&fields->serial==serial&&fields->serial==node.layout_receiver_occurrence->constructor_serial&&
  fields->image.expired()&&fields->grid.expired()&&fields->box.expired()&&fields->context.expired()&&
  fields->standalone_label.expired()&&fields->button_parser_progress==Progress::common_returned&&
  fields->button_void_missing_read_mask==3u&&fields->button_void_current_matches_owned()&&
  fields->button_id58_return.lock()==base.id_58&&button->texture_binding_earned&&
  !button->pending_label_178_&&!button->label_retirement_&&button->child_178_observation,
  "ordinary Button60A4B0 requires actual5AAA20/543910/CommonReturn and both executed Void MISS readers");
 require(returned&&returned->creator==thread&&returned->core.lock()==actual&&
  returned->occurrence.lock()==node.layout_receiver_occurrence&&returned->selected.lock()==node.full_xml_occurrence&&
  returned->receiver.lock()==receiver&&returned->serial==serial&&
  returned->count==node.full_xml_occurrence->value.children.size(),
  "ordinary Button60A4B0 must retain its genuine returning common ordered78 result");
 require_native_ordinary_current_children_without_callbacks(receiver,base.common_return_owned,base.children_78);
 require(fields->button_label_return.lock()==button->label_178_&&
  fields->button_label_factory_return.lock()==button->label_178_factory_return_,
  "ordinary Button60A4B0 owned178 association differs from actual CommonReturn");
 if(*button->child_178_observation==Button::Child178Observation::constructor_null)
  require(!button->label_178_&&!button->label_178_factory_return_,
   "ordinary Button60A4B0 actual String MISS constructorNULL178 changed");
 else require(*button->child_178_observation==Button::Child178Observation::factory_child&&
  button->label_178_&&button->label_178_factory_return_,
  "ordinary Button60A4B0 actual String PRESENT genuine703120 return absent");
 // No virtual30, constructor completion or metatype table is inferred here.
 // Native parent44BF40/Context67373E stores are a separately owned producer.
}
