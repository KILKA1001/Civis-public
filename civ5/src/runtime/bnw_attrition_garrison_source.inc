namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::require_attrition_garrison_parent_source_without_callbacks(
 const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease*lease,bool idle)const{
 stock_thread_->check();const auto b=bootstrap_;production_need(b&&b->operation_control,"Garrison actual Source absent");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,b->state->ui_source_revision);
 const auto previous=cursor.attrition_garrison_previous_serial_without_callbacks();
 production_need(previous&&previous<UINT64_MAX&&b->operation_control->serial==previous+(idle?0U:1U),
                 "Garrison requires exact next actual damage Source operation");
 cursor.require_attrition_damage_return_without_callbacks(core,*b->selected,*b->state,*this,
     b->operation_control.get(),b->generation,previous,true);
 // Actual Cursor entry checks the garrison frontier; Core's closed issuer
 // independently requires physically changed damage. No callbackful view().
}
void BnwSelectedStockLuaSourceOwner::issue_attrition_garrison_owned(const BnwRetainedTurnCursor&cursor,
 const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease&lease,const BnwNativeAttritionPrefix&prefix)const{
 require_attrition_garrison_parent_source_without_callbacks(cursor,core,&lease,false);const auto b=bootstrap_;
 prefix.prepare_garrison_owned(core,*b->selected,*b->state,*this,b->operation_control.get(),b->generation,lease.operation_serial());
}
void BnwSelectedStockLuaSourceOwner::seal_attrition_garrison_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const BnwNativeAttritionPrefix&prefix,const BnwStockLuaAttemptLease&lease,bool executed)const{
 stock_thread_->check();const auto b=bootstrap_;production_need(b&&b->operation_control,"Garrison Source seal absent");
 prefix.require_garrison_without_callbacks(core,*b->selected,*b->state,*this,b->operation_control.get(),b->generation,lease.operation_serial(),executed);
}
void BnwSelectedStockLuaSourceOwner::adopt_actual_attrition_garrison_return(const BnwRetainedTurnCursor&cursor,
 const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease&lease)const{
 stock_thread_->check();const auto b=bootstrap_;production_need(b&&b->operation_control,"Garrison Source adoption absent");
 require_player_end_turn_source_without_callbacks(cursor,&lease,false,b->operation_world_revision);
 production_need(b->state->ui_source_revision==b->operation_world_revision,"Garrison query unexpectedly changed world");
 cursor.require_attrition_garrison_return_without_callbacks(core,*b->selected,*b->state,*this,
     b->operation_control.get(),b->generation,lease.operation_serial(),false);
}
void BnwSelectedStockLuaSourceOwner::require_attrition_garrison_source_without_callbacks(const BnwRetainedTurnCursor&cursor,
 const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease*lease,bool idle)const{
 stock_thread_->check();const auto b=bootstrap_;production_need(b&&b->operation_control,"Garrison Source post absent");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,b->state->ui_source_revision);
 cursor.require_attrition_garrison_return_without_callbacks(core,*b->selected,*b->state,*this,
     b->operation_control.get(),b->generation,b->operation_control->serial,true);
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_player_attrition_garrison_prefix(
 std::shared_ptr<BnwRetainedTurnCursor>supplied,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&weak){
 stock_thread_->check();const auto pin=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(weak);
 const auto self=retain_action_factory_owner();const auto b=self->bootstrap_;
 production_need(b&&b->vm&&supplied,"Garrison genuine Source/Cursor absent");const auto cursor=supplied->retain_engine_factory_owner();
 production_need(cursor==supplied,"Garrison foreign Cursor");cursor->require_attrition_garrison_entry_without_callbacks(pin->current(),*self);
 b->check();cursor->require_engine_host(*b->selected);
 try{const auto result=b->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
   b->check_active();const BnwStockLuaAttemptLease lease(b->operation_control);const auto serial=lease.operation_serial();
   cursor->execute_attrition_garrison_with_actual_stock(pin->current(),*self,lease);
   b->check_active();lease.require_operation(serial);cursor->require_engine_host(*b->selected);
   require_attrition_garrison_source_without_callbacks(*cursor,pin->current(),&lease,false);return cursor->view();
  });require_attrition_garrison_source_without_callbacks(*cursor,pin->current(),nullptr,true);return result;
 }catch(...){if(!b->retained_error)b->retained_error=std::current_exception();b->quarantine();
  cursor->fail_attrition_garrison_without_callbacks();throw;}
}
}
