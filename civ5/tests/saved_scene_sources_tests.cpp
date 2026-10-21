#include "civ5/scene/saved_scene_sources.hpp"
#include <iostream>
#include <stdexcept>
#include <xmmintrin.h>

using namespace civ5;
namespace {
void check(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}
template <class Action> void rejects(Action action) {
    bool rejected = false;
    try { action(); } catch (const std::invalid_argument &) { rejected = true; }
    check(rejected, "Unresolved saved scene input was accepted");
}
std::uint32_t mask_at(const terrain::TerrainSourceGrid &grid, int x, int y) {
    const auto at = (static_cast<std::size_t>(x) * grid.height + y) * 8;
    return std::uint32_t(grid.cells[at]) | std::uint32_t(grid.cells[at + 1]) << 8 |
           std::uint32_t(grid.cells[at + 2]) << 16 | std::uint32_t(grid.cells[at + 3]) << 24;
}
} // namespace
int main() {
    saves::SavedMapTerrainSource terrain_source{};
    terrain_source.grid = {4, 3};
    terrain_source.wrap_x = true;
    for (int x = 0; x < 4; ++x)
        for (int y = 0; y < 3; ++y) {
            saves::SavedTerrainCellSource cell{};
            cell.native_terrain_mask = std::uint32_t{1} << (x * 3 + y);
            cell.native_category = static_cast<std::uint8_t>(x * 3 + y);
            cell.native_terrain_mask_known = true;
            cell.rivers = saves::SavedRiverPlot{};
            cell.auxiliary = saves::SavedTerrainAuxiliaryPlot{0, 2, 0, -1};
            terrain_source.cells.push_back(cell);
        }
    saves::SavedMapTeamArtObservation team_art{};
    for (std::uint64_t i = 0; i < terrain_source.grid.size(); ++i)
        team_art.plots.push_back({i, 0, false, {}, {}, -1});
    team_art.plots[0].revealed = true;
    team_art.plots[0].visibility_count = 1;
    team_art.plots[8].revealed = true;
    team_art.plots[8].visibility_count = 1;
    team_art.plots[3].revealed = true;
    const auto build = [&] {
        return scene::build_saved_scene_sources(terrain_source, team_art, 0x1234, {},
            [](float value) { return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(value))); });
    };
    auto assembled = build();
    check(assembled.world.source_grid.width == 4 && assembled.world.source_grid.height == 3 &&
          assembled.terrain_grid.width == 4 && assembled.terrain_grid.height == 7,
          "Saved world used the wrong grid mode or dimensions");
    check(mask_at(assembled.terrain_grid, 0, 2) == 1 && mask_at(assembled.terrain_grid, 3, 4) == 1u << 11 &&
          mask_at(assembled.terrain_grid, 0, 0) == 0x2000 && mask_at(assembled.terrain_grid, 3, 6) == 0x2000,
          "Saved x-major terrain was inverted in the padded worker grid");
    const auto index = (std::size_t(3) * assembled.terrain_grid.height + 4) * 8;
    check(assembled.terrain_grid.cells[index + 4] == 11 && assembled.terrain_grid.cells[index + 5] == 0 &&
          assembled.terrain_grid.cells[index + 6] == 0x34 && assembled.terrain_grid.cells[index + 7] == 0x12,
          "Saved category, claim or caller-owned reserved bytes changed");
    check(assembled.river_edges.empty() && assembled.river_network.chains().empty() &&
          assembled.auxiliary.relief.empty() && assembled.auxiliary.feature2.empty(),
          "Dry ordinary saved map invented river or feature producers");
    check(assembled.fog_counts == scene::SavedFogOfWarCounts{2, 1, 9, 0} &&
          assembled.fog.visibility()[0] == 0 && assembled.fog.visibility()[2] == 0 &&
          assembled.fog.visibility()[9] == 1 && !assembled.fog.dirty(),
          "Saved FOW and terrain source coordinates diverged");

    saves::SavedSessionSnapshot snapshot{};
    snapshot.map_header.layout = saves::GameStateLayout::vp542_corpus;
    snapshot.map_header.grid = terrain_source.grid;
    snapshot.map_header.wrap_x = terrain_source.wrap_x;
    snapshot.map_header.wrap_y = terrain_source.wrap_y;
    snapshot.map_terrain = terrain_source;
    snapshot.pregame.emplace();
    snapshot.pregame->layout = snapshot.map_header.layout;
    snapshot.pregame->archive.integers["active_player"] = 0;
    snapshot.pregame->archive.integer_arrays["team_types"] = {0};
    snapshot.map_plots.emplace();
    snapshot.map_plots->layout = snapshot.map_header.layout;
    snapshot.map_plots->dimensions.players = 1;
    snapshot.map_plots->dimensions.teams = 1;
    for (std::uint64_t i = 0; i < terrain_source.grid.size(); ++i) {
        saves::PlotObservation plot{};
        plot.index = i;
        const auto [x, y] = terrain_source.grid.coordinates(i);
        plot.x = x;
        plot.y = y;
        plot.fields.push_back({"revealed_bit_count", 1});
        plot.arrays.push_back({"teams", 10, {0, team_art.plots[i].visibility_count,
            0, 0, 0, -1, -1, 0, 0, 0}});
        plot.arrays.push_back({"revealed_words", 1, {*team_art.plots[i].revealed ? 1 : 0}});
        snapshot.map_plots->plots.push_back(std::move(plot));
    }
    auto from_snapshot = scene::build_saved_scene_sources(snapshot, 0x1234, {},
        [](float value) { return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(value))); });
    check(from_snapshot.terrain_grid.cells == assembled.terrain_grid.cells &&
          from_snapshot.fog_counts == assembled.fog_counts &&
          from_snapshot.fog.visibility() == assembled.fog.visibility(),
          "One-save scene entry point changed map orientation or active-team visibility");
    snapshot.map_header.grid.width = 5;
    rejects([&] { (void)scene::build_saved_scene_sources(snapshot, 0x1234, {},
        [](float value) { return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(value))); }); });
    snapshot.map_header.grid.width = 4;
    snapshot.map_plots->plots[1].y = 2;
    rejects([&] { (void)scene::build_saved_scene_sources(snapshot, 0x1234, {},
        [](float value) { return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(value))); }); });
    snapshot.map_plots->plots[1].y = 0;
    snapshot.map_plots->plots.pop_back();
    rejects([&] { (void)scene::build_saved_scene_sources(snapshot, 0x1234, {},
        [](float value) { return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(value))); }); });

    terrain_source.cells.back().native_terrain_mask_known = false;
    rejects([&] { (void)build(); });
    terrain_source.cells.back().native_terrain_mask_known = true;
    terrain_source.cells.back().auxiliary.reset();
    rejects([&] { (void)build(); });
    terrain_source.cells.back().auxiliary = saves::SavedTerrainAuxiliaryPlot{0, 2, 0, -1};
    team_art.plots.back().revealed.reset();
    rejects([&] { (void)build(); });
    std::cout << "PASS saved scene source alignment and unresolved-input guards\n";
}
