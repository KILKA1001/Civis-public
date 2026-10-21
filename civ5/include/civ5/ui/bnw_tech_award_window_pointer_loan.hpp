#pragma once
#include "civ5/ui/client_window_input.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <stdexcept>
namespace civ5::ui {
class Win32ClientWindowBinding;class UiScreenGlRenderer;
// Closed WM_MOUSEMOVE source. Its entered authority ends with the actual
// sink target's destruction. Deferred readers require a separate successful
// dispatch completion and the same live native binding/viewport epoch.
class BnwTechAwardWindowPointerLoan final {
 friend class Win32ClientWindowBinding;friend class UiScreenGlRenderer;
 struct State {
  const HWND window;const DWORD thread;const ClientWindowInput* const input;
  const Win32ClientWindowBinding* const binding;
  std::weak_ptr<const void> operation;
  std::atomic<bool> alive{true},failed{false};
  std::uint64_t epoch=1,sequence=0,active_sequence=0;
  State(HWND w,const ClientWindowInput& i,const Win32ClientWindowBinding& b)
   :window(w),thread(GetCurrentThreadId()),input(&i),binding(&b){}
 };
 const std::shared_ptr<State> state_;
 const std::uint64_t sequence_,epoch_;
 const ClientPoint point_;const ClientViewport viewport_;
 mutable bool captured_=false,dispatch_completed_=false;
 std::weak_ptr<const BnwTechAwardWindowPointerLoan> self_;
 BnwTechAwardWindowPointerLoan(std::shared_ptr<State> s,std::uint64_t n,ClientPoint p,ClientViewport v)
  :state_(std::move(s)),sequence_(n),epoch_(state_->epoch),point_(p),viewport_(v){}
 void require_source()const {
  if(GetCurrentThreadId()!=state_->thread||!state_->alive.load()||state_->failed.load())
   throw std::runtime_error("TechAward native pointer source retired/foreign thread");
  // Independent source state is checked before the borrowed input facade.
  const auto operation=state_->operation.lock();
  if(!operation||!IsWindow(state_->window)||GetWindowThreadProcessId(state_->window,nullptr)!=state_->thread||
     GetPropW(state_->window,L"Civis.ProductionClientWindow")!=state_->binding||
     !state_->alive.load()||state_->failed.load()||state_->epoch!=epoch_)
   throw std::runtime_error("TechAward native pointer binding/epoch retired");
  if(state_->input->viewport()!=viewport_)
   throw std::runtime_error("TechAward native pointer viewport replaced");
  RECT rect{};
  if(!GetClientRect(state_->window,&rect)||rect.right!=viewport_.client_width||rect.bottom!=viewport_.client_height)
   throw std::runtime_error("TechAward native pointer client size changed");
  if(!state_->alive.load()||state_->failed.load()||state_->epoch!=epoch_)
   throw std::runtime_error("TechAward native pointer source changed during native validation");
 }
 void require_entered()const {
  require_source();
  if(!sequence_||state_->active_sequence!=sequence_||state_->sequence!=sequence_||dispatch_completed_)
   throw std::runtime_error("TechAward pointer loan is outside actual native sink");
 }
 void require_completed()const {
  require_source();
  if(!dispatch_completed_||!captured_||!sequence_||state_->active_sequence||state_->sequence<sequence_)
   throw std::runtime_error("TechAward pointer dispatch did not complete normally");
 }
 public:
 ~BnwTechAwardWindowPointerLoan()=default;
 BnwTechAwardWindowPointerLoan(const BnwTechAwardWindowPointerLoan&)=delete;
 BnwTechAwardWindowPointerLoan& operator=(const BnwTechAwardWindowPointerLoan&)=delete;
};
}
#endif
