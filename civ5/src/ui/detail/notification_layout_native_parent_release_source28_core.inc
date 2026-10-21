//Source28: real41/48 listv0 releases actual owned storage before NULL78.
//Found childbit8/freshv84 still precedes edits and refuses missing destruction.
std::shared_ptr<const void> BnwWorldViewControlTree::native_ordinary_concrete_without_callbacks(
 const std::shared_ptr<Core>& actual,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver){
 using Base=detail::NotificationLayoutBaseline;
 const auto id=native_ordinary_common_receiver_id_without_callbacks(actual,receiver);
 const auto& state=*Core::node(actual->nodes,id).layout_baseline;const auto fields=receiver->impl_;
 switch(state.kind_){
  case Base::Kind::image:require(state.image_&&fields->image.lock()==state.image_,"Native children actual Image allocation changed");return state.image_;
  case Base::Kind::box:require(state.box_&&fields->box.lock()==state.box_,"Native children actual Box allocation changed");return state.box_;
  case Base::Kind::button:require(state.button_&&fields->button.lock()==state.button_,"Native children actual Button allocation changed");return state.button_;
  case Base::Kind::grid:require(state.grid_&&fields->grid.lock()==state.grid_,"Native children actual Grid allocation changed");return state.grid_;
  case Base::Kind::label:require(state.standalone_label_&&fields->standalone_label.lock()==state.standalone_label_,"Native children actual Label allocation changed");return state.standalone_label_;
  case Base::Kind::stack:{const auto value=fields->stack.lock();require(state.stack_&&value&&
   value.get()==&*state.stack_&&!value.owner_before(actual)&&!actual.owner_before(value),
   "Native children actual Stack physical/Core allocation alias changed");return value;}
  default:throw runtime::ScriptError("Native children require genuine concrete subtype/Context allocation supplier");
 }
}

