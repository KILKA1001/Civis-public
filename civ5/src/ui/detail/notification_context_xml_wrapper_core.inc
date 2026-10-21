// PRIVATE4FEE80 wrapper prefix. This is not46FF80/directv10.
// Original cursor reads are ported onto SAME retained declaration attributes;
// only the proved absentStyle/positiveASCIIFileName path is admitted here.
struct BnwWorldViewControlTree::ContextXmlWrapperFrame final {
 using State=detail::NotificationLayoutContextOrdinaryState;
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true},failed{false};
 const std::weak_ptr<Core> core;
 const std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
 const runtime::BnwWorldViewSourceOperationLoan*const operation;
 const std::uint64_t scope,serial;
 const runtime::detail::ContextLoaderResourceOperand operand;
 const std::shared_ptr<const runtime::ScriptUiChildSource> descriptor;
 const std::shared_ptr<const runtime::detail::ContextLoaderRequestCustody> request;
 const std::shared_ptr<const content::XmlNode> declaration;
 const std::optional<detail::NotificationContextRootReadChildrenLoan::Guard> parent_guard;
 const std::shared_ptr<State> state;
 const std::shared_ptr<const detail::NotificationContextNativeConstructorReturn> constructor;
 std::shared_ptr<const detail::NotificationContextNativeWideStringValue> b4;
 std::uint64_t b4_revision,sequence,effects,history_size;
 std::uint32_t flags;
 std::shared_ptr<const detail::NotificationLayoutContextRealmRef> expected_context_realm;
 ContextXmlWrapperFrame(std::shared_ptr<Core> actual,
  std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> owner,
  const runtime::BnwWorldViewSourceOperationLoan*op,std::uint64_t current_scope,std::uint64_t current,
  runtime::detail::ContextLoaderResourceOperand captured,
  std::shared_ptr<const runtime::ScriptUiChildSource> child,
  std::shared_ptr<const runtime::detail::ContextLoaderRequestCustody> input,
  std::shared_ptr<State> physical,
  std::shared_ptr<const detail::NotificationContextNativeConstructorReturn> ctor,
  std::shared_ptr<const detail::NotificationContextNativeWideStringValue> original_b4,
  std::uint64_t original_revision,std::uint64_t original_sequence,std::uint64_t original_effects,
  std::uint64_t original_history,std::uint32_t original_flags,
  std::optional<detail::NotificationContextRootReadChildrenLoan::Guard> parent)
  :core(actual),source(std::move(owner)),operation(op),scope(current_scope),serial(current),
   operand(std::move(captured)),descriptor(std::move(child)),request(std::move(input)),
   declaration(request->declaration_),parent_guard(std::move(parent)),state(std::move(physical)),constructor(std::move(ctor)),
   b4(std::move(original_b4)),b4_revision(original_revision),sequence(original_sequence),effects(original_effects),
   history_size(original_history),flags(original_flags){}
 const std::string* attribute_without_callbacks(std::string_view name)const {
  const auto& attrs=declaration->attributes;
  const auto at=std::find_if(attrs.begin(),attrs.end(),[name](const auto& a){return a.first==name;});
  return at==attrs.end()?nullptr:&at->second;
 }
 void first()const {
  require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),"Context wrapper lexical frame retired FIRST");
  if(parent_guard)parent_guard->require_current_without_callbacks();
  const auto actual=core.lock();require(actual&&actual->creator==std::this_thread::get_id()&&actual->facade_alive.load()&&
   !actual->failed.load()&&!actual->busy,"Context wrapper actual physical Core retired/busy FIRST");
  require(State::Impl::same(actual->source,source),"Context wrapper canonical Source replaced FIRST");
  source->require_operation_origin_without_callbacks(*operation,scope); // pointer+stored scope BEFORE old operand.
  operation->require_origin_without_callbacks();require(operation->operation_serial()==serial,"Context wrapper current serial changed");
  actual->layout_origin();
  require(actual->context_xml_wrapper_frame.lock().get()==this&&actual->context_direct_load_frame.expired()&&
   actual->context_load_name_frame.expired()&&!actual->context_cursor_load_prefix&&!actual->context_lua_preparation&&
   actual->context_layout_baseline&&!actual->context_layout_baseline->parsed_source_serial_&&
   State::Impl::same(actual->context_ordinary_state,state)&&state&&state->impl_&&state->impl_->alive.load()&&!state->impl_->failed.load(),
   "Context wrapper actual constructor/load/current parser phase changed");
  require(constructor&&constructor->impl_&&constructor->impl_->alive.load()&&!constructor->impl_->failed.load()&&
   State::Impl::same(state->impl_->native_constructor_return,constructor),"Context wrapper constructor authority retired");
  constructor->impl_->port->require_completed();
  require(descriptor&&request&&declaration&&declaration->name=="LuaContext"&&
   request->origin_==runtime::detail::ContextLoaderRequestCustody::Origin::retained_lua_context_declaration&&
   State::Impl::same(request->declaration_,declaration)&&State::Impl::same(operand.request_,request)&&
   State::Impl::same(operand.source_,descriptor)&&operand.bundle_&&operand.generation_&&operand.serial_==serial&&
   operand.context_==actual->context&&request->context_==actual->context&&request->original_utf8_==operand.stem_,
   "Context wrapper SAME selected declaration/descriptor/Bank custody changed");
  const auto* filename=attribute_without_callbacks("FileName");
  require(filename&&*filename==request->original_utf8_&&!filename->empty(),"Context wrapper original FileName source changed");
  require(State::Impl::same(state->impl_->wrapper_context_realm_3c,expected_context_realm)&&
   State::Impl::same(actual->context_layout_baseline->base_.context_realm_3c,expected_context_realm)&&
   actual->context_layout_baseline->base_.context_3c_observation==(expected_context_realm?
    detail::NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm:
    detail::NotificationLayoutBaseline::Base::Context3CObservation::constructor_null),
   "Context wrapper physical3C/Base observed realm changed across callback");
  require(State::Impl::same(state->impl_->b4_value,b4)&&state->impl_->b4_revision==b4_revision&&
   actual->context_layout_baseline->sequence_==sequence&&actual->effect_revision==effects&&
   actual->context_layout_baseline->history_.size()==history_size&&actual->context_layout_baseline->base_.flags_40==flags,
   "Context wrapper actual B4/Base histories changed across callback");
 }
 void full()const {first();if(parent_guard){parent_guard->require_source_owned(*operation);first();}
  operation->require_current();first();}
 void accept_native_store_without_callbacks(){const auto actual=core.lock();require(actual!=nullptr,"Context wrapper store owner retired");
  expected_context_realm=state->impl_->wrapper_context_realm_3c;
  b4=state->impl_->b4_value;b4_revision=state->impl_->b4_revision;
  sequence=actual->context_layout_baseline->sequence_;effects=actual->effect_revision;
  history_size=actual->context_layout_baseline->history_.size();flags=actual->context_layout_baseline->base_.flags_40;}
};

