#pragma once
#include "civ5/materials/river_surface_draw.hpp"
#include "civ5/materials/water_surface.hpp"
#include "gl_context.hpp"
#include <string_view>

namespace civ5::graphics {
std::string_view river_surface_vertex_shader();
std::string_view river_surface_fragment_shader(unsigned selector);
struct GlRiverSurfaceInputs {
    // Original slots: LEAN0..3, reflection4, alpha5, sRGB color6, unused7,
    // fog ramp8, overlay9, FOW10, TransFOWColor volume11, ColorKey volume12.
    // LEAN0/2 require RGBA16 UNORM; LEAN1/3 require R16 UNORM with L16 swizzle.
    std::array<GLuint, 13> pixel{};
};
class GlRiverSurfaceRenderer {
  public:
    GlRiverSurfaceRenderer(std::shared_ptr<GlContext>, const terrain::RiverSurfaceMesh &, unsigned selector);
    GlRiverSurfaceRenderer(const GlRiverSurfaceRenderer &pipeline, const terrain::RiverSurfaceMesh &);
    ~GlRiverSurfaceRenderer();
    GlRiverSurfaceRenderer(const GlRiverSurfaceRenderer &) = delete;
    GlRiverSurfaceRenderer &operator=(const GlRiverSurfaceRenderer &) = delete;
    // Caller owns target and depth attachments, viewport and texture-view alias
    // exclusions. Applies original blend8/depth4 state and draws the owned mesh.
    void draw(const materials::WaterConstantBlock &, const GlRiverSurfaceInputs &, int width, int height);
    // Validation capture: native clip4 + T0..3 + world3 + clip3 + T6 = 22 floats.
    std::vector<float> capture(const materials::WaterConstantBlock &, bool indexed);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
