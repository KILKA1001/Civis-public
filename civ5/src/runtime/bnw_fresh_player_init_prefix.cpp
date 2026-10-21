#include "civ5/runtime/bnw_fresh_player_init_prefix.hpp"
#include <algorithm>
#include <utility>

namespace civ5::runtime {
namespace {
void block(BnwFreshPlayerInitPrefix &result,
           BnwFreshPlayerInitBlockerCode code, std::string message) {
    result.blockers.push_back({code, std::move(message)});
}
} // namespace

bool BnwFreshPlayerInitPrefix::has_blocker(
    BnwFreshPlayerInitBlockerCode code) const {
    return std::any_of(blockers.begin(), blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshPlayerInitPrefix::rollback() noexcept {
    team_members.clear();
    planned_alive.clear();
    major_colors.clear();
    source_order.clear();
    member_prefix_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshPlayerInitPrefix draft_bnw_fresh_player_init_prefix(
    const BnwFreshPregameRosterDraft &roster,
    const BnwFreshPlayerColorsDraft &colors) {
    BnwFreshPlayerInitPrefix result;
    if (!roster.slot_prefix_complete || roster.initialized_by_gamecore ||
        !roster.has_blocker(BnwFreshRosterBlockerCode::player_ai_init_pending) ||
        !colors.major_colors_complete ||
        !colors.has_blocker(BnwFreshColorsBlockerCode::remaining_init_players_pending) ||
        roster.selected_rules_sha256.empty() ||
        colors.selected_rules_sha256 != roster.selected_rules_sha256 ||
        roster.slots.empty() || roster.teams_reset.empty() ||
        colors.major_colors.empty() ||
        colors.major_colors.size() > roster.slots.size()) {
        block(result, BnwFreshPlayerInitBlockerCode::pregame_roster_or_colors_pending,
              "Selected major colors and detached CvGame::InitPlayers slots are required");
        return result;
    }
    result.selected_rules_sha256 = roster.selected_rules_sha256;
    result.team_members.assign(roster.teams_reset.size(), 0);
    result.planned_alive.assign(roster.slots.size(), false);
    result.major_colors = colors.major_colors;
    // CvGame::InitPlayers invokes CvPlayerAI::init in ascending player ID.
    // Its inherited CvPlayer::init allocates major-only notifications, resets
    // serialized data, initializes containers, sets up graphics, increments
    // team membership, then calls setAlive for TAKEN/COMPUTER slots.
    for (std::size_t index = 0; index < roster.slots.size(); ++index) {
        const auto player = static_cast<std::int32_t>(index);
        const auto team = roster.slots[index].team_id;
        if (team < 0 || static_cast<std::size_t>(team) >= result.team_members.size()) {
            result.rollback();
            block(result, BnwFreshPlayerInitBlockerCode::pregame_roster_or_colors_pending,
                  "A selected pregame player has no valid team");
            return result;
        }
        const auto append = [&](BnwFreshPlayerInitStep step) {
            result.source_order.push_back({step, player, team});
        };
        if (index < colors.major_colors.size())
            append(BnwFreshPlayerInitStep::notifications_pending);
        append(BnwFreshPlayerInitStep::reset_pending);
        append(BnwFreshPlayerInitStep::containers_pending);
        append(BnwFreshPlayerInitStep::setup_graphical_pending);
        ++result.team_members[static_cast<std::size_t>(team)];
        append(BnwFreshPlayerInitStep::team_member_increment);
        const auto status = roster.slots[index].status;
        if (status == BnwFreshSlotStatus::taken ||
            status == BnwFreshSlotStatus::computer) {
            result.planned_alive[index] = true;
            append(BnwFreshPlayerInitStep::set_alive_pending);
        }
        append(BnwFreshPlayerInitStep::ai_init_pending);
    }
    result.member_prefix_complete = true;
    block(result, BnwFreshPlayerInitBlockerCode::notifications_and_containers_pending,
          "Major notification objects and all player containers must initialize");
    block(result, BnwFreshPlayerInitBlockerCode::player_reset_pending,
          "Full selected CvPlayer::reset serialized defaults remain pending");
    block(result, BnwFreshPlayerInitBlockerCode::setup_graphical_pending,
          "CvPlayer::setupGraphical host callback remains pending");
    block(result, BnwFreshPlayerInitBlockerCode::set_alive_callbacks_pending,
          "setAlive must update leagues, minor AI, team alive/ever-alive, turns and DLL slots");
    block(result, BnwFreshPlayerInitBlockerCode::ai_init_pending,
          "CvPlayerAI::AI_init and later player state remain pending");
    return result;
}
} // namespace civ5::runtime
