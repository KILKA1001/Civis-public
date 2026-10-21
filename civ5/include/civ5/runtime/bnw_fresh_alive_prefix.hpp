#pragma once

#include "civ5/runtime/bnw_fresh_player_init_prefix.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace civ5::runtime {

enum class BnwFreshAliveStep : std::uint8_t {
    prior_player_member_increment, current_player_member_increment,
    player_alive_written, team_alive_incremented, league_callback_pending,
};
struct BnwFreshAliveEvent {
    BnwFreshAliveStep step;
    std::int32_t player_id, team_id;
    bool operator==(const BnwFreshAliveEvent &) const = default;
};
enum class BnwFreshAliveBlockerCode {
    init_prefix_unverified,
    no_active_player,
    player_reset_graphics_pending,
    league_callback_pending,
};
struct BnwFreshAliveBlocker {
    BnwFreshAliveBlockerCode code;
    std::string message;
};
struct BnwFreshAliveBeforeLeague {
    std::vector<std::int32_t> team_members_at_boundary;
    std::vector<std::int32_t> team_alive_at_boundary;
    std::vector<bool> player_alive_at_boundary;
    std::int32_t first_active_player = -1;
    std::vector<BnwFreshAliveEvent> source_order;
    std::string selected_rules_sha256;
    std::vector<BnwFreshAliveBlocker> blockers;
    bool pre_league_prefix_complete = false;
    bool initialized_by_gamecore = false;
    bool has_blocker(BnwFreshAliveBlockerCode code) const;
    void rollback() noexcept;
};

// First fresh CvPlayer::setAlive(true) only: m_bAlive and team alive count
// precede GameLeagues::DoPlayerAliveStatusChanged. No later player or team
// ever-alive state is invented while that callback is unresolved.
BnwFreshAliveBeforeLeague draft_bnw_fresh_first_alive_before_league(
    const BnwFreshPregameRosterDraft &roster,
    const BnwFreshPlayerInitPrefix &init_prefix);

} // namespace civ5::runtime
