#pragma once

#include "civ5/ui/bnw_notification_popup_owner.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace civ5::ui {

enum class BnwAddPopupQueueStatus { accepted, blocked };

struct BnwAddPopupQueueResult {
    BnwAddPopupQueueStatus status = BnwAddPopupQueueStatus::blocked;
    std::string reason;
};

// A capacity-one, UI-owned AddPopup sink. Enqueue does not activate a
// notification or supply the other required BNW UI effects.
class BnwAddPopupQueue {
  public:
    BnwAddPopupQueue(const runtime::LiveSession &session,
                     std::int64_t active_player_id);

    BnwAddPopupQueueResult enqueue(const runtime::LiveSession &source,
                                   const BnwNotificationPopupInfo &popup);
    // An acknowledgement means the UI owner accepted this popup for display;
    // it remains current until the matching owner explicitly closes it.
    bool acknowledge(const runtime::LiveSession &source,
                     std::uint64_t owner_sequence);
    bool close(const runtime::LiveSession &source,
               std::uint64_t owner_sequence);
    std::optional<BnwNotificationPopupInfo> current(
        const runtime::LiveSession &source) const;
    std::optional<BnwNotificationPopupInfo> unacknowledged(
        const runtime::LiveSession &source) const;

    // Borrows this queue and source session; both must outlive the callback.
    // A rejected enqueue throws so callers cannot report activation success.
    std::function<void(const BnwNotificationPopupInfo &)> add_popup_sink(
        const runtime::LiveSession &source);

  private:
    bool source_is_current(const runtime::LiveSession &source) const;

    const runtime::LiveSession *session_;
    std::int64_t active_player_id_;
    std::uint64_t last_sequence_ = 0;
    std::optional<BnwNotificationPopupInfo> current_;
    bool acknowledged_ = false;
};

} // namespace civ5::ui
