#pragma once

#include "civ5/runtime/bnw_fresh_pregame_roster.hpp"
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

enum class BnwFreshPlayerResetStep : std::uint8_t {
    major_notifications_pending,
    sync_archive_reset,
    player_uninit_pending,
    id_and_personality_write,
    tutorial_reset,
    yield_vectors_reset,
    player_vectors_reset,
    options_and_strings_clear,
    resource_info_arrays_pending,
};
struct BnwFreshPlayerResetEvent {
    BnwFreshPlayerResetStep step;
    std::int32_t player_id;
    bool operator==(const BnwFreshPlayerResetEvent &) const = default;
};
enum class BnwFreshPlayerResetBlockerCode {
    roster_rules_or_native_constants_unverified,
    pregame_leader_info_unverified,
    notifications_and_uninit_pending,
    resource_info_and_remaining_reset_pending,
};
struct BnwFreshPlayerResetBlocker {
    BnwFreshPlayerResetBlockerCode code;
    std::string message;
};
struct BnwFreshPlayerEarlyReset {
    std::int32_t id = -1;
    std::int32_t personality = -1; // CvPreGame::leaderHead, NO_LEADER=-1
    bool ever_popped_goody = false;
    // CvPlayer::reset writes these ten NUM_YIELD_TYPES vectors in SDK order.
    std::array<std::vector<std::int32_t>,10> yield_vectors;
    std::vector<std::int32_t> proximity_to_player;
    std::vector<std::int32_t> research_agreement_counter;
    std::vector<std::int32_t> incoming_unit_types;
    std::vector<std::int32_t> incoming_unit_countdowns;
    std::vector<std::int32_t> minor_friendship_anchors;
    std::vector<std::int32_t> siphon_luxury_count;
    std::vector<std::int32_t> options;
    std::string religion_key, script_data, embarked_graphic_override;
};
struct BnwFreshPlayerResetPrefix {
    std::vector<BnwFreshPlayerEarlyReset> players;
    std::vector<BnwFreshPlayerResetEvent> source_order;
    std::string selected_rules_sha256;
    std::vector<BnwFreshPlayerResetBlocker> blockers;
    bool early_arrays_complete = false;
    bool initialized_by_gamecore = false;
    bool has_blocker(BnwFreshPlayerResetBlockerCode code) const;
    void rollback() noexcept;
};

// Detached early CvPlayer::reset(eID) state, one player at a time. Explicit
// pregame leader IDs are checked against selected Leaders Info. The owner does
// not pass the missing notification/uninit or later resource-array boundary.
BnwFreshPlayerResetPrefix draft_bnw_fresh_player_early_reset(
    const BnwFreshGameContentResult &selected,
    const BnwFreshPregameRosterDraft &roster,
    std::string_view selected_rules_sha256,
    std::span<const content::GameplayDefine> native_constants,
    std::span<const std::int32_t> pregame_leader_ids);

} // namespace civ5::runtime
