//Actual526580 JMPv88. Button dispatch7AA830 is distinct from base5FC7C0.
void BnwWorldViewControlTree::set_native_ordinary_disabled88_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,bool supplied){
 using Base=detail::NotificationLayoutBaseline;using Parent=detail::NotificationLayoutParentReceiver;
 const auto actual=core_;
 const auto pure=[&](bool busy){actual->layout_origin(busy);
  const auto& current=actual->source->current_operation_loan();actual->layout_origin(busy);
  require(&current==&operation,"SetDisabled SAME Source operation replaced");current.require_origin_without_callbacks();
  (void)native_ordinary_flag_word_without_callbacks(actual,receiver,concrete);actual->layout_origin(busy);};
 try{
  pure(false);actual->layout_current_owned();pure(false);operation.require_current();pure(false);
  const auto id=receiver->impl_->id;auto& state=*actual->nodes.at(id-1).layout_baseline;
  const bool button=state.kind_==Base::Kind::button;
  //Only7AA830 compares current disabled bit first. Base5FC7C0 always stores.
  if(button&&bool(native_ordinary_flag_word_without_callbacks(actual,receiver,concrete)&0x20000u)==supplied){pure(false);return;}
  actual->busy=true;struct Exit{bool& value;~Exit(){value=false;}}exit{actual->busy};pure(true);
  if(button){
   const auto physical=state.button_;require(physical&&physical->backing_118_&&physical->backing_118_->impl_,
    "SetDisabled Button actual C4/118 storage absent");
   const auto backing=physical->backing_118_->impl_;backing->require_same_baseline_without_callbacks(*physical,state);
   require(state.sequence_<UINT64_MAX&&state.history_.size()<state.history_.max_size()&&
    actual->effect_revision<UINT64_MAX&&actual->layout_sequence<UINT64_MAX,"SetDisabled actual C4 journal exhausted");
   state.history_.reserve(state.history_.size()+1);actual->initial_lua_preflight_store_owned();pure(true);
   const auto before_state=receiver->impl_->button_state104_store_count;
   const auto before_words=backing->store_count;
   const auto record_physical_prefix=[&]{
    //Own store counters remain inspectable in retained C++ custody after a
    //failed Source callback. This journal grants no current native return.
    if(receiver->impl_->button_state104_store_count!=before_state||backing->store_count!=before_words){
     state.record_owned(Base::Mutation::receiver_prefix);++actual->effect_revision;++actual->layout_sequence;
     actual->initial_lua_record_store_owned(Core::InitialLuaWriter::receiver_store,id,
      static_cast<std::uint64_t>(Parent::Store::Kind::button));
    }
   };
   try{
    //Actual A3C9C0+C4=6B3610 FIRST writes104 via6A9610, then genuine
    //non80000000 x87/SSE geometry150/14C and5C7B50 coordinates supplier.
    //The existing physical actor retains every preceding store on failure.
    detail::BnwNormalButtonSubobject118Backing::execute_visual_state6b3610_owned(*physical,state,supplied?3u:0u);
   }catch(...){record_physical_prefix();throw;}
   record_physical_prefix();pure(true);
  }
  auto flags=native_ordinary_flag_word_without_callbacks(actual,receiver,concrete);
  //5FC7CA or5FC7FC always writes, including unchanged bit in base receivers.
  commit_native_ordinary_flag_word_owned(actual,receiver,concrete,NativeOrdinaryFlagWrite::disabled_member_5fc7c0,
   supplied?(flags|0x20000u):(flags&~0x20000u));pure(true);
  flags=native_ordinary_flag_word_without_callbacks(actual,receiver,concrete);
  if(supplied&&(flags&0x10000u)){
   //5FC7E0 clears real physical hover bit BEFORE GUI removal/callback.
   commit_native_ordinary_flag_word_owned(actual,receiver,concrete,NativeOrdinaryFlagWrite::disabled_hover_clear_5fc7e0,
    flags&~0x10000u);pure(true);
   throw runtime::ScriptError("SetDisabled hovered suffix requires actual GUI global02DC1B1C/680EE0 list968C/96A4 removal return then SAME receiver4B82A0(0) animation arrays7C/80/v54 owner");
  }
  if(button){
   //Even after5FC7C0 returned, actual7AA860 reads GUI CB8CF4+13B4.
   //No absent provider is taken as the original skip; cached mouse words
   //alone do not grant the HWND+A4/PostMessageA conditional return.
   throw runtime::ScriptError("SetDisabled Button7AA860 requires actual GUI CB8CF4/13B4 current eligibility; false branch must perform PostMessageA(HWND+A4,WM_MOUSEMOVE,global02DC1B1C+650,packed64C/64E)");
  }
  pure(true);operation.require_current();pure(true);
 }catch(...){actual->failed.store(true);throw;}
}

