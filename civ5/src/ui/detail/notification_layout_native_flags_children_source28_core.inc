//Source22 successor of paired Source14 flag40 writer; direct mouse masks only.
//Genuine current ordinary receiver only. Actual selected v50=4CCEC0 writes
//bit2;72C700 writes40000. No parent/4C94/manager effect exists in either body.
std::uint32_t BnwWorldViewControlTree::native_ordinary_flag_word_without_callbacks(
 const std::shared_ptr<Core>&actual,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&receiver,
 const std::shared_ptr<const void>&concrete){
 const auto id=native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);
 const auto&node=Core::node(actual->nodes,id);const auto&base=*node.layout_baseline;
 if(base.kind_==detail::NotificationLayoutBaseline::Kind::label){
  const auto fields=base.standalone_label_->impl_;fields->first();
  require(fields->core.lock()==actual&&fields->occurrence.lock()==node.layout_receiver_occurrence&&
   fields->selected.lock()==node.full_xml_occurrence&&fields->source_serial==receiver->impl_->serial,
   "Ordinary flags Label actual40 allocation/Source association changed");
  const auto*word=std::get_if<std::uint32_t>(&fields->fields[0x40/4]);
  require(word&&*word==base.base_.flags_40,"Ordinary flags real Label40/Baseline alias differs");
  return *word;
 }
 return base.base_.flags_40; //Other admitted subtype families own Base directly.
}
void BnwWorldViewControlTree::commit_native_ordinary_flag_word_owned(
 const std::shared_ptr<Core>&actual,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&receiver,
 const std::shared_ptr<const void>&concrete,NativeOrdinaryFlagWrite site,std::uint32_t value){
 using Base=detail::NotificationLayoutBaseline;using Parent=detail::NotificationLayoutParentReceiver;
 actual->layout_origin(true);const auto before=native_ordinary_flag_word_without_callbacks(actual,receiver,concrete);
 const auto id=receiver->impl_->id;auto&node=actual->nodes.at(id-1);auto&base=*node.layout_baseline;
 std::uint32_t mask;Parent::Store::Kind kind;
 switch(site){
  case NativeOrdinaryFlagWrite::anchor_51e6a0:mask=0x1b0u;kind=Parent::Store::Kind::ordinary_anchor;break;
  case NativeOrdinaryFlagWrite::hide_v50_4ccec0:mask=2u;kind=Parent::Store::Kind::ordinary_hide;break;
  case NativeOrdinaryFlagWrite::no_clip_72c700:mask=0x40000u;kind=Parent::Store::Kind::ordinary_no_clip;break;
  case NativeOrdinaryFlagWrite::consume_mouse_over_4d7210:mask=0x800u;kind=Parent::Store::Kind::ordinary_consume_mouse_over;break;
  case NativeOrdinaryFlagWrite::consume_mouse_button_437ec0:mask=0x400u;kind=Parent::Store::Kind::ordinary_consume_mouse_button;break;
  case NativeOrdinaryFlagWrite::consume_mouse_wheel_6798e0:mask=0x1000u;kind=Parent::Store::Kind::ordinary_consume_mouse_wheel;break;
  case NativeOrdinaryFlagWrite::disabled_member_5fc7c0:mask=0x20000u;kind=Parent::Store::Kind::ordinary_disabled;break;
  case NativeOrdinaryFlagWrite::disabled_hover_clear_5fc7e0:mask=0x10000u;kind=Parent::Store::Kind::ordinary_disabled_hover_clear;break;
  case NativeOrdinaryFlagWrite::modal_7a5fc0:mask=0x80000u;kind=Parent::Store::Kind::ordinary_modal;break;
  case NativeOrdinaryFlagWrite::tooltip_callback_6dbf40:mask=0x4000u;kind=Parent::Store::Kind::ordinary_tooltip_callback;break;
  case NativeOrdinaryFlagWrite::tooltip_enable_692040:mask=0x4000u;kind=Parent::Store::Kind::ordinary_tooltip_enable;break;
  case NativeOrdinaryFlagWrite::tooltip_type_47cb60_4e3da9:mask=0x4000u;kind=Parent::Store::Kind::ordinary_tooltip_type;break;
  case NativeOrdinaryFlagWrite::tooltip_text_5f3b90:mask=0x4000u;kind=Parent::Store::Kind::ordinary_tooltip_text;break;
  case NativeOrdinaryFlagWrite::tooltip_clear_73fe50_69ade0:mask=0x4000u;kind=Parent::Store::Kind::ordinary_tooltip_clear;break;
  default:throw runtime::ScriptError("Required actual native flag-store callsite");
 }
 require(((before^value)&~mask)==0,"Native flag-store changes bits outside its genuine member");
 require(base.sequence_<UINT64_MAX&&base.history_.size()<base.history_.max_size()&&
  actual->effect_revision<UINT64_MAX&&actual->layout_sequence<UINT64_MAX,"Ordinary flag journal exhausted");
 base.history_.reserve(base.history_.size()+1);actual->layout_origin(true);
 std::shared_ptr<detail::NotificationLayoutStandaloneLabelState::Impl> label;
 if(base.kind_==Base::Kind::label){label=base.standalone_label_->impl_;label->first();
  require(label->sequence<UINT64_MAX&&label->history.size()<label->history.max_size(),"Actual Label40 journal exhausted");
  label->history.reserve(label->history.size()+1);label->first();}
 actual->initial_lua_preflight_store_owned();actual->layout_origin(true);
 require(native_ordinary_flag_word_without_callbacks(actual,receiver,concrete)==before,
  "Ordinary flag preimage changed before genuine store");
 //Every storage and journal capacity is owned BEFORE the single physical
 //write. Label40 and Baseline are two port representations of THAT word.
 if(label){label->history.push_back({++label->sequence,0x40u});
  std::get<std::uint32_t>(label->fields[0x40/4])=value;}
 base.record_owned(Base::Mutation::receiver_prefix);base.base_.flags_40=value;
 if(site==NativeOrdinaryFlagWrite::hide_v50_4ccec0){
  node.hidden=(value&2u)!=0;node.native_update_exclusion=(node.native_update_exclusion&~2u)|(value&2u);
  node.layout_hidden_command=node.hidden; //Existing port visibility mirrors, not extra native events.
 }
 if(site==NativeOrdinaryFlagWrite::disabled_member_5fc7c0)
  node.disabled=(value&0x20000u)!=0; //Port input mirror follows actual physical bit17 only.
 ++actual->effect_revision;++actual->layout_sequence;
 actual->initial_lua_record_store_owned(Core::InitialLuaWriter::receiver_store,id,static_cast<std::uint64_t>(kind));
 actual->layout_origin(true);require(native_ordinary_flag_word_without_callbacks(actual,receiver,concrete)==value,
  "Actual native flag aliases/store POST diverged");
}
void BnwWorldViewControlTree::set_native_ordinary_boolean_flag_owned(
 const runtime::BnwWorldViewSourceOperationLoan&operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&receiver,
 const std::shared_ptr<const void>&concrete,NativeOrdinaryFlagWrite site,bool value){
 const auto actual=core_;
 const auto pure=[&](bool busy){actual->layout_origin(busy);
  const auto&current=actual->source->current_operation_loan();actual->layout_origin(busy);
  require(&current==&operation,"Native bool setter Source operation changed");current.require_origin_without_callbacks();
  (void)native_ordinary_flag_word_without_callbacks(actual,receiver,concrete);actual->layout_origin(busy);};
 try{
  std::uint32_t mask;
  switch(site){
   case NativeOrdinaryFlagWrite::hide_v50_4ccec0:mask=2u;break;
   case NativeOrdinaryFlagWrite::no_clip_72c700:mask=0x40000u;break;
   case NativeOrdinaryFlagWrite::consume_mouse_over_4d7210:mask=0x800u;break;
   case NativeOrdinaryFlagWrite::consume_mouse_button_437ec0:mask=0x400u;break;
   case NativeOrdinaryFlagWrite::consume_mouse_wheel_6798e0:mask=0x1000u;break;
   case NativeOrdinaryFlagWrite::modal_7a5fc0:mask=0x80000u;break;
   case NativeOrdinaryFlagWrite::tooltip_enable_692040:mask=0x4000u;break;
   default:throw runtime::ScriptError("Native bool setter requires actual qualified member identity");
  }
  pure(false);actual->layout_current_owned();pure(false);operation.require_current();pure(false);
  actual->busy=true;struct Exit{bool&value;~Exit(){value=false;}}exit{actual->busy};pure(true);
  //The six real ctor-vptr50 operands are4CCEC0, earned by Source4/14 PE.
  //Sealed current concrete receiver authenticates the actual subtype first;
  //Stack export and all unqualified subtypes stay strict in that borrower.
  const auto old=native_ordinary_flag_word_without_callbacks(actual,receiver,concrete);
  commit_native_ordinary_flag_word_owned(actual,receiver,concrete,site,value?(old|mask):(old&~mask));
  pure(true);operation.require_current();pure(true); //No no-change early return: actual opcode always stores.
 }catch(...){actual->failed.store(true);throw;}
}
std::uint32_t BnwWorldViewControlTree::read_native_ordinary_uint_owned(
 const runtime::BnwWorldViewSourceOperationLoan&operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>&receiver,
 const std::shared_ptr<const void>&concrete,NativeOrdinaryUintRead read)const{
 const auto actual=core_;
 const auto pure=[&]{actual->layout_origin();
  const auto&current=actual->source->current_operation_loan();actual->layout_origin();
  require(&current==&operation,"Native unsigned getter Source operation changed");current.require_origin_without_callbacks();
  (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);actual->layout_origin();};
 try{
  pure();actual->layout_current_owned();pure();operation.require_current();pure();
  const auto id=receiver->impl_->id;const auto&state=*Core::node(actual->nodes,id).layout_baseline;
  std::uint32_t result;
  if(read==NativeOrdinaryUintRead::hidden_40){
   result=(native_ordinary_flag_word_without_callbacks(actual,receiver,concrete)>>1)&1u;
  }else if(read==NativeOrdinaryUintRead::children_78_08){
   const auto array=state.base_.children_78; //Actual current THIS78, not XML/count snapshot.
   if(!array){
    if(state.kind_==detail::NotificationLayoutBaseline::Kind::label){const auto label=state.standalone_label_->impl_;label->first();
     require(std::holds_alternative<detail::NotificationLayoutStandaloneLabelState::Impl::Null>(label->fields[0x78/4]),
      "GetNumChildren actual NULLBase78/Label78 differs");}
    pure();return 0; //Native47E65E after authentic SAME listv0 retirement.
   }
   require(array->impl_&&array->impl_->creator==std::this_thread::get_id()&&array->impl_->parent.lock()==receiver,
    "GetNumChildren current actual78 object/parent custody changed");
   const auto&fields=*array->impl_;
   if(state.kind_==detail::NotificationLayoutBaseline::Kind::label){
    const auto label=state.standalone_label_->impl_;label->first();
    const auto*children=std::get_if<detail::NotificationLayoutStandaloneLabelState::Impl::ChildrenBorrow>(&label->fields[0x78/4]);
    require(children&&children->object==array,"GetNumChildren actual Label78 alias changed");
   }
   if(fields.stack_list){
    fields.stack_list->first_without_callbacks();
    require(fields.stack_list->phase==detail::NotificationLayoutOrdinaryChildren78::Impl::StackList::Phase::stable&&
     fields.stack_list->count==fields.ordered.size(),"GetNumChildren actual list08/view differs");
    result=fields.stack_list->count; //Actual supplier-owned physical scalar.
   }else{
    //Other existing mapped owners use the OWN vector whose real append
    //executors change size only AFTER38/3C and actual child return. size is
    //the mapped native+08 scalar; Common.count/XML never supply the result.
    require(fields.ordered.size()<=UINT32_MAX,"GetNumChildren actual08 exceeds original DWORD domain");
    result=static_cast<std::uint32_t>(fields.ordered.size());
   }
  }else if(read==NativeOrdinaryUintRead::consume_mouse_over_40_bit11_75d480){
   result=(native_ordinary_flag_word_without_callbacks(actual,receiver,concrete)>>11)&1u;
  }else if(read==NativeOrdinaryUintRead::consume_mouse_button_40_bit10_6d8390){
   result=(native_ordinary_flag_word_without_callbacks(actual,receiver,concrete)>>10)&1u;
  }else if(read==NativeOrdinaryUintRead::consume_mouse_wheel_40_bit12_488050){
   result=(native_ordinary_flag_word_without_callbacks(actual,receiver,concrete)>>12)&1u;
  }else if(read==NativeOrdinaryUintRead::disabled_40_bit17_6d2930){
   result=(native_ordinary_flag_word_without_callbacks(actual,receiver,concrete)>>17)&1u;
  }else if(read==NativeOrdinaryUintRead::modal_40_bit19_70c030){
   result=(native_ordinary_flag_word_without_callbacks(actual,receiver,concrete)>>19)&1u;
  }else if(read==NativeOrdinaryUintRead::tooltip_enabled_40_bit14_453350){
   result=(native_ordinary_flag_word_without_callbacks(actual,receiver,concrete)>>14)&1u;
  }else if(read==NativeOrdinaryUintRead::mouse_over_42_bit0_669850){
   //Actual66986E reads byte42&1, exactly the current flags40 mask0x10000.
   //Constructor/store lineage owns this field before any native dispatch;
   //the Host borrower refuses it after the unsupported input route enters.
   //No GUI/window/cache or future input writer is required for this read.
   require(!state.history_.empty()&&state.history_.front().command==detail::NotificationLayoutBaseline::Mutation::construct,
    "HasMouseOver requires actual canonical constructor/store flags40 lineage");
   result=(native_ordinary_flag_word_without_callbacks(actual,receiver,concrete)&0x10000u)!=0u?1u:0u;
  }else throw runtime::ScriptError("Required actual unsigned getter identity");
  pure();return result; //Only POD leaves; strong physical borrowers are destroyed before Lua result push.
 }catch(...){actual->failed.store(true);throw;}
}
