// Actual GUI / Source scopes unwind before the opaque original-event completion.
// One issued pending queue head only; ordinary empty-pump admission is separate.
namespace civ5::runtime {
void BnwSelectedStockLuaBootstrap::dispatch_unitflag_health_head_owned(
 const std::weak_ptr<graphics::BnwUnitDamageCommandOwner>& supplied_queue,
 const BnwOwnedGameplayEventPumpLoan&pump,const BnwStockLuaAttemptLease&lease,
 const BnwSelectedStockLuaSourceOwner&source){
 // Immutable creator FIRST before operation/GUI/VM access.
 need(impl_&&impl_->thread==std::this_thread::get_id(),"Health Bootstrap foreign creator");
 try{
  impl_->healthy();
  need(impl_->operation&&!impl_->diagnostic_world&&
   !impl_->operation->control_.owner_before(lease.control_)&&
   !lease.control_.owner_before(impl_->operation->control_),
   "Health Bootstrap actual Stock operation differs");
  const auto identity=lease.pin_active_scope_without_callbacks();
  lease.require_scope_identity_without_callbacks(identity);
  pump.require_source_owner_without_callbacks(source);pump.require_for_source(lease);
  lease.require_scope_identity_without_callbacks(identity);impl_->healthy();
  need(impl_->host&&impl_->worldview_sources&&impl_->unitflag_native&&impl_->gui_screen,
   "Health Bootstrap actual VM/metadata/UnitFlag/GUI member missing");
  const auto queue=supplied_queue.lock();
  need(static_cast<bool>(queue),"Health Bootstrap issued queue expired or absent");
  // Queue itself verifies the canonical Scene/Core/Host/world/control binding.
  queue->with_health_fifo_owned(pump,lease,source,*this,
   [&](const BnwUnitFlagHealthFifoLoan&fifo)->BnwUnitFlagHealthFifoLoan::DeliveryReturn{
    fifo.require_origin_without_callbacks();
    lease.require_scope_identity_without_callbacks(identity);
    const auto gui=impl_->gui_screen;
    std::shared_ptr<LuaScriptHost::UnitDamageEventReturn> returned;
    impl_->host->with_unitflag_health_instance_owned(fifo,lease,*impl_->unitflag_native,
     [&](const BnwUnitFlagHealthInstanceLoan&membership){
      membership.require_origin_without_callbacks();fifo.require_origin_without_callbacks();
      gui->with_actual_unitflag_lifecycle_owned(lease,
       [&](const ui::BnwOwnedCameraGuiScreen::UnitFlagLifecycleLoan&gui_life){
        membership.require_origin_without_callbacks();fifo.require_origin_without_callbacks();
        BnwWorldViewNativeContextIssuer issuer(impl_->worldview_sources,lease,*this,*impl_->host,
          gui_life,membership,impl_->worldview_trees);
        returned=impl_->host->dispatch_worldview_unit_damage_owned(issuer.operation(),membership,fifo);
        // Pure FIRST before callbackful lender POST; any caught failure stays sticky.
        membership.require_origin_without_callbacks();fifo.require_origin_without_callbacks();
        issuer.require_current();
        membership.require_origin_without_callbacks();fifo.require_origin_without_callbacks();
       });
      // Source DTOR has retired its active metadata slot. Never re-check the
      // bound Membership against the now-retired Source pointer. The real
      // member factory below owns the inactive-slot/sticky-failure POST.
      fifo.require_origin_without_callbacks();
      lease.require_scope_identity_without_callbacks(identity);
     });
    // GUI, NativeIssuer/Source and Membership facades have all unwound here.
    fifo.require_origin_without_callbacks();
    lease.require_scope_identity_without_callbacks(identity);
    need(static_cast<bool>(returned),"Health original event did not return an owned VM record");
    auto completion=impl_->host->complete_unitflag_health_fifo_owned(fifo,returned);
    fifo.require_origin_without_callbacks();
    lease.require_scope_identity_without_callbacks(identity);
    returned.reset();
    fifo.require_origin_without_callbacks();
    return completion;
   });
  lease.require_scope_identity_without_callbacks(identity);
  pump.require_source_owner_without_callbacks(source);pump.require_for_source(lease);
  lease.require_scope_identity_without_callbacks(identity);impl_->healthy();
 }catch(...){impl_->fail();throw;}
}
}
