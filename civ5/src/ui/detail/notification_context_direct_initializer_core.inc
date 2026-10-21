// AFTER the complete genuine DirectLoadFrame and RootRead completion bodies.
// Direct root has no legacy parser occurrence. Actual ordinary child parsers
// and the original pertype child+10 dispatcher are retained unchanged.
std::shared_ptr<BnwWorldViewControlTree::Core>
BnwWorldViewControlTree::require_context_direct_initialization_input_owned(
 const std::shared_ptr<Core>& actual,const std::shared_ptr<ContextDirectLoadFrame>& caller,
 const std::shared_ptr<const detail::NotificationContextRootReadCompletionSlot>& read){
 require(actual&&caller&&read,"Direct5761 actual caller/Read/receiver absent");
 caller->first();require(caller->core.lock()==actual&&actual->context_direct_load_frame.lock()==caller&&
  actual->context_root_read_pending_completion==read&&actual->context_layout_baseline&&
  !actual->context_layout_baseline->parsed_source_serial_&&!actual->layout_context_initialization_origin&&
  !actual->layout_initialization_return&&!actual->context_initialization_active,
  "Direct5761 must start at the SAME completedRead/nonparser/new initializer occurrence");
 const auto guard=detail::NotificationContextRootReadCompletionSlot::Guard(read->impl_);
 guard.require_source_owned(*caller->operation);caller->first();
 const auto receiver=actual->context_layout_baseline->base_.receiver_owned;
 require(receiver!=nullptr,"Direct5761 actual root receiver retired");
 guard.require_actual_root_receiver_without_callbacks(*receiver);
 require(actual->context_construction_return&&caller->constructor&&
  actual->context_ordinary_state==caller->state&&
  caller->state->impl_->native_constructor_return==caller->constructor&&
  read->impl_->state==caller->state&&read->impl_->receiver==actual->context_layout_baseline->base_.receiver_owned,
  "Direct5761 actual native constructor/physical Read receiver differs");
 caller->full();return actual;
}

BnwWorldViewControlTree::LayoutContextInitializationOrigin::LayoutContextInitializationOrigin(
 const std::shared_ptr<Core>& actual,
 const std::shared_ptr<ContextDirectLoadFrame>& caller,
 const std::shared_ptr<const detail::NotificationContextRootReadCompletionSlot>& read)
 :tree_(require_context_direct_initialization_input_owned(actual,caller,read)),
  topology_(actual->topology_revision),parser_serial_(0),
  initial_size_(actual->context_layout_baseline->base_.size_30_34),
  native_zero_(actual->context_gui_port->arithmetic_pair_without_callbacks(*caller->operation)),
  construction_(actual->context_construction_return),direct_read_(read),direct_caller_(caller),
  direct_constructor_(caller->constructor),direct_scope_(caller->source_scope),direct_serial_(caller->serial),
  direct_before_effects_(caller->effects),direct_before_sequence_(caller->sequence){
 require_context_direct_initialization_input_owned(actual,caller,read);
}

