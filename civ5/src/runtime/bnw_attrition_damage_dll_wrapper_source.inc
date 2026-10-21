namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::require_attrition_dll_wrapper_parent_without_callbacks(const BnwRetainedTurnCursor&cursor,
 const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease*lease,bool idle)const{
 stock_thread_->check();const auto b=bootstrap_;production_need(b&&b->operation_control,"Damage wrapper actual Source absent");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,b->state->ui_source_revision);
 const auto previous=cursor.attrition_dll_wrapper_previous_serial_without_callbacks();production_need(previous&&previous<UINT64_MAX&&
  b->operation_control->serial==previous+(idle?0U:1U),"Damage wrapper exact next source operation absent");
 cursor.require_attrition_dll_wrapper_parent_return_without_callbacks(core,*b->selected,*b->state,*this,b->operation_control.get(),b->generation,previous);
}
std::shared_ptr<const BnwAttritionDamageDllWrapperPrefix> BnwSelectedStockLuaSourceOwner::issue_attrition_dll_wrapper_owned(const BnwRetainedTurnCursor&cursor,
 const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease&lease,const BnwNativeAttritionPrefix&prefix)const{
 require_attrition_dll_wrapper_parent_without_callbacks(cursor,core,&lease,false);const auto b=bootstrap_;
 return prefix.prepare_damage_wrapper_owned(core,*b->selected,*b->state,*this,b->operation_control.get(),b->generation,lease.operation_serial(),cursor);
}
void BnwSelectedStockLuaSourceOwner::require_attrition_dll_wrapper_operation_without_callbacks(const BnwRetainedTurnCursor&cursor,
 const BnwStockLuaAttemptLease*lease,std::uint64_t serial,std::uint64_t revision,bool adopted)const{
 stock_thread_->check();const auto b=bootstrap_;production_need(b&&b->operation_control,"Damage command actual Source/control absent");
 if(b->running){const BnwStockLuaAttemptLease active(b->operation_control);require_player_end_turn_source_without_callbacks(cursor,lease?lease:&active,false,revision);}
 else {production_need(adopted&&!lease,"Damage command unreturned operation cannot become idle");require_player_end_turn_source_without_callbacks(cursor,nullptr,true,revision);}
 production_need(b->operation_control->serial==serial&&b->operation_world_revision==revision&&b->state->ui_source_revision==revision,
  "Damage wrapper exact Source/control/world operation differs");cursor.require_attrition_dll_wrapper_operation_without_callbacks(serial,revision,adopted);
}
void BnwSelectedStockLuaSourceOwner::adopt_attrition_dll_wrapper_without_callbacks(std::uint64_t revision)const noexcept{
 if(!stock_thread_||!bootstrap_||!bootstrap_->operation_control||bootstrap_->operation_world_revision!=revision||bootstrap_->state->ui_source_revision!=revision)std::terminate();
 bootstrap_->operation_world_revision=revision; // Utility command append changes no logical world fields.
}
void BnwSelectedStockLuaSourceOwner::require_attrition_dll_wrapper_source_without_callbacks(const BnwRetainedTurnCursor&cursor,
 const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease*lease,bool idle)const{
 stock_thread_->check();const auto b=bootstrap_;production_need(b&&b->operation_control,"Damage command Source return owner absent");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,b->state->ui_source_revision);cursor.require_attrition_dll_wrapper_return_without_callbacks(core,*this);
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_player_attrition_dll_wrapper_prefix(std::shared_ptr<BnwRetainedTurnCursor>supplied,
 const std::weak_ptr<const BnwNativeUnitAllocatorCore>&weak,const std::shared_ptr<ui::BnwClientSceneFrame>&scene){
 stock_thread_->check();const auto pin=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(weak);const auto self=retain_action_factory_owner();
 const auto b=self->bootstrap_;production_need(b&&b->vm&&supplied,"Damage command genuine Source/Cursor absent");const auto cursor=supplied->retain_engine_factory_owner();
 production_need(cursor==supplied,"Damage command foreign Cursor");cursor->require_attrition_dll_wrapper_entry_without_callbacks(pin->current(),*self);
 try{
  const auto result=b->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]()->BnwRetainedTurnView{
   b->check_active();const BnwStockLuaAttemptLease lease(b->operation_control);const auto serial=lease.operation_serial();
   const auto gui=b->vm->retain_research_choice_gui_storage_without_callbacks();cursor->require_attrition_city_info_facade_without_callbacks();
   require_attrition_dll_wrapper_parent_without_callbacks(*cursor,pin->current(),&lease,false);
   cursor->execute_attrition_dll_wrapper_until_popup(pin->current(),*self,lease,scene,gui);
   // Actual append/no-op return was already adopted before callbackful POST.
   b->check_active();cursor->require_attrition_city_info_facade_without_callbacks();lease.require_operation(serial);cursor->require_attrition_city_info_facade_without_callbacks();
   cursor->require_engine_host(*b->selected);cursor->require_attrition_city_info_facade_without_callbacks();
   require_attrition_dll_wrapper_source_without_callbacks(*cursor,pin->current(),&lease,false);return cursor->view();
  });require_attrition_dll_wrapper_source_without_callbacks(*cursor,pin->current(),nullptr,true);return result;
 }catch(...){if(!b->retained_error)b->retained_error=std::current_exception();b->quarantine();cursor->fail_attrition_dll_wrapper_without_callbacks();throw;}
}
}
