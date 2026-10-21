#pragma once
#include "civ5/materials/material_assets.hpp"
#include "gl_texture.hpp"

namespace civ5::graphics {
// Owns real raw DDS resources and lazily generated entry0. Caller supplies
// exact authored sources; source rows and local U16 IDs never become GL names.
// Context and source data remain alive. All use/destruction on render thread.
class GlMaterialTextures {
  public:
    GlMaterialTextures(std::shared_ptr<GlContext> context,
                       std::shared_ptr<const materials::MaterialAssets> source);
    GlMaterialTextures(const GlMaterialTextures &) = delete;
    GlMaterialTextures &operator=(const GlMaterialTextures &) = delete;
    const GlTexture &resource(std::uint16_t local_id);
    // Resolves all16 names before binding units0..15. Sampler belongs to caller.
    std::array<GLuint, 16> bind(std::span<const std::int32_t> selected);
    // Reupload every resource that has already been materialized on a
    // different live context. The retained source remains authoritative.
    void rebuild_context(std::shared_ptr<GlContext> context);

  private:
    std::shared_ptr<GlContext> context_;
    std::shared_ptr<const materials::MaterialAssets> source_;
    std::vector<std::unique_ptr<GlTexture>> resources_;
};
} // namespace civ5::graphics
