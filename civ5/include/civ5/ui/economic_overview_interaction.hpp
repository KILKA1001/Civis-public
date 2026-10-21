#pragma once

#include "civ5/ui/economic_overview_open_bridge.hpp"

namespace civ5::ui {

// Civis-owned local interaction path. The renderer/presenter still owns
// measurement and delivery of GoldPerTurn and CloseButton input.
class EconomicOverviewInteraction {
  public:
    EconomicOverviewInteraction(const runtime::LiveSession &session,
        std::int64_t active_player_id, EconomicOverviewLifecycle &lifecycle,
        EconomicGoldSourceProof open_source,
        EconomicCloseSourceProof close_source);

    EconomicGoldClickResult gold_click(const runtime::LiveSession &source_session,
        const UiFrameSnapshot &frame, const EconomicGoldClick &measurement,
        std::int32_t x, std::int32_t y);

    EconomicCloseResult close(const runtime::LiveSession &source_session,
        const UiFrameSnapshot &frame, const EconomicCloseMeasurement &measurement,
        EconomicCloseInput input, std::int32_t x, std::int32_t y);

  private:
    const runtime::LiveSession *session_;
    EconomicOverviewLifecycle *lifecycle_;
    EconomicOverviewOpenBridge open_;
    EconomicCloseSourceProof close_source_;
    EconomicOverviewClose close_;
    bool sources_match_ = false;
};

} // namespace civ5::ui
