#pragma once
#include "civ5/runtime/bnw_plot_yield_refresh.hpp"
#include "civ5/runtime/bnw_plot_gameplay_event.hpp"
#include "civ5/ui/live_ui_host.hpp"
#include <memory>
#include <vector>
namespace civ5::ui {
class SelectedBnwPreviewHost;
class BnwPlotYieldRefreshOwnedUiContext;class BnwPlotYieldRefreshOwnedUiStorage;class BnwClientSceneFrame;
struct BnwPlotYieldRefreshUiAdvance {
 runtime::BnwPlotYieldRefreshProgress progress;
 std::optional<LiveUiFrame> refreshed_ui;
 std::uint64_t dirty_deliveries=0;
};
// Actual gameplay/UI event receiver. No supplied selection/ack/callback.
// Does not retain itself from Session and does not own a renderer capability.
class BnwPlotYieldRefreshUiAdapter final {
 public:
 static std::shared_ptr<BnwPlotYieldRefreshUiAdapter> from_selected_host(
  std::shared_ptr<const SelectedBnwPreviewHost>);
 ~BnwPlotYieldRefreshUiAdapter();
 BnwPlotYieldRefreshUiAdapter(const BnwPlotYieldRefreshUiAdapter&)=delete;
 BnwPlotYieldRefreshUiAdapter& operator=(const BnwPlotYieldRefreshUiAdapter&)=delete;
 void require_current()const;
 void retire()noexcept;
 // Only the private owned Context factory supplies Scene/provider authority.
 // Authenticates the privately issued bundle before native source callbacks.
 // Factual state events remain queued; symbols is an unacknowledged boundary.
 BnwPlotYieldRefreshUiAdvance advance(
  const std::shared_ptr<const runtime::BnwPlotYieldRefreshPending>&,
  const LiveUiFrame&);
 static constexpr bool symbols_listener_ready()noexcept{return false;}
 static constexpr bool full_city_refresh_ready()noexcept{return false;}
 // Exact registered latest cursor; predicate and its true dirty publication
 // run synchronously. A symbols publication is another native phase call.
 runtime::BnwPlotYieldRefreshProgress continue_plot(std::int64_t plot);
 // Move factual notifications to the consumer; this never acknowledges
 // rendering. Restored old-generation queues cannot silently be re-emitted.
 std::vector<runtime::BnwPlotGameplayEvent> take_plot_events();
 private:
 friend class BnwPlotYieldRefreshOwnedUiContext;
 struct Impl;struct Call;std::shared_ptr<Impl> impl_;
 explicit BnwPlotYieldRefreshUiAdapter(std::shared_ptr<Impl>)noexcept;
 static std::shared_ptr<BnwPlotYieldRefreshUiAdapter> from_owned_context(
  std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage>,
  std::shared_ptr<BnwClientSceneFrame>,std::shared_ptr<LiveUiHost>);
 static void require_current(const std::shared_ptr<Impl>&);
 static runtime::BnwPlotYieldRefreshProgress continue_plot(const std::shared_ptr<Impl>&,std::int64_t);
};
}
