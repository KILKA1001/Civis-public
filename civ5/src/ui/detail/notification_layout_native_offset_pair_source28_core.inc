//Inside civ5::ui after actual ordinary storage/v64 returns and Parent::Binding.
//The Host lends an already returned physical CData owner. No expired lexical
//RootRead/ExportLoan guard is renewed, and no public Control/XML ID is cast.
std::uint64_t BnwWorldViewControlTree::native_ordinary_geometry_receiver_id_without_callbacks(
 const std::shared_ptr<Core>& actual,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete){
 using Base=detail::NotificationLayoutBaseline;
 using Factory=detail::NotificationContextRootOrdinaryFactoryReturn;
 using Returned=detail::NotificationLayoutNativeChildControlsReturn;
 require(actual&&receiver&&receiver->impl_&&concrete,"SetOffset current physical receiver/owner absent");
 const auto fields=receiver->impl_;const auto occurrence=fields->occurrence.lock();
 require(fields->creator==std::this_thread::get_id()&&fields->core.lock()==actual&&
  fields->canonical.lock()==receiver&&occurrence&&fields->id&&fields->serial&&
  occurrence->id==fields->id&&occurrence->constructor_serial==fields->serial,
  "SetOffset SAME canonical receiver/Core/constructor occurrence changed");
 const auto& node=Core::node(actual->nodes,fields->id);
 require(node.alive&&!node.dynamic&&node.layout_baseline&&node.full_xml_occurrence&&
  node.full_xml_occurrence->source.lock()==actual->source&&node.layout_receiver_occurrence==occurrence&&
  node.layout_baseline->base_.receiver_owned==receiver&&node.layout_baseline->parsed_source_serial_&&
  *node.layout_baseline->parsed_source_serial_==fields->serial,
  "SetOffset canonical native storage/parser/source association replaced");
 const auto& state=*node.layout_baseline;const auto& base=state.base_;
 const auto common=base.common_return_owned?base.common_return_owned->impl_:nullptr;
 require(common&&common->creator==std::this_thread::get_id()&&common->core.lock()==actual&&
  common->occurrence.lock()==occurrence&&common->selected.lock()==node.full_xml_occurrence&&
  common->receiver.lock()==receiver&&common->serial==fields->serial&&
  common->count==node.full_xml_occurrence->value.children.size(),
  "SetOffset lacks its genuinely returned canonical common storage");
 require_native_ordinary_current_children_without_callbacks(receiver,base.common_return_owned,base.children_78);
 //Common.count seals the constructor XML lineage; current native08 is mutable.
 //The immutable v64 return owns its exact allocation. Only its returned
 //physical fields are borrowed: its old RootRead cursor is not called here.
 const auto done=node.root_ordinary_controls_return;
 const auto returned=done?done->impl_:nullptr;
 require(returned&&returned->creator==std::this_thread::get_id()&&returned->alive.load()&&
  returned->canonical.lock()==done&&returned->origin==Returned::Origin::ordinary_control&&
  returned->ordinary&&returned->ordinary->exported&&returned->ordinary->export_loan,
  "SetOffset requires a genuine retained ordinary v64/CData return");
 const auto frame=returned->ordinary->frame;
 require(frame&&frame->creator==std::this_thread::get_id()&&!frame->alive.load()&&!frame->failed.load()&&
  frame->canonical.lock()==done&&frame->core.lock()==actual&&frame->receiver==receiver&&
  frame->id==fields->id&&frame->operation==returned->operation&&
  frame->traversed_count==frame->selected_children.size()&&
  returned->ordinary->completed_children.size()==frame->selected_children.size(),
  "SetOffset authentic closed ordinary occurrence/return custody changed");
 const auto same_owner=[&](const auto& typed){return typed&&typed.get()==concrete.get()&&
  !typed.owner_before(concrete)&&!concrete.owner_before(typed);};
 switch(state.kind_){
  case Base::Kind::image:
   require(frame->subtype==Factory::Subtype::image&&state.actual_type_=="Image"&&
    fields->image.lock()==state.image_&&same_owner(state.image_),"SetOffset actual Image CData replaced");break;
  case Base::Kind::grid:
   require(frame->subtype==Factory::Subtype::grid&&state.actual_type_=="Grid"&&
    fields->grid.lock()==state.grid_&&same_owner(state.grid_),"SetOffset actual Grid CData replaced");break;
  case Base::Kind::box:
   require(frame->subtype==Factory::Subtype::box&&state.actual_type_=="Box"&&
    fields->box.lock()==state.box_&&same_owner(state.box_),"SetOffset actual Box CData replaced");break;
  case Base::Kind::label:
   require(frame->subtype==Factory::Subtype::label&&state.actual_type_=="Label"&&
    fields->standalone_label.lock()==state.standalone_label_&&same_owner(state.standalone_label_),
    "SetOffset actual standalone Label CData replaced");break;
  case Base::Kind::button:
   require(frame->subtype==Factory::Subtype::button&&same_owner(state.button_),
    "SetOffset actual Button CData replaced");
   require_root_ordinary_button_cdata_owner_without_callbacks(actual,fields->id,receiver,concrete,fields->serial);break;
  default:
   throw runtime::ScriptError("SetOffset required genuine current ordinary subtype storage; Stack allocation/export remains unissued");
 }
 return fields->id;
}
void BnwWorldViewControlTree::require_native_ordinary_geometry_receiver_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete)const{
 const auto actual=core_;actual->layout_origin();
 const auto& current=actual->source->current_operation_loan();actual->layout_origin();
 require(&current==&operation,"SetOffset offered historical/foreign source operation");
 current.require_origin_without_callbacks();
 (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);
 actual->layout_origin();
}
void BnwWorldViewControlTree::set_native_ordinary_offset_4c8fe0_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,const std::array<float,2>& supplied){
 execute_native_ordinary_offset_pair_owned(operation,receiver,concrete,supplied,
  NativeOrdinaryOffsetPairMember::vector_4c8fe0);
}
void BnwWorldViewControlTree::set_native_ordinary_offset_69cd90_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,const std::array<float,2>& supplied){
 execute_native_ordinary_offset_pair_owned(operation,receiver,concrete,supplied,
  NativeOrdinaryOffsetPairMember::pair_69cd90);
}
void BnwWorldViewControlTree::set_native_ordinary_offset_axis_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,NativeOrdinaryOffsetAxis axis,float supplied){
 const auto actual=core_;
 try{
  require_native_ordinary_geometry_receiver_without_callbacks(operation,receiver,concrete);
  const auto id=receiver->impl_->id;
  std::array<float,2> pair;
  if(axis==NativeOrdinaryOffsetAxis::x_4a8380){
   //Actual4A8380 reads SAME current74 before loading its incoming scalarX.
   const float y=Core::node(actual->nodes,id).layout_baseline->base_.margin_70_74[1];
   const float x=supplied;pair={x,y};
  }else if(axis==NativeOrdinaryOffsetAxis::y_6158f0){
   //Actual6158F0 loads incoming scalarY, then SAME current70.
   const float y=supplied;
   const float x=Core::node(actual->nodes,id).layout_baseline->base_.margin_70_74[0];pair={x,y};
  }else throw runtime::ScriptError("Required genuine scalar offset method identity");
  set_native_ordinary_offset_69cd90_owned(operation,receiver,concrete,pair);
  require_native_ordinary_geometry_receiver_without_callbacks(operation,receiver,concrete);
 }catch(...){actual->failed.store(true);throw;}
}
void BnwWorldViewControlTree::execute_native_ordinary_offset_pair_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,const std::array<float,2>& supplied,NativeOrdinaryOffsetPairMember member){
 using Base=detail::NotificationLayoutBaseline;
 using Parent=detail::NotificationLayoutParentReceiver;using View=Parent::TreeView;
 const auto actual=core_;std::shared_ptr<View::Binding> binding;
 const auto pure=[&](bool busy){actual->layout_origin(busy);
  const auto& current=actual->source->current_operation_loan();actual->layout_origin(busy);
  require(&current==&operation,"SetOffset current source operation replaced");current.require_origin_without_callbacks();
  (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);actual->layout_origin(busy);};
 try{
  require(member==NativeOrdinaryOffsetPairMember::vector_4c8fe0||member==NativeOrdinaryOffsetPairMember::pair_69cd90,
   "Native offset pair requires its real executing member identity");
  pure(false);actual->layout_current_owned();pure(false);operation.require_current();pure(false);
  const auto id=receiver->impl_->id;
  //Actual UCOMISS/LAHF/TEST44 compares both incoming FP32s BEFORE floor.
  //Equal +/-0 and equal infinities return without stores or virtual dispatch.
  const auto old=Core::node(actual->nodes,id).layout_baseline->base_.margin_70_74;
  if(old[0]==supplied[0]&&old[1]==supplied[1]){pure(false);return;}
  actual->busy=true;struct Exit{bool&value;~Exit(){value=false;}}exit{actual->busy};pure(true);
  auto& state=*actual->nodes.at(id-1).layout_baseline;
  require(state.history_.max_size()-state.history_.size()>=4&&state.sequence_<=UINT64_MAX-4&&
   actual->effect_revision<=UINT64_MAX-4&&actual->layout_sequence<=UINT64_MAX-4,
   "SetOffset physical store history exhausted");
  state.history_.reserve(state.history_.size()+4);pure(true);
  //Nonfinite changed inputs are an explicit unsupported execution profile,
  //not a Lua decoder type/finite predicate or a fabricated successful result.
  for(float v:supplied)require(std::isfinite(v),"SetOffset changed nonfinite FP32 floor profile unsupported");
  const auto commit=[&](std::array<float,2>& destination,std::size_t axis,float value){
   pure(true);actual->initial_lua_preflight_store_owned();pure(true);
   state.record_owned(Base::Mutation::receiver_prefix);destination[axis]=value;
   ++actual->effect_revision;++actual->layout_sequence;
   actual->initial_lua_record_store_owned(Core::InitialLuaWriter::receiver_store,id,
    static_cast<std::uint64_t>(Parent::Store::Kind::ordinary_offset));pure(true);
  };
  //4C8FE0 stores4C9017/9028/903C/9044;69CD90 stores69CDC2/D1/E6/F1.
  //Original float SetOffsetVal uses69CD90; no authored/int-truncation setter.
  const double floor_x=std::floor(static_cast<double>(supplied[0]));
  commit(state.base_.margin_70_74,0,static_cast<float>(floor_x));
  const double floor_y=std::floor(static_cast<double>(supplied[1]));
  commit(state.base_.margin_70_74,1,static_cast<float>(floor_y));
  require(std::isfinite(state.base_.size_30_34[0]),"SetOffset actual live30 size outside supported profile");
  //4C8FE0 uses its rounded localX;69CD90 freshly reloads stored70.
  //Both real FP32 operands are this same just-stored margin in the port.
  const double actual_x=member==NativeOrdinaryOffsetPairMember::pair_69cd90?
   static_cast<double>(state.base_.margin_70_74[0]):static_cast<double>(static_cast<float>(floor_x));
  const float extent_x=static_cast<float>(std::fabs(actual_x)+static_cast<double>(state.base_.size_30_34[0]));
  require(std::isfinite(extent_x),"SetOffset actual FSTP20 nonfinite profile unsupported");
  commit(state.base_.extent_20_24,0,extent_x);
  require(std::isfinite(state.base_.size_30_34[1]),"SetOffset actual live34 size outside supported profile");
  //4C8FE0 retains floorY in x87; distinct69CD90 RELOADS the actual
  //stored74. Preserve both proven operand routes in this common owner.
  const double actual_y=member==NativeOrdinaryOffsetPairMember::pair_69cd90?
   static_cast<double>(state.base_.margin_70_74[1]):floor_y;
  const float extent_y=static_cast<float>(std::fabs(actual_y)+static_cast<double>(state.base_.size_30_34[1]));
  require(std::isfinite(extent_y),"SetOffset actual FSTP24 nonfinite profile unsupported");
  commit(state.base_.extent_20_24,1,extent_y);
  //All four prefix stores remain real/ journalled if a later supplier fails.
  binding=std::make_shared<View::Binding>(actual,operation);pure(true);
  std::vector<Parent::Store> stores;std::vector<Parent::FourCReturn> returns;
  {
   View view(binding,binding->captured);view.require_pure();
   const Parent::NodeKey key{0,id};
   Parent::slot4c(view,key); //Actual4C9047 OR69CDF4; genuine subtype dispatch.
   view.require_pure();Parent::slot94(view,key); //Fresh4C9053 OR69CE00, AFTER4C returned.
   view.require_full();view.require_pure();stores=view.stores_;returns=view.four_c_returns_;view.require_pure();
  }
  binding->pure(false);require(binding->destruction_passed,"SetOffset required actual4C/94 View destruction return");
  const auto mirror=[&](const std::shared_ptr<Core>& core,std::uint64_t control,
   std::uint64_t topology,std::uint64_t effects){
   if(!control)return;(void)Core::node(core->nodes,control);auto& n=core->nodes.at(control-1);
   const auto& current=*n.layout_baseline;auto quote=n.layout.value_or(LayoutState{});
   quote.native_offset=current.base_.offset_28_2c;quote.calculated_size=current.base_.size_30_34;
   quote.native_extent=current.base_.extent_20_24;quote.native_bounds=current.base_.bounds_10_1c;
   quote.calculate_returned=false;quote.source_serial=binding->serial;
   quote.topology_revision=topology;quote.effect_revision=effects;quote.command_sequence=core->layout_sequence;
   n.layout=std::move(quote);
  };
  mirror(actual,id,binding->domains.front().topology,binding->domains.front().effects);
  for(const auto& store:stores){const auto& domain=binding->domains.at(store.key.domain);
   mirror(domain.core,store.key.id,domain.topology,domain.effects);
   if(store.key.id&&store.kind==Parent::Store::Kind::stack_anchor){auto& n=domain.core->nodes.at(store.key.id-1);
    require(n.layout&&n.layout_baseline&&n.layout_baseline->stack_,"SetOffset real Stack anchor mirror absent");
    n.layout->anchor_origin={n.layout_baseline->stack_->field_90,n.layout_baseline->stack_->field_94};
    n.layout->anchor_prepass_stored=true;n.layout->anchoring_returned=true;}
  }
  for(const auto& stamp:returns){const auto& domain=binding->domains.at(stamp.key.domain);
   Core::LayoutFourCReturn earned{stamp.actual_offset,stamp.baseline_sequence,binding->serial,
    domain.topology,stamp.offset_store_sequence};
   if(stamp.key.id==0)domain.core->context_layout_four_c_return=earned;
   else domain.core->nodes.at(stamp.key.id-1).layout_four_c_return=earned;
  }
  binding->pure(false);binding->finished=true;binding.reset();
  pure(true);operation.require_current();pure(true);
 }catch(...){if(binding)binding->poison();actual->failed.store(true);throw;}
}