void BnwWorldViewControlTree::calculate_native_ordinary_visibility94_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete){
 using Parent=detail::NotificationLayoutParentReceiver;using View=Parent::TreeView;
 const auto actual=core_;std::shared_ptr<View::Binding> binding;
 const auto pure=[&](bool busy){
  actual->layout_origin(busy);
  const auto& current=actual->source->current_operation_loan();actual->layout_origin(busy);
  require(&current==&operation,"CalculateVisibilityBox current SAME Source operation replaced");
  current.require_origin_without_callbacks();
  (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);
  actual->layout_origin(busy);
 };
 try{
  pure(false);actual->layout_current_owned();pure(false);operation.require_current();pure(false);
  const auto id=receiver->impl_->id;
  actual->busy=true;struct Exit{bool& value;~Exit(){value=false;}}exit{actual->busy};pure(true);
  binding=std::make_shared<View::Binding>(actual,operation);pure(true);
  std::vector<Parent::Store> stores;std::vector<Parent::FourCReturn> returns;
  {
   View view(binding,binding->captured);view.require_pure();
   //Actual5F6840 JMP fresh vptr94. Existing real Parent receiver owns
   //6EBC30 bounds, Stack76E380/4D5110, children and changed parent94.
   //Label generic94/Context full child vectors remain mandatory suppliers.
   Parent::slot94(view,{0,id});
   view.require_full();view.require_pure();
   stores=view.stores_;returns=view.four_c_returns_;view.require_pure();
  }
  binding->pure(false);
  require(binding->destruction_passed,"CalculateVisibilityBox genuine layout View did not return/destruct normally");
  //Refresh only existing port mirrors AFTER actual virtual/child returns and
  //View destruction. These are not extra native stores or an XML snapshot.
  const auto mirror=[&](const std::shared_ptr<Core>& core,std::uint64_t control,
   std::uint64_t topology,std::uint64_t effects){
   if(!control)return;(void)Core::node(core->nodes,control);auto& n=core->nodes.at(control-1);
   require(n.layout_baseline.has_value(),"CalculateVisibilityBox returned mirror lacks actual baseline");
   const auto& current=*n.layout_baseline;auto quote=n.layout.value_or(LayoutState{});
   quote.native_offset=current.base_.offset_28_2c;quote.calculated_size=current.base_.size_30_34;
   quote.native_extent=current.base_.extent_20_24;quote.native_bounds=current.base_.bounds_10_1c;
   quote.calculate_returned=false;quote.source_serial=binding->serial;
   quote.topology_revision=topology;quote.effect_revision=effects;quote.command_sequence=core->layout_sequence;
   n.layout=std::move(quote);
  };
  mirror(actual,id,binding->domains.front().topology,binding->domains.front().effects);
  for(const auto& store:stores){const auto& domain=binding->domains.at(store.key.domain);
   mirror(domain.core,store.key.id,domain.topology,domain.effects);
   if(store.key.id&&store.kind==Parent::Store::Kind::stack_anchor){auto& n=domain.core->nodes.at(store.key.id-1);
    require(n.layout&&n.layout_baseline&&n.layout_baseline->stack_,"CalculateVisibilityBox returned Stack mirror lacks actual owner");
    n.layout->anchor_origin={n.layout_baseline->stack_->field_90,n.layout_baseline->stack_->field_94};
    n.layout->anchor_prepass_stored=true;n.layout->anchoring_returned=true;
   }
  }
  for(const auto& stamp:returns){const auto& domain=binding->domains.at(stamp.key.domain);
   Core::LayoutFourCReturn earned{stamp.actual_offset,stamp.baseline_sequence,binding->serial,
    domain.topology,stamp.offset_store_sequence};
   if(stamp.key.id==0)domain.core->context_layout_four_c_return=earned;
   else domain.core->nodes.at(stamp.key.id-1).layout_four_c_return=earned;
  }
  binding->pure(false);binding->finished=true;binding.reset();
  pure(true);operation.require_current();pure(true);
 }catch(...){if(binding)binding->poison();actual->failed.store(true);throw;}
}

