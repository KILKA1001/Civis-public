#pragma once

#include "civ5/ui/minimap_visible_bounds.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <vector>

namespace civ5::ui {

// The original kind-9 ice source is a separate 20-byte world-region table.
// Its geometry handle lives at region +0x10; the first 16 bytes are this box.
struct MinimapIceRegion {
    MinimapWorldBounds bounds;
};

struct MinimapIceDraw {
    std::size_t region_index;
    MinimapWorldBounds visible_bounds;
    float vertex_offset_x;
};

// Retail 0x0077DCF0 visits the base view, then its left and right copies.
// It translates the view for intersection testing and applies the inverse
// offset in the kind-9 vertex shader, so the same region mesh can wrap.
inline std::vector<MinimapIceDraw> plan_minimap_ice_draws(
    MinimapWorldBounds base, float world_span_x,
    std::span<const MinimapIceRegion> regions) {
    const auto valid = [](MinimapWorldBounds box) {
        return std::isfinite(box.min_x) && std::isfinite(box.min_y) &&
               std::isfinite(box.max_x) && std::isfinite(box.max_y) &&
               box.max_x >= box.min_x && box.max_y >= box.min_y;
    };
    if (!valid(base) || !std::isfinite(world_span_x) || world_span_x <= 0)
        throw std::invalid_argument("Invalid minimap ice view");
    for (const auto &region : regions)
        if (!valid(region.bounds))
            throw std::invalid_argument("Invalid minimap ice region");

    const std::array<float, 3> view_shifts{0.0f, -world_span_x, world_span_x};
    std::vector<MinimapIceDraw> draws;
    for (const float shift : view_shifts) {
        const MinimapWorldBounds visible{base.min_x + shift, base.min_y,
                                         base.max_x + shift, base.max_y};
        for (std::size_t index = 0; index < regions.size(); ++index) {
            const auto &box = regions[index].bounds;
            // COMISS / JC at 0x0077DEC8..0x0077DEFC excludes only strict gaps.
            if (visible.max_x < box.min_x || box.max_x < visible.min_x ||
                visible.max_y < box.min_y || box.max_y < visible.min_y)
                continue;
            draws.push_back({index, visible, -shift});
        }
    }
    return draws;
}

} // namespace civ5::ui
