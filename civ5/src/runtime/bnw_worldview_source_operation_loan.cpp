#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include "civ5/runtime/detail/bnw_civis_application_state_owner.hpp"
#include "civ5/assets/detail/civis_loaded_xml_physical_path_loan.hpp"
#include "civ5/assets/detail/civis_loaded_bytes_physical_path_loan.hpp"
#include <algorithm>
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/graphics/detail/bnw_font_texture_observation.hpp"
#include "civ5/ui/detail/bnw_font_texture_request.hpp"
#include "civ5/runtime/detail/bnw_font_resource_loan.hpp"
#include "civ5/runtime/detail/bnw_font_resource_loan.hpp"
#include "civ5/runtime/bnw_selected_worldview_context_owner.hpp"
#include "civ5/runtime/detail/bnw_context_lua_profile_config_loan.hpp"
#include "civ5/runtime/bnw_unitflag_health_instance_loan.hpp"
#include "civ5/runtime/bnw_selected_stock_lua_bootstrap.hpp"
#include "civ5/runtime/bnw_selected_user_options_owner.hpp"
#include "civ5/content/bnw_gameinfo_actions_owner.hpp"
#include "../content/bnw_notification_entries_source_owner.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/ui/bnw_worldview_renderer_projection_loan.hpp"
#include "../../validation/bnw_label_process_module_owner.hpp"
#include "civ5/ui/civis_input_policy_owner.hpp"
#include "civ5/ui/win32_client_window.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <atomic>
#include <exception>
#include <stdexcept>
#include <cerrno>
#include <cstdio>
#include <thread>
#include <utility>
namespace civ5::runtime {
namespace {
void log_font_query_primary_failure(const char* message)noexcept {
 const int saved_errno=errno;
#ifdef _WIN32
 const DWORD saved_last_error=GetLastError();
#endif
 // Diagnostics only: no Source/Gui/lender callbacks or state admission.
 std::fprintf(stderr,"[BNW_FONT_QUERY_PRIMARY_FAILURE] %.2048s\n",message?message:"<null exception message>");
 std::fflush(stderr);
 errno=saved_errno;
#ifdef _WIN32
 SetLastError(saved_last_error);
#endif
}
void log_source_primary_failure(const char* message)noexcept {
 const int saved_errno=errno;
#ifdef _WIN32
 const DWORD saved_last_error=GetLastError();
#endif
 std::fprintf(stderr,"[BNW_SOURCE_OPERATION_PRIMARY_FAILURE] %.2048s\n",message?message:"<null runtime_error message>");
 std::fflush(stderr);
 errno=saved_errno;
#ifdef _WIN32
 SetLastError(saved_last_error);
#endif
}
void need(bool value,const char* why){if(!value)throw ScriptError(why);}
}
struct BnwWorldViewSourceOperationLoan::Impl {
 using Event=ui::BnwOwnedCameraGuiScreen::UnitFlagLifecycleLoan;
 using Membership=BnwUnitFlagHealthInstanceLoan;
 const Purpose purpose;
 const std::thread::id creator=std::this_thread::get_id();
 const int unwind=std::uncaught_exceptions();
 const std::shared_ptr<Event::State> event;
 const std::shared_ptr<Membership::State> membership;
 const std::weak_ptr<BnwSelectedWorldViewContextOwner> owner;
 const std::shared_ptr<BnwSelectedWorldViewContextOwner::State> storage;
 const BnwStockLuaAttemptLease lease;
 const BnwStockLuaAttemptLease::ScopeIdentity identity;
 const BnwSelectedStockLuaBootstrap* const bootstrap;
 const LuaScriptHost* const vm;
 const ui::BnwOwnedCameraGuiScreen::Loan* const gui;
 const graphics::BnwOwnedCameraRendererWindowLease::Loan* const window;
 const ui::BnwWorldViewGridToWorldLoan* const grid;
 std::weak_ptr<const ui::SelectedBnwPreviewHost> host;
 std::weak_ptr<const LiveSessionState> world;
 std::uint64_t generation=0,scope_serial=0;
 std::pair<const void*,std::uint64_t> vm_identity{};
 mutable std::atomic<bool> failed{false};
 // This bit only bounds diagnostics; it is never an admission predicate.
 mutable std::atomic<bool> primary_failure_reported{false};
 mutable bool checking=false;
 mutable bool screen_querying=false;
 Impl(BnwSelectedWorldViewContextOwner& source,const BnwStockLuaAttemptLease& active,
  const BnwSelectedStockLuaBootstrap& actual,const LuaScriptHost& actual_vm,
  const ui::BnwOwnedCameraGuiScreen::Loan& actual_gui,
  const graphics::BnwOwnedCameraRendererWindowLease::Loan& actual_window,const ui::BnwWorldViewGridToWorldLoan& actual_grid)
  :purpose(Purpose::render),owner(source.canonical_owner_without_callbacks()),storage(source.state_),lease(active),
   identity(active.pin_active_scope_without_callbacks()),bootstrap(&actual),vm(&actual_vm),
   gui(&actual_gui),window(&actual_window),grid(&actual_grid){
  need(identity.vm==bootstrap&&identity.control&&identity.world,"Native WorldView belongs to another actual Bootstrap VM/world");
  host=identity.control->selected_host;world=identity.control->world;
  // Pure current Host allocation is read only inside this native capture.
  // These temporary locks end before any provider callback or Lua entry.
  {
   const auto actual_host=host.lock();need(static_cast<bool>(actual_host),"Native WorldView selected Host expired");
   const auto actual_world=actual_host->session().read_state_lease();
   need(actual_world&&actual_world.get()==identity.world&&
     !actual_world.owner_before(world)&&!world.owner_before(actual_world),"Native WorldView source Host has another actual world control");
   generation=actual_host->session().state_generation();need(generation!=0,"Native WorldView actual generation absent");
  }
 }
 Impl(BnwSelectedWorldViewContextOwner& source,const BnwStockLuaAttemptLease& active,
  const BnwSelectedStockLuaBootstrap& actual,const LuaScriptHost& actual_vm,
  const Event& actual_event,const Membership& actual_membership)
  :purpose(Purpose::unitflag_health),event(actual_event.retain_storage_without_callbacks()),
   membership(actual_membership.retain_storage_without_callbacks()),owner(source.canonical_owner_without_callbacks()),
   storage(source.state_),lease(active),identity(active.pin_active_scope_without_callbacks()),
   bootstrap(&actual),vm(&actual_vm),gui(nullptr),window(nullptr),grid(nullptr){
  need(identity.vm==bootstrap&&identity.control&&identity.world,"Native WorldView belongs to another actual Bootstrap VM/world");
  host=identity.control->selected_host;world=identity.control->world;
  // Pure current Host allocation is read only inside this native capture.
  // These temporary locks end before any provider callback or Lua entry.
  {
   const auto actual_host=host.lock();need(static_cast<bool>(actual_host),"Native WorldView selected Host expired");
   const auto actual_world=actual_host->session().read_state_lease();
   need(actual_world&&actual_world.get()==identity.world&&
     !actual_world.owner_before(world)&&!world.owner_before(actual_world),"Native WorldView source Host has another actual world control");
   generation=actual_host->session().state_generation();need(generation!=0,"Native WorldView actual generation absent");
  }
  Event::require_origin_from_storage_without_callbacks(event);
  Membership::require_origin_from_storage_without_callbacks(membership);
 }
 void creator_current()const {
  if(creator!=std::this_thread::get_id()){failed.store(true);lease.invalidate();throw ScriptError("Native WorldView loan foreign creator");}
  need(!failed.load(),"Native WorldView loan poisoned");
  if(purpose==Purpose::unitflag_health){
   try{
    Event::require_origin_from_storage_without_callbacks(event);
    Membership::require_origin_from_storage_without_callbacks(membership);
   }catch(...){failed.store(true);lease.invalidate();throw;}
  }
 }
};
#include "bnw_system_font_language_operation_core.inc"
bool BnwWorldViewSourceOperationLoan::context_touch_bit8_owned()const{
 require_render_operation_owned();require_origin_without_callbacks();const auto& p=*impl_;
 try{
  need(p.bootstrap&&p.vm,"Context physical bit has no canonical Bootstrap/VM");
  const auto serial=operation_serial();require_same_vm(*p.vm);
  const auto owner=p.bootstrap->ensure_context_input_policy_before_load_owned(*this);
  require_operation(serial);require_same_vm(*p.vm);require_origin_without_callbacks();
  const bool bit=owner->is_touch_enabled(*this);
  require_operation(serial);require_same_vm(*p.vm);require_origin_without_callbacks();
  const auto repeated=p.bootstrap->ensure_context_input_policy_before_load_owned(*this);
  need(repeated.get()==owner.get()&&!owner.owner_before(repeated)&&!repeated.owner_before(owner),
       "Context actual physical input policy owner replaced");
  require_operation(serial);require_origin_without_callbacks();return bit;
 }catch(...){p.failed.store(true);p.lease.invalidate();diagnose_caught_primary_failure_owned();throw;}
}
BnwWorldViewSourceOperationLoan::BnwWorldViewSourceOperationLoan(
 BnwSelectedWorldViewContextOwner& source,const BnwStockLuaAttemptLease& lease,
 const BnwSelectedStockLuaBootstrap& bootstrap,const LuaScriptHost& vm,
 const ui::BnwOwnedCameraGuiScreen::Loan& gui,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,const ui::BnwWorldViewGridToWorldLoan& grid)
 :impl_(std::make_unique<Impl>(source,lease,bootstrap,vm,gui,window,grid)){
 const auto& p=*impl_;
 try {
  p.creator_current();p.lease.require_scope_identity_without_callbacks(p.identity);
  // Same weak source descriptor only proves lineage; this real fresh Lease's
  // current serial is independently authenticated and never copied from prior.
  p.lease.require_same_source_as_stored(source.stored_source_lineage_without_callbacks());
  p.lease.require_scope_identity_without_callbacks(p.identity);
  gui.owner_.require_worldview_source_origin_without_callbacks(gui,window.owner_,p.host,p.world,p.generation);
  gui.require_current();p.lease.require_scope_identity_without_callbacks(p.identity);
  window.require_current();p.lease.require_scope_identity_without_callbacks(p.identity);
  impl_->vm_identity=bootstrap.require_worldview_vm_origin_without_callbacks(source,vm,lease,*this);
  p.lease.require_operation(p.identity.serial);p.lease.require_scope_identity_without_callbacks(p.identity);
  source.require_metadata_current();p.lease.require_scope_identity_without_callbacks(p.identity);
  gui.owner_.require_worldview_source_origin_without_callbacks(gui,window.owner_,p.host,p.world,p.generation);
  need(bootstrap.require_worldview_vm_origin_without_callbacks(source,vm,lease,*this)==p.vm_identity,
       "Native WorldView actual VM/failure epoch changed during construction");
  source.seal_initial_native_generation_without_callbacks(p.world,p.generation);
  impl_->scope_serial=source.lend_operation_scope(*this);
  require_current();
  // NativeIssuer constructs trees immediately after this render Loan.
  // Bind real sources/lifetime before those constructors enter Host scopes.
  bootstrap.bind_worldview_sources_before_context_owned(*this);
  require_current();
 }catch(...){
  impl_->failed.store(true);p.lease.invalidate();
  diagnose_caught_primary_failure_owned();
  if(p.scope_serial)BnwSelectedWorldViewContextOwner::retire_operation_storage(p.storage,*this,p.scope_serial);
  throw;
 }
}
BnwWorldViewSourceOperationLoan::BnwWorldViewSourceOperationLoan(
 BnwSelectedWorldViewContextOwner& source,const BnwStockLuaAttemptLease& lease,
 const BnwSelectedStockLuaBootstrap& bootstrap,const LuaScriptHost& vm,
 const ui::BnwOwnedCameraGuiScreen::UnitFlagLifecycleLoan& event,
 const BnwUnitFlagHealthInstanceLoan& membership)
 :impl_(std::make_unique<Impl>(source,lease,bootstrap,vm,event,membership)){
 const auto& p=*impl_;
 try{
  p.creator_current();p.lease.require_scope_identity_without_callbacks(p.identity);
  p.lease.require_same_source_as_stored(source.stored_source_lineage_without_callbacks());p.creator_current();
  Impl::Event::require_source_origin_from_storage_without_callbacks(p.event,p.host,p.world,p.generation);
  p.creator_current();
  Impl::Membership::require_source_origin_from_storage_without_callbacks(
      p.membership,vm,source,p.host,p.world,p.generation);p.creator_current();
  Impl::Event::require_current_from_storage_owned(p.event);p.creator_current();
  impl_->vm_identity=bootstrap.require_worldview_vm_origin_without_callbacks(source,vm,lease,*this);
  p.creator_current();p.lease.require_operation(p.identity.serial);p.creator_current();
  source.require_metadata_current();p.creator_current();
  Impl::Event::require_source_origin_from_storage_without_callbacks(p.event,p.host,p.world,p.generation);
  p.creator_current();
  Impl::Membership::require_source_origin_from_storage_without_callbacks(
      p.membership,vm,source,p.host,p.world,p.generation);p.creator_current();
  need(bootstrap.require_worldview_vm_origin_without_callbacks(source,vm,lease,*this)==p.vm_identity,
       "UnitFlag lifecycle actual VM/failure epoch changed during construction");
  p.creator_current();source.seal_initial_native_generation_without_callbacks(p.world,p.generation);
  p.creator_current();impl_->scope_serial=source.lend_operation_scope(*this);
  // Bind the earned metadata slot and DISTINCT fresh Stock serial before POST.
  Impl::Membership::bind_source_scope_from_storage_without_callbacks(
      p.membership,*this,p.scope_serial,p.identity.serial);p.creator_current();
  // Only now may the genuine membership checker inspect active metadata.
  require_current();
 }catch(...){
  poison_owned();
  diagnose_caught_primary_failure_owned();
  if(p.scope_serial)BnwSelectedWorldViewContextOwner::retire_operation_storage(p.storage,*this,p.scope_serial);
  throw;
 }
}

#include "bnw_context_lua_physical_source_core.inc"

std::shared_ptr<const assets::detail::CivisLoadedXmlPhysicalPathLoan>
BnwWorldViewSourceOperationLoan::context_xml_physical_source_owned(const assets::AssetKey& key)const {
 require_render_operation_owned();require_origin_without_callbacks();
 const auto bank=selected_provider_startup_bank_owned();require_origin_without_callbacks();
 const auto capability=bank->capability();require_origin_without_callbacks();
 const auto result=capability.loaded_xml_physical_path_owned(key);require_origin_without_callbacks();
 result->require_same_bank_without_callbacks(*bank);require_current();
 result->require_same_bank_without_callbacks(*bank);require_origin_without_callbacks();return result;
}

void BnwWorldViewSourceOperationLoan::poison_owned()const noexcept {
 impl_->failed.store(true);impl_->lease.invalidate();
}
void BnwWorldViewSourceOperationLoan::diagnose_caught_primary_failure_owned()const noexcept {
 if(impl_->primary_failure_reported.exchange(true))return;
 // Every caller is a catch handler. Bare rethrow preserves the active error
 // without std::current_exception allocation/copy or any lender callback.
 try{throw;}
 catch(const std::runtime_error& error){
  // Qualified base call avoids dispatch through an arbitrary what() override.
  log_source_primary_failure(error.std::runtime_error::what());
 }catch(...){log_source_primary_failure("<non-runtime_error exception>");}
}

BnwWorldViewSourceOperationLoan::~BnwWorldViewSourceOperationLoan(){
 const auto& p=*impl_;
 if(p.creator!=std::this_thread::get_id())std::terminate();
 if(std::uncaught_exceptions()>p.unwind){p.failed.store(true);p.lease.invalidate();}
 BnwSelectedWorldViewContextOwner::retire_operation_storage(p.storage,*this,p.scope_serial);
}
void BnwWorldViewSourceOperationLoan::require_origin_without_callbacks()const {
 const auto& p=*impl_;p.creator_current();
 try {
  p.lease.require_scope_identity_without_callbacks(p.identity);
  const auto source=p.owner.lock();need(static_cast<bool>(source),"Native WorldView source facade retired");
  source->require_operation_origin_without_callbacks(*this,p.scope_serial);
  source->require_world_and_generation_without_callbacks(p.world,p.generation);
  // Actual closed Bootstrap checks its canonical source member and SAME Lua
  // allocation/failure epoch before any raw VM or GUI/window borrowed entry.
  need(p.bootstrap->require_worldview_vm_origin_without_callbacks(*source,*p.vm,p.lease,*this)==p.vm_identity,
       "Native WorldView canonical VM/failure identity changed");
  if(p.purpose==Purpose::render){
   need(p.gui&&p.window&&p.grid,"Native render operation lenders absent");
   p.gui->owner_.require_worldview_source_origin_without_callbacks(*p.gui,p.window->owner_,p.host,p.world,p.generation);
   p.window->owner_.require_origin_lifetime_without_native_callbacks();
  }else{
   Impl::Event::require_source_origin_from_storage_without_callbacks(p.event,p.host,p.world,p.generation);
   p.creator_current();
   Impl::Membership::require_source_origin_from_storage_without_callbacks(
       p.membership,*p.vm,*source,p.host,p.world,p.generation);p.creator_current();
  }
  p.lease.require_scope_identity_without_callbacks(p.identity);
 }catch(...){p.failed.store(true);p.lease.invalidate();diagnose_caught_primary_failure_owned();throw;}
}
#include "bnw_source_context_small_ui_policy_core.inc"
#include "bnw_source_gameinfo_actions_core.inc"
#include "bnw_source_notification_entries_core.inc"
#include "bnw_source_mod_activated_entrypoints_owned.inc"
#include "bnw_context_lua_include_collection_core.inc"
void BnwWorldViewSourceOperationLoan::require_render_operation_owned()const {
 require_origin_without_callbacks();const auto& p=*impl_;
 if(p.purpose!=Purpose::render){
  p.failed.store(true);p.lease.invalidate();
  throw ScriptError("Lifecycle Source operation cannot grant render/geometry/input/layout/frame");
 }
 require_origin_without_callbacks();
}
void BnwWorldViewSourceOperationLoan::require_same_render_loans_without_callbacks(
 const ui::BnwOwnedCameraGuiScreen::Loan& gui,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& window)const {
 require_render_operation_owned();const auto& p=*impl_;
 try{
  need(p.gui==&gui&&p.window==&window,"Resource0 initializer requires actual Source lexical GUI/Window loans");
  require_origin_without_callbacks();
 }catch(...){p.failed.store(true);p.lease.invalidate();diagnose_caught_primary_failure_owned();throw;}
}
bool BnwWorldViewSourceOperationLoan::unitflag_health_operation_without_callbacks()const {
 require_origin_without_callbacks();const auto result=impl_->purpose==Purpose::unitflag_health;
 require_origin_without_callbacks();return result;
}
void BnwWorldViewSourceOperationLoan::require_unitflag_health_control_owned(
 const ui::BnwWorldViewControlTree::Control& control,BnwUnitFlagHealthCommand command)const {
 require_origin_without_callbacks();const auto& p=*impl_;
 try{
  need(p.purpose==Purpose::unitflag_health,"Non-health Source cannot issue health mutation");
  Impl::Membership::require_control_from_storage_without_callbacks(p.membership,control,command);
  require_origin_without_callbacks();
 }catch(...){p.failed.store(true);p.lease.invalidate();diagnose_caught_primary_failure_owned();throw;}
}
void BnwWorldViewSourceOperationLoan::require_current()const {
 const auto& p=*impl_;p.creator_current();
 if(p.checking){p.failed.store(true);p.lease.invalidate();throw ScriptError("Native WorldView source operation reentered");}
 p.checking=true;struct End{bool& active;~End(){active=false;}} end{p.checking};
 try {
  require_origin_without_callbacks();
  if(p.purpose==Purpose::render){
   p.gui->require_current();require_origin_without_callbacks();
   p.window->require_current();require_origin_without_callbacks();
  }else{
   Impl::Event::require_current_from_storage_owned(p.event);require_origin_without_callbacks();
   Impl::Membership::require_current_from_storage_owned(p.membership);require_origin_without_callbacks();
  }
  p.lease.require_operation(p.identity.serial);require_origin_without_callbacks();
 }catch(...){p.failed.store(true);p.lease.invalidate();diagnose_caught_primary_failure_owned();throw;}
}
std::shared_ptr<const ui::BnwWorldViewWindowMouseCacheLoan>
BnwWorldViewSourceOperationLoan::mouse_cache_owned()const {
 require_render_operation_owned();
 require_origin_without_callbacks();
 try {
  require_current();require_origin_without_callbacks();
#ifdef _WIN32
  const auto cache=ui::Win32ClientWindowBinding::borrow_mouse_cache_owned(*impl_->window);
  require_origin_without_callbacks();cache->require_source_owned();
  require_current();require_origin_without_callbacks();cache->require_source_owned();
  return cache;
#else
  throw ScriptError("Actual Win32 persistent mouse source unavailable");
#endif
 }catch(...){impl_->failed.store(true);impl_->lease.invalidate();diagnose_caught_primary_failure_owned();throw;}
}
std::uint64_t BnwWorldViewSourceOperationLoan::operation_serial()const {
 require_origin_without_callbacks();return impl_->identity.serial;
}
void BnwWorldViewSourceOperationLoan::require_operation(std::uint64_t serial)const {
 require_origin_without_callbacks();
 if(serial!=impl_->identity.serial){impl_->failed.store(true);impl_->lease.invalidate();throw ScriptError("Native WorldView operation serial substituted");}
 require_current();
}
void BnwWorldViewSourceOperationLoan::require_same_source(const ScriptUiChildSource& descriptor)const {
 require_origin_without_callbacks();const auto source=impl_->owner.lock();
 need(static_cast<bool>(source),"Native WorldView source facade absent");
 source->require_source_member_without_callbacks(descriptor);require_origin_without_callbacks();
}
#include "bnw_gui_language_refresh_operation_core.inc"
#include "bnw_context_lua_profile_config_operation_core.inc"
#include "bnw_label_colorset_operation_core.inc"
#include "bnw_font_resource_operation_core.inc"
#include "bnw_font_texture_query_operation_core.inc"
#include "bnw_label_system_font_atlas_query_operation_core.inc"
detail::ContextLoaderResourceOperand BnwWorldViewSourceOperationLoan::context_loader_resource_operand_owned(
 const std::shared_ptr<const ScriptUiChildSource>& descriptor)const {
 try{
  require_render_operation_owned();require_origin_without_callbacks();
  const auto serial=operation_serial();std::shared_ptr<BnwSelectedWorldViewContextOwner> source;
  {source=impl_->owner.lock();need(static_cast<bool>(source),"Context loader canonical Source expired");}
  require_operation(serial);require_origin_without_callbacks();
  auto result=source->context_loader_resource_operand_owned(*this,descriptor);
  source.reset();require_origin_without_callbacks();require_operation(serial);require_origin_without_callbacks();
  return result;
 }catch(...){impl_->failed.store(true);impl_->lease.invalidate();diagnose_caught_primary_failure_owned();throw;}
}
std::vector<std::shared_ptr<ui::BnwWorldViewControlTree>>
BnwWorldViewSourceOperationLoan::selected_client_retained_context_trees_owned()const{
 try{
  require_origin_without_callbacks();require_render_operation_owned();
  const auto serial=operation_serial();require_origin_without_callbacks();
  auto source=impl_->owner.lock();need(static_cast<bool>(source),"Retained Context enumeration Source retired");
  auto result=source->selected_client_retained_context_trees_owned(*this);
  source.reset();require_origin_without_callbacks();require_operation(serial);require_origin_without_callbacks();
  // Real retained objects only; the renderer still supplies its independent
  // projection/GUI/window/Bank and revalidates this enumeration at each use.
  return result;
 }catch(...){impl_->failed.store(true);impl_->lease.invalidate();diagnose_caught_primary_failure_owned();throw;}
}
const LuaScriptHost& BnwWorldViewSourceOperationLoan::context_lua_host_owned()const {
 require_origin_without_callbacks();need(impl_->vm!=nullptr,"Context518 authentic Host absent");
 const auto& actual=*impl_->vm;require_same_vm(actual);return actual;
}
void BnwWorldViewSourceOperationLoan::require_same_vm(const LuaScriptHost& actual)const {
 require_origin_without_callbacks();
 if(&actual!=impl_->vm){impl_->failed.store(true);impl_->lease.invalidate();throw ScriptError("Native WorldView raw VM is not actual retained host");}
 require_origin_without_callbacks();
}
BnwStockLuaAttemptLease BnwWorldViewSourceOperationLoan::sp_client_source_owned()const {
 const auto& p=*impl_;p.creator_current();
 try {
  require_render_operation_owned();require_origin_without_callbacks();
  const auto result=p.lease;
  require_origin_without_callbacks();return result;
 }catch(...){p.failed.store(true);p.lease.invalidate();diagnose_caught_primary_failure_owned();throw;}
}
void BnwWorldViewSourceOperationLoan::require_sp_client_bootstrap_owned(
 const BnwSelectedStockLuaBootstrap& bootstrap,
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& selected,
 const std::shared_ptr<const LiveSessionState>& world)const {
 const auto& p=*impl_;p.creator_current();
 try {
  require_render_operation_owned();require_origin_without_callbacks();
  need(p.bootstrap==&bootstrap&&p.identity.control&&p.identity.control->vm==&bootstrap,
       "SP update Source has another actual Bootstrap VM/control");
  {
   const auto actual_host=p.host.lock();const auto actual_world=p.world.lock();
   need(actual_host&&selected&&actual_host.get()==selected.get()&&
        !actual_host.owner_before(selected)&&!selected.owner_before(actual_host),
        "SP update Source has another canonical selected Host");
   need(actual_world&&world&&actual_world.get()==world.get()&&
        p.identity.world==world.get()&&!actual_world.owner_before(world)&&!world.owner_before(actual_world),
        "SP update Source has another canonical live World");
  }
  require_origin_without_callbacks();
 }catch(...){p.failed.store(true);p.lease.invalidate();diagnose_caught_primary_failure_owned();throw;}
}
ui::detail::NotificationContextGuiClientOperand BnwWorldViewSourceOperationLoan::context_gui_client_operand_owned()const {
 require_render_operation_owned();require_origin_without_callbacks();const auto& p=*impl_;
 try{
  p.window->require_context_gui_client_profile_owned();require_origin_without_callbacks();
  const auto origin=input_policy_window_origin_owned();require_origin_without_callbacks();
  const auto scene=p.gui->owner_.context_gui_scene_origin_without_callbacks(*p.gui);require_origin_without_callbacks();
#ifdef _WIN32
  const auto hwnd=reinterpret_cast<HWND>(origin.hwnd);RECT rect{};
  const auto success=GetClientRect(hwnd,&rect);require_origin_without_callbacks();
  const std::int64_t width=std::int64_t(rect.right)-rect.left,height=std::int64_t(rect.bottom)-rect.top;
  need(success&&width>0&&height>0&&width<=INT32_MAX&&height<=INT32_MAX,
       "Context GUI actual client rectangle unsupported");
  require_input_policy_window_owned(origin);require_origin_without_callbacks();
  require_grid_scene_origin_without_callbacks(scene);require_origin_without_callbacks();
  need(origin.viewport.client_width==width&&origin.viewport.client_height==height,
       "Context GUI measured rectangle changed across SAME HWND observation");
  return ui::detail::NotificationContextGuiClientOperand(p.owner,p.host,p.world,scene,origin.window,
      p.generation,p.identity.serial,origin.viewport_revision,origin.hwnd,origin.occurrence,
      origin.process,origin.ui_thread,static_cast<std::int32_t>(width),static_cast<std::int32_t>(height));
#else
  throw ScriptError("Context GUI client port requires real Win32 HWND");
#endif
 }catch(...){p.failed.store(true);p.lease.invalidate();diagnose_caught_primary_failure_owned();throw;}
}
void BnwWorldViewSourceOperationLoan::require_context_gui_port_identity_without_callbacks(
 const std::weak_ptr<BnwSelectedWorldViewContextOwner>& source,const std::weak_ptr<const ui::SelectedBnwPreviewHost>& host,
 const std::weak_ptr<const LiveSessionState>& world,const std::weak_ptr<ui::BnwClientSceneFrame>& scene,
 const std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease>& window,std::uint64_t generation,
 std::uintptr_t hwnd,std::uintptr_t occurrence,std::uint64_t revision,std::uint32_t process,std::uint32_t thread)const{
 require_render_operation_owned();require_origin_without_callbacks();const auto& p=*impl_;
 const auto same=[](const auto&a,const auto&b){return !a.owner_before(b)&&!b.owner_before(a);};
 need(same(source,p.owner)&&same(host,p.host)&&same(world,p.world)&&generation==p.generation&&
      !source.expired()&&!host.expired()&&!world.expired(),"Context GUI canonical Source/Host/world/generation differs");
 {const auto actual=window.lock();need(actual&&actual.get()==&p.window->owner_&&same(window,actual),
      "Context GUI canonical renderer window allocation differs");}
 // Do not query HWND/native functions in this pure View/operand boundary.
 // The actual Source loan admits its existing GUI/window/Scene scope FIRST;
 // native rectangle/current occurrence is checked at full port boundaries.
 need(hwnd&&occurrence&&revision&&process&&thread,"Context GUI constructor window provenance absent");
 require_grid_scene_origin_without_callbacks(scene);require_origin_without_callbacks();
}
std::weak_ptr<BnwSelectedWorldViewContextOwner>
BnwWorldViewSourceOperationLoan::clear_hex_highlight_source_origin_without_callbacks_owned()const{
 require_render_operation_owned();require_origin_without_callbacks();return impl_->owner;
}
std::shared_ptr<graphics::BnwClearHexHighlightOwner>
BnwWorldViewSourceOperationLoan::clear_hex_highlight_owner_owned()const{
 require_render_operation_owned();require_origin_without_callbacks();
 const auto result=impl_->grid->clear_hex_highlight_owner_owned(*this);
 require_origin_without_callbacks();
 if(!result)throw ScriptError("Highlight canonical selected Scene owner absent");
 result->require_current_owned(*this);require_origin_without_callbacks();return result;
}
void BnwWorldViewSourceOperationLoan::require_grid_scene_origin_without_callbacks(const std::weak_ptr<ui::BnwClientSceneFrame>& scene)const {
 require_render_operation_owned();
 require_origin_without_callbacks();
 impl_->gui->owner_.require_worldview_scene_origin_without_callbacks(*impl_->gui,scene);
 require_origin_without_callbacks();
}
std::shared_ptr<ui::detail::BnwLabelProcessModuleOwner>
BnwWorldViewSourceOperationLoan::label_module_owned()const{
 require_render_operation_owned();require_origin_without_callbacks();
 if(!impl_->grid)throw ScriptError("Label module actual renderer lender absent");
 impl_->grid->require_for_source(*this);require_origin_without_callbacks();
 const auto result=impl_->grid->label_module_for_source_owned(*this);
 require_origin_without_callbacks();impl_->grid->require_for_source(*this);
 if(!result)throw ScriptError("Label module provider did not return genuine module");
 result->require_current_without_callbacks();require_origin_without_callbacks();return result;
}
std::array<double,3>BnwWorldViewSourceOperationLoan::grid_to_world_owned(std::int32_t x,std::int32_t y)const {
 require_render_operation_owned();
 require_current();impl_->grid->require_for_source(*this);require_current();
 const auto result=impl_->grid->world_xyz(*this,x,y);
 require_current();impl_->grid->require_for_source(*this);require_current();return result;
}

std::weak_ptr<const ui::SelectedBnwPreviewHost>
BnwWorldViewSourceOperationLoan::input_policy_host_origin_without_callbacks()const {
 require_render_operation_owned();
 require_origin_without_callbacks();const auto result=impl_->host;
 require_origin_without_callbacks();return result;
}
ui::CivisInputWindowOrigin BnwWorldViewSourceOperationLoan::input_policy_window_origin_owned()const {
 require_render_operation_owned();
 require_current();const auto& p=*impl_;
 try {
  ui::CivisInputWindowOrigin result;
  // The checker owns this temporary canonical lock only in this pure identity
  // interval; no strong window pin survives the following source bookend.
  {const auto actual=p.window->retain_owner();result.window=actual;}
  require_origin_without_callbacks();
  result.hwnd=p.window->window();require_origin_without_callbacks();
  result.process=p.window->process();require_origin_without_callbacks();
  result.ui_thread=p.window->ui_thread();require_origin_without_callbacks();
  result.viewport_revision=p.window->viewport_revision();require_origin_without_callbacks();
  result.occurrence=p.window->window_occurrence();require_origin_without_callbacks();
  result.viewport.framebuffer_width=p.gui->width();require_origin_without_callbacks();
  result.viewport.framebuffer_height=p.gui->height();require_origin_without_callbacks();
#ifdef _WIN32
  const auto hwnd=reinterpret_cast<HWND>(result.hwnd);
  need(GetCurrentThreadId()==result.ui_thread&&GetCurrentProcessId()==result.process,
       "Owned input policy has another actual native process/thread");
  RECT rect{};const auto measured=GetClientRect(hwnd,&rect);require_origin_without_callbacks();
  need(measured&&rect.left==0&&rect.top==0&&rect.right>0&&rect.bottom>0,
       "Owned input policy actual client extent unavailable");
  result.viewport.client_width=rect.right;result.viewport.client_height=rect.bottom;
  using GetWindowDpi=UINT(WINAPI*)(HWND);
  const auto user=GetModuleHandleW(L"user32.dll");require_origin_without_callbacks();
  const auto dpi_query=user?reinterpret_cast<GetWindowDpi>(GetProcAddress(user,"GetDpiForWindow")):nullptr;
  require_origin_without_callbacks();need(dpi_query!=nullptr,"Owned input policy actual DPI query unavailable");
  const auto dpi=dpi_query(hwnd);require_origin_without_callbacks();
  need(dpi!=0,"Owned input policy actual DPI unavailable");result.viewport.dpi=dpi;
#else
  throw ScriptError("Owned input policy requires actual Win32 client");
#endif
  need(result.viewport.client_width==result.viewport.framebuffer_width&&
       result.viewport.client_height==result.viewport.framebuffer_height,
       "Owned input policy requires same actual client and GUI viewport");
  require_current();return result;
 }catch(...){p.failed.store(true);p.lease.invalidate();diagnose_caught_primary_failure_owned();throw;}
}
void BnwWorldViewSourceOperationLoan::require_input_policy_window_owned(const ui::CivisInputWindowOrigin& expected)const {
 require_render_operation_owned();
 require_origin_without_callbacks();const auto actual=input_policy_window_origin_owned();
 require_origin_without_callbacks();
 {
  const auto a=actual.window.lock(),b=expected.window.lock();
  need(a&&b&&a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a),
       "Owned input policy canonical window get/control changed");
 }
 require_origin_without_callbacks();
 need(actual.hwnd==expected.hwnd&&actual.occurrence==expected.occurrence&&
      actual.process==expected.process&&actual.ui_thread==expected.ui_thread&&
      actual.viewport_revision==expected.viewport_revision&&actual.viewport==expected.viewport,
      "Owned input policy actual window/viewport occurrence changed");
 require_current();
}

