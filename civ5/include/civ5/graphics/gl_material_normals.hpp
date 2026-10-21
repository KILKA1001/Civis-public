#pragma once
#include "gl_context.hpp"
#include <array>
#include <cstdint>
#include <string_view>

namespace civ5::graphics {
std::string_view material_normal_vertex_shader();
std::string material_normal_fragment_shader();
// Key4A /05D99CC0: eight RED taps of material MRT1, scale16, alpha0.
// Owns program/VAO/clamp-linear sampler5 and retains its creating context.
class GlMaterialNormalRenderer {
  public:
    explicit GlMaterialNormalRenderer(std::shared_ptr<GlContext> context);
    ~GlMaterialNormalRenderer();
    GlMaterialNormalRenderer(const GlMaterialNormalRenderer &) = delete;
    GlMaterialNormalRenderer &operator=(const GlMaterialNormalRenderer &) = delete;
    // Caller owns viewport, raster/depth/blend state and framebuffer/formats.
    // Source must be a complete live 2D texture in this context, remain alive
    // through draw, and share NO storage with any output attachment (including
    // texture views). Direct same-name framebuffer feedback is rejected; view
    // aliasing is caller's responsibility. Binds program, VAO, texture0 and the
    // separate sampler5.
    void draw(GLuint material_height, std::int32_t resolution,
              const std::array<float, 4> &corners = {0, 0, 1, 1});
    // Original normal quad has UV0 at the first D3D9 pixel center. Texture rows
    // use the same convention as the blend renderer's d3d9_cache mode.
    void draw_cache(GLuint material_height, std::int32_t resolution);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
