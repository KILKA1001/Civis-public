#include "civ5/runtime/bnw_fresh_economic_ai_init.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_selected_fresh_economic_ai_init() {
    using namespace civ5::runtime;
    BnwFreshGameContentResult selected;
    selected.profile.kind = ContentProfileKind::dlc;
    selected.rules = std::make_unique<civ5::content::RulesDatabase>();
    selected.rules->apply_sql(R"sql(
        CREATE TABLE AIEconomicStrategies(ID INTEGER PRIMARY KEY, Type TEXT);
        INSERT INTO AIEconomicStrategies VALUES(0,'STRATEGY_0');
        INSERT INTO AIEconomicStrategies VALUES(2,'STRATEGY_2');
        CREATE TABLE Flavors(ID INTEGER PRIMARY KEY, Type TEXT);
        INSERT INTO Flavors VALUES(0,'FLAVOR_0');
        INSERT INTO Flavors VALUES(1,'FLAVOR_1');
        CREATE TABLE Defines(Name TEXT PRIMARY KEY, Value TEXT);
        INSERT INTO Defines VALUES('AI_STRATEGY_MINIMUM_SETTLE_FERTILITY','6');
        INSERT INTO Defines VALUES('AI_STRATEGY_EARLY_EXPANSION_NUM_CITIES_LIMIT','10');
    )sql");
    selected.activation.emplace();
    selected.activation->profile_kind = ContentProfileKind::dlc;
    selected.activation->rules_database_ready = true;
    const auto sha = identify_bytes(selected.rules->gameplay_fingerprint_data()).sha256;
    BnwFreshPlayerInfoReset player_info;
    player_info.selected_rules_sha256 = sha;
    player_info.selected_arrays_complete = true;
    player_info.players.resize(2);
    player_info.players[0].early.id = 0;
    player_info.players[1].early.id = 1;
    player_info.blockers.push_back({
        BnwFreshPlayerInfoBlockerCode::ai_services_and_remaining_reset_pending,
        "fixture"});
    auto draft = draft_bnw_fresh_economic_ai_init(
        selected,player_info,sha);
    check(draft.economic_ai_init_complete && !draft.initialized_by_gamecore &&
              draft.players.size() == 2 && draft.source_order.size() == 18 &&
              draft.source_order[0] == BnwFreshEconomicInitEvent{
                  BnwFreshEconomicInitStep::selected_strategies_and_player_bound,0} &&
              draft.source_order[8] == BnwFreshEconomicInitEvent{
                  BnwFreshEconomicInitStep::military_ai_pending,0} &&
              draft.source_order[9] == BnwFreshEconomicInitEvent{
                  BnwFreshEconomicInitStep::selected_strategies_and_player_bound,1},
          "EconomicAI Init and Reset are interleaved per fresh player");
    const auto &player = draft.players[0];
    check(player.using_strategy == std::vector<bool>({false,false,false}) &&
              player.turn_strategy_adopted ==
                  std::vector<std::int32_t>({-1,-1,-1}) &&
              player.temp_flavors.size() == 2 &&
              !player.temp_flavors[0] && !player.temp_flavors[1] &&
              player.yields == std::array<std::int32_t,6>{} &&
              player.requested_savings.size() == 6 &&
              player.requested_savings[5] == BnwFreshPurchaseRequest{5,0,0} &&
              player.exploration_plots_dirty && player.recon_state == -1 &&
              player.minimum_settle_fertility == 6 &&
              player.early_city_number_target == 10 &&
              draft.has_blocker(
                  BnwFreshEconomicInitBlockerCode::military_and_remaining_ai_pending),
          "Selected strategy slots, undefined temp flavors and cached defines persist");
    const auto limited = draft_bnw_fresh_economic_ai_init(
        selected,player_info,sha,1);
    check(limited.has_blocker(
              BnwFreshEconomicInitBlockerCode::allocation_budget_exceeded) &&
              limited.players.empty(),
          "Economic allocation budget is atomic");
    selected.rules->apply_sql(
        "DELETE FROM Defines WHERE Name = 'AI_STRATEGY_MINIMUM_SETTLE_FERTILITY';");
    const auto changed_sha = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    player_info.selected_rules_sha256 = changed_sha;
    const auto missing = draft_bnw_fresh_economic_ai_init(
        selected,player_info,changed_sha);
    check(missing.has_blocker(
              BnwFreshEconomicInitBlockerCode::economic_strategy_or_define_unverified) &&
              missing.players.empty(),
          "Missing selected cached define blocks fresh EconomicAI");
    draft.rollback();
    check(draft.players.empty() && draft.source_order.empty() &&
              !draft.economic_ai_init_complete && player_info.players.size() == 2,
          "Rollback leaves earlier selected player arrays untouched");
}
} // namespace

int main() { test_selected_fresh_economic_ai_init(); }
