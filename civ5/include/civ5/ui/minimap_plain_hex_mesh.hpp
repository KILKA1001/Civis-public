#pragma once

#include "civ5/ui/minimap_draw_batches.hpp"
#include "civ5/ui/minimap_hex_geometry.hpp"
#include <array>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>

namespace civ5::ui {
struct MinimapPlainHexMesh {
    std::vector<MinimapPoint2> vertices;
    std::vector<std::uint32_t> indices;
};

// Native 0x00688950 appends the center, six corners in storage order, then
// emits these six fan triangles. Masked cells use 0x0043A3A0's subdivided
// edge route and must not be silently approximated with this plain mesh.
inline constexpr std::array<std::array<std::uint32_t, 2>, 6> minimap_plain_pairs{{
    {1, 2}, {3, 4}, {5, 0}, {4, 5}, {0, 1}, {2, 3},
}};

inline MinimapPlainHexMesh build_minimap_plain_hex_mesh(
    std::span<const MinimapDrawCell> cells, const MinimapWorldTransform &transform) {
    if (cells.size() > std::numeric_limits<std::uint32_t>::max() / 7u)
        throw std::invalid_argument("Minimap plain hex mesh exceeds 32-bit indices");
    MinimapPlainHexMesh mesh;
    mesh.vertices.reserve(cells.size() * 7);
    mesh.indices.reserve(cells.size() * 18);
    for (const auto &cell : cells) {
        if (cell.edge_mask != 0)
            throw std::invalid_argument("Masked minimap hex requires native edge subdivision");
        const auto hex = minimap_hex_geometry(cell.x, cell.y, transform);
        const auto base = static_cast<std::uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back(hex.center);
        mesh.vertices.insert(mesh.vertices.end(), hex.corners.begin(), hex.corners.end());
        for (const auto &pair : minimap_plain_pairs) {
            mesh.indices.push_back(base);
            mesh.indices.push_back(base + 1 + pair[0]);
            mesh.indices.push_back(base + 1 + pair[1]);
        }
    }
    return mesh;
}
} // namespace civ5::ui
