#pragma once

#include "civ5/runtime/bnw_fresh_player_colors.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace civ5::runtime {

enum class BnwFreshPlayerInitStep : std::uint8_t {
    notifications_pending, reset_pending, containers_pending,
    setup_graphical_pending, team_member_increment,
    set_alive_pending, ai_init_pending,
};
struct BnwFreshPlayerInitEvent {
    BnwFreshPlayerInitStep step;
    std::int32_t player_id;
    std::int32_t team_id;
    bool operator==(const BnwFreshPlayerInitEvent &) const = default;
};
enum class BnwFreshPlayerInitBlockerCode {
    pregame_roster_or_colors_pending,
    notifications_and_containers_pending,
    player_reset_pending,
    setup_graphical_pending,
    set_alive_callbacks_pending,
    ai_init_pending,
};
struct BnwFreshPlayerInitBlocker {
    BnwFreshPlayerInitBlockerCode code;
    std::string message;
};
struct BnwFreshPlayerInitPrefix {
    std::vector<std::int32_t> team_members;
    std::vector<bool> planned_alive;
    std::vector<std::int32_t> major_colors;
    std::vector<BnwFreshPlayerInitEvent> source_order;
    std::string selected_rules_sha256;
    std::vector<BnwFreshPlayerInitBlocker> blockers;
    bool member_prefix_complete = false;
    bool initialized_by_gamecore = false;
    bool has_blocker(BnwFreshPlayerInitBlockerCode code) const;
    void rollback() noexcept;
};

// Detached prefix of CvPlayerAI::init (inherited CvPlayer::init) over the
// already selected pregame slots. The member increments are computed but may
// not be published before reset/setupGraphical; setAlive remains a plan only.
BnwFreshPlayerInitPrefix draft_bnw_fresh_player_init_prefix(
    const BnwFreshPregameRosterDraft &roster,
    const BnwFreshPlayerColorsDraft &colors);

} // namespace civ5::runtime
