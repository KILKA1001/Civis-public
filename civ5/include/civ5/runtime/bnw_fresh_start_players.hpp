#pragma once

#include "civ5/runtime/bnw_fresh_start_luxuries.hpp"
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace civ5::runtime {
struct BnwMapIncludedSource;

struct BnwFreshStartPlayerSlot {
    bool ever_alive = false;
    std::int32_t team_id = -1;
};
struct BnwFreshStartTeamSlot { bool ever_alive = false; };
struct BnwFreshStartPlayerTeamSnapshot {
    // Only a real GameCore player/team initializer may set this. Full arrays
    // represent Players[0..MAX_CIV_PLAYERS) and Teams[0..MAX_CIV_TEAMS).
    bool initialized_by_gamecore = false;
    std::int32_t max_major_civs = 0, max_civ_players = 0,
                 max_civ_teams = 0;
    std::vector<BnwFreshStartPlayerSlot> players;
    std::vector<BnwFreshStartTeamSlot> teams;
    std::string selected_rules_sha256;
};
enum class BnwStartPlayersBlockerCode {
    luxury_weights_pending,
    stock_helper_unverified,
    player_team_init_pending,
    invalid_player_team_snapshot,
    region_generation_pending,
};
struct BnwStartPlayersBlocker {
    BnwStartPlayersBlockerCode code;
    std::string message;
};
struct BnwFreshStartPlayerTeamInfo {
    std::int32_t major_civs = 0, city_states = 0;
    std::vector<std::int32_t> player_ids;
    bool team_game = false;
    std::vector<std::int32_t> teams_with_major_civs;
    // Lua table.fill(0,MAX_CIV_PLAYERS) then [0]=0: indices 0..max.
    std::vector<std::int32_t> civs_per_team;
    std::string selected_rules_sha256;
    SimulationRandomState map_random_before{}, map_random_after{};
    std::vector<BnwStartPlayersBlocker> blockers;
    bool player_team_info_complete = false;
    bool has_blocker(BnwStartPlayersBlockerCode code) const;
    void rollback() noexcept;
};

// Exact read-only GetPlayerAndTeamInfo called first in GenerateRegions.
// No player/team objects are created here and no map RNG is consumed.
BnwFreshStartPlayerTeamInfo draft_bnw_fresh_start_player_team_info(
    const BnwFreshStartLuxuryWeights &luxuries,
    const BnwFreshStartPlayerTeamSnapshot &snapshot,
    const SimulationRandom &map_random,
    std::span<const BnwMapIncludedSource> selected_includes);

} // namespace civ5::runtime
