#pragma once

#include "civ5/runtime/bnw_fresh_alive_prefix.hpp"
#include "civ5/runtime/bnw_fresh_diplo_votes.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

enum class BnwFreshLeagueStep : std::uint8_t {
    empty_active_league_scan,
    votes_setter_called,
    league_screen_dirty_pending,
};

enum class BnwFreshLeagueBlockerCode {
    initial_state_unverified,
    first_alive_or_votes_unverified,
    league_screen_dirty_pending,
};

struct BnwFreshLeagueBlocker {
    BnwFreshLeagueBlockerCode code;
    std::string message;
};

// CvGameLeagues constructor and empty Init() precede CvGame::InitPlayers.
// This owner holds the first callback's detached state; it is not a live
// CvGameLeagues until the remaining engine callbacks are bound.
struct BnwFreshLeagueState {
    std::string selected_rules_sha256;
    std::vector<std::int32_t> active_league_ids;
    std::int32_t leagues_ever_founded = 0;
    std::int32_t diplomatic_victor = -1; // NO_PLAYER
    std::int32_t generated_id_count = 0;
    std::int32_t last_era_trigger = -1; // NO_ERA
    std::int32_t votes_needed = 0; // CvGame::reset
    std::int32_t first_alive_player = -1;
    std::vector<BnwFreshLeagueStep> source_order;
    std::vector<BnwFreshLeagueBlocker> blockers;
    bool constructor_and_init_complete = false;
    bool first_alive_callback_core_complete = false;
    bool league_screen_dirty_emitted = false;
    bool initialized_by_gamecore = false;
    bool has_blocker(BnwFreshLeagueBlockerCode code) const;
    void rollback() noexcept;
};

BnwFreshLeagueState init_bnw_fresh_league_state(
    std::string_view selected_rules_sha256);

// The first fresh setAlive scans an empty active-league list, updates the
// selected vote field, then calls the engine UI dirty callback. Stage exactly
// through the vote setter, retaining an ordered, unacknowledged UI event.
bool stage_bnw_first_alive_league_callback(
    BnwFreshLeagueState &state,
    const BnwFreshAliveBeforeLeague &before_league,
    const BnwFreshDiploVotesDraft &votes);

} // namespace civ5::runtime
