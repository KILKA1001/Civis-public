#pragma once
#include "civ5/saves/all_players.hpp"
void all_players_tests() {
    static_assert(sizeof(void *) == 8);
    std::istringstream stream;
    runtime::BinaryReader input(stream, 0);
    rejects([&] { saves::observe_all_players(input, saves::GameStateLayout::vp542_corpus); });
    saves::AllPlayersContext bad_count;
    bad_count.player_count = 0;
    rejects(
        [&] { saves::observe_all_players(input, saves::GameStateLayout::expansion2_version1, bad_count); });
    saves::AllPlayersContext bad_vp;
    bad_vp.player_count = 1;
    bad_vp.profile_extensions.emplace<saves::VpPlayerKnowledgeWireDimensions>();
    bad_vp.traits.emplace();
    bad_vp.trade_culture.emplace();
    rejects([&] { saves::observe_all_players(input, saves::GameStateLayout::vp542_corpus, bad_vp); });
    bad_vp.profile_extensions.emplace<saves::VpAllPlayersWireDimensions>(
        saves::VpAllPlayersWireDimensions{1, 1, 0});
    rejects([&] { saves::observe_all_players(input, saves::GameStateLayout::vp542_corpus, bad_vp); });
    check(input.position() == 0, "Invalid all-player context consumed input");

    saves::GameProgressionObservation progression{};
    progression.layout = saves::GameStateLayout::expansion2_version1;
    saves::ProgressionArray projects{};
    projects.name = "project_created_count";
    projects.rows = {{0, {}}, {123, {7}}, {0, {}}, {123, {2}}};
    progression.arrays.push_back(projects);
    saves::AllPlayersContext context;
    context.player_count = 4097;
    const auto resolved = saves::resolve_saved_player_context(progression, context);
    check(resolved.bnw_projects == 4 && resolved.player_count == 4097 && !context.bnw_projects,
          "Saved project span lost null/duplicate hashes or changed caller context");
    check(saves::resolve_saved_player_context(progression, resolved).bnw_projects == 4,
          "Matching explicit project dimension was rejected");
    context.bnw_projects = 3;
    rejects([&] { (void)saves::resolve_saved_player_context(progression, context); });
    context.bnw_projects.reset();
    progression.arrays.front().rows.clear();
    check(saves::resolve_saved_player_context(progression, context).bnw_projects == 0,
          "Empty saved project collection became missing context");
    progression.arrays.push_back(projects);
    rejects([&] { (void)saves::resolve_saved_player_context(progression, context); });
    progression.arrays.clear();
    rejects([&] { (void)saves::resolve_saved_player_context(progression, context); });
    progression.layout = saves::GameStateLayout::vp542_corpus;
    check(!saves::resolve_saved_player_context(progression, context).bnw_projects,
          "VP context acquired an invented BNW project dimension");
}
