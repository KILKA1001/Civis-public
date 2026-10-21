// PRIVATE supported Civis direct loader, after the real Context constructor.
// Original46FF80 effects order; this is a ported caller, not an observed x86 call.
struct BnwWorldViewControlTree::ContextDirectLoadFrame final {
 using State=detail::NotificationLayoutContextOrdinaryState;
 using Input=detail::NotificationContextNativeLoadNameInputLoan;
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true},failed{false};
 bool normal_returned=false,destruction_passed=false;
 const std::weak_ptr<Core> core;
 const std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
 const runtime::BnwWorldViewSourceOperationLoan*const operation;
 const std::uint64_t source_scope,serial;
 const runtime::detail::ContextLoaderResourceOperand operand;
 const std::shared_ptr<const runtime::ScriptUiChildSource> descriptor;
 const std::shared_ptr<const runtime::detail::ContextLoaderRequestCustody> request;
 const std::shared_ptr<State> state;
 const std::shared_ptr<const detail::NotificationContextNativeConstructorReturn> constructor;
 const std::shared_ptr<detail::NotificationContextGuiPort> manager;
 std::shared_ptr<const Input> input;
 std::shared_ptr<const detail::NotificationContextCursorLoadPrefixReturn> cursor;
 std::shared_ptr<const detail::NotificationContextLuaStatePreparation> prepared;
 std::uint64_t sequence,effects;
 std::uint32_t flags;
 std::array<float,2> size;
 ContextDirectLoadFrame(std::shared_ptr<Core> actual,std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> owner,
  const runtime::BnwWorldViewSourceOperationLoan*op,std::uint64_t scope,std::uint64_t current,
  runtime::detail::ContextLoaderResourceOperand captured,
  std::shared_ptr<const runtime::ScriptUiChildSource> child,
  std::shared_ptr<const runtime::detail::ContextLoaderRequestCustody> named,
  std::shared_ptr<State> physical,std::shared_ptr<const detail::NotificationContextNativeConstructorReturn> ctor,
  std::shared_ptr<detail::NotificationContextGuiPort> gui)
  :core(actual),source(std::move(owner)),operation(op),source_scope(scope),serial(current),operand(std::move(captured)),
   descriptor(std::move(child)),request(std::move(named)),state(std::move(physical)),constructor(std::move(ctor)),manager(std::move(gui)),
   sequence(actual->context_layout_baseline->sequence_),effects(actual->effect_revision),
   flags(actual->context_layout_baseline->base_.flags_40),size(actual->context_layout_baseline->base_.size_30_34){}
 void first()const {
  require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),"Direct Context load lexical frame retired FIRST");
  const auto actual=core.lock();require(actual&&actual->creator==std::this_thread::get_id()&&actual->facade_alive.load()&&
   !actual->failed.load()&&!actual->busy,"Direct Context current physical Core retired/busy FIRST");
  require(State::Impl::same(actual->source,source),"Direct Context canonical Source replaced before operand");
  source->require_operation_origin_without_callbacks(*operation,source_scope); // pointer+scope FIRST, no oldop read.
  operation->require_origin_without_callbacks();require(serial==operation->operation_serial(),"Direct Context operation serial changed");
  actual->layout_origin();
  require(actual->context_direct_load_frame.lock().get()==this&&State::Impl::same(actual->context_ordinary_state,state)&&
   state&&state->impl_&&constructor&&constructor->impl_&&constructor->impl_->alive.load()&&!constructor->impl_->failed.load()&&
   State::Impl::same(state->impl_->native_constructor_return,constructor)&&State::Impl::same(actual->context_gui_port,manager)&&
   actual->context_layout_baseline&&actual->context_layout_baseline->sequence_==sequence&&actual->effect_revision==effects&&
   actual->context_layout_baseline->base_.flags_40==flags&&actual->context_layout_baseline->base_.size_30_34==size&&
   !actual->context_layout_baseline->parsed_source_serial_,"Direct Context constructor/physical journals changed or cold parser ran");
  constructor->impl_->port->require_completed();
  require(request&&descriptor&&operand.request_.get()==request.get()&&State::Impl::same(operand.request_,request)&&
   State::Impl::same(operand.source_,descriptor)&&operand.bundle_&&operand.generation_&&operand.serial_==serial&&
   operand.context_==actual->context&&request->context_==actual->context&&request->original_utf8_==operand.stem_&&
   !request->original_utf8_.empty(),"Direct Context actual selected request/descriptor/Bank custody changed");
  if(request->origin_==runtime::detail::ContextLoaderRequestCustody::Origin::retained_lua_context_declaration){
   require(request->declaration_&&request->declaration_->name=="LuaContext","Direct declaration custody missing");
   const auto&attrs=request->declaration_->attributes;
   const auto authored=std::find_if(attrs.begin(),attrs.end(),[](const auto&value){return value.first=="FileName";});
   require(authored!=attrs.end()&&authored->second==request->original_utf8_,"Direct Context declaration lost original FileName");
  }
  if(input){require(input->impl_&&input->impl_->alive.load()&&!input->impl_->failed.load()&&
   State::Impl::same(actual->context_load_name_frame.lock(),input->impl_),"Direct Context closed Name input frame changed");}
  else require(actual->context_load_name_frame.expired(),"Direct Context foreign Name input active");
  require(State::Impl::same(actual->context_cursor_load_prefix,cursor)&&State::Impl::same(actual->context_lua_preparation,prepared),
   "Direct Context ordered cursor/518 actual publication changed");
  manager->impl_->first(*operation);
 }
 void full()const {first();operation->require_current();first();}
 void accept_store_without_callbacks(){const auto actual=core.lock();require(actual!=nullptr,"Direct Context store owner retired");
  sequence=actual->context_layout_baseline->sequence_;effects=actual->effect_revision;
  flags=actual->context_layout_baseline->base_.flags_40;size=actual->context_layout_baseline->base_.size_30_34;}
};


