// Real selected child4FD460 entry. Host owns 664FB0/6768B0 Lua operations;
// this leaf publishes their sealed physical operands, never an export return.
struct BnwWorldViewControlTree::ContextChildControlsFrame final {
 using State=detail::NotificationLayoutContextOrdinaryState;
 using Controls=detail::NotificationContextLuaControlsNamedReference;
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true},failed{false};
 const std::weak_ptr<ContextRootChildAttempt> attempt;
 const detail::NotificationContextRootReadChildrenLoan::Guard read;
 const std::shared_ptr<const detail::NotificationContextRootChildFactoryReturn> factory;
 const std::shared_ptr<const Controls> controls;
 const Controls::Guard controls_guard;
 const std::shared_ptr<const detail::NotificationLayoutControlID58> id58;
 const std::shared_ptr<State> context;
 const std::shared_ptr<State> parent_context;
 const std::shared_ptr<const detail::NotificationContextNativeWideStringValue> parent_b4;
 const std::uint64_t parent_b4_revision;
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> receiver;
 const std::shared_ptr<const runtime::ScriptUiChildSource> source;
 const runtime::BnwWorldViewSourceOperationLoan*const operation;
 ContextChildControlsFrame(std::weak_ptr<ContextRootChildAttempt> a,
  detail::NotificationContextRootReadChildrenLoan::Guard g,
  std::shared_ptr<const detail::NotificationContextRootChildFactoryReturn> returned,
  std::shared_ptr<const Controls> refs,Controls::Guard rg,
  std::shared_ptr<const detail::NotificationLayoutControlID58> id,
  std::shared_ptr<State> physical,std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> r,
  std::shared_ptr<const runtime::ScriptUiChildSource> descriptor,std::shared_ptr<State> actual_parent,
  std::shared_ptr<const detail::NotificationContextNativeWideStringValue> screen,std::uint64_t screen_revision,
  const runtime::BnwWorldViewSourceOperationLoan*op)
  :attempt(std::move(a)),read(std::move(g)),factory(std::move(returned)),controls(std::move(refs)),controls_guard(std::move(rg)),
   id58(std::move(id)),context(std::move(physical)),parent_context(std::move(actual_parent)),
   parent_b4(std::move(screen)),parent_b4_revision(screen_revision),receiver(std::move(r)),source(std::move(descriptor)),operation(op){}
 void first()const {
  require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load()&&operation,
   "child v64 lexical frame retired/failed/foreign FIRST");
  read.require_current_without_callbacks();
  read.require_child_factory_return_identity_without_callbacks(*factory);
  const auto a=attempt.lock();require(a&&a->child_core&&a->child,
   "child v64 actual retained Context allocation retired");
  const auto root=read.impl_->evidence;root->first();const auto read_frame=root->frame.lock();
  require(read_frame!=nullptr,"child v64 actual parent read frame retired");
  controls_guard.require_current_without_callbacks(*operation,read_frame->prepared,root->defaults);
  require(root->controls==controls&&a->factory==factory&&a->child_core->context_child_controls_frame.lock().get()==this&&
   a->descriptor==source&&a->receiver==receiver&&a->child_core->context_ordinary_state==context&&
   a->child_core->context_layout_baseline->base_.id_58==id58&&id58&&
   id58->length>0&&id58->length<id58->bytes.size()&&id58->bytes[id58->length]=='\0'&&
   context&&context->impl_&&context->impl_->wrapper_context_realm_3c==a->realm&&
   a->realm&&a->realm->impl_&&a->realm->impl_->context.lock()==root->core.lock()&&
   parent_context&&parent_context->impl_&&parent_context==read_frame->state.lock()&&
   parent_context->impl_->b4_value==parent_b4&&parent_b4&&parent_context->impl_->b4_revision==parent_b4_revision&&
   a->wrapper&&a->wrapper->children&&a->wrapper->children->impl_->ordered.empty(),
   "child v64 SAME parent Controls/real child ID58/3C/zero78 changed");
 }
 void full()const {
  first();read.require_source_owned(*operation);first();
  const auto root=read.impl_->evidence;const auto read_frame=root->frame.lock();
  require(read_frame!=nullptr,"child v64 parent read frame retired before FULL");
  const auto prepared=read_frame->prepared;const auto defaults=root->defaults;
  controls_guard.require_current_owned(*operation,prepared,defaults);first();
 }
 const char* terminated_id_without_callbacks()const {first();return id58->bytes.data();}
 std::u16string_view parent_screen_b4_without_callbacks()const {first();
  return {parent_b4->terminated_data_without_callbacks(),parent_b4->value_.size()};}
};

