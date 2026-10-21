#pragma once

#include "civ5/ui/minimap_hex_geometry.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace civ5::ui {

struct MinimapEdgeQuery {
    std::array<std::int32_t, 3> cube;
    std::uint8_t direction;
    bool operator==(const MinimapEdgeQuery &) const = default;
};

// 0x00424BD0 stores terrain-edge vertices relative to their projected cell
// center. 0x005CFA70 wraps only the record-table address: it expands the
// retrieved points relative to the original, unwrapped owned cube.
struct MinimapTerrainEdgeRecord {
    std::vector<MinimapPoint2> relative_world_points;
};

// Retail EXE tables 0x00C7FB08, 0x00A2B2BC and 0x00A2BBF0.
inline constexpr std::array<std::array<std::int32_t, 3>, 6> minimap_neighbor_cube_steps{{
    {0, 1, -1}, {-1, 0, 1}, {1, -1, 0},
    {0, -1, 1}, {1, 0, -1}, {-1, 1, 0},
}};
inline constexpr std::array<std::uint8_t, 6> minimap_preceding_edge{{4, 5, 3, 1, 2, 0}};
inline constexpr std::array<std::uint8_t, 6> minimap_following_edge{{5, 3, 4, 2, 0, 1}};
inline constexpr std::array<std::uint8_t, 6> minimap_reverse_terrain_slot{{9, 10, 11, 6, 7, 8}};

// 0x005CFA70 moves directions 3..5 to the neighboring owned cell and uses
// the reverse slot from 0x00A02A14. The world-specific wrap is applied by the
// record lookup, so the unwrapped query remains available for its fallback.
inline MinimapEdgeQuery minimap_owned_terrain_edge(MinimapEdgeQuery query) {
    if (query.direction >= 6)
        throw std::invalid_argument("Invalid minimap terrain edge direction");
    if (query.direction >= 3) {
        for (std::size_t lane = 0; lane < 3; ++lane)
            query.cube[lane] = std::bit_cast<std::int32_t>(
                static_cast<std::uint32_t>(query.cube[lane]) +
                static_cast<std::uint32_t>(
                    minimap_neighbor_cube_steps[query.direction][lane]));
        query.direction = minimap_reverse_terrain_slot[query.direction];
    }
    return query;
}

inline std::array<MinimapEdgeQuery, 3> minimap_boundary_edge_queries(
    std::array<std::int32_t, 3> center, std::uint8_t direction,
    std::uint8_t edge_mask) {
    if (direction >= 6 || (edge_mask & (1u << direction)) == 0)
        throw std::invalid_argument("Minimap boundary query requires a marked edge");
    const auto neighbor = [&](std::uint8_t adjacent) {
        auto cube = center;
        for (std::size_t lane = 0; lane < 3; ++lane)
            cube[lane] = std::bit_cast<std::int32_t>(
                static_cast<std::uint32_t>(cube[lane]) +
                static_cast<std::uint32_t>(minimap_neighbor_cube_steps[adjacent][lane]));
        return cube;
    };
    auto preceding = minimap_preceding_edge[direction];
    auto following = minimap_following_edge[direction];
    auto preceding_cube = center;
    auto following_cube = center;
    if ((edge_mask & (1u << preceding)) == 0) {
        preceding_cube = neighbor(preceding);
        preceding = minimap_preceding_edge[(preceding + 3) % 6];
    }
    if ((edge_mask & (1u << following)) == 0) {
        following_cube = neighbor(following);
        following = minimap_following_edge[(following + 3) % 6];
    }
    return {{{preceding_cube, preceding}, {center, direction},
             {following_cube, following}}};
}

// 0x00597870's fallback for a missing terrain-edge polyline. 0x00658CF0
// translates the six cube corners by the queried cell before the current
// global cube-to-world matrix. The two points lie 1/4 and 3/4 along that edge.
inline std::array<MinimapPoint2, 2> minimap_synthetic_edge_polyline(
    const MinimapEdgeQuery &query, const MinimapWorldTransform &transform) {
    if (query.direction >= 6)
        throw std::invalid_argument("Invalid minimap synthetic edge direction");
    constexpr std::array<std::uint8_t, 6> first_corner{{2, 4, 0, 5, 1, 3}};
    constexpr std::array<std::uint8_t, 6> second_corner{{1, 3, 5, 4, 0, 2}};
    const auto project = [&](std::uint8_t corner) {
        const auto &offset = minimap_cube_corners[corner];
        auto raw = transform;
        raw.scale_x = raw.scale_y = 1.0f;
        raw.offset_x = raw.offset_y = 0.0f;
        return minimap_project_cube(
            raw, {static_cast<float>(query.cube[0]) + offset[0],
                  static_cast<float>(query.cube[1]) + offset[1],
                  static_cast<float>(query.cube[2]) + offset[2], 1.0f});
    };
    const auto first = project(first_corner[query.direction]);
    const auto second = project(second_corner[query.direction]);
    return {{{first.x * 0.75f + second.x * 0.25f,
              first.y * 0.75f + second.y * 0.25f},
             {first.x * 0.25f + second.x * 0.75f,
              first.y * 0.25f + second.y * 0.75f}}};
}

