#include "civ5/runtime/bnw_fresh_team_init_prefix.hpp"
#include <algorithm>
#include <limits>
#include <optional>
#include <utility>

namespace civ5::runtime {
namespace {
std::optional<std::int32_t> native_constant(
    std::span<const content::GameplayDefine> constants, std::string_view name) {
    std::optional<std::int32_t> result;
    for (const auto &constant : constants) {
        if (constant.name != name) continue;
        if (result || constant.value <= 0 ||
            constant.value > std::numeric_limits<std::int32_t>::max())
            return std::nullopt;
        result = static_cast<std::int32_t>(constant.value);
    }
    return result;
}
void block(BnwFreshTeamInitPrefix &result,
           BnwFreshTeamInitBlockerCode code, std::string message) {
    result.blockers.push_back({code, std::move(message)});
}
} // namespace

bool BnwFreshTeamInitPrefix::has_blocker(
    BnwFreshTeamInitBlockerCode code) const {
    return std::any_of(blockers.begin(), blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshTeamInitPrefix::rollback() noexcept {
    teams.clear();
    source_order.clear();
    fixed_reset_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshTeamInitPrefix draft_bnw_fresh_team_init_fixed_prefix(
    const BnwFreshPregameRosterDraft &roster,
    std::string_view selected_rules_sha256,
    std::span<const content::GameplayDefine> native_constants) {
    BnwFreshTeamInitPrefix result;
    const auto max_teams = native_constant(native_constants, "MAX_TEAMS");
    const auto max_players = native_constant(native_constants, "MAX_PLAYERS");
    const auto domains = native_constant(native_constants, "NUM_DOMAIN_TYPES");
    if (!roster.slot_prefix_complete || roster.initialized_by_gamecore ||
        roster.selected_rules_sha256.empty() ||
        roster.selected_rules_sha256 != selected_rules_sha256 ||
        !max_teams || !max_players || !domains || *domains != 5 ||
        *max_teams > 4096 || *max_players > 4096 ||
        roster.teams_reset.size() != static_cast<std::size_t>(*max_teams) ||
        roster.slots.size() != static_cast<std::size_t>(*max_players) ||
        roster.source_order.size() < static_cast<std::size_t>(*max_teams) ||
        !std::all_of(roster.teams_reset.begin(), roster.teams_reset.end(),
                     [](bool initialized) { return !initialized; })) {
        block(result, BnwFreshTeamInitBlockerCode::roster_or_native_constants_unverified,
              "Selected pregame roster and Expansion2 fixed Team dimensions are required");
        return result;
    }
    for (std::int32_t i = 0; i < *max_teams; ++i) {
        if (roster.source_order[static_cast<std::size_t>(i)] !=
            BnwFreshRosterEvent{BnwFreshRosterEventKind::team_reset,i}) {
            block(result,
                  BnwFreshTeamInitBlockerCode::roster_or_native_constants_unverified,
                  "InitPlayers team reset order differs from SDK");
            return result;
        }
    }
    result.selected_rules_sha256 = roster.selected_rules_sha256;
    result.teams.reserve(static_cast<std::size_t>(*max_teams));
    result.source_order.reserve(static_cast<std::size_t>(*max_teams) * 6);
    for (std::int32_t id = 0; id < *max_teams; ++id) {
        BnwFreshTeamFixedState team;
        // Fresh CvTeam constructor had reset(0,true): no Info batch, and
        // CvTeamTechs arrays are null. reset(eID) frees/clears these first.
        result.source_order.push_back({
            BnwFreshTeamInitStep::batch_free_and_pointer_clear,id});
        result.source_order.push_back({BnwFreshTeamInitStep::team_techs_uninit,id});
        result.source_order.push_back({BnwFreshTeamInitStep::scalar_reset,id});
        team.relations.assign(static_cast<std::size_t>(*max_teams), {});
        team.found_players_territory.assign(static_cast<std::size_t>(*max_players),false);
        result.source_order.push_back({BnwFreshTeamInitStep::fixed_array_reset,id});
        team.id = id;
        result.source_order.push_back({BnwFreshTeamInitStep::team_id_write,id});
        result.source_order.push_back({BnwFreshTeamInitStep::selected_info_batch_pending,id});
        result.teams.push_back(std::move(team));
    }
    result.fixed_reset_complete = true;
    block(result, BnwFreshTeamInitBlockerCode::selected_info_batch_pending,
          "CvTeam::reset requires selected VoteSource/Victory/Route/Build/Project/Unit/Building/Terrain/Improvement Info arrays");
    block(result, BnwFreshTeamInitBlockerCode::team_techs_init_pending,
          "CvTeam::reset next calls CvTeamTechs::Init with selected GameTechs");
    return result;
}
} // namespace civ5::runtime
