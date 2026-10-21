// After the real ordinary lexical frame. Only its authentic completed common
// return plus exact ordered78 membership can issue a recursive child operand.
void BnwWorldViewControlTree::require_recursive_root_ordinary_origin_without_callbacks(
 const RecursiveOrdinaryOrigin& origin){
 using Factory=detail::NotificationContextRootOrdinaryFactoryReturn;
 require(origin.creator==std::this_thread::get_id(),"ordinary78 recursive origin foreign FIRST");
 const auto parent=origin.parent.lock();require(parent!=nullptr,"ordinary78 actual parent frame retired");
 // The parent may genuinely have returned. This checks physical retained
// ancestry; it never captures or reactivates its expired lexical loan.
 parent->physical_without_callbacks(parent->alive.load());
 const auto actual=parent->core.lock();require(actual&&origin.parent_common&&origin.common&&origin.parent_xml&&
  origin.xml&&origin.occurrence&&origin.receiver&&origin.typed_state&&origin.id&&parent->children78&&parent->children78->impl_,
  "ordinary78 recursive actual parent/common/occurrence custody missing");
 const auto& parent_node=Core::node(actual->nodes,parent->id);
 require(parent_node.layout_baseline&&parent_node.full_xml_occurrence==origin.parent_xml&&
  parent_node.layout_baseline->base_.common_return_owned==origin.parent_common&&
  origin.parent_common->impl_&&origin.parent_common->impl_->receiver.lock()==parent->receiver&&
  origin.parent_common->impl_->children.lock()==parent->children78&&
  origin.parent_common->impl_->selected.lock()==origin.parent_xml&&
  origin.parent_common->impl_->count==parent->selected_children.size()&&
  origin.ordinal<parent->selected_children.size()&&parent->selected_children[origin.ordinal]==origin.receiver&&
  parent->children78->impl_->ordered[origin.ordinal]==origin.receiver&&origin.xml->parent.lock()==origin.parent_xml&&
  origin.xml->ordinal==origin.ordinal,
  "ordinary78 child must come from SAME actual completed parent factory ordered member");
 const auto& node=Core::node(actual->nodes,origin.id);
 const auto receiver=origin.receiver->impl_;const auto native=origin.common->impl_;
 require(node.alive&&!node.dynamic&&node.native_initial_parent==parent->id&&node.layout_baseline&&
  node.full_xml_occurrence==origin.xml&&node.layout_receiver_occurrence==origin.occurrence&&
  !node.layout_baseline->parser_runtime_dimensions_&&receiver&&receiver->canonical.lock()==origin.receiver&&
  receiver->core.lock()==actual&&receiver->occurrence.lock()==origin.occurrence&&receiver->id==origin.id&&
  receiver->serial==parent->operation->operation_serial()&&node.layout_baseline->base_.receiver_owned==origin.receiver&&
  node.layout_baseline->base_.common_return_owned==origin.common&&native&&
  native->creator==origin.creator&&native->core.lock()==actual&&native->occurrence.lock()==origin.occurrence&&
  native->selected.lock()==origin.xml&&native->receiver.lock()==origin.receiver&&
  native->serial==receiver->serial&&native->children.lock()==node.layout_baseline->base_.children_78&&
  node.layout_baseline->base_.children_78&&node.layout_baseline->base_.children_78->impl_&&
  node.layout_baseline->base_.children_78->impl_->parent.lock()==origin.receiver&&
  node.layout_baseline->base_.children_78->impl_->ordered.size()==native->count&&
  native->count==origin.xml->value.children.size()&&(node.layout_baseline->base_.flags_40&8u)&&
  node.layout_baseline->base_.parent_38.lock()==parent->receiver&&
  node.layout_baseline->base_.context_realm_3c==origin.inherited_3c&&
  node.layout_baseline->base_.context_3c_observation==origin.inherited_observation,
  "ordinary78 SAME native child v44/CommonReturn/DTOR/parent38/inherited3C changed");
 // The parent common factory issued its return ONLY after each actual child
 // Runtime and Binding DTOR and44BF40. Its retained typed member is the issuer.
 const auto& state=*node.layout_baseline;
 switch(origin.subtype){
  case Factory::Subtype::image:require(origin.xml->value.name=="Image"&&state.actual_type_=="Image"&&
   state.image_&&receiver->image.lock()==state.image_&&state.image_.get()==origin.typed_state.get(),
   "ordinary78 genuine Image child state changed");break;
  case Factory::Subtype::box:require(origin.xml->value.name=="Box"&&state.actual_type_=="Box"&&
   state.box_&&receiver->box.lock()==state.box_&&state.box_.get()==origin.typed_state.get(),
   "ordinary78 genuine Box child state changed");break;
  case Factory::Subtype::label:require(origin.xml->value.name=="Label"&&state.actual_type_=="Label"&&
   state.standalone_label_&&receiver->standalone_label.lock()==state.standalone_label_&&
   state.standalone_label_.get()==origin.typed_state.get(),
   "ordinary78 genuine standalone Label child state changed");break;
  case Factory::Subtype::grid:require(origin.xml->value.name=="Grid"&&state.actual_type_=="Grid"&&
   state.grid_&&receiver->grid.lock()==state.grid_&&state.grid_.get()==origin.typed_state.get(),
   "ordinary78 genuine Grid child state changed");break;
  case Factory::Subtype::button:
   require_root_ordinary_button_cdata_owner_without_callbacks(actual,origin.id,origin.receiver,origin.typed_state,
    parent->operation->operation_serial());break;
  case Factory::Subtype::stack:
   throw runtime::ScriptError("ordinary78 needs genuine Stack returning owned allocation/common factory, not a node tag");
 }
 parent->physical_without_callbacks(parent->alive.load());
}

