#pragma once

#include "civ5/ui/minimap_draw_batches.hpp"
#include "civ5/ui/minimap_edge_control_points.hpp"
#include "civ5/ui/minimap_hex_geometry.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace civ5::ui {

struct MinimapMaskedHexMesh {
    std::vector<MinimapPoint2> vertices;
    std::vector<std::uint32_t> indices;
};

// 0x0043A3A0 visits edge bits in this order. Each ordinary edge contributes
// two corners from the 0x00A2B168 table; a marked edge uses 0x006D4620's
// projected control points instead.
inline constexpr std::array<std::array<std::uint8_t, 3>, 6> minimap_masked_edges{{
    {4, 0, 1}, {0, 1, 2}, {5, 2, 3},
    {1, 3, 4}, {3, 4, 5}, {2, 5, 0},
}};

// 0x006DD420 evaluates the three-point quadratic from retail float literals
// 0x00B8CF84..0x00B8CFA4. The final integer parameter is zero at its minimap
// call site; the caller samples t in [0, control_points.size() - 2].
inline MinimapPoint2 sample_minimap_boundary_curve(
    const std::vector<MinimapPoint2> &control_points, float t) {
    if (control_points.size() < 3 || !std::isfinite(t) || t < 0.0f)
        throw std::invalid_argument("Invalid minimap boundary curve sample");
    const bool at_end = t >= static_cast<float>(control_points.size() - 2);
    const auto segment = at_end ? control_points.size() - 3
                                : static_cast<std::size_t>(t);
    const float fraction = at_end ? 1.0f : t - static_cast<float>(segment);
    const float squared = fraction * fraction;
    const float first = 0.5f * squared - fraction + 0.5f;
    const float middle = -squared + fraction + 0.5f;
    const float last = 0.5f * squared;
    return {middle * control_points[segment + 1].x +
                first * control_points[segment].x +
                last * control_points[segment + 2].x,
            middle * control_points[segment + 1].y +
                first * control_points[segment].y +
                last * control_points[segment + 2].y};
}

// The control points are the projected and smoothed output of 0x006D4620.
// This stage faithfully reproduces the sampling and triangle topology without
// inventing control points from straight hex edges.
inline MinimapMaskedHexMesh build_minimap_masked_hex_mesh(
    const MinimapDrawCell &cell, const MinimapWorldTransform &transform,
    const std::array<std::vector<MinimapPoint2>, 6> &edge_control_points,
    std::uint32_t samples_per_marked_edge) {
    if (cell.edge_mask == 0 || (cell.edge_mask & ~0x3fu) != 0 ||
        samples_per_marked_edge < 2)
        throw std::invalid_argument("Invalid masked minimap hex arguments");
    std::size_t vertex_count = 1;
    for (const auto &edge : minimap_masked_edges) {
        const bool marked = (cell.edge_mask & (1u << edge[0])) != 0;
        if (marked && edge_control_points[edge[0]].size() < 3)
            throw std::invalid_argument("Marked minimap edge lacks native control points");
        vertex_count += marked ? samples_per_marked_edge : 2;
    }
    if (vertex_count > std::numeric_limits<std::uint32_t>::max())
        throw std::invalid_argument("Masked minimap mesh exceeds 32-bit indices");

    const auto hex = minimap_hex_geometry(cell.x, cell.y, transform);
    MinimapMaskedHexMesh mesh;
    mesh.vertices.reserve(vertex_count);
    mesh.indices.reserve((vertex_count - 1) * 3);
    mesh.vertices.push_back(hex.center);
    std::array<std::uint32_t, 6> first{}, last{};
    for (std::size_t index = 0; index < minimap_masked_edges.size(); ++index) {
        const auto &edge = minimap_masked_edges[index];
        first[index] = static_cast<std::uint32_t>(mesh.vertices.size());
        if ((cell.edge_mask & (1u << edge[0])) == 0) {
            mesh.vertices.push_back(hex.corners[edge[1]]);
            mesh.vertices.push_back(hex.corners[edge[2]]);
        } else {
            const auto &points = edge_control_points[edge[0]];
            float fraction = 0.0f;
            const float increment = 1.0f / static_cast<float>(samples_per_marked_edge - 1);
            for (std::uint32_t sample = 0; sample < samples_per_marked_edge; ++sample) {
                mesh.vertices.push_back(sample_minimap_boundary_curve(
                    points, static_cast<float>(points.size() - 2) * fraction));
                fraction += increment;
            }
        }
        last[index] = static_cast<std::uint32_t>(mesh.vertices.size() - 1);
    }
    for (std::size_t index = 0; index < minimap_masked_edges.size(); ++index) {
        for (auto vertex = first[index]; vertex < last[index]; ++vertex)
            mesh.indices.insert(mesh.indices.end(), {0u, vertex, vertex + 1});
        mesh.indices.insert(mesh.indices.end(),
                            {0u, last[index], first[(index + 1) % 6]});
    }
    return mesh;
}

// Fog calls 0x0043A3A0 with its fourth argument zero. For that route
// 0x00658360 always uses the synthetic edge source in 0x00597870.
inline MinimapMaskedHexMesh build_minimap_synthetic_masked_hex_mesh(
    const MinimapDrawCell &cell, const MinimapWorldTransform &transform,
    std::uint32_t samples_per_marked_edge) {
    const auto hex = minimap_hex_geometry(cell.x, cell.y, transform);
    std::array<std::vector<MinimapPoint2>, 6> curves{};
    for (std::uint8_t direction = 0; direction < 6; ++direction) {
        if ((cell.edge_mask & (1u << direction)) == 0) continue;
        const auto queries = minimap_boundary_edge_queries(
            hex.cube, direction, cell.edge_mask);
        const auto preceding = minimap_synthetic_edge_polyline(queries[0], transform);
        const auto current = minimap_synthetic_edge_polyline(queries[1], transform);
        const auto following = minimap_synthetic_edge_polyline(queries[2], transform);
        curves[direction] = smooth_minimap_edge_control_points(
            preceding, current, following, transform);
    }
    return build_minimap_masked_hex_mesh(cell, transform, curves,
                                        samples_per_marked_edge);
}

// Coast, land and culture call 0x0043A3A0 in mode one. Their marked edges
// consult the terrain-edge records before taking the synthetic fallback.
template <class Lookup>
inline MinimapMaskedHexMesh build_minimap_terrain_masked_hex_mesh(
    const MinimapDrawCell &cell, const MinimapWorldTransform &transform,
    std::uint32_t samples_per_marked_edge, Lookup &&lookup) {
    const auto hex = minimap_hex_geometry(cell.x, cell.y, transform);
    std::array<std::vector<MinimapPoint2>, 6> curves{};
    for (std::uint8_t direction = 0; direction < 6; ++direction) {
        if ((cell.edge_mask & (1u << direction)) == 0) continue;
        const auto queries = minimap_boundary_edge_queries(
            hex.cube, direction, cell.edge_mask);
        const auto preceding = minimap_terrain_edge_polyline(
            queries[0], transform, lookup);
        const auto current = minimap_terrain_edge_polyline(
            queries[1], transform, lookup);
        const auto following = minimap_terrain_edge_polyline(
            queries[2], transform, lookup);
        curves[direction] = smooth_minimap_edge_control_points(
            preceding, current, following, transform);
    }
    return build_minimap_masked_hex_mesh(cell, transform, curves,
                                        samples_per_marked_edge);
}

} // namespace civ5::ui
