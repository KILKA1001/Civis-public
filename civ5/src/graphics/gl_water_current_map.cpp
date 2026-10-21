#include "civ5/graphics/gl_water_current_map.hpp"
#include "civ5/graphics/gl_water_current.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/materials/material_sources.hpp"
#include <exception>
#include <stdexcept>

namespace civ5::graphics {
namespace {
constexpr unsigned extent = 256;
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
} // namespace
struct GlWaterCurrentMap::Impl {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    materials::WaterCurrentConstants constants;
    GlTexture source, target;
    GlWaterCurrentRenderer renderer;
    GLuint framebuffer = 0;
    Impl(std::shared_ptr<GlContext> c, materials::WaterCurrentConstants values, bool conditional_clear)
        : context(std::move(c)), gl(*context), constants(values),
          source(GlTexture::rgba8(*context, materials::material_resource_zero())),
          target(GlTexture::render_target(*context, extent, extent, 1, RenderTextureFormat::rgba8)),
          renderer(context) {
        try {
            context->check("Before owned water current pass");
            gl.CreateFramebuffers(1, &framebuffer);
            gl.NamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0, target.id(), 0);
            const GLenum buffer = GL_COLOR_ATTACHMENT0;
            gl.NamedFramebufferDrawBuffers(framebuffer, 1, &buffer);
            require(gl.CheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                    "Incomplete water current target");
            gl.BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            // The original current pass is ordinary dispatch, blend0/depth9,
            // flags0x80, target mip0 and descriptor-sized viewport. No depth
            // or stencil surface is attached. All sixteen textures unbind.
            for (unsigned slot = 0; slot < 16; ++slot)
                context->api().BindTextureUnit(slot, 0);
            gl.Viewport(0, 0, extent, extent);
            for (auto state : {GL_DEPTH_TEST, GL_STENCIL_TEST, GL_BLEND, GL_CULL_FACE,
                               GL_POLYGON_OFFSET_FILL, GL_POLYGON_OFFSET_LINE, GL_SCISSOR_TEST,
                               GL_DITHER, GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD})
                gl.Disable(state);
            GLint clip_planes = 0;
            context->api().GetIntegerv(GL_MAX_CLIP_DISTANCES, &clip_planes);
            for (GLint i = 0; i < clip_planes; ++i)
                gl.Disable(GL_CLIP_DISTANCE0 + i);
            gl.DepthMask(GL_FALSE);
            gl.DepthFunc(GL_LEQUAL);
            gl.PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            gl.BlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
            gl.BlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
            if (conditional_clear) {
                const float clear[]{0, 0, 0, 0};
                gl.ClearNamedFramebufferfv(framebuffer, GL_COLOR, 0, clear);
            }
            renderer.draw(constants, source.id(), extent, extent);
            gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
            context->check("Owned original water current map");
        } catch (...) {
            release();
            throw;
        }
    }
    void release() noexcept {
        try {
            if (framebuffer) {
                context->api();
                gl.DeleteFramebuffers(1, &framebuffer);
                framebuffer = 0;
            }
        } catch (...) {
            std::terminate();
        }
    }
    ~Impl() { release(); }
};
GlWaterCurrentMap::GlWaterCurrentMap(std::shared_ptr<GlContext> context, std::int32_t width,
                                     std::int32_t height, materials::WaterCurrentSettings settings,
                                     bool conditional_clear) {
    require(bool(context), "Water current map requires an owned context");
    require(width > 0 && height > 0, "Water current map requires a positive worker grid");
    const auto constants = materials::water_current_constants(width, height, settings);
    impl_ = std::make_unique<Impl>(std::move(context), constants, conditional_clear);
}
GlWaterCurrentMap::~GlWaterCurrentMap() = default;
const GlTexture &GlWaterCurrentMap::texture() const { return impl_->target; }
const GlTexture &GlWaterCurrentMap::source_texture() const { return impl_->source; }
const materials::WaterCurrentConstants &GlWaterCurrentMap::constants() const { return impl_->constants; }
} // namespace civ5::graphics
