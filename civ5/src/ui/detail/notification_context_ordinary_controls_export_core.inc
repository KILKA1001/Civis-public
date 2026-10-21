// After RootOrdinaryControlsFrame is complete. This typed operand does not
// issue origin from Node metadata: authentic factory guards are checked first.
struct detail::NotificationContextRootOrdinaryControlsExportLoan::Impl final {
 const std::shared_ptr<BnwWorldViewControlTree::RootOrdinaryControlsFrame> frame;
 const std::shared_ptr<const void> concrete;
 const NativeSubtype subtype;
 const std::shared_ptr<NotificationLayoutContextOrdinaryState> parent;
 const std::shared_ptr<const NotificationContextNativeWideStringValue> parent_b4;
 const std::shared_ptr<const NotificationContextLuaControlsNamedReference> controls;
 const std::array<char,32> terminated_id;
 const std::uint64_t parent_b4_revision;
 Impl(std::shared_ptr<BnwWorldViewControlTree::RootOrdinaryControlsFrame> f,
  std::shared_ptr<const void> c,NativeSubtype type,
  std::shared_ptr<NotificationLayoutContextOrdinaryState> context,
  std::shared_ptr<const NotificationContextNativeWideStringValue> b4,std::uint64_t revision,
  std::shared_ptr<const NotificationContextLuaControlsNamedReference> named,std::array<char,32> id)
  :frame(std::move(f)),concrete(std::move(c)),subtype(type),parent(std::move(context)),
   parent_b4(std::move(b4)),controls(std::move(named)),terminated_id(id),parent_b4_revision(revision){}
};
detail::NotificationContextRootOrdinaryControlsExportLoan::NotificationContextRootOrdinaryControlsExportLoan(std::shared_ptr<Impl> p)
 :impl_(std::move(p)){}
detail::NotificationContextRootOrdinaryControlsExportLoan::~NotificationContextRootOrdinaryControlsExportLoan()=default;
detail::NotificationContextRootOrdinaryControlsExportLoan::Guard::Guard(std::shared_ptr<Impl> p):impl_(std::move(p)){}
detail::NotificationContextRootOrdinaryControlsExportLoan::Guard
detail::NotificationContextRootOrdinaryControlsExportLoan::capture_guard_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan& operation)const {
 Guard g(impl_);g.require_current_without_callbacks(operation);return g;
}
void detail::NotificationContextRootOrdinaryControlsExportLoan::Guard::require_current_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan& operation)const {
 BnwWorldViewControlTree::require_root_ordinary_controls_export_operand_without_callbacks(impl_,operation);
}
void detail::NotificationContextRootOrdinaryControlsExportLoan::Guard::require_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation)const {
 require_current_without_callbacks(operation);impl_->frame->full();require_current_without_callbacks(operation);
}
std::shared_ptr<const detail::NotificationContextLuaControlsNamedReference>
detail::NotificationContextRootOrdinaryControlsExportLoan::Guard::parent_controls_without_callbacks()const {
 impl_->frame->first();require_current_without_callbacks(*impl_->frame->operation);return impl_->controls;
}
std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>
detail::NotificationContextRootOrdinaryControlsExportLoan::Guard::receiver_without_callbacks()const {
 impl_->frame->first();require_current_without_callbacks(*impl_->frame->operation);return impl_->frame->receiver;
}
std::shared_ptr<const void>
detail::NotificationContextRootOrdinaryControlsExportLoan::Guard::concrete_cdata_owner_without_callbacks()const {
 impl_->frame->first();require_current_without_callbacks(*impl_->frame->operation);return impl_->concrete;
}
std::shared_ptr<ui::BnwWorldViewControlTree>
detail::NotificationContextRootOrdinaryControlsExportLoan::Guard::tree_without_callbacks()const {
 impl_->frame->first();require_current_without_callbacks(*impl_->frame->operation);const auto core=impl_->frame->core.lock();
 require(core!=nullptr,"ordinary CData actual Root Core retired");auto tree=core->layout_resource_tree.lock();
 require(tree!=nullptr,"ordinary CData actual Root Tree retired");return std::const_pointer_cast<ui::BnwWorldViewControlTree>(tree);
}
detail::NotificationContextRootOrdinaryControlsExportLoan::NativeSubtype
detail::NotificationContextRootOrdinaryControlsExportLoan::Guard::subtype_without_callbacks()const {
 impl_->frame->first();require_current_without_callbacks(*impl_->frame->operation);return impl_->subtype;
}
const char* detail::NotificationContextRootOrdinaryControlsExportLoan::Guard::terminated_id_without_callbacks()const {
 impl_->frame->first();require_current_without_callbacks(*impl_->frame->operation);return impl_->terminated_id.data();
}
std::u16string_view detail::NotificationContextRootOrdinaryControlsExportLoan::Guard::parent_screen_b4_without_callbacks()const {
 impl_->frame->first();require_current_without_callbacks(*impl_->frame->operation);
 return BnwWorldViewControlTree::root_ordinary_controls_screen_b4_without_callbacks(impl_);
}