struct detail::NotificationContextChildControlsExportLoan::Impl final {
 const std::shared_ptr<BnwWorldViewControlTree::ContextChildControlsFrame> frame;
 explicit Impl(std::shared_ptr<BnwWorldViewControlTree::ContextChildControlsFrame> f):frame(std::move(f)){}
};
detail::NotificationContextChildControlsExportLoan::NotificationContextChildControlsExportLoan(std::shared_ptr<Impl> p):impl_(std::move(p)){}
detail::NotificationContextChildControlsExportLoan::~NotificationContextChildControlsExportLoan()=default;
detail::NotificationContextChildControlsExportLoan::Guard::Guard(std::shared_ptr<Impl> p):impl_(std::move(p)){}
detail::NotificationContextChildControlsExportLoan::Guard
detail::NotificationContextChildControlsExportLoan::capture_guard_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan& operation)const {
 const auto saved=impl_;Guard result(saved);result.require_current_without_callbacks(operation);return result;
}
void detail::NotificationContextChildControlsExportLoan::Guard::require_current_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan& operation)const {
 const auto saved=impl_;require(saved&&saved->frame,"child controls operand physical frame absent");
 saved->frame->first();require(saved->frame->operation==&operation,"child controls operand offered foreign Source");
}
void detail::NotificationContextChildControlsExportLoan::Guard::require_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation)const {
 const auto saved=impl_;require_current_without_callbacks(operation);saved->frame->full();require_current_without_callbacks(operation);
}
std::shared_ptr<const detail::NotificationContextLuaControlsNamedReference>
detail::NotificationContextChildControlsExportLoan::Guard::parent_controls_without_callbacks()const {impl_->frame->first();return impl_->frame->controls;}
std::shared_ptr<const runtime::ScriptUiChildSource>
detail::NotificationContextChildControlsExportLoan::Guard::child_source_without_callbacks()const {impl_->frame->first();return impl_->frame->source;}
std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>
detail::NotificationContextChildControlsExportLoan::Guard::child_receiver_without_callbacks()const {impl_->frame->first();return impl_->frame->receiver;}
std::shared_ptr<detail::NotificationLayoutContextOrdinaryState>
detail::NotificationContextChildControlsExportLoan::Guard::child_context_without_callbacks()const {impl_->frame->first();return impl_->frame->context;}
std::shared_ptr<BnwWorldViewControlTree>
detail::NotificationContextChildControlsExportLoan::Guard::child_tree_without_callbacks()const {
 impl_->frame->first();const auto a=impl_->frame->attempt.lock();require(a&&a->child,"child controls facade retired");return a->child;
}
const char* detail::NotificationContextChildControlsExportLoan::Guard::terminated_id_without_callbacks()const {
 return impl_->frame->terminated_id_without_callbacks();
}
std::u16string_view detail::NotificationContextChildControlsExportLoan::Guard::parent_screen_b4_without_callbacks()const {
 return impl_->frame->parent_screen_b4_without_callbacks();
}

#include "notification_context_child_named_map588_core.inc"
#include "notification_context_child_controls_v64_return_core.inc"
#include "notification_context_distinct_child_attach_core.inc"

