#include "civ5/runtime/bnw_fresh_player_info_reset.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_selected_player_info_arrays() {
    using namespace civ5::runtime;
    BnwFreshGameContentResult selected;
    selected.profile.kind = ContentProfileKind::dlc;
    selected.rules = std::make_unique<civ5::content::RulesDatabase>();
    constexpr const char *tables[] = {
        "Resources","Improvements","UnitCombatInfos","Buildings",
        "UnitPromotions","UnitClasses","BuildingClasses","Projects",
        "HurryInfos","VoteSources"};
    for (const auto *table : tables) {
        selected.rules->apply_sql(
            std::string("CREATE TABLE ") + table +
            "(ID INTEGER PRIMARY KEY, Type TEXT); INSERT INTO " + table +
            " VALUES(0,'ROW_0');");
    }
    selected.rules->apply_sql(
        "INSERT INTO Resources VALUES(2,'ROW_2');"
        "INSERT INTO Improvements VALUES(1,'ROW_1');");
    selected.activation.emplace();
    selected.activation->profile_kind = ContentProfileKind::dlc;
    selected.activation->rules_database_ready = true;
    const auto sha = identify_bytes(selected.rules->gameplay_fingerprint_data()).sha256;
    BnwFreshPlayerResetPrefix early;
    early.early_arrays_complete = true;
    early.selected_rules_sha256 = sha;
    early.players.resize(2);
    early.players[0].id = 0;
    early.players[1].id = 1;
    early.blockers.push_back({
        BnwFreshPlayerResetBlockerCode::resource_info_and_remaining_reset_pending,
        "fixture"});
    const std::vector<civ5::content::GameplayDefine> constants{
        {"MAX_CIV_PLAYERS",1}};
    auto draft = draft_bnw_fresh_player_info_reset(
        selected,early,sha,constants);
    check(draft.selected_arrays_complete && !draft.initialized_by_gamecore &&
              draft.players.size() == 2 && draft.source_order.size() == 46 &&
              draft.sizes.resources == 3 && draft.sizes.improvements == 2 &&
              draft.source_order[0] == BnwFreshPlayerInfoEvent{
                  BnwFreshPlayerInfoArray::early_reset_prefix,0} &&
              draft.source_order[22] == BnwFreshPlayerInfoEvent{
                  BnwFreshPlayerInfoArray::ai_services_pending,0} &&
              draft.source_order[23] == BnwFreshPlayerInfoEvent{
                  BnwFreshPlayerInfoArray::early_reset_prefix,1},
          "Info slots including ID holes and per-player source order are preserved");
    const auto &player = draft.players[0];
    check(player.resource_used == std::vector<std::int32_t>({0,0,0}) &&
              player.improvement_count == std::vector<std::int32_t>({0,0}) &&
              player.loyal_member == std::vector<bool>({true}) &&
              player.gets_science_from_player == std::vector<bool>({false}) &&
              player.early.id == 0 &&
              draft.has_blocker(
                  BnwFreshPlayerInfoBlockerCode::ai_services_and_remaining_reset_pending),
          "SDK zero and true defaults are owned without claiming AI completion");
    const auto limited = draft_bnw_fresh_player_info_reset(
        selected,early,sha,constants,1);
    check(limited.has_blocker(
              BnwFreshPlayerInfoBlockerCode::allocation_budget_exceeded) &&
              limited.players.empty(),
          "Array budget refusal does not expose a partial player");
    selected.rules->apply_sql("DROP TABLE Improvements;");
    const auto changed_sha = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    early.selected_rules_sha256 = changed_sha;
    const auto missing = draft_bnw_fresh_player_info_reset(
        selected,early,changed_sha,constants);
    check(missing.has_blocker(
              BnwFreshPlayerInfoBlockerCode::selected_info_collections_unverified) &&
              missing.players.empty(),
          "Missing selected Improvement Info blocks all player arrays");
    draft.rollback();
    check(draft.players.empty() && draft.source_order.empty() &&
              !draft.selected_arrays_complete && early.players.size() == 2,
          "Rollback leaves the caller-owned early reset intact");
}
} // namespace

int main() { test_selected_player_info_arrays(); }
