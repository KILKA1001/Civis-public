#include "civ5/ui/economic_overview_popup_feed.hpp"

namespace civ5::ui {

EconomicPopupFeedResult project_economic_popup_frame(const PopupFrame &popup) {
    if (popup.player_id < 0 || popup.turn < 0 || popup.owner_sequence == 0 ||
        popup.events.size() != 1 ||
        popup.events.front().context_id != "EconomicOverview" ||
        popup.events.front().event_id < 0)
        return {std::nullopt,
            "EconomicOverview PopupFrame lacks a unique current player, turn, owner sequence or event ID"};
    return {LivePopupEventFeed{popup.player_id, popup.turn,
        popup.owner_sequence, popup.events}, {}};
}

EconomicPopupFeedResult capture_economic_overview_popup_feed(
    const runtime::LiveSession &session, const EconomicOverviewLifecycle &owner,
    const UiFrameSnapshot *frame, std::int32_t viewport_width,
    std::int32_t viewport_height) {
    const auto popup = owner.active_popups(session);
    if (!popup)
        return {std::nullopt,
            "No current EconomicOverview popup for this live session"};
    auto result = project_economic_popup_frame(*popup);
    if (!result.feed) return result;
    if (popup->turn != session.state().setup.game_turn)
        return {std::nullopt, "EconomicOverview popup turn is stale"};
    if (frame) {
        const auto current = owner.current_owner(session, *frame,
            viewport_width, viewport_height);
        if (!current || current->player_id != popup->player_id ||
            current->turn != popup->turn ||
            current->owner_sequence != popup->owner_sequence ||
            current->event_id != popup->events.front().event_id ||
            current->lua_sha256.empty() || current->xml_path.empty())
            return {std::nullopt,
                "EconomicOverview frame, viewport or selected source owner is stale"};
    }
    return result;
}

std::function<std::optional<LivePopupEventFeed>(const runtime::LiveSession &)>
make_economic_overview_popup_feed_provider(
    const runtime::LiveSession &session, const EconomicOverviewLifecycle &owner) {
    return [&session, &owner](const runtime::LiveSession &requested)
        -> std::optional<LivePopupEventFeed> {
        if (&requested != &session) return std::nullopt;
        return capture_economic_overview_popup_feed(requested, owner).feed;
    };
}

} // namespace civ5::ui
