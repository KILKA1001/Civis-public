#include "civ5/runtime/bnw_fresh_start_players.hpp"
#include "civ5/runtime/bnw_map_script.hpp"
#include <algorithm>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr std::string_view stock_mapmaker_sha256 =
    "ea463458f9fde6571ce2269c2ebf994f58d8f5b9859201bc506703d176c03f47";
constexpr std::string_view stock_starting_plots_sha256 =
    "a957db6a15138a09aef4221217c654ef146dca65aee17aea701e13b3db3b1b32";
bool selected_stock_mapmaker(
    std::span<const BnwMapIncludedSource> includes) {
    bool found = false;
    for (const auto &source : includes) {
        if (source.name != "MapmakerUtilities" &&
            source.name != "MapmakerUtilities.lua") continue;
        if (source.sha256 != stock_mapmaker_sha256) return false;
        found = true;
    }
    return found;
}
void block(BnwFreshStartPlayerTeamInfo &result,
           BnwStartPlayersBlockerCode code, std::string message) {
    result.blockers.push_back({code, std::move(message)});
}
} // namespace

bool BnwFreshStartPlayerTeamInfo::has_blocker(
    BnwStartPlayersBlockerCode code) const {
    return std::any_of(blockers.begin(), blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshStartPlayerTeamInfo::rollback() noexcept {
    major_civs = city_states = 0;
    player_ids.clear();
    team_game = false;
    teams_with_major_civs.clear();
    civs_per_team.clear();
    player_team_info_complete = false;
}

BnwFreshStartPlayerTeamInfo draft_bnw_fresh_start_player_team_info(
    const BnwFreshStartLuxuryWeights &luxuries,
    const BnwFreshStartPlayerTeamSnapshot &snapshot,
    const SimulationRandom &map_random,
    std::span<const BnwMapIncludedSource> selected_includes) {
    BnwFreshStartPlayerTeamInfo result;
    result.map_random_before = result.map_random_after = map_random.state();
    if (!luxuries.luxury_weights_complete ||
        !luxuries.has_blocker(BnwStartLuxuriesBlockerCode::custom_init_pending) ||
        luxuries.selected_rules_sha256.empty() ||
        luxuries.stock_helper_sha256 != stock_starting_plots_sha256 ||
        luxuries.map_random_after != map_random.state() ||
        map_random.algorithm() != RandomAlgorithm::bnw) {
        block(result, BnwStartPlayersBlockerCode::luxury_weights_pending,
              "Selected stock luxury weights must precede GenerateRegions");
        return result;
    }
    if (!selected_stock_mapmaker(selected_includes)) {
        block(result, BnwStartPlayersBlockerCode::stock_helper_unverified,
              "GetPlayerAndTeamInfo requires exact selected Expansion2 MapmakerUtilities bytes");
        return result;
    }
    if (!snapshot.initialized_by_gamecore) {
        block(result, BnwStartPlayersBlockerCode::player_team_init_pending,
              "Fresh GameCore must initialize Players and Teams before GenerateRegions");
        return result;
    }
    const auto major = snapshot.max_major_civs;
    const auto players = snapshot.max_civ_players;
    const auto teams = snapshot.max_civ_teams;
    if (snapshot.selected_rules_sha256 != luxuries.selected_rules_sha256 ||
        major <= 0 || players < major || teams <= 0 || teams > players ||
        players > 4096 || snapshot.players.size() !=
                              static_cast<std::size_t>(players) ||
        snapshot.teams.size() != static_cast<std::size_t>(teams)) {
        block(result, BnwStartPlayersBlockerCode::invalid_player_team_snapshot,
              "Player/team slots or selected GameDefines are incomplete");
        return result;
    }
    for (const auto &player : snapshot.players)
        if (player.ever_alive &&
            (player.team_id < 0 || player.team_id >= teams ||
             !snapshot.teams[static_cast<std::size_t>(player.team_id)].ever_alive)) {
            block(result, BnwStartPlayersBlockerCode::invalid_player_team_snapshot,
                  "An ever-alive player has no ever-alive team");
            return result;
        }
    result.selected_rules_sha256 = luxuries.selected_rules_sha256;
    // MapmakerUtilities.GetPlayerAndTeamInfo: majors, then city states, then
    // all civ teams. Keep first-seen major team order, not team-ID order.
    for (std::int32_t id = 0; id < major; ++id)
        if (snapshot.players[static_cast<std::size_t>(id)].ever_alive) {
            ++result.major_civs;
            result.player_ids.push_back(id);
        }
    for (std::int32_t id = major; id < players; ++id)
        if (snapshot.players[static_cast<std::size_t>(id)].ever_alive)
            ++result.city_states;
    std::int32_t ever_alive_teams = 0;
    for (const auto &team : snapshot.teams)
        if (team.ever_alive) ++ever_alive_teams;
    result.team_game = ever_alive_teams - result.city_states < result.major_civs;
    result.civs_per_team.assign(static_cast<std::size_t>(players) + 1, 0);
    for (std::int32_t id = 0; id < major; ++id) {
        const auto &player = snapshot.players[static_cast<std::size_t>(id)];
        if (!player.ever_alive) continue;
        ++result.civs_per_team[static_cast<std::size_t>(player.team_id)];
        if (std::find(result.teams_with_major_civs.begin(),
                      result.teams_with_major_civs.end(), player.team_id) ==
            result.teams_with_major_civs.end())
            result.teams_with_major_civs.push_back(player.team_id);
    }
    result.player_team_info_complete = true;
    block(result, BnwStartPlayersBlockerCode::region_generation_pending,
          "GenerateRegions biggest-landmass fertility and division remain pending");
    return result;
}
} // namespace civ5::runtime
