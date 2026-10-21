#include "civ5/runtime/bnw_fresh_team_info_init.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_selected_team_info_and_tech_arrays() {
    using namespace civ5::runtime;
    BnwFreshGameContentResult selected;
    selected.profile.kind = ContentProfileKind::dlc;
    selected.rules = std::make_unique<civ5::content::RulesDatabase>();
    constexpr const char *tables[] = {
        "VoteSources","Victories","SmallAwards","Routes","Builds",
        "Projects","UnitClasses","BuildingClasses","Buildings",
        "Terrains","Improvements","Technologies"};
    for (const auto *table : tables) {
        selected.rules->apply_sql(
            std::string("CREATE TABLE ") + table +
            "(ID INTEGER PRIMARY KEY, Type TEXT); INSERT INTO " + table +
            " VALUES(0,'ROW_0');");
    }
    selected.rules->apply_sql(
        "INSERT INTO Victories VALUES(1,'ROW_1');"
        "INSERT INTO Technologies VALUES(2,'ROW_2');");
    selected.activation.emplace();
    selected.activation->profile_kind = ContentProfileKind::dlc;
    selected.activation->rules_database_ready = true;
    const auto sha = identify_bytes(selected.rules->gameplay_fingerprint_data()).sha256;
    BnwFreshTeamInitPrefix fixed;
    fixed.selected_rules_sha256 = sha;
    fixed.fixed_reset_complete = true;
    fixed.teams.resize(2);
    fixed.blockers.push_back({
        BnwFreshTeamInitBlockerCode::selected_info_batch_pending,"fixture"});
    fixed.blockers.push_back({
        BnwFreshTeamInitBlockerCode::team_techs_init_pending,"fixture"});
    constexpr BnwFreshTeamInitStep prefix[] = {
        BnwFreshTeamInitStep::batch_free_and_pointer_clear,
        BnwFreshTeamInitStep::team_techs_uninit,
        BnwFreshTeamInitStep::scalar_reset,
        BnwFreshTeamInitStep::fixed_array_reset,
        BnwFreshTeamInitStep::team_id_write,
        BnwFreshTeamInitStep::selected_info_batch_pending};
    for (std::int32_t id = 0; id < 2; ++id) {
        fixed.teams[static_cast<std::size_t>(id)].id = id;
        for (auto step : prefix) fixed.source_order.push_back({step,id});
    }
    auto draft = draft_bnw_fresh_team_info_init(selected,fixed,sha);
    check(draft.info_and_tech_arrays_complete &&
              !draft.initialized_by_gamecore &&
              draft.teams.size() == 2 &&
              draft.sizes.victories == 2 && draft.sizes.technologies == 3 &&
              draft.source_order.size() == 26 &&
              draft.source_order[0] == BnwFreshTeamInfoEvent{
                  BnwFreshTeamInfoStep::fixed_batch_clear,0} &&
              draft.source_order[12] == BnwFreshTeamInfoEvent{
                  BnwFreshTeamInfoStep::revealed_resources_cleared,0} &&
              draft.source_order[13] == BnwFreshTeamInfoEvent{
                  BnwFreshTeamInfoStep::fixed_batch_clear,1},
          "Selected Info slots and full Team reset events use source order");
    const auto &team = draft.teams.front();
    check(team.victory_countdown == std::vector<std::int32_t>({-1,-1}) &&
              team.has_tech == std::vector<bool>({false,false,false}) &&
              team.tech_count == std::vector<std::int32_t>({0,0,0}) &&
              team.improvement_yield.size() == 1 &&
              team.improvement_yield.front().size() == 6 &&
              team.turn_team_met == std::vector<std::int32_t>({-1,-1}) &&
              team.project_art_types.size() == 1 &&
              team.project_art_types.front().empty() &&
              team.revealed_resources.empty() &&
              team.fixed.dynamic_info_arrays_allocated &&
              draft.has_blocker(
                  BnwFreshTeamInfoBlockerCode::remaining_gamecore_init_pending),
          "GameTechs, yields, countdown and project art own selected arrays");
    auto budget_refused = draft_bnw_fresh_team_info_init(selected,fixed,sha,1);
    check(budget_refused.has_blocker(
              BnwFreshTeamInfoBlockerCode::allocation_budget_exceeded) &&
              budget_refused.teams.empty(),
          "Array budget rejection publishes no partial team");
    selected.rules->apply_sql("DROP TABLE Improvements;");
    const auto changed_sha = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    fixed.selected_rules_sha256 = changed_sha;
    auto missing = draft_bnw_fresh_team_info_init(selected,fixed,changed_sha);
    check(missing.has_blocker(
              BnwFreshTeamInfoBlockerCode::selected_info_collections_unverified) &&
              missing.teams.empty(),
          "A missing selected Info table blocks all Team arrays");
    draft.rollback();
    check(draft.teams.empty() && draft.source_order.empty() &&
              !draft.info_and_tech_arrays_complete &&
              fixed.teams.size() == 2,
          "Rollback does not mutate the fixed Team prefix");
}
} // namespace

int main() { test_selected_team_info_and_tech_arrays(); }
