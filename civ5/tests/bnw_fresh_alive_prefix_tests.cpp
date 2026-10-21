#include "civ5/runtime/bnw_fresh_alive_prefix.hpp"
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_first_alive_before_league() {
    using namespace civ5::runtime;
    BnwFreshPregameRosterDraft roster;
    roster.slot_prefix_complete = true;
    roster.selected_rules_sha256 = "selected-rules-fixture";
    roster.teams_reset = {false, false};
    roster.slots = {{BnwFreshSlotStatus::closed,0},
                    {BnwFreshSlotStatus::taken,1},
                    {BnwFreshSlotStatus::computer,1}};
    BnwFreshPlayerInitPrefix prefix;
    prefix.member_prefix_complete = true;
    prefix.selected_rules_sha256 = roster.selected_rules_sha256;
    prefix.team_members = {1,2};
    prefix.planned_alive = {false,true,true};
    prefix.blockers.push_back({
        BnwFreshPlayerInitBlockerCode::set_alive_callbacks_pending,"fixture"});
    auto before_league = draft_bnw_fresh_first_alive_before_league(
        roster, prefix);
    check(before_league.pre_league_prefix_complete &&
              !before_league.initialized_by_gamecore &&
              before_league.first_active_player == 1 &&
              before_league.team_members_at_boundary ==
                  std::vector<std::int32_t>({1,1}) &&
              before_league.team_alive_at_boundary ==
                  std::vector<std::int32_t>({0,1}) &&
              before_league.player_alive_at_boundary ==
                  std::vector<bool>({false,true,false}) &&
              before_league.source_order == std::vector<BnwFreshAliveEvent>({
                  {BnwFreshAliveStep::prior_player_member_increment,0,0},
                  {BnwFreshAliveStep::current_player_member_increment,1,1},
                  {BnwFreshAliveStep::player_alive_written,1,1},
                  {BnwFreshAliveStep::team_alive_incremented,1,1},
                  {BnwFreshAliveStep::league_callback_pending,1,1}}) &&
              before_league.has_blocker(BnwFreshAliveBlockerCode::league_callback_pending),
          "First alive player stops at the mandatory league callback in exact order");
    auto mismatch = prefix;
    mismatch.planned_alive[0] = true;
    const auto refused = draft_bnw_fresh_first_alive_before_league(
        roster, mismatch);
    check(refused.has_blocker(BnwFreshAliveBlockerCode::init_prefix_unverified) &&
              refused.player_alive_at_boundary.empty(),
          "Inconsistent player init plan cannot produce partial alive state");
    auto no_active = roster;
    no_active.slots[1].status = BnwFreshSlotStatus::closed;
    no_active.slots[2].status = BnwFreshSlotStatus::closed;
    auto inactive_prefix = prefix;
    inactive_prefix.planned_alive = {false,false,false};
    const auto empty = draft_bnw_fresh_first_alive_before_league(
        no_active, inactive_prefix);
    check(empty.has_blocker(BnwFreshAliveBlockerCode::no_active_player) &&
              empty.player_alive_at_boundary.empty(),
          "No initialized player may be inferred from closed slots");
    before_league.rollback();
    check(!before_league.pre_league_prefix_complete &&
              before_league.team_members_at_boundary.empty() &&
              !before_league.initialized_by_gamecore &&
              roster.slots[1].status == BnwFreshSlotStatus::taken,
          "Rollback leaves caller-owned pregame roster untouched");
}
} // namespace

int main() { test_first_alive_before_league(); }
