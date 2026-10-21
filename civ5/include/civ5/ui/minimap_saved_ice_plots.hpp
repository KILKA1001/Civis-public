#pragma once

#include "civ5/content/info_types.hpp"
#include "civ5/saves/map_state.hpp"
#include "civ5/ui/minimap_hex_geometry.hpp"
#include "civ5/ui/minimap_visible_bounds.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace civ5::ui {

struct MinimapSavedIcePlot {
    std::size_t plot_index;
    std::uint32_t map_x, map_y;
    MinimapPoint2 center;
    std::array<MinimapPoint2, 6> corners;
    MinimapWorldBounds bounds;
};

// This projects saved FEATURE_ICE cells, not the original renderer's
// 20-byte kind-9 region table or geometry handles. Those are separate runtime
// data. The sparse source is useful for verifying the selected save and for
// matching an eventual native region capture to its underlying plot features.
inline std::vector<MinimapSavedIcePlot> project_saved_minimap_ice_plots(
    const saves::SavedMapProjection &map, const MinimapWorldTransform &transform) {
    const auto width = map.grid.width, height = map.grid.height;
    if (width == 0 || height == 0 || map.wrap_y ||
        width > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) ||
        height > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) ||
        width > std::numeric_limits<std::size_t>::max() / height ||
        map.cells.size() != static_cast<std::size_t>(width) * height ||
        !std::isfinite(transform.scale_x) || !std::isfinite(transform.scale_y) ||
        transform.scale_x == 0 || transform.scale_y == 0 ||
        !std::isfinite(transform.offset_x) || !std::isfinite(transform.offset_y))
        throw std::invalid_argument("Invalid saved minimap ice source");
    for (const float value : transform.cube_to_world)
        if (!std::isfinite(value))
            throw std::invalid_argument("Invalid saved minimap ice transform");

    const auto ice_hash = content::saved_info_type_hash("FEATURE_ICE");
    std::vector<MinimapSavedIcePlot> result;
    for (std::size_t index = 0; index < map.cells.size(); ++index) {
        const auto &cell = map.cells[index];
        if (cell.index != index || cell.x != index % width ||
            cell.y != index / width || (cell.feature_type && cell.feature_type_hash))
            throw std::invalid_argument("Incomplete saved minimap ice plot");
        if (cell.feature_type != 0 && cell.feature_type_hash != ice_hash)
            continue;
        const auto hex = minimap_hex_geometry(static_cast<std::int32_t>(cell.x),
                                               static_cast<std::int32_t>(cell.y), transform);
        MinimapWorldBounds bounds{hex.corners[0].x, hex.corners[0].y,
                                  hex.corners[0].x, hex.corners[0].y};
        for (const auto &corner : hex.corners) {
            if (!std::isfinite(corner.x) || !std::isfinite(corner.y))
                throw std::invalid_argument("Invalid saved minimap ice position");
            bounds.min_x = std::min(bounds.min_x, corner.x);
            bounds.min_y = std::min(bounds.min_y, corner.y);
            bounds.max_x = std::max(bounds.max_x, corner.x);
            bounds.max_y = std::max(bounds.max_y, corner.y);
        }
        result.push_back({index, static_cast<std::uint32_t>(cell.x),
                          static_cast<std::uint32_t>(cell.y), hex.center,
                          hex.corners, bounds});
    }
    return result;
}

} // namespace civ5::ui
