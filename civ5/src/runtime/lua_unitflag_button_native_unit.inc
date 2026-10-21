// Existing genuine VM/native scope and provider validate button operands.
void LuaScriptHost::require_unitflag_button_unit_owned(
 const BnwWorldViewSourceOperationLoan& loan,std::int32_t actor,std::int32_t full_id)const {
 const auto p=impl_;
 if(p->ui_thread!=std::this_thread::get_id())throw ScriptError("UnitFlag native button unit foreign creator");
 const auto first=[&]{
  p->worldview_require_callback_origin();
  if(p->worldview_operation!=&loan||p->worldview_host_owner!=this)
   throw ScriptError("UnitFlag button lacks SAME actual VM/source lender");
 };
 first();const auto serial=loan.operation_serial();first();
 try{
  if(actor<0||actor>=64||full_id<0||!p->gameplay.units)throw ScriptError("UnitFlag button actual unit provider absent");
  loan.require_same_vm(*this);first();
  p->gameplay.units->require_current();first();loan.require_operation(serial);first();
  const auto unit=p->gameplay.units->find(actor,full_id);first();loan.require_operation(serial);first();
  if(!unit)throw ScriptError("UnitFlag button full-ID unit absent");
  unit->require_current();first();loan.require_operation(serial);first();
  const auto owner=unit->read("GetOwner");first();loan.require_operation(serial);first();
  const auto id=unit->read("GetID");first();loan.require_operation(serial);first();
  const auto* a=std::get_if<std::int64_t>(&owner);const auto* b=std::get_if<std::int64_t>(&id);
  if(!a||!b||*a!=actor||*b!=full_id)throw ScriptError("UnitFlag button low-slot alias cannot replace complete native identity");
  unit->require_current();first();loan.require_operation(serial);first();
 }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
