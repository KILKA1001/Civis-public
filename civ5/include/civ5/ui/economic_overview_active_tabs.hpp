#pragma once

#include "civ5/ui/economic_overview_lifecycle.hpp"

namespace civ5::ui {

// Binds the source-authored tab callbacks to the currently queued Civis popup.
// The presenter owns measured rectangles; the live gameplay owner supplies
// GAMEOPTION_NO_HAPPINESS. General remains usable when that option is unknown.
class EconomicOverviewActiveTabs {
  public:
    EconomicOverviewActiveTabs(EconomicOverviewLifecycle &lifecycle,
        EconomicTabSourceProof tab_source,
        EconomicCloseSourceProof popup_source);

    bool bind(const runtime::LiveSession &session, const UiFrameSnapshot &frame,
        const EconomicTabMeasurement &measurement,
        std::optional<bool> happiness_enabled, std::string &blocker);
    EconomicTabView view(const runtime::LiveSession &session,
        const UiFrameSnapshot &frame) const;
    EconomicTabClick click(const runtime::LiveSession &session,
        const UiFrameSnapshot &frame, std::int32_t x, std::int32_t y);

  private:
    std::optional<EconomicTabOwner> current(
        const runtime::LiveSession &session,
        const UiFrameSnapshot &frame) const;

    EconomicOverviewLifecycle *lifecycle_;
    EconomicTabSourceProof source_;
    EconomicOverviewTabs tabs_;
    std::optional<EconomicTabMeasurement> measurement_;
    bool sources_match_ = false;
};

} // namespace civ5::ui
