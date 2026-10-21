#pragma once
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include <stdexcept>
namespace bnw_frame_fixture {
using namespace civ5;
void check(bool valid, const char *reason) { if (!valid) throw std::runtime_error(reason); }
template<class F> void rejects(F action) {
    try { action(); } catch (const std::exception &) { return; }
    throw std::runtime_error("Required rejection missing");
}
runtime::LiveSession session() {
    using namespace runtime;
    LiveSessionState state{saves::SavedSessionSnapshot{}, ResearchContext{}, {},
        PlayerResearchContext{}, CityYieldContext{}, BnwUnhappinessContext{}, MaintenanceContext{},
        TreasuryContext{}, ScienceContext{}, ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw, {5, 0, 0}),
         SimulationRandom(RandomAlgorithm::bnw, {7, 0, 0})},
        {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};
    constexpr auto bnw = saves::GameStateLayout::expansion2_version1;
    state.setup.game_turn = 12;
    state.setup.players[7].alive = true;
    state.setup.players[7].team = 3;
    state.maintenance.players[7].units[21].type = 4;
    auto &world = state.world;
    world.game_state.layout = world.map_header.layout = bnw;
    world.map_header.grid = {2, 1};
    world.map_header.guid[0] = 42;
    world.pregame.emplace().layout = bnw;
    world.pregame->archive.integers["active_player"] = 7;
    world.pregame->archive.integer_arrays["team_types"] = std::vector<std::int64_t>(80, 3);
    world.pregame->archive.integers["game_turn"] = 12;
    auto &map = world.map_projection.emplace(); map.grid = {2, 1};
    auto &plots = world.map_plots.emplace(); plots.layout = bnw; plots.dimensions.teams = 80; plots.dimensions.players = 80;
    auto &teams = world.teams.emplace(); teams.layout = bnw; teams.dimensions.teams = 64;
    teams.teams.resize(64);
    for (std::uint64_t i = 0; i < 2; ++i) {
        saves::SavedMapCellProjection cell; cell.index = cell.x = i; cell.y = 0;
        cell.terrain_type = 2; cell.revealed_teams = std::set<std::int64_t>{3};
        map.cells.push_back(cell);
        saves::PlotObservation plot; plot.version = 7; plot.index = plot.x = i; plot.y = 0;
        saves::PlotArrayObservation raw{"visibility", 1, std::vector<std::int64_t>(80, 0)};
        raw.values[3] = i == 0 ? 1 : 0;
        plot.arrays.push_back(std::move(raw));
        plot.arrays.push_back({"revealed_words", 1, {8, 0, 0}});
        plot.arrays.push_back({"revealed_improvement_hashes", 1, std::vector<std::int64_t>(80, 0)});
        plot.arrays.push_back({"revealed_routes", 1, std::vector<std::int64_t>(80, -1)});
        plots.plots.push_back(std::move(plot));
    }
    rebuild_bnw_live_plot_visibility(state);
    saves::SavedSession source; source.snapshot = world; source.source.bytes = {1, 2, 3};
    return LiveSession::from_restored_bnw(std::move(source), std::move(state));
}
ui::UiContextRegistry registry() {
    return ui::UiContextRegistry::from_documents(content::read_xml_metadata(
        R"(<Context><LuaContext ID="WorldView" FileName="Assets/UI/InGame/WorldView/WorldView"/><LuaContext ID="TestModal" FileName="TestModal" Modal="1"/></Context>)"),
        content::read_xml_metadata("<Context/>"));
}

graphics::BnwRendererSceneReceipt receipt(const runtime::LiveSession &live) {
    const auto &world=live.state().world;
    graphics::BnwRendererSceneReceipt result;
    result.scene_file_sha256=std::string(64,'1');
    result.map_guid=world.map_header.guid;
    result.width=world.map_header.grid.width; result.height=world.map_header.grid.height;
    result.wrap_x=world.map_header.wrap_x; result.wrap_y=world.map_header.wrap_y;
    result.game_turn=live.state().setup.game_turn;
    const auto art=saves::observe_saved_map_team_art(*world.pregame,*world.map_plots);
    result.active_player=art.active_player; result.active_team=art.active_team;
    result.active_team_art=art.plots;
    for (const auto &cell:world.map_projection->cells)
        result.cells.push_back(graphics::bnw_scene_cell_revision(cell));
    return result;
}
}
