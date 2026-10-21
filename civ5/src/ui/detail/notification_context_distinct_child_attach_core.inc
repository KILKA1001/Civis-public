// Actual parent673730/44BF40 for a returned distinct physical Context. No
// Core::node(parent,id0) may stand in for the child. Each real pointer store
// earns an owned phase/journal; the old factory return is not weakened.
struct BnwWorldViewControlTree::ContextDistinctChildAttachProgress final {
 enum class Stage {before38,parent38,inherited3c,storage20,appended78,context3c,returned};
 const std::thread::id creator=std::this_thread::get_id();
 const std::weak_ptr<ContextRootChildAttempt> attempt;
 const detail::NotificationContextRootReadChildrenLoan::Guard read;
 const detail::NotificationLayoutNativeChildControlsReturn::Guard controls;
 const std::weak_ptr<Core> parent,child;
 const std::shared_ptr<detail::NotificationLayoutContextOrdinaryState> state;
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> root,receiver;
 const std::shared_ptr<const detail::NotificationLayoutContextRealmRef> original,inherited,context_this;
 const std::shared_ptr<const detail::NotificationContextRootReadCompletionSlot> pending;
 const runtime::BnwWorldViewSourceOperationLoan*const operation;
 const std::uint64_t before_sequence,before_effects,before_layout;
 const std::size_t before_history,before_count;
 const std::vector<std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>> before_members;
 Stage stage=Stage::before38;
 std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> ordered;
 std::shared_ptr<const detail::NotificationContextRootChildAttachReturn> returned;
 ContextDistinctChildAttachProgress(std::weak_ptr<ContextRootChildAttempt> a,
  detail::NotificationContextRootReadChildrenLoan::Guard r,detail::NotificationLayoutNativeChildControlsReturn::Guard g,
  std::shared_ptr<Core> p,std::shared_ptr<Core> c,std::shared_ptr<detail::NotificationLayoutContextOrdinaryState> s,
  std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> parent_receiver,
  std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> child_receiver,
  std::shared_ptr<const detail::NotificationLayoutContextRealmRef> before,
  std::shared_ptr<const detail::NotificationLayoutContextRealmRef> inherited_value,
  std::shared_ptr<const detail::NotificationLayoutContextRealmRef> actual_this,
  std::shared_ptr<const detail::NotificationContextRootReadCompletionSlot> completion,
  const runtime::BnwWorldViewSourceOperationLoan*op,std::uint64_t q,std::uint64_t fx,std::uint64_t l,
  std::size_t h,std::size_t count,std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> array,
  std::vector<std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>> members)
  :attempt(std::move(a)),read(std::move(r)),controls(std::move(g)),parent(std::move(p)),child(std::move(c)),state(std::move(s)),
   root(std::move(parent_receiver)),receiver(std::move(child_receiver)),original(std::move(before)),inherited(std::move(inherited_value)),
   context_this(std::move(actual_this)),pending(std::move(completion)),operation(op),before_sequence(q),before_effects(fx),before_layout(l),
   before_history(h),before_count(count),before_members(std::move(members)),ordered(std::move(array)){}
 std::size_t stores()const {return stage==Stage::before38?0:stage==Stage::parent38?1:
  (stage==Stage::inherited3c||stage==Stage::storage20||stage==Stage::appended78)?2:3;}
 void first()const {BnwWorldViewControlTree::require_context_distinct_attach_progress_without_callbacks(*this);}
 void full()const {
  first();const auto parent_custody=parent.lock();const auto child_custody=child.lock();
  require(parent_custody&&child_custody,"distinct attach FULL physical local Core custody retired");
  first();read.require_source_owned(*operation);first();controls.require_source_owned(*operation);first();
 }
};

