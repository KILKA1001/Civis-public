#include "civ5/ui/economic_overview_gold_input.hpp"

namespace civ5::ui {
namespace {
bool valid(const OverviewRect &r, std::int32_t width, std::int32_t height) {
    return width > 0 && height > 0 && r.x >= 0 && r.y >= 0 &&
        r.width > 0 && r.height > 0 &&
        std::int64_t(r.x) + r.width <= width &&
        std::int64_t(r.y) + r.height <= height;
}

bool contains(const OverviewRect &r, std::int32_t x, std::int32_t y) {
    return x >= r.x && y >= r.y &&
        std::int64_t(x) < std::int64_t(r.x) + r.width &&
        std::int64_t(y) < std::int64_t(r.y) + r.height;
}
} // namespace

EconomicOverviewGoldInput::EconomicOverviewGoldInput(
    const runtime::LiveSession &session, const UiScreenController &screens,
    EconomicOverviewOpenBridge &bridge,
    const EconomicOverviewLifecycle &lifecycle)
    : session_(&session), screens_(&screens), bridge_(&bridge),
      lifecycle_(&lifecycle) {}

EconomicGoldInputResult EconomicOverviewGoldInput::click(
    const runtime::LiveSession &session, const LiveUiFrame &frame,
    const EconomicGoldInputMeasurement &measurement,
    std::int32_t viewport_width, std::int32_t viewport_height,
    std::int32_t press_x, std::int32_t press_y,
    std::int32_t release_x, std::int32_t release_y) {
    EconomicGoldInputResult result;
    const auto &m = measurement.click;
    const auto &r = m.gold_button;
    // Validate against the viewport in which the presenter measured this
    // control. A later resize may clip it, but visible old pixels still belong
    // to UI input and must not leak to the camera.
    if (!valid(r, m.viewport_width, m.viewport_height) ||
        viewport_width <= 0 || viewport_height <= 0) {
        result.blocker = "Measured GoldPerTurn rectangle is unavailable";
        return result;
    }
    const auto on_screen = [&](std::int32_t x, std::int32_t y) {
        return x >= 0 && y >= 0 && x < viewport_width &&
            y < viewport_height;
    };
    const bool pressed = on_screen(press_x, press_y) &&
        contains(r, press_x, press_y);
    const bool released = on_screen(release_x, release_y) &&
        contains(r, release_x, release_y);
    result.consumed = pressed || released;
    if (!result.consumed) return result;
    if (!pressed || !released) {
        result.blocker = "GoldPerTurn press and release must remain on one measured control";
        return result;
    }
    if (&session != session_ || !frame.owner || !frame.sequence ||
        measurement.frame_owner != frame.owner ||
        measurement.frame_sequence != frame.sequence ||
        frame.snapshot.frame() != m.frame ||
        frame.snapshot.active_player_id() != m.player_id ||
        frame.snapshot.turn() != m.turn ||
        m.turn != session.state().setup.game_turn ||
        m.viewport_width != viewport_width ||
        m.viewport_height != viewport_height ||
        !frame.top_panel || !frame.top_panel->show_info_stack ||
        !frame.top_panel->gold_text || frame.top_panel->gold_text->empty()) {
        result.blocker = "GoldPerTurn live frame, owner or viewport is stale";
        return result;
    }
    if (const auto focused = screens_->focused_screen()) {
        const auto *context = screens_->registry().find(*focused);
        if (context && context->modal) {
            result.blocker = "A modal screen owns GoldPerTurn input";
            return result;
        }
    }
    const auto queued = bridge_->click(session, frame.snapshot, m,
                                       release_x, release_y);
    result.status = queued.status;
    result.blocker = queued.blocker;
    if (queued.status != EconomicPopupQueueStatus::queued) return result;
    const auto popup = lifecycle_->active_popups(session);
    if (!popup || popup->player_id != m.player_id ||
        popup->turn != m.turn || popup->owner_sequence != m.owner_sequence ||
        popup->events.size() != 1 ||
        popup->events.front().context_id != "EconomicOverview" ||
        popup->events.front().event_id != m.event_id) {
        result.status = EconomicPopupQueueStatus::uncertain;
        result.blocker = "GoldPerTurn effect did not publish its current popup owner";
        return result;
    }
    result.popup = *popup;
    return result;
}

} // namespace civ5::ui
