#pragma once

#include "civ5/ui/minimap_hex_geometry.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace civ5::ui {

struct MinimapProjectedVertex {
    MinimapPoint2 clip;
    MinimapPoint2 world_uv;
    MinimapPoint2 fog_uv;
};

// Retail MiniMapShader setup uploads this pair from block +0x20 to pixel c1.
// The kind-5 city marker shader fades its outer radial edge from 0.8 to 1.0.
inline constexpr MinimapPoint2 minimap_city_fade{0.8f, 1.0f};

// 0x005CC580 writes the kind-7 pixel shader's g_vWaterTextureTiling (c3).
// PS 0x05D94A38 samples MiniMapWater.dds at world_xy*c3.x+c3.y.
inline constexpr MinimapPoint2 minimap_water_texture_tiling{0.002f, 100.0f};

// 0x005CC580 writes the shared MiniMapShader pixel-constant block. Kind 2
// reads CultureTexture tiling from c4 (+0x40); kind 9 reads IceTexture tiling
// from c5 (+0x50). Both use the same spatial scale as water, with different
// offsets into their authored DDS patterns.
inline constexpr MinimapPoint2 minimap_culture_texture_tiling{0.002f, 211.0f};
inline constexpr MinimapPoint2 minimap_ice_texture_tiling{0.002f, 176.0f};

// 0x0057D150 selects sampler record 2 for all eight minimap slots. The
// original 0x008E3BBD constructor and 0x008F6D60 D3D9 consumer establish
// U/V/W wrap, linear MAG/MIN/MIP, anisotropy 1 and sRGB texture decode.
// Entries are in that native consumer order, with sRGB as the final byte.
inline constexpr std::array<std::uint32_t, 8> minimap_sampler_2{
    1, 1, 1, 2, 2, 2, 1, 1};

inline MinimapPoint2 minimap_water_texture_uv(MinimapPoint2 world) {
    return {world.x * minimap_water_texture_tiling.x +
                minimap_water_texture_tiling.y,
            world.y * minimap_water_texture_tiling.x +
                minimap_water_texture_tiling.y};
}

// 0x006E1200 writes g_vOffset (c3) and g_vScale (c4) from the active world
// bounds. The common MiniMapShader VS at 0x05D935F8 uses them for kinds 0..4.
struct MinimapClipProjection {
    MinimapPoint2 offset;
    MinimapPoint2 scale;

    MinimapProjectedVertex project(MinimapPoint2 world) const {
        const MinimapPoint2 clip{world.x * scale.x - offset.x,
                                 world.y * scale.y - offset.y};
        // VS o0 receives raw world XY. VS o1 is the screen-aligned fog UV:
        // x*0.5+0.5, y*(-0.5)+0.5. Preserve this Y sign in GL and readbacks.
        return {clip, world,
                {clip.x * 0.5f + 0.5f, clip.y * -0.5f + 0.5f}};
    }

    // Draw kind 7 (water) receives a unit quad rather than world vertices.
    // VS 0x05D93900 maps its UV through g_vWorldVisibleMin/Max before the
    // shared clip/fog transform. The resulting world XY is also its texture UV.
    MinimapProjectedVertex project_visible_quad(MinimapPoint2 quad_uv,
                                                MinimapPoint2 visible_min,
                                                MinimapPoint2 visible_max) const {
        const MinimapPoint2 world{
            quad_uv.x * (visible_max.x - visible_min.x) + visible_min.x,
            quad_uv.y * (visible_max.y - visible_min.y) + visible_min.y};
        return project(world);
    }
};

inline MinimapClipProjection minimap_clip_projection(float min_x, float min_y,
                                                      float max_x, float max_y) {
    if (!std::isfinite(min_x) || !std::isfinite(min_y) ||
        !std::isfinite(max_x) || !std::isfinite(max_y) ||
        !(max_x > min_x) || !(max_y > min_y))
        throw std::invalid_argument("Invalid native minimap world bounds");
    const float middle_x = (max_x + min_x) * 0.5f;
    const float middle_y = (max_y + min_y) * 0.5f;
    const float scale_x = 2.0f / (max_x - min_x);
    const float scale_y = 2.0f / (max_y - min_y);
    if (!std::isfinite(scale_x) || !std::isfinite(scale_y) ||
        scale_x == 0.0f || scale_y == 0.0f)
        throw std::invalid_argument("Native minimap projection is not finite");
    return {{middle_x * scale_x, middle_y * scale_y},
            {scale_x, scale_y}};
}

} // namespace civ5::ui
