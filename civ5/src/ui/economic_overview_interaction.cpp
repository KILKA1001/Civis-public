#include "civ5/ui/economic_overview_interaction.hpp"

namespace civ5::ui {

EconomicOverviewInteraction::EconomicOverviewInteraction(
    const runtime::LiveSession &session, std::int64_t active_player_id,
    EconomicOverviewLifecycle &lifecycle, EconomicGoldSourceProof open_source,
    EconomicCloseSourceProof close_source)
    : session_(&session), lifecycle_(&lifecycle),
      open_(session, active_player_id, lifecycle, open_source),
      close_source_(std::move(close_source)),
      sources_match_(open_source.verified && close_source_.verified &&
          !open_source.economic_lua_sha256.empty() &&
          open_source.economic_lua_sha256 == close_source_.lua_sha256) {}

EconomicGoldClickResult EconomicOverviewInteraction::gold_click(
    const runtime::LiveSession &source_session, const UiFrameSnapshot &frame,
    const EconomicGoldClick &measurement, std::int32_t x, std::int32_t y) {
    if (!sources_match_)
        return {false, EconomicPopupQueueStatus::blocked,
                "Gold and EconomicOverview listener sources do not match"};
    return open_.click(source_session, frame, measurement, x, y);
}

EconomicCloseResult EconomicOverviewInteraction::close(
    const runtime::LiveSession &source_session, const UiFrameSnapshot &frame,
    const EconomicCloseMeasurement &measurement, EconomicCloseInput input,
    std::int32_t x, std::int32_t y) {
    EconomicCloseResult unavailable;
    if (!sources_match_ || session_ != &source_session) {
        unavailable.blocker = "EconomicOverview close source or session is unavailable";
        return unavailable;
    }
    const auto popups = lifecycle_->active_popups(source_session);
    if (!popups) {
        unavailable.blocker = "No current EconomicOverview popup event";
        return unavailable;
    }
    const auto owner = lifecycle_->current_owner(source_session, frame,
        measurement.owner.viewport_width, measurement.owner.viewport_height);
    if (!owner) {
        unavailable.blocker = "EconomicOverview owner frame is stale";
        return unavailable;
    }
    std::string blocker;
    if (!close_.bind(frame, *popups, close_source_, measurement, true, blocker)) {
        unavailable.blocker = std::move(blocker);
        return unavailable;
    }
    return close_.activate(*owner, input, x, y,
        lifecycle_->close_service(source_session, frame,
            measurement.owner.viewport_width,
            measurement.owner.viewport_height));
}

} // namespace civ5::ui
