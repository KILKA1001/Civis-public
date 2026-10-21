// The original six ordinary subtype +64 slots are the SAME 4FD460 body.
// Their factory origin is still distinct from the real LuaContext origin.
// This first closed branch performs native ID58 absent/empty plus actual
// child78 NULL/empty traversal. Positive ID reaches its mandatory typed Host
// exporter; positive child count reaches genuine per-child v64 recursion.
#include "notification_context_ordinary_controls_78_origin_core.inc"

struct BnwWorldViewControlTree::RootOrdinaryControlsFrame final {
 using Factory=detail::NotificationContextRootOrdinaryFactoryReturn;
 using Return=detail::NotificationLayoutNativeChildControlsReturn;
 using Read=detail::NotificationContextRootReadChildrenLoan;
 enum class IdOutcome {null_pointer,empty_first_byte,positive_first_byte};
 enum class ChildrenOutcome {null_pointer,empty_array,positive_array};
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true},failed{false};
 const std::weak_ptr<Core> core;
 const std::shared_ptr<const Read> root;
 const Read::Guard read;
 const std::shared_ptr<const Factory> factory;
 const Factory::Guard factory_guard;
 const Factory::Subtype subtype;
 const std::uint64_t id;
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> receiver;
 const std::shared_ptr<const detail::NotificationLayoutContextRealmRef> realm;
 const std::shared_ptr<const detail::NotificationLayoutControlID58> id58;
 const std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> children78;
 const IdOutcome id_outcome;
 const ChildrenOutcome children_outcome;
 const std::vector<std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>> selected_children;
 const std::shared_ptr<const RecursiveOrdinaryOrigin> recursive_origin;
 std::size_t traversed_count=0;
 const runtime::BnwWorldViewSourceOperationLoan*const operation;
 std::weak_ptr<const Return> staged,canonical;
 std::shared_ptr<const RootOrdinaryName588Registration> name588_registration;
 RootOrdinaryControlsFrame(std::shared_ptr<Core> c,std::shared_ptr<const Read> loan,Read::Guard g,
  std::shared_ptr<const Factory> returned,Factory::Guard fg,
  Factory::Subtype concrete,std::uint64_t control,
  std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> physical,
  std::shared_ptr<const detail::NotificationLayoutContextRealmRef> context,
  std::shared_ptr<const detail::NotificationLayoutControlID58> identity,
  std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> array,
  IdOutcome io,ChildrenOutcome co,const runtime::BnwWorldViewSourceOperationLoan* op,
  std::vector<std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>> members,
  std::shared_ptr<const RecursiveOrdinaryOrigin> nested={})
  :core(std::move(c)),root(std::move(loan)),read(std::move(g)),factory(std::move(returned)),
   factory_guard(std::move(fg)),subtype(concrete),id(control),receiver(std::move(physical)),
   realm(std::move(context)),id58(std::move(identity)),children78(std::move(array)),
   id_outcome(io),children_outcome(co),selected_children(std::move(members)),recursive_origin(std::move(nested)),operation(op){}
 void physical_without_callbacks(bool live)const {
  // The genuine entered Root checks canonical Source before the historical
  // operation pointer, weak Core or any physical receiver field is used.
  read.require_current_without_callbacks();factory_guard.require_current_without_callbacks();
  require(creator==std::this_thread::get_id()&&alive.load()==live&&!failed.load()&&
   operation&&root&&factory&&receiver&&realm&&id,
   "ordinary4FD460 lexical custody retired/failed/foreign FIRST");
  factory_guard.require_same_root_read_without_callbacks(*root);
  const auto actual=core.lock();require(actual&&read.impl_->evidence->core.lock()==actual,
   "ordinary4FD460 requires the SAME actual Root Core");
  const auto& node=Core::node(actual->nodes,id);
  require(node.alive&&!node.dynamic&&node.layout_baseline&&node.full_xml_occurrence,
   "ordinary4FD460 canonical physical receiver or XML occurrence absent");
  const auto& base=node.layout_baseline->base_;
  require(
   base.receiver_owned==receiver&&base.id_58==id58&&base.children_78==children78&&
   factory_guard.actual_context_realm_without_callbacks()==realm,
   "ordinary4FD460 actual ancestor custody/receiver/ID58/78 changed");
  if(recursive_origin){
   recursive_origin->first();require(recursive_origin->receiver==receiver&&recursive_origin->subtype==subtype&&
    recursive_origin->id==id&&recursive_origin->xml==node.full_xml_occurrence,
    "ordinary4FD460 recursive member is not the actual native child return");
  }else require(factory_guard.actual_receiver_without_callbacks()==receiver&&
   factory_guard.actual_subtype_without_callbacks()==subtype&&factory_guard.actual_control_id_without_callbacks()==id&&
   Core::same_layout_xml(node.full_xml_occurrence->value,factory_guard.actual_declaration_without_callbacks()),
   "ordinary4FD460 root factory cannot be substituted by recursive child ancestry");
  switch(id_outcome){
   case IdOutcome::null_pointer:require(!id58,"ordinary4FD46D NULL ID58 observation changed");break;
   case IdOutcome::empty_first_byte:require(id58&&id58->bytes[0]=='\0',"ordinary4FD477 empty first byte changed");break;
   case IdOutcome::positive_first_byte:require(id58&&id58->bytes[0]&&id58->length&&
    id58->length<id58->bytes.size()&&id58->bytes[id58->length]=='\0',
    "ordinary4FD477 positive terminated ID58 observation changed");break;
  }
  switch(children_outcome){
   case ChildrenOutcome::null_pointer:require(!children78,"ordinary4FD4AA NULL78 observation changed");break;
   case ChildrenOutcome::empty_array:require(children78&&children78->impl_&&
    children78->impl_->parent.lock()==receiver&&children78->impl_->ordered.empty(),
    "ordinary4FD4B1 true zero78 observation changed");break;
   case ChildrenOutcome::positive_array:require(children78&&children78->impl_&&
    children78->impl_->parent.lock()==receiver&&!children78->impl_->ordered.empty(),
    "ordinary4FD4B1 positive78 observation changed");break;
  }

  require(traversed_count<=selected_children.size()&&
   (children78?(children78->impl_&&children78->impl_->ordered==selected_children):selected_children.empty()),
   "ordinary4FD4C1 immutable actual78 order/count or earned cursor changed");
  const auto returned=canonical.lock();
  if(returned)require(!live&&node.root_ordinary_controls_return==returned&&staged.expired(),
   "ordinary4FD4D2 canonical publication changed");
  else require(!node.root_ordinary_controls_return,
   "ordinary4FD460 cannot overwrite an actual completed per-receiver return");
  read.require_current_without_callbacks();
 }
 void first()const {physical_without_callbacks(true);}
 void full()const {first();factory_guard.require_source_owned(*operation);first();
  read.require_source_owned(*operation);first();}
};

