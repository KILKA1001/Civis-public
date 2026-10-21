#include "civ5/runtime/bnw_fresh_military_ai_init.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_selected_fresh_military_ai_reset() {
    using namespace civ5::runtime;
    BnwFreshGameContentResult selected;
    selected.profile.kind = ContentProfileKind::dlc;
    selected.rules = std::make_unique<civ5::content::RulesDatabase>();
    selected.rules->apply_sql(R"sql(
        CREATE TABLE AIMilitaryStrategies(ID INTEGER PRIMARY KEY, Type TEXT);
        INSERT INTO AIMilitaryStrategies VALUES(0,'STRATEGY_0');
        INSERT INTO AIMilitaryStrategies VALUES(1,'STRATEGY_1');
        CREATE TABLE Flavors(ID INTEGER PRIMARY KEY, Type TEXT);
        INSERT INTO Flavors VALUES(0,'FLAVOR_0');
        INSERT INTO Flavors VALUES(1,'FLAVOR_1');
    )sql");
    selected.activation.emplace();
    selected.activation->profile_kind = ContentProfileKind::dlc;
    selected.activation->rules_database_ready = true;
    const auto sha = identify_bytes(selected.rules->gameplay_fingerprint_data()).sha256;
    BnwFreshPlayerInfoReset player_info;
    player_info.selected_rules_sha256 = sha;
    player_info.selected_arrays_complete = true;
    player_info.sizes.civ_players = 3;
    player_info.players.resize(2);
    player_info.players[0].early.id = 0;
    player_info.players[1].early.id = 1;
    BnwFreshEconomicAIInit economic;
    economic.selected_rules_sha256 = sha;
    economic.economic_ai_init_complete = true;
    economic.players.resize(2);
    economic.players[0].player_id = 0;
    economic.players[1].player_id = 1;
    economic.blockers.push_back({
        BnwFreshEconomicInitBlockerCode::military_and_remaining_ai_pending,
        "fixture"});
    auto draft = draft_bnw_fresh_military_ai_init(
        selected,player_info,economic,sha);
    check(draft.arrays_and_reset_complete && !draft.initialized_by_gamecore &&
              draft.players.size() == 2 && draft.source_order.size() == 12 &&
              draft.source_order[0] == BnwFreshMilitaryInitEvent{
                  BnwFreshMilitaryInitStep::selected_strategies_and_player_bound,0} &&
              draft.source_order[5] == BnwFreshMilitaryInitEvent{
                  BnwFreshMilitaryInitStep::city_specialization_pending,0} &&
              draft.source_order[6] == BnwFreshMilitaryInitEvent{
                  BnwFreshMilitaryInitStep::selected_strategies_and_player_bound,1},
          "MilitaryAI Init/Reset stages each player before the next one");
    const auto &first = draft.players.front();
    check(first.using_strategy == std::vector<bool>({false,false}) &&
              first.turn_strategy_adopted ==
                  std::vector<std::int32_t>({-1,-1}) &&
              first.temp_flavors.size() == 2 &&
              !first.temp_flavors[0] &&
              first.last_war_state.size() == 3 &&
              first.last_war_state[0] == -1 &&
              first.last_war_state[1] == -1 &&
              !first.last_war_state[2] &&
              first.total_threat_weight == 1 &&
              first.land_defense_state == -1 &&
              draft.has_blocker(
                  BnwFreshMilitaryInitBlockerCode::diplomacy_handle_and_remaining_ai_pending),
          "Reset writes only strategy-count prefix of MAX_CIV_PLAYERS arrays");
    selected.rules->apply_sql(
        "INSERT INTO AIMilitaryStrategies VALUES(3,'STRATEGY_3');");
    const auto changed_sha = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    player_info.selected_rules_sha256 = changed_sha;
    economic.selected_rules_sha256 = changed_sha;
    const auto overflow = draft_bnw_fresh_military_ai_init(
        selected,player_info,economic,changed_sha);
    check(overflow.has_blocker(
              BnwFreshMilitaryInitBlockerCode::sdk_opponent_array_overflow) &&
              overflow.players.empty(),
          "SDK Reset indexing cannot overrun selected MAX_CIV_PLAYERS allocation");
    draft.rollback();
    check(draft.players.empty() && draft.source_order.empty() &&
              !draft.arrays_and_reset_complete &&
              economic.players.size() == 2,
          "Rollback leaves selected EconomicAI state untouched");
}
} // namespace

int main() { test_selected_fresh_military_ai_reset(); }
