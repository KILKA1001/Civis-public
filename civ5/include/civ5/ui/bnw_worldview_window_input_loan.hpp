#pragma once
#include "civ5/ui/client_window_input.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
namespace civ5::runtime {class BnwSelectedStockLuaSourceOwner;class BnwSelectedStockLuaBootstrap;}
namespace civ5::ui {
class Win32ClientWindowBinding;
class Win32ViewportTransaction;
class BnwWorldViewWindowMouseCacheLoan;
// Entered only through the genuine native sink COPY/INVOKE/DESTROY interval.
// Later delivery uses its successful completion; public event DTOs issue none.
class BnwWorldViewWindowInputLoan final {
 friend class Win32ClientWindowBinding;
 friend class Win32ViewportTransaction;
 friend class ::civ5::graphics::BnwOwnedCameraRendererWindowLease;
 friend class BnwWorldViewWindowMouseCacheLoan;
 friend class ::civ5::runtime::BnwSelectedStockLuaSourceOwner;
 friend class ::civ5::runtime::BnwSelectedStockLuaBootstrap;
 struct State {
  const HWND window;const DWORD thread;const ClientWindowInput* const input;
  const Win32ClientWindowBinding* const binding;std::weak_ptr<const void> operation;
  std::atomic<bool> alive{true},failed{false};
  std::uint64_t epoch=1,sequence=0,active_sequence=0;
  // +650 accepts every actual200..20E; +64C/+64E only actual200.
  // Absence is unobserved, never a fabricated constructor-zero grant.
  std::optional<std::uint32_t> mouse_w_650,mouse_l_64c_64e;
  std::uint64_t mouse_cache_sequence=0;
  State(HWND w,const ClientWindowInput& i,const Win32ClientWindowBinding& b)
   :window(w),thread(GetCurrentThreadId()),input(&i),binding(&b){}
 };
 const std::shared_ptr<State> state_;
 const std::uint64_t sequence_,epoch_;
 const UINT message_;const WPARAM w_;const LPARAM l_;const ClientViewport viewport_;
 mutable bool captured_=false,dispatch_completed_=false,claimed_=false;
 mutable bool shutdown_completed_=false;
 mutable std::uint64_t claim_boundary_=0;
 std::weak_ptr<const BnwWorldViewWindowInputLoan> self_;
 // Written only by the SAME physical renderer observer AFTER the real native
 // close continuation and binding completion return. No close request grants it.
 mutable std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> close_window_owner_;
 mutable const graphics::BnwOwnedCameraRendererWindowLease* close_window_get_=nullptr;
 void require_observed_close_window(const graphics::BnwOwnedCameraRendererWindowLease::Loan& actual)const {
  if(message_!=WM_CLOSE)return;
  if(close_window_owner_.expired()||close_window_get_!=&actual.owner_||
     close_window_owner_.owner_before(actual.owner_.self_)||actual.owner_.self_.owner_before(close_window_owner_))
   throw std::runtime_error("WorldView close has no SAME returned native renderer observer");
 }
 BnwWorldViewWindowInputLoan(std::shared_ptr<State> state,std::uint64_t sequence,
  UINT message,WPARAM w,LPARAM l,ClientViewport viewport)
  :state_(std::move(state)),sequence_(sequence),epoch_(state_->epoch),message_(message),w_(w),l_(l),viewport_(viewport){}
 void require_source()const {
  if(GetCurrentThreadId()!=state_->thread||!state_->alive.load()||state_->failed.load())
   throw std::runtime_error("WorldView input creator/source retired");
  const auto storage=state_->operation.lock();
  if(!storage||!IsWindow(state_->window)||GetWindowThreadProcessId(state_->window,nullptr)!=state_->thread||
   GetPropW(state_->window,L"Civis.ProductionClientWindow")!=state_->binding||
   !state_->alive.load()||state_->failed.load()||state_->epoch!=epoch_)
   throw std::runtime_error("WorldView input actual binding/epoch retired");
  if(state_->input->viewport()!=viewport_)throw std::runtime_error("WorldView input viewport changed");
  RECT rect{};
  if(!GetClientRect(state_->window,&rect)||rect.right!=viewport_.client_width||rect.bottom!=viewport_.client_height||
   !state_->alive.load()||state_->failed.load()||state_->epoch!=epoch_)
   throw std::runtime_error("WorldView input current client extent changed");
 }
 void require_entered()const {
  require_source();
  if(!sequence_||state_->sequence!=sequence_||state_->active_sequence!=sequence_||dispatch_completed_||claimed_)
   throw std::runtime_error("WorldView input outside actual native sink");
 }
 void capture()const {
  require_entered();if(captured_)throw std::runtime_error("WorldView native input captured twice");
  captured_=true;
 }
 void require_completed()const {
  require_source();
  if(!captured_||!dispatch_completed_||claimed_||!sequence_||state_->active_sequence||state_->sequence<sequence_)
   throw std::runtime_error("WorldView native input dispatch did not complete");
 }
 void claim_for(const graphics::BnwOwnedCameraRendererWindowLease::Loan& actual)const {
  require_completed();actual.require_current();require_completed();
  if(actual.window()!=reinterpret_cast<std::uintptr_t>(state_->window)||actual.ui_thread()!=state_->thread)
   throw std::runtime_error("WorldView input belongs to another actual renderer window");
  require_observed_close_window(actual);
  require_completed();claim_boundary_=state_->sequence;claimed_=true;
  require_claimed_for(actual);
 }
 void require_claimed_for(const graphics::BnwOwnedCameraRendererWindowLease::Loan& actual)const {
  require_source();
  if(!claimed_||!captured_||!dispatch_completed_||state_->active_sequence||state_->sequence!=claim_boundary_)
   throw std::runtime_error("WorldView input changed/reentered during deferred native delivery");
  actual.require_current();require_source();require_observed_close_window(actual);
  if(actual.window()!=reinterpret_cast<std::uintptr_t>(state_->window)||actual.ui_thread()!=state_->thread||
   state_->active_sequence||state_->sequence!=claim_boundary_)
   throw std::runtime_error("WorldView deferred input actual window replaced");
 }
 void require_shutdown_returned()const {
  require_source();
  if(message_!=WM_CLOSE||close_window_owner_.expired()||!close_window_get_||
     !shutdown_completed_||!claimed_||!captured_||!dispatch_completed_||
     state_->active_sequence||state_->sequence!=claim_boundary_)
   throw std::runtime_error("WorldView close has no actual returned shutdown/source bookends");
 }
 public:
 ~BnwWorldViewWindowInputLoan()=default;
 BnwWorldViewWindowInputLoan(const BnwWorldViewWindowInputLoan&)=delete;
 BnwWorldViewWindowInputLoan& operator=(const BnwWorldViewWindowInputLoan&)=delete;
};
}
#endif