#include "notification_context_ordinary_button_cdata_core.inc"
#include "notification_context_ordinary_controls_export_core.inc"
#include "notification_context_ordinary_named_map588_core.inc"

struct detail::NotificationLayoutNativeChildControlsReturn::Impl::Ordinary final {
 const std::shared_ptr<BnwWorldViewControlTree::RootOrdinaryControlsFrame> frame;
 const std::shared_ptr<const NotificationContextRootOrdinaryControlsExportLoan> export_loan;
 const std::shared_ptr<const NotificationContextRootOrdinaryControlsExportReturn> exported;
 const std::vector<std::shared_ptr<const NotificationLayoutNativeChildControlsReturn>> completed_children;
 Ordinary(std::shared_ptr<BnwWorldViewControlTree::RootOrdinaryControlsFrame> actual,
  std::shared_ptr<const NotificationContextRootOrdinaryControlsExportLoan> loan={},
  std::shared_ptr<const NotificationContextRootOrdinaryControlsExportReturn> output={},
  std::vector<std::shared_ptr<const NotificationLayoutNativeChildControlsReturn>> children={})
  :frame(std::move(actual)),export_loan(std::move(loan)),exported(std::move(output)),completed_children(std::move(children)){}
};

#include "notification_context_ordinary_controls_78_child_core.inc"

