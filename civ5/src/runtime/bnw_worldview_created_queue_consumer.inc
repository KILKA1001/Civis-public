namespace civ5::runtime {
bool BnwSelectedStockLuaBootstrap::drain_worldview_unit_created_owned(
 const BnwWorldViewSourceOperationLoan&loan,const std::weak_ptr<graphics::BnwUnitSetupGraphicalQueue>&queue){
 require_prepared_owned();loan.require_current();
 need(impl_->worldview_operation==&loan&&impl_->worldview_sources&&impl_->host&&impl_->unitflag_native,
  "Created consumer needs SAME canonical current Bootstrap/Source/UnitFlag owner");
 const auto source=impl_->worldview_sources->prepared_source("UnitFlagManager");
 loan.require_same_source(*source);loan.require_current();
 const bool consumed=graphics::BnwUnitSetupGraphicalQueue::with_first_created_owned(queue,loan,
  [&](const auto&occurrence){impl_->host->dispatch_worldview_unit_created_owned(loan,occurrence);});
 loan.require_current();return consumed;
}
}
