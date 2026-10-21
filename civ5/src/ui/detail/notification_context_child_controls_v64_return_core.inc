// Genuine selected4FD4A7/78-zero traversal and4FD4D2 return. This persistent
// capability does not keep/renew the expired child export lexical loan. Its
// Host checker receives this earned capability and rechecks the SAME retained
// Lua table/CData/metatype without creating tables or invoking Lua/GC.
struct detail::NotificationLayoutNativeChildControlsReturn::Impl final {
 using Tree=BnwWorldViewControlTree;
 struct Ordinary;
 const Origin origin;
 const std::shared_ptr<const Ordinary> ordinary;
 const std::thread::id creator=std::this_thread::get_id();
 mutable std::atomic<bool> alive{true};
 const std::weak_ptr<Tree::ContextRootChildAttempt> attempt;
 const NotificationContextRootReadChildrenLoan::Guard read;
 const std::weak_ptr<Tree::ContextChildControlsFrame> ended_frame;
 const std::shared_ptr<const NotificationContextRootChildFactoryReturn> factory;
 const std::shared_ptr<const NotificationContextChildControlsExportReturn> exported;
 const std::shared_ptr<const Tree::ContextChildName588Registration> named588;
 const std::shared_ptr<NotificationLayoutContextOrdinaryState> context;
 const std::shared_ptr<const NotificationLayoutOrdinaryReceiver> receiver;
 const std::shared_ptr<const NotificationLayoutControlID58> id58;
 const std::shared_ptr<NotificationLayoutOrdinaryChildren78> children78;
 const std::shared_ptr<const runtime::ScriptUiChildSource> selected;
 const runtime::BnwWorldViewSourceOperationLoan*const operation;
 const std::size_t traversed_count;
 mutable std::weak_ptr<const NotificationLayoutNativeChildControlsReturn> canonical;
 Impl(std::weak_ptr<Tree::ContextRootChildAttempt> a,NotificationContextRootReadChildrenLoan::Guard g,
  std::weak_ptr<Tree::ContextChildControlsFrame> frame,
  std::shared_ptr<const NotificationContextRootChildFactoryReturn> f,
  std::shared_ptr<const NotificationContextChildControlsExportReturn> e,
  std::shared_ptr<const Tree::ContextChildName588Registration> n,
  std::shared_ptr<NotificationLayoutContextOrdinaryState> state,
  std::shared_ptr<const NotificationLayoutOrdinaryReceiver> r,
  std::shared_ptr<const NotificationLayoutControlID58> id,
  std::shared_ptr<NotificationLayoutOrdinaryChildren78> array,
  std::shared_ptr<const runtime::ScriptUiChildSource> source,
  const runtime::BnwWorldViewSourceOperationLoan*op,std::size_t count)
  :origin(Origin::distinct_context),ordinary(),attempt(std::move(a)),read(std::move(g)),ended_frame(std::move(frame)),factory(std::move(f)),exported(std::move(e)),
   named588(std::move(n)),context(std::move(state)),receiver(std::move(r)),id58(std::move(id)),
   children78(std::move(array)),selected(std::move(source)),operation(op),traversed_count(count){}
 Impl(std::shared_ptr<const Ordinary> p,NotificationContextRootReadChildrenLoan::Guard g,
  const runtime::BnwWorldViewSourceOperationLoan*op)
  :origin(Origin::ordinary_control),ordinary(std::move(p)),attempt(),read(std::move(g)),ended_frame(),factory(),
   exported(),named588(),context(),receiver(),id58(),children78(),selected(),operation(op),traversed_count(0){}
};
detail::NotificationLayoutNativeChildControlsReturn::NotificationLayoutNativeChildControlsReturn(std::shared_ptr<const Impl> p)
 :impl_(std::move(p)){}
detail::NotificationLayoutNativeChildControlsReturn::~NotificationLayoutNativeChildControlsReturn(){impl_->alive.store(false);}
detail::NotificationLayoutNativeChildControlsReturn::Guard::Guard(std::shared_ptr<const Impl> p):impl_(std::move(p)){}

