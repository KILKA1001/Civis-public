//Current returned ordinary CData; actual member7A5510/4AB4F0 dispatches v98.
//No resize-registration replay, XML dimensions or successful layout substitute.
namespace {
float bnw_controlbase670f10_extent_owned(const float& margin,const float& size){
#if (defined(__GNUC__)||defined(__clang__)) && \
    (defined(__i386__)||defined(__x86_64__)) && !defined(_MSC_VER)
 //Reuse the actual FXSAVE/CW guard already included with Button118 above.
 //670F29/34 is FLD/FABS/FADD/FSTP; host double/long-double or SSE addition
 //would replace its real x87 precision/rounding control and sticky flags.
 const BnwButton118ActualX87Environment before;before.require_admitted_without_callbacks();
 const auto control=before.control();float result;
 __asm__ volatile("flds %[margin]\n\tfabs\n\tfadds %[size]\n\tfstps %[result]"
  :[result]"=m"(result):[margin]"m"(margin),[size]"m"(size):"memory","st");
 const BnwButton118ActualX87Environment after;
 require(after.control()==control&&after.empty(),"SetSize670F10 actual x87 control/stack changed");
 return result;
#else
 (void)margin;(void)size;
 throw runtime::ScriptError("SetSize670F10 requires genuine GNU x86/x64 x87 backend");
#endif
}
float bnw_controlbase4b0e90_dimension_owned(std::uint32_t supplied){
#if (defined(__GNUC__)||defined(__clang__)) && \
    (defined(__i386__)||defined(__x86_64__)) && !defined(_MSC_VER)
 const BnwButton118ActualX87Environment before;before.require_admitted_without_callbacks();
 const auto control=before.control();const auto signed_value=std::bit_cast<std::int32_t>(supplied);
 const std::uint32_t correction_word=0x4f800000u;float result; //Physical9DD390.
 __asm__ volatile("fildl %[signed_value]\n\ttestl %[word],%[word]\n\t"
  "jns .Lbnw_size_dimension_%=\n\tfadds %[correction]\n\t"
  ".Lbnw_size_dimension_%=:\n\tfstps %[result]"
  :[result]"=m"(result):[signed_value]"m"(signed_value),[word]"r"(supplied),
   [correction]"m"(correction_word):"memory","cc","st");
 const BnwButton118ActualX87Environment after;
 require(after.control()==control&&after.empty(),"Grid4B0E90 actual dimension x87 control/stack changed");
 return result;
#else
 (void)supplied;
 throw runtime::ScriptError("Grid4B0E90 requires genuine GNU x86/x64 dimension conversion backend");
#endif
}
}
void BnwWorldViewControlTree::set_native_ordinary_size_axis_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,NativeOrdinarySizeAxis axis,float supplied){
 const auto actual=core_;
 try{
  require_native_ordinary_geometry_receiver_without_callbacks(operation,receiver,concrete);
  std::array<float,2> pair;
  if(axis==NativeOrdinarySizeAxis::x_4e99c0){
   //4E99C0 FLD[SAME+34] BEFORE FLD incomingX and actual v98 call.
   const float y=native_ordinary_current_size_cell_without_callbacks(actual,receiver,concrete,1);
   const float x=supplied;pair={x,y};
  }else if(axis==NativeOrdinarySizeAxis::y_68a240){
   //68A240 FLD incomingY BEFORE actual fresh FLD[SAME+30].
   const float y=supplied;
   const float x=native_ordinary_current_size_cell_without_callbacks(actual,receiver,concrete,0);pair={x,y};
  }else throw runtime::ScriptError("Required genuine scalar SetSize member identity");
  set_native_ordinary_size98_owned(operation,receiver,concrete,pair);
  require_native_ordinary_geometry_receiver_without_callbacks(operation,receiver,concrete);
 }catch(...){actual->failed.store(true);throw;}
}
void BnwWorldViewControlTree::set_native_ordinary_size98_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,const std::array<float,2>& supplied){
 using Base=detail::NotificationLayoutBaseline;
 using Parent=detail::NotificationLayoutParentReceiver;using View=Parent::TreeView;
 const auto actual=core_;std::shared_ptr<View::Binding> binding;
 const auto pure=[&](bool busy){actual->layout_origin(busy);
  const auto& current=actual->source->current_operation_loan();actual->layout_origin(busy);
  require(&current==&operation,"SetSize SAME current Source operation replaced");current.require_origin_without_callbacks();
  (void)native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);actual->layout_origin(busy);};
 try{
  pure(false);actual->layout_current_owned();pure(false);operation.require_current();pure(false);
  const auto id=receiver->impl_->id;
  actual->busy=true;struct Exit{bool& value;~Exit(){value=false;}}exit{actual->busy};pure(true);
  auto& state=*actual->nodes.at(id-1).layout_baseline;
  const auto kind=state.kind_;
  const auto label=state.standalone_label_;const auto button=state.button_;const auto image=state.image_;const auto grid=state.grid_;
  require(state.sequence_<=UINT64_MAX-6&&state.history_.max_size()-state.history_.size()>=6&&
   actual->effect_revision<=UINT64_MAX-6&&actual->layout_sequence<=UINT64_MAX-6,"SetSize native prefix journals exhausted");
  state.history_.reserve(state.history_.size()+6);pure(true);
  if(kind==Base::Kind::label){
   require(label&&label->impl_,"SetSize genuine Label physical storage missing");const auto fields=label->impl_;fields->first();
   require(fields->sequence<=UINT64_MAX-4&&fields->history.max_size()-fields->history.size()>=4,
    "SetSize current Label physical journal exhausted");fields->history.reserve(fields->history.size()+4);fields->first();pure(true);
  }
  const auto commit=[&](std::array<float,2>& target,std::size_t lane,float value,
   std::uint32_t label_offset=0,std::uint32_t button_offset=0){
   pure(true);actual->initial_lua_preflight_store_owned();pure(true);
   if(label_offset){const auto fields=label->impl_;fields->first();
    require(kind==Base::Kind::label&&fields->history.size()<fields->history.capacity()&&fields->sequence<UINT64_MAX,
     "SetSize actual Label store capacity/custody changed");
    fields->history.push_back({++fields->sequence,label_offset});
    fields->fields[label_offset/4]=std::bit_cast<std::uint32_t>(value);
   }
   if(button_offset){const auto fields=button->backing_118_->impl_;
    fields->require_same_baseline_without_callbacks(*button,state);
    fields->put(button_offset,std::bit_cast<std::uint32_t>(value));
   }
   state.record_owned(Base::Mutation::receiver_prefix);target[lane]=value;
   ++actual->effect_revision;++actual->layout_sequence;
   actual->initial_lua_record_store_owned(Core::InitialLuaWriter::receiver_store,id,
    static_cast<std::uint64_t>(Parent::Store::Kind::ordinary_size));pure(true);
  };
  //Actual selected v98: Image5485B0,Button6FEDA0,Grid408E90;
  //Box/Label/Stack670F10. Prefix cells are genuine independently owned
  //destinations, and occur BEFORE the common670F10 size/extent stores.
  if(kind==Base::Kind::image){
   require(image&&state.actual_type_=="Image","SetSize actual Image v98 receiver missing");
   commit(image->image_a8_ac,0,supplied[0]);commit(image->image_a8_ac,1,supplied[1]);
  }else if(kind==Base::Kind::button){
   require(button&&state.actual_type_=="Button"&&button->backing_118_&&button->backing_118_->impl_,
    "SetSize real Button6FEDA0/118 backing unavailable");
   const auto fields=button->backing_118_->impl_;fields->require_same_baseline_without_callbacks(*button,state);
   require(fields->store_count<=UINT64_MAX-2,"SetSize Button13C/140 physical journal exhausted");
   //Pair the actual original +118 word24/28 with its typed alias. No old
   //constructor/6F6C10 return is promoted by these new explicit native stores.
   commit(button->image_13c_140,0,supplied[0],0,0x24);
   commit(button->image_13c_140,1,supplied[1],0,0x28);
   fields->require_same_baseline_without_callbacks(*button,state);
  }else require(kind==Base::Kind::box||kind==Base::Kind::label||kind==Base::Kind::grid,
   "SetSize actual subtype v98/physical export unissued");
  //Complete670F10: always stores30,34,20,24, even identical sizes.
  //No clamp/nonnegative/int/changed-only predicate. Genuine reached finite
  //layout receiver may refuse later; these preceding physical stores remain.
  commit(state.base_.size_30_34,0,supplied[0],kind==Base::Kind::label?0x30u:0u);
  commit(state.base_.size_30_34,1,supplied[1],kind==Base::Kind::label?0x34u:0u);
  const float extent_x=bnw_controlbase670f10_extent_owned(state.base_.margin_70_74[0],state.base_.size_30_34[0]);
  commit(state.base_.extent_20_24,0,extent_x,kind==Base::Kind::label?0x20u:0u);
  const float extent_y=bnw_controlbase670f10_extent_owned(state.base_.margin_70_74[1],state.base_.size_30_34[1]);
  commit(state.base_.extent_20_24,1,extent_y,kind==Base::Kind::label?0x24u:0u);
  binding=std::make_shared<View::Binding>(actual,operation);pure(true);
  std::vector<Parent::Store> stores;std::vector<Parent::FourCReturn> returns;
  {
   View view(binding,binding->captured);view.require_pure();const Parent::NodeKey key{0,id};
   Parent::reprocess_anchoring(view,key); //Actual670F3F, all child/parent effects.
   view.require_pure();Parent::slot94(view,key); //Distinct FRESH670F4E suffix, not the earlier94.
   view.require_full();view.require_pure();pure(true);
   if(kind==Base::Kind::grid){
    require(grid&&state.actual_type_=="Grid"&&!grid->failed,"SetSize actual Grid408E90 receiver changed");
    //408EAC only AFTER670F10 RETURN: actual47CC00 dirty bit1 branch.
    if(state.base_.flags_40&1u){
     require((_mm_getcsr()&0xe040u)==0,"Grid47CC00 actual SSE normalization mode unsupported");
     require(state.sequence_<=UINT64_MAX-96&&state.history_.max_size()-state.history_.size()>=96,
      "Grid47CC00/4B0E90 actual reached store journal exhausted");
     state.history_.reserve(state.history_.size()+96);view.require_full();view.require_pure();pure(true);
     const auto add=[](float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));};
     const auto sub=[](float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));};
     const auto mul=[](float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));};
     const auto div=[](float a,float b){return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(a),_mm_set_ss(b)));};
     const auto get=[&](std::size_t block,std::size_t word){view.require_pure();return std::bit_cast<float>(grid->words_c8_274[block][word]);};
     const auto set=[&](std::size_t block,std::size_t word,float value){
      view.require_pure();pure(true);view.preflight_store_owned(key);pure(true);
      state.record_owned(Base::Mutation::receiver_prefix);grid->words_c8_274[block][word]=std::bit_cast<std::uint32_t>(value);
      view.record_store_owned(key,Parent::Store::Kind::grid);view.require_pure();pure(true);
     };
     const auto size=state.base_.size_30_34;const auto k=grid->kerning_a0_ac;
     //Actual47CC5A/A2/AA/B2/DF then47CD0C store order. These arrays are
     //the same native C8/110/158/1A0/1E8/230 fields, not a staged graphic DTO.
     set(1,2,add(sub(add(sub(size[0],get(1,12)),k[0]),get(1,10)),k[1]));
     set(1,7,add(sub(add(sub(size[1],get(1,11)),k[2]),get(1,15)),k[3]));
     set(1,0,add(add(sub(sub(size[0],get(1,6)),get(1,8)),k[1]),k[0]));
     set(1,1,add(add(sub(sub(size[1],get(1,3)),get(1,5)),k[3]),k[2]));
     set(1,9,add(sub(add(sub(size[1],get(1,13)),k[2]),get(1,17)),k[3]));
     set(1,4,add(sub(add(sub(size[0],get(1,16)),k[0]),get(1,14)),k[1]));
     set(0,10,0.0f);set(0,11,0.0f);set(0,2,sub(get(1,10),k[0]));set(0,3,0.0f);
     set(0,12,sub(size[0],get(1,12)));set(0,13,0.0f);set(0,6,0.0f);set(0,7,sub(get(1,11),k[2]));
     set(0,0,sub(get(1,6),k[0]));set(0,1,sub(get(1,3),k[2]));
     set(0,8,sub(size[0],get(1,8)));set(0,9,sub(get(1,13),k[2]));
     set(0,14,0.0f);set(0,15,sub(size[1],get(1,15)));
     set(0,4,sub(get(1,14),k[0]));set(0,5,sub(size[1],get(1,5)));
     set(0,16,sub(size[0],get(1,16)));set(0,17,sub(size[1],get(1,17)));
     //47CE55 tail4B0E90: do not re-enter initializer70F8E0 or its separate
     //800000 origin flag effects. Actual NULL278 legitimately preserves UVs.
     require(grid->texture_binding_earned,"Grid4B0E90 current278 binding has not returned");
     if(grid->field_278==detail::NotificationLayoutGridState::Field278::constructor_null){
      require(grid->texture_key.empty()&&!grid->texture_keeper&&!grid->texture_extent_278,
       "Grid4B0E90 genuine NULL278 contradicts retained current resource");
     }else{
      const auto keeper=grid->texture_keeper;
      require(keeper&&grid->texture_extent_278&&*grid->texture_extent_278==
       std::array<std::uint32_t,2>{keeper->native_114_118_[1],keeper->native_114_118_[0]},
       "Grid4B0E90 SAME real keeper118WIDTH/114HEIGHT required");
      const auto dims=*grid->texture_extent_278;
      require(dims[0]&&dims[1],"Grid4B0E90 positive resource divisor unsupported");
      //Actual4B0EAC..DE FILD signed/+2^32/FSTP uses the current x87 CW.
      //Reciprocal DIVSS follows the FIRST raw1E8/1EC stores, not before them.
      const float width=bnw_controlbase4b0e90_dimension_owned(dims[0]);
      const float height=bnw_controlbase4b0e90_dimension_owned(dims[1]);
      float ix,iy;const auto offset=grid->current_c0_c4;
      for(std::size_t pair=0;pair<9;++pair){const auto x=pair*2,y=x+1;
       const auto sx=add(get(2,x),offset[0]);float sy;
       if(pair==0||pair==4||pair==6||pair==8)sy=add(get(2,y),offset[1]);
       set(4,x,sx);
       if(pair==0){
        set(4,y,sy);ix=div(1.0f,width);iy=div(1.0f,height);
        set(4,x,mul(get(4,x),ix));set(4,y,mul(get(4,y),iy));
       }else if(pair==6){
        //4B1463/147F/1485/1499: raw218,raw21C,norm218,norm21C.
        set(4,y,sy);set(4,x,mul(get(4,x),ix));set(4,y,mul(get(4,y),iy));
       }else{
        if(pair==3)sy=add(get(2,y),offset[1]); //4B11D0, after raw200.
        set(4,x,mul(get(4,x),ix));
        if(pair==1||pair==2||pair==5||pair==7)sy=add(get(2,y),offset[1]);
        set(4,y,sy);set(4,y,mul(get(4,y),iy));
       }
       const auto nx=std::bit_cast<float>(std::bit_cast<std::uint32_t>(get(2,x))^0x80000000u);
       const auto ny=std::bit_cast<float>(std::bit_cast<std::uint32_t>(get(2,y))^0x80000000u);
       set(5,x,add(nx,get(3,x)));set(5,y,add(get(3,y),ny));
       set(5,x,mul(ix,get(5,x)));set(5,y,mul(get(5,y),iy));
      }
     }
    } //Actual flags1 clear returns without ANY cache/texture call or stores.
   }
   view.require_full();view.require_pure();pure(true);
   stores=view.stores_;returns=view.four_c_returns_;view.require_pure();
  }
  binding->pure(false);require(binding->destruction_passed,"SetSize actual v98/reflow/cache scope destruction incomplete");
  const auto mirror=[&](const std::shared_ptr<Core>& core,std::uint64_t control,std::uint64_t topology,std::uint64_t effects){
   if(!control)return;(void)Core::node(core->nodes,control);auto& n=core->nodes.at(control-1);
   const auto& current=*n.layout_baseline;auto value=n.layout.value_or(LayoutState{});
   value.native_offset=current.base_.offset_28_2c;value.calculated_size=current.base_.size_30_34;
   value.native_extent=current.base_.extent_20_24;value.native_bounds=current.base_.bounds_10_1c;
   value.calculate_returned=false;value.source_serial=binding->serial;value.topology_revision=topology;
   value.effect_revision=effects;value.command_sequence=core->layout_sequence;n.layout=std::move(value);
  };
  mirror(actual,id,binding->domains.front().topology,binding->domains.front().effects);
  for(const auto& store:stores){const auto& domain=binding->domains.at(store.key.domain);
   mirror(domain.core,store.key.id,domain.topology,domain.effects);
   if(store.key.id&&store.kind==Parent::Store::Kind::stack_anchor){auto& n=domain.core->nodes.at(store.key.id-1);
    require(n.layout&&n.layout_baseline&&n.layout_baseline->stack_,"SetSize returned Stack anchor owner absent");
    n.layout->anchor_origin={n.layout_baseline->stack_->field_90,n.layout_baseline->stack_->field_94};
    n.layout->anchor_prepass_stored=true;n.layout->anchoring_returned=true;}
  }
  for(const auto& stamp:returns){const auto& domain=binding->domains.at(stamp.key.domain);
   Core::LayoutFourCReturn value{stamp.actual_offset,stamp.baseline_sequence,binding->serial,domain.topology,stamp.offset_store_sequence};
   if(stamp.key.id==0)domain.core->context_layout_four_c_return=value;else domain.core->nodes.at(stamp.key.id-1).layout_four_c_return=value;
  }
  binding->pure(false);binding->finished=true;binding.reset();pure(true);operation.require_current();pure(true);
 }catch(...){if(binding)binding->poison();actual->failed.store(true);throw;}
}