void BnwWorldViewControlTree::execute_native_ordinary_children_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,NativeOrdinaryChildrenMember member,const void* argument){
 using Base=detail::NotificationLayoutBaseline;using Parent=detail::NotificationLayoutParentReceiver;
 using Children=detail::NotificationLayoutOrdinaryChildren78;
 const auto actual=core_;
 const auto pure=[&](bool busy){actual->layout_origin(busy);
  const auto& current=actual->source->current_operation_loan();actual->layout_origin(busy);
  require(&current==&operation,"Native parent/children SAME current Source operation replaced");current.require_origin_without_callbacks();
  (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);actual->layout_origin(busy);};
 try{
  pure(false);actual->layout_current_owned();pure(false);operation.require_current();pure(false);
  require(member==NativeOrdinaryChildrenMember::change_parent_5335e0||
   member==NativeOrdinaryChildrenMember::release_child_6ab0d0||
   member==NativeOrdinaryChildrenMember::destroy_all_children_781ff0,"Actual39/41/48 member identity required");
  //5335EA genuine NULL newParent skips even the old parent38 field read.
  if(member==NativeOrdinaryChildrenMember::change_parent_5335e0&&!argument){pure(false);return;}
  actual->busy=true;struct Exit{bool& value;~Exit(){value=false;}}exit{actual->busy};pure(true);
  const auto array_for=[&](const auto& parent){
   const auto id=native_ordinary_common_receiver_id_without_callbacks(actual,parent);
   const auto& state=*Core::node(actual->nodes,id).layout_baseline;const auto array=state.base_.children_78;
   if(!array){
    if(state.kind_==Base::Kind::label){const auto label=state.standalone_label_->impl_;label->first();
     require(std::holds_alternative<detail::NotificationLayoutStandaloneLabelState::Impl::Null>(label->fields[0x78u/4u]),
      "Native children current NULLBase78/Label78 alias changed");}
    return array; //Only actual constructor-list storage retirement admits NULL.
   }
   const auto fields=array->impl_;require(fields&&fields->creator==std::this_thread::get_id()&&fields->parent.lock()==parent,
    "Native parent/children actual78 object/parent association changed");
   require(fields->ordered.size()<=UINT32_MAX,"Native children original DWORD count domain exceeded");
   if(fields->stack_list){fields->stack_list->first_without_callbacks();
    require(fields->stack_list->phase==Children::Impl::StackList::Phase::stable&&
     fields->stack_list->count==fields->ordered.size(),"Native children actual Stack04/08 association changed");
    for(std::size_t i=0;i<fields->ordered.size();++i)require(fields->stack_list->data[i]==fields->ordered[i],
     "Native children actual Stack04/logical slot alias differs");}
   if(state.kind_==Base::Kind::label){const auto label=state.standalone_label_->impl_;label->first();
    const auto* alias=std::get_if<detail::NotificationLayoutStandaloneLabelState::Impl::ChildrenBorrow>(&label->fields[0x78u/4u]);
    require(alias&&alias->object==array,"Native children actual Label78 alias changed");}
   return array;
  };
  const auto preflight=[&](std::uint64_t id){auto& state=*actual->nodes.at(id-1).layout_baseline;
   require(state.sequence_<UINT64_MAX&&state.history_.size()<state.history_.max_size()&&
    actual->effect_revision<UINT64_MAX&&actual->layout_sequence<UINT64_MAX,"Native children journal exhausted");
   state.history_.reserve(state.history_.size()+1);actual->initial_lua_preflight_store_owned();pure(true);};
  const auto journal=[&](std::uint64_t id,Parent::Store::Kind kind){
   auto& state=*actual->nodes.at(id-1).layout_baseline;state.record_owned(Base::Mutation::receiver_prefix);
   ++actual->effect_revision;++actual->layout_sequence;
   actual->initial_lua_record_store_owned(Core::InitialLuaWriter::receiver_store,id,static_cast<std::uint64_t>(kind));
  };
  const auto release_list=[&](const auto& parent,const auto& array){
   pure(true);const auto id=native_ordinary_common_receiver_id_without_callbacks(actual,parent);
   require(array&&array_for(parent)==array&&array->impl_&&array->impl_.use_count()==1,
    "Native listv0 requires SAME sole owned physical storage; retained active storage borrower remains");
   //Native578010 never destructs childControls. The actual empty count means
   //there are no live child slots; stale pointer capacity is released by RAII.
   const auto*const fields=array->impl_.get();
   require(fields->ordered.empty(),"Native listv0 current08 must be zero before storage release");
   if(fields->stack_list){fields->stack_list->first_without_callbacks();
    require(fields->stack_list->count==0,"Native listv0 actual Stack08 differs");}
   auto& state=*actual->nodes.at(id-1).layout_baseline;
   const auto label=state.kind_==Base::Kind::label?state.standalone_label_->impl_:nullptr;
   preflight(id);
   if(label){label->first();require(label->sequence<UINT64_MAX&&label->history.size()<label->history.max_size(),
     "Native listv0 Label78 journal exhausted");label->history.reserve(label->history.size()+1);pure(true);}
   require(array_for(parent)==array&&array->impl_.get()==fields&&array->impl_.use_count()==1,
    "Native listv0 storage changed during preflight");
   //Destroy the REAL owned Impl, including ordered allocation and Stack
   //inline/heap slots. Historical wrappers survive without physical storage.
   //No allocator, provider or Lua callback occurs between v0 and pairedNULL.
   array->impl_.reset();
   state.base_.children_78.reset();
   if(label){label->fields[0x78u/4u]=detail::NotificationLayoutStandaloneLabelState::Impl::Null{};
    label->history.push_back({++label->sequence,0x78u});}
   journal(id,Parent::Store::Kind::ordinary_children_null);pure(true);
  };
  const auto require_child_v84=[&](const auto& child){
   const auto child_id=native_ordinary_common_receiver_id_without_callbacks(actual,child);
   const auto& child_state=*Core::node(actual->nodes,child_id).layout_baseline;
   if(child_state.kind_==Base::Kind::label){const auto label=child_state.standalone_label_->impl_;label->first();
    const auto* flags=std::get_if<std::uint32_t>(&label->fields[0x40u/4u]);
    require(flags&&*flags==child_state.base_.flags_40,"Release/Destroy actual Label40/Base alias changed");}
   if(child_state.base_.flags_40&8u)
    throw runtime::ScriptError("Release/Destroy foundbit8 requires fresh actual subtypev84 Image685D70/Box6CC600/Button78DD90/Label7A0630/Stack4E9B10/Grid773170: childv0(0)/typed destruction and real GUI/Controls/CData/callback lifetime retirement before list edits");
  };
  const auto remove=[&](const auto& parent,const void* target){

   pure(true);const auto id=native_ordinary_common_receiver_id_without_callbacks(actual,parent);const auto array=array_for(parent);
   if(!array){pure(true);return;} //7703D8/6AB0D8 actual NULL78 skip.
   auto*const fields=array->impl_.get();const auto count=fields->ordered.size();const auto data=fields->ordered.data();
   std::size_t ordinal=count;
   //Native7703F5/6AB0F5 compares pointer values; NULL argument is allowed.
   if(target)for(std::size_t i=0;i<count;++i){
    const auto child=fields->ordered[i];const auto actual_child=native_ordinary_concrete_without_callbacks(actual,child);
    if(actual_child.get()==target){ordinal=i;break;}}
   if(ordinal==count){
    if(!count){release_list(parent,array);pure(true);return;} //6AB0F9/7703F9 genuine listv0 thenNULL78.
    pure(true);return; //Actual nonempty miss leaves every pointer/count unchanged.
   }
   if(member==NativeOrdinaryChildrenMember::release_child_6ab0d0)
    require_child_v84(fields->ordered[ordinal]); //Actual6AB115 BEFORE first shift.
   for(std::size_t i=ordinal+1;i<count;++i){
    pure(true);require(array_for(parent)==array&&fields->ordered.data()==data&&fields->ordered.size()==count,
     "Native remove captured pointer-list04/end changed");
    preflight(id);const auto next=fields->ordered[i];
    if(fields->stack_list)fields->stack_list->data[i-1]=next;
    fields->ordered[i-1]=next; //Exact original left shift of surviving pointer values.
    journal(id,Parent::Store::Kind::ordinary_children_slot);
   }
   pure(true);preflight(id);
   if(fields->stack_list)--fields->stack_list->count;
   fields->ordered.pop_back(); //Mapped native08 decrement; vector capacity/storage remains owned.
   journal(id,Parent::Store::Kind::ordinary_children_count);pure(true);
   //7703D0 and bit8=false6AB0D0 do not clear child38/3C/list20 or retire callbacks.
  };
  if(member==NativeOrdinaryChildrenMember::destroy_all_children_781ff0){
   const auto id=native_ordinary_common_receiver_id_without_callbacks(actual,receiver);const auto array=array_for(receiver);
   if(!array){pure(true);operation.require_current();pure(true);return;} //781FF8 real NULL78 skip.
   auto*const fields=array->impl_.get();const auto count=fields->ordered.size();const auto*const data=fields->ordered.data();
   //781FFD captures data04/end BEFORE each fresh child40/v84 dispatch.
   for(std::size_t i=0;i<count;++i){pure(true);
    require(array_for(receiver)==array&&array->impl_.get()==fields&&fields->ordered.data()==data&&fields->ordered.size()==count,
     "DestroyAllChildren captured real04/end replaced");
    require_child_v84(fields->ordered[i]); //No successful destructor bypass.
   }
   pure(true);preflight(id);
   if(fields->stack_list)fields->stack_list->count=0;
   fields->ordered.clear(); //781FF0 unconditional native08=0 after the loop.
   journal(id,Parent::Store::Kind::ordinary_children_count);pure(true);
   release_list(receiver,array);pure(true);operation.require_current();pure(true);return;
  }
  if(member==NativeOrdinaryChildrenMember::change_parent_5335e0){
   const auto& child=*actual->nodes.at(receiver->impl_->id-1).layout_baseline;
   const auto previous=child.base_.parent_38.lock();
   if(!previous){const std::weak_ptr<const detail::NotificationLayoutOrdinaryReceiver> empty;
    require(!child.base_.parent_38.owner_before(empty)&&!empty.owner_before(child.base_.parent_38),
     "ChangeParent real nonNULL previousParent38 retired; cannot become constructorNULL");}
   if(previous)remove(previous,concrete.get());pure(true);
   //Do not replace the fresh target vptr38 with a public/XML attach helper.
   throw runtime::ScriptError("ChangeParent533601 positive target requires real current target receiver/Tree/concrete v38: five44BF40 or Stack685AA0 flags prefix then child38/3C, actual495E10/list20/capacity5B45D0/append04/count08 owner; previous7703D0 effects retained");
  }else{
   remove(receiver,argument);pure(true);operation.require_current();pure(true);return;
  }
 }catch(...){actual->failed.store(true);throw;}
}