void BnwWorldViewControlTree::require_context_child_v64_physical_without_callbacks(
 const detail::NotificationLayoutNativeChildControlsReturn::Impl& r){
 require(r.origin==detail::NotificationLayoutNativeChildControlsReturn::Origin::distinct_context&&!r.ordinary&&
  r.creator==std::this_thread::get_id()&&r.alive.load()&&r.operation,
  "child4FD4D2 return retired/foreign FIRST");
 r.read.require_current_without_callbacks(); // actual SAME entered Root Source FIRST
 const auto a=r.attempt.lock();const auto f=r.ended_frame.lock();
 const auto parent=r.read.impl_->evidence->core.lock();const auto current=r.canonical.lock();
 require(a&&f&&parent&&a->child_core&&a->child&&parent!=a->child_core&&
  a->child->core_==a->child_core&&r.read.impl_==a->parent.impl_&&
  f->read.impl_==r.read.impl_&&f->attempt.lock()==a&&f->operation==r.operation&&
  !f->alive.load()&&!f->failed.load()&&f->creator==r.creator&&
  a->child_core->context_child_controls_frame.lock()==f&&
  a->factory==r.factory&&a->controls_export_return==r.exported&&a->name588_registration==r.named588&&
  f->factory==r.factory&&f->controls==r.read.impl_->evidence->controls&&
  f->context==r.context&&f->receiver==r.receiver&&f->id58==r.id58&&f->source==r.selected&&
  r.context&&r.context->impl_&&r.receiver&&r.id58&&r.children78&&r.children78->impl_&&
  a->descriptor==r.selected&&a->receiver==r.receiver&&a->child_core->context_ordinary_state==r.context&&
  a->child_core->context_layout_baseline&&a->child_core->context_layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::context&&
  a->child_core->context_layout_baseline->base_.receiver_owned==r.receiver&&
  a->child_core->context_layout_baseline->base_.id_58==r.id58&&
  a->child_core->context_layout_baseline->base_.children_78==r.children78&&
  r.children78->impl_->parent.lock()==r.receiver&&r.children78->impl_->ordered.size()==r.traversed_count&&
  r.traversed_count==0&&f->parent_context==parent->context_ordinary_state&&
  f->parent_context->impl_->b4_value==f->parent_b4&&f->parent_context->impl_->b4_revision==f->parent_b4_revision,
  "childv64 physical zero78/read/factory/real distinct Context or lexical DTOR changed");
 // Pending allocated DATA and the published return have distinct canonical
 // fields. Only the actual producer below moves after genuine DTOR/LAST FULL.
 if(current)require(current->impl_.get()==&r&&a->controls_v64_return==current&&!a->pending_controls_v64_return,
  "childv64 actual canonical returned capability replaced");
 else require(a->pending_controls_v64_return&&a->pending_controls_v64_return->impl_.get()==&r&&!a->controls_v64_return,
  "childv64 neither genuine pending nor actual canonical publication");
 r.read.require_child_factory_return_identity_without_callbacks(*r.factory);
 r.named588->first();r.read.require_current_without_callbacks();
}
void detail::NotificationLayoutNativeChildControlsReturn::require_export_return_operands_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan& op,const NotificationContextChildControlsExportReturn& exported,
 const NotificationContextLuaControlsNamedReference& parent,const NotificationLayoutContextOrdinaryState& physical,
 const BnwWorldViewControlTree& tree,const runtime::ScriptUiChildSource& selected,
 const NotificationLayoutOrdinaryReceiver& receiver,std::string_view id)const{
 BnwWorldViewControlTree::require_context_child_v64_export_operands_without_callbacks(*this,op,exported,
  parent,physical,tree,selected,receiver,id);
}
void detail::NotificationLayoutNativeChildControlsReturn::require_export_return_origin_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan& op,const NotificationContextChildControlsExportReturn& exported)const{
 BnwWorldViewControlTree::require_context_child_v64_export_origin_without_callbacks(*this,op,exported);
}
void BnwWorldViewControlTree::require_context_child_v64_export_origin_without_callbacks(
 const detail::NotificationLayoutNativeChildControlsReturn& returned,
 const runtime::BnwWorldViewSourceOperationLoan& op,const detail::NotificationContextChildControlsExportReturn& exported){
 const auto r=returned.impl_;require(r!=nullptr,"childv64 Host origin missing real returned body");
 require_context_child_v64_physical_without_callbacks(*r);
 require(r->canonical.lock().get()==&returned&&r->operation==&op&&r->exported.get()==&exported,
  "childv64 Host origin is not SAME actual returned capability/export/Source");
}
void BnwWorldViewControlTree::require_context_child_v64_export_operands_without_callbacks(
 const detail::NotificationLayoutNativeChildControlsReturn& returned,
 const runtime::BnwWorldViewSourceOperationLoan& op,const detail::NotificationContextChildControlsExportReturn& exported,
 const detail::NotificationContextLuaControlsNamedReference& parent,const detail::NotificationLayoutContextOrdinaryState& physical,
 const BnwWorldViewControlTree& tree,const runtime::ScriptUiChildSource& selected,
 const detail::NotificationLayoutOrdinaryReceiver& receiver,std::string_view id){
 const auto r=returned.impl_;require(r!=nullptr,"childv64 physical exporter missing real returned body");
 require_context_child_v64_physical_without_callbacks(*r);
 const auto a=r->attempt.lock();const auto canonical=r->canonical.lock();
 require(canonical.get()==&returned&&r->operation==&op&&r->exported.get()==&exported&&
  r->read.impl_->evidence->controls.get()==&parent&&r->context.get()==&physical&&a&&a->child.get()==&tree&&
  r->selected.get()==&selected&&r->receiver.get()==&receiver&&
  id==std::string_view(r->id58->bytes.data(),r->id58->length),
  "childv64 Host physical checker received foreign return/actual operands");
}
void BnwWorldViewControlTree::require_context_child_v64_current_without_callbacks(
 const std::shared_ptr<const detail::NotificationLayoutNativeChildControlsReturn::Impl>& r){
 require(r!=nullptr,"childv64 sealed return control absent");
 if(r->origin==detail::NotificationLayoutNativeChildControlsReturn::Origin::ordinary_control){
  require_root_ordinary_child_v64_export_current_without_callbacks(*r);return;
 }
 require_context_child_v64_physical_without_callbacks(*r);
 const auto current=r->canonical.lock();require(current&&current->impl_==r,"childv64 pending DATA cannot grant attach");
 r->exported->require_current_after_child_v64_without_callbacks(*r->operation,*current);
 require_context_child_v64_physical_without_callbacks(*r);
}
void detail::NotificationLayoutNativeChildControlsReturn::Guard::require_current_without_callbacks()const{
 BnwWorldViewControlTree::require_context_child_v64_current_without_callbacks(impl_);
}
void detail::NotificationLayoutNativeChildControlsReturn::Guard::require_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& op)const{
 const auto r=impl_;require_current_without_callbacks();require(r->operation==&op,"childv64 offered foreign current Source");
 r->read.require_source_owned(op);require_current_without_callbacks();
}
void detail::NotificationLayoutNativeChildControlsReturn::Guard::require_same_root_read_without_callbacks(
 const NotificationContextRootReadChildrenLoan& read)const{
 BnwWorldViewControlTree::require_context_child_v64_same_read_without_callbacks(impl_,read);
}
void BnwWorldViewControlTree::require_context_child_v64_same_read_without_callbacks(
 const std::shared_ptr<const detail::NotificationLayoutNativeChildControlsReturn::Impl>& r,
 const detail::NotificationContextRootReadChildrenLoan& read){
 require_context_child_v64_current_without_callbacks(r);
 require(r->read.impl_==read.impl_,"childv64 different actual Root cursor");
}
const detail::NotificationContextRootChildFactoryReturn&
detail::NotificationLayoutNativeChildControlsReturn::Guard::actual_factory_return_without_callbacks()const{
 require_current_without_callbacks();require(impl_->origin==Origin::distinct_context&&impl_->factory,
  "childv64 Context factory getter cannot return ordinary origin");return *impl_->factory;
}
std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>
detail::NotificationLayoutNativeChildControlsReturn::Guard::actual_receiver_without_callbacks()const{
 require_current_without_callbacks();
 if(impl_->origin==Origin::ordinary_control)return BnwWorldViewControlTree::root_ordinary_child_v64_receiver_without_callbacks(*impl_);
 return impl_->receiver;
}
detail::NotificationLayoutNativeChildControlsReturn::Guard
detail::NotificationLayoutNativeChildControlsReturn::capture_guard_for_attach_owned(
 const runtime::BnwWorldViewSourceOperationLoan& op,const NotificationContextRootReadChildrenLoan& read)const{
 Guard g(impl_);g.require_same_root_read_without_callbacks(read);g.require_source_owned(op);return g;
}

