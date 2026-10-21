#pragma once
#include "civ5/materials/material_mesh.hpp"
#include "civ5/materials/material_shadows.hpp"
#include "gl_context.hpp"
#include <string_view>

namespace civ5::graphics {
std::string_view material_shadow_vertex_shader();
std::string material_shadow_fragment_shader();
// Original selector3: native material48 POSITION+0 / TEXCOORD0+12, world ALPHA.
// Owns program/mesh/sampler0, retains context, same render-thread lifetime.
class GlMaterialShadowRenderer {
  public:
    GlMaterialShadowRenderer(std::shared_ptr<GlContext> context, const materials::MaterialPatchMesh &mesh);
    ~GlMaterialShadowRenderer();
    GlMaterialShadowRenderer(const GlMaterialShadowRenderer &) = delete;
    GlMaterialShadowRenderer &operator=(const GlMaterialShadowRenderer &) = delete;
    // Caller owns framebuffer, viewport, blend/depth/raster state. Source must
    // be a complete2D texture in this context, with NO output-storage alias.
    // Direct names are checked; texture-view aliases remain caller responsibility.
    // Local convention: GL row0 is native top row, with explicit Z/half-pixel
    // conversion. Not a proof of whole material/normal/LAND orientation.
    void draw(GLuint world_height, const materials::MaterialShadowParameters &parameters, int width,
              int height, const std::array<float, 2> &texel_offsets = {0, 0});
    // Caller clears black. Sets separate ONE+ONE/ADD, all channels, then leaves
    // blending DISABLED. Validates every sample before making any target writes.
    // Empty span emits no draws. Other state is still caller-owned.
    void draw_accumulated(GLuint world_height, std::span<const materials::MaterialShadowParameters> samples,
                          int width, int height);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
