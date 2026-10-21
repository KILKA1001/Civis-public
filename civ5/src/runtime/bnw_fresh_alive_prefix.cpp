#include "civ5/runtime/bnw_fresh_alive_prefix.hpp"
#include <algorithm>
#include <utility>

namespace civ5::runtime {
namespace {
void block(BnwFreshAliveBeforeLeague &result,
           BnwFreshAliveBlockerCode code, std::string message) {
    result.blockers.push_back({code, std::move(message)});
}
} // namespace

bool BnwFreshAliveBeforeLeague::has_blocker(BnwFreshAliveBlockerCode code) const {
    return std::any_of(blockers.begin(), blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshAliveBeforeLeague::rollback() noexcept {
    team_members_at_boundary.clear();
    team_alive_at_boundary.clear();
    player_alive_at_boundary.clear();
    source_order.clear();
    first_active_player = -1;
    pre_league_prefix_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshAliveBeforeLeague draft_bnw_fresh_first_alive_before_league(
    const BnwFreshPregameRosterDraft &roster,
    const BnwFreshPlayerInitPrefix &init_prefix) {
    BnwFreshAliveBeforeLeague result;
    if (!roster.slot_prefix_complete || roster.initialized_by_gamecore ||
        !init_prefix.member_prefix_complete ||
        init_prefix.initialized_by_gamecore ||
        roster.selected_rules_sha256.empty() ||
        init_prefix.selected_rules_sha256 != roster.selected_rules_sha256 ||
        !init_prefix.has_blocker(
            BnwFreshPlayerInitBlockerCode::set_alive_callbacks_pending) ||
        roster.slots.size() != init_prefix.planned_alive.size() ||
        roster.teams_reset.size() != init_prefix.team_members.size()) {
        block(result, BnwFreshAliveBlockerCode::init_prefix_unverified,
              "Selected CvPlayerAI init prefix and pregame slots are required");
        return result;
    }
    result.selected_rules_sha256 = roster.selected_rules_sha256;
    result.team_members_at_boundary.assign(roster.teams_reset.size(), 0);
    result.team_alive_at_boundary.assign(roster.teams_reset.size(), 0);
    result.player_alive_at_boundary.assign(roster.slots.size(), false);
    for (std::size_t index = 0; index < roster.slots.size(); ++index) {
        const auto status = roster.slots[index].status;
        const bool active = status == BnwFreshSlotStatus::taken ||
                            status == BnwFreshSlotStatus::computer;
        if (init_prefix.planned_alive[index] != active) {
            result.rollback();
            block(result, BnwFreshAliveBlockerCode::init_prefix_unverified,
                  "Selected slot activation differs from CvPlayerAI init plan");
            return result;
        }
        const auto team = roster.slots[index].team_id;
        if (team < 0 || static_cast<std::size_t>(team) >=
                            result.team_members_at_boundary.size()) {
            result.rollback();
            block(result, BnwFreshAliveBlockerCode::init_prefix_unverified,
                  "Selected player team is outside initialized Team slots");
            return result;
        }
        const auto player = static_cast<std::int32_t>(index);
        ++result.team_members_at_boundary[static_cast<std::size_t>(team)];
        result.source_order.push_back({
            active ? BnwFreshAliveStep::current_player_member_increment
                   : BnwFreshAliveStep::prior_player_member_increment,
            player, team});
        if (!active) continue;
        result.first_active_player = player;
        result.player_alive_at_boundary[index] = true;
        result.source_order.push_back({BnwFreshAliveStep::player_alive_written,
                                       player, team});
        ++result.team_alive_at_boundary[static_cast<std::size_t>(team)];
        result.source_order.push_back({BnwFreshAliveStep::team_alive_incremented,
                                       player, team});
        result.source_order.push_back({BnwFreshAliveStep::league_callback_pending,
                                       player, team});
        result.pre_league_prefix_complete = true;
        block(result, BnwFreshAliveBlockerCode::player_reset_graphics_pending,
              "CvPlayer reset, containers and setupGraphical precede this detached prefix");
        block(result, BnwFreshAliveBlockerCode::league_callback_pending,
              "GameLeagues::DoPlayerAliveStatusChanged updates diplomatic victory before ever-alive state");
        return result;
    }
    result.rollback();
    block(result, BnwFreshAliveBlockerCode::no_active_player,
          "No TAKEN or COMPUTER player enters setAlive during fresh InitPlayers");
    return result;
}
} // namespace civ5::runtime
