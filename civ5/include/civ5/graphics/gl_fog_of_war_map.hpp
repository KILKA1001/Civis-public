#pragma once
#include "gl_fog_of_war.hpp"
#include "gl_texture.hpp"
#include <functional>

namespace civ5::graphics {
// Owns the mask and authored cloud resources. Scene color/depth are supplied
// by the frame owner, preserving depth from the preceding terrain passes.
class GlFogOfWarMap {
  public:
    GlFogOfWarMap(std::shared_ptr<GlContext> context, const materials::FogOfWarAssets &assets);
    ~GlFogOfWarMap();
    GlFogOfWarMap(const GlFogOfWarMap &) = delete;
    GlFogOfWarMap &operator=(const GlFogOfWarMap &) = delete;
    // The optional scene callback runs after mask generation and before
    // visibility-depth/cloud passes. It establishes its own framebuffer and
    // raster state and can sample the completed mask in LAND/water shaders.
    // The complete frame and targets are validated before any draw/callback.
    // Recursive render calls are rejected. A callback exception unbinds this
    // owner's framebuffer and permits a later retry; already drawn pixels are
    // retained. The callback must not destroy or move the owner or its targets.
    void render(const materials::FogOfWarFrame &frame, const GlTexture &color, const GlTexture &depth,
                const std::function<void(const GlTexture &)> &draw_scene = {});
    // Advances before every GPU render attempt and after context recovery.
    // A failed/uncertain write must invalidate any prior frame receipt too.
    std::uint64_t render_revision() const;
    const GlTexture &mask() const;
    const std::vector<GlTexture> &clouds() const;
    // Recreate the mask target, cloud textures, framebuffer and shader owner
    // on a different live context. The visibility frame is caller-owned and
    // must be rendered again after recovery.
    // The old context must remain usable for resource release on this thread.
    void rebuild_context(std::shared_ptr<GlContext> context, const materials::FogOfWarAssets &assets);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