void BnwWorldViewControlTree::require_root_ordinary_controls_export_operand_without_callbacks(
 const std::shared_ptr<detail::NotificationContextRootOrdinaryControlsExportLoan::Impl>& p,
 const runtime::BnwWorldViewSourceOperationLoan& operation){
 require_root_ordinary_controls_export_operand_physical_without_callbacks(p,operation,true);
}
void BnwWorldViewControlTree::require_root_ordinary_controls_export_operand_physical_without_callbacks(
 const std::shared_ptr<detail::NotificationContextRootOrdinaryControlsExportLoan::Impl>& p,
 const runtime::BnwWorldViewSourceOperationLoan& operation,bool live){
 using Loan=detail::NotificationContextRootOrdinaryControlsExportLoan;
 using Factory=detail::NotificationContextRootOrdinaryFactoryReturn;
 require(p&&p->frame,"ordinary +7C actual lexical operand missing");
 const auto f=p->frame;f->physical_without_callbacks(live);require(f->operation==&operation&&p->concrete&&
  f->id_outcome==RootOrdinaryControlsFrame::IdOutcome::positive_first_byte,
  "ordinary +7C positiveID/actual CData source changed");
 const auto actual=f->core.lock();require(actual&&f->realm&&f->realm->impl_&&
  f->realm->impl_->context.lock()==actual&&actual->context_ordinary_state==p->parent&&
  p->parent&&p->parent->impl_&&p->parent->impl_->b4_value==p->parent_b4&&
  p->parent_b4&&p->parent->impl_->b4_revision==p->parent_b4_revision&&
  f->read.impl_->evidence->controls==p->controls&&p->terminated_id==f->id58->bytes,
  "ordinary +7C real parent3C/B4/Controls/terminatedID changed");
 const auto& node=Core::node(actual->nodes,f->id);const auto receiver=f->receiver->impl_;
 require(receiver&&receiver->core.lock()==actual&&receiver->id==f->id&&
  receiver->canonical.lock()==f->receiver&&receiver->occurrence.lock()==node.layout_receiver_occurrence,
  "ordinary +7C concrete receiver is not the authentic current native allocation");
 switch(f->subtype){
  case Factory::Subtype::image:require(p->subtype==Loan::NativeSubtype::image&&
   node.layout_baseline->image_&&receiver->image.lock()==node.layout_baseline->image_&&
   node.layout_baseline->image_.get()==p->concrete.get(),"ordinary Image40EAF0 actual CData changed");break;
  case Factory::Subtype::box:require(p->subtype==Loan::NativeSubtype::box&&
   node.layout_baseline->box_&&receiver->box.lock()==node.layout_baseline->box_&&
   node.layout_baseline->box_.get()==p->concrete.get(),"ordinary Box6D1730 actual CData changed");break;
  case Factory::Subtype::label:require(p->subtype==Loan::NativeSubtype::label&&
   node.layout_baseline->standalone_label_&&receiver->standalone_label.lock()==node.layout_baseline->standalone_label_&&
   node.layout_baseline->standalone_label_.get()==p->concrete.get(),"ordinary Label564BD0 actual CData changed");break;
  case Factory::Subtype::grid:require(p->subtype==Loan::NativeSubtype::grid&&
   node.layout_baseline->grid_&&receiver->grid.lock()==node.layout_baseline->grid_&&
   node.layout_baseline->grid_.get()==p->concrete.get(),"ordinary Grid5B46F0 actual CData changed");break;
  case Factory::Subtype::button:
   require(p->subtype==Loan::NativeSubtype::button,"ordinary Button native CData type differs from earned factory");
   require_root_ordinary_button_cdata_owner_without_callbacks(actual,f->id,f->receiver,p->concrete,operation.operation_serial());break;
  case Factory::Subtype::stack:
   throw runtime::ScriptError("required genuine Stack ordinary allocation/return/receiver before typed CData");
 }
 f->physical_without_callbacks(live);
}

