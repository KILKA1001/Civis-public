//Source18: direct native common storage, without renewing a lexical v64 loan.
//4B9E80 has no own reset stores. Its six qualified v54 entries recurse only.
std::uint64_t BnwWorldViewControlTree::native_ordinary_common_receiver_id_without_callbacks(
 const std::shared_ptr<Core>& actual,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver){
 require(actual&&receiver&&receiver->impl_,"Native branch current common receiver absent");
 const auto fields=receiver->impl_;const auto occurrence=fields->occurrence.lock();
 require(fields->creator==std::this_thread::get_id()&&fields->core.lock()==actual&&
  fields->canonical.lock()==receiver&&occurrence&&fields->id&&fields->serial&&
  occurrence->id==fields->id&&occurrence->constructor_serial==fields->serial,
  "Native branch SAME canonical common allocation/occurrence changed");
 const auto& node=Core::node(actual->nodes,fields->id);
 require(node.alive&&!node.dynamic&&node.layout_baseline&&node.full_xml_occurrence&&
  node.full_xml_occurrence->source.lock()==actual->source&&node.layout_receiver_occurrence==occurrence&&
  node.layout_baseline->base_.receiver_owned==receiver&&node.layout_baseline->parsed_source_serial_&&
  *node.layout_baseline->parsed_source_serial_==fields->serial,
  "Native branch physical canonical parser/source association replaced");
 const auto& base=node.layout_baseline->base_;
 const auto common=base.common_return_owned?base.common_return_owned->impl_:nullptr;
 require(common&&common->creator==std::this_thread::get_id()&&common->core.lock()==actual&&
  common->occurrence.lock()==occurrence&&common->selected.lock()==node.full_xml_occurrence&&
  common->receiver.lock()==receiver&&common->serial==fields->serial&&
  common->count==node.full_xml_occurrence->value.children.size(),
  "Native branch requires actual returned common factory allocation");
 require_native_ordinary_current_children_without_callbacks(receiver,base.common_return_owned,base.children_78);
 return fields->id;
}