void BnwWorldViewControlTree::require_root_ordinary_child_v64_current_without_callbacks(
 const detail::NotificationLayoutNativeChildControlsReturn::Impl& returned){
 using Return=detail::NotificationLayoutNativeChildControlsReturn;
 require(returned.origin==Return::Origin::ordinary_control&&returned.ordinary&&
  !returned.factory&&!returned.exported&&!returned.context&&!returned.receiver&&!returned.id58&&
  !returned.children78&&!returned.selected&&returned.attempt.expired()&&returned.ended_frame.expired(),
  "ordinary4FD4D2 origin cannot be a LuaContext/flat-control alias");
 const auto frame=returned.ordinary->frame;require(frame&&returned.creator==std::this_thread::get_id()&&
  returned.alive.load(),"ordinary4FD4D2 closed return retired/foreign");
 frame->physical_without_callbacks(false);
 const auto result=returned.canonical.lock();
 require(result&&result->impl_.get()==&returned&&frame->canonical.lock()==result&&
  returned.operation==frame->operation&&returned.read.impl_==frame->read.impl_&&
  frame->traversed_count==frame->selected_children.size()&&
  returned.ordinary->completed_children.size()==frame->selected_children.size(),
  "ordinary4FD4D2 requires every actual ordered78 virtual64 return, not pending/proclaimed completion");
 for(std::size_t ordinal=0;ordinal<returned.ordinary->completed_children.size();++ordinal){
  const auto child=returned.ordinary->completed_children[ordinal];
  require(child&&child->impl_&&child->impl_->origin==Return::Origin::ordinary_control&&child->impl_->ordinary&&
   child->impl_->ordinary->frame&&child->impl_->ordinary->frame->recursive_origin,
   "ordinary4FD4D2 required actual recursive child capability absent");
  const auto nested=child->impl_->ordinary->frame;const auto origin=nested->recursive_origin;
  require(origin->parent.lock()==frame&&origin->ordinal==ordinal&&nested->receiver==frame->selected_children[ordinal],
   "ordinary4FD4D2 completed child belongs to another parent/ordered slot");
  require_root_ordinary_child_v64_current_without_callbacks(*child->impl_);
 }
 frame->physical_without_callbacks(false);
 if(frame->id_outcome==RootOrdinaryControlsFrame::IdOutcome::positive_first_byte){
  require(returned.ordinary->export_loan&&returned.ordinary->exported&&frame->name588_registration,
   "ordinary4FD4D2 positiveID requires authentic returned +7C and515120 effects");
  const auto loan=returned.ordinary->export_loan->impl_;
  require(loan&&loan->frame==frame,"ordinary4FD4D2 retained physical export loan is from another frame");
  require_root_ordinary_controls_export_operand_physical_without_callbacks(loan,*returned.operation,false);
  frame->name588_registration->after_scope_without_callbacks();
 }else require(!returned.ordinary->export_loan&&!returned.ordinary->exported&&!frame->name588_registration,
  "ordinary4FD4D2 no-ID branch must not claim positive export/name588 effects");
 frame->physical_without_callbacks(false);
}

#include "notification_context_ordinary_controls_v64_retained_core.inc"

const detail::NotificationContextRootOrdinaryFactoryReturn&
detail::NotificationLayoutNativeChildControlsReturn::Guard::actual_ordinary_factory_return_without_callbacks()const {
 require_current_without_callbacks();require(impl_->origin==Origin::ordinary_control&&impl_->ordinary&&
  !impl_->ordinary->frame->recursive_origin,
  "childv64 root ordinary factory getter cannot expose ancestor custody as recursive child origin");
 return *impl_->ordinary->frame->factory;
}
detail::NotificationLayoutNativeChildControlsReturn::Origin
detail::NotificationLayoutNativeChildControlsReturn::Guard::actual_origin_without_callbacks()const {
 require_current_without_callbacks();return impl_->origin;
}
std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>
BnwWorldViewControlTree::root_ordinary_child_v64_receiver_without_callbacks(
 const detail::NotificationLayoutNativeChildControlsReturn::Impl& returned){
 require_root_ordinary_child_v64_current_without_callbacks(returned);return returned.ordinary->frame->receiver;
}

