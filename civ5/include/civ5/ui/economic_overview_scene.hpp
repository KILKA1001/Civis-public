#pragma once

#include "civ5/ui/economic_overview_fields.hpp"
#include "civ5/ui/economic_overview_gold_input.hpp"
#include "civ5/ui/economic_overview_live_input.hpp"
#include "civ5/ui/economic_overview_popup_feed.hpp"

namespace civ5::ui {

// Owns the selected BNW EconomicOverview source proofs and local UI lifecycle.
// The caller supplies the SAME UiScreenController and LiveSession as its frame
// provider; this object must outlive the callback installed into sources.
class EconomicOverviewScene {
  public:
    EconomicOverviewScene(const runtime::LiveSession &session,
        std::int64_t active_player_id, UiScreenController &screens,
        const assets::ResourceMountTable &resources,
        EconomicPopupProcessedSink processed_sink = {});
    EconomicOverviewScene(const EconomicOverviewScene &) = delete;
    EconomicOverviewScene &operator=(const EconomicOverviewScene &) = delete;
    EconomicOverviewScene(EconomicOverviewScene &&) = delete;
    EconomicOverviewScene &operator=(EconomicOverviewScene &&) = delete;

    // A different popup provider must be composed by its owner; overwriting it
    // here would silently remove live modal events.
    bool install_popup_feed(LiveSessionUiSources &sources,
                            std::string &blocker) const;
    EconomicGoldInputResult gold_click(const runtime::LiveSession &session,
        const LiveUiFrame &frame, const EconomicGoldInputMeasurement &measurement,
        std::int32_t viewport_width, std::int32_t viewport_height,
        std::int32_t press_x, std::int32_t press_y,
        std::int32_t release_x, std::int32_t release_y);
    std::optional<EconomicTabOwner> current_owner(
        const runtime::LiveSession &session, const LiveUiFrame &frame,
        std::int32_t viewport_width, std::int32_t viewport_height) const;
    bool bind_popup(const runtime::LiveSession &session,
        const LiveUiFrame &frame, const EconomicOverviewInputRects &rects,
        std::optional<bool> happiness_enabled, std::string &blocker);
    EconomicOverviewInputResult popup_click(const runtime::LiveSession &session,
        const LiveUiFrame &frame, std::int32_t press_x, std::int32_t press_y,
        std::int32_t release_x, std::int32_t release_y);
    EconomicBoundFields fields(const runtime::LiveSession &session,
        const LiveUiFrame &frame, std::int32_t viewport_width,
        std::int32_t viewport_height) const;
    EconomicTabView tab_view(const runtime::LiveSession &session,
        const LiveUiFrame &frame) const;

    const EconomicOverviewLifecycle &lifecycle() const { return lifecycle_; }
    const EconomicGoldSourceProof &gold_source() const { return gold_source_; }
    const EconomicCloseSourceProof &close_source() const { return close_source_; }
    const EconomicTabSourceProof &tab_source() const { return tab_source_; }
    const EconomicFieldSourceProof &field_source() const { return field_source_; }

  private:
    static OverviewSource selected_overview(
        const assets::ResourceMountTable &resources,
        const UiContextRegistry &registry);

    const runtime::LiveSession *session_;
    OverviewSource overview_;
    EconomicGoldSourceProof gold_source_;
    EconomicTabSourceProof tab_source_;
    EconomicCloseSourceProof close_source_;
    EconomicFieldSourceProof field_source_;
    EconomicOverviewLifecycle lifecycle_;
    EconomicOverviewOpenBridge open_bridge_;
    EconomicOverviewInteraction interaction_;
    EconomicOverviewActiveTabs tabs_;
    EconomicOverviewFields fields_;
    EconomicOverviewGoldInput gold_input_;
    EconomicOverviewLiveInput popup_input_;
};

} // namespace civ5::ui