void BnwWorldViewControlTree::prepare_root_context_child_controls_export_prefix_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<ContextRootChildAttempt>& input){
 using Frame=ContextChildControlsFrame;using Loan=detail::NotificationContextChildControlsExportLoan;
 const auto a=input;require(a&&a->factory,"child4FD460 requires actual returned529520/v44 child");
 const auto read=a->parent;read.require_source_owned(operation);read.require_child_factory_return_identity_without_callbacks(*a->factory);
 const auto root=read.impl_->evidence;const auto child=a->child_core;const auto factory=a->factory;
 const auto controls=root->controls;const auto cg=root->controls_guard;
 const auto id=child->context_layout_baseline->base_.id_58;const auto state=child->context_ordinary_state;
 const auto receiver=a->receiver;const auto source=a->descriptor;
 const auto parent_core=root->core.lock();require(parent_core&&parent_core->context_ordinary_state,
  "child4FD460 actual physical parent3C retired");
 const auto parent_state=parent_core->context_ordinary_state;
 const auto parent_b4=parent_state->impl_->b4_value;const auto parent_b4_revision=parent_state->impl_->b4_revision;
 const auto before=[&]{read.require_current_without_callbacks();read.require_child_factory_return_identity_without_callbacks(*factory);
  require(read.impl_->evidence==root&&root->controls==controls&&a->factory==factory&&a->child_core==child&&
   child->context_child_controls_frame.expired()&&!a->controls_export_operand&&child->context_layout_baseline->base_.id_58==id&&
   child->context_ordinary_state==state&&a->receiver==receiver&&a->descriptor==source&&
   root->core.lock()==parent_core&&a->realm&&a->realm->impl_->context.lock()==parent_core&&
   parent_core->context_ordinary_state==parent_state&&parent_state->impl_->b4_value==parent_b4&&
   parent_b4&&parent_state->impl_->b4_revision==parent_b4_revision&&!a->controls_export_return,
   "child4FD460 actual entry preimage changed or replayed");};
 const auto full=[&]{before();read.require_source_owned(operation);before();
  const auto read_frame=root->frame.lock();require(read_frame!=nullptr,"child v64 parent read frame retired before entry FULL");
  const auto prepared=read_frame->prepared;const auto defaults=root->defaults;
  cg.require_current_owned(operation,prepared,defaults);before();};
 // 4FD46D/474 reads the completed child's physical ID58 and its real first
 // byte. Selected Source25 has positive ID, not a fabricated missing-ID skip.
 full();require(id&&id->length&&id->bytes[0]&&a->wrapper&&a->wrapper->children&&
  a->wrapper->children->impl_->ordered.empty(),"selected child4FD460 positiveID/zero78 profile required");
 auto frame=std::make_shared<Frame>(a,read,factory,controls,cg,id,state,receiver,source,parent_state,parent_b4,parent_b4_revision,&operation);full();
 child->context_child_controls_frame=frame;
 {
 struct Exit{std::shared_ptr<Frame> frame;~Exit(){frame->alive.store(false);}}exit{frame};
 try{
  frame->full();auto fields=std::make_shared<Loan::Impl>(frame);frame->full();
  auto operand=std::shared_ptr<const Loan>(new Loan(fields));frame->full();
  a->controls_export_operand=operand; // live operand; no VM write/export return
  const auto guard=operand->capture_guard_without_callbacks(operation);guard.require_source_owned(operation);
  const auto& host=operation.context_lua_host_owned();frame->full();
  const auto returned=host.export_context_child_controls_owned(operation,operand);frame->full();
  require(returned!=nullptr,"actual child664FB0/6768B0 did not return");
  const auto rg=returned->capture_guard_without_callbacks(operation,*operand);
  rg.require_source_owned(operation,*operand);frame->full();
  a->controls_export_return=returned;rg.require_current_without_callbacks(operation,*operand);
  // Export returned in the genuine parent VM. Native4FD485 now reaches true
  // enabled + physical3C ->515120 before78. No fullv64/attach from this receipt.
  register_root_context_child_name588_owned(operation,a,frame);frame->full();
  require(a->name588_registration!=nullptr,"actual child515120 association did not return");
  a->name588_registration->first();rg.require_source_owned(operation,*operand);frame->full();
  stage_context_child_v64_return_owned(operation,a,frame);frame->full();
 }catch(...){frame->failed.store(true);throw;}
 } // actual4FD4CF..4FD4D2 epilogue: real lexical v64 frame Exit DTOR
 publish_context_child_v64_after_scope_owned(operation,a);
}
