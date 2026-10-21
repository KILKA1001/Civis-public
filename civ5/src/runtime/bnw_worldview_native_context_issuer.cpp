#include "civ5/runtime/bnw_worldview_native_context_issuer.hpp"
#include "civ5/ui/bnw_worldview_renderer_projection_loan.hpp"
#include "civ5/ui/bnw_scroll_animation_update_loan.hpp"
#include <utility>
namespace civ5::runtime {
namespace {
constexpr const char* native_contexts[]={"InGame","WorldView","UnitFlagManager","ActionInfoPanel","NotificationPanel"};
std::shared_ptr<BnwSelectedWorldViewContextOwner> canonical_source(std::shared_ptr<BnwSelectedWorldViewContextOwner> source){
 if(!source)throw ScriptError("Native WorldView canonical source member missing");return source;
}
std::size_t context_index(std::string_view context){
 for(std::size_t i=0;i<5;++i)if(context==native_contexts[i])return i;
 throw ScriptError("Native WorldView context is outside selected InGame declarations");
}
}
std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>
BnwWorldViewNativeContextIssuer::construct_contexts(
 const std::shared_ptr<BnwSelectedWorldViewContextOwner>& owner,
 const BnwWorldViewSourceOperationLoan& loan,
 std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>& actual_slots){
 try{
 loan.require_render_operation_owned();
 const auto serial=loan.operation_serial();loan.require_current();
 auto root=actual_slots[0];
 if(!root){
  const auto gui_port=ui::BnwWorldViewControlTree::capture_context_gui_port_owned(loan,actual_slots);
  root=ui::BnwWorldViewControlTree::capture_owned(owner,"InGame",loan,gui_port);
  loan.require_operation(serial);
  // Actual allocated root is retained even if its genuine loader fails.
  actual_slots[0]=root;
  root->issue_direct_root_scalar_parent_owned(loan,root);loan.require_operation(serial);
  root->load_selected_context_native_root_owned(loan);loan.require_operation(serial);
 }
 if(!root)throw ScriptError("Native issuer actual InGame root absent");
 const auto result=root->borrow_direct_loaded_native_slots_owned(loan,root);
 loan.require_operation(serial);
 for(std::size_t i=0;i<result.size();++i){
  if(actual_slots[i]&&(actual_slots[i].get()!=result[i].get()||
     actual_slots[i].owner_before(result[i])||result[i].owner_before(actual_slots[i])))
   throw ScriptError("Native issuer existing slot is not the genuine RootRead child facade");
  if(!result[i])throw ScriptError("Native issuer required actual loaded RootRead slot absent");
 }
 // Registry is only an index of the already-created/attached native graph.
 // Its direct hooks validate whole LoadReady and preserve actual parent38.
 owner->publish_selected_client_context_slot1_owned(loan,result);loan.require_operation(serial);
 actual_slots=result;loan.require_operation(serial);
 return result;
 }catch(...){loan.diagnose_caught_primary_failure_owned();throw;}
}
BnwWorldViewNativeContextIssuer::BnwWorldViewNativeContextIssuer(
 std::shared_ptr<BnwSelectedWorldViewContextOwner> owner,const BnwStockLuaAttemptLease& lease,
 const BnwSelectedStockLuaBootstrap& bootstrap,const LuaScriptHost& vm,
 const ui::BnwOwnedCameraGuiScreen::Loan& gui,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,
 const ui::BnwWorldViewGridToWorldLoan& grid,
 std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>& actual_slots)
 :owner_(canonical_source(std::move(owner))),loan_(*owner_,lease,bootstrap,vm,gui,window,grid),
  trees_([&]{
   loan_.require_current();
   grid.prepare_clear_hex_highlights_owned(loan_,gui,window);
   loan_.require_current();
   return construct_contexts(owner_,loan_,actual_slots);
  }()){
  require_current();
  vm.drain_clear_hex_highlight_events_owned(loan_);
  require_current();
  grid.prepare_notification_cpu_geometry_owned(loan_,gui,window,trees_);
  require_current();
 }
BnwWorldViewNativeContextIssuer::BnwWorldViewNativeContextIssuer(
 std::shared_ptr<BnwSelectedWorldViewContextOwner> owner,const BnwStockLuaAttemptLease& lease,
 const BnwSelectedStockLuaBootstrap& bootstrap,const LuaScriptHost& vm,
 const ui::BnwOwnedCameraGuiScreen::UnitFlagLifecycleLoan& event,
 const BnwUnitFlagHealthInstanceLoan& membership,
 const std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>& actual_slots)
 :owner_(canonical_source(std::move(owner))),loan_(*owner_,lease,bootstrap,vm,event,membership),
  trees_{nullptr,nullptr,actual_slots[2],nullptr,nullptr},lifecycle_slots_(&actual_slots){
  // No tree capture, frame borrowing, Grid or initial geometry preparation.
  require_current();
}
void BnwWorldViewNativeContextIssuer::require_lifecycle_slot_owned()const {
 try{
  const auto pure=[&]{
   // Saved GUI/membership FIRST inside Source precedes the borrowed slot array.
   loan_.require_origin_without_callbacks();
   const auto& selected=trees_[2];const auto& actual=(*lifecycle_slots_)[2];
   if(!selected||!actual||selected.get()!=actual.get()||selected.owner_before(actual)||actual.owner_before(selected))
    throw ScriptError("UnitFlag lifecycle canonical native slot retired or replaced");
  };
  pure();const auto& selected=trees_[2];
  selected->require_current();pure();
  {
   const auto metadata=selected->metadata_source_owner();pure();
   if(metadata.get()!=owner_.get()||metadata.owner_before(owner_)||owner_.owner_before(metadata))
    throw ScriptError("UnitFlag lifecycle native slot metadata control differs");
   const auto context=selected->context();pure();
   if(context!="UnitFlagManager")throw ScriptError("UnitFlag lifecycle native slot context differs");
   const auto descriptor=selected->actual_source();pure();
   loan_.require_same_source(*descriptor);pure();
  }
  // Temporary checker owners are gone before final canonical slot validation.
  pure();
 }catch(...){loan_.poison_owned();loan_.diagnose_caught_primary_failure_owned();throw;}
}
BnwWorldViewNativeContextIssuer::~BnwWorldViewNativeContextIssuer()=default;
void BnwWorldViewNativeContextIssuer::dispatch_layout_control_owned(
 ui::BnwWorldViewControlTree& tree,const ui::BnwWorldViewControlTree::Control& control,
 const BnwWorldViewSourceOperationLoan& operation,LayoutControlCommand command,double value){
 // The complete-Core consumer owns FIRST, actual Control copying, SAME Source
 // callback bookends, genuine opaque initializer admission and Loan lifetime.
 using C=ui::BnwWorldViewControlTree::LayoutOperandCommand;
 switch(command){
 case LayoutControlCommand::calculate_size:tree.dispatch_layout_operand_owned(control,operation,C::calculate_size,value);return;
 case LayoutControlCommand::reprocess_anchoring:tree.dispatch_layout_operand_owned(control,operation,C::reprocess_anchoring,value);return;
 case LayoutControlCommand::set_size_y:tree.dispatch_layout_operand_owned(control,operation,C::set_size_y,value);return;
 case LayoutControlCommand::calculate_internal_size:tree.dispatch_layout_operand_owned(control,operation,C::calculate_internal_size,value);return;
 }
 // No callbacks or mutable Tree borrow are performed for an unissued enum.
 throw ScriptError("Layout NativeIssuer command is outside genuine four-command ABI");
}
void BnwWorldViewNativeContextIssuer::advance_scroll_updates_owned(
 const BnwWorldViewSourceOperationLoan& operation,
 const std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>& trees,const ui::BnwScrollAnimationUpdateLoan& update){
 operation.require_render_operation_owned();
 // The actual paint borrower preserves its original four renderer slots and
 // appends NotificationPanel; this differs from internal construction order.
 constexpr const char* update_contexts[]={"WorldView","UnitFlagManager","ActionInfoPanel","InGame","NotificationPanel"};
 update.require_current_without_callbacks();operation.require_origin_without_callbacks();
 operation.require_grid_scene_origin_without_callbacks(update.actual_scene_without_callbacks());
 update.require_current_without_callbacks();operation.require_origin_without_callbacks();
 for(std::size_t i=0;i<trees.size();++i){
  update.require_current_without_callbacks();operation.require_origin_without_callbacks();
  if(!trees[i])throw ScriptError("Scroll issuer genuine native tree absent");
  const auto source=trees[i]->actual_source();update.require_current_without_callbacks();
  operation.require_same_source(*source);update.require_current_without_callbacks();
  if(trees[i]->context()!=update_contexts[i])throw ScriptError("Scroll issuer native tree slot substituted");
  update.require_current_without_callbacks();operation.require_origin_without_callbacks();
  const auto hidden=trees[i]->context_update_hidden_without_callbacks(operation);
  update.require_current_without_callbacks();operation.require_origin_without_callbacks();
  if(!hidden)trees[i]->advance_selected_scroll_update_owned(operation,update);
  update.require_current_without_callbacks();operation.require_origin_without_callbacks();
 }
}
void BnwWorldViewNativeContextIssuer::require_current()const {
 const auto serial=loan_.operation_serial();loan_.require_current();
 if(lifecycle_slots_)require_lifecycle_slot_owned();
 owner_->require_sources_current();loan_.require_operation(serial);
}
const BnwWorldViewSourceOperationLoan& BnwWorldViewNativeContextIssuer::operation()const {
 require_current();return loan_;
}
std::shared_ptr<const ScriptUiChildSource>BnwWorldViewNativeContextIssuer::source(std::string_view context)const {
 loan_.require_render_operation_owned();
 const auto serial=loan_.operation_serial();require_current();
 auto result=owner_->prepared_source(context);loan_.require_operation(serial);
 loan_.require_same_source(*result);loan_.require_operation(serial);return result;
}
std::shared_ptr<ui::BnwWorldViewControlTree>BnwWorldViewNativeContextIssuer::tree(std::string_view context)const {
 loan_.require_render_operation_owned();
 const auto serial=loan_.operation_serial();require_current();
 auto result=trees_[context_index(context)];
 if(!result)throw ScriptError("Native WorldView actual canonical tree absent");
 result->require_current();loan_.require_operation(serial);return result;
}
}
