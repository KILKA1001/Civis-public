//After Source14's actual flags/Label40 owner and the real Parent::Binding.
//Only genuine current ordinary Controls/CData can enter. No old initializer
//or XML-derived dimensions/child count are used as a returned layout owner.
void BnwWorldViewControlTree::reprocess_native_ordinary_anchoring_4a7f10_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete){
 using Parent=detail::NotificationLayoutParentReceiver;using View=Parent::TreeView;
 const auto actual=core_;std::shared_ptr<View::Binding> binding;
 const auto pure=[&](bool busy){
  actual->layout_origin(busy);
  const auto& current=actual->source->current_operation_loan();actual->layout_origin(busy);
  require(&current==&operation,"ReprocessAnchoring current SAME Source operation replaced");
  current.require_origin_without_callbacks();
  (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);
  actual->layout_origin(busy);
 };
 try{
  pure(false);actual->layout_current_owned();pure(false);operation.require_current();pure(false);
  const auto id=receiver->impl_->id;
  actual->busy=true;struct Exit{bool& value;~Exit(){value=false;}}exit{actual->busy};pure(true);
  binding=std::make_shared<View::Binding>(actual,operation);pure(true);
  std::vector<Parent::Store> stores;std::vector<Parent::FourCReturn> returns;
  {
   View view(binding,binding->captured);view.require_pure();
   //Complete Original4A7F10: v4C at4A7F18, FRESH vptr/v94 at4A7F1A..24,
   //then real78/data04/count08 BEFORE forward child4A7F42 recursion.
   //The existing owner executes that order and each genuine typed dispatch,
   //actual parent94 propagation and reached full-child-vector admission.
   //Its unissued Label generic4C/94, Stack export or Context vector remains
   //an actual refusal; no successful anchored rectangle substitutes them.
   Parent::reprocess_anchoring(view,{0,id});
   view.require_full();view.require_pure();
   stores=view.stores_;returns=view.four_c_returns_;view.require_pure();
  }
  binding->pure(false);
  require(binding->destruction_passed,"ReprocessAnchoring genuine layout View did not return/destruct normally");
  //Refresh only existing port mirrors AFTER actual virtual/child returns and
  //View destruction. These are not extra native stores or an XML snapshot.
  const auto mirror=[&](const std::shared_ptr<Core>& core,std::uint64_t control,
   std::uint64_t topology,std::uint64_t effects){
   if(!control)return;(void)Core::node(core->nodes,control);auto& n=core->nodes.at(control-1);
   require(n.layout_baseline.has_value(),"ReprocessAnchoring returned mirror lacks actual baseline");
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
    require(n.layout&&n.layout_baseline&&n.layout_baseline->stack_,"ReprocessAnchoring returned Stack mirror lacks actual owner");
    n.layout->anchor_origin={n.layout_baseline->stack_->field_90,n.layout_baseline->stack_->field_94};
    n.layout->anchor_prepass_stored=true;n.layout->anchoring_returned=true;
   }
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

float BnwWorldViewControlTree::native_ordinary_current_size_cell_without_callbacks(
 const std::shared_ptr<Core>& actual,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,std::size_t axis){
 require(axis<2,"Size getter actual axis unavailable");
 const auto id=native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);
 const auto& state=*Core::node(actual->nodes,id).layout_baseline;
 //All six actual selected ctor vptrs have A4=77E730, A8=691D40,
 //AC=428990. Stack export remains unissued in the genuine receiver borrower.
 float current=state.base_.size_30_34[axis];
 if(state.kind_==detail::NotificationLayoutBaseline::Kind::label){
  //Label owns separately typed physical words, paired by its real parser/
  //initializer writers. Require those actual current cells and their alias,
  //rather than treating the Baseline projection as a native field grant.
  const auto fields=state.standalone_label_?state.standalone_label_->impl_:nullptr;
  require(bool(fields),"Size getter current Label physical storage absent");fields->first();
  const auto* word=std::get_if<std::uint32_t>(&fields->fields[(0x30u+4u*axis)/4u]);
  require(word&&*word==std::bit_cast<std::uint32_t>(current),"Size getter Label30/34 current physical alias changed/unknown");
  current=std::bit_cast<float>(*word);
 }
 return current;
}

std::array<float,2> BnwWorldViewControlTree::read_native_ordinary_size_a4_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete)const{
 const auto actual=core_;
 const auto pure=[&]{actual->layout_origin();
  const auto& current=actual->source->current_operation_loan();actual->layout_origin();
  require(&current==&operation,"GetSize offered a retired/different Source operation");current.require_origin_without_callbacks();
  (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);actual->layout_origin();};
 try{
  pure();actual->layout_current_owned();pure();operation.require_current();pure();
  //5F4190 JMP[SAME vptr+A4]→77E730: actual out buffer receives30
  //then34 and is returned in EAX/RET4. No callback occurs between cells.
  const float x=native_ordinary_current_size_cell_without_callbacks(actual,receiver,concrete,0);
  const float y=native_ordinary_current_size_cell_without_callbacks(actual,receiver,concrete,1);
  const std::array<float,2> observed{x,y};
  pure();return observed; //POD copy only; no raw pointer or strong Lua borrower.
 }catch(...){actual->failed.store(true);throw;}
}
float BnwWorldViewControlTree::read_native_ordinary_size_axis_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,NativeOrdinarySizeRead read)const{
 const auto actual=core_;
 const auto pure=[&]{actual->layout_origin();
  const auto& current=actual->source->current_operation_loan();actual->layout_origin();
  require(&current==&operation,"Size getter offered a retired/different Source operation");current.require_origin_without_callbacks();
  (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);actual->layout_origin();};
 try{
  pure();actual->layout_current_owned();pure();operation.require_current();pure();
  std::size_t axis;
  switch(read){
   case NativeOrdinarySizeRead::virtual_a8_691d40:
   case NativeOrdinarySizeRead::direct_30_467420:axis=0;break;
   case NativeOrdinarySizeRead::virtual_ac_428990:
   case NativeOrdinarySizeRead::direct_34_467420:axis=1;break;
   default:throw runtime::ScriptError("Required actual size getter identity");
  }
  const float observed=native_ordinary_current_size_cell_without_callbacks(actual,receiver,concrete,axis);
  pure();return observed;
 }catch(...){actual->failed.store(true);throw;}
}
