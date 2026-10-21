#pragma once

#include "civ5/ui/economic_overview_lifecycle.hpp"
#include "civ5/ui/live_session_frame.hpp"

namespace civ5::ui {

struct EconomicPopupFeedResult {
    std::optional<LivePopupEventFeed> feed;
    std::string blocker;
};

// Only the current lifecycle owner may publish this event. A saved popup slot
// and screen visibility alone are insufficient. The optional frame check is
// for consumers that also need viewport-bound current_owner evidence; the
// LiveSessionUiSources callback runs before its new frame exists.
EconomicPopupFeedResult project_economic_popup_frame(const PopupFrame &popup);
EconomicPopupFeedResult capture_economic_overview_popup_feed(
    const runtime::LiveSession &session, const EconomicOverviewLifecycle &owner,
    const UiFrameSnapshot *frame = nullptr,
    std::int32_t viewport_width = 0, std::int32_t viewport_height = 0);

// The caller keeps session and owner alive for the lifetime of this callback.
// It can be assigned to LiveSessionUiSources::active_popups for a session whose
// only popup source is this EconomicOverview owner.
std::function<std::optional<LivePopupEventFeed>(const runtime::LiveSession &)>
make_economic_overview_popup_feed_provider(
    const runtime::LiveSession &session, const EconomicOverviewLifecycle &owner);

} // namespace civ5::ui