std::shared_ptr<BnwCivisApplicationStateOwner> BnwWorldViewSourceOperationLoan::context_application_state_owned()const {
 require_render_operation_owned();require_origin_without_callbacks();
 const auto actual_host=impl_->host.lock();need(bool(actual_host),"Application actual selected Host expired");
 const auto application=actual_host->bnw_civis_application_state_.lock();
 need(bool(application),"Application current state supplier absent");
 application->require_loaded_host_owned(*actual_host);require_origin_without_callbacks();return application;
}

std::pair<std::int64_t,std::int64_t> BnwWorldViewSourceOperationLoan::screen_size_owned()const {
 require_render_operation_owned();
 // This private query borrows the actual SAME Source/GUI/window lender. A public
 // screen provider or copied viewport cannot issue this result.
 require_origin_without_callbacks();const auto& p=*impl_;
 if(p.screen_querying){p.failed.store(true);p.lease.invalidate();throw ScriptError("Native WorldView screen query reentered");}
 p.screen_querying=true;struct End{bool& active;~End(){active=false;}}end{p.screen_querying};
 try {
  require_current();require_origin_without_callbacks();
  const auto revision=p.gui->frame_revision();require_origin_without_callbacks();
  const auto origin=input_policy_window_origin_owned();require_origin_without_callbacks();
  need(origin.viewport.client_width>0&&origin.viewport.client_height>0,
       "Native WorldView actual client screen extent invalid");
  const std::pair<std::int64_t,std::int64_t> result{origin.viewport.client_width,origin.viewport.client_height};
  // Re-measure the same actual HWND, DPI, viewport revision and canonical weak
  // window control after the supplier. No temporary strong window crosses it.
  require_input_policy_window_owned(origin);require_origin_without_callbacks();
  const auto after=p.gui->frame_revision();require_origin_without_callbacks();
  need(after==revision,"Native WorldView screen GUI frame revision changed");
  require_current();require_origin_without_callbacks();return result;
 }catch(...){p.failed.store(true);p.lease.invalidate();diagnose_caught_primary_failure_owned();throw;}
}

}

namespace civ5::ui {
void BnwWorldViewGridToWorldLoan::require_scene_for_source(
 const runtime::BnwWorldViewSourceOperationLoan& source,
 const std::weak_ptr<BnwClientSceneFrame>& scene){
 source.require_grid_scene_origin_without_callbacks(scene);
}
}
