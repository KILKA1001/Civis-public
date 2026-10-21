#pragma once

#include "civ5/terrain/height_owner.hpp"
#include "civ5/ui/minimap_hex_geometry.hpp"
#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace civ5::ui {
// The renderer's global D80E80 matrix is WorldLayout::transforms[16..31].
// The minimap context used by 0x006A8BC0 is the same height owner set up by
// native 0x00482C70: +A8/+AC scale X/Y and +B4/+B8 center X/Y.
inline MinimapWorldTransform minimap_world_transform(
    const terrain::WorldLayout &layout, const terrain::HeightOwnerParameters &owner) {
    if (layout.source_grid.width <= 0 || layout.source_grid.height <= 0 ||
        layout.source_grid.width > MinimapTileGrid::stride ||
        layout.source_grid.height > MinimapTileGrid::stride ||
        static_cast<std::int64_t>(layout.worker_grid.width) * 64 != owner.width ||
        static_cast<std::int64_t>(layout.worker_grid.height) * 64 != owner.height)
        throw std::invalid_argument("Minimap world and height owner dimensions differ");
    MinimapWorldTransform transform;
    std::copy_n(layout.transforms.begin() + 16, 16, transform.cube_to_world.begin());
    transform.map_to_cube = layout.source_grid.second_matrix;
    transform.scale_x = owner.scale_x;
    transform.scale_y = owner.scale_y;
    transform.offset_x = owner.center_x;
    transform.offset_y = owner.center_y;
    return transform;
}
} // namespace civ5::ui
