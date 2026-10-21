#pragma once
#include "civ5/materials/water_current.hpp"
#include "gl_context.hpp"
#include <string_view>

namespace civ5::graphics {
std::string_view water_current_vertex_shader();
std::string_view water_current_fragment_shader();
class GlWaterCurrentRenderer {
  public:
    explicit GlWaterCurrentRenderer(std::shared_ptr<GlContext>);
    ~GlWaterCurrentRenderer();
    GlWaterCurrentRenderer(const GlWaterCurrentRenderer &) = delete;
    GlWaterCurrentRenderer &operator=(const GlWaterCurrentRenderer &) = delete;
    // Caller owns target/raster state. Ordinary original source is resource0;
    // explicit alternate height inputs are useful for independent shader tests.
    void draw(const materials::WaterCurrentConstants &, GLuint source, int width, int height);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
