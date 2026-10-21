#include "civ5/terrain/terrain_source_grid.hpp"
#include <limits>
#include <stdexcept>

namespace civ5::terrain {
namespace {
LogicalHex transform(LogicalHex v, const IntMatrix4 &m) {
    std::array<std::int32_t, 3> output{};
    for (unsigned i = 0; i < 3; ++i)
        output[i] = wrap_add(wrap_add(wrap_add(wrap_mul(v.z, m[8 + i]), wrap_mul(v.x, m[i])),
                                     wrap_mul(v.y, m[4 + i])), m[12 + i]);
    return {output[0], output[1], output[2]};
}
void store(std::span<std::uint8_t> cells, std::size_t index, TerrainSourceCell value, std::uint16_t reserved) {
    const auto at = index * 8;
    for (unsigned byte = 0; byte < 4; ++byte)
        cells[at + byte] = static_cast<std::uint8_t>(value.mask >> (byte * 8));
    cells[at + 4] = value.category;
    cells[at + 5] = 0;
    cells[at + 6] = static_cast<std::uint8_t>(reserved);
    cells[at + 7] = static_cast<std::uint8_t>(reserved >> 8);
}
} // namespace
TerrainSourceGrid build_terrain_source_grid(const WorldGridLayout &layout,
    std::span<const TerrainSourceCell> source, std::uint16_t reserved_word, std::uint64_t maximum_cells) {
    const auto &input = layout.source_grid;
    const auto &output = layout.worker_grid;
    if (input.width <= 0 || input.height <= 0 || output.width <= 0 || output.height <= 0)
        throw std::invalid_argument("Invalid terrain source grid dimensions");
    const auto count = std::uint64_t(output.width) * output.height;
    if (std::uint64_t(input.width) * input.height != source.size() || count > maximum_cells ||
        count > std::numeric_limits<std::size_t>::max() / 8)
        throw std::invalid_argument("Terrain source grid exceeds input or storage budget");
    TerrainSourceGrid result{output.width, output.height, std::vector<std::uint8_t>(static_cast<std::size_t>(count) * 8)};
    for (std::size_t i = 0; i < count; ++i)
        store(result.cells, i, {0x2000, 255}, reserved_word);
    for (std::int32_t x = 0; x < input.width; ++x)
        for (std::int32_t y = 0; y < input.height; ++y) {
            const auto cube_x = wrap_add(x, wrap_neg(y >> 1));
            auto cube = transform({cube_x, y, wrap_neg(wrap_add(cube_x, y))}, input.second_matrix);
            cube.z = wrap_neg(wrap_add(cube.x, cube.y));
            cube = transform(cube, output.first_matrix);
            const auto column = wrap_add(cube.x, cube.y >> 1), row = cube.y;
            if (column < 0 || row < 0 || column >= output.width || row >= output.height)
                throw std::invalid_argument("Terrain source coordinate outside worker grid");
            store(result.cells, std::size_t(column) * output.height + row,
                  source[std::size_t(x) * input.height + y], reserved_word);
        }
    return result;
}
} // namespace civ5::terrain
