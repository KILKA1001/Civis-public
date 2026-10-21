#include "civ5/scene/saved_scene_sources.hpp"
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace civ5::scene {
SavedSceneSources build_saved_scene_sources(
    const saves::SavedSessionSnapshot &snapshot, std::uint16_t reserved_word,
    const terrain::FactoryDescriptor &factory,
    const terrain::ReciprocalEstimate &reciprocal) {
    if (!snapshot.pregame || !snapshot.map_plots || !snapshot.map_terrain)
        throw std::invalid_argument("Saved scene requires setup, plots and content-resolved terrain");
    const auto &grid = snapshot.map_terrain->grid;
    if (snapshot.map_header.grid.width != grid.width ||
        snapshot.map_header.grid.height != grid.height ||
        snapshot.map_header.wrap_x != snapshot.map_terrain->wrap_x ||
        snapshot.map_header.wrap_y != snapshot.map_terrain->wrap_y ||
        snapshot.map_plots->plots.size() != grid.size() ||
        snapshot.map_plots->layout != snapshot.map_header.layout)
        throw std::invalid_argument("Saved scene map observations do not describe one grid");
    for (std::uint64_t i = 0; i < grid.size(); ++i) {
        const auto &plot = snapshot.map_plots->plots[static_cast<std::size_t>(i)];
        const auto [x, y] = grid.coordinates(i);
        if (plot.index != i || plot.x != x || plot.y != y)
            throw std::invalid_argument("Saved scene plot coordinates differ from terrain grid");
    }
    const auto art = saves::observe_saved_map_team_art(*snapshot.pregame, *snapshot.map_plots);
    return build_saved_scene_sources(*snapshot.map_terrain, art, reserved_word, factory, reciprocal);
}

SavedSceneSources build_saved_scene_sources(
    const saves::SavedMapTerrainSource &source,
    const saves::SavedMapTeamArtObservation &art,
    std::uint16_t reserved_word, const terrain::FactoryDescriptor &factory,
    const terrain::ReciprocalEstimate &reciprocal) {
    const auto count = source.grid.size();
    if (source.cells.size() != count || count > 1024 * 1024 ||
        source.grid.width > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) ||
        source.grid.height > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::invalid_argument("Saved scene source dimensions or cell count differ");
    const auto width = static_cast<std::int32_t>(source.grid.width);
    const auto height = static_cast<std::int32_t>(source.grid.height);
    const auto mode = terrain::game_world_grid_mode(source.wrap_x);
    auto world = terrain::build_world_layout(width, height, mode, factory, reciprocal);
    const terrain::WorldGridLayout grids{world.source_grid, world.worker_grid};
    std::vector<terrain::TerrainSourceCell> classified;
    std::vector<terrain::TerrainRiverPlot> rivers;
    std::vector<terrain::TerrainAuxiliaryPlot> auxiliary;
    classified.reserve(source.cells.size());
    rivers.reserve(source.cells.size());
    auxiliary.reserve(source.cells.size());
    for (const auto &cell : source.cells) {
        if (!cell.native_terrain_mask_known || !cell.native_category || !cell.rivers || !cell.auxiliary)
            throw std::invalid_argument("Saved scene source has unresolved terrain, river or feature input");
        classified.push_back({cell.native_terrain_mask, *cell.native_category});
        const auto &river = *cell.rivers;
        rivers.push_back({river.is_river, river.west, river.northwest, river.northeast,
                          river.flow_e, river.flow_se, river.flow_sw});
        const auto &a = *cell.auxiliary;
        auxiliary.push_back({a.terrain, a.plot, a.continent, a.feature});
    }
    auto terrain_grid = terrain::build_terrain_source_grid(grids, classified, reserved_word);
    auto auxiliary_sources = terrain::build_terrain_auxiliary_sources(world.source_grid, auxiliary);
    auto river_edges = terrain::build_terrain_river_edges(world.source_grid, rivers);
    auto river_network = terrain::RiverNetwork::build(width, height, river_edges);
    materials::FogOfWarState fog(width, height, {}, world.bounds[0], world.bounds[3]);
    auto fog_counts = apply_saved_map_fog(source.grid, art, fog);
    fog.tick(0.f);
    return {std::move(world), std::move(terrain_grid), std::move(auxiliary_sources),
            std::move(river_edges), std::move(river_network), std::move(fog), fog_counts};
}
} // namespace civ5::scene
