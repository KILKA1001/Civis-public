#pragma once
#include "civ5/materials/lighting_sources.hpp"
#include "gl_texture.hpp"

namespace civ5::graphics {
// Ten authored environment cubes with explicit original sampler2 sRGB decode.
// Handles here are real GL names, NOT the native low16 resource-table indices.
// Each texture retains the creating context; use/destruction stay on its thread.
class GlLightingCubes {
  public:
    GlLightingCubes(const GlContext &context, const materials::LightingCubeAssets &assets);
    GlLightingCubes(const GlLightingCubes &) = delete;
    GlLightingCubes &operator=(const GlLightingCubes &) = delete;
    const GlTexture &sunlight(unsigned style) const;
    const GlTexture &shadowlight(unsigned style) const;
    // Reupload the retained ten authored cubes on a different live context.
    void rebuild_context(std::shared_ptr<GlContext> context);

  private:
    std::shared_ptr<const materials::LightingCubeAssets> assets_;
    std::vector<GlTexture> textures_;
};
} // namespace civ5::graphics