void BnwWorldViewControlTree::require_context_distinct_attach_progress_without_callbacks(
 const ContextDistinctChildAttachProgress& p){
 require(p.creator==std::this_thread::get_id()&&p.operation,"distinct Context attach foreign FIRST");
 p.read.require_current_without_callbacks(); // SAME actual entered Root Source before old raw loan
 const auto parent=p.parent.lock();const auto child=p.child.lock();
 const auto a=p.attempt.lock();
 require(parent&&child&&parent->context_layout_baseline&&child->context_layout_baseline&&
  child->creator==p.creator&&child->facade_alive.load()&&!child->failed.load()&&child->source==parent->source,
  "distinct attach physical Core/baseline/Source retired");
 const auto& b=*child->context_layout_baseline;
 require(a&&a->attach_progress.get()==&p&&a->child_core==child&&child!=parent&&
  p.read.impl_==a->parent.impl_&&p.read.impl_->evidence->core.lock()==parent&&
  a->receiver==p.receiver&&a->child_core->context_ordinary_state==p.state&&p.state&&p.state->impl_&&
  p.state->impl_->alive.load()&&!p.state->impl_->failed.load()&&
  b.kind_==detail::NotificationLayoutBaseline::Kind::context&&b.base_.receiver_owned==p.receiver&&
  p.receiver&&p.receiver->impl_&&p.receiver->impl_->core.lock()==child&&p.receiver->impl_->context.lock()==p.state&&
  parent->context_layout_baseline->base_.receiver_owned==p.root&&
  p.read.actual_root_receiver_without_callbacks()==p.root&&p.read.actual_context_realm_without_callbacks()==p.context_this&&
  a->wrapper&&a->wrapper->sequence==p.before_sequence&&a->wrapper->effects==p.before_effects&&
  b.sequence_==p.before_sequence+p.stores()&&b.history_.size()==p.before_history+p.stores()&&
  child->effect_revision==p.before_effects+p.stores()&&child->layout_sequence==p.before_layout+p.stores(),
  "distinct attach real child/parent/immutable wrapper origin/earned mutation journal changed");
 const bool after38=p.stage!=ContextDistinctChildAttachProgress::Stage::before38;
 require(after38?(b.base_.parent_38.lock()==p.root&&p.state->impl_->actual_parent_38.lock()==p.root&&
  child->context_parent_core.lock()==parent&&child->context_parent_kind==Core::ContextParentKind::canonical_context):
  (b.base_.parent_38.expired()&&p.state->impl_->actual_parent_38.expired()),
  "distinct Context actual paired native38/parent Core bridge changed");
 const auto expected=p.stage==ContextDistinctChildAttachProgress::Stage::before38||
  p.stage==ContextDistinctChildAttachProgress::Stage::parent38?p.original:
  p.stage==ContextDistinctChildAttachProgress::Stage::inherited3c||p.stage==ContextDistinctChildAttachProgress::Stage::storage20||
  p.stage==ContextDistinctChildAttachProgress::Stage::appended78?p.inherited:p.context_this;
 const auto observation=expected?detail::NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm:
  detail::NotificationLayoutBaseline::Base::Context3CObservation::root_inherited_null;
 require(b.base_.context_realm_3c==expected&&p.state->impl_->wrapper_context_realm_3c==expected&&
  b.base_.context_3c_observation==observation,
  "distinct Context paired actual3C differs from earned native stage");
 for(std::size_t i=0;i<p.stores();++i){const auto& entry=b.history_[p.before_history+i];
  require(entry.sequence==p.before_sequence+i+1&&entry.command==(i?detail::NotificationLayoutBaseline::Mutation::context_realm_3c:
   detail::NotificationLayoutBaseline::Mutation::receiver_prefix),"distinct Context physical mutation journal kind/order changed");}
 const auto actual=parent->context_layout_baseline->base_.children_78;
 const bool appended=p.stage==ContextDistinctChildAttachProgress::Stage::appended78||
  p.stage==ContextDistinctChildAttachProgress::Stage::context3c||p.stage==ContextDistinctChildAttachProgress::Stage::returned;
 require(actual==p.ordered,"distinct Context actual parent78 allocation changed");
 require(p.before_members.size()==p.before_count,"distinct Context immutable pre-append parent78 prefix length changed");
 if(actual){require(actual->impl_&&actual->impl_->parent.lock()==p.root&&actual->impl_->ordered.size()==p.before_count+(appended?1u:0u)&&
   std::equal(p.before_members.begin(),p.before_members.end(),actual->impl_->ordered.begin()),
   "distinct Context actual parent78 owner/count changed");
  if(appended)require(actual->impl_->ordered[p.before_count]==p.receiver,
   "distinct Context original ordered append membership changed");
  else require(std::find(actual->impl_->ordered.begin(),actual->impl_->ordered.end(),p.receiver)==actual->impl_->ordered.end(),
   "distinct Context attached child membership before native append");
  if(p.stage==ContextDistinctChildAttachProgress::Stage::storage20||appended)
   require(actual->impl_->native_storage_byte20&&*actual->impl_->native_storage_byte20==0,
    "distinct native44BF6C parent78 storage byte20 clear missing");
 }else require(!p.before_count&&!appended,"distinct Context required parent78 append absent");
 if(p.stage==ContextDistinctChildAttachProgress::Stage::returned)
  require(p.returned&&child->context_root_distinct_attach_return==p.returned,
   "distinct Context actual pending attach return replaced");
 p.read.require_current_without_callbacks();
}