std::shared_ptr<const detail::NotificationContextRootOrdinaryControlsExportLoan>
BnwWorldViewControlTree::make_root_ordinary_controls_export_operand_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const std::shared_ptr<RootOrdinaryControlsFrame>& frame){
 using Loan=detail::NotificationContextRootOrdinaryControlsExportLoan;
 using Factory=detail::NotificationContextRootOrdinaryFactoryReturn;
 require(frame!=nullptr,"ordinary +7C needs the actual lexical v64 frame");frame->full();
 require(frame->operation==&operation&&frame->id_outcome==RootOrdinaryControlsFrame::IdOutcome::positive_first_byte,
  "ordinary +7C real positiveID observation missing");
 const auto actual=frame->core.lock();require(actual&&actual==core_,"ordinary +7C actual parent Core differs");
 const auto& state=*Core::node(actual->nodes,frame->id).layout_baseline;
 std::shared_ptr<const void> concrete;Loan::NativeSubtype subtype;
 switch(frame->subtype){
  case Factory::Subtype::image:concrete=state.image_;subtype=Loan::NativeSubtype::image;break;
  case Factory::Subtype::box:concrete=state.box_;subtype=Loan::NativeSubtype::box;break;
  case Factory::Subtype::label:concrete=state.standalone_label_;subtype=Loan::NativeSubtype::label;break;
  case Factory::Subtype::grid:concrete=state.grid_;subtype=Loan::NativeSubtype::grid;break;
  case Factory::Subtype::button:
   concrete=state.button_;subtype=Loan::NativeSubtype::button;
   require_root_ordinary_button_cdata_owner_without_callbacks(actual,frame->id,frame->receiver,concrete,operation.operation_serial());break;
  case Factory::Subtype::stack:
   throw runtime::ScriptError("required native Stack owned factory allocation/receiver before ordinary CData loan");
 }
 require(concrete!=nullptr,"ordinary +7C constructor returned without actual owned CData state");
 const auto parent=actual->context_ordinary_state;require(parent&&parent->impl_&&parent->impl_->b4_value,
  "ordinary +7C authentic parent3C/B4 absent");
 const auto b4=parent->impl_->b4_value;const auto revision=parent->impl_->b4_revision;
 const auto controls=frame->read.impl_->evidence->controls;const auto terminated=frame->id58->bytes;
 auto fields=std::make_shared<Loan::Impl>(frame,concrete,subtype,parent,b4,revision,controls,terminated);frame->full();
 auto loan=std::shared_ptr<const Loan>(new Loan(fields));frame->full();
 const auto guard=loan->capture_guard_without_callbacks(operation);guard.require_source_owned(operation);frame->full();return loan;
}

std::u16string_view BnwWorldViewControlTree::root_ordinary_controls_screen_b4_without_callbacks(
 const std::shared_ptr<detail::NotificationContextRootOrdinaryControlsExportLoan::Impl>& p){
 require(p&&p->frame,"ordinary +7C actual parentB4 operand absent");
 require_root_ordinary_controls_export_operand_without_callbacks(p,*p->frame->operation);
 return {p->parent_b4->terminated_data_without_callbacks(),p->parent_b4->value_.size()};
}
