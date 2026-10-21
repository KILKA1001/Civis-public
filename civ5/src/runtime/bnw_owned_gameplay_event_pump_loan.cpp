#include "civ5/runtime/bnw_owned_gameplay_event_pump_loan.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/ui/live_ui_host.hpp"
#include <limits>
#include <stdexcept>
namespace civ5::runtime {
namespace {void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}}
void BnwOwnedGameplayEventPumpLoan::Clock::enter_native_batch(){
 need(creator==std::this_thread::get_id(),"Gameplay pump native batch foreign creator");
 if(active||native_batch){failed=true;throw std::runtime_error("Gameplay pump/native batch reentry");}
 need(alive&&!failed,"Gameplay pump native owner retired");native_batch=true;
}
void BnwOwnedGameplayEventPumpLoan::Clock::leave_native_batch(){
 need(creator==std::this_thread::get_id()&&alive&&!failed&&native_batch&&!active,
  "Gameplay pump missing actual native batch unwind");
 need(native_sequence!=std::numeric_limits<std::uint64_t>::max(),"Gameplay pump native sequence exhausted");
 ++native_sequence;native_batch=false;
}
BnwOwnedGameplayEventPumpLoan::BnwOwnedGameplayEventPumpLoan(Clock& clock,
 std::weak_ptr<BnwSelectedStockLuaSourceOwner> source,std::weak_ptr<const ui::BnwClientSceneFrame> scene,
 std::weak_ptr<const ui::LiveUiHost> ui,std::function<void()> current)
 :clock_(clock),native_(clock.native_sequence),boundary_(clock.boundary+1),source_(std::move(source)),
  scene_(std::move(scene)),ui_(std::move(ui)),camera_current_(std::move(current)){
 need(clock.creator==std::this_thread::get_id(),"Gameplay pump foreign creator");
 if(clock.active||clock.native_batch){clock.failed=true;throw std::runtime_error("Gameplay pump entry reentry/before native unwind");}
 try{
  need(clock.alive&&!clock.failed&&native_&&clock.boundary!=std::numeric_limits<std::uint64_t>::max()&&camera_current_,
   "Gameplay pump owner/native publication unavailable");
  clock.active=true;clock.boundary=boundary_;
  {const auto actual=source_.lock();need(bool(actual),"Gameplay pump actual selected source expired");
   lineage_=actual->require_gameplay_pump_source_without_callbacks();}
  full_camera();
 }catch(...){clock.failed=true;clock.active=false;throw;}
}
BnwOwnedGameplayEventPumpLoan::~BnwOwnedGameplayEventPumpLoan(){if(!returned_)fail();clock_.active=false;}
void BnwOwnedGameplayEventPumpLoan::fail()const noexcept {clock_.failed=true;}
void BnwOwnedGameplayEventPumpLoan::pure()const{
 try{
 need(clock_.creator==std::this_thread::get_id(),"Gameplay pump borrower foreign creator");
 need(clock_.alive&&!clock_.failed&&clock_.active&&!clock_.native_batch&&clock_.native_sequence==native_&&
  clock_.boundary==boundary_&&!returned_,"Gameplay pump expired/reentered native boundary");
 const auto actual=source_.lock();need(actual&&actual.use_count()>1,"Gameplay pump actual outside selected source retired");
 const auto current=actual->require_gameplay_pump_source_without_callbacks();
 const auto scene=scene_.lock();const auto ui=ui_.lock();const auto host=lineage_.host.lock();const auto world=lineage_.world.lock();
 need(scene&&ui&&host&&world&&scene.use_count()>1&&ui.use_count()>1&&
  current.vm==lineage_.vm&&current.generation==lineage_.generation&&lineage_.vm&&
  !current.host.owner_before(lineage_.host)&&!lineage_.host.owner_before(current.host)&&
  !current.world.owner_before(lineage_.world)&&!lineage_.world.owner_before(current.world)&&
  !current.control.owner_before(lineage_.control)&&!lineage_.control.owner_before(current.control)&&
  !current.canonical_source.owner_before(lineage_.canonical_source)&&!lineage_.canonical_source.owner_before(current.canonical_source)&&
  scene->gui_self_.lock().get()==scene.get()&&ui->selected_owner_lifetime_.lock().get()==ui.get()&&
  scene->selected_host_.get()==host.get()&&!scene->selected_host_.owner_before(host)&&!host.owner_before(scene->selected_host_)&&
  scene->scene_state_==world.get()&&scene->scene_state_generation_==lineage_.generation,
  "Gameplay pump canonical selected Host/world/VM/Scene/UI lineage changed");
 // The closed camera construction captures its actual Scene-created UI slot;
 // full_camera checks genuine Scene::accepts around real source callbacks.
 // No callback-bearing plot-yield currentness enters this pure boundary.
 }catch(...){fail();throw;}
}
void BnwOwnedGameplayEventPumpLoan::full_camera()const{
 pure();
 if(checking_){fail();throw std::runtime_error("Gameplay pump camera callback reentry");}
 checking_=true;
 try{
  std::function<void()> current=camera_current_;pure();
  current();pure();current={};pure();checking_=false;
 }catch(...){fail();checking_=false;throw;}
}
void BnwOwnedGameplayEventPumpLoan::require_source_owner_without_callbacks(const BnwSelectedStockLuaSourceOwner& source)const{
 try{
  pure();{const auto actual=source_.lock();need(actual&&actual.get()==&source,"Gameplay pump substituted actual source owner");}
  pure();
 }catch(...){fail();throw;}
}
void BnwOwnedGameplayEventPumpLoan::require_for_source(const BnwStockLuaAttemptLease& source)const{
 try{
  full_camera();
  // This pure closed seam proves canonical SourceOwner/VM/current active
  // Control before checked() can enter its real source/provider callbacks.
  auto identity=source.pin_active_scope_without_callbacks();pure();
  const auto control=source.control_.lock();
  need(control&&identity.source_owner.get()==lineage_.canonical_source.lock().get()&&
   !identity.source_owner.owner_before(lineage_.canonical_source)&&!lineage_.canonical_source.owner_before(identity.source_owner)&&
   identity.control.get()==control.get()&&!identity.control.owner_before(control)&&!control.owner_before(identity.control)&&
   identity.vm&&identity.vm==lineage_.vm&&identity.world&&identity.world==lineage_.world.lock().get()&&
   !source.control_.owner_before(lineage_.control)&&!lineage_.control.owner_before(source.control_)&&
   identity.serial&&identity.creator==clock_.creator,
   "Gameplay pump source operation/VM/control lineage substituted");
  const auto prior=source_control_.lock();
  if(prior)need(prior.get()==control.get()&&!prior.owner_before(control)&&!control.owner_before(prior)&&
   vm_==identity.vm&&operation_==identity.serial,"Gameplay pump active receiver source replaced");
  else {source_control_=control;vm_=identity.vm;operation_=identity.serial;}
  source.require_scope_identity_without_callbacks(identity);pure();
  source.require_operation(operation_);pure();source.require_scope_identity_without_callbacks(identity);
  identity={};full_camera();
 }catch(...){fail();throw;}
}
std::uint64_t BnwOwnedGameplayEventPumpLoan::boundary()const{pure();return boundary_;}
void BnwOwnedGameplayEventPumpLoan::complete(){full_camera();returned_=true;}
}
