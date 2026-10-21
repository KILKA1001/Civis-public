#pragma once

#include "civ5/terrain/height_owner.hpp"
#include "civ5/ui/minimap_hex_geometry.hpp"
#include "civ5/ui/minimap_panel.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace civ5::ui {
struct MinimapWorldBounds {
    float min_x, min_y, max_x, max_y;
};

struct MinimapVisibleBounds {
    std::array<MinimapWorldBounds, 2> segments{};
    std::uint32_t segment_count = 1;
    std::uint32_t edge_subdivisions = 2;
    float inverse_edge_steps = 1.0f;
    MinimapTextureGeometry texture{};
};

// Native 0x004FB380 prepares the projected corner extents and texture aspect;
// 0x006DB300 expands the selected cell interval and stores the kind-7 quad
// world bounds at minimap +0x12C/+0x130/+0x144/+0x148. The caller supplies
// the grid's current inclusive cell limits (initially the whole map).
inline MinimapVisibleBounds plan_minimap_visible_bounds(
    const MinimapWorldTransform &transform,
    const terrain::HeightOwnerParameters &owner,
    std::int32_t min_cell_x, std::int32_t min_cell_y,
    std::int32_t max_cell_x, std::int32_t max_cell_y, bool wrap_x) {
    if (min_cell_x > max_cell_x || min_cell_y > max_cell_y ||
        !std::isfinite(owner.world_span_x) || !std::isfinite(owner.world_span_y) ||
        !std::isfinite(owner.center_x) || !std::isfinite(owner.center_y) ||
        owner.world_span_x <= 0 || owner.world_span_y <= 0)
        throw std::invalid_argument("Invalid minimap visible-bounds input");
    const auto texture = plan_minimap_texture_geometry(owner.world_span_x,
                                                        owner.world_span_y);
    const auto first = minimap_hex_geometry(min_cell_x, min_cell_y, transform);
    const auto last = minimap_hex_geometry(max_cell_x, max_cell_y, transform);
    const auto origin = minimap_project_cube(transform, {0, 0, 0, 1});
    std::array<MinimapPoint2, 6> offsets{};
    for (std::size_t index = 0; index < offsets.size(); ++index) {
        const auto projected = minimap_project_cube(transform, minimap_cube_corners[index]);
        offsets[index] = {projected.x - origin.x, projected.y - origin.y};
    }
    float offset_min_x = offsets[0].x, offset_max_x = offsets[0].x;
    float offset_min_y = offsets[0].y, offset_max_y = offsets[0].y;
    for (const auto &offset : offsets) {
        offset_min_x = std::min(offset_min_x, offset.x);
        offset_max_x = std::max(offset_max_x, offset.x);
        offset_min_y = std::min(offset_min_y, offset.y);
        offset_max_y = std::max(offset_max_y, offset.y);
    }
    const float half_hex_x = (offset_max_x - offset_min_x) * 0.5f;
    const float half_hex_y = (offset_max_y - offset_min_y) * 0.5f;
    MinimapWorldBounds bounds{first.center.x - half_hex_x,
                              first.center.y - half_hex_y,
                              last.center.x + half_hex_x,
                              last.center.y + half_hex_y};
    const float span_x = bounds.max_x - bounds.min_x;
    const float span_y = bounds.max_y - bounds.min_y;
    const float center_x = (bounds.min_x + bounds.max_x) * 0.5f;
    const float center_y = (bounds.min_y + bounds.max_y) * 0.5f;
    const float aspect = texture.active_aspect;
    if (!(span_x > 0 && span_y > 0 && std::isfinite(span_x) && std::isfinite(span_y)))
        throw std::invalid_argument("Minimap cell projection has invalid span");
    if (span_x / span_y <= aspect) {
        float margin = (span_y / 190.0f) * 20.0f;
        if (owner.world_span_y <= 2.0f * margin + span_y)
            margin = std::max(0.0f, (owner.world_span_y - span_y) * 0.5f);
        bounds.min_y -= margin;
        bounds.max_y += margin;
        const float half_width = (bounds.max_y - bounds.min_y) * aspect * 0.5f;
        bounds.min_x = center_x - half_width;
        bounds.max_x = center_x + half_width;
    } else {
        float margin = (span_x / static_cast<float>(texture.active_width)) * 20.0f;
        if (owner.world_span_x <= 2.0f * margin + span_x)
            margin = std::max(0.0f, (owner.world_span_x - span_x) * 0.5f);
        bounds.min_x -= margin;
        bounds.max_x += margin;
        const float half_height = ((bounds.max_x - bounds.min_x) / aspect) * 0.5f;
        bounds.min_y = center_y - half_height;
        bounds.max_y = center_y + half_height;
    }
    const float full_min_x = owner.center_x - owner.world_span_x * 0.5f;
    const float full_max_x = owner.center_x + owner.world_span_x * 0.5f;
    const float full_min_y = owner.center_y - owner.world_span_y * 0.5f;
    const float full_max_y = owner.center_y + owner.world_span_y * 0.5f;
    if (full_max_y <= bounds.max_y && bounds.min_y <= full_min_y) {
        const float excess = ((bounds.max_y - bounds.min_y) -
                              (full_max_y - full_min_y)) * 0.5f;
        bounds.min_y -= excess;
        bounds.max_y -= excess;
    }
    const float edge_x = offsets[1].x - offsets[0].x;
    const float edge_y = offsets[1].y - offsets[0].y;
    const float edge_length = std::sqrt(edge_x * edge_x + edge_y * edge_y);
    const float pixel_scale = std::max(static_cast<float>(texture.active_width) /
                                           (bounds.max_x - bounds.min_x),
                                       190.0f / (bounds.max_y - bounds.min_y));
    const float subdivision = std::ceil(edge_length * pixel_scale * 0.25f);
    if (!(std::isfinite(subdivision) && subdivision <= 65536.0f))
        throw std::invalid_argument("Minimap edge subdivision is invalid");
    MinimapVisibleBounds result;
    result.segments[0] = bounds;
    result.texture = texture;
    result.edge_subdivisions = static_cast<std::uint32_t>(std::max(2.0f, subdivision));
    result.inverse_edge_steps = 1.0f / static_cast<float>(result.edge_subdivisions - 1);
    if (wrap_x && bounds.min_x < full_min_x) {
        const float shifted_min = full_max_x - (full_min_x - bounds.min_x);
        result.segments[1] = {shifted_min, bounds.min_y,
                              shifted_min + (bounds.max_x - bounds.min_x), bounds.max_y};
        result.segment_count = 2;
    } else if (wrap_x && full_max_x < bounds.max_x) {
        const float shifted_min = full_min_x - (full_max_x - bounds.min_x);
        result.segments[1] = {shifted_min, bounds.min_y,
                              shifted_min + (bounds.max_x - bounds.min_x), bounds.max_y};
        result.segment_count = 2;
    }
    return result;
}
} // namespace civ5::ui
