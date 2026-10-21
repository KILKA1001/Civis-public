#pragma once

#include "civ5/ui/live_session_frame.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <set>
#include <string>

namespace civ5::ui {

// The fields passed to ICvDLLUserInterface::AddPopup by BNW
// CvNotifications::Activate for NOTIFICATION_TECH. This is an owned value,
// never a view into the saved notification slot.
struct BnwNotificationPopupInfo {
    std::string button_popup_type;
    std::int64_t data1 = -1, data2 = -1, data3 = -1;
    std::int32_t flags = 0;
    bool option1 = false, option2 = false;
    std::string text;
    std::int64_t notification_lookup = -1;
    std::int32_t notification_x = -1, notification_y = -1;
    std::int32_t notification_turn = -1;
    std::int32_t turn = -1;
    std::uint64_t owner_sequence = 0;
};

struct BnwNotificationActivationServices {
    // These are the three ordered external calls in CvNotifications::Activate.
    // They must report completion by returning normally. A throwing callback
    // leaves an uncertain action that may not be replayed automatically.
    std::function<void(std::int64_t, std::uint32_t, const std::string &,
                       std::int32_t, std::int32_t, std::int64_t,
                       std::int64_t, std::int64_t)> activate_notification;
    std::function<void(std::int32_t, std::int32_t, std::int64_t)> minimap_notification;
    std::function<void(const BnwNotificationPopupInfo &)> add_popup;
};

enum class BnwNotificationActivationStatus { published, blocked, uncertain };
struct BnwNotificationActivationResult {
    BnwNotificationActivationStatus status = BnwNotificationActivationStatus::blocked;
    std::string reason;
};

// Owns only post-load UI events. A saved notification is not an open popup:
// this owner emits a CHOOSETECH event only after an explicit Activate lookup
// and all required native-equivalent callbacks complete.
class BnwNotificationPopupOwner {
  public:
    BnwNotificationPopupOwner(const runtime::LiveSession &session,
                              std::int64_t active_player_id);

    BnwNotificationActivationResult activate_tech(
        std::int64_t lookup_id, std::int32_t expected_turn,
        const BnwNotificationActivationServices &services);
    std::optional<std::string> tech_unavailable_reason(
        std::int64_t lookup_id, std::int32_t expected_turn,
        const BnwNotificationActivationServices &services) const;
    std::optional<LiveWorldViewNotificationFeed> active_notifications() const;
    std::optional<LivePopupEventFeed> active_popups() const;
    std::optional<ProgressionTechPopupEvent> active_tech_popup() const;
    std::int64_t player_id() const noexcept { return active_player_id_; }
    bool owns(const runtime::LiveSession &session,
              std::int64_t player_id) const noexcept {
        return session_ == &session && active_player_id_ == player_id;
    }
    const std::optional<BnwNotificationPopupInfo> &queued_popup() const noexcept {
        return popup_;
    }
    bool close(std::uint64_t owner_sequence);

  private:
    const runtime::LiveSession *session_;
    std::int64_t active_player_id_;
    std::uint64_t next_sequence_ = 1;
    std::set<std::int64_t> attempted_lookups_;
    // This first owner supports one focused CHOOSETECH modal at a time.
    // The pending value is a capacity-one owned queue, never a saved slot.
    std::optional<BnwNotificationPopupInfo> popup_;
};

// Bind the three frame callbacks to exactly one restored session and player.
// Reusing these sources with a foreign session publishes no notification or
// popup event, even if its player and turn numbers happen to match.
void bind_bnw_notification_frame_sources(LiveSessionUiSources &sources,
                                         BnwNotificationPopupOwner &owner);

} // namespace civ5::ui
