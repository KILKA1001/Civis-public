#pragma once
#include "civ5/materials/lighting_mesh.hpp"
#include "gl_texture.hpp"
#include <string_view>

namespace civ5::graphics {
// Ordinary remains the prior default. Measured modes are explicit device-scoped
// translations, not universal cube precision or a solution for zero directions.
enum class LightingCubeSampling { ordinary, integer_no_coordinates, measured_coordinates };
std::string_view material_lighting_vertex_shader();
std::string material_lighting_fragment_shader(bool specular);
// Binary project-side observation aid, NOT an original Civ V resource.
GlTexture make_lighting_coordinate_probe(const GlContext &context);

class GlMaterialLightingRenderer {
  public:
    GlMaterialLightingRenderer(std::shared_ptr<GlContext> context, const materials::LightingPatchMesh &mesh,
                               bool specular, LightingCubeSampling sampling = LightingCubeSampling::ordinary);
    ~GlMaterialLightingRenderer();
    GlMaterialLightingRenderer(const GlMaterialLightingRenderer &) = delete;
    GlMaterialLightingRenderer &operator=(const GlMaterialLightingRenderer &) = delete;
    // Caller owns target(s), viewport, blend/depth/raster/formats. All source
    // textures must be sampling-complete and have no output-storage alias.
    // Direct name aliases checked; view-storage aliases remain caller-owned.
    // Owns/restores seamless-cube enable, not the rest of the GL draw state.
    void draw(GLuint normal, GLuint albedo, GLuint shadow, GLuint spec, const GlTexture &sun,
              const GlTexture &shade, GLuint color_correct, int width, int height,
              const std::array<float, 3> &half = {0, 0, 1});

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
