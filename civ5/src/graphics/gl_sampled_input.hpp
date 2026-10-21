#pragma once
#include "civ5/graphics/gl_render_api.hpp"
#include <stdexcept>
#include <string>

namespace civ5::graphics::detail {
// Shared 2D/3D/cube input guard; no texture-view storage identity is
// inferred from object names. Caller must exclude those aliases explicitly.
inline void validate_sampled_texture(const GlContext &context, const GlRenderApi &gl, GLuint texture,
                                     GLenum target, bool current_framebuffer = true) {
    const auto require = [](bool ok, const char *why) {
        if (!ok)
            throw std::invalid_argument(why);
    };
    const auto &api = context.api();
    require(target == GL_TEXTURE_2D || target == GL_TEXTURE_3D || target == GL_TEXTURE_CUBE_MAP,
            "Unsupported sampled source target");
    require(texture != 0 && api.IsTexture(texture), "Sampled source must be a real texture");
    context.check("Before sampled source validation");
    GLint previous = 0;
    api.GetIntegerv(target == GL_TEXTURE_2D   ? GL_TEXTURE_BINDING_2D
                    : target == GL_TEXTURE_3D ? GL_TEXTURE_BINDING_3D
                                              : GL_TEXTURE_BINDING_CUBE_MAP,
                    &previous);
    api.BindTexture(target, texture);
    const auto error = api.GetError();
    api.BindTexture(target, static_cast<GLuint>(previous));
    context.check("Restore sampled source-validation binding");
    if (error != GL_NO_ERROR) {
        const auto target_name = target == GL_TEXTURE_2D ? "2D" :
                                 target == GL_TEXTURE_3D ? "3D" : "cube";
        throw std::invalid_argument(std::string("Sampled source target mismatch target=") + target_name +
                                    " texture=" + std::to_string(texture));
    }
    GLint base = 0, width = 0, height = 0;
    api.GetTextureParameteriv(texture, GL_TEXTURE_BASE_LEVEL, &base);
    api.GetTextureLevelParameteriv(texture, base, GL_TEXTURE_WIDTH, &width);
    api.GetTextureLevelParameteriv(texture, base, GL_TEXTURE_HEIGHT, &height);
    GLint depth = 1;
    if (target == GL_TEXTURE_3D)
        api.GetTextureLevelParameteriv(texture, base, GL_TEXTURE_DEPTH, &depth);
    context.check("Query sampled base image");
    require(width > 0 && height > 0 && depth > 0, "Sampled source has no base image");
    if (!current_framebuffer)
        return; // A future pass validates its explicit attachments separately.
    GLint fbo = 0, attachments = 0;
    api.GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fbo);
    api.GetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &attachments);
    if (fbo != 0) {
        const auto no_alias = [&](GLenum attachment) {
            GLint type = 0, name = 0;
            gl.GetNamedFramebufferAttachmentParameteriv(fbo, attachment,
                                                        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
            if (type == GL_TEXTURE) {
                gl.GetNamedFramebufferAttachmentParameteriv(fbo, attachment,
                                                            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);
                require(static_cast<GLuint>(name) != texture, "Sampled framebuffer feedback rejected");
            }
        };
        for (int i = 0; i < attachments; ++i)
            no_alias(GL_COLOR_ATTACHMENT0 + i);
        no_alias(GL_DEPTH_ATTACHMENT);
        no_alias(GL_STENCIL_ATTACHMENT);
    }
    context.check("Sampled texture input validated");
}
inline void validate_sampled_2d(const GlContext &context, const GlRenderApi &gl, GLuint texture,
                               bool current_framebuffer = true) {
    validate_sampled_texture(context, gl, texture, GL_TEXTURE_2D, current_framebuffer);
}
} // namespace civ5::graphics::detail
