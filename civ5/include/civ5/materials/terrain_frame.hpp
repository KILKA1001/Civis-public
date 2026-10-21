#pragma once
#include "civ5/materials/distance_fog.hpp"
#include "civ5/materials/water_reflection.hpp"
#include "civ5/materials/land_material.hpp"

namespace civ5::materials {
struct TerrainLighting {
    WaterReflectionAngles angles;
    float scalar;
    terrain::Vector3 shadow, sun, reflection;
};
// Original startup 5494EA..549608 uses stored double radians directly. The
// subsequent debug-menu update uses the separately stored float degree angles.
TerrainLighting terrain_lighting_defaults(terrain::X87Precision precision);
TerrainLighting terrain_lighting_update(const TerrainLighting &state, terrain::X87Precision precision);
struct TerrainCacheLighting {
    terrain::Vector3 shadow, half_vector;
};
// Original cache preparation calls 4372F0 with identity camera matrices. Its
// half-vector therefore uses +Z, independently of the active scene camera.
TerrainCacheLighting terrain_cache_lighting(const TerrainLighting &lighting,
                                            const terrain::ReciprocalSqrtEstimate &estimate);

struct TerrainFrameInput {
    terrain::Matrix4 view, inverse_view, projection, shadow_projection, overlay_projection;
    terrain::Vector3 world_scales, shadow_direction, sun_direction;
    std::array<std::uint32_t, 2> dimensions, screen;
    terrain::Vector4 overlay;
    terrain::Vector4 fog_color;
    // fog_distance is the historical API name of the ramp's normalized zoom
    // coordinate. It does not contain a distance in world units.
    float fog_density, fog_strength, fog_distance;
    float texture_extent, control;
    std::uint32_t level;
    // Original terrain owner fields 2404, 240C, 2418 and 241C.
    std::array<std::uint32_t, 4> options;
};
struct TerrainFrameState {
    // Original global wrapper12: VS c0..34; PS c0..20 starts at vector16.
    // The producer preserves every unwritten component of the prior block.
    std::array<terrain::Vector4, 37> constants{};
    // Original global wrapper19: PS c21..22, no vertex constants.
    std::array<terrain::Vector4, 2> fog{};
    std::uint32_t minimum_level = 0xffffffffu, maximum_level = 0;
};
// Original 4372F0 fog wrapper calculation. The height product keeps the x87
// precision of LAND; the water producer has a separate float32 product.
std::array<terrain::Vector4, 2> terrain_fog_constants(const DistanceFogFrame &fog, float camera_height,
                                                      terrain::X87Precision precision);
// Complete arithmetic/state port of 4372F0. Wrapper cloning and queue lifetime
// are owned by the rendering layer; the numerical block is identical in both paths.
TerrainFrameState terrain_frame(const TerrainFrameInput &input, const TerrainFrameState &prior,
                                terrain::X87Precision precision,
                                const terrain::ReciprocalSqrtEstimate &estimate);
// Map original global wrapper registers to the complete cached LAND shader.
// Patch world/offset/height fields are supplied by the material world owner.
LandMaterialConstants land_material_frame(const TerrainFrameState &frame);
} // namespace civ5::materials
