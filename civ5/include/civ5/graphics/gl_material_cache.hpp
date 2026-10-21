#pragma once
#include "civ5/materials/material_cache.hpp"
#include "gl_texture.hpp"

namespace civ5::graphics {
// Explicit effective automatic-mip behavior for the three recovered formats.
// The original D3D9 request may be accepted without automatic mip support.
struct MaterialCacheMipPolicy {
    bool rgba16_float, rgba8, rgb565;
};
// Owns physical cache resources and their context. The allocation plan comes
// from the production material-cache producer, not captured resource handles.
// Raw compression intermediates and DXT rendering remain unsupported and fail
// before any allocation. No format substitution or hidden sRGB conversion.
class GlMaterialCacheTextures {
  public:
    GlMaterialCacheTextures(std::shared_ptr<GlContext> context,
                            std::span<const materials::MaterialCacheTexturePlan> plans,
                            MaterialCacheMipPolicy mip_policy);
    GlMaterialCacheTextures(const GlMaterialCacheTextures &) = delete;
    GlMaterialCacheTextures &operator=(const GlMaterialCacheTextures &) = delete;
    GlMaterialCacheTextures(GlMaterialCacheTextures &&) noexcept = default;
    GlMaterialCacheTextures &operator=(GlMaterialCacheTextures &&) noexcept = default;
    std::span<const materials::MaterialCacheTexturePlan> plans() const { return plans_; }
    const GlTexture &texture(materials::MaterialCacheRole role, std::uint32_t index = 0) const;
    void generate_mips(materials::MaterialCacheRole role, std::uint32_t index = 0);
    // Recreate the physical cache textures on a different live context. The
    // caller must regenerate cache pixels after this operation.
    void rebuild_context(std::shared_ptr<GlContext> context);

  private:
    std::size_t locate(materials::MaterialCacheRole role, std::uint32_t index) const;
    std::shared_ptr<GlContext> context_;
    std::vector<materials::MaterialCacheTexturePlan> plans_;
    MaterialCacheMipPolicy mip_policy_;
    std::vector<GlTexture> textures_;
};
} // namespace civ5::graphics
