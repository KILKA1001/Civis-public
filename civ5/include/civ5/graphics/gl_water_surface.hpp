#pragma once
#include "civ5/materials/water_surface.hpp"
#include "gl_context.hpp"
#include <string_view>

namespace civ5::graphics {
// Vertex source retains D3D clip Z; draw() supplies GL_ZERO_TO_ONE and the
// half-pixel viewport offset, restoring the caller's clip/viewport state.
std::string_view water_surface_vertex_shader();
std::string_view water_surface_fragment_shader(unsigned selector);
struct GlWaterSurfaceInputs {
    // Original pixel sampler slots. Slot 7 is the current height window,
    // slot 11 is a volume LUT, slot 6 requires an sRGB storage view.
    // Slot 5 is the produced current map; slot 4 is the reflection map.
    std::array<GLuint, 14> pixel{};
};
class GlWaterSurfaceRenderer {
  public:
    GlWaterSurfaceRenderer(std::shared_ptr<GlContext>, const materials::WaterSurfaceMesh &,
                           unsigned selector);
    // Allocate independent geometry while retaining the same immutable shader
    // and sampler resources. Both renderers keep their own mesh and lifetime.
    GlWaterSurfaceRenderer(const GlWaterSurfaceRenderer &pipeline, const materials::WaterSurfaceMesh &);
    ~GlWaterSurfaceRenderer();
    GlWaterSurfaceRenderer(const GlWaterSurfaceRenderer &) = delete;
    GlWaterSurfaceRenderer &operator=(const GlWaterSurfaceRenderer &) = delete;
    // Caller owns target, raster/depth/blend state and excludes storage aliases.
    // A range is the original packet's first index and triangle count.
    void draw(const materials::WaterConstantBlock &, const GlWaterSurfaceInputs &, std::uint32_t first_index,
              std::uint32_t triangle_count, int width, int height);
    // Original pre-API clip4 + T0..3 (8) + world3 + clip3 + T6 (4): 22 floats.
    std::vector<float> capture(const materials::WaterConstantBlock &, bool indexed);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
