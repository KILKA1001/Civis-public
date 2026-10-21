#include "civ5/ui/economic_overview_live_input.hpp"

namespace civ5::ui {
namespace {

bool same_owner(const EconomicTabOwner &a, const EconomicTabOwner &b) {
    return a.player_id == b.player_id && a.event_id == b.event_id &&
        a.turn == b.turn && a.frame == b.frame &&
        a.owner_sequence == b.owner_sequence &&
        a.lua_sha256 == b.lua_sha256 && a.xml_path == b.xml_path &&
        a.viewport_width == b.viewport_width &&
        a.viewport_height == b.viewport_height;
}

bool valid(const OverviewRect &r, const EconomicTabOwner &owner) {
    return r.x >= 0 && r.y >= 0 && r.width > 0 && r.height > 0 &&
        std::int64_t(r.x) + r.width <= owner.viewport_width &&
        std::int64_t(r.y) + r.height <= owner.viewport_height;
}

bool overlaps(const OverviewRect &a, const OverviewRect &b) {
    return std::int64_t(a.x) < std::int64_t(b.x) + b.width &&
        std::int64_t(b.x) < std::int64_t(a.x) + a.width &&
        std::int64_t(a.y) < std::int64_t(b.y) + b.height &&
        std::int64_t(b.y) < std::int64_t(a.y) + a.height;
}

bool contains(const OverviewRect &r, std::int32_t x, std::int32_t y) {
    return x >= r.x && y >= r.y &&
        std::int64_t(x) < std::int64_t(r.x) + r.width &&
        std::int64_t(y) < std::int64_t(r.y) + r.height;
}

bool published_in_frame(const UiFrameSnapshot &frame,
    const EconomicTabOwner &owner) {
    const auto *popup = frame.find<PopupFrame>();
    return popup && popup->player_id == owner.player_id &&
        popup->turn == owner.turn &&
        popup->owner_sequence == owner.owner_sequence &&
        popup->events.size() == 1 &&
        popup->events.front().context_id == "EconomicOverview" &&
        popup->events.front().event_id == owner.event_id;
}

EconomicOverviewInputAction hit(const EconomicOverviewInputRects &rects,
    std::int32_t x, std::int32_t y) {
    if (contains(rects.general_button, x, y))
        return EconomicOverviewInputAction::general;
    if (contains(rects.happiness_button, x, y))
        return EconomicOverviewInputAction::happiness;
    if (contains(rects.close_button, x, y))
        return EconomicOverviewInputAction::close;
    return EconomicOverviewInputAction::none;
}

} // namespace

EconomicOverviewLiveInput::EconomicOverviewLiveInput(
    const runtime::LiveSession &session, EconomicOverviewLifecycle &lifecycle,
    EconomicOverviewActiveTabs &tabs, EconomicOverviewInteraction &interaction)
    : session_(&session), lifecycle_(&lifecycle), tabs_(&tabs),
      interaction_(&interaction) {}

bool EconomicOverviewLiveInput::bind(const runtime::LiveSession &session,
    const LiveUiFrame &frame, const EconomicOverviewInputRects &rects,
    std::optional<bool> happiness_enabled, std::string &blocker) {
    rects_.reset();
    frame_owner_ = nullptr;
    frame_sequence_ = snapshot_frame_ = 0;
    const auto current = lifecycle_->current_owner(session, frame.snapshot,
        rects.owner.viewport_width, rects.owner.viewport_height);
    if (&session != session_ || !frame.owner || !frame.sequence ||
        !current || !same_owner(*current, rects.owner) ||
        !published_in_frame(frame.snapshot, rects.owner) ||
        !valid(rects.general_button, rects.owner) ||
        !valid(rects.happiness_button, rects.owner) ||
        !valid(rects.close_button, rects.owner) ||
        overlaps(rects.general_button, rects.happiness_button) ||
        overlaps(rects.general_button, rects.close_button) ||
        overlaps(rects.happiness_button, rects.close_button)) {
        blocker = "Current EconomicOverview frame or nonoverlapping measured controls are unavailable";
        return false;
    }
    if (!tabs_->bind(session, frame.snapshot,
            EconomicTabMeasurement{rects.owner, rects.general_button,
                                   rects.happiness_button},
            happiness_enabled, blocker))
        return false;
    rects_ = rects;
    frame_owner_ = frame.owner;
    frame_sequence_ = frame.sequence;
    snapshot_frame_ = frame.snapshot.frame();
    blocker.clear();
    return true;
}

EconomicOverviewInputResult EconomicOverviewLiveInput::click(
    const runtime::LiveSession &session, const LiveUiFrame &frame,
    std::int32_t press_x, std::int32_t press_y,
    std::int32_t release_x, std::int32_t release_y) {
    EconomicOverviewInputResult result;
    if (&session != session_) {
        result.blocker = "EconomicOverview input belongs to another session";
        return result;
    }
    if (!lifecycle_->active_popups(session)) {
        result.blocker = "EconomicOverview popup is closed or stale";
        return result;
    }
    result.consumed = true;
    if (!rects_ || !frame.owner || frame.owner != frame_owner_ ||
        !frame.sequence || frame.sequence != frame_sequence_ ||
        frame.snapshot.frame() != snapshot_frame_ ||
        !lifecycle_->current_owner(session, frame.snapshot,
            rects_ ? rects_->owner.viewport_width : 0,
            rects_ ? rects_->owner.viewport_height : 0) ||
        !published_in_frame(frame.snapshot, rects_->owner) ||
        !tabs_->view(session, frame.snapshot).available) {
        result.blocker = "EconomicOverview input frame or measured owner is stale";
        return result;
    }
    const auto pressed = hit(*rects_, press_x, press_y);
    const auto released = hit(*rects_, release_x, release_y);
    if (pressed == EconomicOverviewInputAction::none || pressed != released) {
        result.blocker = "EconomicOverview press and release must match one measured control";
        return result;
    }
    result.action = pressed;
    if (pressed == EconomicOverviewInputAction::close) {
        const auto closed = interaction_->close(session, frame.snapshot,
            EconomicCloseMeasurement{rects_->owner, rects_->close_button},
            EconomicCloseInput::button_click, release_x, release_y);
        result.blocker = closed.blocker;
        if (closed.status == EconomicCloseStatus::performed) {
            result.status = EconomicOverviewInputStatus::performed;
            rects_.reset();
        } else if (closed.status == EconomicCloseStatus::uncertain)
            result.status = EconomicOverviewInputStatus::uncertain;
        return result;
    }
    const auto tab = tabs_->click(session, frame.snapshot,
                                  release_x, release_y);
    result.blocker = tab.blocker;
    if (tab.performed) result.status = EconomicOverviewInputStatus::performed;
    return result;
}

} // namespace civ5::ui
