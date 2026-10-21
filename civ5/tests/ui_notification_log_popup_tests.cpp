#include "civ5/ui/notification_log_popup.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool value, const char *reason) {
    if (!value) throw std::runtime_error(reason);
}

struct ReadyBackend final : civ5::ui::UiGameplayBackend {
    mutable int availability_calls = 0;
    civ5::ui::UiCommandResult availability(
        const civ5::ui::UiGameplayCommand &command,
        const civ5::ui::UiFrameSnapshot &) const override {
        require(command.source_id() == "NotificationLogPopup" &&
                command.as<civ5::ui::NotificationLogActivate>() &&
                command.as<civ5::ui::NotificationLogActivate>()->notification_id == 101,
                "only the current live notification reaches activation backend");
        ++availability_calls;
        return {civ5::ui::UiCommandStatus::ready, {}};
    }
    civ5::ui::UiCommandResult execute(
        const civ5::ui::UiGameplayCommand &,
        const civ5::ui::UiFrameSnapshot &) override {
        return {civ5::ui::UiCommandStatus::performed, {}};
    }
};
}

int main(int argc, char **argv) {
    try {
        require(argc == 2, "provide selected BNW asset root");
        using namespace civ5::ui;
        const std::filesystem::path root(argv[1]);
        auto base = std::make_shared<civ5::assets::LooseResourceIndex>(root);
        auto expansion = std::make_shared<civ5::assets::LooseResourceIndex>(root);
        auto bnw = std::make_shared<civ5::assets::LooseResourceIndex>(root);
        base->scan({"Assets/UI"});
        expansion->scan({"Assets/DLC/Expansion/UI"});
        bnw->scan({"Assets/DLC/Expansion2/UI"});
        civ5::assets::ResourceMountTable resources(civ5::assets::resource_lookup(base));
        resources.mount({1, 0, 0, civ5::assets::resource_lookup(expansion)});
        resources.mount({2, 0, 0, civ5::assets::resource_lookup(bnw)});
        const auto registry = UiContextRegistry::from_resources(resources);
        const auto catalog = PopupCatalog::from_resources(resources, registry);
        const auto *source = catalog.find("NotificationLogPopup");
        require(source && source->selected_source_path.find("Expansion2") !=
                    std::string::npos, "selected BNW notification log XML");

        PopupEvent event;
        event.context_id = "NotificationLogPopup";
        event.event_id = 77;
        event.dismissible = true;
        PopupFrame popups;
        popups.player_id = 0;
        popups.turn = 12;
        popups.owner_sequence = 3;
        popups.events.push_back(event);
        NotificationLogSnapshot history;
        history.owner_provided = true;
        history.player_id = 0;
        history.turn = 12;
        history.owner_sequence = 3;
        history.event_id = 77;
        history.frame_number = 1;
        history.selected_source_path = source->selected_source_path;
        history.visible_rows = {
            {101, 11, "An active notification", "Turn 11", false,
                {186, 233, 890, 60}},
            {100, 10, "An old notification", "Turn 10", true,
                {186, 293, 890, 60}},
        };
        const auto frame = UiFrameSnapshot(1, 0, 12)
            .with<PopupFrame>(std::make_shared<const PopupFrame>(popups))
            .with<NotificationLogSnapshot>(
                std::make_shared<const NotificationLogSnapshot>(history));
        UiScreenController controller(registry);
        require(plan_notification_log_popup(*source, controller, frame, 1280, 960)
                    .paints.empty(), "queued but unfocused log is not painted");
        require(controller.dispatch(UiOpenScreen{"NotificationLogPopup"}, frame).status ==
                    UiCommandStatus::navigation, "open modal notification log");
        const auto plan = plan_notification_log_popup(*source, controller, frame,
            1280, 960, nullptr, [](std::string_view key) -> std::optional<std::string> {
                if (key == "TXT_KEY_CLOSE") return "Close";
                if (key == "TXT_KEY_POP_NOTIFICATION_LOG") return "Notification Log";
                return std::nullopt;
            });
        require(plan.hits.size() == 3 && !plan.hits[0].enabled &&
                !plan.hits[1].enabled && plan.hits[2].enabled &&
                std::holds_alternative<UiCloseScreen>(plan.hits[2].command),
                "historical rows are inert without activation backend; close works");
        require(plan.hits[0].bounds.x == 186 && plan.hits[0].bounds.y == 233 &&
                plan.hits[2].bounds.x == 199 && plan.hits[2].bounds.y == 759,
                "1280x960 active BNW scroll and close geometry");
        auto full_hd_history = history;
        full_hd_history.visible_rows[0].visible_bounds = {506, 293, 890, 60};
        full_hd_history.visible_rows[1].visible_bounds = {506, 353, 890, 60};
        const auto full_hd_frame = UiFrameSnapshot(1, 0, 12)
            .with<PopupFrame>(std::make_shared<const PopupFrame>(popups))
            .with<NotificationLogSnapshot>(
                std::make_shared<const NotificationLogSnapshot>(full_hd_history));
        const auto full_hd = plan_notification_log_popup(*source, controller,
            full_hd_frame, 1920, 1080);
        require(full_hd.hits.size() == 3 && full_hd.hits[0].bounds.x == 506 &&
                full_hd.hits[0].bounds.y == 293 &&
                full_hd.hits[2].bounds.x == 519 &&
                full_hd.hits[2].bounds.y == 819,
                "1920x1080 active BNW scroll and close geometry");
        const auto *action = std::get_if<UiGameplayCommand>(&plan.hits[0].command);
        require(action && action->as<NotificationLogActivate>() &&
                action->as<NotificationLogActivate>()->notification_id == 101 &&
                action->as<NotificationLogActivate>()->owner_sequence == 3,
                "notification activation command retains owner sequence and ID");
        NotificationLogUnavailableBackend unavailable;
        require(unavailable.availability(*action, frame).status ==
                    UiCommandStatus::unavailable &&
                unavailable.execute(*action, frame).status ==
                    UiCommandStatus::unavailable,
                "absent live activation never reports success");
        ReadyBackend ready;
        require(!plan_notification_log_popup(*source, controller, frame,
                    1280, 960, &ready).hits[0].enabled &&
                ready.availability_calls == 0,
                "an unrelated injected backend cannot enable activation");
        auto connected_history = history;
        connected_history.activation_backend_owner = &ready;
        const auto connected_frame = UiFrameSnapshot(1, 0, 12)
            .with<PopupFrame>(std::make_shared<const PopupFrame>(popups))
            .with<NotificationLogSnapshot>(
                std::make_shared<const NotificationLogSnapshot>(connected_history));
        const auto enabled = plan_notification_log_popup(*source, controller,
            connected_frame, 1280, 960, &ready);
        require(enabled.hits.size() == 3 && enabled.hits[0].enabled &&
                !enabled.hits[1].enabled && ready.availability_calls == 1,
                "only non-dismissed measured row can reach injected backend");
        auto invalid_history = history;
        invalid_history.visible_rows[1].notification_id = 101;
        const auto invalid_frame = UiFrameSnapshot(1, 0, 12)
            .with<PopupFrame>(std::make_shared<const PopupFrame>(popups))
            .with<NotificationLogSnapshot>(
                std::make_shared<const NotificationLogSnapshot>(invalid_history));
        require(plan_notification_log_popup(*source, controller, invalid_frame,
                    1280, 960).hits.size() == 1,
                "duplicate historical identity withholds all row actions");
        auto changed_source = *source;
        const auto change_scroll = [&](const auto &self,
                civ5::content::XmlNode &node) -> bool {
            if (node.attribute("ID") == "NotificationScrollPanel") {
                for (auto &[key, value] : node.attributes)
                    if (key == "Size") { value = "900,499"; return true; }
            }
            for (auto &child : node.children)
                if (self(self, child)) return true;
            return false;
        };
        require(change_scroll(change_scroll, changed_source.document) &&
                plan_notification_log_popup(changed_source, controller,
                    frame, 1280, 960).hits.empty(),
                "changed selected XML cannot inherit old click rectangles");
        const auto stale = UiFrameSnapshot(2, 0, 12)
            .with<PopupFrame>(std::make_shared<const PopupFrame>(popups))
            .with<NotificationLogSnapshot>(
                std::make_shared<const NotificationLogSnapshot>(history));
        const auto stale_plan = plan_notification_log_popup(*source, controller,
            stale, 1280, 960);
        require(stale_plan.hits.size() == 1 &&
                std::holds_alternative<UiCloseScreen>(stale_plan.hits[0].command),
                "stale history is withheld while close remains usable");
        require(controller.dispatch(UiOpenScreen{"NewEraPopup"}, frame).status ==
                    UiCommandStatus::navigation &&
                plan_notification_log_popup(*source, controller, frame, 1280, 960)
                    .hits.empty(), "covered notification log loses input focus");
        require(controller.dispatch(UiCloseScreen{"NewEraPopup"}, frame).status ==
                    UiCommandStatus::navigation &&
                plan_notification_log_popup(*source, controller, frame, 1280, 960)
                    .hits.size() == 3, "queue restores notification log focus");
        require(controller.dispatch(UiCloseScreen{"NotificationLogPopup"}, frame).status ==
                    UiCommandStatus::navigation &&
                plan_notification_log_popup(*source, controller, frame, 1280, 960)
                    .hits.empty(), "close removes the modal log");
        std::cout << "ui notification log popup tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
