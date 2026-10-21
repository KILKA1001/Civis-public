#pragma once
#include "civ5/materials/material_cache_free_lists.hpp"
#include "gl_material_cache.hpp"

namespace civ5::graphics {
// Original uncompressed pool descriptors own physical RGB565 storage through
// GlMaterialCacheTextures. Logical handles are 1-based plan indices, not GL
// texture names. Acquisition and returns reuse storage without GPU allocation.
// The owner and all its resources remain on the creating render thread.
class GlMaterialCachePool final : public materials::MaterialCacheTexturePool {
  public:
    GlMaterialCachePool(std::shared_ptr<GlContext> context, std::span<const std::uint32_t, 4> counts,
                        bool automatic_mips);
    GlMaterialCachePool(const GlMaterialCachePool &) = delete;
    GlMaterialCachePool &operator=(const GlMaterialCachePool &) = delete;
    std::uint32_t take(std::uint32_t requested_size, std::uint32_t &actual_size) override;
    void give(std::uint32_t handle) override;
    // Recreate all physical pool textures on a different live context. All
    // logical handles must be available, and the caller must regenerate pixels.
    void rebuild_context(std::shared_ptr<GlContext> context);
    // Only a currently leased texture can be used for rendering or sampling.
    const GlTexture &texture(std::uint32_t handle) const;
    const materials::MaterialCacheFreeLists &state() const { return state_; }
    std::span<const materials::MaterialCacheTexturePlan> plans() const { return textures_.plans(); }

  private:
    std::shared_ptr<GlContext> context_;
    GlMaterialCacheTextures textures_;
    materials::MaterialCacheFreeLists state_;
    std::vector<const GlTexture *> handles_;
};
} // namespace civ5::graphics
