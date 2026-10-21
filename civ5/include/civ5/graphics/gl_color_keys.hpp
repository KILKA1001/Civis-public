#pragma once
#include "civ5/materials/color_keys.hpp"
#include "gl_texture.hpp"

namespace civ5::graphics {
class GlColorKeys {
  public:
    GlColorKeys(const GlContext &context, const materials::ColorKeyAssets &assets);
    const GlTexture &at(materials::ColorKeySystem system, unsigned style) const;
    // key4B slot8 uses Terrain; normal LAND slots6/7 use TerrainFOW/Shadow.
    std::array<GLuint, 5> color_correct() const;
    bool belongs_to(const GlContext &context) const;
    // Reupload all 25 retained ColorKey volumes on a different live context.
    void rebuild_context(std::shared_ptr<GlContext> context);

  private:
    std::shared_ptr<const materials::ColorKeyAssets> assets_;
    std::vector<GlTexture> volumes_;
};
} // namespace civ5::graphics
