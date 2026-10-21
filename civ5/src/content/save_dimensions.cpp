#include "civ5/content/save_dimensions.hpp"
#include "civ5/saves/vp_all_players_profile.hpp"
#include "civ5/saves/vp_game_progression_profile.hpp"
#include "civ5/saves/vp_roster_profile.hpp"
#include <stdexcept>

namespace civ5::content {
namespace {
std::uint64_t source_span(const RulesDatabase &rules, const char *table) {
    const auto collection = rules.prefetch_info(table);
    if (!collection.all_succeeded())
        throw std::runtime_error(std::string("Cannot read source rules collection ") + table + ": " +
                                 collection.error);
    return collection.slots;
}
} // namespace
VpSaveWireDimensions vp_save_wire_dimensions(const RulesDatabase &rules, std::uint64_t player_slots,
                                             std::uint64_t team_slots) {
    if (!player_slots || !team_slots)
        throw std::runtime_error("Missing source GameCore slot dimensions");
    const auto span = [&](const char *table) { return source_span(rules, table); };
    VpSaveWireDimensions result;
    result.roster = {player_slots, team_slots};
    result.roster.profile_extensions.emplace<saves::VpRosterWireDimensions>(
        saves::VpRosterWireDimensions{span("Resources")});
    result.progression.vote_team_slots = team_slots;
    auto &vp_progression = result.progression.profile_extensions.emplace<saves::VpGameProgressionWireDimensions>();
    constexpr std::array tables{"Units",        "UnitClasses", "BuildingClasses", "Projects",
                                "SpecialUnits", "Victories",   "Contracts"};
    for (std::size_t i = 0; i < tables.size(); ++i)
        vp_progression.rows[i] = span(tables[i]);
    const auto count =
        rules.query({"SELECT Value FROM Defines WHERE Name='NUM_VICTORY_POINT_AWARDS'", {}}, [&](auto row) {
            const auto value = std::get_if<std::int64_t>(&row[0]);
            if (!value || *value <= 0)
                throw std::runtime_error("Invalid victory award dimension");
            result.progression.victory_award_slots = static_cast<std::uint64_t>(*value);
        });
    if (count != 1)
        throw std::runtime_error("Missing or ambiguous victory award dimension");
    return result;
}
saves::VpTeamContentDimensions vp_team_content_dimensions(const RulesDatabase &rules) {
    const auto span = [&](const char *table) { return source_span(rules, table); };
    return {span("VoteSources"), span("SmallAwards"), span("Technologies"), span("Routes"),
            span("Builds"),      span("Projects"),    span("UnitClasses"),  span("BuildingClasses"),
            span("Buildings"),   span("Terrains"),    span("Features"),     span("Improvements"),
            span("Victories")};
}
saves::VpPlayerStrategyWireDimensions player_strategy_content_dimensions(const RulesDatabase &rules) {
    return {source_span(rules, "Policies"),
            source_span(rules, "PolicyBranchTypes"),
            source_span(rules, "Flavors"),
            source_span(rules, "AIEconomicStrategies"),
            source_span(rules, "AIMilitaryStrategies"),
            source_span(rules, "AIGrandStrategies")};
}
saves::VpPlayerKnowledgeWireDimensions player_knowledge_content_dimensions(const RulesDatabase &rules) {
    return {source_span(rules, "Technologies"), source_span(rules, "Flavors")};
}
saves::SavedSessionDecodeContext vp_saved_session_context(const RulesDatabase &rules,
                                                          const VpSessionSourceDimensions &source) {
    for (const auto size : {source.players, source.teams, source.civ_players, source.civ_teams,
                           source.major, source.fixed_players, source.fixed_teams, source.yields,
                           source.domains, source.terrain_types, source.approaches,
                           source.minor_approaches, source.statements, source.log_entries})
        if (!size)
            throw std::runtime_error("VP session source dimensions are incomplete");
    if (source.civ_players > source.players || source.civ_teams > source.teams ||
        source.major > source.civ_players || source.minor != source.civ_players - source.major)
        throw std::runtime_error("VP session source dimensions disagree");
    const auto progression = vp_save_wire_dimensions(rules, source.players, source.teams);
    const auto team = vp_team_content_dimensions(rules);
    const auto strategy = player_strategy_content_dimensions(rules);
    saves::SavedSessionDecodeContext result;
    result.layout = saves::GameStateLayout::vp542_corpus;
    result.roster = progression.roster;
    result.progression = progression.progression;
    result.trade = {source.major, source.yields};
    result.plots = {source.players, source.teams, source.major, source.yields, 0};
    result.regions = {source.fixed_players, source.fixed_teams, source.players, source.yields};
    result.teams = {source.teams, source.players, source.civ_teams, source.fixed_players,
                    source.fixed_teams, source.domains, source.major, source.yields, {}};
    result.teams.profile_extensions.emplace<saves::VpTeamContentDimensions>(team);
    result.decode_map_regions = true;
    result.decode_teams = true;
    result.decode_all_players = true;
    result.decode_supporting_classes = true;
    saves::AllPlayersContext players;
    players.player_count = source.players;
    players.strategy = {source.major, source.civ_players, source.yields, strategy.grand, {}};
    players.strategy.profile_extensions.emplace<saves::VpPlayerStrategyWireDimensions>(strategy);
    players.diplomacy = {source.major, source.minor, source.civ_players, source.players,
                         source.approaches, source.minor_approaches, source.statements, source.log_entries};
    players.profile_extensions.emplace<saves::VpPlayerKnowledgeWireDimensions>(
        player_knowledge_content_dimensions(rules));
    players.planning = {source.major, source.civ_teams, source.fixed_teams};
    players.traits = saves::PlayerTraitsWireDimensions{source.yields, source.terrain_types};
    players.trade_culture = saves::PlayerTradeCultureWireDimensions{source.major, source.yields};
    players.profile_extensions.emplace<saves::VpAllPlayersWireDimensions>(
        saves::VpAllPlayersWireDimensions{source_span(rules, "AICityStrategies"),
                                         source_span(rules, "EmphasizeInfos"), team.buildings});
    result.all_players = std::move(players);
    return result;
}
} // namespace civ5::content
