#pragma once
#include "world_layout.hpp"
#include <span>
#include <vector>

namespace civ5::terrain {
struct TerrainSourceCell {
    std::uint32_t mask;
    std::uint8_t category;
};
struct TerrainSourceGrid {
    std::int32_t width, height;
    std::vector<std::uint8_t> cells;
};
// Classified input is x-major (x*sourceHeight+y). Original 404004..40481e
// initializes coast/category-255 padding, transforms source coordinates and
// stores each cell with zero claim byte. No RNG/occurrences/auxiliary pass runs.
// The original copies one untouched stack word into bytes +6/+7 of every cell;
// callers own this reserved value explicitly. It is not inferred from a save.
TerrainSourceGrid build_terrain_source_grid(const WorldGridLayout &layout,
    std::span<const TerrainSourceCell> source, std::uint16_t reserved_word,
    std::uint64_t maximum_cells = 1024 * 1024);
} // namespace civ5::terrain
