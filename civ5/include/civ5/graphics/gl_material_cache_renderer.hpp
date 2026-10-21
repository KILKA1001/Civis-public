#pragma once
#include "gl_material_cache_raster.hpp"
#include "gl_material_lighting.hpp"
#include "gl_terrain_decal.hpp"
#include "civ5/materials/material_mesh.hpp"

namespace civ5::graphics {
// These resident frame inputs are distinct from the plan's local constant arena.
// Source-world height windows and the ColorCorrect producer remain caller-owned.
struct GlMaterialCacheFrame {
    std::array<float, 384> material_coefficients;
    std::array<float, 3> world_scales, sun_direction, half_vector;
    std::array<float, 2> height_texture_scale;
};
struct GlMaterialCacheVolume {
    std::uint16_t handle;
    GLuint texture;
};
struct GlMaterialCacheDecals {
    // Shares the cache context; resources use the provider's original U16 IDs.
    // Resolver calls happen only during preflight. Keep the backend and textures
    // alive and unchanged throughout execute; callbacks must not reenter it.
    GlTerrainDecalRenderer &renderer;
    std::uint32_t shader_binding;
    TerrainDecalTextureResolver resolve;
};
// Executes ordinary, uncompressed, diffuse plans including optional decal layers.
// Owns patch renderers, geometry and framebuffer; textures remain with their
// caller. Resource handles are logical IDs, including the explicit resource0.
// The current geometry transport is one complete patch (start/base both zero).
class GlMaterialCacheRenderer {
  public:
    GlMaterialCacheRenderer(std::shared_ptr<GlContext> context, const materials::MaterialPatchMesh &material,
                            const materials::LightingPatchMesh &lighting,
                            const materials::MaterialCachePassSource &bindings,
                            LightingCubeSampling sampling = LightingCubeSampling::ordinary);
    ~GlMaterialCacheRenderer();
    GlMaterialCacheRenderer(const GlMaterialCacheRenderer &) = delete;
    GlMaterialCacheRenderer &operator=(const GlMaterialCacheRenderer &) = delete;
    // Recreate the owned cache shaders, geometry and framebuffer helpers on a
    // different live context. Caller-owned cache textures and volumes must be
    // recreated separately for the replacement context.
    void rebuild_context(std::shared_ptr<GlContext> context);
    // Preflights the whole supported plan before target writes. A device error
    // during execution can leave partial output; callers must discard that job.
    // Every target's effective mip chain is generated after its producer pass.
    // Only texture slots consumed by the original selected shaders are fetched;
    // packet fields for unused slots remain preserved in the CPU plan.
    void execute(const materials::MaterialCachePassPlan &plan,
                 std::span<const GlMaterialCacheTarget> resources,
                 std::span<const GlMaterialCacheVolume> volumes, const GlMaterialCacheFrame &frame,
                 bool conditional_clear, const GlMaterialCacheDecals *decals = nullptr);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
