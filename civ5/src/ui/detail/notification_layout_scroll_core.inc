// Include inside namespace civ5::ui AFTER notification_layout_parent_core.inc
// and its genuine complete Binding definition. Initializer graph mint remains
// the NativeContextIssuer's separate prerequisite, never a LayoutOperandLoan.
void BnwWorldViewControlTree::run_layout_scroll_receiver_owned(
 const Control& control,const LayoutOperandLoan& loan,LayoutScrollCommand command,double value){
 using Parent=detail::NotificationLayoutParentReceiver;
 using Scroll=detail::NotificationLayoutScrollReceiver;
 using View=Parent::TreeView;
 const auto saved=core_;const auto actual=control;std::shared_ptr<View::Binding> binding;
 try {
  saved->layout_origin();saved->layout_current_owned();saved->layout_origin();const auto id=saved->checked(actual);
  const auto& target=Core::node(saved->nodes,id);
  require(target.layout_baseline&&target.layout_baseline->scroll_&&target.authored.name=="ScrollPanel",
   "Scroll receiver requires actual captured typed canonical baseline");
  require(command!=LayoutScrollCommand::size_y||(std::isfinite(value)&&std::isfinite(static_cast<float>(value))),
   "Scroll SetSizeY requires actual finite FP32 Lua argument");
  saved->busy=true;struct Exit{bool& value;~Exit(){value=false;}}exit{saved->busy};
  binding=std::make_shared<View::Binding>(saved,loan,id);
  std::vector<Parent::FourCReturn> four_c_returns;
  {
   View view(binding,binding->captured);
   if(command==LayoutScrollCommand::size_y)Scroll::set_size_y(view,id,static_cast<float>(value));
   else {require(command==LayoutScrollCommand::internal_size,"Scroll command unissued");Scroll::internal_size(view,id);}
   view.require_full();view.require_pure();
   four_c_returns=view.four_c_returns_;view.require_pure();
  } // Actual View target destruction FIRST; no returned quote issued before it.
  binding->pure(false);require(binding->destruction_passed,"Scroll scope target destructor failed");
  auto& node=saved->nodes.at(id-1);const auto& baseline=*node.layout_baseline;const auto& native=*baseline.scroll_;
  LayoutState out; // New command invalidates previous returned ratio/size flags.
  out.native_offset=baseline.base_.offset_28_2c;out.calculated_size=baseline.base_.size_30_34;
  out.native_extent=baseline.base_.extent_20_24;out.native_bounds=baseline.base_.bounds_10_1c;
  out.source_serial=binding->serial;out.topology_revision=binding->domains.front().topology;
  out.effect_revision=binding->domains.front().effects;out.command_sequence=saved->layout_sequence;
  LayoutState::ScrollPanelFields fields;
  fields.viewport_30_34=baseline.base_.size_30_34;fields.calculated_84_88=native.calculated_84_88;
  fields.offset_9c_a0=native.offset_9c_a0;fields.internal_8c_90_94_98=native.internal_8c_98;
  fields.fraction_a4=native.fraction_a4;fields.ratio_a8=native.ratio_a8;
  fields.range_ac=native.range_ac;fields.range_b0=native.range_b0;fields.step_b4=native.step_b4;
  fields.flags_40=baseline.base_.flags_40;
  fields.store_mask=command==LayoutScrollCommand::size_y?3u:1020u;
  out.scroll_panel=fields;
  if(command==LayoutScrollCommand::internal_size){out.ratio=native.ratio_a8;out.internal_returned=true;}
  node.layout=std::move(out);
  for(const auto& returned:four_c_returns){
   const auto& domain=binding->domains.at(returned.key.domain);
   Core::LayoutFourCReturn stamp{returned.actual_offset,returned.baseline_sequence,binding->serial,domain.topology,returned.offset_store_sequence};
   if(returned.key.id==0)domain.core->context_layout_four_c_return=stamp;
   else {Core::node(domain.core->nodes,returned.key.id);domain.core->nodes.at(returned.key.id-1).layout_four_c_return=stamp;}
  }
  binding->pure(false);binding->finished=true;
 }catch(...){if(binding)binding->poison();saved->failed.store(true);throw;}
}

// Root's genuine initializer retains this move-only BODY receipt across the
// actual View destructor. Numeric prefixes never issue initialized state.
void detail::NotificationLayoutScrollReceiver::publish_slider_initialization_owned(
 InitializationReturn& receipt){
 const auto binding=receipt.binding_;
 try {
  binding->pure(false); // FIRST: saved canonical lifetimes/source/epochs.
  if(!binding->destruction_passed||binding->graph_active||binding->finished)
   throw std::runtime_error("Slider initializer target destruction did not complete");
  if(!receipt.state_)throw std::runtime_error("Slider initializer completion receipt moved/consumed");
  const auto found=std::find_if(binding->captured.begin(),binding->captured.end(),
   [&](const auto& node){return node.key==receipt.key_;});
  if(found==binding->captured.end()||receipt.key_.id==0)
   throw std::runtime_error("Slider initializer receipt is outside actual closed Binding");
  auto& actual=*found->state;
  if(actual.kind_!=detail::NotificationLayoutBaseline::Kind::slider||!actual.slider_||
     actual.slider_.get()!=receipt.state_.get()||actual.slider_.owner_before(receipt.state_)||
     receipt.state_.owner_before(actual.slider_)||actual.sequence_!=receipt.baseline_sequence_||
     !actual.slider_->initialization_entered_||actual.slider_->initialization_returned_)
   throw std::runtime_error("Slider initializer same canonical state/sequence changed or receipt replayed");
  actual.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_return);
  actual.slider_->initialization_returned_=true;actual.slider_->initialization_entered_=false;
  receipt.state_.reset();binding->pure(false);
 }catch(...){binding->poison();throw;}
}
