#include "civ5/ui/economic_overview_active_tabs.hpp"

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
} // namespace

EconomicOverviewActiveTabs::EconomicOverviewActiveTabs(
    EconomicOverviewLifecycle &lifecycle, EconomicTabSourceProof tab_source,
    EconomicCloseSourceProof popup_source)
    : lifecycle_(&lifecycle), source_(std::move(tab_source)),
      sources_match_(source_.verified && popup_source.verified &&
          !source_.lua_sha256.empty() &&
          source_.lua_sha256 == popup_source.lua_sha256 &&
          source_.xml_path == popup_source.xml_path) {}

std::optional<EconomicTabOwner> EconomicOverviewActiveTabs::current(
    const runtime::LiveSession &session, const UiFrameSnapshot &frame) const {
    if (!sources_match_ || !measurement_) return std::nullopt;
    const auto owner = lifecycle_->current_owner(session, frame,
        measurement_->owner.viewport_width,
        measurement_->owner.viewport_height);
    if (!owner || !same_owner(*owner, measurement_->owner))
        return std::nullopt;
    return owner;
}

bool EconomicOverviewActiveTabs::bind(const runtime::LiveSession &session,
    const UiFrameSnapshot &frame, const EconomicTabMeasurement &measurement,
    std::optional<bool> happiness_enabled, std::string &blocker) {
    measurement_.reset();
    if (!sources_match_) {
        blocker = "EconomicOverview tab and popup sources disagree";
        return false;
    }
    const auto popups = lifecycle_->active_popups(session);
    const auto owner = lifecycle_->current_owner(session, frame,
        measurement.owner.viewport_width, measurement.owner.viewport_height);
    if (!popups || !owner || !same_owner(*owner, measurement.owner)) {
        blocker = "Current EconomicOverview popup owner is unavailable";
        return false;
    }
    if (!tabs_.bind(frame, *popups, source_, measurement,
                    happiness_enabled, blocker))
        return false;
    measurement_ = measurement;
    return true;
}

EconomicTabView EconomicOverviewActiveTabs::view(
    const runtime::LiveSession &session, const UiFrameSnapshot &frame) const {
    const auto owner = current(session, frame);
    if (!owner) {
        EconomicTabView unavailable;
        unavailable.blocker = "EconomicOverview tab owner is stale or absent";
        return unavailable;
    }
    return tabs_.view(*owner);
}

EconomicTabClick EconomicOverviewActiveTabs::click(
    const runtime::LiveSession &session, const UiFrameSnapshot &frame,
    std::int32_t x, std::int32_t y) {
    const auto owner = current(session, frame);
    if (!owner) {
        EconomicTabClick unavailable;
        unavailable.blocker = "EconomicOverview tab owner is stale or absent";
        return unavailable;
    }
    return tabs_.click(*owner, x, y);
}

} // namespace civ5::ui
