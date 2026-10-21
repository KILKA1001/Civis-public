#pragma once
#include "civ5/graphics/gl_fog_of_war_map.hpp"

namespace civ5::graphics {
struct GlFogOfWarMap::Impl {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    materials::FogOfWarStartup state;
    GlTexture mask;
    std::vector<GlTexture> clouds;
    GlFogOfWarRenderer renderer;
    GLuint framebuffer = 0;
    bool rendering = false;
    std::uint64_t render_revision = 0;
    Impl(std::shared_ptr<GlContext> c, const materials::FogOfWarAssets &assets);
    ~Impl();
    void bind(const GlTexture &color, const GlTexture *depth);
};
} // namespace civ5::graphics