void BnwWorldViewControlTree::parse_retained_context_xml_wrapper_prefix_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::NotificationContextRootReadChildrenLoan* root_cursor){
 using Frame=ContextXmlWrapperFrame;using State=Frame::State;
 using Wide=detail::NotificationContextNativeWideStringValue;
 std::optional<detail::NotificationContextRootReadChildrenLoan::Guard> parent_guard;
 if(root_cursor){parent_guard.emplace(root_cursor->capture_guard_without_callbacks());parent_guard->require_source_owned(operation);}
 const auto actual=core_;require(actual!=nullptr,"Context wrapper actual Core absent");
 const auto source=actual->source;require(source!=nullptr,"Context wrapper actual Source absent");
 const auto scope=source->capture_active_operation_scope_serial_without_callbacks(&operation);
 const auto cold=[&]{if(parent_guard)parent_guard->require_current_without_callbacks();
  source->require_operation_origin_without_callbacks(operation,scope);
  require(State::Impl::same(actual->source,source),"Context wrapper Source replaced before old operand");actual->layout_origin();
  require(actual->context_ordinary_state&&actual->context_ordinary_state->impl_&&actual->context_layout_baseline&&
   !actual->context_layout_baseline->parsed_source_serial_&&actual->context_xml_wrapper_frame.expired()&&
   actual->context_direct_load_frame.expired()&&actual->context_load_name_frame.expired()&&
   !actual->context_cursor_load_prefix&&!actual->context_lua_preparation,"Context wrapper requires genuine constructor once before parsing/loading");};
 cold();operation.require_current();cold();
 const auto descriptor=source->prepared_source(actual->context);cold();
 auto operand=operation.context_loader_resource_operand_owned(descriptor);cold();
 const auto request=operand.request_;
 require(request&&request->origin_==runtime::detail::ContextLoaderRequestCustody::Origin::retained_lua_context_declaration&&
  request->declaration_&&request->declaration_->name=="LuaContext",
  "required actual retained LuaContext declaration issuer; selected direct request is not wrapper input");
 const auto state=actual->context_ordinary_state;const auto constructor=state->impl_->native_constructor_return;
 require(constructor&&constructor->impl_&&state->impl_->b4_value,"Context wrapper complete native constructor/B4 absent");
 constructor->impl_->port->require_completed();
 const auto serial=operation.operation_serial();
 // Capture every physical field VALUE BEFORE make_shared/new_handler. The
 // constructor never dereferences mutable actual fields after that callback.
 const auto original_b4=state->impl_->b4_value;
 const auto original_revision=state->impl_->b4_revision;
 const auto original_sequence=actual->context_layout_baseline->sequence_,original_effects=actual->effect_revision;
 const auto original_history=actual->context_layout_baseline->history_.size();
 const auto original_flags=actual->context_layout_baseline->base_.flags_40;
 auto frame=std::make_shared<Frame>(actual,source,&operation,scope,serial,std::move(operand),descriptor,request,state,constructor,
  original_b4,original_revision,original_sequence,original_effects,original_history,original_flags,parent_guard);
 cold();operation.require_current();cold();actual->context_xml_wrapper_frame=frame;
 struct Exit{std::shared_ptr<Frame> frame;~Exit(){frame->alive.store(false);}}exit{frame};
 try{
  frame->full();
  //5C23C0 starts with empty FString/local cursor. On absent Style it goes
  // directly to8353F0 and sharedempty cleanup: no stylesheet read/v44 call.
  require(frame->attribute_without_callbacks("Style")==nullptr,
   "required present Style5C23C0 local/global stylesheet lookup and genuine recursive v44 before FileName");
  //4FEEB7/4FEEDE: actual retained cursor FileName string DATA, not root Name.
  const auto* filename=frame->attribute_without_callbacks("FileName");
  require(filename&&filename->size()<260&&std::all_of(filename->begin(),filename->end(),[](unsigned char ch){return ch>0&&ch<128;}),
   "required FileName CP65001 fixed260 conversion outside proved positive ASCII path");
  // Native zeroes0x206 bytes, converts explicit byte length into capacity260,
  // then544D50 scans its NUL. ASCII<260 produces exactly these UTF16 units.
  const auto text=*filename;frame->full(); // retained declaration pins source before allocating copy.
  std::u16string wide;wide.resize(text.size());frame->full();
  for(std::size_t i=0;i<text.size();++i)wide[i]=static_cast<unsigned char>(text[i]);
  auto pending=std::unique_ptr<Wide>(new Wide(std::move(wide)));frame->full();
  auto value=std::shared_ptr<const Wide>(std::move(pending));frame->full();
  require(!value->value_.empty()&&value->terminated_data_without_callbacks()[value->value_.size()]==u'\0'&&
   frame->b4_revision<UINT64_MAX,"Context wrapper544D50 positive value/NUL/revision unsupported");
  //544D50 genuine positive assignment; no731370/v10/NameLoan issuer here.
  const auto retired=frame->b4;frame->full();
  state->impl_->b4_value=value;++state->impl_->b4_revision;frame->accept_native_store_without_callbacks();frame->full();
  //409390 reads actual DeferLoad into owned FString. Empty query semantics
  // are not inferred; absent is proven nativefalse, nonempty first-byte exact.
  const auto* authored=frame->attribute_without_callbacks("DeferLoad");
  std::string deferred;if(authored){require(!authored->empty(),"required409390 empty DeferLoad query semantics");deferred=*authored;frame->full();}
  const bool defer=authored&&(deferred.front()=='t'||deferred.front()=='T'||deferred.front()=='1');
  if(defer){
   auto&base=*actual->context_layout_baseline;
   require(actual->effect_revision<UINT64_MAX&&actual->layout_sequence<UINT64_MAX&&base.sequence_<UINT64_MAX,
    "Context wrapper DeferLoad journal exhausted");
   base.history_.reserve(base.history_.size()+1);frame->full();
   base.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
   base.base_.flags_40|=0x10000000U;++actual->effect_revision;++actual->layout_sequence;
   frame->accept_native_store_without_callbacks();frame->full();
  }
  // Native438170 precedes physical3C=actualRealm and FString/cursor DTOR.
  // Source20 only supplies declaration DATA. It has no ordinary-parser realm
  // loan/current parent receiver, so no wrapper return or3C store is issued.
  if(!parent_guard)throw std::runtime_error("required4FEFA3 common438170 SAME retained declaration ordinary parser+actual Realm argument; then3C and temporary DTOR before wrapper return");
  complete_root_context_child_common_wrapper_owned(operation,frame);
 }catch(...){frame->failed.store(true);state->impl_->failed.store(true);actual->failed.store(true);throw;}
}