std::shared_ptr<const detail::NotificationLayoutNativeChildControlsReturn>
BnwWorldViewControlTree::consume_root_ordinary_child_controls_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationContextRootReadChildrenLoan>& root,
 const std::shared_ptr<const detail::NotificationContextRootOrdinaryFactoryReturn>& factory){
 using Frame=RootOrdinaryControlsFrame;using Return=detail::NotificationLayoutNativeChildControlsReturn;
 require(root&&factory,"ordinary4FD460 needs the genuine returned XML529520/v44 factory");
 const auto read=root->capture_guard_without_callbacks();read.require_source_owned(operation);
 const auto fg=factory->capture_guard_for_controls_owned(operation,*root);
 const auto actual=read.impl_->evidence->core.lock();require(actual&&actual==core_,
  "ordinary4FD460 actual entered Root differs from receiver Core");
 const auto id=fg.actual_control_id_without_callbacks();const auto receiver=fg.actual_receiver_without_callbacks();
 const auto subtype=fg.actual_subtype_without_callbacks();const auto realm=fg.actual_context_realm_without_callbacks();
 const auto first=[&]{read.require_current_without_callbacks();fg.require_current_without_callbacks();
  fg.require_same_root_read_without_callbacks(*root);require(read.impl_->evidence->core.lock()==actual&&
   fg.actual_control_id_without_callbacks()==id&&fg.actual_receiver_without_callbacks()==receiver&&
   fg.actual_subtype_without_callbacks()==subtype&&fg.actual_context_realm_without_callbacks()==realm,
   "ordinary4FD460 actual returned operands replaced before entry");};
 const auto full=[&]{first();fg.require_source_owned(operation);first();read.require_source_owned(operation);first();};
 full();const auto& base=Core::node(actual->nodes,id).layout_baseline->base_;
 const auto identity=base.id_58;const auto children=base.children_78;
 const auto io=!identity?Frame::IdOutcome::null_pointer:!identity->bytes[0]?
  Frame::IdOutcome::empty_first_byte:Frame::IdOutcome::positive_first_byte;
 const auto co=!children?Frame::ChildrenOutcome::null_pointer:children->impl_->ordered.empty()?
  Frame::ChildrenOutcome::empty_array:Frame::ChildrenOutcome::positive_array;
 require(!Core::node(actual->nodes,id).root_ordinary_controls_return,"ordinary4FD460 receiver replay");
 std::vector<std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>> selected;
 const auto count=children?children->impl_->ordered.size():0;
 selected.reserve(count);full();
 if(children)for(const auto& member:children->impl_->ordered)selected.push_back(member);
 full();require(!children||children->impl_->ordered==selected,"ordinary4FD460 actual78 snapshot changed");
 auto frame=std::make_shared<Frame>(actual,root,read,factory,fg,subtype,id,receiver,realm,identity,children,io,co,&operation,
  std::move(selected));full();frame->full();return run_root_ordinary_controls_frame_owned(operation,frame);
}

