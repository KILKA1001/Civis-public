// Included after existing Mountain1 Source methods; common TU owned by Root.
namespace civ5::runtime {
void BnwSelectedStockLuaSourceOwner::require_attrition_damage_parent_source_without_callbacks(
 const BnwRetainedTurnCursor&cursor,const BnwNativeUnitAllocatorCore&core,
 const BnwStockLuaAttemptLease*lease,bool idle)const{
 stock_thread_->check();const auto bound=bootstrap_;
 production_need(bound&&bound->operation_control,"Attrition damage real Source absent");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,bound->state->ui_source_revision);
 const auto issued=cursor.attrition_damage_previous_serial_without_callbacks();
 production_need(issued&&issued<UINT64_MAX&&bound->operation_control->serial==issued+(idle?0U:1U),
                 "Attrition damage requires exact next authentic Source operation");
 cursor.require_attrition_damage_parent_without_callbacks(core,*bound->selected,*bound->state,*this,
     bound->operation_control.get(),bound->generation,issued);
}
void BnwSelectedStockLuaSourceOwner::issue_attrition_damage_owned(const BnwRetainedTurnCursor&cursor,
 const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease&lease,
 const BnwNativeAttritionPrefix&prefix,const std::shared_ptr<const ui::LiveUiHost>&actual_ui,
 const std::weak_ptr<ui::BnwEngineUiTurnOwner>&engine)const{
 require_attrition_damage_parent_source_without_callbacks(cursor,core,&lease,false);const auto bound=bootstrap_;
 // Reuse genuine existing issuer, verifying SAME private receiver/control.
 // Release the extra retained reference BEFORE native Core/provider pinning.
 {const auto actual=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(actual_ui,bound->selected);
  production_need(actual&&!engine.owner_before(actual)&&!actual.owner_before(engine),
                  "Attrition damage different actual Engine localization issuer");}
 require_attrition_damage_parent_source_without_callbacks(cursor,core,&lease,false);
 prefix.prepare_damage_owned(core,*bound->selected,*bound->state,*this,bound->operation_control.get(),
     bound->generation,lease.operation_serial(),engine);
}
void BnwSelectedStockLuaSourceOwner::seal_attrition_damage_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const BnwNativeAttritionPrefix&prefix,const BnwStockLuaAttemptLease&lease,bool executed)const{
 stock_thread_->check();const auto bound=bootstrap_;
 production_need(bound&&bound->operation_control,"Attrition damage Source seal absent");
 prefix.require_damage_without_callbacks(core,*bound->selected,*bound->state,*this,
     bound->operation_control.get(),bound->generation,lease.operation_serial(),executed);
}
void BnwSelectedStockLuaSourceOwner::adopt_actual_attrition_damage_return(const BnwRetainedTurnCursor&cursor,
 const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease&lease)const{
 stock_thread_->check();const auto bound=bootstrap_;
 production_need(bound&&bound->operation_control,"Attrition damage actual adoption absent");
 // This primitive checks the closed Source/control/VM/world operation without
 // calling old physical row validators or callbackful currentness after HP.
 require_player_end_turn_source_without_callbacks(cursor,&lease,false,bound->operation_world_revision);
 cursor.require_attrition_damage_return_without_callbacks(core,*bound->selected,*bound->state,*this,
     bound->operation_control.get(),bound->generation,lease.operation_serial(),false);
 bound->operation_world_revision=bound->state->ui_source_revision;
}
void BnwSelectedStockLuaSourceOwner::require_attrition_damage_source_without_callbacks(const BnwRetainedTurnCursor&cursor,
 const BnwNativeUnitAllocatorCore&core,const BnwStockLuaAttemptLease*lease,bool idle)const{
 stock_thread_->check();const auto bound=bootstrap_;
 production_need(bound&&bound->operation_control,"Attrition damage actual Source post absent");
 require_player_end_turn_source_without_callbacks(cursor,lease,idle,bound->state->ui_source_revision);
 cursor.require_attrition_damage_return_without_callbacks(core,*bound->selected,*bound->state,*this,
     bound->operation_control.get(),bound->generation,bound->operation_control->serial,true);
}
BnwRetainedTurnView BnwSelectedStockLuaSourceOwner::run_player_attrition_damage_prefix(
 std::shared_ptr<BnwRetainedTurnCursor>supplied,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&weak_core,
 const std::shared_ptr<const ui::LiveUiHost>&actual_ui){
 stock_thread_->check();const auto pin=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(weak_core);
 const auto self=retain_action_factory_owner();const auto bound=self->bootstrap_;
 production_need(bound&&bound->vm&&supplied&&actual_ui,"Attrition damage genuine Source/Cursor/UI absent");
 const auto cursor=supplied->retain_engine_factory_owner();
 production_need(cursor==supplied,"Attrition damage foreign Cursor control");
 cursor->require_attrition_damage_entry_without_callbacks(pin->current(),*self);
 bound->check();cursor->require_engine_host(*bound->selected);
 try{const auto result=bound->run_operation(BnwStockLuaOperationKind::engine_ui_turn,[&]{
   bound->check_active();const BnwStockLuaAttemptLease lease(bound->operation_control);const auto serial=lease.operation_serial();
   cursor->execute_attrition_damage_with_actual_stock(pin->current(),*self,lease,actual_ui);
   bound->check_active();lease.require_operation(serial);cursor->require_engine_host(*bound->selected);
   require_attrition_damage_source_without_callbacks(*cursor,pin->current(),&lease,false);return cursor->view();
  });require_attrition_damage_source_without_callbacks(*cursor,pin->current(),nullptr,true);return result;
 }catch(...){if(!bound->retained_error)bound->retained_error=std::current_exception();
  bound->quarantine();cursor->fail_attrition_damage_without_callbacks();throw;}
}
}
