//Lexical sole Tree issuer after Source5 actual514/2E4 return. No registry/load
//replay and no Host origin/bool substitutes for the genuine completedRead lane.
{
 using Completion=detail::NotificationContextRootReadCompletionSlot;
 using Input=detail::NotificationContextPostreadGlobalsInputLoan;
 const auto path=state->impl_->lua514_path_return;
 const auto read=actual->context_root_read_pending_completion;
 require(path&&read&&path->read.lock()==read&&!state->impl_->postread_globals_reference,
  "postRead Globals requires genuine path/watcher return and first local1C occurrence");
 const std::weak_ptr weak_actual=actual;
 const std::weak_ptr weak_state=state;
 const std::weak_ptr weak_source=owner;
 const std::weak_ptr weak_frame=frame;
 const std::weak_ptr weak_read=read;
 const std::weak_ptr weak_constructor=constructor;
 const std::weak_ptr weak_manager=manager;
 const auto*const operation_identity=&operation;
 const auto first=[weak_actual,weak_state,weak_source,weak_frame,weak_read,weak_constructor,weak_manager,
                   path,prepared,operation_identity,source_scope]{
  const auto source=weak_source.lock();const auto core=weak_actual.lock();const auto physical=weak_state.lock();
  const auto caller=weak_frame.lock();const auto completed_read=weak_read.lock();
  const auto current_constructor=weak_constructor.lock();const auto current_manager=weak_manager.lock();
  if(!source||!core||!physical||!caller||!completed_read||
   !current_constructor||!current_manager||
   caller->creator!=std::this_thread::get_id()||!caller->alive.load()||caller->failed.load())
   throw std::runtime_error("postRead Globals actual Tree/source/direct caller retired FIRST");
  const auto& current=source->current_operation_loan();
  if(&current!=operation_identity)throw std::runtime_error("postRead Globals current operation replaced FIRST");
  source->require_operation_origin_without_callbacks(current,source_scope);
  current.require_origin_without_callbacks();core->layout_origin();
  if(!State::Impl::same(core->source,source)||!State::Impl::same(caller->source,source)||
   caller->operation!=&current||caller->serial!=current.operation_serial()||caller->source_scope!=source_scope||
   core->context_direct_load_frame.lock()!=caller||!State::Impl::same(core->context_ordinary_state,physical)||
   !physical->impl_||physical->impl_->creator!=std::this_thread::get_id()||!physical->impl_->alive.load()||
   physical->impl_->failed.load()||!State::Impl::same(physical->impl_->native_constructor_return,current_constructor)||
   !State::Impl::same(core->context_gui_port,current_manager)||
   !State::Impl::same(core->context_lua_preparation,prepared)||!State::Impl::same(caller->prepared,prepared)||
   !State::Impl::same(core->context_root_read_pending_completion,completed_read)||!core->context_layout_baseline)
   throw std::runtime_error("postRead Globals SAME current constructor/Context/VM518 changed");
  const auto guard=Completion::Guard(completed_read->impl_);
  guard.require_completed_current_without_callbacks();
  const auto receiver=core->context_layout_baseline->base_.receiver_owned;
  if(!receiver)throw std::runtime_error("postRead Globals actual root receiver absent");
  guard.require_actual_root_receiver_without_callbacks(*receiver);
  if(physical->impl_->lua514_path_return!=path||path->creator!=std::this_thread::get_id()||
   path->read.lock()!=completed_read||path->serial!=current.operation_serial()||path->scope!=source_scope||
   path->prefix!=physical->impl_->lua_name514_prefix_return||!path->temporary||!path->physical||!path->bytes||
   !State::Impl::same(path->value,physical->impl_->file_514_value)||physical->impl_->file_514_revision!=3||
   !State::Impl::same(path->watcher,physical->impl_->file_watcher_2e4)||!path->watcher||
   core->topology_revision!=path->topology||core->effect_revision!=path->effects||
   core->layout_sequence!=path->sequence||core->context_layout_baseline->sequence_!=path->root_sequence)
   throw std::runtime_error("postRead Globals real Lua514/2E4 return/current histories changed");
  path->physical->require_current_without_callbacks();
  if(!path->watcher->field||!path->watcher->field->watcher)
   throw std::runtime_error("postRead Globals actual2E4 held watcher allocation retired");
  path->watcher->field->watcher->require_reached_without_callbacks();
 };
 const auto full=[first,weak_source,weak_read,path,operation_identity]{
  first();const auto source=weak_source.lock();const auto read=weak_read.lock();
  if(!source||!read)throw std::runtime_error("postRead Globals actual Source/Read retired before FULL");
  const auto&current=source->current_operation_loan();
  if(&current!=operation_identity)throw std::runtime_error("postRead Globals replaced operation before FULL");
  const auto guard=Completion::Guard(read->impl_);
  guard.require_source_owned(current);first();
  const auto bank=current.selected_provider_startup_bank_owned();first();
  path->physical->require_same_bank_without_callbacks(*bank);first();
 };
 full();
 //Private constructor is reachable only here, after all required ordered effects.
 const auto input_loan=std::shared_ptr<const Input>(new Input(weak_actual,prepared,weak_state,first,full));full();
 const auto globals=host.acquire_context_postread_globals_reference_owned(operation,input_loan);full();
 require(globals!=nullptr,"postRead Globals Host returned no genuine owning local refs");
 host.require_context_postread_globals_reference_owned(operation,*globals);full();
 require(!state->impl_->postread_globals_reference,"postRead Globals canonical Context field publication replay");
 state->impl_->postread_globals_reference=globals;full();
 host.require_context_postread_globals_reference_owned(operation,*globals);full();
 const auto root_cdata=host.create_context_postread_root_cdata_owned(operation,globals);full();
 require(root_cdata!=nullptr,"postRead42F060 actual current root Context reflection return absent");
 host.require_context_postread_root_cdata_owned(operation,*root_cdata);full();
 require(!state->impl_->postread_root_cdata_return,"postRead current root Context reflection occurrence replay");
 state->impl_->postread_root_cdata_return=root_cdata;full();
 host.require_context_postread_root_cdata_owned(operation,*root_cdata);full();
  //Capture the real457652 GUI and4576B6 embedded TT value operands OUTER.
  const auto ui_manager_value=std::shared_ptr<const detail::NotificationContextUiManagerValueLoan>(
   new detail::NotificationContextUiManagerValueLoan(manager,owner,operation,first,full));full();
  const auto tt_manager_operand=manager->capture_tooltip_manager_operand_owned(operation);full();
  //Actual raw lookup/copy/local-DTOR runs only after the SAME root creator.
  root_cdata->acquire_contextptr_lookup_owned(operation,ui_manager_value,tt_manager_operand);full();
  root_cdata->require_contextptr_lookup_owned(operation);full();
  root_cdata->register_postread_environment_owned(operation);full();
  root_cdata->require_contextptr_lookup_owned(operation);full();
  //Ordered actual registrar returns precede5761 and the direct suffix.

}
