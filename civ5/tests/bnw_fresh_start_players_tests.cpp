#include "civ5/runtime/bnw_fresh_start_players.hpp"
#include "civ5/runtime/bnw_map_script.hpp"
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_get_player_and_team_info() {
    using namespace civ5::runtime;
    SimulationRandom random(RandomAlgorithm::bnw, {74, 19, 1});
    BnwFreshStartLuxuryWeights luxuries;
    luxuries.luxury_weights_complete = true;
    luxuries.selected_rules_sha256 = "selected-rules-fixture";
    luxuries.stock_helper_sha256 =
        "a957db6a15138a09aef4221217c654ef146dca65aee17aea701e13b3db3b1b32";
    luxuries.map_random_before = luxuries.map_random_after = random.state();
    luxuries.blockers.push_back({BnwStartLuxuriesBlockerCode::custom_init_pending,
                                 "fixture"});
    const std::vector<BnwMapIncludedSource> includes{{
        "MapmakerUtilities",
        "ea463458f9fde6571ce2269c2ebf994f58d8f5b9859201bc506703d176c03f47",
        {civ5::assets::SourceKind::loose, "Expansion2",
         "Assets/DLC/Expansion2/Gameplay/Lua/MapmakerUtilities.lua", std::nullopt}}};
    BnwFreshStartPlayerTeamSnapshot snapshot;
    snapshot.max_major_civs = 3;
    snapshot.max_civ_players = 6;
    snapshot.max_civ_teams = 6;
    snapshot.selected_rules_sha256 = luxuries.selected_rules_sha256;
    snapshot.players = {{true, 2}, {true, 2}, {true, 0},
                        {true, 1}, {false, -1}, {false, -1}};
    snapshot.teams = {{true}, {true}, {true}, {false}, {false}, {false}};
    const auto uninitialized = draft_bnw_fresh_start_player_team_info(
        luxuries, snapshot, random, includes);
    check(uninitialized.has_blocker(BnwStartPlayersBlockerCode::player_team_init_pending) &&
              !uninitialized.player_team_info_complete &&
              uninitialized.player_ids.empty(),
          "GenerateRegions must wait for real player/team initialization");
    snapshot.initialized_by_gamecore = true;
    const auto before = random.state();
    auto info = draft_bnw_fresh_start_player_team_info(
        luxuries, snapshot, random, includes);
    check(info.player_team_info_complete &&
              info.has_blocker(BnwStartPlayersBlockerCode::region_generation_pending) &&
              info.major_civs == 3 && info.city_states == 1 &&
              info.player_ids == std::vector<std::int32_t>({0, 1, 2}) &&
              info.team_game &&
              info.teams_with_major_civs == std::vector<std::int32_t>({2, 0}) &&
              info.civs_per_team.size() == 7 &&
              info.civs_per_team[2] == 2 && info.civs_per_team[0] == 1 &&
              info.map_random_before == before && info.map_random_after == before &&
              random.state() == before,
          "GetPlayerAndTeamInfo preserves major, minor and first-seen team order");
    auto invalid = snapshot;
    invalid.teams[2].ever_alive = false;
    const auto refused = draft_bnw_fresh_start_player_team_info(
        luxuries, invalid, random, includes);
    check(refused.has_blocker(BnwStartPlayersBlockerCode::invalid_player_team_snapshot) &&
              refused.player_ids.empty(),
          "A partial player/team snapshot cannot be published");
    const auto wrong_helper = draft_bnw_fresh_start_player_team_info(
        luxuries, snapshot, random, {});
    check(wrong_helper.has_blocker(BnwStartPlayersBlockerCode::stock_helper_unverified),
          "Player-team helper requires selected stock Lua bytes");
    info.rollback();
    check(!info.player_team_info_complete && info.player_ids.empty() &&
              info.civs_per_team.empty() && random.state() == before,
          "Rollback discards detached player-team data");
}
} // namespace

int main() { test_get_player_and_team_info(); }
