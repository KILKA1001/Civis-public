#pragma once
#include "civ5/materials/material_cache_loading.hpp"
#include "civ5/materials/material_world.hpp"
#include "civ5/materials/terrain_frame.hpp"
#include "gl_color_keys.hpp"
#include "gl_land_material.hpp"
#include "gl_land_patch_resources.hpp"
#include "gl_material_cache.hpp"
#include "gl_material_cache_renderer.hpp"

namespace civ5::graphics {
// Source-world constants, style masks and height-window selection come from
// MaterialWorld. from_terrain computes the original cache lighting from the
// source terrain lighting state. When the renderer owns
// GlColorKeys, color_correct must be zero and comes from that resource owner.
struct GlMaterialWorldLighting {
    std::array<float, 3> sun_direction, half_vector;
    std::array<GLuint, 5> color_correct;
    static GlMaterialWorldLighting from_terrain(const materials::TerrainLighting &lighting,
                                                const terrain::ReciprocalSqrtEstimate &estimate,
                                                std::array<GLuint, 5> color_correct = {});
};
struct GlMaterialWorldPrepared {
    materials::WorldMaterialPatch patch;
    materials::MaterialCachePassPlan plan;
    GlMaterialCacheFrame frame;
    std::uint32_t height_table_slot;
};
// Quality-owned temporary/common-cache dimensions are independent of each
// patch's final resolution. Original high-detail DX9 quality uses 512/256.
struct GlMaterialWorldCacheExtents {
    std::uint32_t temporary_size, cache_size;
};
struct GlMaterialWorldScheduleResult {
    bool selection_finished = false;
    std::size_t prepared = 0, completed = 0, generated = 0;
};
struct GlMaterialWorldLoadResult {
    bool finished = false;
    std::size_t prepared = 0, completed = 0, generated = 0;
};
// One immutable world owns the source identity of material caches and LAND.
// The renderer retains that world, uploads its actual A8 windows, reuses
// temporary cache storage and retains one final texture per prepared patch.
// Normal estimates, quality, lighting/LUT and scene-wide LAND inputs are
// explicit.
class GlMaterialWorldRenderer {
  public:
    GlMaterialWorldRenderer(std::shared_ptr<GlContext> context,
                            std::shared_ptr<const materials::MaterialWorld> world,
                            const materials::LightingCubeAssets &cubes, MaterialCacheMipPolicy mip_policy,
                            LightingCubeSampling sampling = LightingCubeSampling::ordinary,
                            LandShadowArithmetic arithmetic = LandShadowArithmetic::instruction_order,
                            LandMipSampling mip_sampling = LandMipSampling::ordinary,
                            std::shared_ptr<const GlColorKeys> color_keys = {});
    ~GlMaterialWorldRenderer();
    GlMaterialWorldRenderer(const GlMaterialWorldRenderer &) = delete;
    GlMaterialWorldRenderer &operator=(const GlMaterialWorldRenderer &) = delete;
    // Attach the original terrain-owner decal provider before generating LAND
    // resources or enabling scheduling. Command availability is the explicit
    // caller-owned 8E3570 result for the color/shadow layers.
    void enable_decals(std::shared_ptr<const terrain::TerrainDecalAssets> assets,
                       std::array<std::uint32_t, 2> available_commands);
    // Initial population is allowed before explicit prepare/scheduling.
    // Later mutations require an idle retained scheduler and refresh affected
    // regions through original 684240 without changing source_refresh_stamp.
    // revision and score_seed are used by that retained refresh; the revision
    // must advance beyond the affected patches' last generated frame stamp.
    std::uint32_t submit_decal(const terrain::TerrainDecalRecord &material,
                               const terrain::TerrainDecalGeometry &geometry, std::uint8_t mode,
                               std::uint32_t revision, const materials::MaterialNormalSeed &score_seed);
    void remove_decal(std::uint32_t handle, std::uint32_t revision,
                      const materials::MaterialNormalSeed &score_seed);
    const materials::TerrainDecalStore &decal_store() const;
    // Ordinary diffuse cache plan, including the optional owned decal provider.
    // This overload uses the patch's final resolution for both temporary/common-cache extents.
    // Publish/replacement happens only after the entire GPU job succeeds.
    // Returned CPU data are the actual producers, optional for the caller to
    // keep.
    GlMaterialWorldPrepared prepare(std::size_t index, const materials::MaterialPatchSettings &settings,
                                    const materials::MaterialNormalSeed &score_seed,
                                    const materials::MaterialNormalSeed &vertex_seed,
                                    const GlMaterialWorldLighting &lighting, std::uint32_t shadow_samples,
                                    bool conditional_clear);
    // Explicit quality extents select the thirteen shared target dimensions.
    // settings.cache_size still selects this patch's final texture and geometry.
    GlMaterialWorldPrepared prepare(std::size_t index, const materials::MaterialPatchSettings &settings,
                                    GlMaterialWorldCacheExtents extents,
                                    const materials::MaterialNormalSeed &score_seed,
                                    const materials::MaterialNormalSeed &vertex_seed,
                                    const GlMaterialWorldLighting &lighting, std::uint32_t shadow_samples,
                                    bool conditional_clear);
    // Start a retained ordinary diffuse scheduler with actual RGB565 leases.
    // Call on a renderer without generated patches. Counts are 64/128/256/512;
    // the base size is 64. Owner/state addresses remain stable across frames.
    void enable_scheduling(std::span<const std::uint32_t, 4> counts,
                           const materials::MaterialPatchSettings &settings,
                           GlMaterialWorldCacheExtents extents);
    void set_scheduled_patch(std::size_t index, bool enabled, std::uint32_t blocked = 0,
                             std::uint32_t priority = 0, std::uint32_t source_revision = 1);
    const materials::MaterialCacheSchedulerPatch &scheduled_patch(std::size_t index) const;
    const materials::MaterialCacheSchedulerState &scheduler_state() const;
    // Original source-region marking and full 7805F0 CPU refresh jobs, before
    // the ordinary scheduler regenerates textures. Uses the immutable world's
    // current fields. Each rectangle keeps its own job barrier and duplicates.
    std::size_t refresh_material_regions(std::span<const terrain::TerrainMarshApplication> regions,
                                        std::uint32_t revision,
                                        const materials::MaterialNormalSeed &score_seed);
    std::uint32_t source_refresh_stamp() const;
    const std::optional<materials::MaterialCacheHeightMetrics> &patch_height_metrics(std::size_t index) const;
    std::size_t free_cache_count(std::uint32_t width) const;
    // Begin original initial loading on a fresh retained scheduler. Cull and
    // BlockOnLoad remain explicit caller settings. Foreground loads serialize
    // at most four real material jobs per step; background membership survives
    // into ordinary frames. Normal scheduling is rejected until finished.
    void begin_loading(std::span<const materials::MaterialCacheSchedulerView> views,
                       bool block_on_load, bool cull);
    const materials::MaterialCacheLoadState &loading_state() const;
    GlMaterialWorldLoadResult
    load_step(std::uint32_t frame_stamp, const materials::MaterialNormalSeed &score_seed,
              const materials::MaterialNormalSeed &vertex_seed, const GlMaterialWorldLighting &lighting,
              std::uint32_t shadow_samples, bool conditional_clear,
              const std::function<void(std::size_t, const GlMaterialWorldPrepared &)> &generated = {});
    // Up to four views, shared LAND geometry, ordinary diffuse and base64 are
    // the connected world path. Repeated patch membership is retained as one
    // independent worker batch per original pending entry; every view is kept.
    // Selection -> preparation -> all workers -> pending
    // completion runs on the render thread. No asynchronous OS queue is implied.
    // On a producer/GPU exception, generated leases are released and all stable
    // patch owners return to state0 for a retry. Source resources survive.
    GlMaterialWorldScheduleResult
    schedule_frame(std::span<const materials::MaterialCacheSchedulerView> views,
                   const materials::MaterialCacheSchedulerSettings &settings, std::uint32_t frame_stamp,
                   const materials::MaterialNormalSeed &score_seed,
                   const materials::MaterialNormalSeed &vertex_seed, const GlMaterialWorldLighting &lighting,
                   std::uint32_t shadow_samples, bool conditional_clear,
                   const std::function<void(std::size_t, const GlMaterialWorldPrepared &)> &generated = {});
    // Use the published camera shared with water/FOW. Its projection replaces
    // settings.projection; all budgets, culling and arithmetic controls remain
    // caller-owned. Wrapped views and repeated membership are never dropped or
    // deduplicated.
    GlMaterialWorldScheduleResult
    schedule_camera_frame(const terrain::CameraFrameState &camera,
                          const materials::MaterialCacheSchedulerSettings &settings, std::uint32_t frame_stamp,
                          const materials::MaterialNormalSeed &score_seed,
                          const materials::MaterialNormalSeed &vertex_seed, const GlMaterialWorldLighting &lighting,
                          std::uint32_t shadow_samples, bool conditional_clear,
                          const std::function<void(std::size_t, const GlMaterialWorldPrepared &)> &generated = {});
    const GlTexture &higher_texture(std::size_t index) const;
    // Release generated caches, shared targets and LAND draw resources on a
    // live context. Source world/assets, height windows, ColorKeys and the
    // optional decal provider (including GPU batches) survive.
    // Use prepare() again or enable_scheduling() with a fresh pool before
    // drawing. To replace the live context, use rebuild_context().
    void invalidate_generated_resources();
    // Rebuild source GPU owners on a different live context. This does not
    // recover an unusable/lost WGL context: old owners must still be releasable
    // on their creating context/thread. The caller supplies the original
    // lighting/ColorKey assets. Any external GlColorKeys shared owners must
    // be released first so their old textures are destroyed on the old
    // context before it is discarded. Scheduling/generated state is reset;
    // external scene textures/targets must also be recreated by the caller.
    // Reentrant calls from material callbacks are rejected. Construction
    // failures leave the previous renderer and its published patches intact.
    // Decal source handles, uploaded prefixes and pending edits survive;
    // replacement textures are resolved lazily on the new context.
    void rebuild_context(std::shared_ptr<GlContext> context,
                         const materials::LightingCubeAssets &cubes,
                         const materials::ColorKeyAssets *color_key_assets = nullptr);
    bool prepared(std::size_t index) const;
    const GlTexture &final_texture(std::size_t index) const;
    const GlTexture &height_texture(std::size_t index) const;
    const GlTexture &temporary_texture(std::uint32_t size, materials::MaterialCacheRole role) const;
    const GlTexture &temporary_texture(GlMaterialWorldCacheExtents extents,
                                       materials::MaterialCacheRole role) const;
    const materials::MaterialCachePassSource &source_bindings() const;
    const GlLandGeometryPool &land_geometry_pool() const;
    const materials::MaterialPacketPool &land_packet_pool() const;
    const materials::MaterialCachePacket &land_packet(std::size_t index, unsigned slot = 0) const;
    const materials::MaterialCacheLifecycleState &patch_state(std::size_t index) const;
    // One call after a completed whole-world frame, never once per patch.
    // Advances the original four-slot retirement ring and collects retired IDs.
    void finish_frame();
    // Replaces only world/offset/height-scale vectors with this owner's data.
    materials::LandMaterialConstants land_constants(std::size_t index,
                                                    materials::LandMaterialConstants frame) const;
    // Original LAND slots6/7 for this patch's source style, also usable by a
    // retained cache replay sharing this world's source identity.
    GlLandMaterialInputs color_key_inputs(std::size_t index, GlLandMaterialInputs scene) const;
    // Actual original local-grid diffuse selector1, step1. Binds this patch's
    // selected final/higher cache to slot0 and same-world A8 to displacement
    // unit10. Scheduled states0/1/4 are skipped, as in original LAND selection.
    // Other scene textures, camera and framebuffer/raster state remain explicit.
    void draw(std::size_t index, const materials::LandMaterialConstants &frame, GlLandMaterialInputs scene,
              int width, int height);
    std::vector<float> capture(std::size_t index, const materials::LandMaterialConstants &frame,
                               bool indexed);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
