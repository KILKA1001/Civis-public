//Actual runtime common fields40/48/54, distinct from parser facades.
std::uint64_t BnwWorldViewControlTree::native_ordinary_tooltip_fields_id_without_callbacks(
 const std::shared_ptr<Core>& actual,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete){
 const auto id=native_ordinary_geometry_receiver_id_without_callbacks(actual,receiver,concrete);
 (void)native_ordinary_flag_word_without_callbacks(actual,receiver,concrete);
 const auto& state=*Core::node(actual->nodes,id).layout_baseline;
 if(state.kind_==detail::NotificationLayoutBaseline::Kind::label){
  const auto label=state.standalone_label_->impl_;label->first();
  const auto* type=std::get_if<std::uint32_t>(&label->fields[0x48u/4u]);
  require(type&&*type==state.base_.tooltip_type_48,"Tooltip actual Label48/Base alias differs");
  if(state.base_.tooltip_54){
   const auto* text=std::get_if<detail::NotificationLayoutStandaloneLabelState::Impl::TooltipBorrow>(&label->fields[0x54u/4u]);
   require(text&&text->object==state.base_.tooltip_54,"Tooltip actual Label54/Base owner differs");
  }else require(std::holds_alternative<detail::NotificationLayoutStandaloneLabelState::Impl::Null>(label->fields[0x54u/4u]),
   "Tooltip real Label54 NULL store differs");
 }
 return id;
}

//524CC0 compares the actual current tooltip-manager5A4. The runtime borrower
//is the SAME returned ordinary receiver/CData; a parser scope is not invented.
void BnwWorldViewControlTree::refresh_native_ordinary_tooltip524cc0_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,const std::function<void()>& rooted){
 using Gui=detail::NotificationContextGuiPort;
 const auto actual=core_;actual->layout_origin(true);
 const auto id=native_ordinary_tooltip_fields_id_without_callbacks(actual,receiver,concrete);
 const auto port=actual->context_gui_port;
 require(port&&port->impl_,"Tooltip524CC0 actual GUI02DC1B1C+28 owner absent");
 const auto gui=port->impl_;const auto manager=gui->tooltip_manager;
 require(bool(manager),"Tooltip524CC0 physical manager owner absent");
 gui->first(operation);manager->life->first();require(!manager->busy,"Tooltip524CC0 reentered");
 const auto occurrence=actual->nodes.at(id-1).layout_receiver_occurrence;
 require(bool(occurrence),"Tooltip524CC0 real receiver occurrence absent");
 const auto source_serial=operation.operation_serial();
 const auto topology=actual->topology_revision,effects=actual->effect_revision;
 const auto manager_sequence=manager->sequence,hover_store=manager->hover_store_sequence;
 require(manager->fields_5a0_5bc[1].has_value()&&hover_store,
  "Tooltip524CC0 current5A4 has no genuine initialization/input store");
 const auto hover=*manager->fields_5a0_5bc[1];
 const auto pure=[&]{
  if(rooted)rooted();actual->layout_origin(true);gui->first(operation);manager->life->first();
  operation.require_origin_without_callbacks();
  require(&actual->source->current_operation_loan()==&operation&&operation.operation_serial()==source_serial&&
   actual->context_gui_port==port&&port->impl_==gui&&gui->tooltip_manager==manager&&manager->gui.lock()==gui&&
   actual->topology_revision==topology&&actual->effect_revision==effects&&
   actual->nodes.at(id-1).layout_receiver_occurrence==occurrence&&occurrence->id==id&&
   occurrence->constructor_serial&&occurrence->instance&&manager->constructor_source_serial&&
   manager->history.size()>=9&&manager->registration_sequence&&gui->tooltip_registrations.size()==1&&
   manager->sequence==manager_sequence&&manager->hover_store_sequence==hover_store&&
   manager->fields_5a0_5bc[1].has_value()&&manager->fields_5a0_5bc[1]->index()==hover.index(),
   "Tooltip524CC0 SAME current Source/GUI5A4/physical receiver changed");
  const auto& registration=gui->tooltip_registrations.front();
  require(registration.target.lock()==manager&&registration.store_sequence==manager->registration_sequence&&
   bool(registration.callable),"Tooltip524CC0 actual retained728020 registration changed");
  require(native_ordinary_tooltip_fields_id_without_callbacks(actual,receiver,concrete)==id,
   "Tooltip524CC0 SAME ordinary CData receiver changed");
  if(rooted)rooted();actual->layout_origin(true);
 };
 try{
  pure();manager->busy=true;
  {struct End{std::shared_ptr<Gui::Impl::TooltipKnownFields> manager;~End(){manager->busy=false;}}end{manager};
   operation.require_current();pure();
   if(std::holds_alternative<Gui::Impl::TooltipKnownFields::ControlIdentity>(hover)){
    const auto& key=std::get<Gui::Impl::TooltipKnownFields::ControlIdentity>(hover);
    const auto hover_core=key.core.lock();const auto hover_occurrence=key.allocation.lock();
    require(hover_core&&hover_occurrence,"Tooltip524CC0 nonNULL input5A4 receiver retired");
    //No expired/unknown input identity can become constructorNULL/nonmatch.
    if(hover_core==actual&&hover_occurrence==occurrence&&key.id==id&&
     key.instance==occurrence->instance&&key.constructor_serial==occurrence->constructor_serial)
     throw runtime::ScriptError("Tooltip524CC0 actual hovered match requires genuine6C4A50 type62BE40/text4875E0 and callback4C/50/fresh const-v80/8F7AE0 effects");
    throw runtime::ScriptError("Tooltip524CC0 nonNULL5A4 requires real current native input writer/receiver lifecycle");
   }
   //Genuine current constructorNULL is unequal to this actual nonNULL THIS.
   operation.require_current();pure();
  }
  pure();require(!manager->busy,"Tooltip524CC0 actual cleanup failed");
 }catch(...){manager->life->failed.store(true);gui->failed.store(true);throw;}
}

