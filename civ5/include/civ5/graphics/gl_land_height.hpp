#pragma once
#include "civ5/terrain/height_addressing.hpp"
#include "gl_context.hpp"
#include <string_view>

namespace civ5::graphics {
std::string_view land_height_vertex_shader();
std::string_view land_height_fragment_shader();
// Native local-grid displacement with height-only diagnostic presentation.
// Explicit field, camera and immutable index step; NOT runtime LOD/visibility.
// Owns resources + field/context lifetimes; creating render thread only.
class GlLandHeightRenderer {
  public:
    GlLandHeightRenderer(std::shared_ptr<GlContext> context,
                         std::shared_ptr<const terrain::HeightField> field, int index_step);
    ~GlLandHeightRenderer();
    GlLandHeightRenderer(const GlLandHeightRenderer &) = delete;
    GlLandHeightRenderer &operator=(const GlLandHeightRenderer &) = delete;
    const terrain::LandLayout &layout() const;
    // Caller owns framebuffer, viewport, depth/blend/cull state. Binds program,
    // VAO, height texture+sampler0; does not change raster enable state.
    void draw(int column, int row, const std::array<float, 16> &view_projection);
    // Actual linked-program transform feedback: world XYZW + height UV.
    // Restores TF binding/discard; rejects an already active feedback operation.
    std::vector<float> capture(int column, int row, bool indexed);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
