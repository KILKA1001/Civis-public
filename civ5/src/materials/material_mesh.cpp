#include "civ5/materials/material_mesh.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace civ5::materials {
MaterialPatchMesh::MaterialPatchMesh(int columns, int rows, std::span<const std::uint8_t> material48,
                                     std::span<const std::uint16_t> indices)
    : columns_(columns), rows_(rows) {
    static_assert(std::endian::native == std::endian::little);
    if (columns < 2 || rows < 2 || columns > 67 || rows > 67)
        throw std::invalid_argument("Material patch dimensions outside2..67");
    const auto count = static_cast<std::size_t>(columns * rows);
    if (material48.size() != count * 48 ||
        indices.size() != static_cast<std::size_t>((columns - 1) * (rows - 1) * 6) ||
        !std::all_of(indices.begin(), indices.end(), [&](auto i) { return i < count; }))
        throw std::invalid_argument("Material48 vertex/index extent mismatch");
    for (std::size_t vertex = 0; vertex < count; ++vertex) {
        for (std::size_t offset : {0, 4, 8, 20, 24, 28}) {
            float value;
            std::memcpy(&value, material48.data() + vertex * 48 + offset, 4);
            if (!std::isfinite(value))
                throw std::invalid_argument("Nonfinite material shader input");
        }
        for (std::size_t lane = 0; lane < 8; ++lane)
            if (material48[vertex * 48 + 40 + lane] >= 96)
                throw std::invalid_argument("Material coefficient index outside96 entries");
    }
    vertices_.assign(material48.begin(), material48.end());
    indices_.assign(indices.begin(), indices.end());
}
} // namespace civ5::materials