void BnwWorldViewControlTree::require_context_direct_initialization_origin_owned(
 const std::shared_ptr<Core>& actual,const std::shared_ptr<const LayoutContextInitializationOrigin>& origin){
 require(actual&&origin&&origin->direct_read_&&origin->direct_caller_&&origin->direct_constructor_,
  "Direct5761 genuine typed constructor/Read history absent");
 actual->layout_origin();const auto caller=origin->direct_caller_;
 require(caller->creator==std::this_thread::get_id()&&!caller->failed.load()&&caller->core.lock()==actual&&
  actual->context_direct_load_frame.lock()==caller&&actual->source==caller->source&&
  actual->context_ordinary_state==caller->state&&actual->context_gui_port==caller->manager&&
  actual->context_lua_preparation==caller->prepared&&actual->context_cursor_load_prefix==caller->cursor&&
  actual->context_construction_return==origin->construction_&&origin->tree_.lock()==actual&&
  origin->topology_==actual->topology_revision&&origin->parser_serial_==0&&
  actual->context_layout_baseline&&!actual->context_layout_baseline->parsed_source_serial_&&
  caller->constructor==origin->direct_constructor_&&caller->state->impl_->native_constructor_return==origin->direct_constructor_&&
  caller->source_scope==origin->direct_scope_&&caller->serial==origin->direct_serial_&&
  actual->context_root_read_pending_completion==origin->direct_read_,
  "Direct5761 historical constructor/Read/caller/VM identity changed");
 // The old loader afterstate is not renewed across owned5761 or original Lua.
 // RootRead's physical history checks already allow subsequent initialization.
 caller->source->require_operation_origin_without_callbacks(*caller->operation,origin->direct_scope_);
 require(caller->operation->operation_serial()==origin->direct_serial_,"Direct5761 actual Source serial changed");
 const auto guard=detail::NotificationContextRootReadCompletionSlot::Guard(origin->direct_read_->impl_);
 guard.require_completed_current_without_callbacks();
 const auto receiver=actual->context_layout_baseline->base_.receiver_owned;
 require(receiver!=nullptr,"Direct5761 historical root receiver retired");
 guard.require_actual_root_receiver_without_callbacks(*receiver);
 origin->direct_constructor_->impl_->port->require_completed();actual->layout_origin();
}

