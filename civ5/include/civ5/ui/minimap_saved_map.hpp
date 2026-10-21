#pragma once

#include "civ5/saves/map_state.hpp"
#include "civ5/ui/minimap_tile_grid.hpp"
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace civ5::ui {
// Populate the original minimap's terrain records from decoded save plots.
// Culture and visibility are separate native update streams; this projection
// does not infer either from ownership or saved reveal bits.
inline MinimapTileGrid project_saved_minimap_terrain(
    const saves::SavedMapProjection &map) {
    if (map.wrap_y || map.grid.width == 0 || map.grid.height == 0 ||
        map.grid.width > MinimapTileGrid::stride ||
        map.grid.height > MinimapTileGrid::stride ||
        map.cells.size() != map.grid.size())
        throw std::invalid_argument("Saved map cannot populate native minimap storage");
    MinimapTileGrid grid(static_cast<std::uint32_t>(map.grid.width),
                         static_cast<std::uint32_t>(map.grid.height), map.wrap_x);
    for (std::size_t index = 0; index < map.cells.size(); ++index) {
        const auto &cell = map.cells[index];
        if (cell.index != index || cell.x >= map.grid.width ||
            cell.y >= map.grid.height ||
            map.grid.index(cell.x, cell.y) != index || !cell.terrain_type ||
            *cell.terrain_type < std::numeric_limits<std::int32_t>::min() ||
            *cell.terrain_type > std::numeric_limits<std::int32_t>::max())
            throw std::invalid_argument("Saved minimap terrain plot is incomplete");
        grid.populate_terrain_cell(static_cast<std::int32_t>(cell.x),
                                   static_cast<std::int32_t>(cell.y),
                                   static_cast<std::int32_t>(*cell.terrain_type));
    }
    grid.build_initial_terrain_edges();
    return grid;
}
} // namespace civ5::ui
