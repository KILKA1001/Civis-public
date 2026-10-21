#pragma once
#include "civ5/terrain/river_surface.hpp"
#include <optional>

namespace civ5::materials {
struct RiverSurfaceUpload {
    std::uint32_t vertex_count, index_count, vertex_bytes, index_bytes;
    bool active;
};
// Original 5654B0 uses format 10 / stride 32 and D3DFMT_INDEX32. Managed
// allocation capacities round up to 8192 vertex bytes and 4096 index bytes.
RiverSurfaceUpload river_surface_upload(const terrain::RiverSurfaceMesh &);

struct RiverSurfaceResources {
    std::uint32_t vertex_buffer, index_buffer;
    // 0/1 LEAN1, 2/3 LEAN2, 4 ReflectionMap, 5 WaterBlendMap, 6 WaterColor,
    // 7 local texture (unused by these shaders), 8 WaterFogRamp, 9 OverlayMap,
    // 10 FOWMask, 11 TransFOWColor volume, 12 ColorKey volume.
    // These are logical original handles; packet fields retain their low 16 bits.
    std::array<std::uint32_t, 13> pixel_textures;
};
struct RiverSurfaceDraw {
    std::array<std::uint8_t, 100> packet{};
    terrain::Matrix4 world = terrain::identity_matrix();
    std::uint32_t selector, triangle_count, vertex_capacity;
    // Original pass 8/4: SRCALPHA/INVSRCALPHA RGB, ONE/ONE alpha;
    // depth test and writes enabled, LESSEQUAL, clockwise faces culled.
    static constexpr std::uint32_t blend_state = 8, depth_state = 4, primitive = 4;
};
// Complete triangles and finite consumed vertex lanes are required. Empty
// streams submit no draw. Unused COLOR0 bits do not participate in validation.
std::optional<RiverSurfaceDraw> river_surface_draw(const terrain::RiverSurfaceMesh &,
                                                   const RiverSurfaceResources &, std::uint32_t quality);
} // namespace civ5::materials
