#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace civ5::saves { struct SavedSession; }

namespace civ5::runtime {

struct LiveSessionState;

struct BnwNotificationLoadSlot {
    bool needs_broadcast=false;
    bool wait_extra_turn=false;
    bool operator==(const BnwNotificationLoadSlot &) const = default;
};
struct BnwNotificationLoadPlayer {
    std::array<BnwNotificationLoadSlot,100> slots{};
    // Source-order ring positions, including entries that Update may expire.
    std::vector<std::size_t> active_undismissed;
    bool operator==(const BnwNotificationLoadPlayer &) const = default;
};
struct BnwNotificationLoadTransients {
    std::map<std::int64_t,BnwNotificationLoadPlayer> players;
    bool current=false;
    // Active entries still require CvNotifications::Update expiration checks,
    // engine UI broadcast and network/hotseat handling. No broadcast occurs.
    bool external_update_pending=false;
};

// CvPlayer::Read constructs/Init's CvNotifications, then CvNotifications::Read
// deserializes each of 100 slots. Notification operator>> unconditionally
// sets needsBroadcast=true and waitExtraTurn=false for every loaded slot.
// Version 0/1 need the separate V1_IndexToHash conversion and fail closed.
// This is a derived, read-only post-load context: rebuild it after an ID36
// notification change; never treat it as an Add/Dismiss gameplay action.
BnwNotificationLoadTransients rebuild_bnw_notification_load_transients(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
