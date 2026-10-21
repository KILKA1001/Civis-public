#pragma once
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <thread>
namespace civ5::validation {template<class LandRenderer>struct CameraPreviewLoop;}
namespace civ5::ui {class BnwClientSceneFrame;class LiveUiHost;class BnwEngineUiTurnOwner;}
namespace civ5::runtime {
// One actual camera-loop lender. Its private construction site is after the
// real PeekMessage/DispatchMessage batch unwound and native publication gates.
// An ordinary next source serial, callback or bool cannot construct this loan.
class BnwOwnedGameplayEventPumpLoan final {
 public:
  BnwOwnedGameplayEventPumpLoan(const BnwOwnedGameplayEventPumpLoan&)=delete;
  BnwOwnedGameplayEventPumpLoan& operator=(const BnwOwnedGameplayEventPumpLoan&)=delete;
  BnwOwnedGameplayEventPumpLoan(BnwOwnedGameplayEventPumpLoan&&)=delete;
  BnwOwnedGameplayEventPumpLoan& operator=(BnwOwnedGameplayEventPumpLoan&&)=delete;
  void require_source_owner_without_callbacks(const BnwSelectedStockLuaSourceOwner&)const;
  void require_for_source(const BnwStockLuaAttemptLease&)const;
  std::uint64_t boundary()const;
 private:
  friend class BnwSelectedStockLuaSourceOwner;
  friend class ::civ5::ui::BnwEngineUiTurnOwner;
  struct SourceLineage final {
   std::weak_ptr<const ui::SelectedBnwPreviewHost> host;
   std::weak_ptr<const LiveSessionState> world;
   std::weak_ptr<const BnwStockLuaAttemptLease::Control> control;
   std::weak_ptr<const void> canonical_source;
   const BnwSelectedStockLuaBootstrap* vm=nullptr;
   std::uint64_t generation=0;
  };
  template<class LandRenderer>friend struct ::civ5::validation::CameraPreviewLoop;
  struct Clock final {
   const std::thread::id creator=std::this_thread::get_id();
   std::atomic<bool> alive{true},failed{false};
   bool native_batch=false,active=false;std::uint64_t native_sequence=0,boundary=0;
   void enter_native_batch();void leave_native_batch();
   ~Clock(){alive=false;failed=true;}
  };
  Clock& clock_;const std::uint64_t native_,boundary_;
  const std::weak_ptr<BnwSelectedStockLuaSourceOwner> source_;
  const std::weak_ptr<const ui::BnwClientSceneFrame> scene_;
  const std::weak_ptr<const ui::LiveUiHost> ui_;
  SourceLineage lineage_;
  const std::function<void()> camera_current_;
  mutable bool checking_=false;
  mutable std::weak_ptr<const BnwStockLuaAttemptLease::Control> source_control_;
  mutable const BnwSelectedStockLuaBootstrap* vm_=nullptr;
  mutable std::uint64_t operation_=0;
  bool returned_=false;
  BnwOwnedGameplayEventPumpLoan(Clock&,std::weak_ptr<BnwSelectedStockLuaSourceOwner>,
      std::weak_ptr<const ui::BnwClientSceneFrame>,std::weak_ptr<const ui::LiveUiHost>,std::function<void()>);
  ~BnwOwnedGameplayEventPumpLoan();
  void pure()const;void full_camera()const;void complete();
  void fail()const noexcept;
};
}