void BnwWorldViewControlTree::stage_context_child_v64_return_owned(
 const runtime::BnwWorldViewSourceOperationLoan& op,const std::shared_ptr<ContextRootChildAttempt>& a,
 const std::shared_ptr<ContextChildControlsFrame>& frame){
 using Return=detail::NotificationLayoutNativeChildControlsReturn;
 require(a&&frame&&a->name588_registration&&a->controls_export_operand&&a->controls_export_return&&
  !a->pending_controls_v64_return&&!a->controls_v64_return,"4FD4A7 genuine live v64 operands absent/replayed");
 frame->full();a->name588_registration->first();
 const auto eg=a->controls_export_return->capture_guard_without_callbacks(op,*a->controls_export_operand);
 const auto full=[&]{frame->full();a->name588_registration->first();eg.require_source_owned(op,*a->controls_export_operand);frame->full();};
 full();const auto child=a->child_core;const auto array=child->context_layout_baseline->base_.children_78;
 require(array&&array==a->wrapper->children&&array->impl_&&array->impl_->parent.lock()==a->receiver,
  "4FD4A7 physical child78 is not the actual completed wrapper array");
 // Selected native4FD4AA is nonNULL,4FD4B1 compares count to zero, then
 // 4FD4B4 jumps4FD4CE. Positive counts require genuine subtype recursion.
 const auto count=array->impl_->ordered.size();require(count==0,"required genuine child78 subtype-v64 recursion for positive count");
 full();require(array==child->context_layout_baseline->base_.children_78&&array->impl_->ordered.size()==count,
  "child4FD4B4 zero78 outcome changed after callback");
 auto body=std::make_shared<Return::Impl>(a,a->parent,frame,a->factory,a->controls_export_return,a->name588_registration,
  frame->context,frame->receiver,frame->id58,array,frame->source,&op,count);full();
 auto result=std::shared_ptr<const Return>(new Return(body));full();
 require(!a->pending_controls_v64_return&&!a->controls_v64_return&&array->impl_->ordered.empty(),
  "child4FD4D2 pending return allocation changed true78 outcome");
 a->pending_controls_v64_return=result;full(); // DATA; real v64-frame DTOR still mandatory
}
void BnwWorldViewControlTree::publish_context_child_v64_after_scope_owned(
 const runtime::BnwWorldViewSourceOperationLoan& op,const std::shared_ptr<ContextRootChildAttempt>& a){
 require(a&&a->pending_controls_v64_return&&!a->controls_v64_return,
  "child4FD4D2 actual returned pending DATA absent/replayed");
 const auto returned=a->pending_controls_v64_return;const auto r=returned->impl_;
 const auto first=[&]{require_context_child_v64_physical_without_callbacks(*r);require(r->operation==&op,
  "child4FD4D2 return different actual Source");};
 first();r->read.require_source_owned(op);first(); // LAST FULL after actual Exit DTOR, never renew old loan
 r->canonical=returned;a->controls_v64_return=returned;a->pending_controls_v64_return.reset();first();
 detail::NotificationLayoutNativeChildControlsReturn::Guard guard(r);guard.require_source_owned(op);first();
}
