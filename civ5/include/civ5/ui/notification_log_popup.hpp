#pragma once

#include "civ5/ui/popup_screens.hpp"
#include <algorithm>
#include <limits>
#include <set>

namespace civ5::ui {

// The live Player notification-log owner supplies historical rows in the
// selected Lua order (newest first). WorldViewState.notifications is a current
// notification list and must not be converted into this history.
struct NotificationLogRecord {
    std::int64_t notification_id = -1;
    std::int32_t turn = -1;
    std::string description, localized_turn;
    bool dismissed = false;
    // Physical client pixels measured after the native dynamic TextStack row
    // height and scroll clipping. Only fully visible rectangles are supplied.
    WorldViewRect visible_bounds;
};

struct NotificationLogSnapshot {
    bool owner_provided = false;
    std::int64_t player_id = -1, event_id = -1;
    std::int32_t turn = -1;
    std::uint64_t owner_sequence = 0, frame_number = 0;
    std::string selected_source_path;
    // The live action owner explicitly binds the backend that can activate a
    // historical entry. An unrelated shared gameplay backend is insufficient.
    const UiGameplayBackend *activation_backend_owner = nullptr;
    std::vector<NotificationLogRecord> visible_rows;
};

struct NotificationLogActivate {
    std::uint64_t owner_sequence = 0;
    std::int64_t event_id = -1, notification_id = -1;
};

inline UiCommand notification_log_activate(const NotificationLogSnapshot &owner,
                                            std::int64_t notification_id) {
    return UiGameplayCommand::make("NotificationLogPopup",
        NotificationLogActivate{owner.owner_sequence, owner.event_id, notification_id});
}

// This fallback never acknowledges an entry. The live UI.ActivateNotification
// equivalent must be supplied by a separate backend before activation is ready.
class NotificationLogUnavailableBackend final : public UiGameplayBackend {
  public:
    UiCommandResult availability(const UiGameplayCommand &command,
                                 const UiFrameSnapshot &frame) const override {
        const auto *action = command.as<NotificationLogActivate>();
        const auto *owner = frame.find<NotificationLogSnapshot>();
        if (command.source_id() != "NotificationLogPopup" || !action ||
            action->notification_id < 0 || !owner || !owner->owner_provided ||
            action->owner_sequence != owner->owner_sequence ||
            action->event_id != owner->event_id)
            return {UiCommandStatus::invalid, "Invalid notification-log owner or action"};
        return {UiCommandStatus::unavailable,
                "Live notification activation backend is unavailable"};
    }
    UiCommandResult execute(const UiGameplayCommand &command,
                            const UiFrameSnapshot &frame) override {
        return availability(command, frame);
    }
};

namespace notification_log_detail {

inline const content::XmlNode *find_id(const content::XmlNode &node,
                                       std::string_view id) {
    if (node.attribute("ID") == id) return &node;
    for (const auto &child : node.children)
        if (const auto *result = find_id(child, id)) return result;
    return nullptr;
}

inline const content::XmlNode *parent_of(const content::XmlNode &node,
                                         std::string_view id) {
    for (const auto &child : node.children) {
        if (child.attribute("ID") == id) return &node;
        if (const auto *result = parent_of(child, id)) return result;
    }
    return nullptr;
}

inline const content::XmlNode *panel(const content::XmlNode &document) {
    for (const auto &child : document.children)
        if (child.name == "Grid" && child.attribute("Size") == "960,658" &&
            child.attribute("Anchor") == "C,C" &&
            child.attribute("Offset") == "0,36" &&
            child.attribute("Style") == "Grid9DetailFive140") return &child;
    return nullptr;
}

inline const content::XmlNode *row_instance(const content::XmlNode &document) {
    for (const auto &child : document.children)
        if (child.name == "Instance" &&
            child.attribute("Name") == "NotificationButton") return &child;
    return nullptr;
}

inline bool inside(WorldViewRect outer, WorldViewRect inner) {
    return inner.width > 0 && inner.height > 0 && inner.x >= outer.x &&
        inner.y >= outer.y &&
        std::int64_t(inner.x) + inner.width <= std::int64_t(outer.x) + outer.width &&
        std::int64_t(inner.y) + inner.height <= std::int64_t(outer.y) + outer.height;
}

} // namespace notification_log_detail

// This independent plan can be selected by the popup compositor once its live
// historical-row publisher and activation route exist. The controller already
// owns modal queue/focus; no row is displayed from an unfocused queued popup.
inline PopupScreenPlan plan_notification_log_popup(
    const PopupScreenSource &source, const UiScreenController &controller,
    const UiFrameSnapshot &frame, std::int32_t width, std::int32_t height,
    const UiGameplayBackend *activation_backend = nullptr,
    const std::function<std::optional<std::string>(std::string_view)> &localize = {}) {
    PopupScreenPlan plan{source.context_id, source.selected_source_path,
                         {}, {}, {}, std::nullopt, std::nullopt};
    if (source.context_id != "NotificationLogPopup" ||
        controller.focused_screen() != std::optional<std::string>{source.context_id})
        return plan;
    const auto *events = frame.find<PopupFrame>();
    if (!events || !frame.active_player_id() || !frame.turn() ||
        events->player_id != *frame.active_player_id() ||
        events->turn != *frame.turn() || !events->owner_sequence) {
        plan.unresolved.push_back("Current notification-log popup owner is unavailable");
        return plan;
    }
    const PopupEvent *event = nullptr;
    for (const auto &item : events->events) {
        if (item.context_id != source.context_id) continue;
        if (event) {
            plan.unresolved.push_back("Notification-log popup owner is ambiguous");
            return plan;
        }
        event = &item;
    }
    if (!event || event->event_id < 0) {
        plan.unresolved.push_back("No current notification-log popup event");
        return plan;
    }
    const auto *panel = notification_log_detail::panel(source.document);
    const auto *scroll = notification_log_detail::find_id(
        source.document, "NotificationScrollPanel");
    const auto *row_instance = notification_log_detail::row_instance(source.document);
    const auto *stack = notification_log_detail::find_id(
        source.document, "NotificationButtonStack");
    const auto *close = notification_log_detail::find_id(source.document, "CloseButton");
    const auto *close_box = notification_log_detail::parent_of(
        source.document, "CloseButton");
    if (source.selected_source_path.find("Expansion2") == std::string::npos ||
        !panel || !scroll || !row_instance || !stack || !close || !close_box ||
        !notification_log_detail::find_id(source.document, "NotificationTurnText") ||
        !notification_log_detail::find_id(source.document, "NotificationText") ||
        row_instance->attribute("Size") != "890,60" ||
        scroll->attribute("Anchor") != "L,T" ||
        scroll->attribute("Offset") != "16,46" ||
        scroll->attribute("Size") != "900,500" ||
        stack->attribute("Offset") != "10,0" ||
        close_box->attribute("Anchor") != "C,B" ||
        close_box->attribute("Offset") != "0,54" ||
        close_box->attribute("Size") != "910,56" ||
        close->attribute("Anchor") != "L,B" ||
        close->attribute("Size") != "150,32" ||
        close->attribute("Offset") != "14,0" ||
        width < 960 || height < 658) {
        plan.unresolved.push_back("Selected BNW notification-log geometry is unavailable");
        return plan;
    }
    const WorldViewRect bounds{(width - 960) / 2, (height - 658) / 2 + 36,
                               960, 658};
    const WorldViewRect viewport{bounds.x + 16, bounds.y + 46, 900, 500};
    const WorldViewRect close_bounds{bounds.x + 39, bounds.y + 572, 150, 32};
    if (!notification_log_detail::inside({0, 0, width, height}, bounds) ||
        !notification_log_detail::inside(bounds, viewport) ||
        !notification_log_detail::inside(bounds, close_bounds)) {
        plan.unresolved.push_back("Notification-log viewport does not fit screen");
        return plan;
    }
    plan.paints.push_back({WorldViewPaintKind::rectangle, bounds,
        panel->attribute("Style"), {}, {}, {}});
    if (localize)
        if (auto title = localize("TXT_KEY_POP_NOTIFICATION_LOG"); title &&
            !title->empty() && !title->starts_with("TXT_KEY_"))
            plan.paints.push_back({WorldViewPaintKind::text,
                {bounds.x + 48, bounds.y + 12, bounds.width - 96, 32}, {}, {},
                *title, "MenuTitleCaption"});

    const auto *history = frame.find<NotificationLogSnapshot>();
    const bool valid_history = history && history->owner_provided &&
        history->player_id == events->player_id &&
        history->turn == events->turn &&
        history->owner_sequence == events->owner_sequence &&
        history->event_id == event->event_id &&
        history->frame_number == frame.frame() &&
        history->selected_source_path == source.selected_source_path;
    if (!valid_history) {
        plan.unresolved.push_back("Live historical notification rows are unavailable or stale");
    } else {
        std::set<std::int64_t> seen;
        std::int32_t previous_bottom = viewport.y;
        std::int32_t previous_turn = std::numeric_limits<std::int32_t>::max();
        bool rows_valid = true;
        for (const auto &row : history->visible_rows) {
            const auto box = row.visible_bounds;
            if (row.notification_id < 0 || row.turn < 0 ||
                row.turn > previous_turn ||
                row.description.empty() || row.localized_turn.empty() ||
                row.localized_turn.starts_with("TXT_KEY_") || box.height < 26 ||
                !seen.insert(row.notification_id).second ||
                !notification_log_detail::inside(viewport, box) ||
                box.x != viewport.x + 10 || box.width != 890 ||
                box.y < previous_bottom) {
                plan.unresolved.push_back("Live notification-log row is invalid or unmeasured");
                rows_valid = false;
                break;
            }
            previous_bottom = box.y + box.height;
            previous_turn = row.turn;
        }
        if (rows_valid) for (const auto &row : history->visible_rows) {
            const auto box = row.visible_bounds;
            plan.paints.push_back({WorldViewPaintKind::text,
                {box.x + 16, box.y + 5, box.width - 112, box.height - 10},
                {}, {}, row.description, "TwCenMT18"});
            plan.paints.push_back({WorldViewPaintKind::text,
                {box.x + box.width - 100, box.y + 7, 95, 26},
                {}, {}, row.localized_turn, "TwCenMT18"});
            const auto command = notification_log_activate(*history, row.notification_id);
            const auto *gameplay = std::get_if<UiGameplayCommand>(&command);
            const auto available = row.dismissed || !activation_backend || !gameplay ||
                activation_backend != history->activation_backend_owner
                ? UiCommandResult{UiCommandStatus::unavailable,
                    row.dismissed ? "Notification was dismissed"
                                  : "Live notification activation backend is unavailable"}
                : activation_backend->availability(*gameplay, frame);
            plan.hits.push_back({box, command,
                available.status == UiCommandStatus::ready, available.reason});
        }
    }
    plan.paints.push_back({WorldViewPaintKind::rectangle, close_bounds,
        close->attribute("Style"), {}, {}, {}});
    if (localize)
        if (auto label = localize("TXT_KEY_CLOSE"); label &&
            !label->empty() && !label->starts_with("TXT_KEY_"))
            plan.paints.push_back({WorldViewPaintKind::text, close_bounds,
                {}, {}, *label, "TwCenMT18"});
    plan.hits.push_back({close_bounds, UiCloseScreen{source.context_id}, true, {}});
    return plan;
}

} // namespace civ5::ui
