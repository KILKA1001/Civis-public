//After frozen Source16 real ordinary borrower and actual x87 environment guard.
//Only SetColor21/SetAlpha22/GetAlpha23; no Parent4C/94 or constructor replay.
void BnwWorldViewControlTree::set_native_ordinary_color58_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,const std::array<float,4>& supplied){
 using Base=detail::NotificationLayoutBaseline;using Parent=detail::NotificationLayoutParentReceiver;
 const auto actual=core_;
 const auto pure=[&](bool busy){actual->layout_origin(busy);
  const auto& current=actual->source->current_operation_loan();actual->layout_origin(busy);
  require(&current==&operation,"SetColor SAME current Source operation replaced");current.require_origin_without_callbacks();
  (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);actual->layout_origin(busy);};
 try{
  pure(false);actual->layout_current_owned();pure(false);operation.require_current();pure(false);
  const auto id=receiver->impl_->id;
  //Actual LabelA48DD0+58 is412770 RET16: no colour/cache/callback stores.
  //This exact no-op grants no generic Label Parent4C/94 or paint return.
  if(actual->nodes.at(id-1).layout_baseline->kind_==Base::Kind::label){pure(false);return;}
  actual->busy=true;struct Exit{bool& value;~Exit(){value=false;}}exit{actual->busy};pure(true);
  auto& state=*actual->nodes.at(id-1).layout_baseline;
  require(state.sequence_<=UINT64_MAX-4&&state.history_.max_size()-state.history_.size()>=4&&
   actual->effect_revision<=UINT64_MAX-4&&actual->layout_sequence<=UINT64_MAX-4,
   "SetColor actual four-store journals exhausted");
  state.history_.reserve(state.history_.size()+4);pure(true);
  std::array<float,4>* destination=nullptr;
  const auto button=state.button_;
  switch(state.kind_){
   case Base::Kind::image:
    require(state.actual_type_=="Image"&&state.image_,"SetColor actual Image6CBAF0 storage absent");
    destination=&state.image_->color_98_a4;break;
   case Base::Kind::box:
    require(state.actual_type_=="Box"&&state.box_,"SetColor actual Box414560 storage absent");
    destination=&state.box_->color_84_90;break;
   case Base::Kind::grid:
    require(state.actual_type_=="Grid"&&state.grid_&&!state.grid_->failed,"SetColor actual Grid6BF870 storage absent");
    destination=&state.grid_->color_90_9c;break;
   case Base::Kind::button:
    require(state.actual_type_=="Button"&&button&&button->backing_118_&&button->backing_118_->impl_,
     "SetColor actual Button5CABC0/118 storage absent");
    button->backing_118_->impl_->require_same_baseline_without_callbacks(*button,state);
    require(button->backing_118_->impl_->store_count<=UINT64_MAX-4,"SetColor Button118 four-store journal exhausted");
    destination=&button->color_12c_138;break;
   default:throw runtime::ScriptError("SetColor actual current subtype/export unissued");
  }
  //6CBAF0/414560/6BF870/5CABC0 copy exactly four argument words in order.
  //No clamp, finite predicate, alpha44 change, coordinates or paint callback.
  for(std::size_t lane=0;lane<4;++lane){
   pure(true);actual->initial_lua_preflight_store_owned();pure(true);
   if(state.kind_==Base::Kind::button){const auto fields=button->backing_118_->impl_;
    fields->require_same_baseline_without_callbacks(*button,state);
    fields->put(0x14u+4u*static_cast<std::uint32_t>(lane),std::bit_cast<std::uint32_t>(supplied[lane]));
   }
   state.record_owned(Base::Mutation::receiver_prefix);(*destination)[lane]=supplied[lane];
   ++actual->effect_revision;++actual->layout_sequence;
   actual->initial_lua_record_store_owned(Core::InitialLuaWriter::receiver_store,id,
    static_cast<std::uint64_t>(Parent::Store::Kind::ordinary_color));pure(true);
  }
  pure(true);operation.require_current();pure(true);
 }catch(...){actual->failed.store(true);throw;}
}

