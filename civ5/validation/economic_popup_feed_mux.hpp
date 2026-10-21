#pragma once

#include "civ5/ui/live_session_frame.hpp"
#include <memory>

namespace civ5::validation {

using PopupFeedProvider = std::function<std::optional<civ5::ui::LivePopupEventFeed>(
    const civ5::runtime::LiveSession &)>;

struct PopupFeedMuxStatus {
    std::string blocker;
};

inline std::optional<civ5::ui::LivePopupEventFeed> select_popup_feed(
    std::optional<civ5::ui::LivePopupEventFeed> existing,
    std::optional<civ5::ui::LivePopupEventFeed> economy,
    PopupFeedMuxStatus *status = nullptr) {
    if (existing && economy) {
        if (status) status->blocker =
            "Concurrent notification and EconomicOverview popup owners "
            "cannot share one PopupFrame owner sequence";
        return std::nullopt;
    }
    if (status) status->blocker.clear();
    return existing ? std::move(existing) : std::move(economy);
}

// Retain both independent owners. One PopupFrame has one owner_sequence, so
// concurrent feeds cannot be merged without inventing event identity. Refuse
// the collision and let each owner's queue resume when the other closes.
inline PopupFeedProvider compose_popup_feeds(
    PopupFeedProvider existing, PopupFeedProvider economy,
    std::shared_ptr<PopupFeedMuxStatus> status = {}) {
    return [existing = std::move(existing), economy = std::move(economy),
            status = std::move(status)](
        const civ5::runtime::LiveSession &session)
        -> std::optional<civ5::ui::LivePopupEventFeed> {
        return select_popup_feed(
            existing ? existing(session) : std::nullopt,
            economy ? economy(session) : std::nullopt, status.get());
    };
}

} // namespace civ5::validation
