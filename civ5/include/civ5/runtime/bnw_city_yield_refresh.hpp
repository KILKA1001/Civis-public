#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
namespace civ5::saves {struct SavedSession;}
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
class BnwCityFoodGrowthPending;class BnwCityPopulationPending;class LiveSession;class BnwCityEntryPending;class BnwPlotYieldRefreshPending;
class BnwPlotYieldRefreshIssuer;struct LiveSessionState;struct SessionCheckpointIdentity;
class BnwCityYieldRefreshRegistry;class BnwCityYieldRefreshCheckpoint;
enum class BnwCityYieldRefreshPhase {native_slot,plot_call,source_loop_returned,terminal_failure,quarantined};
struct BnwCityYieldRefreshProgress {
 std::int32_t owner=-1,city=-1;
 std::uint64_t operation=0,attempts=0;
 std::uint8_t next_slot=0,null_skips=0,returned_calls=0;
 std::optional<std::uint64_t> current_child_operation;
 BnwCityYieldRefreshPhase phase=BnwCityYieldRefreshPhase::native_slot;
};
// Session-owned source cursor. Public progress is diagnostic, never an effect
// receipt. Only a registered actual Entry call can consume private completion.
class BnwCityYieldRefreshPending final {
 public:
 ~BnwCityYieldRefreshPending();
 BnwCityYieldRefreshPending(const BnwCityYieldRefreshPending&)=delete;
 BnwCityYieldRefreshPending& operator=(const BnwCityYieldRefreshPending&)=delete;
 void require_current()const;
 BnwCityYieldRefreshProgress progress()const;
 private:
 friend class BnwCityFoodGrowthPending;friend class LiveSession;friend class BnwCityYieldRefresh;
 friend class BnwCityYieldRefreshRegistry;friend class BnwCityEntryPending;
 friend class BnwPlotYieldRefreshPending;
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwCityYieldRefreshPending(std::unique_ptr<Impl>)noexcept;
 static std::shared_ptr<BnwCityYieldRefreshPending> prepare(LiveSession&,
  const std::shared_ptr<const ui::SelectedBnwPreviewHost>&,
  const std::shared_ptr<BnwCityYieldRefreshRegistry>&,std::int64_t,std::int64_t,
  const std::shared_ptr<BnwCityEntryPending>&,const std::shared_ptr<BnwCityPopulationPending>& population={});
 void advance(LiveSession&,const std::shared_ptr<const ui::SelectedBnwPreviewHost>&);
 std::int64_t require_unissued_slot()const;
 std::pair<std::uint64_t,std::uint8_t> private_call_identity()const;
 void require_child_source(const BnwPlotYieldRefreshIssuer&,const LiveSessionState&,
  const saves::SavedSession&,const SessionCheckpointIdentity&)const;
 void bind_child(const std::shared_ptr<BnwPlotYieldRefreshPending>&);
 void require_exact_child(const BnwPlotYieldRefreshPending&)const;
 void accept_child_return(const BnwPlotYieldRefreshPending&);
 void require_entry_child_prefix(const BnwCityEntryPending&,const BnwPlotYieldRefreshPending&)const;
 void require_completed_for_entry(const BnwCityEntryPending&)const;
 void require_population_child_prefix(const BnwCityPopulationPending&,const BnwPlotYieldRefreshPending&)const;
 void require_completed_for_population(const BnwCityPopulationPending&)const;
 void require_population_registration(const BnwCityPopulationPending&,const LiveSession&)const;
 void invalidate()noexcept;
};
// An external handle retains the real Host. Session -> Pending has no strong
// Host/facade backedge, and no callback can acknowledge a child as completed.
class BnwCityYieldRefresh final {
 public:
 static std::shared_ptr<BnwCityYieldRefresh> from_selected_host(
  std::shared_ptr<ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
 ~BnwCityYieldRefresh();
 BnwCityYieldRefresh(const BnwCityYieldRefresh&)=delete;
 BnwCityYieldRefresh& operator=(const BnwCityYieldRefresh&)=delete;
 void advance();
 BnwCityYieldRefreshProgress progress()const;
 private:
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwCityYieldRefresh(std::unique_ptr<Impl>)noexcept;
};
// Registry/checkpoint have no public construction or restore/apply permission.
class BnwCityYieldRefreshRegistry final {
 private:
 friend class BnwCityFoodGrowthPending;friend class LiveSession;friend class BnwCityYieldRefreshPending;
 struct Impl;std::shared_ptr<Impl> impl_;
 BnwCityYieldRefreshRegistry();
 bool has_history()const;
 void retire()noexcept;
 std::shared_ptr<const BnwCityYieldRefreshCheckpoint> checkpoint()const;
 static std::shared_ptr<BnwCityYieldRefreshRegistry> restore_quarantined(
  const std::shared_ptr<const BnwCityYieldRefreshCheckpoint>&,
  const std::shared_ptr<BnwCityYieldRefreshRegistry>&);
 void commit_quarantine()noexcept;
};
class BnwCityYieldRefreshCheckpoint final {
 private:
 friend class BnwCityYieldRefreshRegistry;
 struct Impl;std::shared_ptr<const Impl> impl_;
 explicit BnwCityYieldRefreshCheckpoint(std::shared_ptr<const Impl>)noexcept;
};
}
