#pragma once
#include "civ5/graphics/gl_render_api.hpp"

namespace civ5::graphics::detail {
// LAND, water and FOW keep the original D3D clip Z. Remapping it through 2*z-w
// introduces a rounding step before perspective division and D24 storage.
// Their shaders already flip Y to retain the existing texture/readback origin.
class NativeClipScope {
    const GlRenderApi &gl_;
    GLint origin_ = GL_LOWER_LEFT, depth_ = GL_NEGATIVE_ONE_TO_ONE;
    GLfloat viewport_[4]{};

  public:
    NativeClipScope(const GlContext &context, const GlRenderApi &gl) : gl_(gl) {
        context.api().GetIntegerv(GL_CLIP_ORIGIN, &origin_);
        context.api().GetIntegerv(GL_CLIP_DEPTH_MODE, &depth_);
        gl_.GetFloati_v(GL_VIEWPORT, 0, viewport_);
        gl_.ClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
        // D3D9 samples at integer pixel centers. Apply its half-pixel offset
        // during viewport mapping, avoiding a rounded clip-space W product.
        gl_.ViewportIndexedf(0, viewport_[0] + .5f, viewport_[1] + .5f, viewport_[2], viewport_[3]);
    }
    ~NativeClipScope() {
        gl_.ViewportIndexedf(0, viewport_[0], viewport_[1], viewport_[2], viewport_[3]);
        gl_.ClipControl(static_cast<GLenum>(origin_), static_cast<GLenum>(depth_));
    }
    NativeClipScope(const NativeClipScope &) = delete;
    NativeClipScope &operator=(const NativeClipScope &) = delete;
};
} // namespace civ5::graphics::detail
