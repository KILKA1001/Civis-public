#include "gl_fog_of_war_map_impl.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <stdexcept>

namespace civ5::graphics {
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
} // namespace
GlFogOfWarMap::Impl::Impl(std::shared_ptr<GlContext> c, const materials::FogOfWarAssets &assets)
        : context(std::move(c)), gl(*context), state(assets.state()),
          mask(GlTexture::render_target(*context, state.mask_extent, state.mask_extent, 1,
                                        RenderTextureFormat::rgba8)),
          renderer(context) {
        for (const auto &data : assets.clouds())
            clouds.emplace_back(*context, data);
        gl.CreateFramebuffers(1, &framebuffer);
    }
GlFogOfWarMap::Impl::~Impl() {
        try {
            context->api();
            gl.DeleteFramebuffers(1, &framebuffer);
        } catch (...) {
            std::terminate();
        }
    }
void GlFogOfWarMap::Impl::bind(const GlTexture &color, const GlTexture *depth) {
        for (unsigned slot = 0; slot < 16; ++slot)
            context->api().BindTextureUnit(slot, 0);
        gl.NamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0, color.id(), 0);
        gl.NamedFramebufferTexture(framebuffer, GL_DEPTH_STENCIL_ATTACHMENT, depth ? depth->id() : 0, 0);
        const GLenum buffer = GL_COLOR_ATTACHMENT0;
        gl.NamedFramebufferDrawBuffers(framebuffer, 1, &buffer);
        require(gl.CheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                "FOW framebuffer incomplete");
        gl.BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        gl.Viewport(0, 0, color.levels()[0].width, color.levels()[0].height);
    }