// 0x00424BD0 registers each incoming world-space polyline twice, in forward
// and reverse order, subtracting the owned cell's cube-to-world center. The
// direction-slot table remains the caller's responsibility.
inline MinimapTerrainEdgeRecord make_minimap_terrain_edge_record(
    std::array<std::int32_t, 3> anchor_cube,
    std::span<const MinimapPoint2> world_points,
    const MinimapWorldTransform &transform, bool reverse) {
    if (world_points.size() < 2)
        throw std::invalid_argument("Native minimap terrain edge needs two points");
    auto raw = transform;
    raw.scale_x = raw.scale_y = 1.0f;
    raw.offset_x = raw.offset_y = 0.0f;
    const auto center = minimap_project_cube(
        raw, {static_cast<float>(anchor_cube[0]), static_cast<float>(anchor_cube[1]),
              static_cast<float>(anchor_cube[2]), 1.0f});
    MinimapTerrainEdgeRecord record{{}};
    record.relative_world_points.reserve(world_points.size());
    for (std::size_t index = 0; index < world_points.size(); ++index) {
        const auto &point = world_points[reverse ? world_points.size() - 1 - index : index];
        record.relative_world_points.push_back(
            {point.x - center.x, point.y - center.y});
    }
    return record;
}

// 0x006A9750 first asks 0x005CFA70 for a recorded terrain polyline. A
// missing record falls back to 0x00597870, using the original query. The
// lookup returns the native relative-XY record; the unwrapped owned cube is
// kept for world projection even if lookup aliases a wrapped table cell.
template <class Lookup>
inline std::vector<MinimapPoint2> minimap_terrain_edge_polyline(
    const MinimapEdgeQuery &query, const MinimapWorldTransform &transform,
    Lookup &&lookup) {
    const auto owned = minimap_owned_terrain_edge(query);
    if (auto record = lookup(owned)) {
        if (record->relative_world_points.size() < 2)
            throw std::invalid_argument("Native minimap terrain edge record is incomplete");
        auto raw = transform;
        raw.scale_x = raw.scale_y = 1.0f;
        raw.offset_x = raw.offset_y = 0.0f;
        const auto &cube = owned.cube;
        const auto center = minimap_project_cube(
            raw, {static_cast<float>(cube[0]), static_cast<float>(cube[1]),
                  static_cast<float>(cube[2]), 1.0f});
        std::vector<MinimapPoint2> world;
        world.reserve(record->relative_world_points.size());
        for (const auto &point : record->relative_world_points)
            world.push_back({point.x + center.x, point.y + center.y});
        return world;
    }
    const auto synthetic = minimap_synthetic_edge_polyline(query, transform);
    return {synthetic.begin(), synthetic.end()};
}

// 0x006D4620 places the previous edge's last point and the next edge's first
// point around the current edge, smooths each point with 1/4, 1/2, 1/4, then
// 0x0065AE20 applies the minimap owner's XY scale and translation.
inline std::vector<MinimapPoint2> smooth_minimap_edge_control_points(
    std::span<const MinimapPoint2> preceding, std::span<const MinimapPoint2> current,
    std::span<const MinimapPoint2> following, const MinimapWorldTransform &transform) {
    if (preceding.size() < 2 || current.size() < 2 || following.size() < 2 ||
        !std::isfinite(transform.scale_x) || !std::isfinite(transform.scale_y) ||
        !std::isfinite(transform.offset_x) || !std::isfinite(transform.offset_y))
        throw std::invalid_argument("Invalid minimap boundary control points");
    std::vector<MinimapPoint2> raw;
    raw.reserve(current.size() + 4);
    raw.push_back(preceding[preceding.size() - 2]);
    raw.push_back(preceding.back());
    raw.insert(raw.end(), current.begin(), current.end());
    raw.push_back(following.front());
    raw.push_back(following[1]);
    std::vector<MinimapPoint2> result;
    result.reserve(current.size() + 2);
    for (std::size_t index = 1; index + 1 < raw.size(); ++index) {
        const auto &before = raw[index - 1];
        const auto &point = raw[index];
        const auto &after = raw[index + 1];
        const float x = before.x * 0.25f + point.x * 0.5f + after.x * 0.25f;
        const float y = before.y * 0.25f + point.y * 0.5f + after.y * 0.25f;
        result.push_back({transform.scale_x * x - transform.offset_x,
                          transform.scale_y * y - transform.offset_y});
    }
    return result;
}

} // namespace civ5::ui
