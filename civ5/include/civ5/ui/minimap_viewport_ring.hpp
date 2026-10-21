#pragma once

#include "civ5/ui/minimap_visible_bounds.hpp"
#include "civ5/terrain/view_bounds.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace civ5::ui {

// Retail 0x0041964A compares the largest visible/world span ratio with the
// constructor's 1.0 threshold before asking 0x006E9A60 for kind 8.
inline bool minimap_needs_viewport_ring(MinimapWorldBounds visible,
                                        float world_span_x, float world_span_y) {
    if (!(std::isfinite(visible.min_x) && std::isfinite(visible.min_y) &&
          std::isfinite(visible.max_x) && std::isfinite(visible.max_y) &&
          visible.max_x > visible.min_x && visible.max_y > visible.min_y &&
          std::isfinite(world_span_x) && std::isfinite(world_span_y) &&
          world_span_x > 0 && world_span_y > 0))
        throw std::invalid_argument("Invalid minimap viewport-ring extents");
    const float x_ratio = (visible.max_x - visible.min_x) / world_span_x;
    const float y_ratio = (visible.max_y - visible.min_y) / world_span_y;
    return (x_ratio > y_ratio ? x_ratio : y_ratio) > 1.0f;
}

// The source perimeter is 0 -> 1 -> 3 -> 2 -> 0, as indexed by retail
// 0x00A2B108. The 0->3 and 1->2 vectors are its two diagonals.
using MinimapViewportCorners = std::array<terrain::Vector3, 4>;

// The camera event at 0x0069CA50 stores these corners in minimap context
// +0x278, which the kind-8 ring producer at 0x006294F0 reads unchanged.
inline std::vector<MinimapViewportCorners> minimap_viewport_corners(
    std::span<const terrain::ViewFrustum> frusta, float plane_height) {
    if (frusta.size() > 3)
        throw std::invalid_argument("At most three minimap viewport quads are supported");
    std::vector<MinimapViewportCorners> result;
    result.reserve(frusta.size());
    for (const auto &frustum : frusta)
        result.push_back(terrain::frustum_horizontal_corners(frustum, plane_height));
    return result;
}

struct MinimapCameraViewport {
    MinimapViewportCorners corners{};
    std::array<float, 2> low{}, high{};
};

// The same retail camera callback stores XY extrema at context +0x238 and
// +0x258 after intersecting its four corners. These are per-view bounds,
// separate from the full-world minimap projection bounds.
inline std::vector<MinimapCameraViewport> minimap_camera_viewports(
    std::span<const terrain::ViewFrustum> frusta, float plane_height) {
    const auto corners = minimap_viewport_corners(frusta, plane_height);
    std::vector<MinimapCameraViewport> result;
    result.reserve(corners.size());
    for (const auto &quad : corners) {
        MinimapCameraViewport view;
        view.corners = quad;
        view.low = {quad[0][0], quad[0][1]};
        view.high = view.low;
        for (std::size_t corner = 1; corner < quad.size(); ++corner)
            for (std::size_t axis = 0; axis < 2; ++axis) {
                view.low[axis] = std::min(view.low[axis], quad[corner][axis]);
                view.high[axis] = std::max(view.high[axis], quad[corner][axis]);
            }
        result.push_back(view);
    }
    return result;
}

struct MinimapViewportVertex {
    std::array<float, 3> position{};
    // The retail D3DCOLOR input is white for the four source corners and
    // zero for the eight fringe vertices. Kind-8 PS maps red to RGB and
    // sqrt(red) to alpha.
    std::uint32_t color = 0;
};

struct MinimapViewportRing {
    std::vector<MinimapViewportVertex> vertices;
    std::vector<std::uint16_t> indices;
};

inline constexpr std::array<std::uint16_t, 48> minimap_viewport_ring_indices{{
    0, 1, 5, 0, 5, 4, 1, 3, 5, 3, 5, 7,
    3, 7, 6, 3, 6, 2, 0, 4, 6, 0, 6, 2,
    0, 9, 1, 0, 9, 8, 1, 11, 9, 1, 11, 3,
    3, 10, 11, 3, 10, 2, 0, 2, 10, 0, 10, 8}};

