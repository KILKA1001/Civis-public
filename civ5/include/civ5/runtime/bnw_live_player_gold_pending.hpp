#pragma once
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include "civ5/runtime/treasury_turn.hpp"
#include "civ5/runtime/bnw_live_game_context.hpp"
#include <array>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
namespace civ5::ui {class SelectedBnwPreviewHost;class BnwClientSelectionControl;class BnwEngineUiTurnOwner;}
namespace civ5::runtime {
class LiveSession;class BnwTreasurySameVmIncomeLoan;class BnwNativeUnitAllocatorCore;
enum class BnwLivePlayerGoldPhase {issued,executing,dirty_owner_required,dirty_executing,deficit_owner_required,deficit_executing,deficit_random_owner_required,deficit_random_executing,deficit_military_owner_required,native_returned,uncertain};
struct BnwLivePlayerGoldView {
 std::int64_t actor=-1,turn=-1;
 BnwLivePlayerGoldPhase phase=BnwLivePlayerGoldPhase::issued;
 std::uint64_t revision_before=0,revision_after=0,paired_stores=0,income_queries=0;
 std::array<std::int64_t,6> before{},after{};
 std::optional<TreasuryDirty> required_dirty;
 std::uint64_t dirty_setters=0;std::array<std::uint64_t,3> dirty_client_revisions{};
 std::int32_t deficit_combat_units=-1,deficit_cities=-1;
 bool deficit_scan_returned=false,deficit_no_disband_returned=false;
 bool deficit_rng_stored=false,deficit_rng_returned=false,deficit_rng_civis_trace_retained=false;
 bool deficit_rng_original_diagnostic_parity_unknown=true;
 std::optional<std::uint64_t> deficit_rng_value;
 SimulationRandomState deficit_rng_before{},deficit_rng_after{};
 bool standalone_component=true,achievement_backend_disabled=false;
};
// Closed original-import component. It NEVER grants CvPlayer scheduler entry.
// The actual Source factory supplies physical session/control and fresh real VM
// income loans; public callers cannot supply a world, ready bit or callback ACK.
class BnwLivePlayerGoldPending final {
 public:
 ~BnwLivePlayerGoldPending();
 BnwLivePlayerGoldPending(const BnwLivePlayerGoldPending&)=delete;
 BnwLivePlayerGoldPending&operator=(const BnwLivePlayerGoldPending&)=delete;
 BnwLivePlayerGoldView view()const;
 private:
 friend class BnwSelectedStockLuaSourceOwner;friend class BnwLiveGameContext;
 friend class ui::BnwClientSelectionControl;friend class ui::BnwEngineUiTurnOwner;
 void require_dirty_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,TreasuryDirty,const BnwStockLuaAttemptLease&)const;
 void require_dirty_identity_without_callbacks(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,TreasuryDirty,std::uint64_t)const;
 void accept_actual_dirty_return(const ui::BnwClientSelectionControl&,TreasuryDirty)const;
 static std::shared_ptr<const BnwLivePlayerGoldPending> retain_factory_owner(const std::shared_ptr<const BnwLivePlayerGoldPending>&);
 void require_deficit_source(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwStockLuaAttemptLease&)const;
 void require_deficit_identity_without_callbacks(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const std::weak_ptr<const BnwNativeUnitAllocatorCore>&,std::uint64_t)const;
 void accept_actual_deficit_scan_return(const BnwSelectedStockLuaSourceOwner&,const BnwNativeUnitAllocatorCore&)const;
 void begin_actual_deficit_random(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwStockLuaAttemptLease&)const;
 void require_deficit_rng_identity_without_callbacks(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::uint64_t)const;
 std::shared_ptr<void> prepare_actual_deficit_rng_store(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::uint64_t)const;
 void commit_actual_deficit_rng_store(LiveSessionState&,std::uint64_t,void*,std::uint64_t,
   const SimulationRandomState&,const SimulationRandomState&,std::uint64_t)const noexcept;
 void accept_actual_deficit_rng_return(const BnwLiveGameContext::GameReference&)const;
 struct Binding {
  explicit Binding(BnwStockLuaAttemptLease actual):lease(std::move(actual)){}
  BnwStockLuaAttemptLease lease;
  std::shared_ptr<LiveSessionState> world;
  const LiveSession* session=nullptr;
  std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
  std::weak_ptr<const BnwSelectedStockLuaSourceOwner> source;
  std::shared_ptr<const void> facade_storage;
  std::function<void()> require_facade;
  std::shared_ptr<const BnwLivePlayerGoldPending>* source_registry_slot=nullptr;
  std::function<std::unique_ptr<BnwTreasurySameVmIncomeLoan>()> fresh_income;
  // Sole private Source factory dispatches the real canonical receiver. It
  // cannot earn progress: only a sealed actual Selection store can accept.
  std::function<void(const std::shared_ptr<const BnwLivePlayerGoldPending>&,TreasuryDirty,const BnwStockLuaAttemptLease&)> actual_dirty;
  std::weak_ptr<const BnwNativeUnitAllocatorCore> deficit_core;
  std::function<void(const std::shared_ptr<const BnwLivePlayerGoldPending>&,const BnwStockLuaAttemptLease&)> actual_deficit;
  // Closed actual Source factory installs pure preparation and a typed
  // noexcept commit. The temporary strong scope lives only over native stores.
  std::function<std::shared_ptr<void>(std::uint64_t,std::uint64_t)> prepare_revision;
  void (*commit_revision)(void*,std::uint64_t) noexcept=nullptr;
  TreasuryRules treasury_rules;
  CityYieldRules city_rules;
  MaintenanceRules maintenance_rules;
 };
 explicit BnwLivePlayerGoldPending(Binding,std::int64_t);
 static std::shared_ptr<const BnwLivePlayerGoldPending> execute_owned(
  Binding,std::int64_t,std::shared_ptr<const BnwLivePlayerGoldPending>& issued_receipt);
 // Defined in GoldCPP beside the SAME ActualRawTreasuryMirror140 class.
 static std::unique_ptr<TreasuryNativeMirror> make_actual_mirror(LiveSessionState&,std::int64_t);
 struct Impl;std::unique_ptr<Impl> impl_;
};
class BnwLivePlayerGoldSourceFailure final:public std::runtime_error {
 public:
 const std::shared_ptr<const BnwLivePlayerGoldPending>& receipt()const noexcept{return receipt_;}
 private:
 friend class BnwSelectedStockLuaSourceOwner;
 explicit BnwLivePlayerGoldSourceFailure(std::shared_ptr<const BnwLivePlayerGoldPending> receipt,std::string reason)
  :std::runtime_error(std::move(reason)),receipt_(std::move(receipt)){}
 std::shared_ptr<const BnwLivePlayerGoldPending> receipt_;
};
}