void BnwWorldViewControlTree::attach_distinct_context_child_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const std::shared_ptr<const detail::NotificationContextRootReadChildrenLoan>& loan,
 const std::shared_ptr<ContextRootChildAttempt>& a){
 using Progress=ContextDistinctChildAttachProgress;using Return=detail::NotificationContextRootChildAttachReturn;
 require(loan&&a&&a->controls_v64_return&&!a->attach_progress,"distinct Context673730 needs genuine returned child-v64");
 const auto read=loan->capture_guard_without_callbacks();read.require_source_owned(operation);
 const auto controls=a->controls_v64_return->capture_guard_for_attach_owned(operation,*loan);
 controls.require_source_owned(operation);const auto pending=read.capture_completion_slot_without_callbacks();
 const auto parent=core_;const auto child=a->child_core;
 require(parent&&child&&parent!=child&&parent->context_layout_baseline&&child->context_layout_baseline&&
  child->context_ordinary_state&&child->context_ordinary_state->impl_,"distinct attach actual physical Context owners absent");
 const auto state=child->context_ordinary_state;
 const auto root=read.actual_root_receiver_without_callbacks();const auto receiver=controls.actual_receiver_without_callbacks();
 const auto inherited=parent->context_layout_baseline->base_.context_realm_3c;const auto realm=read.actual_context_realm_without_callbacks();
 const auto original=state->impl_->wrapper_context_realm_3c;const auto array=parent->context_layout_baseline->base_.children_78;
 require(!array||(array->impl_&&array->impl_->parent.lock()==root),"distinct attach actual parent78 physical allocator owner changed");
 const auto count=array?array->impl_->ordered.size():0;const auto sequence=child->context_layout_baseline->sequence_;
 const auto effects=child->effect_revision;const auto layout=child->layout_sequence;const auto history=child->context_layout_baseline->history_.size();
 std::vector<std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>> prefix;
 const auto first=[&]{read.require_current_without_callbacks();controls.require_current_without_callbacks();
  require(parent!=child&&read.impl_==a->parent.impl_&&read.impl_->evidence->core.lock()==parent&&a->child_core==child&&
   a->receiver==receiver&&child->context_ordinary_state==state&&!a->attach_progress&&!child->context_root_distinct_attach_return&&
   child->context_layout_baseline->base_.parent_38.expired()&&state->impl_->actual_parent_38.expired()&&
   child->context_layout_baseline->sequence_==sequence&&child->effect_revision==effects&&child->layout_sequence==layout&&
   child->context_layout_baseline->history_.size()==history&&state->impl_->wrapper_context_realm_3c==original&&
   parent->context_layout_baseline->base_.children_78==array&&(!array||(array->impl_&&array->impl_->ordered.size()==count))&&
   (prefix.empty()||std::equal(prefix.begin(),prefix.end(),array->impl_->ordered.begin())),
   "distinct Context673730 authentic actual returned preimage changed");};
 const auto full=[&]{first();read.require_source_owned(operation);first();controls.require_source_owned(operation);first();};
 full();require(sequence<=UINT64_MAX-3&&effects<=UINT64_MAX-3&&layout<=UINT64_MAX-3,"distinct attach physical journals exhausted");
 require(count<=prefix.max_size(),"distinct attach parent78 immutable prefix capacity exhausted");prefix.reserve(count);full();
 if(array)for(const auto& existing:array->impl_->ordered)prefix.push_back(existing);
 full();require(prefix.size()==count,"distinct attach parent78 immutable actual prefix incomplete");
 auto p=std::make_shared<Progress>(a,read,controls,parent,child,state,root,receiver,original,inherited,realm,pending,
  &operation,sequence,effects,layout,history,count,array,prefix);full();
 // Allocate local scalar history storage BEFORE exposing mutation authority;
 // no borrowed Base/vector is used across new_handler. Three actual stores.
 std::vector<detail::NotificationLayoutBaseline::Entry> staged;
 require(history<=staged.max_size()-3,"distinct attach history capacity exhausted");staged.reserve(history+3);full();
 for(const auto& entry:child->context_layout_baseline->history_)staged.push_back(entry);
 child->context_layout_baseline->history_.swap(staged);
 try{
 a->attach_progress=p;p->full();
 const auto record=[&](detail::NotificationLayoutBaseline::Mutation m){
  child->context_layout_baseline->record_owned(m);++child->effect_revision;++child->layout_sequence;
 };
 //44BF4C actual child38 first, paired onto its distinct owned Context state.
 record(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
 child->context_layout_baseline->base_.parent_38=root;state->impl_->actual_parent_38=root;
 child->context_parent_core=parent;child->context_parent_control_id=0; // parent Context domain's own root, never child alias
 child->context_parent_kind=Core::ContextParentKind::canonical_context;p->stage=Progress::Stage::parent38;p->full();
 //44BF4F/52 copies parent's current3C even if actual nativeNULL.
 record(detail::NotificationLayoutBaseline::Mutation::context_realm_3c);
 child->context_layout_baseline->base_.context_realm_3c=inherited;state->impl_->wrapper_context_realm_3c=inherited;
 child->context_layout_baseline->base_.context_3c_observation=inherited?
  detail::NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm:
  detail::NotificationLayoutBaseline::Base::Context3CObservation::root_inherited_null;
 p->stage=Progress::Stage::inherited3c;p->full();
 if(!p->ordered){
  auto fields=std::make_shared<detail::NotificationLayoutOrdinaryChildren78::Impl>(root);p->full();
  auto created=std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78>(new detail::NotificationLayoutOrdinaryChildren78(fields));p->full();
  require(!parent->context_layout_baseline->base_.children_78,"distinct495E10 actual NULL78 preimage changed");
  parent->context_layout_baseline->base_.children_78=created;p->ordered=created;p->full();
 }
 //Native44BF6C writes actual byte[78object+20]=0 before vector capacity helper.
 p->ordered->impl_->native_storage_byte20=0;p->stage=Progress::Stage::storage20;p->full();
 std::vector<std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>> growth;
 require(count<growth.max_size(),"distinct78 count exhausted");growth.reserve(count+1);p->full();
 for(const auto& existing:p->ordered->impl_->ordered)growth.push_back(existing);
 p->ordered->impl_->ordered.swap(growth);p->first();
 p->ordered->impl_->ordered.push_back(receiver);p->stage=Progress::Stage::appended78;p->full();
 //67373E overrides3C with actual parentTHIS only AFTER44BF40 append returned.
 record(detail::NotificationLayoutBaseline::Mutation::context_realm_3c);
 child->context_layout_baseline->base_.context_realm_3c=realm;state->impl_->wrapper_context_realm_3c=realm;
 child->context_layout_baseline->base_.context_3c_observation=detail::NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm;
 p->stage=Progress::Stage::context3c;p->full();
 const auto completed=pending->capture_pending_data_guard_without_callbacks();p->full();
 auto data=std::make_shared<const Return::Impl>(parent,parent->context_ordinary_state,root,receiver,p->ordered,realm,
  completed,count,child,state);p->full();
 auto returned=std::shared_ptr<const Return>(new Return(data));p->full();
 require(!child->context_root_distinct_attach_return,"distinct Context attach return replayed");
 child->context_root_distinct_attach_return=returned;p->returned=returned;p->stage=Progress::Stage::returned;p->full();
 // Pending attachment DATA only. It requires actual wholeRootRead completion
 // before initialization; source31 never publishes that completion stamp.
 }catch(...){child->failed.store(true);state->impl_->failed.store(true);parent->failed.store(true);throw;}
}

void BnwWorldViewControlTree::require_context_distinct_attachment_physical_without_callbacks(
 const std::shared_ptr<const detail::NotificationContextRootChildAttachReturn::Impl>& r){
 using Kind=detail::NotificationContextRootChildAttachReturn::Impl::ReceiverKind;
 require(r&&r->receiver_kind==Kind::distinct_context&&!r->child_id,
  "distinct attach current initializer receipt is not a genuine physical Context variant");
 const auto parent=r->core.lock();const auto child=r->distinct_context_child.lock();
 const auto state=r->distinct_context_state.lock();const auto receiver=r->child.lock();const auto root=r->root.lock();
 const auto array=r->children.lock();const auto realm=r->realm.lock();
 require(parent&&child&&state&&receiver&&root&&array&&realm&&child!=parent&&
  child->creator==std::this_thread::get_id()&&child->facade_alive.load()&&!child->failed.load()&&
  child->source==parent->source&&child->context_root_distinct_attach_return&&child->context_root_distinct_attach_return->impl_==r&&
  child->context_ordinary_state==state&&state->impl_&&state->impl_->alive.load()&&!state->impl_->failed.load()&&
  state->impl_->native_constructor_return&&state->impl_->native_constructor_return->impl_->alive.load()&&
  receiver->impl_&&receiver->impl_->canonical.lock()==receiver&&receiver->impl_->core.lock()==child&&
  receiver->impl_->context.lock()==state&&child->context_layout_baseline&&
  child->context_layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::context&&
  child->context_layout_baseline->base_.receiver_owned==receiver&&child->context_layout_baseline->base_.parent_38.lock()==root&&
  state->impl_->actual_parent_38.lock()==root&&child->context_layout_baseline->base_.context_realm_3c==realm&&
  state->impl_->wrapper_context_realm_3c==realm&&child->context_layout_baseline->base_.context_3c_observation==
   detail::NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm&&child->context_parent_core.lock()==parent&&
  child->context_parent_kind==Core::ContextParentKind::canonical_context&&
  array->impl_&&r->append_index<array->impl_->ordered.size()&&array->impl_->ordered[r->append_index]==receiver,
  "distinct Context actual paired38/3C/parent78 membership/current return changed");
 // Future legitimate child initialization changes geometry/style/B4/sequence.
 // Attachment DATA retains only actual pointer relations, never old Read loans.
}
