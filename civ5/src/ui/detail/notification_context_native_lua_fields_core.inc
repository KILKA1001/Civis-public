// Inside Tree namespace after actual direct caller and physical receiver storage.
// The opaque handles are real private Host-owned LuaRef objects, not CData,
// cached values, callbacks supplied by a caller, or initialization receipts.
std::shared_ptr<const void> BnwWorldViewControlTree::read_native_initial_lua_function_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<detail::NotificationLayoutContextOrdinaryState>& root,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,NativeInitialLuaField field,std::uint32_t event,bool busy)const{
 const auto actual=core_;actual->layout_origin(busy);operation.require_origin_without_callbacks();
 require(&actual->source->current_operation_loan()==&operation&&actual->initial_lua_mutation_journal,
  "Native initial function borrow requires SAME actual direct Lua writer scope");
 if(field==NativeInitialLuaField::button_event_84){
  require(!root&&receiver&&concrete&&event<7,"Button native callback requires its real seven-slot receiver");
  (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);
  const auto p=receiver->impl_;
  require(p&&p->button_callbacks_84_b4&&p->button_void_f4_f8&&p->button.lock()&&
   p->canonical.lock()==receiver,"Button native callback constructor/physical storage retired");
  return (*p->button_callbacks_84_b4)[event];
 }
 require(root&&root==actual->context_ordinary_state&&root->impl_&&!receiver&&!concrete&&event==0&&
  actual->context_layout_baseline&&actual->context_layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::context&&
  actual->context_layout_baseline->base_.receiver_owned&&
  actual->context_layout_baseline->base_.receiver_owned->impl_->context.lock()==root&&
  root->impl_->creator==std::this_thread::get_id()&&
  root->impl_->alive.load()&&!root->impl_->failed.load()&&root->impl_->core.lock()==actual&&
  root->impl_->source.lock()==actual->source&&root->impl_->native_constructor_return&&
  root->impl_->native_constructor_prefix&&root->impl_->lua_functions_90_51c_524,
  "Context native function requires SAME real constructor/root receiver");
 const auto prefix=root->impl_->native_constructor_prefix->impl_;
 const auto suffix=root->impl_->native_constructor_return->impl_;
 require(prefix&&suffix&&prefix->creator==root->impl_->creator&&suffix->creator==root->impl_->creator&&
  prefix->alive.load()&&!prefix->failed.load()&&suffix->alive.load()&&!suffix->failed.load()&&
  prefix->core.lock()==actual&&suffix->core.lock()==actual&&prefix->source.lock()==actual->source&&
  suffix->source.lock()==actual->source&&prefix->projection.lock()==root&&suffix->projection.lock()==root&&
  suffix->prefix==root->impl_->native_constructor_prefix&&prefix->port&&suffix->port,
  "Context native current fields lost their genuine constructor prefix/suffix custody");
 root->impl_->native_constructor_prefix->impl_->port->require_reached();
 root->impl_->native_constructor_return->impl_->port->require_completed();
 const std::size_t index=field==NativeInitialLuaField::context_update_90?0:
  field==NativeInitialLuaField::context_input_51c?1:field==NativeInitialLuaField::context_showhide_524?2:3;
 require(index<3,"Context native function field differs from qualified member");
 return (*root->impl_->lua_functions_90_51c_524)[index];
}

void BnwWorldViewControlTree::replace_native_initial_lua_function_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<detail::NotificationLayoutContextOrdinaryState>& root,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,NativeInitialLuaField field,std::uint32_t event,
 const std::shared_ptr<const void>& expected,const std::shared_ptr<const void>& value){
 const auto actual=core_;
 const auto pure=[&](bool busy){actual->layout_origin(busy);
  require(read_native_initial_lua_function_owned(operation,root,receiver,concrete,field,event,busy)==expected,
   "Native function actual field changed before owned assignment");};
 try{
  // The real unref/field-reset/publication order must have no callbackful
  // Source gap after oldref destruction. Host performs FULL before entering
  // assignment; this commit repeats only the current callback-free lender.
  pure(false);actual->busy=true;struct Exit{bool&value;~Exit(){value=false;}}exit{actual->busy};pure(true);
  auto*base=field==NativeInitialLuaField::button_event_84?
   &*Core::mutable_node_owned(actual->nodes,receiver->impl_->id).layout_baseline:&*actual->context_layout_baseline;
  require(base->sequence_<UINT64_MAX&&base->history_.size()<base->history_.max_size()&&
   actual->effect_revision<UINT64_MAX&&actual->layout_sequence<UINT64_MAX,
   "Native function physical assignment journal exhausted");
  base->history_.reserve(base->history_.size()+1);pure(true);actual->initial_lua_preflight_store_owned();pure(true);
  base->record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
  if(field==NativeInitialLuaField::button_event_84)(*receiver->impl_->button_callbacks_84_b4)[event]=value;
  else (*root->impl_->lua_functions_90_51c_524)[field==NativeInitialLuaField::context_update_90?0:
   field==NativeInitialLuaField::context_input_51c?1:2]=value;
  ++actual->effect_revision;++actual->layout_sequence;
  actual->initial_lua_record_store_owned(Core::InitialLuaWriter::receiver_store,
   receiver?receiver->impl_->id:0,static_cast<std::uint64_t>(field));
  require(read_native_initial_lua_function_owned(operation,root,receiver,concrete,field,event,true)==value,
   "Native function actual assignment publication changed");
 }catch(...){actual->failed.store(true);throw;}
}