void BnwWorldViewControlTree::branch_reset_native_ordinary_animation54_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete){
 using Base=detail::NotificationLayoutBaseline;
 const auto actual=core_;
 const auto pure=[&](bool busy){actual->layout_origin(busy);
  const auto& current=actual->source->current_operation_loan();actual->layout_origin(busy);
  require(&current==&operation,"BranchResetAnimation SAME Source operation replaced");
  current.require_origin_without_callbacks();
  (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);actual->layout_origin(busy);};
 try{
  pure(false);actual->layout_current_owned();pure(false);operation.require_current();pure(false);
  actual->busy=true;struct Exit{bool& value;~Exit(){value=false;}}exit{actual->busy};pure(true);
  std::vector<const detail::NotificationLayoutOrdinaryReceiver*> path;
  const auto flags=[&](const auto& current){
   pure(true);const auto id=native_ordinary_common_receiver_id_without_callbacks(actual,current);
   const auto& state=*Core::node(actual->nodes,id).layout_baseline;
   if(state.kind_==Base::Kind::label){const auto label=state.standalone_label_?state.standalone_label_->impl_:nullptr;
    require(bool(label),"BranchResetAnimation actual Label40 storage missing");label->first();
    const auto* word=std::get_if<std::uint32_t>(&label->fields[0x40u/4u]);
    require(word&&*word==state.base_.flags_40,"BranchResetAnimation physical Label40/Base alias changed");
    return *word;
   }
   return state.base_.flags_40;
  };
  const auto visit=[&](auto&& self,const auto& current)->void{
   pure(true);const auto id=native_ordinary_common_receiver_id_without_callbacks(actual,current);
   require(std::find(path.begin(),path.end(),current.get())==path.end(),
    "BranchResetAnimation native common child graph contains a cycle");
   path.push_back(current.get());
   //Fresh actual child vptr54 dispatch. No constructor type name alone issues it.
   const auto& state=*Core::node(actual->nodes,id).layout_baseline;const auto fields=current->impl_;
   switch(state.kind_){
    case Base::Kind::image:require(state.actual_type_=="Image"&&state.image_&&fields->image.lock()==state.image_,"BranchResetAnimation actual Image v54 receiver changed");break;
    case Base::Kind::box:require(state.actual_type_=="Box"&&state.box_&&fields->box.lock()==state.box_,"BranchResetAnimation actual Box v54 receiver changed");break;
    case Base::Kind::button:require(state.actual_type_=="Button"&&state.button_&&fields->button.lock()==state.button_&&
     fields->button_parser_progress==detail::NotificationLayoutOrdinaryReceiver::Impl::ButtonParserProgress::common_returned,
     "BranchResetAnimation actual Button v54/CommonReturn missing");break;
    case Base::Kind::grid:require(state.actual_type_=="Grid"&&state.grid_&&!state.grid_->failed&&fields->grid.lock()==state.grid_,"BranchResetAnimation actual Grid v54 receiver changed");break;
    case Base::Kind::label:require(state.actual_type_=="Label"&&state.standalone_label_&&fields->standalone_label.lock()==state.standalone_label_,"BranchResetAnimation actual Label v54 receiver changed");break;
    case Base::Kind::stack:require(state.actual_type_=="Stack"&&state.stack_&&!fields->stack.expired()&&
     fields->stack_parser_progress==detail::NotificationLayoutOrdinaryReceiver::Impl::StackParserProgress::common_returned,
     "BranchResetAnimation actual Stack v54/CommonReturn missing");break;
    default:throw runtime::ScriptError("BranchResetAnimation requires actual child subtype v54 supplier");
   }
   const auto array=state.base_.children_78;
   //4B9E80 genuine NULL78 and zero08 returns. No fake animation completion.
   if(!array){path.pop_back();pure(true);return;}
   const auto list=array->impl_;const auto count=list->ordered.size();const auto data=list->ordered.data();
   require(count<=UINT32_MAX,"BranchResetAnimation original DWORD08 domain exceeded");
   if(list->stack_list){list->stack_list->first_without_callbacks();
    require(list->stack_list->phase==detail::NotificationLayoutOrdinaryChildren78::Impl::StackList::Phase::stable&&
     list->stack_list->count==count,"BranchResetAnimation physical Stack08 differs");}
   if(state.kind_==Base::Kind::label){const auto label=state.standalone_label_->impl_;label->first();
    const auto* alias=std::get_if<detail::NotificationLayoutStandaloneLabelState::Impl::ChildrenBorrow>(&label->fields[0x78u/4u]);
    require(alias&&alias->object==array,"BranchResetAnimation physical Label78 differs");}
   if(!count){path.pop_back();pure(true);return;}
   const auto native_data=list->stack_list?list->stack_list->data:data;
   for(std::size_t i=0;i<count;++i){
    pure(true);require(list->ordered.data()==data&&list->ordered.size()==count,
     "BranchResetAnimation captured native data04/end changed");
    if(list->stack_list)require(list->stack_list->data==native_data&&list->stack_list->count==count,
     "BranchResetAnimation captured actual Stack04/end changed");
    const auto child=native_data[i];
    require(child&&child==list->ordered[i],"BranchResetAnimation physical Stack/native78 member differs");
    const auto child_id=native_ordinary_common_receiver_id_without_callbacks(actual,child);
    require(Core::node(actual->nodes,child_id).layout_baseline->base_.parent_38.lock()==current,
     "BranchResetAnimation current ordinary child has another native parent38");
    //Original4B9EA2 TEST [child+40],2002 precedes its fresh virtual54 call.
    if((flags(child)&0x2002u)==0)self(self,child);
    pure(true);
   }
   path.pop_back();pure(true);
  };
  visit(visit,receiver);pure(true);operation.require_current();pure(true);
 }catch(...){actual->failed.store(true);throw;}
}

BnwWorldViewControlTree::NativeOrdinaryIDRead BnwWorldViewControlTree::read_native_ordinary_id58_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete)const{
 using Base=detail::NotificationLayoutBaseline;const auto actual=core_;
 const auto pure=[&]{actual->layout_origin();const auto& current=actual->source->current_operation_loan();actual->layout_origin();
  require(&current==&operation,"GetID SAME current Source operation replaced");current.require_origin_without_callbacks();
  (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);actual->layout_origin();};
 try{
  pure();actual->layout_current_owned();pure();operation.require_current();pure();
  const auto& state=*Core::node(actual->nodes,receiver->impl_->id).layout_baseline;
  const auto id=state.base_.id_58;
  if(state.kind_==Base::Kind::label){const auto label=state.standalone_label_->impl_;label->first();
   if(id){const auto* physical=std::get_if<detail::NotificationLayoutStandaloneLabelState::Impl::IDBorrow>(&label->fields[0x58u/4u]);
    require(physical&&physical->object==id,"GetID physical Label58/Base identity differs");}
   else{const auto* physical=std::get_if<std::uint32_t>(&label->fields[0x58u/4u]);
    require(physical&&*physical==0u,"GetID genuine Label58 NULL word changed");}
  }
  NativeOrdinaryIDRead observed;
  if(id){require(id->length<id->bytes.size()&&id->bytes[id->length]=='\0'&&
    std::strlen(id->bytes.data())==id->length,"GetID actual564300 terminated58 storage invalid");
   observed.bytes=id->bytes;observed.length=id->length;
  }else{observed.bytes.fill('\0');observed.length=0;} //Actual4DD8B0 empty literalA3EBE2 branch.
  pure();return observed; //POD copy ends all strong borrowers before Lua pushstring.
 }catch(...){actual->failed.store(true);throw;}
}
