#pragma once
#include "gl_texture.hpp"
#include "civ5/materials/water_reflection.hpp"
#include <functional>

namespace civ5::graphics {
// Owns original reflection color storage and retains the renderer's shared
// depth surface. Shared depth dimensions/format are explicit device inputs;
// this class does not infer them from the reflection texture dimensions.
class GlWaterReflectionMap {
  public:
    GlWaterReflectionMap(std::shared_ptr<GlContext> context, std::uint32_t quality,
                         std::uint32_t reflections, std::shared_ptr<const GlTexture> shared_depth = {});
    ~GlWaterReflectionMap();
    GlWaterReflectionMap(const GlWaterReflectionMap &) = delete;
    GlWaterReflectionMap &operator=(const GlWaterReflectionMap &) = delete;
    const materials::WaterReflectionResource &resource() const;
    // Target pixels become available after a successful render. Fallback
    // pixels are complete at construction and do not require a render pass.
    const GlTexture &texture() const;
    // Original clear pass, followed by caller-supplied reflection draws.
    // Establishes original blend8/depth0 state; ends on framebuffer zero,
    // including when draws throw. No child renderer is silently synthesized.
    void render(const std::function<void()> &draws = {});

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
