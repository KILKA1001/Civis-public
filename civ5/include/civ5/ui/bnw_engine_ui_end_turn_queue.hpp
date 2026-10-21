#pragma once
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <thread>
namespace civ5::runtime {class BnwUnitFlagHealthInstanceLoan;class BnwOwnedGameplayEventPumpLoan;class LuaScriptHost;class BnwPlayerTurnActiveDirtyIssuer;class ScriptUiChildSource;class ScriptUiContextSourceOwner;}
namespace civ5::ui {
class LiveUiHost;class SelectedBnwPreviewHost;class BnwEngineUiTurnOwner;
// Source stages only: activation and the later actual event-pump lender remain
// private owner integration gates. A source-operation serial is NOT a pump loan.
class BnwEngineUiEndTurnQueue final {
 public:
  ~BnwEngineUiEndTurnQueue();
  BnwEngineUiEndTurnQueue(const BnwEngineUiEndTurnQueue&)=delete;
  BnwEngineUiEndTurnQueue& operator=(const BnwEngineUiEndTurnQueue&)=delete;
 private:
  friend class BnwEngineUiTurnOwner;
  friend class ::civ5::runtime::BnwPlayerTurnActiveDirtyIssuer;
  friend class ::civ5::runtime::BnwSelectedStockLuaBootstrap;
  friend class ::civ5::runtime::LuaScriptHost;
  friend class ::civ5::runtime::BnwUnitFlagHealthInstanceLoan;
  struct VmLifetime final {
   const std::thread::id creator=std::this_thread::get_id();
   std::atomic<bool> alive{true},failed{false};
   runtime::LuaScriptHost* host=nullptr;
   const void* vm=nullptr;
  };
  struct Consumer final {
   std::weak_ptr<VmLifetime> lifetime;
   std::weak_ptr<const runtime::ScriptUiContextSourceOwner> sources;
   std::weak_ptr<const runtime::ScriptUiChildSource> source;
   std::size_t child=0;
   std::uint64_t registration=0,epoch=0;
   const void* vm=nullptr;
   const void* registered_state=nullptr;
   Consumer()=default;
  };
  struct Event final {
   const std::uint64_t occurrence,operation;
   const std::shared_ptr<const runtime::BnwPlayerTurnActiveDirtyIssuer> player_dirty_issuer;
   bool setter_returned=false;
   Event(std::uint64_t n,std::uint64_t op):occurrence(n),operation(op){}
   Event(std::uint64_t n,std::uint64_t op,std::shared_ptr<const runtime::BnwPlayerTurnActiveDirtyIssuer> p):occurrence(n),operation(op),player_dirty_issuer(std::move(p)){}
   Event(const Event&)=delete;Event& operator=(const Event&)=delete;
  };
  class Completion final {
   friend class BnwEngineUiEndTurnQueue;
   friend class ::civ5::runtime::BnwPlayerTurnActiveDirtyIssuer;
   friend class ::civ5::runtime::LuaScriptHost;
   Completion(const Event& e,const Consumer& c):event_(&e),consumer_(&c),occurrence_(e.occurrence){}
   const Event* const event_;const Consumer* const consumer_;const std::uint64_t occurrence_;
   Completion(const Completion&)=delete;Completion& operator=(const Completion&)=delete;
  };
  struct State;
  // Constructor is accessible only to the actual Bootstrap pump owner. It must
  // be called at its explicit later UI pump hook, never inline in a setter or
  // merely because another StockSource operation started.
  class PumpLoan final {
   friend class BnwEngineUiEndTurnQueue;
   friend class BnwEngineUiTurnOwner;
   PumpLoan(BnwEngineUiEndTurnQueue&,const runtime::BnwStockLuaAttemptLease&,const runtime::BnwOwnedGameplayEventPumpLoan&);
   ~PumpLoan();
   PumpLoan(const PumpLoan&)=delete;PumpLoan& operator=(const PumpLoan&)=delete;
   const runtime::BnwOwnedGameplayEventPumpLoan& actual_pump_;
   std::shared_ptr<State> state_;std::uint64_t boundary_=0,operation_=0;
   bool returned_=false;
  };
  static std::shared_ptr<BnwEngineUiEndTurnQueue> bind_fresh(
   std::shared_ptr<const BnwEngineUiTurnOwner>,std::shared_ptr<const LiveUiHost>,
   std::shared_ptr<const SelectedBnwPreviewHost>);
  explicit BnwEngineUiEndTurnQueue(std::shared_ptr<State>);
  void attach_consumer(BnwEngineUiTurnOwner&,std::shared_ptr<const Consumer>,
   const runtime::BnwStockLuaAttemptLease&);
  // Only the actual Engine changed old-true/new-false branch calls this AFTER
  // its unconditional field store. No boolean argument can claim that edge.
  std::uint64_t enqueue_changed_false(BnwEngineUiTurnOwner&,const runtime::BnwStockLuaAttemptLease&);
  void publish_player_turn_dirty(BnwEngineUiTurnOwner&,const std::shared_ptr<const runtime::BnwPlayerTurnActiveDirtyIssuer>&,const runtime::BnwStockLuaAttemptLease&);
  void mark_setter_return(BnwEngineUiTurnOwner&,std::uint64_t,const runtime::BnwStockLuaAttemptLease&);
  bool has_pending_owned()const;
  void drain(PumpLoan&,const runtime::BnwStockLuaAttemptLease&);
  static void identity(const std::shared_ptr<State>&);
  static runtime::LuaScriptHost& checked_vm(const std::shared_ptr<State>&);
  std::shared_ptr<State> state_;
};
}
