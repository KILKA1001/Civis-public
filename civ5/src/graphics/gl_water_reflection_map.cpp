#include "civ5/graphics/gl_water_reflection_map.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include <exception>
#include <stdexcept>

namespace civ5::graphics {
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
GlTexture make_color(const GlContext &context, const materials::WaterReflectionResource &plan) {
    if (plan.render_target)
        return GlTexture::render_target(context, plan.width, plan.height, 1, RenderTextureFormat::rgba8);
    const auto fallback = materials::water_reflection_fallback();
    return GlTexture::rgba8(context, std::span(&fallback, 1));
}
} // namespace
struct GlWaterReflectionMap::Impl {
    std::shared_ptr<GlContext> context;
    std::shared_ptr<const GlTexture> depth;
    GlRenderApi gl;
    materials::WaterReflectionResource plan;
    GlTexture color;
    GLuint framebuffer = 0;
    bool ready = false, rendering = false;
    Impl(std::shared_ptr<GlContext> c, materials::WaterReflectionResource p,
         std::shared_ptr<const GlTexture> d)
        : context(std::move(c)), depth(std::move(d)), gl(*context), plan(p),
          color(make_color(*context, plan)), ready(!plan.render_target) {
        if (!plan.render_target)
            return;
        try {
            gl.CreateFramebuffers(1, &framebuffer);
            gl.NamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0, color.id(), 0);
            gl.NamedFramebufferTexture(framebuffer, GL_DEPTH_STENCIL_ATTACHMENT, depth->id(), 0);
            const GLenum buffer = GL_COLOR_ATTACHMENT0;
            gl.NamedFramebufferDrawBuffers(framebuffer, 1, &buffer);
            require(gl.CheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                    "Incomplete reflection framebuffer");
            context->check("Create owned reflection target");
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
GlWaterReflectionMap::GlWaterReflectionMap(std::shared_ptr<GlContext> context, std::uint32_t quality,
    std::uint32_t reflections, std::shared_ptr<const GlTexture> shared_depth) {
    require(bool(context), "Reflection map requires an owned context");
    const auto plan = materials::water_reflection_resource(quality, reflections);
    if (plan.render_target) {
        require(shared_depth && shared_depth->belongs_to(*context), "Reflection requires same-context shared depth");
        require(shared_depth->plan().internal_format == GL_DEPTH24_STENCIL8 &&
                shared_depth->face_count() == 1 && shared_depth->levels().size() == 1,
                "Reflection shared depth requires one-level D24S8 storage");
        const auto &level = shared_depth->levels()[0];
        require(level.width >= plan.width && level.height >= plan.height,
                "Shared depth must cover the entire reflection target");
    } else {
        shared_depth.reset();
    }
    impl_ = std::make_unique<Impl>(std::move(context), plan, std::move(shared_depth));
}
GlWaterReflectionMap::~GlWaterReflectionMap() = default;
const materials::WaterReflectionResource &GlWaterReflectionMap::resource() const { return impl_->plan; }
const GlTexture &GlWaterReflectionMap::texture() const {
    require(impl_->ready && !impl_->rendering, "Reflection pixels have not completed production");
    return impl_->color;
}
void GlWaterReflectionMap::render(const std::function<void()> &draws) {
    auto &s = *impl_;
    require(s.plan.render_target, "Fallback reflection texture is not a render target");
    require(!s.rendering, "Reflection pass is already rendering");
    s.context->api();
    s.ready = false;
    s.rendering = true;
    try {
        s.gl.BindFramebuffer(GL_FRAMEBUFFER, s.framebuffer);
        for (unsigned slot = 0; slot < 16; ++slot)
            s.context->api().BindTextureUnit(slot, 0);
        // D3D9 SetRenderTarget resets the viewport to the target dimensions.
        s.gl.Viewport(0, 0, s.plan.width, s.plan.height);
        s.gl.DepthRange(0, 1);
        for (auto state : {GL_STENCIL_TEST, GL_POLYGON_OFFSET_FILL, GL_POLYGON_OFFSET_LINE,
                           GL_SCISSOR_TEST, GL_DITHER, GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD})
            s.gl.Disable(state);
        GLint clip_planes = 0;
        s.context->api().GetIntegerv(GL_MAX_CLIP_DISTANCES, &clip_planes);
        for (GLint i = 0; i < clip_planes; ++i)
            s.gl.Disable(GL_CLIP_DISTANCE0 + i);
        // D3D9 Clear ignores write masks. GL clear obeys them, so establish
        // full color/depth writes first. Stencil is deliberately retained.
        s.gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        s.gl.DepthMask(GL_TRUE);
        const float black[]{0, 0, 0, 0}, far_depth = 1.f;
        s.gl.ClearNamedFramebufferfv(s.framebuffer, GL_COLOR, 0, black);
        s.gl.ClearNamedFramebufferfv(s.framebuffer, GL_DEPTH, 0, &far_depth);
        s.gl.Enable(GL_DEPTH_TEST);
        s.gl.DepthFunc(GL_LEQUAL);
        s.gl.Enable(GL_BLEND);
        s.gl.BlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        s.gl.BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
        s.gl.Enable(GL_CULL_FACE);
        // With the native-to-GL clip-Y conversion, original D3DCULL_CCW
        // corresponds to back-face culling with GL's CCW front convention.
        s.gl.FrontFace(GL_CCW);
        s.gl.CullFace(GL_BACK);
        s.gl.PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        if (draws)
            draws();
        s.context->api();
        s.gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        s.context->check("Original reflection clear and submitted draws");
        s.ready = true;
        s.rendering = false;
    } catch (...) {
        s.context->api();
        s.gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        s.rendering = false;
        throw;
    }
}
} // namespace civ5::graphics
