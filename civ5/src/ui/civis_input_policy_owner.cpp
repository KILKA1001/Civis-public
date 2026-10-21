#include "civ5/ui/civis_input_policy_owner.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include <atomic>
#include <exception>
#include <limits>
#include <stdexcept>
#include <thread>
#include <utility>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace civ5::ui {
namespace {
void need(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
// Pinned CvEnums.h530: DEBUG0, SELECTION1,...ESTABLISH_TRADE_ROUTE23.
// This is our explicit controller store, never an Original current mode quote.
constexpr std::int32_t selected_mouse_keyboard_initial_mode=1;
constexpr std::int32_t interface_mode_count=24;
}
struct CivisInputPolicyOwner::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool>alive{true},failed{false},retired{false};
 bool busy=false,initialized=false;
 // Actual owned policy state set by this one supported route initializer.
 const bool touch_enabled=false;
 const std::weak_ptr<const SelectedBnwPreviewHost>host;
 const std::weak_ptr<const runtime::LiveSessionState>world;
 const std::weak_ptr<const assets::CivisResourceBankOwner>bank;
 const assets::CivisResourceBankCapability capability;
 const CivisInputWindowOrigin window;
 const CivisInputDeviceFacts devices;
 const std::string session_payload;
 const std::int64_t selected_player;
 const std::vector<CivisInputPolicySource>sources;
 std::int32_t mode=selected_mouse_keyboard_initial_mode;
 std::uint64_t revision=1;
 Impl(const std::shared_ptr<const SelectedBnwPreviewHost>&h,
  const std::shared_ptr<assets::CivisResourceBankOwner>&b,
  std::weak_ptr<const runtime::LiveSessionState>w,CivisInputWindowOrigin origin,
  CivisInputDeviceFacts facts,std::vector<CivisInputPolicySource>records)
  :host(h),world(std::move(w)),bank(b),capability(b->capability()),window(std::move(origin)),
   devices(facts),session_payload(h->identity().payload_sha256),selected_player(h->active_player_id()),
   sources(std::move(records)){}
 void creator_live()const{
  if(creator!=std::this_thread::get_id()){
   const_cast<Impl*>(this)->failed.store(true);throw std::runtime_error("Civis input controller foreign creator");
  }
  need(alive&&!failed&&!retired,"Civis input controller retired or poisoned");
 }
 void pure()const{
  creator_live();need(initialized,"Civis input controller has not completed genuine initialization");
  capability.require_current();
  const auto h=host.lock();const auto b=bank.lock();const auto w=world.lock();
  need(h&&b&&w&&!window.window.expired()&&bank.use_count()>1,
       "Civis input controller producer outside owner retired");
  const auto actual=h->session().read_state_lease();
  need(actual&&actual.get()==w.get()&&!actual.owner_before(w)&&!w.owner_before(actual)&&
       h->identity().payload_sha256==session_payload&&h->active_player_id()==selected_player,
       "Civis input controller selected session/world changed");
  h->require_same_civis_resources(*b);creator_live();
 }
 struct Operation {
  const std::shared_ptr<Impl>p;int exceptions=std::uncaught_exceptions();
  explicit Operation(std::shared_ptr<Impl>i):p(std::move(i)){
   try{p->pure();if(p->busy){p->failed=true;throw std::runtime_error("Civis input controller callback reentry");}
    p->busy=true;}catch(...){p->failed=true;throw;}
  }
  ~Operation(){if(std::uncaught_exceptions()>exceptions)p->failed=true;p->busy=false;}
 };
 void current(const runtime::BnwWorldViewSourceOperationLoan&loan)const{
  pure();
  {
   const auto expected=loan.input_policy_host_origin_without_callbacks();
   const auto actual=expected.lock();const auto stored=host.lock();
   need(actual&&stored&&actual.get()==stored.get()&&!host.owner_before(expected)&&!expected.owner_before(host),
        "Civis input controller has another actual Native source Host");
  }
  creator_live();const auto serial=loan.operation_serial();pure();
  loan.require_operation(serial);pure();
  loan.require_input_policy_window_owned(window);pure();
  {
   // This one producer pin only protects the real callback body; pure() uses
   // its own checker pin, and this explicit fence excludes our operation pin.
   const auto actual_bank=bank.lock();
   need(actual_bank&&bank.use_count()>1,"Civis input bank outside owner absent");
   actual_bank->require_boundary();creator_live();
   need(bank.use_count()>1,"Civis input bank outside owner retired during callback");
  }
  pure();loan.require_operation(serial);pure();
  loan.require_input_policy_window_owned(window);pure();
 }
};
CivisInputPolicyOwner::CivisInputPolicyOwner(std::shared_ptr<Impl>p):impl_(std::move(p)){}
CivisInputPolicyOwner::~CivisInputPolicyOwner(){impl_->alive=false;}
void CivisInputPolicyOwner::retire()noexcept{impl_->retired=true;}
std::shared_ptr<CivisInputPolicyOwner>CivisInputPolicyOwner::initialize_mouse_keyboard(
 const std::weak_ptr<const SelectedBnwPreviewHost>&supplied_host,
 const std::weak_ptr<assets::CivisResourceBankOwner>&supplied_bank,
 const runtime::BnwWorldViewSourceOperationLoan&loan){
#ifndef _WIN32
 (void)supplied_host;(void)supplied_bank;(void)loan;
 throw std::runtime_error("Civis mouse/keyboard controller requires actual Win32 input binding");
#else
 // Creator/current source FIRST, before copying any caller-owned weak state.
 const auto serial=loan.operation_serial();
 const auto selected_host=supplied_host;const auto selected_bank=supplied_bank;
 need(!selected_host.expired()&&!selected_bank.expired(),
      "Civis input initialization requires real selected Host/Bank");
 // Canonical metadata only before any native/client callback. Native helper
 // is pure and independently tied to the actual selected source operation.
 const auto expected=loan.input_policy_host_origin_without_callbacks();
 std::weak_ptr<const SelectedBnwPreviewHost>host_weak;
 std::weak_ptr<assets::CivisResourceBankOwner>bank_weak;
 std::weak_ptr<const runtime::LiveSessionState>world;
 {
  const auto host=expected.lock();const auto supplied_host_owner=selected_host.lock();
  const auto supplied_bank_owner=selected_bank.lock();
  need(host&&supplied_host_owner&&supplied_bank_owner,
       "Civis input selected canonical producer retired before initialization");
  const auto bank=supplied_bank_owner->retain_owner();
  need(host.get()==supplied_host_owner.get()&&!expected.owner_before(selected_host)&&!selected_host.owner_before(expected),
       "Civis input Host supplied another factory control");
  need(bank&&bank.get()==supplied_bank_owner.get()&&!bank.owner_before(selected_bank)&&!selected_bank.owner_before(bank),
       "Civis input Bank supplied another factory control");
  host->require_same_civis_resources(*bank);
  const auto actual_world=host->session().read_state_lease();
  need(static_cast<bool>(actual_world),"Civis input selected session state absent");
  host_weak=host;bank_weak=bank;world=actual_world;
 } // No uncounted Host/world/Bank pin crosses a Native source callback.
 const auto current=[&]{
  loan.require_operation(serial);
  {
   const auto bank=bank_weak.lock();
   need(bank&&bank_weak.use_count()>1,"Civis input bank outside owner absent during initialization");
   bank->require_boundary();
   need(bank_weak.use_count()>1,"Civis input bank outside owner retired during initialization");
  } // Drop our body pin BEFORE the genuine Native source POST.
  loan.require_operation(serial);
  {
   const auto host=host_weak.lock();const auto bank=bank_weak.lock();
   const auto canonical=loan.input_policy_host_origin_without_callbacks();
   need(host&&bank&&host.get()==canonical.lock().get()&&
        !host_weak.owner_before(canonical)&&!canonical.owner_before(host_weak),
        "Civis input selected Host retired or changed during initialization");
   host->require_same_civis_resources(*bank);
  }
 };
 current();
 std::vector<CivisInputPolicySource>sources;
 std::uint64_t staged=0;
 const auto stage=[&](const char*key){
  current();
  {
   const auto bank=bank_weak.lock();
   need(bank&&bank_weak.use_count()>1,"Civis input bank outside owner absent before actual load");
   bank->load(assets::AssetKey(key),assets::CivisResourceParse::bytes);
   need(bank_weak.use_count()>1,"Civis input bank outside owner retired during actual load");
  }
  current();
  const auto loaded=[&]{
   const auto bank=bank_weak.lock();
   need(static_cast<bool>(bank),"Civis input bank retired before loaded-byte read");
   return bank->capability().loaded(assets::AssetKey(key));
  }();
  need(loaded.bytes&&loaded.bytes->size()<=8ULL*1024*1024&&
       staged<=8ULL*1024*1024-loaded.bytes->size(),"Civis input policy source closure exceeds budget");
  staged+=loaded.bytes->size();sources.push_back({key,loaded.candidate,loaded.payload_identity});
  current();return loaded;
 };
 const auto modes=stage("Assets/Gameplay/XML/Interface/CIV5InterfaceModes.xml");
 const auto document=content::read_xml_metadata(std::string(modes.bytes->begin(),modes.bytes->end()));
 const auto*rows=document.child("InterfaceModes");
 need(document.name=="GameData"&&rows,"Civis input actual authored InterfaceModes unavailable");
 std::size_t selections=0,ordinal=0;
 for(const auto&row:rows->children)if(row.name=="Row"){
  const auto*type=row.child("Type");const auto*id=row.child("ID");
  if(ordinal==0)need(type&&type->text=="INTERFACEMODE_DEBUG"&&id&&id->text=="0",
      "Civis input selected interface table has no authentic ID0 sequence origin");
  if(type&&type->text=="INTERFACEMODE_SELECTION"){
   need(ordinal==1&&(!id||id->text=="1"),
        "Civis input authored selection ID differs from SDK1");++selections;
  }
  ++ordinal;
 }
 need(selections==1,"Civis input own selection mode lacks unique actual authored declaration");
 stage("Assets/UI/InGame/WorldView/WorldView.xml");
 stage("Assets/UI/InGame/WorldView/WorldView.lua");
 stage("Assets/UI/Options/OptionsMenu.xml");stage("Assets/UI/Options/OptionsMenu.lua");
 const auto origin=loan.input_policy_window_origin_owned();current();
 need(origin.hwnd&&origin.occurrence&&origin.viewport_revision&&!origin.window.expired()&&
      origin.process==GetCurrentProcessId()&&origin.ui_thread==GetCurrentThreadId()&&origin.viewport.drawable()&&
      origin.viewport.client_width==origin.viewport.framebuffer_width&&
      origin.viewport.client_height==origin.viewport.framebuffer_height,
      "Civis input actual native window origin unsupported");
 const auto native_window=reinterpret_cast<HWND>(origin.hwnd);DWORD process=0;
 need(IsWindow(native_window)&&GetWindowThreadProcessId(native_window,&process)==origin.ui_thread&&
      process==origin.process,"Civis input actual native HWND process/thread changed");
 loan.require_input_policy_window_owned(origin);current();
 const CivisInputDeviceFacts facts{GetSystemMetrics(SM_MOUSEPRESENT),GetSystemMetrics(SM_CMOUSEBUTTONS),
                                  GetSystemMetrics(SM_DIGITIZER),GetKeyboardType(0)};
 loan.require_input_policy_window_owned(origin);current();
 // This named selected-session constructor explicitly initializes the OWN
 // supported Win32 mouse/key route. Hardware touch flags are evidence only:
 // this binding has no WM_POINTER/WM_TOUCH dispatch and no touch policy enable.
 const auto p=[&]{
  const auto host=host_weak.lock();const auto bank=bank_weak.lock();
  need(host&&bank,"Civis input selected producers retired before immutable controller publish");
  return std::make_shared<Impl>(host,bank,world,origin,facts,std::move(sources));
 }(); // Constructor reads pure metadata/capability only; pins drop before POST.
 current();loan.require_input_policy_window_owned(origin);current();
 p->initialized=true; // Only after real source/window/session initialization.
 auto owner=std::shared_ptr<CivisInputPolicyOwner>(new CivisInputPolicyOwner(p));
 owner->require_for_source(loan);return owner;
#endif
}
void CivisInputPolicyOwner::require_for_source(const runtime::BnwWorldViewSourceOperationLoan&loan)const{
 impl_->creator_live();const auto p=impl_;Impl::Operation operation(p);p->current(loan);
}
bool CivisInputPolicyOwner::is_touch_enabled(const runtime::BnwWorldViewSourceOperationLoan&loan)const{
 impl_->creator_live();const auto p=impl_;Impl::Operation operation(p);p->current(loan);
 // Explicit initialized mouse_keyboard route, never a missing-state fallback.
 p->pure();return p->touch_enabled;
}
std::int32_t CivisInputPolicyOwner::interface_mode(const runtime::BnwWorldViewSourceOperationLoan&loan)const{
 impl_->creator_live();const auto p=impl_;Impl::Operation operation(p);p->current(loan);p->pure();return p->mode;
}
CivisInputDeviceFacts CivisInputPolicyOwner::device_facts(const runtime::BnwWorldViewSourceOperationLoan&loan)const{
 impl_->creator_live();const auto p=impl_;Impl::Operation operation(p);p->current(loan);p->pure();return p->devices;
}
std::vector<CivisInputPolicySource>CivisInputPolicyOwner::source_records(const runtime::BnwWorldViewSourceOperationLoan&loan)const{
 impl_->creator_live();const auto p=impl_;Impl::Operation operation(p);p->current(loan);auto result=p->sources;p->pure();return result;
}
CivisInterfaceModeChange CivisInputPolicyOwner::set_interface_mode_owned(
 const runtime::BnwWorldViewSourceOperationLoan&loan,std::int32_t mode){
 impl_->creator_live();const auto p=impl_;Impl::Operation operation(p);p->current(loan);
 need(mode>=0&&mode<interface_mode_count,"Civis input requested interface mode is outside SDK profile");
 const auto previous=p->mode;
 if(mode==previous){p->pure();return {previous,previous,p->revision,false};}
 need(p->revision!=std::numeric_limits<std::uint64_t>::max(),"Civis input controller revision exhausted");
 // All genuine callback/currentness bookends precede this pure owned commit.
 p->pure();p->mode=mode;++p->revision;return {previous,mode,p->revision,true};
}
}
