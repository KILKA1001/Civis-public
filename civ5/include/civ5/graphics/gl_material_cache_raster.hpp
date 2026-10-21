#pragma once
#include "civ5/materials/material_cache_passes.hpp"
#include "gl_texture.hpp"

namespace civ5::graphics {
struct GlMaterialCacheTarget {
    std::uint32_t handle;
    const GlTexture *texture;
};
// Owns the FBO and applies the recovered D3D9 normal-pass state slice for
// height/normal/shadow/lighting/decal passes (blend0/4/8, depth9, default viewport).
// Attachments retain their resource owner outside this object. begin validates
// the complete target mapping before modifying GL state. The pass does not
// preserve previous framebuffer/raster state; subsequent passes set their own.
class GlMaterialCacheRaster {
  public:
    explicit GlMaterialCacheRaster(std::shared_ptr<GlContext> context);
    ~GlMaterialCacheRaster();
    GlMaterialCacheRaster(const GlMaterialCacheRaster &) = delete;
    GlMaterialCacheRaster &operator=(const GlMaterialCacheRaster &) = delete;
    // Read-only preflight, also used by the ordered executor before any clears.
    void validate(const materials::MaterialCachePass &pass,
                  std::span<const GlMaterialCacheTarget> targets) const;
    void begin(const materials::MaterialCachePass &pass, std::span<const GlMaterialCacheTarget> targets,
               bool conditional_clear);
    void end();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