std::shared_ptr<const detail::NotificationLayoutNativeChildControlsReturn>
BnwWorldViewControlTree::consume_root_ordinary_78_child_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const std::shared_ptr<RootOrdinaryControlsFrame>& parent,
 std::size_t ordinal){
 using Frame=RootOrdinaryControlsFrame;using Factory=detail::NotificationContextRootOrdinaryFactoryReturn;
 require(parent!=nullptr,"ordinary4FD4C1 requires its actual entered parent frame");parent->full();
 require(parent->operation==&operation&&parent->children_outcome==Frame::ChildrenOutcome::positive_array&&
  ordinal==parent->traversed_count&&ordinal<parent->selected_children.size(),
  "ordinary4FD4C1 requires exact next actual ordered78 member");
 const auto actual=parent->core.lock();require(actual&&actual==core_,"ordinary78 offered a foreign Core");
 const auto receiver=parent->selected_children[ordinal];
 require(receiver&&receiver->impl_,"ordinary78 actual completed child receiver absent");
 const auto id=receiver->impl_->id;const auto& parent_node=Core::node(actual->nodes,parent->id);
 const auto& node=Core::node(actual->nodes,id);
 require(parent_node.layout_baseline&&node.layout_baseline&&node.layout_baseline->base_.receiver_owned==receiver&&
  node.layout_baseline->base_.common_return_owned&&node.full_xml_occurrence&&node.layout_receiver_occurrence,
  "ordinary78 child is not a returned native common factory allocation");
 const auto parent_common=parent_node.layout_baseline->base_.common_return_owned;
 const auto common=node.layout_baseline->base_.common_return_owned;
 const auto xml=node.full_xml_occurrence;const auto parent_xml=parent_node.full_xml_occurrence;
 const auto occurrence=node.layout_receiver_occurrence;
 // These observations are qualified by native returned operands below. They
 // do not themselves issue a factory or infer a NULL/skip from a cold tag.
 Factory::Subtype subtype;std::shared_ptr<const void> concrete;
 if(node.layout_baseline->image_&&receiver->impl_->image.lock()==node.layout_baseline->image_){
  subtype=Factory::Subtype::image;concrete=node.layout_baseline->image_;}
 else if(node.layout_baseline->box_&&receiver->impl_->box.lock()==node.layout_baseline->box_){
  subtype=Factory::Subtype::box;concrete=node.layout_baseline->box_;}
 else if(node.layout_baseline->standalone_label_&&receiver->impl_->standalone_label.lock()==node.layout_baseline->standalone_label_)
  {subtype=Factory::Subtype::label;concrete=node.layout_baseline->standalone_label_;}
 else if(node.layout_baseline->grid_&&receiver->impl_->grid.lock()==node.layout_baseline->grid_){
  subtype=Factory::Subtype::grid;concrete=node.layout_baseline->grid_;}
 else if(node.layout_baseline->button_&&receiver->impl_->button.lock()==node.layout_baseline->button_){
  subtype=Factory::Subtype::button;concrete=node.layout_baseline->button_;
  require_root_ordinary_button_cdata_owner_without_callbacks(actual,id,receiver,concrete,operation.operation_serial());}
 else throw runtime::ScriptError("ordinary78 actual concrete child factory producer is required before v64");
 const auto inherited=node.layout_baseline->base_.context_realm_3c;
 const auto observation=node.layout_baseline->base_.context_3c_observation;
 require(inherited==parent_node.layout_baseline->base_.context_realm_3c&&
  observation==parent_node.layout_baseline->base_.context_3c_observation,
  "ordinary78 entered child does not carry genuine44BF40 parent3C inheritance");
 auto origin=std::make_shared<const RecursiveOrdinaryOrigin>(parent,parent_common,common,parent_xml,xml,occurrence,
  receiver,concrete,inherited,observation,subtype,id,ordinal);parent->full();origin->first();
 const auto identity=Core::node(actual->nodes,id).layout_baseline->base_.id_58;
 const auto children=Core::node(actual->nodes,id).layout_baseline->base_.children_78;
 const auto io=!identity?Frame::IdOutcome::null_pointer:!identity->bytes[0]?
  Frame::IdOutcome::empty_first_byte:Frame::IdOutcome::positive_first_byte;
 const auto co=!children?Frame::ChildrenOutcome::null_pointer:children->impl_->ordered.empty()?
  Frame::ChildrenOutcome::empty_array:Frame::ChildrenOutcome::positive_array;
 std::vector<std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>> selected;
 const auto count=children?children->impl_->ordered.size():0;
 selected.reserve(count);parent->full();origin->first();
 if(children)for(const auto& member:children->impl_->ordered)selected.push_back(member);
 parent->full();origin->first();
 require(!Core::node(actual->nodes,id).root_ordinary_controls_return,"ordinary78 actual member v64 replay");
 auto frame=std::make_shared<Frame>(actual,parent->root,parent->read,parent->factory,parent->factory_guard,
  subtype,id,receiver,parent->realm,identity,children,io,co,&operation,std::move(selected),origin);
 parent->full();origin->first();frame->full();
 auto returned=run_root_ordinary_controls_frame_owned(operation,frame);
 parent->full();origin->first();require(returned&&returned->impl_->ordinary&&
  returned->impl_->ordinary->frame==frame&&frame->recursive_origin==origin,
  "ordinary4FD4C1 child virtual64 did not return its actual canonical native result");
 const auto guard=returned->capture_guard_for_attach_owned(operation,*parent->root);
 guard.require_source_owned(operation);parent->full();origin->first();return returned;
}