void BnwWorldViewControlTree::set_native_context_function_flag_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<detail::NotificationLayoutContextOrdinaryState>& root,NativeInitialLuaField field){
 const auto actual=core_;actual->layout_origin();
 require(field!=NativeInitialLuaField::button_event_84,"Button callback has no Context flag store");
 const auto mask=field==NativeInitialLuaField::context_update_90?0x00800000u:
  field==NativeInitialLuaField::context_input_51c?0x04000000u:0x08000000u;
 try{
  const auto expected=read_native_initial_lua_function_owned(operation,root,{}, {},field,0);
  require(expected!=nullptr,"Context function flag must follow real owned function publication");
  operation.require_current();
  require(read_native_initial_lua_function_owned(operation,root,{}, {},field,0)==expected,
   "Context function was replaced before original unconditional flag store");
  actual->busy=true;struct Exit{bool&value;~Exit(){value=false;}}exit{actual->busy};actual->layout_origin(true);
  auto&base=*actual->context_layout_baseline;
  require(base.sequence_<UINT64_MAX&&base.history_.size()<base.history_.max_size()&&
   actual->effect_revision<UINT64_MAX&&actual->layout_sequence<UINT64_MAX,
   "Context function flag journal exhausted");
  base.history_.reserve(base.history_.size()+1);actual->layout_origin(true);actual->initial_lua_preflight_store_owned();
  require(read_native_initial_lua_function_owned(operation,root,{}, {},field,0,true)==expected,
   "Context function publication changed across actual flag reserve");
  base.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);base.base_.flags_40|=mask;
  ++actual->effect_revision;++actual->layout_sequence;
  actual->initial_lua_record_store_owned(Core::InitialLuaWriter::receiver_store,0,mask);
  actual->layout_origin(true);operation.require_origin_without_callbacks();
 }catch(...){actual->failed.store(true);throw;}
}

void BnwWorldViewControlTree::set_native_button_void1_4a9bc0_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,std::uint32_t value){
 const auto actual=core_;actual->layout_origin();
 const auto pure=[&](bool busy){actual->layout_origin(busy);
  require(&actual->source->current_operation_loan()==&operation,"Void1 SAME current Source changed");
  operation.require_origin_without_callbacks();
  (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);
  require(actual->initial_lua_mutation_journal&&receiver->impl_->button_void_current_matches_owned()&&
   receiver->impl_->button_callbacks_84_b4,"Void1 requires actual Button ctor fields and direct Lua writer");};
 try{
  pure(false);operation.require_current();pure(false);
  actual->busy=true;struct Exit{bool&value;~Exit(){value=false;}}exit{actual->busy};pure(true);
  auto&base=*Core::mutable_node_owned(actual->nodes,receiver->impl_->id).layout_baseline;
  require(base.sequence_<UINT64_MAX&&base.history_.size()<base.history_.max_size()&&
   actual->effect_revision<UINT64_MAX&&actual->layout_sequence<UINT64_MAX&&
   receiver->impl_->button_void1_4a9bc0_store_count<UINT64_MAX,"Void1 actual journal exhausted");
  base.history_.reserve(base.history_.size()+1);pure(true);actual->initial_lua_preflight_store_owned();pure(true);
  base.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
  (*receiver->impl_->button_void_f4_f8)[0]=value;
  receiver->impl_->button_void1_4a9bc0_value=value;
  ++receiver->impl_->button_void1_4a9bc0_store_count;
  ++actual->effect_revision;++actual->layout_sequence;
  actual->initial_lua_record_store_owned(Core::InitialLuaWriter::receiver_store,receiver->impl_->id,0x4a9bc0u);
  pure(true);
 }catch(...){actual->failed.store(true);throw;}
}
