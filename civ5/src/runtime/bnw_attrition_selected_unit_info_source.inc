namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::require_attrition_selected_unit_info_parent_without_callbacks(const BnwRetainedTurnCursor&cursor,
 const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease*lease,bool idle)const{
 stock_thread_->check();const auto b=bootstrap_;production_need(b&&b->operation_control,"UnitInfo genuine Source absent");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,b->state->ui_source_revision);
 const auto previous=cursor.attrition_selected_unit_info_previous_serial_without_callbacks();
 production_need(previous&&previous<UINT64_MAX&&b->operation_control->serial==previous+(idle?0U:1U),"UnitInfo exact next Source serial required");
 cursor.require_attrition_selected_unit_info_parent_return_without_callbacks(core,*b->selected,*b->state,*this,b->operation_control.get(),b->generation,previous);
}
std::shared_ptr<const BnwAttritionSelectedUnitInfoDirtyLoan> BnwSelectedStockLuaSourceOwner::issue_attrition_selected_unit_info_owned(
 const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease&lease,
 const BnwNativeAttritionPrefix&prefix,const std::shared_ptr<ui::BnwClientSelectionControl>&selection)const{
 require_attrition_selected_unit_info_parent_without_callbacks(cursor,core,&lease,false);const auto b=bootstrap_;
 return prefix.prepare_selected_unit_info_owned(core,*b->selected,*b->state,*this,b->operation_control.get(),b->generation,lease.operation_serial(),cursor,selection);
}
void BnwSelectedStockLuaSourceOwner::require_attrition_selected_unit_info_operation_without_callbacks(const BnwRetainedTurnCursor&cursor,
 const BnwStockLuaAttemptLease*lease,std::uint64_t serial,std::uint64_t revision,bool adopted)const{
 stock_thread_->check();const auto b=bootstrap_;production_need(b&&b->operation_control,"UnitInfo Source operation missing");
 // During owned setter admission the canonical Source operation is active;
 // afterwards the same pure seal can validate its ordinary idle return.
 if(b->running){const BnwStockLuaAttemptLease active(b->operation_control);
  require_player_end_turn_source_without_callbacks(cursor,lease?lease:&active,false,revision);
 }else require_player_end_turn_source_without_callbacks(cursor,lease,true,revision);
 production_need(b->operation_control->serial==serial&&b->operation_world_revision==revision&&b->state->ui_source_revision==revision,
  "UnitInfo exact source operation/current world revision differs");
 cursor.require_attrition_selected_unit_info_operation_without_callbacks(serial,revision,adopted);
}
void BnwSelectedStockLuaSourceOwner::adopt_attrition_selected_unit_info_without_callbacks(std::uint64_t revision)const noexcept{
 if(!stock_thread_||!bootstrap_||!bootstrap_->operation_control||bootstrap_->operation_world_revision!=revision||bootstrap_->state->ui_source_revision!=revision)std::terminate();
 // The conditional second dirty setter changes UI state only. Source world revision remains this exact value.
 bootstrap_->operation_world_revision=revision;
}
void BnwSelectedStockLuaSourceOwner::require_attrition_selected_unit_info_source_without_callbacks(const BnwRetainedTurnCursor&cursor,
 const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease*lease,bool idle)const{
 stock_thread_->check();const auto b=bootstrap_;production_need(b&&b->operation_control,"UnitInfo Source post absent");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,b->state->ui_source_revision);
 cursor.require_attrition_selected_unit_info_return_without_callbacks(core,*this);
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_player_attrition_selected_unit_info_prefix(std::shared_ptr<BnwRetainedTurnCursor>supplied,
 const std::weak_ptr<const BnwNativeUnitAllocatorCore>&weak){
 stock_thread_->check();const auto pin=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(weak);
 const auto self=retain_action_factory_owner();const auto b=self->bootstrap_;production_need(b&&b->vm&&supplied,"UnitInfo Source/Cursor absent");
 const auto cursor=supplied->retain_engine_factory_owner();production_need(cursor==supplied,"UnitInfo supplied foreign Cursor");
 cursor->require_attrition_selected_unit_info_entry_without_callbacks(pin->current(),*self);b->check();cursor->require_engine_host(*b->selected);
 try{const auto result=b->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
  b->check_active();const BnwStockLuaAttemptLease lease(b->operation_control);const auto serial=lease.operation_serial();
  cursor->execute_attrition_selected_unit_info_with_actual_stock(pin->current(),*self,lease);
  b->check_active();lease.require_operation(serial);cursor->require_engine_host(*b->selected);
  require_attrition_selected_unit_info_source_without_callbacks(*cursor,pin->current(),&lease,false);return cursor->view();
 });require_attrition_selected_unit_info_source_without_callbacks(*cursor,pin->current(),nullptr,true);return result;
 }catch(...){if(!b->retained_error)b->retained_error=std::current_exception();b->quarantine();cursor->fail_attrition_selected_unit_info_without_callbacks();throw;}
}
}
