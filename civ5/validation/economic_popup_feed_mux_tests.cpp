#include "economic_popup_feed_mux.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char *reason) {
    if (!condition) throw std::runtime_error(reason);
}
}

int main() {
    try {
        using namespace civ5::ui;
        PopupEvent notification;
        notification.context_id = "TechPopup";
        notification.event_id = 77;
        PopupEvent economy;
        economy.context_id = "EconomicOverview";
        economy.event_id = 4;
        LivePopupEventFeed first{7, 12, 77, {notification}};
        LivePopupEventFeed second{7, 12, 4, {economy}};
        civ5::validation::PopupFeedMuxStatus status;
        const auto notification_only = civ5::validation::select_popup_feed(
            first, std::nullopt, &status);
        require(notification_only && notification_only->owner_sequence == 77 &&
                notification_only->events.front().context_id == "TechPopup" &&
                status.blocker.empty(),
                "existing notification retains its owner and event");
        const auto economy_only = civ5::validation::select_popup_feed(
            std::nullopt, second, &status);
        require(economy_only && economy_only->owner_sequence == 4 &&
                economy_only->events.front().context_id == "EconomicOverview" &&
                status.blocker.empty(),
                "economic popup appears when original provider is idle");
        require(!civ5::validation::select_popup_feed(
                    first, second, &status) && !status.blocker.empty(),
                "two owner sequences cannot form one PopupFrame");
        require(!civ5::validation::select_popup_feed(
                    first, first, &status) && !status.blocker.empty(),
                "duplicate event claims from two providers are refused");
        const auto resumed = civ5::validation::select_popup_feed(
            first, std::nullopt, &status);
        require(resumed && resumed->owner_sequence == 77 &&
                status.blocker.empty(),
                "notification resumes after collision clears");
        std::cout << "Economic popup feed mux: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
