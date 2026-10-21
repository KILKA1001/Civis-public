#pragma once
#include "civ5/materials/material_mesh.hpp"
#include "gl_context.hpp"
#include <array>
#include <string_view>

namespace civ5::graphics {
std::string_view material_blend_vertex_shader();
std::string material_blend_cache_vertex_shader();
std::string_view material_sampling_glsl();
std::string material_blend_fragment_shader();
enum class MaterialSampling { point_control, ordinary0 };
enum class MaterialBlendCoordinates { gl_control, d3d9_cache };
// Original05D97CD0/05D9AA70 operation order, material48 declaration and two
// distinct cache outputs. No epsilon denominator repair or height-field alias.
// Owns geometry/program/samplers, retains context. Live 2D texture names must
// belong to that same context and remain alive for draw; no captured native
// handles.
class GlMaterialBlendRenderer {
  public:
    GlMaterialBlendRenderer(std::shared_ptr<GlContext> context, const materials::MaterialPatchMesh &mesh,
                            MaterialBlendCoordinates coordinates = MaterialBlendCoordinates::gl_control);
    ~GlMaterialBlendRenderer();
    GlMaterialBlendRenderer(const GlMaterialBlendRenderer &) = delete;
    GlMaterialBlendRenderer &operator=(const GlMaterialBlendRenderer &) = delete;
    // Caller owns MRT formats/attachments, viewport, raster/depth/blend state.
    // Binds program, VAO, and16 textures/samplers. Ordinary0 is linear repeat,
    // no sRGB, anisotropy1, default LOD range/bias; point is a diagnostic
    // control. Original BC3 materials use measured hardware mip filtering;
    // RGBA8 resource0 retains explicit interpolation of integer mip levels.
    void draw(std::span<const float, 384> coefficients, const std::array<float, 2> &texel_offsets,
              const std::array<GLuint, 16> &textures, MaterialSampling sampling,
              std::array<std::int32_t, 2> target_size = {0, 0});
    // d3d9_cache requires positive target_size. It retains native shader offsets,
    // converts integer pixel centers, depth and Y to GL, and stores native
    // top-to-bottom texture rows in GL's increasing texture-row order.
    // Actual linked-program output: clip XYZW,8UV pairs,8 normalized factors.
    // TF binding/discard restored; nested active transform feedback rejected.
    std::vector<float> capture(std::span<const float, 384> coefficients,
                               const std::array<float, 2> &texel_offsets, bool indexed = false);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
