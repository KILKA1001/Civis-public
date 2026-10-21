#pragma once

#include "civ5/ui/minimap_tile_grid.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace civ5::ui {
struct MinimapPoint2 {
    float x, y;
};

// Six float4 cube offsets at retail DX9 VA 0x00B8CF00, in storage order.
// Native 0x004FB380 transforms each offset and subtracts the transformed
// origin before 0x0043A3A0 adds it to a projected plot center.
inline constexpr std::array<std::array<float, 4>, 6> minimap_cube_corners{{
    {2.0f / 3.0f, -1.0f / 3.0f, -1.0f / 3.0f, 1.0f},
    {1.0f / 3.0f, 1.0f / 3.0f, -2.0f / 3.0f, 1.0f},
    {-1.0f / 3.0f, 2.0f / 3.0f, -1.0f / 3.0f, 1.0f},
    {-2.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f, 1.0f},
    {-1.0f / 3.0f, -1.0f / 3.0f, 2.0f / 3.0f, 1.0f},
    {1.0f / 3.0f, -2.0f / 3.0f, 1.0f / 3.0f, 1.0f},
}};

// Native 0x006A8BC0 uses the current global cube-to-world matrix and the map
// context's +0xA8/+0xAC scale and +0xB4/+0xB8 translation. Supplying these
// values explicitly keeps geometry independent of the live game singleton.
struct MinimapWorldTransform {
    std::array<float, 16> cube_to_world{};
    // Native 0x00767E70 first maps a stored offset-row cell through the
    // minimap grid's integer cube matrix, then projects that logical cube.
    std::array<std::int32_t, 16> map_to_cube{
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    float scale_x = 1.0f, scale_y = 1.0f;
    float offset_x = 0.0f, offset_y = 0.0f;
};

struct MinimapHexGeometry {
    std::int32_t map_x, map_y;
    std::array<std::int32_t, 3> cube;
    MinimapPoint2 center;
    std::array<MinimapPoint2, 6> corners;
};

inline MinimapPoint2 minimap_project_cube(const MinimapWorldTransform &transform,
                                          std::array<float, 4> cube) {
    const auto &m = transform.cube_to_world;
    // Preserve the native accumulation order of 0x006A8BC0.
    const float x = cube[0] * m[0] + m[8] * cube[2] + m[4] * cube[1] + m[12];
    const float y = cube[0] * m[1] + m[9] * cube[2] + m[5] * cube[1] + m[13];
    return {transform.scale_x * x - transform.offset_x,
            transform.scale_y * y - transform.offset_y};
}

inline MinimapHexGeometry minimap_hex_geometry(std::int32_t map_x, std::int32_t map_y,
                                                const MinimapWorldTransform &transform) {
    const auto q = map_x - (map_y >> 1);
    const auto r = map_y;
    const auto s = -q - r;
    const auto &m = transform.map_to_cube;
    const auto component = [&](std::size_t lane) {
        std::uint32_t sum = static_cast<std::uint32_t>(m[12 + lane]);
        sum += static_cast<std::uint32_t>(q) * static_cast<std::uint32_t>(m[lane]);
        sum += static_cast<std::uint32_t>(r) * static_cast<std::uint32_t>(m[4 + lane]);
        sum += static_cast<std::uint32_t>(s) * static_cast<std::uint32_t>(m[8 + lane]);
        return std::bit_cast<std::int32_t>(sum);
    };
    const auto logical_q = component(0);
    const auto logical_r = component(1);
    const auto logical_s = std::bit_cast<std::int32_t>(
        0u - static_cast<std::uint32_t>(logical_q) - static_cast<std::uint32_t>(logical_r));
    const auto center = minimap_project_cube(
        transform, {static_cast<float>(logical_q), static_cast<float>(logical_r),
                    static_cast<float>(logical_s), 1.0f});
    const auto origin = minimap_project_cube(transform, {0, 0, 0, 1});
    std::array<MinimapPoint2, 6> corners{};
    for (std::size_t i = 0; i < corners.size(); ++i) {
        const auto offset = minimap_project_cube(transform, minimap_cube_corners[i]);
        corners[i] = {center.x + (offset.x - origin.x),
                      center.y + (offset.y - origin.y)};
    }
    return {map_x, map_y, {logical_q, logical_r, logical_s}, center, corners};
}

inline std::vector<MinimapHexGeometry> project_minimap_hexes(
    const MinimapTileGrid &grid, const MinimapWorldTransform &transform) {
    for (float value : transform.cube_to_world)
        if (!std::isfinite(value))
            throw std::invalid_argument("Minimap cube transform is non-finite");
    if (!std::isfinite(transform.scale_x) || !std::isfinite(transform.scale_y) ||
        !std::isfinite(transform.offset_x) || !std::isfinite(transform.offset_y) ||
        transform.scale_x == 0 || transform.scale_y == 0)
        throw std::invalid_argument("Minimap map-space transform is invalid");
    std::vector<MinimapHexGeometry> hexes;
    hexes.reserve(static_cast<std::size_t>(grid.width()) * grid.height());
    for (std::uint32_t y = 0; y < grid.height(); ++y)
        for (std::uint32_t x = 0; x < grid.width(); ++x)
            hexes.push_back(minimap_hex_geometry(static_cast<std::int32_t>(x),
                                                  static_cast<std::int32_t>(y), transform));
    return hexes;
}
} // namespace civ5::ui