void BnwWorldViewControlTree::load_selected_context_native_root_owned(const runtime::BnwWorldViewSourceOperationLoan& operation){
 using Frame=ContextDirectLoadFrame;using State=Frame::State;using Input=Frame::Input;
 using Wide=detail::NotificationContextNativeWideStringValue;
 const auto actual=core_;require(actual!=nullptr,"Direct Context physical Core missing");
 const auto owner=actual->source;require(owner!=nullptr,"Direct Context actual Source missing");
 const auto source_scope=owner->capture_active_operation_scope_serial_without_callbacks(&operation);
 const auto cold=[&]{owner->require_operation_origin_without_callbacks(operation,source_scope);actual->layout_origin();
  require(State::Impl::same(actual->source,owner)&&actual->context_ordinary_state&&actual->context_ordinary_state->impl_&&
   actual->context_layout_baseline&&!actual->context_layout_baseline->parsed_source_serial_&&
   !actual->context_cursor_load_prefix&&!actual->context_lua_preparation&&actual->context_direct_load_frame.expired()&&
   actual->context_load_name_frame.expired(),"Direct Context must begin at genuine ctor before parser/load once");};
 cold();operation.require_current();cold();
 const auto descriptor=owner->prepared_source(actual->context);cold();
 auto operand=operation.context_loader_resource_operand_owned(descriptor);cold();
 require(operand.request_&&operand.source_&&State::Impl::same(operand.source_,descriptor)&&operand.context_==actual->context,
  "Direct Context Source-issued original request custody missing");
 const auto request=operand.request_;const auto state=actual->context_ordinary_state;
 const auto constructor=state->impl_->native_constructor_return;const auto manager=actual->context_gui_port;
 require(constructor&&constructor->impl_&&manager,"Direct Context full constructor or canonical GUI missing");
 constructor->impl_->port->require_completed();
 const auto serial=operation.operation_serial();
 { //Actual direct caller lexical lifetime; final issue is AFTER its Exit.
 auto frame=std::make_shared<Frame>(actual,owner,&operation,source_scope,serial,std::move(operand),descriptor,request,state,constructor,manager);
 cold();operation.require_current();cold();actual->context_direct_load_frame=frame;
 struct Exit {std::shared_ptr<Frame> frame;const int exceptions=std::uncaught_exceptions();~Exit(){
  frame->destruction_passed=frame->normal_returned&&!frame->failed.load()&&std::uncaught_exceptions()==exceptions;
  frame->alive.store(false);
 }}exit{frame};
 try{
  frame->full();
  // Native81FF90 CP65001, supported exact ASCII is also valid UTF8. Capture
  // the original request VALUE before the allocator; no normalized-key repair.
  require(std::all_of(request->original_utf8_.begin(),request->original_utf8_.end(),[](unsigned char ch){return ch>0&&ch<128;}),
   "required direct request UTF8 conversion outside selected ASCII path");
  std::u16string wide;wide.reserve(request->original_utf8_.size());frame->full();
  for(unsigned char ch:request->original_utf8_)wide.push_back(static_cast<char16_t>(ch));
  auto argument=std::shared_ptr<const Wide>(new Wide(std::move(wide)));frame->full();
  auto input_data=std::make_shared<Input::Impl>(actual,owner,state,constructor,argument,&operation,serial,source_scope);frame->full();
  auto input=std::shared_ptr<const Input>(new Input(input_data));frame->full();
  input_data->canonical_input=input_data;actual->context_load_name_frame=input_data;frame->input=input;
  frame->full();input_data->first(operation);
  const auto name=store_context_native_load_name_owned(operation,*input);frame->full();
  // Genuine direct v10 receives ctor flags; wrapper DeferLoad is a different
  // path. Constructor-native deferred bit must be absent on this proven route.
  require(!(frame->flags&0x10000000U),"Direct constructor unexpectedly deferred; required distinct wrapper producer");
  const auto before_fx=frame->effects,before_seq=frame->sequence;
  const bool touch=operation.context_touch_bit8_owned();frame->full();
  auto&baseline=*actual->context_layout_baseline;
  require(actual->effect_revision<=UINT64_MAX-4&&actual->layout_sequence<=UINT64_MAX-4,"Direct native store journal exhausted");
  baseline.history_.reserve(baseline.history_.size()+4);frame->full();
  if(touch){baseline.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
   baseline.base_.flags_40|=0x400000U;++actual->effect_revision;++actual->layout_sequence;frame->accept_store_without_callbacks();}
  frame->full();const auto changed=manager->set_cursor_owned(operation,1);frame->full();
  auto cursor_data=std::make_shared<const detail::NotificationContextCursorLoadPrefixReturn::Impl>(
   actual,operation,0,before_seq,before_fx,touch,changed);frame->full();
  auto cursor=std::shared_ptr<const detail::NotificationContextCursorLoadPrefixReturn>(
   new detail::NotificationContextCursorLoadPrefixReturn(cursor_data));frame->full();
  actual->context_cursor_load_prefix=cursor;frame->cursor=cursor;frame->full();
  // Cursor parser_serial=0 is deliberately NOT a legacy parser grant. This
  // closed direct frame validates real touch/cursor/constructor identity; Host
  // merely uses this actual sealed cursor custody to create SAME518.
  const auto&host=operation.context_lua_host_owned();frame->full();
  auto prepared=host.prepare_context_lua_state_owned(operation,descriptor,actual,cursor);frame->full();
  require(prepared!=nullptr,"Direct Context actual518 creator returned NULL");
  host.require_context_lua_state_owned(operation,prepared,actual,cursor);frame->full();
  actual->context_lua_preparation=prepared;frame->prepared=prepared;frame->full();
  //4572F1 dimensions happen after genuine cursor and actual perContext VM.
  manager->require_current_owned(operation);frame->full();
  const auto dimensions=manager->dimensions_without_callbacks(operation);frame->first();
  volatile float width=static_cast<float>(dimensions[0]),height=static_cast<float>(dimensions[1]);
  require(std::isfinite(width)&&std::isfinite(height),"Direct dimensions CVTSI2SS unsupported");
  baseline.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
  baseline.base_.size_30_34[0]=width;++actual->effect_revision;++actual->layout_sequence;frame->accept_store_without_callbacks();
  baseline.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
  baseline.base_.size_30_34[1]=height;++actual->effect_revision;++actual->layout_sequence;frame->accept_store_without_callbacks();
  baseline.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
  baseline.base_.flags_40|=0x300000U;++actual->effect_revision;++actual->layout_sequence;frame->accept_store_without_callbacks();
  frame->full();
  // Sole Host owner implements all45731B..47C. Partial prefix may throw;
  // no caller bool/setup facade can replace its actual completed opaque return.
  auto setup=host.prepare_context_lua_load_setup_owned(operation,prepared,name);frame->full();
  require(setup!=nullptr,"Direct Context required genuine full Lua preRead setup return");
  const auto setup_guard=setup->capture_guard_without_callbacks(operation,prepared);
  setup_guard.require_current_owned(operation,prepared);frame->full();
  // Actual Source→SAME selectedHost earlyPackage→selected UserOptions.
  // Absent startup owner or unknown precedence MUST throw in this producer.
  // Neither null nor guessed false may stand in for the mandatory policy.
  auto policy_unique=operation.context_small_ui_policy_owned();frame->full();
  require(policy_unique!=nullptr,"Direct Context required actual selected SmallUI/AutoUI policy owner");
  policy_unique->require_current_owned(operation);frame->full();
  auto policy=std::shared_ptr<const detail::NotificationContextRootSmallUiPolicyLoan>(std::move(policy_unique));frame->full();
  policy->require_current_owned(operation);frame->full();
  read_context_root_file_prefix_until_required_owner_owned(operation,name,prepared,setup,policy);
  //Original457486..4B1 follows the genuine7A9C30 return. Copy/append514
  //uses that real completedRead lender, never cold parser or replayed loading.
  #include "notification_context_post_root_read_lua_name_core.inc"
  #include "notification_context_lua514_path_resolver_core.inc"
  #include "notification_context_postread_globals_caller_core.inc"
  const auto initialized=initialize_context_direct_5761_owned(operation,actual->context_root_read_pending_completion);
  frame->full();
  execute_context_direct_initial_lua_owned(operation,initialized);
  frame->full(); //Only closed suffix writer returns may have adopted real effects.
  frame->normal_returned=true;
 }catch(...){frame->failed.store(true);actual->failed.store(true);throw;}
 } //Actual Exit ran; pending body still cannot replace its lexical DTOR.
 finalize_context_direct_load_return_owned(operation);
}
