namespace civ5::runtime {
bool BnwSelectedStockLuaBootstrap::try_dispatch_unitflag_health_head_owned(
 const std::weak_ptr<graphics::BnwUnitDamageCommandOwner>&supplied_queue,
 const BnwOwnedGameplayEventPumpLoan&pump,const BnwStockLuaAttemptLease&lease,
 const BnwSelectedStockLuaSourceOwner&source){
 need(impl_&&impl_->thread==std::this_thread::get_id(),"Health pending query foreign Bootstrap creator");
 try{
  need(impl_->operation&&!impl_->diagnostic_world&&
   !impl_->operation->control_.owner_before(lease.control_)&&
   !lease.control_.owner_before(impl_->operation->control_),
   "Health pending query differs from actual Stock operation");
  const auto identity=lease.pin_active_scope_without_callbacks();
  lease.require_scope_identity_without_callbacks(identity);
  pump.require_source_owner_without_callbacks(source);pump.require_for_source(lease);
  lease.require_scope_identity_without_callbacks(identity);impl_->healthy();
  bool pending=false;
  {
   const auto queue=supplied_queue.lock();
   need(static_cast<bool>(queue),"Health pending query issued queue expired");
   // Sole Parent native Queue body proves real emptiness, creator and !busy.
   pending=queue->has_pending_health_owned();
   lease.require_scope_identity_without_callbacks(identity);
  }
  // The temporary query pin is gone before the executable head borrower.
  lease.require_scope_identity_without_callbacks(identity);
  if(!pending)return false;
  dispatch_unitflag_health_head_owned(supplied_queue,pump,lease,source);
  lease.require_scope_identity_without_callbacks(identity);
  pump.require_source_owner_without_callbacks(source);pump.require_for_source(lease);
  lease.require_scope_identity_without_callbacks(identity);impl_->healthy();
  return true;
 }catch(...){impl_->fail();throw;}
}
}