void BnwWorldViewControlTree::write_native_ordinary_tooltip_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,NativeOrdinaryTooltipWrite member,
 const char* input,const std::function<void()>& rooted){
 using Base=detail::NotificationLayoutBaseline;using Parent=detail::NotificationLayoutParentReceiver;
 using Label=detail::NotificationLayoutStandaloneLabelState::Impl;
 const auto actual=core_;
 const auto pure=[&](bool busy){if(rooted)rooted();actual->layout_origin(busy);
  const auto& current=actual->source->current_operation_loan();actual->layout_origin(busy);
  require(&current==&operation,"Tooltip SAME current Source operation replaced");current.require_origin_without_callbacks();
  (void)native_ordinary_tooltip_fields_id_without_callbacks(actual,receiver,concrete);
  if(rooted)rooted();actual->layout_origin(busy);};
 try{
  pure(false);actual->layout_current_owned();pure(false);operation.require_current();pure(false);
  const auto id=receiver->impl_->id;auto& state=*actual->nodes.at(id-1).layout_baseline;
  actual->busy=true;struct Exit{bool& value;~Exit(){value=false;}}exit{actual->busy};pure(true);
  const auto flag=[&](NativeOrdinaryFlagWrite site,bool set){
   const auto before=native_ordinary_flag_word_without_callbacks(actual,receiver,concrete);
   commit_native_ordinary_flag_word_owned(actual,receiver,concrete,site,set?(before|0x4000u):(before&~0x4000u));pure(true);
  };
  const auto store_scalar=[&](std::uint32_t offset,std::uint32_t value,
   const std::shared_ptr<const std::string>& text=std::shared_ptr<const std::string>{}){
   require(offset==0x48u||offset==0x54u,"Tooltip physical store requires actual48/54 site");
   require(offset==0x54u?value==0u:!text,"Tooltip text/scalar store operands differ");
   require(offset!=0x54u||!text||!state.base_.tooltip_54,
    "Tooltip positive54 publication requires preceding actual old-owner withdrawal");
   require(state.sequence_<UINT64_MAX&&state.history_.size()<state.history_.max_size()&&
    actual->effect_revision<UINT64_MAX&&actual->layout_sequence<UINT64_MAX,"Tooltip physical store journal exhausted");
   state.history_.reserve(state.history_.size()+1);std::shared_ptr<Label> label;
   if(state.kind_==Base::Kind::label){label=state.standalone_label_->impl_;label->first();
    require(label->sequence<UINT64_MAX&&label->history.size()<label->history.max_size(),"Tooltip Label scalar journal exhausted");
    label->history.reserve(label->history.size()+1);label->first();}
   actual->initial_lua_preflight_store_owned();pure(true);
   if(label){label->history.push_back({++label->sequence,offset});
    if(offset==0x48u)std::get<std::uint32_t>(label->fields[0x48u/4u])=value;
    else if(text)label->fields[0x54u/4u]=Label::TooltipBorrow{text};
    else label->fields[0x54u/4u]=Label::Null{};}
   state.record_owned(Base::Mutation::receiver_prefix);
   if(offset==0x48u)state.base_.tooltip_type_48=value;
   else state.base_.tooltip_54=text; //SAME real x64 owner in Base54/Label54, including NULL.
   ++actual->effect_revision;++actual->layout_sequence;
   actual->initial_lua_record_store_owned(Core::InitialLuaWriter::receiver_store,id,
    static_cast<std::uint64_t>(offset==0x48u?Parent::Store::Kind::ordinary_tooltip_type:Parent::Store::Kind::ordinary_tooltip_text));
   pure(true);
  };
  if(member==NativeOrdinaryTooltipWrite::callback_6dbf40){
   flag(NativeOrdinaryFlagWrite::tooltip_callback_6dbf40,true); //6DBF45 precedes assignment.
   throw runtime::ScriptError("SetToolTipCallback6DBF40 actual OR4000 executed; required SAME receiver4C/50 FLua function-ref assignment8F8A90/old-ref8F8820 and retained VM/Control destruction lifetime");
  }
  if(member==NativeOrdinaryTooltipWrite::localization_lookup_69ae5a){
   //69AE5A calls imported Lookup BEFORE variadic argument traversal. Missing
   //String builder is not replaced by key-only Locale or reordered arg errors.
   throw runtime::ScriptError("LocalizeAndSetToolTip requires actual Localization::Lookup/String84 owner, PushArgument(int,double,const char*)/toUTF8/destructor from IAT9C9160/9168/9170/9164/9158/9154 before receiver5F3B90");
  }
  if(member==NativeOrdinaryTooltipWrite::type_47cb60_4e3d80){
   flag(NativeOrdinaryFlagWrite::tooltip_type_47cb60_4e3da9,input!=nullptr);
   //NonNULL empty string hashes toFFFFFFFF; NULL alone clears type48.
   const auto hash=input?content::saved_info_type_hash(std::string_view(input,std::strlen(input))):0u;
   pure(true);store_scalar(0x48u,hash);
   refresh_native_ordinary_tooltip524cc0_owned(operation,receiver,concrete,rooted);pure(true);
  }else if(member==NativeOrdinaryTooltipWrite::clear_73fe50_69ade0||member==NativeOrdinaryTooltipWrite::text_5f3b90){
   //Both wrappers and5F3B90 withdraw the OLD actual54 BEFORE flag writes.
   //The supported x64 port really owns this shared string. Withdraw BOTH
   //Base54/Label54 aliases in one journaled physical store; RAII ends THIS
   //control's ownership. This is equivalent visible lifetime, not an Original typed
   //v4/free-list/counter execution receipt. Genuine independent read copies
   //keep their own immutable bytes and confer no current54 authority.
   if(state.base_.tooltip_54)store_scalar(0x54u,0u);
   if(member==NativeOrdinaryTooltipWrite::clear_73fe50_69ade0){
    flag(NativeOrdinaryFlagWrite::tooltip_clear_73fe50_69ade0,false); //Genuine oldNULL needs no54 withdrawal.
   }else if(!input){
    flag(NativeOrdinaryFlagWrite::tooltip_text_5f3b90,false); //5F3BB1/BA NULL input flag/return.
   }else{
    flag(NativeOrdinaryFlagWrite::tooltip_text_5f3b90,true);
    std::size_t length=0;while(length<0x4000u&&input[length])++length;pure(true);
    const auto requested=length+1u;
    std::shared_ptr<const std::string> text;
    if(requested<=0x4000u){
     //Own real x64 allocation/copy occurs AFTER OR4000. std::string owns the
     //complete bounded byte extent and its terminating NUL; nonNULL empty
     //input therefore creates a live empty object, distinct from NULL54.
     //The Host rooted Lua TString is independent of the withdrawn54 buffer.
     try{text=std::make_shared<const std::string>(input,length);}
     catch(const std::bad_alloc&){text.reset();} //ONLY this real text allocation.
     pure(true);
     require(!text||(text->size()==length&&text->data()[length]=='\0'),
      "Tooltip real bounded54 allocation/copy extent differs");
    }
    //5F81A0 requested0x4001 is genuine selectorNULL before allocation.
    //Real own allocation failure also publishesNULL and still refreshes.
    //Journal/preflight/source errors retain their strict exception path;
    //allocator diagnostics and Original OOM availability are not claimed.
    store_scalar(0x54u,0u,text);
    refresh_native_ordinary_tooltip524cc0_owned(operation,receiver,concrete,rooted);pure(true);
   }
  }else throw runtime::ScriptError("Tooltip requires qualified actual member identity");
  operation.require_current();pure(true);
 }catch(...){actual->failed.store(true);throw;}
}

void BnwWorldViewControlTree::read_native_ordinary_tooltip54_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const void>& concrete,std::optional<std::string>& output)const{
 const auto actual=core_;
 const auto pure=[&]{actual->layout_origin();const auto& current=actual->source->current_operation_loan();actual->layout_origin();
  require(&current==&operation,"GetToolTipString SAME current Source operation replaced");current.require_origin_without_callbacks();
  (void)native_ordinary_tooltip_fields_id_without_callbacks(actual,receiver,concrete);actual->layout_origin();};
 try{
  pure();actual->layout_current_owned();pure();operation.require_current();pure();
  const auto object=Core::node(actual->nodes,receiver->impl_->id).layout_baseline->base_.tooltip_54;
  //Copy INTO Host-owned call custody, never an automatic std::string across
  //Lua allocation. lua_pushstring stops at the first NUL just as native code.
  if(object)output=*object;else output.reset();
  pure();require(Core::node(actual->nodes,receiver->impl_->id).layout_baseline->base_.tooltip_54==object,
   "GetToolTipString actual54 object changed during physical read");
 }catch(...){actual->failed.store(true);throw;}
}
