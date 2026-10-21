#pragma once

#include "civ5/saves/map_state.hpp"
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

struct MinimapMountainMarker {
    std::uint32_t map_x, map_y;
    MinimapPoint2 center;
};

struct MinimapMountainMarkers {
    float radius = 0.0f;
    std::vector<MinimapMountainMarker> source;
    std::array<std::vector<std::size_t>, 2> visible_indices;
};

// Retail 0x00634DB0..0x00634F60 scans X first, then Y, retaining plots whose
// virtual plot-type accessor returns zero (PLOT_MOUNTAIN). It projects their
// axial cube coordinates through 0x006A8BC0 without the minimap cell buffer's
// separate integer matrix. 0x006351F5..0x0063524F supplies a common sprite
// radius from the larger world span and larger source-map dimension.
inline float minimap_stored_marker_radius(
    const terrain::HeightOwnerParameters &owner, std::uint32_t width,
    std::uint32_t height) {
    if (width == 0 || height == 0 ||
        !(std::isfinite(owner.world_span_x) && std::isfinite(owner.world_span_y) &&
          owner.world_span_x > 0 && owner.world_span_y > 0))
        throw std::invalid_argument("Invalid minimap marker radius source");
    const float radius = std::max(owner.world_span_x, owner.world_span_y) * 0.5f /
                         static_cast<float>(std::max(width, height));
    if (!(std::isfinite(radius) && radius > 0))
        throw std::invalid_argument("Invalid minimap marker radius");
    return radius;
}

inline MinimapMountainMarkers build_minimap_mountain_markers(
    const saves::SavedMapProjection &map, const MinimapWorldTransform &transform,
    const terrain::HeightOwnerParameters &owner, MinimapVisibleBounds visible) {
    const auto width = map.grid.width, height = map.grid.height;
    if (width == 0 || height == 0 ||
        width > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) ||
        height > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) ||
        width > std::numeric_limits<std::size_t>::max() / height ||
        map.cells.size() != static_cast<std::size_t>(width) * height ||
        visible.segment_count == 0 || visible.segment_count > visible.segments.size() ||
        !(std::isfinite(owner.world_span_x) && std::isfinite(owner.world_span_y) &&
          owner.world_span_x > 0 && owner.world_span_y > 0))
        throw std::invalid_argument("Invalid minimap mountain source");
    for (std::uint32_t segment = 0; segment < visible.segment_count; ++segment) {
        const auto &box = visible.segments[segment];
        if (!(std::isfinite(box.min_x) && std::isfinite(box.min_y) &&
              std::isfinite(box.max_x) && std::isfinite(box.max_y) &&
              box.max_x >= box.min_x && box.max_y >= box.min_y))
            throw std::invalid_argument("Invalid minimap mountain segment");
    }
    const auto origin = minimap_project_cube(transform, {0, 0, 0, 1});
    MinimapWorldBounds extents{};
    bool first = true;
    for (const auto &corner : minimap_cube_corners) {
        const auto point = minimap_project_cube(transform, corner);
        const float x = point.x - origin.x, y = point.y - origin.y;
        if (first) {
            extents = {x, y, x, y};
            first = false;
        } else {
            extents.min_x = std::min(extents.min_x, x);
            extents.max_x = std::max(extents.max_x, x);
            extents.min_y = std::min(extents.min_y, y);
            extents.max_y = std::max(extents.max_y, y);
        }
    }
    MinimapMountainMarkers result;
    result.radius = minimap_stored_marker_radius(owner, width, height);
    for (std::uint32_t x = 0; x < width; ++x) {
        for (std::uint32_t y = 0; y < height; ++y) {
            const auto &cell = map.cells[static_cast<std::size_t>(y) * width + x];
            if (cell.x != x || cell.y != y || !cell.plot_type)
                throw std::invalid_argument("Incomplete minimap mountain plot type");
            if (*cell.plot_type != 0) continue;
            const auto q = static_cast<std::int32_t>(x) -
                           (static_cast<std::int32_t>(y) >> 1);
            const auto r = static_cast<std::int32_t>(y);
            const auto center = minimap_project_cube(
                transform, {static_cast<float>(q), static_cast<float>(r),
                            static_cast<float>(-q - r), 1.0f});
            if (!(std::isfinite(center.x) && std::isfinite(center.y)))
                throw std::invalid_argument("Invalid minimap mountain position");
            const auto index = result.source.size();
            result.source.push_back({x, y, center});
            for (std::uint32_t segment = 0; segment < visible.segment_count; ++segment) {
                const auto &box = visible.segments[segment];
                // 0x004D7E90/0x004D7F91/0x004D8090 reject strict gaps only.
                if (center.x + extents.max_x < box.min_x ||
                    box.max_x < center.x + extents.min_x ||
                    center.y + extents.max_y < box.min_y ||
                    box.max_y < center.y + extents.min_y)
                    continue;
                result.visible_indices[segment].push_back(index);
            }
        }
    }
    return result;
}

} // namespace civ5::ui
