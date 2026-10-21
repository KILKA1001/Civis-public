#include "civ5/ui/bnw_selected_notification_backend.hpp"
#include "civ5/ui/world_view.hpp"
#include <algorithm>
#include <utility>

namespace civ5::ui {

BnwSelectedNotificationBackend::BnwSelectedNotificationBackend(
    runtime::LiveSession &session, const runtime::ProductionProgressRules &rules,
    runtime::BnwProductionOrderServices production_services,
    BnwProductionPreview production_preview,
    BnwNotificationPopupOwner &notification_owner,
    BnwNotificationActivationServices notification_services,
    const content::RulesDatabase *database, content::InfoLocalizer localize)
    : BnwCityProductionUiBackend(session, rules, std::move(production_services),
                                 std::move(production_preview), database,
                                 std::move(localize)),
      session_(&session), notification_owner_(&notification_owner),
      notification_services_(std::move(notification_services)) {}

UiCommandResult BnwSelectedNotificationBackend::availability(
    const UiGameplayCommand &command, const UiFrameSnapshot &frame) const {
    const auto *action = command.as<WorldViewAction>();
    if (!action || action->kind != WorldViewActionKind::notification)
        return BnwCityProductionUiBackend::availability(command, frame);
    if (command.source_id() != "ActionInfoPanel" || !frame.active_player_id() ||
        !frame.turn() || *frame.active_player_id() != notification_owner_->player_id() ||
        session_->state().setup.game_turn != *frame.turn())
        return {UiCommandStatus::unavailable,
                "Notification click does not belong to the current live player and turn"};
    const auto *world = frame.find<WorldViewState>();
    const auto current = notification_owner_->active_notifications();
    if (!world || world->player_id != *frame.active_player_id() || !current ||
        current->player_id != *frame.active_player_id() ||
        current->turn != *frame.turn() ||
        std::none_of(world->notifications.begin(), world->notifications.end(),
            [&](const WorldViewNotification &notification) {
                return notification.id == action->id && notification.requires_decision &&
                    std::any_of(current->events.begin(), current->events.end(),
                        [&](const WorldViewNotification &live) {
                            return live.id == notification.id &&
                                   live.text == notification.text;
                        });
            }))
        return {UiCommandStatus::unavailable,
                "Technology notification is absent from the current live frame"};
    if (const auto reason = notification_owner_->tech_unavailable_reason(
            action->id, *frame.turn(), notification_services_))
        return {UiCommandStatus::unavailable, *reason};
    return {UiCommandStatus::ready, {}};
}

UiCommandResult BnwSelectedNotificationBackend::execute(
    const UiGameplayCommand &command, const UiFrameSnapshot &frame) {
    const auto *action = command.as<WorldViewAction>();
    if (!action || action->kind != WorldViewActionKind::notification)
        return BnwCityProductionUiBackend::execute(command, frame);
    const auto available = availability(command, frame);
    if (available.status != UiCommandStatus::ready) return available;
    const auto result = notification_owner_->activate_tech(
        action->id, *frame.turn(), notification_services_);
    return {result.status == BnwNotificationActivationStatus::published
                ? UiCommandStatus::performed : UiCommandStatus::unavailable,
            result.reason};
}

} // namespace civ5::ui