void BnwWorldViewControlTree::set_native_ordinary_alpha9c_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,float supplied){
 using Base=detail::NotificationLayoutBaseline;using Parent=detail::NotificationLayoutParentReceiver;
 const auto actual=core_;
 const auto pure=[&](bool busy){actual->layout_origin(busy);
  const auto& current=actual->source->current_operation_loan();actual->layout_origin(busy);
  require(&current==&operation,"SetAlpha SAME current Source operation replaced");current.require_origin_without_callbacks();
  (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);actual->layout_origin(busy);};
 try{
  pure(false);actual->layout_current_owned();pure(false);operation.require_current();pure(false);
  const auto id=receiver->impl_->id;
  actual->busy=true;struct Exit{bool& value;~Exit(){value=false;}}exit{actual->busy};pure(true);
  auto& state=*actual->nodes.at(id-1).layout_baseline;
  require(state.sequence_<UINT64_MAX&&state.history_.size()<state.history_.max_size()&&
   actual->effect_revision<UINT64_MAX&&actual->layout_sequence<UINT64_MAX,"SetAlpha actual store journal exhausted");
  state.history_.reserve(state.history_.size()+1);pure(true);
  const auto label=state.standalone_label_;
  if(state.kind_==Base::Kind::label){
   require(label&&label->impl_,"SetAlpha real Label physical44 absent");const auto fields=label->impl_;fields->first();
   require(fields->sequence<UINT64_MAX&&fields->history.size()<fields->history.max_size(),"SetAlpha Label44 journal exhausted");
   fields->history.reserve(fields->history.size()+1);fields->first();pure(true);
  }
  pure(true);actual->initial_lua_preflight_store_owned();pure(true);
  const auto word=std::bit_cast<std::uint32_t>(supplied);
  if(state.kind_==Base::Kind::label){const auto fields=label->impl_;fields->first();
   fields->history.push_back({++fields->sequence,0x44u});fields->fields[0x44u/4u]=word;}
  //All six actual vptr9C cells resolve580040 MOVSS[THIS+44],RET4.
  //This raw native field is independent of the four-channel colour above.
  state.record_owned(Base::Mutation::receiver_prefix);state.base_.alpha_word_44=word;
  ++actual->effect_revision;++actual->layout_sequence;
  actual->initial_lua_record_store_owned(Core::InitialLuaWriter::receiver_store,id,
   static_cast<std::uint64_t>(Parent::Store::Kind::ordinary_alpha));pure(true);
  operation.require_current();pure(true);
 }catch(...){actual->failed.store(true);throw;}
}

double BnwWorldViewControlTree::read_native_ordinary_alpha_a0_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete)const{
 using Base=detail::NotificationLayoutBaseline;const auto actual=core_;
 const auto pure=[&]{actual->layout_origin();
  const auto& current=actual->source->current_operation_loan();actual->layout_origin();
  require(&current==&operation,"GetAlpha SAME Source operation replaced");current.require_origin_without_callbacks();
  (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);actual->layout_origin();};
 try{
  pure();actual->layout_current_owned();pure();operation.require_current();pure();
  const auto& state=*Core::node(actual->nodes,receiver->impl_->id).layout_baseline;
  require(state.base_.alpha_word_44.has_value(),"GetAlpha actual physical44 store unissued");
  auto word=*state.base_.alpha_word_44;
  if(state.kind_==Base::Kind::label){const auto fields=state.standalone_label_?state.standalone_label_->impl_:nullptr;
   require(bool(fields),"GetAlpha real current Label physical44 storage absent");fields->first();
   const auto* physical=std::get_if<std::uint32_t>(&fields->fields[0x44u/4u]);
   require(physical&&*physical==word,"GetAlpha actual Label44/base alias changed or unknown");word=*physical;
  }
#if (defined(__GNUC__)||defined(__clang__)) && \
    (defined(__i386__)||defined(__x86_64__)) && !defined(_MSC_VER)
  //729400 FLD physical44 then89E222 FSTPdouble, before Lua allocation.
  //Execute actual x87 conversion/flags; no default alpha or host cast supplier.
  const BnwButton118ActualX87Environment before;before.require_admitted_without_callbacks();
  const auto control=before.control();double observed;
  __asm__ volatile("flds %[word]\n\tfstpl %[observed]"
   :[observed]"=m"(observed):[word]"m"(word):"memory","st");
  const BnwButton118ActualX87Environment after;
  require(after.control()==control&&after.empty(),"GetAlpha actual x87 control/stack changed");
  pure();return observed;
#else
  (void)word;throw runtime::ScriptError("GetAlpha729400/89E222 requires genuine GNU x86/x64 x87 backend");
#endif
 }catch(...){actual->failed.store(true);throw;}
}
