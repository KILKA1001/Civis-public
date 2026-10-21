#pragma once

#include "civ5/ui/minimap_native_ice_records.hpp"
#include "civ5/ui/minimap_projection.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace civ5::ui {

// DX9 kind-9 pixel centers use integer screen coordinates. For the selected
// DX9 driver, triangle setup rounds projected vertices to a 1/256-pixel grid
// before testing those centers with top-left edge ownership. Keep the mask
// independent of texture sampling so the GPU's original ice shader supplies
// the RGB values for all covered fragments.
inline std::vector<MinimapPoint2> raster_minimap_ice_dx9_pixels(
    const MinimapNativeIceRegion &region,
    const std::array<float, 4> &shader_box,
    const std::array<float, 4> &view,
    float world_offset_x,
    int target_width = 320,
    int target_height = 190,
    int viewport_x = 3,
    int viewport_width = 313) {
    if (!(target_width > 0 && target_height > 0 && viewport_x >= 0 &&
          viewport_width > 0 && viewport_x + viewport_width <= target_width &&
          view[2] > view[0] && view[3] > view[1] &&
          shader_box[2] > shader_box[0] && shader_box[3] > shader_box[1] &&
          std::isfinite(world_offset_x)))
        throw std::invalid_argument("Invalid DX9 ice raster bounds");
    struct FixedPoint { std::int64_t x = 0, y = 0; };
    constexpr std::int64_t grid = 256;
    const auto orient = [](FixedPoint a, FixedPoint b, FixedPoint c) {
        return (b.x - a.x) * (c.y - a.y) -
               (b.y - a.y) * (c.x - a.x);
    };
    const auto top_left = [&](FixedPoint a, FixedPoint b, FixedPoint other) {
        if (a.y == b.y) return other.y > a.y;
        if (a.y > b.y) std::swap(a, b);
        return orient(a, b, other) < 0;
    };
    std::vector<FixedPoint> projected;
    projected.reserve(region.vertices.size());
    for (const auto &vertex : region.vertices) {
        const auto normalized = [](std::int16_t value) {
            return std::max(-1.0f, float(value) / 32767.0f);
        };
        const double world_x = double(shader_box[0]) +
            (double(normalized(vertex[0])) * .5 + .5) *
                (double(shader_box[2]) - shader_box[0]) + world_offset_x;
        const double world_y = double(shader_box[1]) +
            (double(normalized(vertex[1])) * .5 + .5) *
                (double(shader_box[3]) - shader_box[1]);
        const double screen_x = viewport_x + (world_x - view[0]) *
            viewport_width / (double(view[2]) - view[0]);
        const double screen_y = (view[3] - world_y) *
            target_height / (double(view[3]) - view[1]);
        if (!(std::isfinite(screen_x) && std::isfinite(screen_y)) ||
            std::abs(screen_x) > 1e7 || std::abs(screen_y) > 1e7)
            throw std::invalid_argument("Invalid projected DX9 ice vertex");
        projected.push_back({static_cast<std::int64_t>(std::floor(screen_x * grid + .5)),
                             static_cast<std::int64_t>(std::floor(screen_y * grid + .5))});
    }
    std::vector<std::uint8_t> mask(
        static_cast<std::size_t>(target_width) * target_height, 0);
    for (std::size_t triangle = 0; triangle < region.indices.size(); triangle += 3) {
        const FixedPoint points[3]{projected[region.indices[triangle]],
                                   projected[region.indices[triangle + 1]],
                                   projected[region.indices[triangle + 2]]};
        const auto area = orient(points[0], points[1], points[2]);
        if (!area) continue;
        const auto range_x = std::minmax({points[0].x, points[1].x, points[2].x});
        const auto range_y = std::minmax({points[0].y, points[1].y, points[2].y});
        const auto first_x = std::max(viewport_x,
            static_cast<int>(std::ceil(double(range_x.first) / grid)));
        const auto last_x = std::min(viewport_x + viewport_width - 1,
            static_cast<int>(std::floor(double(range_x.second) / grid)));
        const auto first_y = std::max(0,
            static_cast<int>(std::ceil(double(range_y.first) / grid)));
        const auto last_y = std::min(target_height - 1,
            static_cast<int>(std::floor(double(range_y.second) / grid)));
        for (int y = first_y; y <= last_y; ++y)
            for (int x = first_x; x <= last_x; ++x) {
                const FixedPoint sample{std::int64_t(x) * grid,
                                        std::int64_t(y) * grid};
                bool inside = true;
                for (int edge_index = 0; edge_index < 3; ++edge_index) {
                    const auto a = points[edge_index], b = points[(edge_index + 1) % 3];
                    const auto other = points[(edge_index + 2) % 3];
                    const auto edge = orient(a, b, sample) * (area > 0 ? 1 : -1);
                    if (edge < 0 || (edge == 0 && !top_left(a, b, other))) {
                        inside = false;
                        break;
                    }
                }
                if (inside) mask[static_cast<std::size_t>(y) * target_width + x] = 1;
            }
    }
    std::vector<MinimapPoint2> pixels;
    for (int y = 0; y < target_height; ++y)
        for (int x = viewport_x; x < viewport_x + viewport_width; ++x)
            if (mask[static_cast<std::size_t>(y) * target_width + x])
                pixels.push_back({float(x), float(y)});
    return pixels;
}

} // namespace civ5::ui
