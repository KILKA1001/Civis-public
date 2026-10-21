#include "civ5/runtime/bnw_fresh_player_init_prefix.hpp"
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_player_init_prefix() {
    using namespace civ5::runtime;
    BnwFreshPregameRosterDraft roster;
    roster.slot_prefix_complete = true;
    roster.selected_rules_sha256 = "selected-rules-fixture";
    roster.teams_reset = {false, false};
    roster.slots = {{BnwFreshSlotStatus::taken,0},
                    {BnwFreshSlotStatus::computer,1},
                    {BnwFreshSlotStatus::closed,0},
                    {BnwFreshSlotStatus::computer,1}};
    roster.blockers.push_back({BnwFreshRosterBlockerCode::player_ai_init_pending,
                               "fixture"});
    BnwFreshPlayerColorsDraft colors;
    colors.major_colors_complete = true;
    colors.major_colors = {3, 4};
    colors.selected_rules_sha256 = roster.selected_rules_sha256;
    colors.blockers.push_back({
        BnwFreshColorsBlockerCode::remaining_init_players_pending,"fixture"});
    auto prefix = draft_bnw_fresh_player_init_prefix(roster, colors);
    check(prefix.member_prefix_complete && !prefix.initialized_by_gamecore &&
              prefix.team_members == std::vector<std::int32_t>({2,2}) &&
              prefix.planned_alive == std::vector<bool>({true,true,false,true}) &&
              prefix.major_colors == colors.major_colors &&
              prefix.source_order.size() == 25 &&
              prefix.source_order[0] == BnwFreshPlayerInitEvent{
                  BnwFreshPlayerInitStep::notifications_pending,0,0} &&
              prefix.source_order[4] == BnwFreshPlayerInitEvent{
                  BnwFreshPlayerInitStep::team_member_increment,0,0} &&
              prefix.source_order[5] == BnwFreshPlayerInitEvent{
                  BnwFreshPlayerInitStep::set_alive_pending,0,0} &&
              prefix.has_blocker(
                  BnwFreshPlayerInitBlockerCode::set_alive_callbacks_pending),
          "Player init visits every slot and stages member/alive effects in SDK order");
    auto wrong_colors = colors;
    wrong_colors.selected_rules_sha256 = "other-rules";
    const auto refused = draft_bnw_fresh_player_init_prefix(
        roster, wrong_colors);
    check(refused.has_blocker(
              BnwFreshPlayerInitBlockerCode::pregame_roster_or_colors_pending) &&
              refused.team_members.empty(),
          "Mismatched selected colors cannot enter player init");
    prefix.rollback();
    check(!prefix.member_prefix_complete && !prefix.initialized_by_gamecore &&
              prefix.team_members.empty() && prefix.planned_alive.empty() &&
              roster.slots[0].status == BnwFreshSlotStatus::taken,
          "Rollback discards detached player init prefix without changing pregame");
}
} // namespace

int main() { test_player_init_prefix(); }
