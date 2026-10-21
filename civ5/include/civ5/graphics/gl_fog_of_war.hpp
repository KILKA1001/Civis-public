#pragma once
#include "gl_render_api.hpp"
#include "civ5/materials/fog_of_war_frame.hpp"
#include <memory>
#include <string_view>

namespace civ5::graphics {
// Techniques 0..4: original key52 selectors; technique5: key53 selector0.
std::string_view fog_of_war_vertex_shader(unsigned technique);
std::string_view fog_of_war_fragment_shader(unsigned technique);
// Geometry and shader owner. The frame owner supplies targets and render state.
class GlFogOfWarRenderer {
  public:
    explicit GlFogOfWarRenderer(std::shared_ptr<GlContext> context);
    ~GlFogOfWarRenderer();
    GlFogOfWarRenderer(const GlFogOfWarRenderer &) = delete;
    GlFogOfWarRenderer &operator=(const GlFogOfWarRenderer &) = delete;
    void draw(const materials::FogOfWarDraw &draw, int width, int height, GLuint cloud_texture = 0,
              GLuint mask_texture = 0);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
