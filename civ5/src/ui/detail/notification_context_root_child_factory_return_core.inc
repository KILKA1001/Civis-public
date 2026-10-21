// Actual selected Context529520 return AFTER the executed v44 wrapper and
// its real lexical temporaries. This type grants neither v64 nor RootRead.
struct detail::NotificationContextRootChildFactoryReturn::Impl final {
 using Attempt=BnwWorldViewControlTree::ContextRootChildAttempt;
 using State=NotificationLayoutContextOrdinaryState;
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true},failed{false};
 const std::weak_ptr<Attempt> attempt;
 const NotificationContextRootReadChildrenLoan::Guard read;
 const std::shared_ptr<BnwWorldViewControlTree::ContextRootXml84> document;
 const content::XmlNode*const declaration;
 const std::weak_ptr<BnwWorldViewControlTree::Core> child;
 const std::shared_ptr<State> state;
 const std::shared_ptr<const NotificationLayoutOrdinaryReceiver> receiver;
 const std::shared_ptr<const NotificationContextNativeConstructorReturn> constructor;
 const std::shared_ptr<const Attempt::WrapperReturn> wrapper;
 const std::shared_ptr<const NotificationLayoutControlID58> id58;
 const std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
 const runtime::BnwWorldViewSourceOperationLoan*const operation;
 const std::uint64_t scope,serial;
 std::weak_ptr<const NotificationContextRootChildFactoryReturn> canonical;
 Impl(std::weak_ptr<Attempt> a,NotificationContextRootReadChildrenLoan::Guard g,
  std::shared_ptr<BnwWorldViewControlTree::ContextRootXml84> xml,const content::XmlNode*decl,
  std::weak_ptr<BnwWorldViewControlTree::Core> c,std::shared_ptr<State> s,
  std::shared_ptr<const NotificationLayoutOrdinaryReceiver> r,
  std::shared_ptr<const NotificationContextNativeConstructorReturn> ctor,
  std::shared_ptr<const Attempt::WrapperReturn> returned,
  std::shared_ptr<const NotificationLayoutControlID58> id,
  std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> owner,
  const runtime::BnwWorldViewSourceOperationLoan*op,std::uint64_t current_scope,std::uint64_t current)
  :attempt(std::move(a)),read(std::move(g)),document(std::move(xml)),declaration(decl),child(std::move(c)),
   state(std::move(s)),receiver(std::move(r)),constructor(std::move(ctor)),wrapper(std::move(returned)),
   id58(std::move(id)),source(std::move(owner)),operation(op),scope(current_scope),serial(current){}
 void first()const {BnwWorldViewControlTree::require_context_root_child_factory_physical_without_callbacks(*this);}
 void full()const {first();read.require_source_owned(*operation);first();}
};

detail::NotificationContextRootChildFactoryReturn::NotificationContextRootChildFactoryReturn(std::shared_ptr<Impl> p)
 :impl_(std::move(p)){}
detail::NotificationContextRootChildFactoryReturn::~NotificationContextRootChildFactoryReturn(){impl_->alive.store(false);}

void BnwWorldViewControlTree::require_context_root_child_factory_physical_without_callbacks(
 const detail::NotificationContextRootChildFactoryReturn::Impl& r){
 using State=detail::NotificationLayoutContextOrdinaryState;
 require(r.creator==std::this_thread::get_id()&&r.alive.load()&&!r.failed.load()&&r.operation,
  "Root child factory return retired/foreign/failed FIRST");
 r.read.require_current_without_callbacks(); // actual parent Source/scope before the old operand
 const auto a=r.attempt.lock();const auto child=r.child.lock();const auto current=r.canonical.lock();
 const auto entered=r.read.impl_->evidence;const auto parent=entered->core.lock();
 require(a&&child&&parent&&current&&parent!=child&&a->child&&a->child->core_==child&&
  a->child_core==child&&a->factory==current&&current->impl_.get()==&r&&a->wrapper==r.wrapper&&
  !a->pending_wrapper&&a->document==r.document&&a->declaration==r.declaration&&
  entered->document==r.document&&&r.read.actual_declaration_without_callbacks()==r.declaration&&
  State::Impl::same(child->source,r.source)&&State::Impl::same(parent->source,r.source)&&
  entered->operation==r.operation&&entered->core.lock()==parent&&
  std::find(parent->context_root_child_attempts.begin(),parent->context_root_child_attempts.end(),a)!=parent->context_root_child_attempts.end(),
  "Root child factory SAME lexical cursor/distinct child/retained allocation changed");
 r.source->require_operation_origin_without_callbacks(*r.operation,r.scope);
 r.operation->require_origin_without_callbacks();
 require(r.operation->operation_serial()==r.serial,"Root child factory current Source serial changed");
 child->layout_origin();
 require(child->creator==std::this_thread::get_id()&&child->facade_alive.load()&&!child->failed.load()&&!child->busy&&
  child->context==a->selected_context&&r.state&&r.state->impl_&&r.state->impl_->alive.load()&&!r.state->impl_->failed.load()&&
  child->context_ordinary_state==r.state&&child->context_layout_baseline&&
  child->context_layout_baseline->base_.receiver_owned==r.receiver&&a->receiver==r.receiver&&
  r.receiver&&r.receiver->impl_&&r.receiver->impl_->canonical.lock()==r.receiver&&
  r.receiver->impl_->core.lock()==child&&r.receiver->impl_->context.lock()==r.state&&
  child->context_xml_wrapper_frame.expired()&&child->context_direct_load_frame.expired()&&
  child->context_load_name_frame.expired()&&!child->context_cursor_load_prefix&&!child->context_lua_preparation,
  "Root child factory actual subtype/receiver/wrapper DTOR phase changed");
 require(r.constructor&&r.constructor->impl_&&r.constructor->impl_->alive.load()&&!r.constructor->impl_->failed.load()&&
  r.state->impl_->native_constructor_return==r.constructor&&a->constructor==r.constructor,
  "Root child factory genuine completed40F constructor changed");
 r.constructor->impl_->port->require_completed();
 const auto& base=*child->context_layout_baseline;
 require(r.wrapper&&r.wrapper->state==r.state&&r.wrapper->realm==a->realm&&
  r.wrapper->children==base.base_.children_78&&r.wrapper->b4_revision==r.state->impl_->b4_revision&&
  base.base_.id_58==r.id58&&r.wrapper->children&&r.wrapper->children->impl_&&
  r.wrapper->children->impl_->parent.lock()==r.receiver&&r.wrapper->children->impl_->ordered.empty(),
  "Root child factory immutable wrapper/ID58/zero78 custody changed");
 if(a->attach_progress){
  // A genuine separate parentv38 producer owns each live pointer/journal
  // mutation. The original wrapper return retains its immutable preimage.
  require_context_distinct_attach_progress_without_callbacks(*a->attach_progress);
 }else require(r.wrapper->sequence==base.sequence_&&r.wrapper->effects==child->effect_revision&&
  r.state->impl_->wrapper_context_realm_3c==a->realm&&base.base_.context_realm_3c==a->realm&&
  base.base_.context_3c_observation==detail::NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm&&
  base.base_.parent_38.expired()&&r.state->impl_->actual_parent_38.expired(),
  "Root child factory original pre-attach wrapper journals/3C/NULL38 changed");
 r.read.require_current_without_callbacks();
}