std::shared_ptr<const detail::NotificationLayoutNativeChildControlsReturn>
BnwWorldViewControlTree::run_root_ordinary_controls_frame_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const std::shared_ptr<RootOrdinaryControlsFrame>& frame){
 using Frame=RootOrdinaryControlsFrame;using Return=detail::NotificationLayoutNativeChildControlsReturn;
 require(frame!=nullptr,"ordinary4FD460 actual lexical frame required");frame->full();
 const auto actual=frame->core.lock();require(actual&&actual==core_&&frame->operation==&operation,
  "ordinary4FD460 selected returning frame offered another Core/Source");
 const auto id=frame->id;const auto root=frame->root;const auto read=frame->read;
 const auto io=frame->id_outcome;
 const auto first=[&]{read.require_current_without_callbacks();frame->factory_guard.require_current_without_callbacks();
  require(frame->core.lock()==actual&&frame->operation==&operation&&frame->root==root,
   "ordinary4FD4D2 actual returning custody changed");if(frame->recursive_origin)frame->recursive_origin->first();};
 const auto full=[&]{first();frame->factory_guard.require_source_owned(operation);first();read.require_source_owned(operation);first();};
 full();frame->first();std::shared_ptr<const Return> result;
 std::shared_ptr<const detail::NotificationContextRootOrdinaryControlsExportLoan> returned_export_loan;
 std::shared_ptr<const detail::NotificationContextRootOrdinaryControlsExportReturn> returned_export;
 std::vector<std::shared_ptr<const Return>> returned_children;
 {
  struct Exit final {std::shared_ptr<Frame> frame;~Exit(){frame->alive.store(false);}} exit{frame};
  try{
   frame->full();
   if(io==Frame::IdOutcome::positive_first_byte){
    const auto operand=make_root_ordinary_controls_export_operand_owned(operation,frame);frame->full();
    const auto& host=operation.context_lua_host_owned();frame->full();
    const auto exported=host.export_root_ordinary_controls_owned(operation,operand);frame->full();
    require(exported!=nullptr,"ordinary selected +7C actual Host CData export did not return");
    const auto eg=exported->capture_guard_without_callbacks(operation,*operand);
    eg.require_source_owned(operation,*operand);frame->full();
    register_root_ordinary_name588_owned(operation,frame,operand,exported);frame->full();
    require(frame->name588_registration!=nullptr,"ordinary515120 actual parent name association did not return");
    frame->name588_registration->first();eg.require_source_owned(operation,*operand);frame->full();
    returned_export_loan=operand;returned_export=exported;frame->full();
   }
   // Real4FD4BA reads actual78 count;4FD4C1 dispatches EACH actual member+64
   // in order. No v44 rerun or root factory alias is inserted into this loop.
   returned_children.reserve(frame->selected_children.size());frame->full();
   for(std::size_t ordinal=0;ordinal<frame->selected_children.size();++ordinal){
    frame->full();require(frame->traversed_count==ordinal&&returned_children.size()==ordinal,
     "ordinary4FD4BA actual ordered78 cursor changed");
    auto child=consume_root_ordinary_78_child_owned(operation,frame,ordinal);frame->full();
    require(child!=nullptr,"ordinary4FD4C1 actual child virtual64 did not return");
    const auto guard=child->capture_guard_for_attach_owned(operation,*root);
    guard.require_source_owned(operation);frame->full();
    returned_children.push_back(std::move(child));frame->traversed_count=ordinal+1;frame->full();
   }
   frame->full();require(frame->traversed_count==frame->selected_children.size(),
    "ordinary4FD4CE before all actual member virtual64 calls returned");
   auto physical=std::make_shared<const Return::Impl::Ordinary>(frame,returned_export_loan,returned_export,
    std::move(returned_children));frame->full();
   auto body=std::make_shared<Return::Impl>(physical,read,&operation);frame->full();
   result=std::shared_ptr<const Return>(new Return(body));frame->full();
   require(frame->staged.expired()&&frame->canonical.expired()&&
    !Core::node(actual->nodes,id).root_ordinary_controls_return,"ordinary4FD4D2 staging replay");
   frame->staged=result;frame->full(); // DATA, neither facade nor completed-v64 yet.
  }catch(...){frame->failed.store(true);throw;}
 } // actual common-v64 lexical DTOR, before LAST FULL and canonical publication.
 frame->physical_without_callbacks(false);full();frame->physical_without_callbacks(false);
 require(result&&frame->staged.lock()==result&&!Core::node(actual->nodes,id).root_ordinary_controls_return,
  "ordinary4FD4D2 actual DTOR/LAST FULL pending result changed");
 result->impl_->canonical=result;frame->canonical=result;frame->staged.reset();
 Core::mutable_node_owned(actual->nodes,id).root_ordinary_controls_return=result;
 require_root_ordinary_child_v64_current_without_callbacks(*result->impl_);
 const auto guard=result->capture_guard_for_attach_owned(operation,*root);
 guard.require_source_owned(operation);frame->physical_without_callbacks(false);return result;
}

#include "notification_context_ordinary_root_attach_identity_core.inc"
#include "notification_context_ordinary_pending_attachment_core.inc"
