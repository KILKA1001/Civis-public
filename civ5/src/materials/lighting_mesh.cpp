#include "civ5/materials/lighting_mesh.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::materials {
LightingPatchMesh::LightingPatchMesh(int columns, int rows, std::span<const std::uint8_t> normal24,
                                     std::span<const std::uint16_t> indices)
    : columns_(columns), rows_(rows) {
    if (columns < 2 || rows < 2 || columns > 67 || rows > 67)
        throw std::invalid_argument("Lighting patch dimensions outside 2..67");
    const auto count = static_cast<std::size_t>(columns * rows);
    if (normal24.size() != count * 24 ||
        indices.size() != static_cast<std::size_t>((columns - 1) * (rows - 1) * 6) ||
        !std::all_of(indices.begin(), indices.end(), [&](auto i) { return i < count; }))
        throw std::invalid_argument("Lighting normal24 vertex/index extent mismatch");
    for (std::size_t vertex = 0; vertex < count; ++vertex)
        for (std::size_t offset : {12, 14, 16, 20, 22}) {
            const auto high = normal24[vertex * 24 + offset + 1];
            if ((high & 0x7c) == 0x7c)
                throw std::invalid_argument("Nonfinite lighting half input");
        }
    vertices_.assign(normal24.begin(), normal24.end());
    indices_.assign(indices.begin(), indices.end());
}
} // namespace civ5::materials
