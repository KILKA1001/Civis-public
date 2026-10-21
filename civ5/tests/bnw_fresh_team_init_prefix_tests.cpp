#include "civ5/runtime/bnw_fresh_team_init_prefix.hpp"
#include <algorithm>
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_fresh_team_fixed_reset() {
    using namespace civ5::runtime;
    BnwFreshPregameRosterDraft roster;
    roster.slot_prefix_complete = true;
    roster.selected_rules_sha256 = "selected-bnw-rules";
    roster.teams_reset = {false,false};
    roster.slots.resize(3);
    roster.source_order = {
        {BnwFreshRosterEventKind::team_reset,0},
        {BnwFreshRosterEventKind::team_reset,1},
        {BnwFreshRosterEventKind::major_color_pass_pending,-1}};
    const std::vector<civ5::content::GameplayDefine> constants{
        {"MAX_TEAMS",2},{"MAX_PLAYERS",3},{"NUM_DOMAIN_TYPES",5}};
    auto draft = draft_bnw_fresh_team_init_fixed_prefix(
        roster,roster.selected_rules_sha256,constants);
    check(draft.fixed_reset_complete && !draft.initialized_by_gamecore &&
              draft.teams.size() == 2 && draft.source_order.size() == 12 &&
              draft.source_order[0] == BnwFreshTeamInitEvent{
                  BnwFreshTeamInitStep::batch_free_and_pointer_clear,0} &&
              draft.source_order[4] == BnwFreshTeamInitEvent{
                  BnwFreshTeamInitStep::team_id_write,0} &&
              draft.source_order[5] == BnwFreshTeamInitEvent{
                  BnwFreshTeamInitStep::selected_info_batch_pending,0} &&
              draft.source_order[6] == BnwFreshTeamInitEvent{
                  BnwFreshTeamInitStep::batch_free_and_pointer_clear,1},
          "Each Team::init fixed reset is staged before the next team");
    const auto &team = draft.teams[1];
    check(team.id == 1 && team.num_members == 0 &&
              team.alive_count == 0 && team.ever_alive_count == 0 &&
              team.best_possible_route == -1 && team.current_era == 0 &&
              team.liberated_by_team == -1 && team.killed_by_team == -1 &&
              team.relations.size() == 2 &&
              team.relations[0].peace_treaty_turn == -1 &&
              !team.relations[0].has_met &&
              team.found_players_territory == std::vector<bool>({false,false,false}) &&
              std::all_of(team.remaining_zero_counters.begin(),
                          team.remaining_zero_counters.end(),
                          [](std::int32_t value) { return value == 0; }) &&
              !team.dynamic_info_arrays_allocated &&
              draft.has_blocker(BnwFreshTeamInitBlockerCode::selected_info_batch_pending) &&
              draft.has_blocker(BnwFreshTeamInitBlockerCode::team_techs_init_pending),
          "Fresh scalar and fixed arrays are owned; selected arrays remain blocked");
    auto wrong_order = roster;
    wrong_order.source_order[0].index = 1;
    auto refused = draft_bnw_fresh_team_init_fixed_prefix(
        wrong_order,roster.selected_rules_sha256,constants);
    check(refused.has_blocker(
              BnwFreshTeamInitBlockerCode::roster_or_native_constants_unverified) &&
              refused.teams.empty(),
          "Invalid source order cannot expose a partial Team array");
    auto bad_constants = constants;
    bad_constants[2].value = 4;
    refused = draft_bnw_fresh_team_init_fixed_prefix(
        roster,roster.selected_rules_sha256,bad_constants);
    check(refused.has_blocker(
              BnwFreshTeamInitBlockerCode::roster_or_native_constants_unverified) &&
              refused.teams.empty(),
          "Compiled DomainTypes dimension must match the Expansion2 SDK");
    draft.rollback();
    check(draft.teams.empty() && draft.source_order.empty() &&
              !draft.fixed_reset_complete &&
              roster.slots.size() == 3,
          "Rollback discards Team fixed state without changing the pregame roster");
}
} // namespace

int main() { test_fresh_team_fixed_reset(); }