// Original 0x006294F0 expands each context+0x278 four-corner rectangle into
// 12 vertices and 16 triangles. Its 10-pixel fringe is normalized from both
// source diagonals in projected minimap pixels. Z is copied, not expanded.
inline MinimapViewportRing build_minimap_viewport_ring(
    std::span<const MinimapViewportCorners> quads, MinimapWorldBounds visible,
    std::uint32_t active_width, std::uint32_t height = 190,
    float fringe_pixels = 10.0f) {
    if (quads.size() > 3 || active_width == 0 || height == 0 ||
        !(std::isfinite(visible.min_x) && std::isfinite(visible.min_y) &&
          std::isfinite(visible.max_x) && std::isfinite(visible.max_y) &&
          visible.max_x > visible.min_x && visible.max_y > visible.min_y &&
          fringe_pixels > 0 && std::isfinite(fringe_pixels)))
        throw std::invalid_argument("Invalid minimap viewport-ring input");
    MinimapViewportRing result;
    result.vertices.reserve(quads.size() * 12);
    result.indices.reserve(quads.size() * minimap_viewport_ring_indices.size());
    // Retail 0x0062950F..0x0062956D divides in x87 precision, then stores
    // each pixel-per-world factor as Float32 before the SSE fringe products.
    const float pixels_per_world_x = static_cast<float>(
        static_cast<double>(active_width) /
        (static_cast<double>(visible.max_x) - visible.min_x));
    const float pixels_per_world_y = static_cast<float>(
        static_cast<double>(height) /
        (static_cast<double>(visible.max_y) - visible.min_y));
    for (const auto &corners : quads) {
        for (const auto &corner : corners)
            for (const auto coordinate : corner)
                if (!std::isfinite(coordinate))
                    throw std::invalid_argument("Nonfinite minimap viewport corner");
        const auto delta = [&](std::size_t from, std::size_t to) {
            std::array<float, 3> vector{};
            for (std::size_t k = 0; k < 3; ++k)
                vector[k] = corners[to][k] - corners[from][k];
            const float dx = vector[0] * pixels_per_world_x;
            const float dy = vector[1] * pixels_per_world_y;
            // 0x006296B7..0x00629705 stores these SSE products as Float32,
            // then squares and normalizes them on the x87 stack. Rounding
            // the sum to Float32 early moves the final GPU fringe pixels.
            const double length_squared = static_cast<double>(dx) * dx +
                                          static_cast<double>(dy) * dy;
            if (!(length_squared > 0 && std::isfinite(length_squared)))
                throw std::invalid_argument("Degenerate minimap viewport diagonal");
            const float fringe_squared = fringe_pixels * fringe_pixels;
            const float scale = static_cast<float>(std::sqrt(
                static_cast<double>(fringe_squared) / (length_squared * 4.0)));
            for (auto &component : vector) component *= scale;
            return vector;
        };
        const auto a = delta(0, 3), b = delta(1, 2);
        const auto add_xy = [](const std::array<float, 3> &p,
                               const std::array<float, 3> &v, float sign) {
            return std::array<float, 3>{p[0] + sign * v[0],
                                        p[1] + sign * v[1], p[2]};
        };
        const std::array<std::array<float, 3>, 12> expanded{{
            corners[0], corners[1], corners[2], corners[3],
            add_xy(corners[0], a, +1), add_xy(corners[1], b, +1),
            add_xy(corners[2], b, -1), add_xy(corners[3], a, -1),
            add_xy(corners[0], a, -1), add_xy(corners[1], b, -1),
            add_xy(corners[2], b, +1), add_xy(corners[3], a, +1)}};
        const auto base = static_cast<std::uint16_t>(result.vertices.size());
        for (std::size_t n = 0; n < expanded.size(); ++n)
            result.vertices.push_back({expanded[n], n < 4 ? 0xFFFFFFFFu : 0u});
        for (const auto index : minimap_viewport_ring_indices)
            result.indices.push_back(static_cast<std::uint16_t>(base + index));
    }
    return result;
}

} // namespace civ5::ui
