#pragma once
#include "civ5/terrain/world_layout.hpp"
#include <span>
#include <vector>

namespace civ5::terrain {
class TerrainWorld;
}
namespace civ5::materials {
struct WaterSurfaceDomain {
    std::int32_t field_width, field_height;
    float origin_x, origin_y, span_x, span_y;
    terrain::Vector3 scales;
    static WaterSurfaceDomain from_world(const terrain::WorldLayout &world);
    static WaterSurfaceDomain from_world(const terrain::TerrainWorld &world);
};
// Original layout 14 is one float4 (world X/Y, height-window U/V). The ocean
// plane's height is supplied by a shader constant, never baked into this stream.
struct WaterSurfaceMesh {
    std::vector<terrain::Vector4> vertices;
    std::vector<std::uint16_t> indices, counts, prefixes;
    std::uint32_t capacity;
    bool reallocate;
};
WaterSurfaceMesh build_water_surface_mesh(const WaterSurfaceDomain &domain,
                                          std::span<const terrain::Vector4> view_bounds,
                                          std::uint32_t prior_capacity, terrain::X87Precision precision);

using WaterConstantBlock = std::array<float, 145>;
struct WaterShaderRegisters {
    std::array<float, 68> vertex{};
    std::array<float, 120> pixel{};
};
// Original key5B global wrapper 28: VS c4..16 receives source vectors 0..12;
// PS c4..27 receives source vectors 13..36. The final three source components
// are not stored by the producer or consumed by these shaders and are zeroed.
// Local World c0..3 is supplied separately for the river vertex shader.
WaterShaderRegisters water_shader_registers(const WaterConstantBlock &constants,
                                            const terrain::Matrix4 &world = terrain::identity_matrix());
struct WaterSurfaceSettings {
    float reflection_amount;
    terrain::Vector4 specular_sun, specular_sky;
    terrain::Vector3 sun_direction, sky_direction;
    terrain::Vector4 fake_view_sun, fake_view_sky;
    float ocean_height, gradient_scale, frequency, river_phase, water_phase;
};
struct WaterSurfaceView {
    terrain::Matrix4 view, overlay, shadow, reflection;
    terrain::Vector3 camera;
    // Original partial scalar/vector stores preserve the other components.
    WaterConstantBlock prior_constants;
};
struct WaterSurfaceFrame {
    terrain::Matrix4 projection;
    terrain::Vector4 fog_color, fog_parameters;
    float shadow_fade;
};
WaterConstantBlock water_surface_constants(const WaterSurfaceDomain &domain,
                                           const WaterSurfaceSettings &settings,
                                           const WaterSurfaceFrame &frame, const WaterSurfaceView &view,
                                           terrain::X87Precision precision);

struct WaterSurfaceResources {
    std::uint32_t vertex_buffer, index_buffer;
    std::uint32_t lean_rgba, lean_luminance, normal_color;
    // Original globals D91114, D91110, D91104, D910F4, D910FC, D80CB0, D9110C.
    std::array<std::uint32_t, 7> shared;
    std::array<std::uint32_t, 256> height_windows;
};
struct WaterSurfaceDraw {
    std::uint32_t view, window, selector, first_index, triangle_count;
    std::array<std::uint8_t, 100> packet;
};
// Resolve resources after any mesh capacity change. Handles are logical values;
// the portable runtime never dereferences the original 32-bit device pointers.
std::vector<WaterSurfaceDraw> water_surface_draws(const WaterSurfaceMesh &mesh,
                                                  const WaterSurfaceResources &resources, unsigned selector);
} // namespace civ5::materials
