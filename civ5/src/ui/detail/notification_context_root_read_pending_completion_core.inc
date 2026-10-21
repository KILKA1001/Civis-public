// Genuine actual RootRead pending DATA. No completed stamp is issued here.
// The separately owned fullRootRead producer must execute every declaration,
// native temporary cleanup and final SourcePOST before installing its typed
// CompletedReturn. An absent completed return remains an initialization error.
struct detail::NotificationContextRootReadCompletionSlot::Impl final {
 using Tree=BnwWorldViewControlTree;
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true};
 const std::weak_ptr<Tree::Core> core;
 const std::weak_ptr<Tree::ContextRootReadFileFrame> actual_frame;
 const std::weak_ptr<Tree::ContextRootChildrenEvidence> actual_cursor;
 const std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
 const std::shared_ptr<NotificationLayoutContextOrdinaryState> state;
 const std::shared_ptr<const NotificationLayoutOrdinaryReceiver> receiver;
 const runtime::BnwWorldViewSourceOperationLoan*const operation;
 const std::uint64_t scope,serial;
 std::weak_ptr<const NotificationContextRootReadCompletionSlot> canonical;
 struct CompletedReturn final {
  using Cleanup=Tree::ContextRootReadFileFrame::Cleanup;
  const std::shared_ptr<Tree::ContextRootReadFileFrame> returned_frame;
  const std::shared_ptr<Tree::ContextRootChildrenEvidence> returned_cursor;
  const std::weak_ptr<const NotificationContextRootReadCompletionSlot> actual_pending;
  const std::array<Cleanup,6> cleanup_effects;
  CompletedReturn(std::shared_ptr<Tree::ContextRootReadFileFrame> f,std::shared_ptr<Tree::ContextRootChildrenEvidence> e,
   std::weak_ptr<const NotificationContextRootReadCompletionSlot> p,std::array<Cleanup,6> effects)
   :returned_frame(std::move(f)),returned_cursor(std::move(e)),actual_pending(std::move(p)),cleanup_effects(effects){}
 };
 std::shared_ptr<const CompletedReturn> staged_return; // DATA before E/frame DTOR, no completed Guard
 std::shared_ptr<const CompletedReturn> completed_return; // actual after-DTOR issuer in owned completion body
 Impl(std::shared_ptr<Tree::Core> c,std::shared_ptr<Tree::ContextRootReadFileFrame> f,
  std::shared_ptr<Tree::ContextRootChildrenEvidence> e,std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> owner,
  std::shared_ptr<NotificationLayoutContextOrdinaryState> s,std::shared_ptr<const NotificationLayoutOrdinaryReceiver> r,
  const runtime::BnwWorldViewSourceOperationLoan*op,std::uint64_t actual_scope,std::uint64_t current)
  :core(std::move(c)),actual_frame(std::move(f)),actual_cursor(std::move(e)),source(std::move(owner)),
   state(std::move(s)),receiver(std::move(r)),operation(op),scope(actual_scope),serial(current){}
};
detail::NotificationContextRootReadCompletionSlot::NotificationContextRootReadCompletionSlot(std::shared_ptr<Impl> p):impl_(std::move(p)){}
detail::NotificationContextRootReadCompletionSlot::~NotificationContextRootReadCompletionSlot(){impl_->alive.store(false);}
detail::NotificationContextRootReadCompletionSlot::Guard::Guard(std::shared_ptr<Impl> p):impl_(std::move(p)){}
detail::NotificationContextRootReadCompletionSlot::Guard
detail::NotificationContextRootReadCompletionSlot::capture_pending_data_guard_without_callbacks()const{
 const auto p=impl_;BnwWorldViewControlTree::require_context_root_read_pending_slot_without_callbacks(*p);return Guard(p);
}
void BnwWorldViewControlTree::require_context_root_read_pending_slot_without_callbacks(
 const detail::NotificationContextRootReadCompletionSlot::Impl& r){
 require(r.creator==std::this_thread::get_id()&&r.alive.load()&&r.operation,
  "RootRead pending slot retired/foreign FIRST");
 const auto frame=r.actual_frame.lock();const auto e=r.actual_cursor.lock();
 require(frame&&e,"RootRead pending actual frame/cursor retired before completed return");
 e->first(); // SAME real lexical Root Source/currentScope before old operation
 const auto actual=r.core.lock();const auto current=r.canonical.lock();
 require(actual&&current&&current->impl_.get()==&r&&actual->context_root_read_pending_completion==current&&
  e->core.lock()==actual&&e->frame.lock()==frame&&frame->operation==r.operation&&e->operation==r.operation&&
  actual->source==r.source&&actual->context_ordinary_state==r.state&&frame->state.lock()==r.state&&
  e->receiver==r.receiver&&frame->receiver==r.receiver&&!r.completed_return,
  "RootRead pending slot SAME actual entered owner/canonical/receiver changed");
 r.source->require_operation_origin_without_callbacks(*r.operation,r.scope);
 require(r.operation->operation_serial()==r.serial,"RootRead pending slot actual operation serial changed");e->first();
}
void BnwWorldViewControlTree::require_context_root_read_completed_slot_without_callbacks(
 const detail::NotificationContextRootReadCompletionSlot::Impl& r){
 require(r.creator==std::this_thread::get_id()&&r.alive.load()&&r.operation,
  "RootRead completed slot retired/foreign FIRST");
 const auto actual=r.core.lock();const auto current=r.canonical.lock();
 require(actual&&current&&actual->creator==r.creator&&actual->facade_alive.load()&&!actual->failed.load()&&
  actual->source==r.source&&current->impl_.get()==&r&&actual->context_root_read_pending_completion==current,
  "RootRead completed slot actual canonical Source/owner retired FIRST");
 // This validates canonical actual Source/scope BEFORE the historical raw op.
 r.source->require_operation_origin_without_callbacks(*r.operation,r.scope);
 require_context_root_read_terminal_without_callbacks(r);
 const auto completed=r.completed_return;
 require(!r.staged_return&&completed!=nullptr,"required genuine whole RootRead return after all declarations/native cleanup/LAST FULL; pending DATA cannot initialize");
 using Cleanup=detail::NotificationContextRootReadCompletionSlot::Impl::CompletedReturn::Cleanup;
 static constexpr std::array expected{Cleanup::cursor34_8353f0,Cleanup::string18_retire,Cleanup::lua2c_8f8820,
  Cleanup::cursor10_8353f0,Cleanup::wide280_686f10,Cleanup::wide68_686f10};
 const auto frame=completed->returned_frame;const auto e=completed->returned_cursor;
 require(frame&&e&&frame==r.actual_frame.lock()&&e==r.actual_cursor.lock()&&
  completed->actual_pending.lock()==current&&completed->cleanup_effects==expected&&
  !frame->alive.load()&&!frame->failed.load()&&!e->alive.load()&&!e->failed.load()&&
  e->frame.lock()==frame&&e->core.lock()==actual&&frame->core.lock()==actual&&
  e->operation==r.operation&&frame->operation==r.operation&&e->ordinal==e->child_count&&
  e->document==frame->expected_xml84&&e->document&&e->document->storage&&e->document->storage->document&&
  e->document->storage->document->children.size()==e->child_count&&
  frame->effect_count==frame->effects.size()&&frame->effects[0]==ContextRootReadFileFrame::Effect::copy_b4_to_510&&
  frame->effects[1]==ContextRootReadFileFrame::Effect::append_xml&&
  actual->context_ordinary_state==r.state&&frame->state.lock()==r.state&&r.state&&r.state->impl_&&
  r.state->impl_->alive.load()&&!r.state->impl_->failed.load()&&r.state->impl_->native_constructor_return==frame->constructor&&
  r.state->impl_->b4_value==frame->actual_b4&&r.state->impl_->b4_revision==frame->b4_revision&&
  r.state->impl_->file_510_value==frame->expected_510&&r.state->impl_->xml84_current==frame->expected_xml84&&
  r.state->impl_->file_watcher_b8==frame->expected_watcher_b8&&actual->context_layout_baseline&&
  actual->context_layout_baseline->base_.receiver_owned==r.receiver&&e->receiver==r.receiver&&
  r.operation->operation_serial()==r.serial,
  "RootRead completed typed return/real frame DTOR/all declarations/physical Context/cleanup journal changed");
 frame->constructor->impl_->port->require_completed();
}
void detail::NotificationContextRootReadCompletionSlot::Guard::require_completed_current_without_callbacks()const{
 const auto p=impl_;require(p!=nullptr,"RootRead completion guard actual slot absent");
 BnwWorldViewControlTree::require_context_root_read_completed_slot_without_callbacks(*p);
}
void detail::NotificationContextRootReadCompletionSlot::Guard::require_actual_root_receiver_without_callbacks(
 const NotificationLayoutOrdinaryReceiver& receiver)const{
 require_completed_current_without_callbacks();require(impl_->receiver.get()==&receiver,"RootRead completion substituted actual root receiver");
}
void detail::NotificationContextRootReadCompletionSlot::Guard::require_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& op)const{
 require_completed_current_without_callbacks();require(impl_->operation==&op,"RootRead completion offered foreign current Source");
 op.require_current();require_completed_current_without_callbacks();
}
std::shared_ptr<const detail::NotificationContextRootReadCompletionSlot>
BnwWorldViewControlTree::capture_pending_context_root_read_completion_owned(
 const detail::NotificationContextRootReadChildrenLoan::Guard& read){
 using Slot=detail::NotificationContextRootReadCompletionSlot;
 read.require_current_without_callbacks();const auto e=read.impl_->evidence;const auto actual=e->core.lock();
 const auto frame=e->frame.lock();require(actual&&frame,"pending RootRead slot real lexical frame absent");
 const auto first=[&]{read.require_current_without_callbacks();require(e->frame.lock()==frame&&e->core.lock()==actual&&
  frame->core.lock()==actual&&frame->operation==e->operation,"pending RootRead slot actual cursor/frame changed");};
 const auto full=[&]{first();read.require_source_owned(*e->operation);first();};
 full();
 if(const auto prior=actual->context_root_read_pending_completion){
  require(prior->impl_&&prior->impl_->actual_cursor.lock()==e,"pending RootRead slot belongs to other actual read");
  require_context_root_read_pending_slot_without_callbacks(*prior->impl_);return prior;
 }
 const auto source=actual->source;const auto state=actual->context_ordinary_state;const auto receiver=e->receiver;
 const auto scope=source->capture_active_operation_scope_serial_without_callbacks(e->operation);
 const auto serial=e->operation->operation_serial();full();
 auto fields=std::make_shared<Slot::Impl>(actual,frame,e,source,state,receiver,e->operation,scope,serial);full();
 auto returned=std::shared_ptr<const Slot>(new Slot(fields));full();
 require(!actual->context_root_read_pending_completion,"pending RootRead slot canonical changed during allocation");
 fields->canonical=returned;actual->context_root_read_pending_completion=returned;e->pending_completion=returned;
 require_context_root_read_pending_slot_without_callbacks(*fields);full();require_context_root_read_pending_slot_without_callbacks(*fields);
 return returned; // pending only; no completed stamp producer in this leaf
}
