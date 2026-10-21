#pragma once

#include "civ5/runtime/bnw_fresh_slot_info.hpp"
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

enum class BnwFreshFirstPlayerObjectStep : std::uint8_t {
    notifications_allocated,
    notifications_constructor_uninit,
    diplomacy_requests_allocated,
    diplomacy_requests_constructor_uninit,
    reset_sync_archive_pending,
};
struct BnwFreshFirstPlayerObjectEvent {
    BnwFreshFirstPlayerObjectStep step;
    std::int32_t player_id;
    bool operator==(const BnwFreshFirstPlayerObjectEvent &) const = default;
};
enum class BnwFreshFirstPlayerObjectBlockerCode {
    selected_pregame_or_constants_unverified,
    reset_sync_archive_and_uninit_pending,
};
struct BnwFreshFirstPlayerObjectBlocker {
    BnwFreshFirstPlayerObjectBlockerCode code;
    std::string message;
};
struct BnwFreshNotificationsConstructorState {
    std::int32_t player = -1; // NO_PLAYER until CvNotifications::Init
    std::int32_t current_lookup_index = 0;
    std::int32_t begin_index = -1, end_index = -1;
    bool entries_empty = true;
};
struct BnwFreshDiplomacyRequestsConstructorState {
    std::int32_t player = -1, next_ai_player = -1;
    std::int32_t active_from_player = -1;
    bool request_active = false, entries_empty = true;
};
struct BnwFreshFirstPlayerObjects {
    std::int32_t player_id = -1;
    BnwFreshNotificationsConstructorState notifications;
    BnwFreshDiplomacyRequestsConstructorState diplomacy_requests;
    std::vector<BnwFreshFirstPlayerObjectEvent> source_order;
    std::string selected_rules_sha256;
    std::vector<BnwFreshFirstPlayerObjectBlocker> blockers;
    bool constructors_complete = false;
    bool initialized_by_gamecore = false;
    bool has_blocker(BnwFreshFirstPlayerObjectBlockerCode code) const;
    void rollback() noexcept;
};

// Only first CvPlayerAI::init. eID=0 is major in the selected native roster.
// The two FNEW constructors call Uninit; CvNotifications::Init and
// CvDiplomacyRequests::Init occur much later in CvPlayer::reset after AI
// services. The next actual operation is m_syncArchive.reset()/uninit().
BnwFreshFirstPlayerObjects draft_bnw_fresh_first_player_objects(
    const BnwFreshGameContentResult &selected,
    const BnwFreshSlotInfoDraft &slot_info,
    std::string_view selected_rules_sha256,
    std::span<const content::GameplayDefine> native_constants);

} // namespace civ5::runtime
