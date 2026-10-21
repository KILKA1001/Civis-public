#include "bnw_actual_policy_reformation_resource_probe.hpp"
#include "bnw_policy_adoption_fixture.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <stdexcept>
#include <utility>
namespace civ5::tests {
std::shared_ptr<const runtime::BnwPolicyAdoptionPending>
run_actual_policy_reformation_resource_probe(runtime::BnwSelectedStockLuaSourceOwner& source,
 const ui::SelectedBnwPreviewHost& host,const runtime::BnwPolicyAdoptionCommand& command,
 std::shared_ptr<const ui::LiveUiHost> actual_ui,ActualPolicyReformationResourceScenario scenario){
 if(scenario!=ActualPolicyReformationResourceScenario::reformation&&scenario!=ActualPolicyReformationResourceScenario::resource)
  throw std::invalid_argument("Unknown actual policy effect scenario");
 // Observe the original physical world before the real command. The closed
 // Pending/host seal in the independent oracle rejects a substituted Host.
 const auto before=runtime::observe_bnw_policy_native(host,command.player);
 const bool had_actual_ui=bool(actual_ui);
 const auto receipt=source.run_same_policy_adoption_prefix(command,std::move(actual_ui));
 verify_actual_bnw_policy_adoption_prefix(host,before,receipt);
 if(had_actual_ui&&receipt->phase()==runtime::BnwPolicyAdoptionPhase::policy_happiness_dirty_pending)
  throw std::runtime_error("Actual UI policy caller did not consume mandatory happiness GameData effect");
 if(scenario==ActualPolicyReformationResourceScenario::reformation){
  if(!receipt->reformation_notification_native_returned()||!receipt->reformation_notification_returned())
   throw std::runtime_error("UNAVAILABLE: natural policy scenario did not reach actual Reformation native Add return");
 }else{
  std::optional<std::int64_t> installed_reveals;
  const auto finishing=receipt->finisher_granted()?receipt->finishing_policy():std::optional<std::int64_t>{};
  const auto report=host.database().query_rows({
   "SELECT COUNT(*) FROM Resources R JOIN Policies P ON R.PolicyReveal=P.Type WHERE P.ID = ? OR P.ID = ?",
   {std::to_string(command.policy),std::to_string(finishing.value_or(-1))}},
   [&](const content::RulesRowView& row){if(installed_reveals)throw std::runtime_error("Duplicate natural PolicyReveal oracle row");installed_reveals=row.integer_at(0);});
  receipt->require_stored_current(host);
  if(!report.all_succeeded()||report.rows!=1||!installed_reveals||*installed_reveals<=0)
   throw std::runtime_error("UNAVAILABLE: natural command/earned finisher has no installed Resource.PolicyReveal rule");
  if(!receipt->resource_policy_loop_reached())
   throw std::runtime_error("UNAVAILABLE: natural policy scenario did not reach actual PolicyReveal map loop");
  if(!receipt->resource_policy_layout_returned_plots().empty()&&!receipt->resource_policy_layout_effects_current())
   throw std::runtime_error("Actual resource layout history lost canonical runtime PlotData/layout effects");
  if(receipt->resource_policy_layout_plot()){
   if(had_actual_ui)throw std::runtime_error("Actual UI resource caller did not consume its visible layout effect");
   if(receipt->phase()!=runtime::BnwPolicyAdoptionPhase::resource_policy_layout_pending||receipt->resource_policy_loop_returned())
    throw std::runtime_error("Actual revealed resource lost mandatory native layout frontier");
  }else if(!receipt->resource_policy_loop_returned()&&receipt->phase()!=runtime::BnwPolicyAdoptionPhase::plot_ui_pending)
   throw std::runtime_error("Actual resource caller stopped before its genuine yield/UI/layout boundary");
 }
 // Returned receipt reports the precise effect/frontier, never full BNW,
 // renderer delivery, save readiness, or an invented source success flag.
 return receipt;
}
}