void BnwWorldViewControlTree::require_context_direct_initialization_history_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn>& returned)const{
 const auto actual=core_;actual->layout_origin();
 const auto source=actual->source;
 const auto first=[&]{
  actual->layout_origin();
  const auto& fresh=source->current_operation_loan();
  require(&fresh==&operation,"Direct5761 history requires the fresh canonical Source operation");
  fresh.require_origin_without_callbacks();actual->layout_origin();
  require(actual->source==source,"Direct5761 history canonical Source replaced");
 };
 first();
 require(returned&&returned->impl_&&actual->layout_initialization_return&&
  actual->layout_initialization_return->native_return_==returned,"Direct5761 historical return replaced");
 const auto& done=*returned->impl_;const auto origin=done.origin;
 require(origin&&origin->direct_read_&&origin->direct_caller_&&origin->direct_constructor_,
  "Direct5761 historical typed origin absent");
 const auto caller=origin->direct_caller_;
 const auto physical=[&]{
 first();
 require(done.lifetime&&done.lifetime->creator==std::this_thread::get_id()&&!done.lifetime->failed.load()&&
  done.lifetime->destroyed&&done.lifetime->destruction_passed&&!actual->context_initialization_active&&
  done.core.lock()==actual&&origin&&origin==actual->layout_context_initialization_origin&&
  origin->direct_read_&&origin->direct_caller_&&origin->direct_constructor_&&
  done.source_serial==origin->direct_serial_&&origin->direct_serial_!=0&&origin->direct_scope_!=0,
  "Direct5761 real body/DTOR/historical Source custody changed");
 const auto r=origin->direct_read_->impl_;
 require(caller->creator==std::this_thread::get_id()&&!caller->failed.load()&&caller->core.lock()==actual&&
  actual->context_direct_load_frame.lock()==caller&&caller->source==source&&
  actual->context_ordinary_state==caller->state&&actual->context_gui_port==caller->manager&&
  actual->context_lua_preparation==caller->prepared&&actual->context_cursor_load_prefix==caller->cursor&&
  actual->context_construction_return==origin->construction_&&origin->tree_.lock()==actual&&
  origin->topology_==actual->topology_revision&&origin->parser_serial_==0&&
  actual->context_layout_baseline&&!actual->context_layout_baseline->parsed_source_serial_&&
  caller->constructor==origin->direct_constructor_&&caller->state->impl_&&
  caller->state->impl_->creator==std::this_thread::get_id()&&caller->state->impl_->alive.load()&&
  !caller->state->impl_->failed.load()&&caller->state->impl_->core.lock()==actual&&caller->state->impl_->source.lock()==source&&
  caller->state->impl_->native_constructor_return==origin->direct_constructor_&&
  origin->direct_constructor_->impl_&&origin->direct_constructor_->impl_->creator==std::this_thread::get_id()&&
  origin->direct_constructor_->impl_->alive.load()&&!origin->direct_constructor_->impl_->failed.load()&&
  origin->direct_constructor_->impl_->port&&
  caller->source_scope==origin->direct_scope_&&caller->serial==origin->direct_serial_&&
  r&&r->creator==std::this_thread::get_id()&&r->alive.load()&&r->core.lock()==actual&&r->source==source&&
  r->canonical.lock()==origin->direct_read_&&actual->context_root_read_pending_completion==origin->direct_read_&&
  r->scope==origin->direct_scope_&&r->serial==origin->direct_serial_&&!r->staged_return&&r->completed_return,
  "Direct5761 historical constructor/Read/caller/VM identity changed");
 // These are authentic immutable after-DTOR fields retained by the actual
 // private Read publisher. No expired Read/cursor/caller guard is renewed.
 const auto completed=r->completed_return;const auto frame=completed->returned_frame;
 const auto e=completed->returned_cursor;
 using Cleanup=ContextRootReadFileFrame::Cleanup;
 static constexpr std::array expected{Cleanup::cursor34_8353f0,Cleanup::string18_retire,Cleanup::lua2c_8f8820,
  Cleanup::cursor10_8353f0,Cleanup::wide280_686f10,Cleanup::wide68_686f10};
 using CursorType=context_root_read_temporary_detail::XmlCursor::Type;
 using NameLife=context_root_read_temporary_detail::NameString::Life;
 require(frame&&e&&frame==r->actual_frame.lock()&&e==r->actual_cursor.lock()&&
  completed->actual_pending.lock()==origin->direct_read_&&completed->cleanup_effects==expected&&
  !frame->alive.load()&&!frame->failed.load()&&!e->alive.load()&&!e->failed.load()&&
  frame->creator==std::this_thread::get_id()&&e->creator==std::this_thread::get_id()&&
  e->frame.lock()==frame&&e->core.lock()==actual&&frame->core.lock()==actual&&
  e->pending_completion.lock()==origin->direct_read_&&e->ordinal==e->child_count&&
  e->cleanup_count==expected.size()&&frame->cleanup_count==expected.size()&&
  e->cleanup_effects==expected&&frame->cleanup_effects==expected&&
  !e->current_instance&&!e->current_style&&e->document==frame->expected_xml84&&
  e->document&&e->document->storage&&e->document->storage->document&&
  e->document->storage->document->children.size()==e->child_count&&
  frame->effect_count==frame->effects.size()&&frame->effects[0]==ContextRootReadFileFrame::Effect::copy_b4_to_510&&
  frame->effects[1]==ContextRootReadFileFrame::Effect::append_xml&&
  r->state==actual->context_ordinary_state&&frame->state.lock()==r->state&&
  r->state->impl_->alive.load()&&!r->state->impl_->failed.load()&&
  r->state->impl_->native_constructor_return==frame->constructor&&frame->constructor==origin->direct_constructor_&&
  r->state->impl_->b4_value==frame->actual_b4&&r->state->impl_->b4_revision==frame->b4_revision&&
  r->state->impl_->file_510_value==frame->expected_510&&r->state->impl_->xml84_current==frame->expected_xml84&&
  r->state->impl_->file_watcher_b8==frame->expected_watcher_b8&&
  actual->context_layout_baseline->base_.receiver_owned==r->receiver&&e->receiver==r->receiver&&frame->receiver==r->receiver&&
  e->cursor34.type==CursorType::retired_base_b76284&&!e->cursor34.selected&&
  e->cursor10.type==CursorType::retired_base_b76284&&e->name_string18.life==NameLife::retired_canonical_empty&&
  e->name_string18.value.empty()&&e->named_retirement&&frame->temporary280&&frame->temporary68&&
  actual->context_layout_children&&actual->context_root_read_file_frame.expired()&&!actual->busy,
  "Direct5761 historical real Read/frameDTOR/declarations/physical fields/six-cleanup custody changed");
 frame->temporary280->require_retired_without_callbacks();frame->temporary68->require_retired_without_callbacks();
 require(e->cursor_phase==context_root_read_temporary_detail::CursorPhase::exhausted&&
  e->cursor10.selected==(e->child_count?&e->document->storage->document->children.back():e->document->storage->document.get())&&
  frame->temporary68->origin==ContextRootReadFileFrame::PathTemporary::Origin::selected_loose7493b0&&
  frame->temporary280->origin==ContextRootReadFileFrame::PathTemporary::Origin::watch_path7a0030&&
  frame->temporary68->value==frame->expected_510&&frame->temporary280->value==frame->expected_510&&
  frame->temporary68->physical==e->document->physical&&frame->temporary280->physical==e->document->physical,
  "Direct5761 historical Read XML/path/String cleanup changed");
 require_context_root_node_returns_without_callbacks(*e);
 origin->direct_constructor_->impl_->port->require_completed();first();
 };
 physical();
 // Borrow the retained real coroutine/environment through the fresh live VM
 // lender. The saved serial authenticates its creation, not today's loan.
 const auto& host=operation.context_lua_host_owned();first();
 host.require_context_lua_state_history_owned(operation,caller->prepared,actual,caller->cursor,origin->direct_serial_);physical();
 operation.require_current();physical();
 // Deliberately no historical effect/geometry equality or old operation
 // dereference. Later Lua/GC/paint stages require their own actual journals.
}

