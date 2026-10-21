#pragma once
#include "civ5/materials/material_mesh.hpp"
#include "gl_context.hpp"
#include <array>

namespace civ5::graphics {
// Constants recovered from the retail 05D98A10 decal vertex shader. The
// matrix is stored as four D3D9 constant rows (c0..c3), which keeps the
// upload independent of GL's column-major default.
struct GlMaterialDecalFrame {
    std::array<float, 16> transform{};
    std::array<float, 4> texture_scales{};
    std::array<float, 4> bounds{};
    float z_distance = 0.f;
};

// Isolated translation of the retail decal/cache layer. It owns only the
// shader, geometry and sampler state. The caller owns the framebuffer,
// attachments, viewport and target lifetime. This class is deliberately not
// admitted to GlMaterialCacheRenderer until an original same-run caller
// proves the decal constants and target/lease lifetime.
class GlMaterialDecalRenderer {
  public:
    GlMaterialDecalRenderer(std::shared_ptr<GlContext> context,
                            const materials::MaterialPatchMesh &mesh,
                            bool write_specular);
    ~GlMaterialDecalRenderer();
    GlMaterialDecalRenderer(const GlMaterialDecalRenderer &) = delete;
    GlMaterialDecalRenderer &operator=(const GlMaterialDecalRenderer &) = delete;

    // Recreate the owned shader, geometry and sampler objects on a different
    // live context. The mesh and specular-output profile are retained; caller
    // owned input textures and framebuffer attachments must be recreated.
    // The old context must remain usable for resource release on this thread.
    void rebuild_context(std::shared_ptr<GlContext> context);

    // Attachment 0 receives decal albedo, attachment 1 receives the signed
    // height/normal payload from the retail cmp, and optional attachment 2
    // receives decal specular. Raster and texture targets are validated here;
    // framebuffer completeness remains the caller's responsibility.
    void draw(const GlMaterialDecalFrame &frame, GLuint albedo, GLuint height,
              GLuint specular = 0, std::array<std::int32_t, 2> target_size = {0, 0});

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
