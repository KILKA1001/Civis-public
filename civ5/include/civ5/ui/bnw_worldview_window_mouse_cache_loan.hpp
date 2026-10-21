#pragma once
#include "civ5/ui/bnw_worldview_window_input_loan.hpp"
#ifdef _WIN32
#include <bit>
#include <optional>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {
// Private observation of the actual native window's persistent input fields.
// This observes payload, not GUI13B4 eligibility, hover or a returned message.
class BnwWorldViewWindowMouseCacheLoan final {
 friend class Win32ClientWindowBinding;
 friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
 using State=BnwWorldViewWindowInputLoan::State;
 const std::shared_ptr<State> state_;
 const std::weak_ptr<const void> operation_origin_;
 const void* const operation_address_;
 const std::uint64_t epoch_,cache_sequence_;
 const std::uint32_t w_650_,l_64c_64e_;
 const ClientViewport viewport_;
 BnwWorldViewWindowMouseCacheLoan(std::shared_ptr<State> actual,const void* operation)
  :state_(std::move(actual)),operation_origin_(state_->operation),operation_address_(operation),
   epoch_(state_->epoch),cache_sequence_(state_->mouse_cache_sequence),
   w_650_(state_->mouse_w_650.value()),l_64c_64e_(state_->mouse_l_64c_64e.value()),
   viewport_(state_->input->viewport()){}
 void require_source_owned()const {
  const auto saved=state_;
  try {
   if(GetCurrentThreadId()!=saved->thread||!saved->alive.load()||saved->failed.load())
    throw std::runtime_error("Persistent mouse cache creator/window source retired");
   const auto pure=[&]{
    if(!saved->alive.load()||saved->failed.load()||saved->epoch!=epoch_||saved->active_sequence||
       saved->mouse_cache_sequence!=cache_sequence_||!cache_sequence_||
       !saved->mouse_w_650||!saved->mouse_l_64c_64e||
       *saved->mouse_w_650!=w_650_||*saved->mouse_l_64c_64e!=l_64c_64e_||
       saved->operation.owner_before(operation_origin_)||operation_origin_.owner_before(saved->operation))
     throw std::runtime_error("Persistent actual mouse fields/window epoch changed");
   };
   pure();
   {const auto owner=saved->operation.lock();
    if(!owner||owner.get()!=operation_address_||owner.owner_before(operation_origin_)||operation_origin_.owner_before(owner))
     throw std::runtime_error("Persistent mouse actual operation allocation retired/replaced");}
   if(!IsWindow(saved->window)||GetWindowThreadProcessId(saved->window,nullptr)!=saved->thread||
      GetPropW(saved->window,L"Civis.ProductionClientWindow")!=saved->binding)
    throw std::runtime_error("Persistent mouse native binding replaced");
   pure();
   // Only after native facade/source lifetime is repeated may input be read.
   if(saved->input->viewport()!=viewport_)
    throw std::runtime_error("Persistent mouse actual input viewport changed");
   RECT rect{};if(!GetClientRect(saved->window,&rect))
    throw std::runtime_error("Persistent mouse client query failed");
   pure();
   if(rect.right!=viewport_.client_width||rect.bottom!=viewport_.client_height)
    throw std::runtime_error("Persistent mouse actual client extent changed");
  }catch(...){saved->failed.store(true);throw;}
 }
 // Raw Original32 payload semantics; no ScreenToClient/wheel transformation.
 WPARAM wparam_owned()const {require_source_owned();return static_cast<WPARAM>(w_650_);}
 LPARAM lparam_owned()const {require_source_owned();return static_cast<LPARAM>(std::bit_cast<std::int32_t>(l_64c_64e_));}
public:
 ~BnwWorldViewWindowMouseCacheLoan()=default;
 BnwWorldViewWindowMouseCacheLoan(const BnwWorldViewWindowMouseCacheLoan&)=delete;
 BnwWorldViewWindowMouseCacheLoan&operator=(const BnwWorldViewWindowMouseCacheLoan&)=delete;
};
}
#endif