std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn>
BnwWorldViewControlTree::initialize_context_direct_5761_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationContextRootReadCompletionSlot>& read){
 const auto actual=core_;actual->layout_origin();
 const auto caller=actual->context_direct_load_frame.lock();
 try{
  require_context_direct_initialization_input_owned(actual,caller,read);
  require(caller->operation==&operation,"Direct5761 offered another actual Source operation");
  auto origin=std::shared_ptr<const LayoutContextInitializationOrigin>(
   new LayoutContextInitializationOrigin(actual,caller,read));
  require_context_direct_initialization_input_owned(actual,caller,read);
  actual->layout_context_initialization_origin=origin;
  require_context_direct_initialization_origin_owned(actual,origin);
  initialize_context_5761_owned(operation); // SAME native body, every real child+10 and producer DTOR.
  actual->layout_origin();const auto seal=actual->layout_initialization_return;
  require(seal&&seal->native_return_&&seal->native_return_->impl_,"Direct5761 did not return its actual closed seal");
  const auto returned=seal->native_return_;const auto& done=*returned->impl_;
  require_context_initialization_return_owned(actual,returned);
  require(done.origin==origin&&caller->effects==origin->direct_before_effects_&&
   caller->sequence==origin->direct_before_sequence_,"Direct5761 caller expected preimage changed outside owned body");
  const detail::NotificationLayoutContextInitializationReturn::Impl::Domain* root_domain=nullptr;
  for(const auto& domain:done.domains)if(domain.core.lock()==actual){
   require(!root_domain,"Direct5761 duplicate actual root domain");root_domain=&domain;}
  require(root_domain&&root_domain->before_effects==caller->effects&&root_domain->topology==actual->topology_revision&&
   root_domain->effects==actual->effect_revision,"Direct5761 closed returned root before/after journal differs");
  const auto root_member=std::find_if(done.members.begin(),done.members.end(),[](const auto& value){return value.id==0;});
  require(root_member!=done.members.end()&&root_member->parser_serial==0&&
   root_member->initialized_sequence==actual->context_layout_baseline->sequence_,
   "Direct5761 genuine nonparser root member/sequence changed");
  // Only this actual closed body/child-return cohort advances the loader's
  // expected journal. No callback-supplied numeric POST observation is used.
  caller->effects=root_domain->effects;caller->sequence=root_member->initialized_sequence;
  caller->flags=actual->context_layout_baseline->base_.flags_40;
  caller->size=actual->context_layout_baseline->base_.size_30_34;
  caller->full();require_context_direct_initialization_history_owned(operation,returned);
  return returned;
 }catch(...){if(caller)caller->failed.store(true);actual->failed.store(true);throw;}
}
