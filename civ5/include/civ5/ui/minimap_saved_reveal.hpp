#pragma once

#include "civ5/saves/map_state.hpp"
#include "civ5/ui/minimap_tile_grid.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace civ5::ui {
// Replay the active team's serialized reveal bits through the native
// 0x004FD0A0 single-cell fog-clear operation. This is the save's reveal
// state, before gameplay visibility recomputation on load.
inline std::size_t apply_saved_minimap_reveals(
    MinimapTileGrid &grid, const saves::SavedMapTeamArtObservation &art) {
    if (art.plots.size() != static_cast<std::size_t>(grid.width()) * grid.height())
        throw std::invalid_argument("Saved reveal plots differ from minimap extent");
    for (std::size_t index = 0; index < art.plots.size(); ++index) {
        const auto &plot = art.plots[index];
        if (plot.plot_index != index || !plot.revealed.has_value() || plot.visibility_count < 0)
            throw std::invalid_argument("Saved minimap reveal plot is incomplete");
    }
    const MinimapTileProjection projection{{1, 0, 0}, {0, 1, 0}};
    std::size_t revealed = 0;
    for (std::size_t index = 0; index < art.plots.size(); ++index) {
        const auto &plot = art.plots[index];
        if (!*plot.revealed) continue;
        const auto x = static_cast<std::int32_t>(index % grid.width());
        const auto y = static_cast<std::int32_t>(index / grid.width());
        grid.clear_cell_fog(x - (y >> 1), y, projection);
        ++revealed;
    }
    return revealed;
}
} // namespace civ5::ui