GlFogOfWarMap::GlFogOfWarMap(std::shared_ptr<GlContext> context, const materials::FogOfWarAssets &assets) {
    require(bool(context), "FOW map requires an owned context");
    require(assets.clouds().size() == (assets.state().quality ? 3 : 0), "FOW authored layer count");
    impl_ = std::make_unique<Impl>(std::move(context), assets);
}
GlFogOfWarMap::~GlFogOfWarMap() = default;
const std::vector<GlTexture> &GlFogOfWarMap::clouds() const { return impl_->clouds; }
void GlFogOfWarMap::rebuild_context(std::shared_ptr<GlContext> context,
                                    const materials::FogOfWarAssets &assets) {
    require(bool(context), "FOW context recovery needs a live replacement context");
    require(context != impl_->context, "FOW context recovery requires a different OpenGL context");
    require(!impl_->rendering, "FOW context recovery cannot interrupt rendering");
    require(assets.state().quality == impl_->state.quality &&
                assets.state().mask_extent == impl_->state.mask_extent &&
                assets.clouds().size() == impl_->clouds.size(),
            "FOW context recovery must preserve the resource profile");
    require(impl_->render_revision < std::numeric_limits<std::uint64_t>::max(),
            "FOW render revision exhausted");
    auto replacement = std::make_unique<Impl>(std::move(context), assets);
    replacement->render_revision = impl_->render_revision + 1;
    impl_.swap(replacement);
}
void GlFogOfWarMap::render(const materials::FogOfWarFrame &frame, const GlTexture &color,
                           const GlTexture &depth, const std::function<void(const GlTexture &)> &draw_scene) {
    auto &s = *impl_;
    require(!s.rendering, "FOW map cannot render recursively");
    require(color.belongs_to(*s.context) && depth.belongs_to(*s.context) && color.face_count() == 1 &&
                depth.face_count() == 1 && color.depth() == 1 && depth.depth() == 1 &&
                !color.levels().empty() && !depth.levels().empty(),
            "FOW scene targets require owned 2D storage");
    const auto width = color.levels()[0].width, height = color.levels()[0].height;
    require(color.id() != s.mask.id() && depth.plan().internal_format == GL_DEPTH24_STENCIL8 &&
                depth.levels()[0].width == width && depth.levels()[0].height == height &&
                (color.plan().internal_format == GL_RGBA8 || color.plan().internal_format == GL_RGBA16F ||
                 color.plan().internal_format == GL_RGBA32F),
            "FOW scene target format/extent");
    require(frame.passes.size() == 1 || (s.state.quality && frame.passes.size() == 3),
            "FOW frame pass count");
    unsigned end = 0;
    for (unsigned index = 0; index < frame.passes.size(); ++index) {
        const auto &pass = frame.passes[index];
        require(static_cast<unsigned>(pass.kind) == index && pass.begin == end && pass.end >= end &&
                    pass.end <= frame.draws.size(),
                "FOW pass order/extent");
        for (auto v : pass.clear_color)
            require(std::isfinite(v), "Nonfinite FOW clear color");
        for (unsigned at = pass.begin; at < pass.end; ++at) {
            const auto &draw = frame.draws[at];
            require(index == 0   ? draw.geometry < 2 && draw.selector <= 2
                    : index == 1 ? draw.geometry == 5 && draw.selector == 3
                                 : draw.geometry >= 2 && draw.geometry <= 4 && draw.selector == 0,
                    "FOW pass geometry/selector");
            require(draw.constants.size() == (index == 2 ? 197 : 96), "FOW pass constant extent");
            for (const auto &v : draw.constants)
                for (auto x : v)
                    require(std::isfinite(x), "Nonfinite FOW frame constant");
        }
        end = pass.end;
    }
    require(end == frame.draws.size(), "Unreferenced FOW draw");
    require(s.render_revision < std::numeric_limits<std::uint64_t>::max(),
            "FOW render revision exhausted");
    ++s.render_revision;
    s.rendering = true;
    try {
        for (const auto &pass : frame.passes) {
            s.context->api();
            for (auto state : {GL_STENCIL_TEST, GL_SCISSOR_TEST, GL_DITHER, GL_FRAMEBUFFER_SRGB,
                               GL_RASTERIZER_DISCARD, GL_POLYGON_OFFSET_FILL, GL_POLYGON_OFFSET_LINE})
                s.gl.Disable(state);
            GLint clips = 0;
            s.context->api().GetIntegerv(GL_MAX_CLIP_DISTANCES, &clips);
            for (GLint i = 0; i < clips; ++i)
                s.gl.Disable(GL_CLIP_DISTANCE0 + i);
            s.gl.ClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
            s.gl.DepthRange(0, 1);
            s.gl.DepthFunc(GL_LEQUAL);
            s.gl.PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            s.gl.FrontFace(GL_CCW);
            s.gl.CullFace(GL_BACK);
            const bool mask = pass.kind == materials::FogOfWarPassKind::mask;
            const bool cull = pass.kind == materials::FogOfWarPassKind::culling;
            s.bind(mask ? s.mask : color, mask ? nullptr : &depth);
            if (mask) {
                s.gl.Disable(GL_DEPTH_TEST);
                s.gl.Disable(GL_CULL_FACE);
            } else {
                s.gl.Enable(GL_DEPTH_TEST);
                s.gl.Enable(GL_CULL_FACE);
            }
            s.gl.DepthMask(cull ? GL_TRUE : GL_FALSE);
            s.gl.ColorMask(!cull, !cull, !cull, !cull);
            if (cull)
                s.gl.Disable(GL_BLEND);
            else
                s.gl.Enable(GL_BLEND);
            s.gl.BlendEquationSeparate(mask ? GL_MAX : GL_FUNC_ADD, mask ? GL_MAX : GL_FUNC_ADD);
            s.gl.BlendFuncSeparate(mask ? GL_ONE : GL_SRC_ALPHA, mask ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA,
                                   GL_ONE, GL_ONE);
            if (mask)
                s.gl.ClearNamedFramebufferfv(s.framebuffer, GL_COLOR, 0, pass.clear_color.data());
            for (unsigned index = pass.begin; index < pass.end; ++index) {
                const auto &draw = frame.draws[index];
                const bool cloud = pass.kind == materials::FogOfWarPassKind::clouds;
                s.renderer.draw(draw, mask ? s.state.mask_extent : width, mask ? s.state.mask_extent : height,
                                cloud ? s.clouds[draw.geometry - 2].id() : 0, cloud ? s.mask.id() : 0);
            }
            if (mask && draw_scene) {
                s.gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
                draw_scene(s.mask);
            }
        }
        s.context->api();
        s.gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        s.context->check("Original FOW mask/culling/cloud frame");
        s.rendering = false;
    } catch (...) {
        // The callback may have switched contexts before throwing.
        s.context->api();
        s.gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        s.rendering = false;
        throw;
    }
}
} // namespace civ5::graphics