std::shared_ptr<const detail::NotificationContextRootChildFactoryReturn>
BnwWorldViewControlTree::publish_context_root_child_factory_return_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const std::shared_ptr<ContextRootChildAttempt>& input){
 using Return=detail::NotificationContextRootChildFactoryReturn;using State=detail::NotificationLayoutContextOrdinaryState;
 const auto a=input;require(a!=nullptr,"Root factory publisher actual retained attempt absent");
 const auto read=a->parent;read.require_source_owned(operation);
 const auto parent=core_;const auto child=a->child_core;const auto facade=a->child;
 require(child&&facade&&facade->core_==child&&parent!=child&&child->context_layout_baseline,
  "Root factory publisher must use distinct completed child Context");
 const auto source=child->source;require(source&&State::Impl::same(source,parent->source),"Root factory child Source changed");
 const auto scope=source->capture_active_operation_scope_serial_without_callbacks(&operation);
 const auto document=a->document;const auto*declaration=a->declaration;const auto state=child->context_ordinary_state;
 const auto receiver=a->receiver;const auto constructor=a->constructor;const auto wrapper=a->wrapper;
 const auto id58=child->context_layout_baseline->base_.id_58;const auto serial=operation.operation_serial();
 const auto before=[&]{read.require_current_without_callbacks();source->require_operation_origin_without_callbacks(operation,scope);
  require(read.impl_->evidence->core.lock()==parent&&parent==core_&&a->child_core==child&&a->child==facade&&
   a->document==document&&a->declaration==declaration&&&read.actual_declaration_without_callbacks()==declaration&&
   a->wrapper==wrapper&&!a->pending_wrapper&&!a->factory&&child->context_ordinary_state==state&&
   child->context_layout_baseline->base_.receiver_owned==receiver&&child->context_layout_baseline->base_.id_58==id58&&
   state&&state->impl_&&state->impl_->native_constructor_return==constructor&&wrapper&&
   wrapper->state==state&&wrapper->children==child->context_layout_baseline->base_.children_78&&
   wrapper->sequence==child->context_layout_baseline->sequence_&&wrapper->effects==child->effect_revision&&
   wrapper->b4_revision==state->impl_->b4_revision&&state->impl_->wrapper_context_realm_3c==a->realm&&
   child->context_layout_baseline->base_.context_realm_3c==a->realm&&child->context_xml_wrapper_frame.expired()&&
   operation.operation_serial()==serial,"Root factory current wrapper allocation/DTOR POST changed");};
 const auto full=[&]{before();read.require_source_owned(operation);before();};
 std::shared_ptr<Return::Impl> data;
 try{
  full();data=std::make_shared<Return::Impl>(a,read,document,declaration,child,state,receiver,constructor,wrapper,id58,source,&operation,scope,serial);full();
  auto returned=std::shared_ptr<const Return>(new Return(data));full();
  data->canonical=returned;a->factory=returned; // nonallocating own publisher, not a v64 receipt
  data->full();data->first();return returned;
 }catch(...){if(data)data->failed.store(true);child->failed.store(true);throw;}
}

void detail::NotificationContextRootReadChildrenLoan::Guard::require_child_factory_return_identity_without_callbacks(
 const NotificationContextRootChildFactoryReturn& returned)const {
 const auto saved=impl_;saved->first();require(returned.impl_!=nullptr,"Root cursor offered missing actual factory return");
 const auto data=returned.impl_;data->first();saved->first();
 require(data->read.impl_==saved&&data->document==saved->evidence->document&&
  data->declaration==&actual_declaration_without_callbacks(),"Root cursor offered different child's factory return");
}
