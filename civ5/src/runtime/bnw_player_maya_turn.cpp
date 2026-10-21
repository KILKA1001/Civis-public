#include "civ5/runtime/bnw_player_maya_turn.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/content/rules_database.hpp"
#include <optional>
#include <string>
#include <string_view>

namespace civ5::runtime {
namespace {
using B = BnwMayaTurnBlocker;
void require(bool condition, B blocker, const char *message) {
    if (!condition) throw BnwMayaTurnError(blocker, message);
}
std::string wire_key(const std::string &wire) {
    return wire.substr(0, wire.find('\0'));
}
std::optional<std::string> first_calendar_trait(const content::RulesDatabase &rules) {
    std::optional<std::string> result;
    const auto report = rules.query_rows(
        {"SELECT Type FROM Traits WHERE MayaCalendarBonuses=1 ORDER BY ID LIMIT 1", {}},
        [&](const content::RulesRowView &row) {
            const auto type = row.text("Type");
            require(type && !type->empty(), B::selected_rules,
                    "selected BNW Maya trait lacks Type");
            result = std::string(*type);
        }, 1);
    require(report.all_succeeded(), B::selected_rules,
            "selected BNW Maya trait lookup failed");
    return result;
}
bool leader_has_trait(const content::RulesDatabase &rules,
                      const std::string &leader, const std::string &trait) {
    std::int64_t leader_rows = 0;
    auto report = rules.query_rows(
        {"SELECT ID FROM Leaders WHERE Type=? LIMIT 1", {leader}},
        [&](const content::RulesRowView &) { ++leader_rows; }, 1);
    require(report.all_succeeded() && leader_rows == 1, B::leader,
            "saved BNW leader absent from selected Info");
    std::int64_t relations = 0;
    report = rules.query_rows(
        {"SELECT LeaderType FROM Leader_Traits WHERE LeaderType=? AND TraitType=? LIMIT 1",
         {leader, trait}},
        [&](const content::RulesRowView &) { ++relations; }, 1);
    require(report.all_succeeded(), B::selected_rules,
            "selected BNW leader trait relation unavailable");
    return relations == 1;
}
} // namespace

BnwMayaTurnStep apply_bnw_player_maya_turn(
    LiveSessionState &state, std::int64_t player_id,
    const content::RulesDatabase &rules, const SessionCheckpointIdentity &identity) {
    constexpr auto bnw = saves::GameStateLayout::expansion2_version1;
    require(state.setup.profile == content::TechnologyProfile::bnw &&
            state.world.game_state.layout == bnw &&
            state.world.pregame && state.world.pregame->layout == bnw &&
            state.world.all_players && state.world.all_players->layout == bnw,
            B::profile, "BNW Maya turn requires restored Expansion2 world");
    require(!identity.rules_sha256.empty() &&
            identify_bytes(rules.gameplay_fingerprint_data()).sha256 == identity.rules_sha256,
            B::selected_rules, "BNW Maya selected rules identity differs");
    const auto setup = state.setup.players.find(player_id);
    require(setup != state.setup.players.end() && setup->second.alive && player_id >= 0,
            B::player, "BNW Maya turn requires alive restored player");
    std::size_t saved_count = 0;
    for (const auto &candidate : state.world.all_players->players)
        if (candidate.saved_player_id == player_id && candidate.core.layout == bnw)
            ++saved_count;
    require(saved_count == 1, B::player, "BNW Maya saved player identity unresolved");
    const auto it = state.world.pregame->slots.string_arrays.find("leader_keys");
    require(it != state.world.pregame->slots.string_arrays.end() &&
            static_cast<std::uint64_t>(player_id) < it->second.size(),
            B::leader, "BNW Maya saved leader slot unavailable");
    const auto leader = wire_key(it->second[static_cast<std::size_t>(player_id)]);
    require(!leader.empty(), B::leader, "BNW Maya saved leader key absent");
    const auto turn = state.research_turn.players.find(player_id);
    require(state.research_turn.profile == content::TechnologyProfile::bnw &&
            turn != state.research_turn.players.end() &&
            turn->second.leader_type && wire_key(*turn->second.leader_type) == leader,
            B::leader, "BNW Maya live leader differs from saved CvPreGame");
    const auto trait = first_calendar_trait(rules);
    if (!trait) {
        // Source IsUsingMayaCalendar returns false when no matching trait Info
        // exists. The leader still has to resolve under the selected rules.
        (void)leader_has_trait(rules, leader, "");
        return {player_id, false};
    }
    if (!leader_has_trait(rules, leader, *trait)) return {player_id, false};
    throw BnwMayaTurnError(B::maya_calendar_and_boost,
        "BNW Maya calendar date, saved trait transition and boost callback unavailable");
}
} // namespace civ5::runtime
