#pragma once

#include "civ5/ui/economic_overview_open_bridge.hpp"
#include "civ5/ui/live_ui_host.hpp"

namespace civ5::ui {

struct EconomicGoldInputMeasurement {
    EconomicGoldClick click;
    const void *frame_owner = nullptr;
    std::uint64_t frame_sequence = 0;
};

struct EconomicGoldInputResult {
    bool consumed = false;
    EconomicPopupQueueStatus status = EconomicPopupQueueStatus::blocked;
    std::optional<PopupFrame> popup;
    std::string blocker;
};

// Receives physical-pixel press/release from the presenter for its measured
// GoldPerTurn control. The renderer remains responsible for all other hits.
class EconomicOverviewGoldInput {
  public:
    EconomicOverviewGoldInput(const runtime::LiveSession &session,
        const UiScreenController &screens, EconomicOverviewOpenBridge &bridge,
        const EconomicOverviewLifecycle &lifecycle);

    EconomicGoldInputResult click(const runtime::LiveSession &session,
        const LiveUiFrame &frame, const EconomicGoldInputMeasurement &measurement,
        std::int32_t viewport_width, std::int32_t viewport_height,
        std::int32_t press_x, std::int32_t press_y,
        std::int32_t release_x, std::int32_t release_y);

  private:
    const runtime::LiveSession *session_;
    const UiScreenController *screens_;
    EconomicOverviewOpenBridge *bridge_;
    const EconomicOverviewLifecycle *lifecycle_;
};

} // namespace civ5::ui
