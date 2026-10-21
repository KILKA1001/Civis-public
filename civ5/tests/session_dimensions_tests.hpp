#pragma once
#include "civ5/saves/vp_team_profile.hpp"
void session_dimensions_tests() {
    content::RulesDatabase rules;
    // Deliberately unlike the ten-mod corpus. Empty, sparse and duplicate
    // collections exercise source PrefetchCollection spans, not cached counts.
    for (const auto *table : {"Resources", "Units", "UnitClasses", "BuildingClasses", "Projects",
                              "SpecialUnits", "Victories", "Contracts", "VoteSources", "SmallAwards",
                              "Technologies", "Routes", "Builds", "Buildings", "Terrains", "Features",
                              "Improvements", "Policies", "PolicyBranchTypes", "Flavors",
                              "AIEconomicStrategies", "AIMilitaryStrategies", "AIGrandStrategies",
                              "AICityStrategies", "EmphasizeInfos"}) {
        rules.apply_sql(std::string("CREATE TABLE ") + table + "(ID INTEGER);");
        rules.apply_sql(std::string("INSERT INTO ") + table + " VALUES(0),(3),(3);");
    }
    rules.apply_sql("CREATE TABLE Defines(Name,Value);"
                    "INSERT INTO Defines VALUES('NUM_VICTORY_POINT_AWARDS',7);"
                    "DELETE FROM Contracts;"
                    "INSERT INTO Buildings VALUES(4294967297);"
                    "INSERT INTO Terrains VALUES(22);"
                    "INSERT INTO AICityStrategies VALUES(96);"
                    "INSERT INTO EmphasizeInfos VALUES(8);");
    const content::VpSessionSourceDimensions source{
        4097, 8193, 4096, 8192, 100, 3996, 9000, 9001, 19, 7, 9, 8, 6, 125, 80};
    const auto context = content::vp_saved_session_context(rules, source);
    check(context.layout == saves::GameStateLayout::vp542_corpus && context.decode_map_plots &&
              context.decode_map_regions && context.decode_teams && context.decode_all_players &&
              context.decode_supporting_classes && context.all_players,
          "Rules-derived session omitted part of the serialized state");
    check(context.roster.player_slots == 4097 && context.roster.team_slots == 8193 &&
              saves::vp_roster_dimensions(context.roster)->resource_slots == 5 &&
              context.progression.vote_team_slots == 8193 &&
              context.progression.victory_award_slots == 7 &&
              saves::vp_game_progression_dimensions(context.progression)->rows ==
                  std::array<std::uint64_t, 7>{5, 5, 5, 5, 5, 5, 0},
          "Session reused corpus dimensions or miscounted source collection spans");
    check(context.trade.major_slots == 100 && context.trade.yield_slots == 19 &&
              context.plots.players == 4097 && context.plots.teams == 8193 &&
              context.plots.major_civilizations == 100 && context.plots.yields == 19 &&
              context.regions.fixed_players == 9000 && context.regions.fixed_teams == 9001 &&
              context.regions.players == 4097 && context.regions.yields == 19,
          "Map/trade source dimensions became source capacity limits");
    const auto &team = context.teams;
    check(team.teams == 8193 && team.players == 4097 && team.civ_teams == 8192 &&
              team.fixed_players == 9000 && team.fixed_teams == 9001 && team.domains == 7 &&
              team.major == 100 && team.yields == 19 &&
              saves::vp_team_dimensions(team)->buildings == 4294967298ULL &&
              saves::vp_team_dimensions(team)->terrains == 23 && saves::vp_team_dimensions(team)->techs == 5,
          "Team source dimensions were narrowed or replaced by fixed enum lengths");
    const auto &p = *context.all_players;
    const auto &vp_p = *p.profile_extensions.find<saves::VpAllPlayersWireDimensions>();
    const auto *vp_strategy=saves::vp_player_strategy_dimensions(p.strategy);
    check(p.player_count == 4097 && p.strategy.major == 100 && p.strategy.civ_players == 4096 &&
              p.strategy.yields == 19 && vp_strategy && vp_strategy->grand == 5 &&
              vp_strategy->policies == 5 &&
              p.profile_extensions.find<saves::VpPlayerKnowledgeWireDimensions>()->techs == 5 &&
              p.profile_extensions.find<saves::VpPlayerKnowledgeWireDimensions>()->flavors == 5 &&
              p.diplomacy.minor == 3996 && p.diplomacy.players == 4097 && p.diplomacy.approaches == 8 &&
              p.diplomacy.minor_approaches == 6 && p.diplomacy.statements == 125 && p.diplomacy.log_entries == 80 &&
              p.planning.civ_teams == 8192 && p.planning.legacy_teams == 9001 &&
              p.traits->terrains == 9 && p.traits->yields == 19 && p.trade_culture->major == 100 &&
              p.trade_culture->yields == 19 && vp_p.city_strategies == 97 && vp_p.city_emphases == 9 &&
              vp_p.buildings == 4294967298ULL && !p.bnw_projects,
          "Player/city decode context diverged from source enums or activated rules");
    rules.apply_sql("DELETE FROM Buildings; INSERT INTO Buildings VALUES(0);"
                    "INSERT INTO Policies VALUES(31);");
    const auto changed = content::vp_saved_session_context(rules, source);
    const auto &changed_vp = *changed.all_players->profile_extensions.find<saves::VpAllPlayersWireDimensions>();
    check(saves::vp_team_dimensions(changed.teams)->buildings == 1 && changed_vp.buildings == 1 &&
              saves::vp_player_strategy_dimensions(changed.all_players->strategy)->policies == 32 &&
              vp_p.buildings == 4294967298ULL && vp_strategy->policies == 5,
          "Session dimensions retained an old rules cache or shared mutable references");
    rejects([&] { (void)content::vp_saved_session_context(rules, {}); });
    auto bad = source;
    bad.minor = UINT64_MAX;
    rejects([&] { (void)content::vp_saved_session_context(rules, bad); });
    bad = source;
    bad.civ_teams = source.teams + 1;
    rejects([&] { (void)content::vp_saved_session_context(rules, bad); });
    rules.apply_sql("DROP TABLE AICityStrategies;");
    rejects([&] { (void)content::vp_saved_session_context(rules, source); });
    rules.apply_sql("CREATE TABLE AICityStrategies(ID); INSERT INTO AICityStrategies VALUES('wrong');");
    check(content::vp_saved_session_context(rules, source).all_players->profile_extensions
              .find<saves::VpAllPlayersWireDimensions>()->city_strategies == 1,
          "Session rules collection rejected a text ID coerced to zero");
}
