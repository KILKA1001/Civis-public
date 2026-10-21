#include "civ5/graphics/gl_material_cache_raster.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/materials/material_cache_raster.hpp"
#include <exception>
#include <stdexcept>

namespace civ5::graphics {
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
} // namespace
struct GlMaterialCacheRaster::Impl {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    GLuint framebuffer = 0;
    explicit Impl(std::shared_ptr<GlContext> c) : context(std::move(c)), gl(*context) {
        try {
            gl.CreateFramebuffers(1, &framebuffer);
            context->check("Create owned material cache framebuffer");
        } catch (...) {
            gl.DeleteFramebuffers(1, &framebuffer);
            throw;
        }
    }
    ~Impl() {
        try {
            context->api();
            gl.DeleteFramebuffers(1, &framebuffer);
        } catch (...) {
            std::terminate();
        }
    }
};
GlMaterialCacheRaster::GlMaterialCacheRaster(std::shared_ptr<GlContext> context) {
    require(bool(context), "Cache raster requires owned context");
    impl_ = std::make_unique<Impl>(std::move(context));
}
GlMaterialCacheRaster::~GlMaterialCacheRaster() = default;
void GlMaterialCacheRaster::validate(const materials::MaterialCachePass &pass,
                                     std::span<const GlMaterialCacheTarget> targets) const {
    auto &s = *impl_;
    s.context->api();
    using Kind = materials::MaterialCachePassKind;
    require(pass.kind == Kind::height || pass.kind == Kind::normal || pass.kind == Kind::shadow ||
                pass.kind == Kind::lighting || pass.kind == Kind::decal || pass.kind == Kind::decal_shadow,
            "Cache raster does not execute compression or copy commands");
    require(pass.depth_state == 9 && (pass.blend_state == 0 || pass.blend_state == 4 || pass.blend_state == 8) &&
                (pass.flags >> 28) == 0,
            "Unverified material cache raster profile");
    require(!targets.empty() && targets.size() <= 4, "Cache pass requires 1..4 color attachments");
    std::uint32_t width = 0, height = 0;
    for (std::size_t i = 0; i < 4; ++i) {
        if (i >= targets.size()) {
            require(pass.targets[i] == 0, "Missing cache target mapping");
            continue;
        }
        const auto &target = targets[i];
        require(target.handle != 0 && target.handle == pass.targets[i], "Cache target mapping order");
        require(target.texture && target.texture->belongs_to(*s.context), "Foreign or empty cache texture");
        const auto &levels = target.texture->levels();
        require(target.texture->face_count() == 1 && pass.mip < levels.size(),
                "Invalid cache target mip or texture dimension");
        const auto format = target.texture->plan().internal_format;
        require(format == GL_RGBA16F || format == GL_RGBA8 || format == GL_RGB565,
                "Unverified cache render format");
        for (std::size_t j = 0; j < i; ++j)
            require(targets[j].texture->id() != target.texture->id(), "Duplicate cache attachment");
        if (i == 0) {
            width = levels.front().width;
            height = levels.front().height;
        }
        require(levels.front().width == width && levels.front().height == height,
                "Unequal cache attachment dimensions");
    }
}
void GlMaterialCacheRaster::begin(const materials::MaterialCachePass &pass,
                                  std::span<const GlMaterialCacheTarget> targets, bool conditional_clear) {
    validate(pass, targets);
    auto &s = *impl_;
    const auto &api = s.context->api();
    const auto width = targets.front().texture->levels().front().width;
    const auto height = targets.front().texture->levels().front().height;
    s.context->check("Before material cache pass state");
    for (GLuint i = 0; i < 16; ++i)
        api.BindTextureUnit(i, 0);
    std::array<GLenum, 4> buffers{};
    for (unsigned i = 0; i < 4; ++i) {
        s.gl.NamedFramebufferTexture(s.framebuffer, GL_COLOR_ATTACHMENT0 + i,
                                     i < targets.size() ? targets[i].texture->id() : 0, pass.mip);
        buffers[i] = GL_COLOR_ATTACHMENT0 + i;
    }
    s.gl.NamedFramebufferDrawBuffers(s.framebuffer, static_cast<GLsizei>(targets.size()), buffers.data());
    require(s.gl.CheckNamedFramebufferStatus(s.framebuffer, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
            "Incomplete cache pass framebuffer");
    s.gl.BindFramebuffer(GL_FRAMEBUFFER, s.framebuffer);
    // The original consumer uses descriptor base size, even when a mip is
    // attached. Do not silently substitute that mip's reduced dimensions.
    s.gl.Viewport(0, 0, width, height);
    for (auto state :
         {GL_DEPTH_TEST, GL_STENCIL_TEST, GL_CULL_FACE, GL_SCISSOR_TEST, GL_DITHER, GL_FRAMEBUFFER_SRGB,
          GL_RASTERIZER_DISCARD, GL_POLYGON_OFFSET_FILL, GL_POLYGON_OFFSET_LINE})
        s.gl.Disable(state);
    s.gl.DepthMask(GL_FALSE);
    s.gl.DepthFunc(GL_LEQUAL);
    s.gl.PolygonMode(GL_FRONT_AND_BACK, pass.flags & 16 ? GL_LINE : GL_FILL);
    s.gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    s.gl.BlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    // Original blend8 interpolates RGB by source alpha, but ADDS alpha.
    s.gl.BlendFuncSeparate(pass.blend_state == 8 ? GL_SRC_ALPHA : GL_ONE,
                           pass.blend_state == 8 ? GL_ONE_MINUS_SRC_ALPHA : pass.blend_state == 4 ? GL_ONE : GL_ZERO,
                           GL_ONE, pass.blend_state ? GL_ONE : GL_ZERO);
    if (pass.blend_state)
        s.gl.Enable(GL_BLEND);
    else
        s.gl.Disable(GL_BLEND);
    if (materials::material_cache_clear_target(pass.flags, conditional_clear)) {
        const auto color =
            materials::material_cache_clear_rgba(materials::material_cache_clear_argb(pass.clear_color));
        for (unsigned i = 0; i < targets.size(); ++i)
            s.gl.ClearNamedFramebufferfv(s.framebuffer, GL_COLOR, i, color.data());
    }
    s.context->check("Apply original cache color/blend/depth/viewport state");
}
void GlMaterialCacheRaster::end() {
    auto &s = *impl_;
    s.context->api();
    s.gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    s.context->check("End cache pass");
}
} // namespace civ5::graphics
