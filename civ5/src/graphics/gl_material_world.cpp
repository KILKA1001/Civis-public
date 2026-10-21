#include "civ5/graphics/gl_material_world.hpp"
#include "civ5/graphics/gl_lighting_cubes.hpp"
#include "civ5/graphics/gl_material_textures.hpp"
#include "civ5/graphics/gl_material_cache_pool.hpp"
#include "civ5/materials/material_cache_jobs.hpp"
#include "civ5/materials/lighting_mesh.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <map>
#include <numeric>
#include <optional>
#include <stdexcept>

namespace civ5::graphics {
namespace {
using namespace materials;
void require(bool value, const char *why) {
    if (!value)
        throw std::invalid_argument(why);
}
unsigned levels(unsigned size, bool automatic) {
    unsigned count = 1;
    if (automatic)
        for (; size > 1; size >>= 1)
            ++count;
    return count;
}
struct PreparationScope {
    bool &active;
    explicit PreparationScope(bool &value) : active(value) {
        require(!active, "Material preparation cannot reenter its renderer");
        active = true;
    }
    ~PreparationScope() { active = false; }
};
} // namespace
struct GlMaterialWorldRenderer::Impl : MaterialCacheSchedulerBackend, MaterialCachePreparationBackend {
    std::shared_ptr<GlContext> context;
    std::shared_ptr<const MaterialWorld> world;
    std::shared_ptr<const MaterialAssets> assets;
    GlMaterialTextures material_textures;
    GlLightingCubes cubes;
    std::shared_ptr<const GlColorKeys> color_keys;
    MaterialCacheMipPolicy mip_policy;
    LightingCubeSampling sampling;
    LandShadowArithmetic arithmetic;
    LandMipSampling mip_sampling;
    MaterialCachePassSource source{};
    std::uint32_t height_id, final_id;
    std::array<std::uint32_t, 64> flags{};
    std::vector<float> scales;
    std::vector<std::uint32_t> diffuse, height;
    std::map<int, GlTexture> heights;
    struct PatchDraw {
        GlTexture final;
        std::unique_ptr<GlLandPatchResources> resources;
    };
    std::map<std::size_t, PatchDraw> finals;
    std::map<std::pair<unsigned, unsigned>, std::unique_ptr<GlMaterialCacheTextures>> temporary;
    std::shared_ptr<GlLandGeometryPool> land_geometry;
    std::shared_ptr<MaterialPacketPool> land_packets;
    std::map<std::size_t, std::unique_ptr<GlLandPatchResources>> capture_resources;
    std::map<std::pair<int, int>, std::unique_ptr<GlLandMaterialRenderer>> land_renderers;
    LandPacketTextures land_sources{};
    std::map<int, std::uint32_t> land_heights;
    std::uint32_t land_final_first = 0;
    struct ScheduledPatch {
        std::unique_ptr<GlLandPatchResources> resources;
        std::optional<MaterialScoreResult> scores;
        std::optional<MaterialCacheHeightMetrics> height_metrics;
        std::vector<std::unique_ptr<MaterialCacheJobBatch>> batches;
        std::size_t next_execution = 0, next_completion = 0;
    };
    struct CachePool final : MaterialCacheTexturePool {
        GlMaterialCachePool pool;
        std::uint32_t offset;
        CachePool(std::shared_ptr<GlContext> context, std::span<const std::uint32_t, 4> counts,
                  bool mips, std::uint32_t first)
            : pool(std::move(context), counts, mips), offset(first - 1) {}
        std::uint32_t take(std::uint32_t size, std::uint32_t &actual) override {
            const auto handle = pool.take(size, actual);
            return handle ? offset + handle : 0;
        }
        void give(std::uint32_t handle) override {
            require(!handle || handle > offset, "Foreign scheduled cache handle");
            pool.give(handle ? handle - offset : 0);
        }
        const GlTexture &texture(std::uint32_t handle) const {
            require(handle > offset, "Invalid scheduled cache handle");
            return pool.texture(handle - offset);
        }
    };
    std::unique_ptr<CachePool> cache_pool;
    std::vector<ScheduledPatch> scheduled;
    std::vector<MaterialCacheSchedulerPatch> scheduler_patches;
    std::vector<std::uint32_t> all_patches;
    MaterialCacheSchedulerState scheduler;
    std::optional<MaterialCacheLoadState> loading;
    MaterialPatchSettings schedule_settings{};
    GlMaterialWorldCacheExtents schedule_extents{};
    const MaterialNormalSeed *score_seed = nullptr;
    bool preparing = false;
    std::uint32_t preparing_patch = 0, draw_stamp = 0;
    std::uint32_t source_refresh_stamp = 0;

    struct Decals {
        std::shared_ptr<const terrain::TerrainDecalAssets> assets;
        std::unique_ptr<TerrainDecalStore> store;
        GlTerrainDecalRenderer renderer;
        GlTerrainDecalTextures textures;
        std::array<std::uint32_t, 2> available;
        static constexpr std::uint32_t shader_binding = 5;
        Decals(std::shared_ptr<GlContext> context,
               std::shared_ptr<const terrain::TerrainDecalAssets> source,
               const terrain::HeightOwnerParameters &p, std::array<std::uint32_t, 2> commands)
            : assets(std::move(source)), renderer(context), textures(context, assets), available(commands) {
            const auto domain = terrain_decal_store_domain(p.world_span_x, p.world_span_y, p.scale_x, p.scale_y);
            store = std::make_unique<TerrainDecalStore>(domain.bounds, domain.columns, domain.rows,
                                                       assets->blank_specular());
        }
    };
    std::unique_ptr<Decals> decals;
    void upload_decals() { if (decals) decals->store->upload_pending(decals->renderer); }
    void check_refresh(const MaterialNormalSeed &seed) const;
    std::size_t refresh_region(terrain::Point2 lower, terrain::Point2 upper,
                               std::uint32_t revision, const MaterialNormalSeed &seed, bool stamp_owner);
    TerrainDecalRegionRefresh decal_refresh(std::uint32_t revision, const MaterialNormalSeed &seed);

    GlMaterialWorldPrepared generate(std::size_t index, WorldMaterialPatch computed,
                                     const MaterialPatchSettings &settings, GlMaterialWorldCacheExtents extents,
                                     const GlMaterialWorldLighting &lighting, std::uint32_t shadow_samples,
                                     bool conditional_clear, const GlTexture &final, const GlTexture *higher);
    void execute_patch(std::uint32_t index, const MaterialNormalSeed &vertex_seed) {
        auto &p = scheduled.at(index);
        require(p.next_execution < p.batches.size(), "Material worker lost its retained patch batch");
        if (p.batches[p.next_execution])
            world->execute_patch_jobs(index, schedule_settings, p.scores->selected, vertex_seed,
                                      *p.batches[p.next_execution]);
        ++p.next_execution;
    }
    void complete_patch(std::uint32_t index, std::uint32_t frame_stamp, std::uintptr_t argument,
                        const GlMaterialWorldLighting &lighting, std::uint32_t shadow_samples,
                        bool conditional_clear, GlMaterialWorldScheduleResult &result,
                        const std::function<void(std::size_t, const GlMaterialWorldPrepared &)> &generated);
    std::size_t free_count(std::uint32_t width) const override {
        return cache_pool->pool.state().free_handles(width).size();
    }
    std::size_t compressed_base_free_count() const override {
        throw std::logic_error("Compressed world scheduling is not connected");
    }
    bool acquire_base(std::uint32_t index) override {
        return material_cache_acquire_base(scheduled.at(index).resources->state().textures, *cache_pool, 64);
    }
    bool acquire_higher(std::uint32_t index) override {
        return material_cache_acquire_higher(scheduled.at(index).resources->state().textures, *cache_pool, 64);
    }
    void downgrade(std::uint32_t index) override { scheduled.at(index).resources->downgrade_cache(*cache_pool); }
    void invalidate_geometry(std::uint32_t index) override { scheduled.at(index).resources->invalidate(); }
    void release_cache(std::uint32_t index, bool geometry, bool final, bool higher) override {
        scheduled.at(index).resources->release_cache(*cache_pool, geometry, final, higher);
    }
    void refresh_selection(std::uint32_t index) override {
        auto &patch = scheduled.at(index);
        auto settings = schedule_settings;
        settings.parent_flag = scheduler_patches[index].material_parent_flag;
        patch.scores = world->select_patch(index, settings, *score_seed);
        patch.resources->state().textures.specular = patch.scores->selected.specular() != 0;
    }
    void prepare(std::uint32_t index, std::uintptr_t dispatch) override {
        auto &p = scheduled.at(index);
        const auto &placement = world->cache().patches[index];
        preparing_patch = index;
        // Keep one queue entry for every original pending occurrence. Some
        // lifecycle states prepare geometry only and therefore do not invoke
        // prepare_material; their null entry must still cross the worker and
        // completion barriers in order.
        p.batches.push_back(nullptr);
        material_cache_prepare(p.resources->state(),
                               {placement.interior_columns, placement.interior_rows, 1}, *this, true, dispatch);
    }
    std::uintptr_t allocate_staging(std::size_t) override {
        throw std::logic_error("World scheduler uses shared LAND geometry");
    }
    void free_staging(std::uintptr_t) override {
        throw std::logic_error("World staging belongs to its retained material batch");
    }
    void submit_geometry(MaterialCacheGeometryJob &, std::uintptr_t) override {
        throw std::logic_error("World scheduler uses shared LAND geometry");
    }
    void rebuild_geometry(MaterialCacheLifecycleState &) override { unexpected_lifecycle(); }
    void generate_cache(MaterialCacheLifecycleState &, std::uint32_t, std::uintptr_t) override {
        unexpected_lifecycle();
    }
    void release_geometry_pair(std::uint32_t, std::uint32_t) override { unexpected_lifecycle(); }
    void release_vertex_buffer(std::uint32_t) override { unexpected_lifecycle(); }
    void release_index_buffer(std::uint32_t) override { unexpected_lifecycle(); }
    void release_packet(std::uint32_t) override { unexpected_lifecycle(); }
    [[noreturn]] static void unexpected_lifecycle() {
        throw std::logic_error("World lifecycle callbacks belong to the stable LAND resource owner");
    }
    void prepare_material(MaterialCacheLifecycleState &state, std::uintptr_t dispatch) override {
        auto &p = scheduled.at(preparing_patch);
        require(&state == &p.resources->state(), "Material preparation lost its persistent world owner");
        require(!p.batches.empty() && !p.batches.back(), "Material preparation lost its pending batch slot");
        const auto &placement = world->cache().patches[preparing_patch];
        p.batches.back() = std::make_unique<MaterialCacheJobBatch>(state, placement.columns(), placement.rows(),
                                                                    dispatch, true);
    }
    void reset_scheduled() {
        for (auto &p : scheduled) {
            p.batches.clear();
            p.next_execution = p.next_completion = 0;
            p.scores.reset();
            p.resources->release_cache(*cache_pool, true, true, true);
            auto &state = p.resources->state();
            state.requested_state = state.completed_state = 0;
            state.prepared = false;
        }
        scheduler = {};
        loading.reset();
    }

    Impl(std::shared_ptr<GlContext> c, std::shared_ptr<const MaterialWorld> w,
         const LightingCubeAssets &cube_assets, MaterialCacheMipPolicy mip, LightingCubeSampling mode,
         LandShadowArithmetic affine, LandMipSampling filter, std::shared_ptr<const GlColorKeys> keys)
        : context(std::move(c)), world(std::move(w)), assets(world, &world->assets()),
          material_textures(context, assets), cubes(*context, cube_assets), color_keys(std::move(keys)),
          mip_policy(mip), sampling(mode), arithmetic(affine), mip_sampling(filter),
          diffuse(assets->rows().size()), height(assets->rows().size()) {
        require(!color_keys || color_keys->belongs_to(*context),
                "World ColorKey resources belong to another OpenGL context");
        require(mode == LightingCubeSampling::ordinary ||
                    mode == LightingCubeSampling::integer_no_coordinates ||
                    mode == LightingCubeSampling::measured_coordinates,
                "Invalid world cube sampling mode");
        require(affine == LandShadowArithmetic::instruction_order ||
                    affine == LandShadowArithmetic::measured_fma_t_x_z_y,
                "Invalid world LAND arithmetic profile");
        require(filter == LandMipSampling::ordinary || filter == LandMipSampling::d3d9_trilinear ||
                    filter == LandMipSampling::d3d9_rgb565,
                "Invalid world LAND mip sampling profile");
        const auto &land = world->terrain().land();
        const auto &cache = world->cache();
        require(cache.columns == land.columns() && cache.rows == land.rows() &&
                    cache.patches.size() == std::size_t(cache.columns) * cache.rows &&
                    world->terrain().land_styles().masks.size() == cache.patches.size(),
                "Material world lost its LAND/cache/style identity");
        for (std::size_t i = 0; i < cache.patches.size(); ++i) {
            const auto p = patch(i);
            const auto &m = cache.patches[i];
            require(m.origin == MaterialOrigin{p.source_x(), p.source_y()} &&
                        m.interior_columns == p.source_samples_x() && m.interior_rows == p.source_samples_y(),
                    "Material and LAND patch extents differ");
        }
        require(assets->textures().size() < 256, "Material resource table exceeds world transport domain");
        const auto first = static_cast<std::uint32_t>(assets->textures().size() + 1);
        for (unsigned i = 0; i < 13; ++i)
            source.textures[i] = first + i;
        height_id = first + 13;
        final_id = first + 14;
        source.normal_vertex_buffer = 1;
        source.normal_index_buffer = 2;
        source.lighting_vertex_buffer = 3;
        source.material_vertex_buffer = 4;
        source.patch_index_buffer = 5;
        source.shader_bindings = {1, 2, 3, 4};
        for (unsigned i = 0; i < 5; ++i) {
            source.lighting.sunlight[i] = first + 15 + 2 * i;
            source.lighting.shadowlight[i] = first + 16 + 2 * i;
            source.lighting.color_correct[i] = first + 25 + i;
        }
        for (unsigned i = 0; i < assets->rows().size(); ++i) {
            const auto &row = assets->rows()[i];
            scales.push_back(row.scale);
            flags[i] = static_cast<std::uint32_t>(row.projection);
        }
        for (int i = 0; i < static_cast<int>(assets->expansion().source_rows().size()); ++i) {
            const std::array selected{i};
            const auto resources = assets->selected_resources(selected);
            const auto row = assets->expansion().source_rows()[i];
            diffuse[row] = resources[0];
            height[row] = resources[8];
        }
        // The dense temporary is derived from this world's final page owner.
        // Uploaded windows own their GPU data after this local copy is destroyed.
        const auto field = world->terrain().height_field();
        for (int x = 0; x < field.domain().texture_columns(); ++x)
            for (int y = 0; y < field.domain().texture_rows(); ++y)
                heights.emplace(x + 16 * y,
                                GlTexture::alpha8(*context, 1024, 1024, field.extract_a8_texture(x, y)));
        land_geometry = std::make_shared<GlLandGeometryPool>(context);
        land_packets = std::make_shared<MaterialPacketPool>();
        for (unsigned i = 0; i < 6; ++i) land_sources.shared[i] = i + 1;
        for (unsigned i = 0; i < 5; ++i) {
            land_sources.style_diffuse[i] = 7 + i * 2;
            land_sources.style_specular[i] = 8 + i * 2;
        }
        std::uint32_t next = 17;
        for (const auto &[slot, texture] : heights) land_heights.emplace(slot, next++);
        land_final_first = next;
        require(std::uint64_t(next) + cache.patches.size() <= 65536, "LAND packet texture table exceeds U16 handles");
    }
    void release_land_draws() {
        land_renderers.clear();
        if (cache_pool) reset_scheduled();
        scheduled.clear();
        scheduler_patches.clear();
        all_patches.clear();
        cache_pool.reset();
        finals.clear();
        capture_resources.clear();
    }
    ~Impl() {
        try {
            release_land_draws();
        } catch (...) {
            std::terminate();
        }
    }
    terrain::LandPatch patch(std::size_t index) const {
        const auto &land = world->terrain().land();
        require(index < world->cache().patches.size(), "World material patch index outside owner");
        return land.patch(static_cast<int>(index / land.rows()), static_cast<int>(index % land.rows()));
    }
    std::unique_ptr<GlLandPatchResources> make_resources(std::size_t index) {
        auto resources = std::make_unique<GlLandPatchResources>(land_geometry, land_packets);
        resources->state().textures.final[0] = land_final_first + static_cast<std::uint32_t>(index);
        return resources;
    }
    void rebuild_resources(std::size_t index, GlLandPatchResources &resources) {
        const auto p = patch(index);
        resources.rebuild({static_cast<std::uint32_t>(p.source_samples_x()),
                            static_cast<std::uint32_t>(p.source_samples_y()), world->terrain().land_styles().masks[index],
                            land_heights.at(p.texture_table_slot()), 0.f, 6, true}, land_sources, 1);
    }
    std::unique_ptr<GlLandPatchResources> new_resources(std::size_t index) {
        auto resources = make_resources(index);
        rebuild_resources(index, *resources);
        return resources;
    }
    const GlLandPatchResources &resources(std::size_t index) const {
        if (cache_pool) return *scheduled.at(index).resources;
        if (const auto at = finals.find(index); at != finals.end()) return *at->second.resources;
        return *capture_resources.at(index);
    }
    GlLandMaterialRenderer &land_renderer(std::size_t index) {
        const auto p = patch(index);
        if (!cache_pool && !finals.contains(index) && !capture_resources.contains(index)) capture_resources.emplace(index, new_resources(index));
        const auto &geometry = resources(index).geometry();
        const auto key = std::pair{p.source_samples_x(), p.source_samples_y()};
        auto found = land_renderers.find(key);
        if (found == land_renderers.end()) {
            found = land_renderers.emplace(key, std::make_unique<GlLandMaterialRenderer>(
                context, land_geometry, geometry, 1, arithmetic, mip_sampling)).first;
        }
        found->second->set_geometry(geometry);
        return *found->second;
    }
    GlMaterialCacheTextures &targets(GlMaterialWorldCacheExtents extents) {
        const auto key = std::pair{extents.temporary_size, extents.cache_size};
        auto found = temporary.find(key);
        if (found == temporary.end())
            found =
                temporary
                    .emplace(key, std::make_unique<GlMaterialCacheTextures>(
                                      context,
                                      material_cache_texture_plan(extents.temporary_size, extents.cache_size,
                                                                  false),
                                      mip_policy))
                    .first;
        return *found->second;
    }
};
GlMaterialWorldRenderer::GlMaterialWorldRenderer(std::shared_ptr<GlContext> context,
                                                 std::shared_ptr<const MaterialWorld> world,
                                                 const LightingCubeAssets &cubes, MaterialCacheMipPolicy mip,
                                                 LightingCubeSampling sampling,
                                                 LandShadowArithmetic arithmetic,
                                                 LandMipSampling mip_sampling,
                                                 std::shared_ptr<const GlColorKeys> color_keys) {
    require(context && world && !world->cache().patches.empty(),
            "Material renderer needs a live owned world/context");
    impl_ = std::make_unique<Impl>(std::move(context), std::move(world), cubes, mip, sampling, arithmetic,
                                   mip_sampling, std::move(color_keys));
}
GlMaterialWorldRenderer::~GlMaterialWorldRenderer() = default;
void GlMaterialWorldRenderer::enable_decals(std::shared_ptr<const terrain::TerrainDecalAssets> assets,
                                           std::array<std::uint32_t, 2> available_commands) {
    auto &s = *impl_;
    require(assets && !s.decals && !s.preparing && !s.score_seed && !s.cache_pool &&
                s.finals.empty() && s.capture_resources.empty(),
            "Attach one decal provider before generating or scheduling world resources");
    s.decals = std::make_unique<Impl::Decals>(s.context, std::move(assets),
        s.world->terrain().owner().parameters, available_commands);
}
TerrainDecalRegionRefresh GlMaterialWorldRenderer::Impl::decal_refresh(
    std::uint32_t revision, const MaterialNormalSeed &seed) {
    require(decals && !preparing && !score_seed, "Decal mutation requires an idle owned provider");
    if (!cache_pool) {
        require(finals.empty() && capture_resources.empty(),
                "Mutating generated decals requires retained world scheduling");
        return {};
    }
    check_refresh(seed);
    return [this, revision, &seed](std::array<float, 4> bounds, bool stamp, bool) {
        // Original 69E140/73EC80 pass stamp=false; the fourth flag is unused
        // by 684240. Keep each callback's independent refresh/barrier.
        refresh_region({bounds[0], bounds[1]}, {bounds[2], bounds[3]}, revision, seed, stamp);
    };
}
std::uint32_t GlMaterialWorldRenderer::submit_decal(const terrain::TerrainDecalRecord &material,
    const terrain::TerrainDecalGeometry &geometry, std::uint8_t mode, std::uint32_t revision,
    const MaterialNormalSeed &score_seed) {
    auto &s = *impl_;
    const auto refresh = s.decal_refresh(revision, score_seed);
    const auto count = s.decals->assets->textures().size();
    require(material.albedo <= count && material.heightmap <= count && material.specular <= count,
            "Decal material references an unknown provider resource");
    const PreparationScope scope(s.preparing);
    return s.decals->store->submit(material, geometry, mode, refresh);
}
void GlMaterialWorldRenderer::remove_decal(std::uint32_t handle, std::uint32_t revision,
    const MaterialNormalSeed &score_seed) {
    auto &s = *impl_;
    const auto refresh = s.decal_refresh(revision, score_seed);
    const PreparationScope scope(s.preparing);
    s.decals->store->remove(handle, refresh);
}
const TerrainDecalStore &GlMaterialWorldRenderer::decal_store() const {
    require(impl_->decals != nullptr, "World decal provider is absent");
    return *impl_->decals->store;
}
GlMaterialWorldLighting GlMaterialWorldLighting::from_terrain(const TerrainLighting &lighting,
                                                              const terrain::ReciprocalSqrtEstimate &estimate,
                                                              std::array<GLuint, 5> color_correct) {
    const auto cache = terrain_cache_lighting(lighting, estimate);
    return {cache.shadow, cache.half_vector, color_correct};
}
GlMaterialWorldPrepared
GlMaterialWorldRenderer::prepare(std::size_t index, const MaterialPatchSettings &settings,
                                 const MaterialNormalSeed &score_seed, const MaterialNormalSeed &vertex_seed,
                                 const GlMaterialWorldLighting &lighting, std::uint32_t shadow_samples,
                                 bool conditional_clear) {
    const auto size = static_cast<std::uint32_t>(settings.cache_size);
    return prepare(index, settings, {size, size}, score_seed, vertex_seed, lighting, shadow_samples,
                   conditional_clear);
}
GlMaterialWorldPrepared
GlMaterialWorldRenderer::prepare(std::size_t index, const MaterialPatchSettings &settings,
                                 GlMaterialWorldCacheExtents extents, const MaterialNormalSeed &score_seed,
                                 const MaterialNormalSeed &vertex_seed, const GlMaterialWorldLighting &lighting,
                                 std::uint32_t shadow_samples, bool conditional_clear) {
    require(settings.cache_mode == 0 && settings.parent_flag == 0 &&
                (settings.cache_size == 64 || settings.cache_size == 128 || settings.cache_size == 256 ||
                 settings.cache_size == 512) &&
                shadow_samples <= 32,
            "World cache requires ordinary diffuse settings and size64/128/256/512");
    require(extents.temporary_size >= 1 && extents.temporary_size <= 16384 &&
                extents.cache_size >= 1 && extents.cache_size <= 16384,
            "World quality target extents must be 1..16384");
    auto &s = *impl_;
    require(!s.cache_pool, "Explicit preparation is unavailable while retained scheduling is enabled");
    const PreparationScope preparation(s.preparing);
    require(!s.color_keys || lighting.color_correct == std::array<GLuint, 5>{},
            "Owned ColorKey resources cannot be replaced by explicit frame handles");
    s.patch(index);
    s.upload_decals();
    auto resources_owner = s.make_resources(index);
    auto &patch_state = resources_owner->state();
    patch_state.prepared = true;
    patch_state.textures.final_size = static_cast<std::uint32_t>(settings.cache_size);
    auto computed = s.world->build_patch(index, settings, score_seed, vertex_seed, patch_state);
    const auto size = static_cast<unsigned>(settings.cache_size);
    auto final = GlTexture::render_target(*s.context, size, size, levels(size, s.mip_policy.rgb565),
                                          RenderTextureFormat::rgb565);
    auto result = s.generate(index, std::move(computed), settings, extents, lighting, shadow_samples,
                             conditional_clear, final, nullptr);
    s.rebuild_resources(index, *resources_owner);
    s.finals.insert_or_assign(index, Impl::PatchDraw{std::move(final), std::move(resources_owner)});
    s.capture_resources.erase(index);
    return result;
}
GlMaterialWorldPrepared GlMaterialWorldRenderer::Impl::generate(
    std::size_t index, WorldMaterialPatch computed, const MaterialPatchSettings &settings,
    GlMaterialWorldCacheExtents extents, const GlMaterialWorldLighting &lighting,
    std::uint32_t shadow_samples, bool conditional_clear, const GlTexture &final, const GlTexture *higher) {
    auto &s = *this;
    const auto land = s.patch(index);
    const auto size = static_cast<unsigned>(settings.cache_size);
    auto &cache = s.targets(extents);
    // Plan IDs are a separate namespace from the globally unique LAND handles.
    const auto higher_id = s.final_id + 16;
    const auto material = computed.geometry.mesh();
    const LightingPatchMesh normal(material.columns(), material.rows(), computed.geometry.streams.normal24,
                                   material.indices());
    const MaterialCachePassSettings pass_settings{extents.temporary_size, size, shadow_samples, higher ? 5u : 0u, false, false,
                                                  false, false, settings.precision};
    const MaterialCachePassPatch p{static_cast<unsigned>(material.columns()),
                                   static_cast<unsigned>(material.rows()),
                                   {land.texture_offset_x(), land.texture_offset_y()},
                                   s.world->terrain().land_styles().masks[index],
                                   s.height_id,
                                   0,
                                   {higher ? higher_id : 0, 0},
                                   {s.final_id, 0},
                                   0,
                                   0,
                                   0};
    const auto &bounds = s.world->terrain().source().layout.bounds;
    require(computed.scores.selected.specular() == 0,
            "World cache executor does not yet support a specular material selection");
    const MaterialCachePassPalette palette{computed.scores.selected.sources(),
                                           s.assets->expansion().source_rows(),
                                           s.assets->expansion().source_modes(),
                                           s.scales,
                                           s.flags,
                                           s.diffuse,
                                           s.assets->specular_resources(),
                                           s.height,
                                           bounds[0],
                                           bounds[3]};
    std::optional<MaterialCacheDecalLayers> layers;
    if (s.decals)
        layers = prepare_material_cache_decals(pass_settings, p, computed.placement.origin,
            *s.decals->store, s.decals->renderer.index_buffer(), Decals::shader_binding, s.decals->available);
    const auto *provider = layers ? &*layers : nullptr;
    const assets::Bytes entering(material_cache_constant_bytes(pass_settings, provider));
    auto plan = material_cache_pass_plan(pass_settings, p, s.source, palette, entering, {}, provider);
    const auto &parameters = s.world->terrain().owner().parameters;
    GlMaterialCacheFrame frame{{},
                               {parameters.scale_x, parameters.scale_y, parameters.scale_z},
                               lighting.sun_direction,
                               lighting.half_vector,
                               {1.f / 1024, -1.f / 1024}};
    s.world->write_constants(frame.material_coefficients);
    std::vector<GlMaterialCacheTarget> resources;
    for (unsigned i = 0; i <= s.assets->textures().size(); ++i)
        resources.push_back({i, &s.material_textures.resource(static_cast<std::uint16_t>(i))});
    for (unsigned i = 0; i < 13; ++i)
        resources.push_back({s.source.textures[i], &cache.texture(static_cast<MaterialCacheRole>(i))});
    resources.push_back({s.height_id, &s.heights.at(land.texture_table_slot())});
    resources.push_back({s.final_id, &final});
    if (higher) resources.push_back({higher_id, higher});
    std::vector<GlMaterialCacheVolume> volumes;
    const auto color_correct = s.color_keys ? s.color_keys->color_correct() : lighting.color_correct;
    for (unsigned i = 0; i < 5; ++i) {
        resources.push_back({s.source.lighting.sunlight[i], &s.cubes.sunlight(i)});
        resources.push_back({s.source.lighting.shadowlight[i], &s.cubes.shadowlight(i)});
        volumes.push_back({static_cast<std::uint16_t>(s.source.lighting.color_correct[i]), color_correct[i]});
    }
    GlMaterialCacheRenderer renderer(s.context, material, normal, s.source, s.sampling);
    std::optional<GlMaterialCacheDecals> binding;
    if (s.decals)
        binding.emplace(GlMaterialCacheDecals{s.decals->renderer, Decals::shader_binding,
            [&](auto id, auto color) -> const GlTexture & { return s.decals->textures.resource(id, color); }});
    renderer.execute(plan, resources, volumes, frame, conditional_clear, binding ? &*binding : nullptr);
    return {std::move(computed), std::move(plan), frame,
            static_cast<std::uint32_t>(land.texture_table_slot())};
}
void GlMaterialWorldRenderer::enable_scheduling(std::span<const std::uint32_t, 4> counts,
                                                const MaterialPatchSettings &settings,
                                                GlMaterialWorldCacheExtents extents) {
    auto &s = *impl_;
    require(!s.cache_pool && s.finals.empty() && s.capture_resources.empty(),
            "Enable retained scheduling before creating generated LAND resources");
    require(!s.preparing, "Scheduling cannot interrupt material preparation");
    require(settings.cache_size == 64 && settings.cache_mode == 0 && settings.parent_flag == 0 &&
                (settings.precision == terrain::X87Precision::single ||
                 settings.precision == terrain::X87Precision::double_precision),
            "Retained world scheduling requires ordinary diffuse base64 settings");
    require(extents.temporary_size >= 1 && extents.temporary_size <= 16384 &&
                extents.cache_size >= 1 && extents.cache_size <= 16384,
            "World quality target extents must be 1..16384");
    const auto total = std::accumulate(counts.begin(), counts.end(), std::uint64_t{0});
    require(total && std::uint64_t(s.land_final_first) + total <= 65536,
            "Scheduled LAND texture namespace exceeds U16 handles");
    auto pool = std::make_unique<Impl::CachePool>(s.context, counts, s.mip_policy.rgb565, s.land_final_first);
    std::vector<Impl::ScheduledPatch> patches(s.world->cache().patches.size());
    std::vector<MaterialCacheSchedulerPatch> records(patches.size());
    std::vector<std::uint32_t> all(patches.size());
    std::iota(all.begin(), all.end(), 0u);
    for (std::size_t index = 0; index < patches.size(); ++index) {
        auto &p = patches[index];
        p.resources = std::make_unique<GlLandPatchResources>(s.land_geometry, s.land_packets);
        auto &state = p.resources->state();
        state.textures.final_size = state.textures.higher_size = 64;
        state.frame_stamp = 1;
        const auto land = s.patch(index);
        records[index].state = &state;
        records[index].bounds = land.culling_bounds();
        records[index].sphere = land.culling_sphere(settings.precision);
        records[index].enabled = 1;
        records[index].source_revision = 1;
    }
    s.cache_pool = std::move(pool);
    s.scheduled = std::move(patches);
    s.scheduler_patches = std::move(records);
    s.all_patches = std::move(all);
    s.scheduler = {};
    s.loading.reset();
    s.schedule_settings = settings;
    s.schedule_extents = extents;
    s.source_refresh_stamp = 0;
}
void GlMaterialWorldRenderer::set_scheduled_patch(std::size_t index, bool enabled, std::uint32_t blocked,
                                                  std::uint32_t priority, std::uint32_t revision) {
    auto &s = *impl_;
    require(s.cache_pool && !s.score_seed, "Scheduled patch controls require an idle retained world");
    auto &p = s.scheduler_patches.at(index);
    p.enabled = enabled;
    p.blocked = blocked;
    p.priority = priority;
    p.source_revision = revision;
}
const MaterialCacheSchedulerPatch &GlMaterialWorldRenderer::scheduled_patch(std::size_t index) const {
    return impl_->scheduler_patches.at(index);
}
const MaterialCacheSchedulerState &GlMaterialWorldRenderer::scheduler_state() const {
    require(impl_->cache_pool != nullptr, "World scheduling is not enabled");
    return impl_->scheduler;
}
void GlMaterialWorldRenderer::Impl::check_refresh(const MaterialNormalSeed &seed) const {
    require(cache_pool && !score_seed && !preparing && scheduler.pending.empty() && seed,
            "Material source refresh requires an idle retained world and normal provider");
    require(!loading || loading->finished, "Material refresh cannot interrupt initial loading");
    for (const auto &p : scheduled)
        require(p.next_execution == p.next_completion && p.next_completion == p.batches.size(),
                "Material refresh cannot interrupt pending workers");
}
std::size_t GlMaterialWorldRenderer::refresh_material_regions(
    std::span<const terrain::TerrainMarshApplication> regions, std::uint32_t revision,
    const MaterialNormalSeed &score_seed) {
    auto &s = *impl_;
    s.check_refresh(score_seed);
    const PreparationScope scope(s.preparing);
    std::size_t completed = 0;
    for (const auto &region : regions)
        completed += s.refresh_region(region.dirty_lower, region.dirty_upper, revision, score_seed, true);
    return completed;
}
std::size_t GlMaterialWorldRenderer::Impl::refresh_region(
    terrain::Point2 lower, terrain::Point2 upper, std::uint32_t revision,
    const MaterialNormalSeed &score_seed, bool stamp_owner) {
    auto &s = *this;
    struct Backend final : MaterialCacheRefreshBackend {
        Impl &s;
        const MaterialNormalSeed &seed;
        std::uint32_t revision;
        std::vector<std::uint32_t> pending;
        std::size_t completed = 0;
        Backend(Impl &owner, const MaterialNormalSeed &normal, std::uint32_t stamp)
            : s(owner), seed(normal), revision(stamp) {}
        std::uint32_t source_revision() const override { return revision; }
        void clear_selection(std::uint32_t id) override {
            if (s.scheduled[id].scores) s.scheduled[id].scores->selected.clear_sources();
        }
        void submit_refresh(std::uint32_t id) override { pending.push_back(id); }
        void wait_refresh() override {
            for (auto id : pending) {
                auto &p = s.scheduled[id];
                p.height_metrics = s.world->measure_patch(id, s.schedule_settings.precision);
                s.scheduler_patches[id].blocked = p.height_metrics->category;
                auto settings = s.schedule_settings;
                settings.parent_flag = s.scheduler_patches[id].material_parent_flag;
                p.scores = s.world->select_patch(id, settings, seed);
                p.resources->state().textures.specular = p.scores->selected.specular() != 0;
                ++completed;
            }
            pending.clear();
        }
    } backend(s, score_seed, revision);
    s.score_seed = &score_seed;
    try {
        material_cache_refresh_region(s.world->refresh_domain(), lower, upper,
            stamp_owner, s.schedule_settings.precision, s.scheduler_patches, s.source_refresh_stamp, backend);
    } catch (...) {
        s.score_seed = nullptr;
        s.reset_scheduled();
        throw;
    }
    s.score_seed = nullptr;
    return backend.completed;
}
std::uint32_t GlMaterialWorldRenderer::source_refresh_stamp() const { return impl_->source_refresh_stamp; }
const std::optional<MaterialCacheHeightMetrics> &GlMaterialWorldRenderer::patch_height_metrics(std::size_t id) const {
    return impl_->scheduled.at(id).height_metrics;
}
std::size_t GlMaterialWorldRenderer::free_cache_count(std::uint32_t width) const {
    require(impl_->cache_pool != nullptr, "World scheduling is not enabled");
    return impl_->free_count(width);
}
void GlMaterialWorldRenderer::Impl::complete_patch(
    std::uint32_t id, std::uint32_t frame_stamp, std::uintptr_t argument,
    const GlMaterialWorldLighting &lighting, std::uint32_t shadow_samples, bool conditional_clear,
    GlMaterialWorldScheduleResult &result,
    const std::function<void(std::size_t, const GlMaterialWorldPrepared &)> &generated) {
    auto &p = scheduled.at(id);
    const auto land = patch(id);
    const MaterialCacheRebuildSettings rebuild{
        static_cast<std::uint32_t>(land.source_samples_x()),
        static_cast<std::uint32_t>(land.source_samples_y()), world->terrain().land_styles().masks[id],
        land_heights.at(land.texture_table_slot()), 0.f, 6, true};
    auto &batch_owner = p;
    require(batch_owner.next_completion < batch_owner.batches.size(),
            "Material completion lost its pending patch entry");
    // A repeated scheduler occurrence can complete geometry without invoking
    // material generation (3/1 and 5/4 transitions). Keep that batch alive
    // for the later duplicate which does generate; consume the newest batch
    // only when the original lifecycle tail actually asks for append.
    const auto &lifecycle = p.resources->state();
    const bool needs_generation =
        lifecycle.requested_state == 1 ||
        (lifecycle.requested_state == 3 && lifecycle.completed_state != 1) ||
        (lifecycle.requested_state == 5 && lifecycle.completed_state != 4);
    std::size_t batch_index = batch_owner.next_completion;
    MaterialCacheJobBatch *batch = nullptr;
    if (needs_generation) {
        if (batch_owner.batches[batch_index])
            batch = batch_owner.batches[batch_index].get();
        else
            for (std::size_t i = batch_index; i-- > 0;)
                if (batch_owner.batches[i]) {
                    batch_index = i;
                    batch = batch_owner.batches[i].get();
                    break;
                }
        require(batch && &batch->state() == &p.resources->state(),
                "World generation lost its retained material batch");
    }
    const auto generate_cache = [&](MaterialCacheLifecycleState &state, std::uint32_t requested,
                                    std::uintptr_t) {
        require(batch && batch->executed() && &state == &p.resources->state(),
                "World generation requires its completed retained material batch");
        auto geometry = batch->append();
        WorldMaterialPatch computed{world->cache().patches[id], *p.scores, std::move(geometry),
                                    assets->selected_resources(p.scores->selected.sources())};
        auto patch_settings = schedule_settings;
        patch_settings.cache_size = static_cast<int>(state.textures.final_size);
        auto job = generate(id, std::move(computed), patch_settings, schedule_extents,
                            lighting, shadow_samples, conditional_clear,
                            cache_pool->texture(state.textures.final[0]),
                            requested == 5 ? &cache_pool->texture(state.textures.higher[0]) : nullptr);
        ++result.generated;
        if (generated) generated(id, job);
    };
    if (p.resources->complete(*cache_pool, rebuild, land_sources, 1, frame_stamp, argument, generate_cache))
        ++result.completed;
    if (needs_generation)
        batch_owner.batches[batch_index].reset();
    ++batch_owner.next_completion;
}
void GlMaterialWorldRenderer::begin_loading(std::span<const MaterialCacheSchedulerView> views,
                                           bool block_on_load, bool cull) {
    auto &s = *impl_;
    require(s.cache_pool && !s.score_seed && !s.loading && s.scheduler.pending.empty(),
            "Initial material loading requires a fresh idle scheduler");
    for (const auto &p : s.scheduled) {
        const auto &state = p.resources->state();
        require(!state.completed_state && !state.requested_state && p.batches.empty() &&
                    state.textures.final == std::array<std::uint32_t, 2>{} &&
                    state.textures.higher == std::array<std::uint32_t, 2>{},
                "Initial material loading requires fresh patch owners");
    }
    MaterialCacheLoadState owner;
    material_cache_begin_load(s.scheduler_patches, s.all_patches, views, block_on_load, cull, owner);
    s.scheduler.background_cursor = owner.background_cursor;
    s.loading = std::move(owner);
}
const MaterialCacheLoadState &GlMaterialWorldRenderer::loading_state() const {
    require(impl_->loading.has_value(), "Initial material loading has not begun");
    return *impl_->loading;
}
GlMaterialWorldLoadResult GlMaterialWorldRenderer::load_step(
    std::uint32_t frame_stamp, const MaterialNormalSeed &score_seed, const MaterialNormalSeed &vertex_seed,
    const GlMaterialWorldLighting &lighting, std::uint32_t shadow_samples, bool conditional_clear,
    const std::function<void(std::size_t, const GlMaterialWorldPrepared &)> &generated) {
    auto &s = *impl_;
    require(s.cache_pool && s.loading && !s.score_seed && s.scheduler.pending.empty(),
            "Material loading step requires an idle loading owner");
    require(score_seed && vertex_seed && shadow_samples <= 32,
            "Material loading requires normal providers and valid shadow samples");
    require(!s.color_keys || lighting.color_correct == std::array<GLuint, 5>{},
            "Owned ColorKey resources cannot be replaced by explicit frame handles");
    s.context->check("Material world loading step");
    GlMaterialWorldScheduleResult result;
    // GL cache passes own and bind their targets per patch. The original
    // frame/generation wrappers need no shared camera or dispatch resource here;
    // world-space lighting is supplied by the same producer as ordinary frames.
    struct Backend final : MaterialCacheLoadBackend {
        Impl &s;
        std::function<void(std::uint32_t)> execute;
        std::function<void(std::uint32_t, std::uint32_t, std::uintptr_t)> finish;
        GlMaterialWorldScheduleResult &result;
        Backend(Impl &owner, std::function<void(std::uint32_t)> worker,
                std::function<void(std::uint32_t, std::uint32_t, std::uintptr_t)> completion,
                GlMaterialWorldScheduleResult &counts)
            : s(owner), execute(std::move(worker)), finish(std::move(completion)), result(counts) {}
        void prepare_frame() override {}
        void begin_step() override {}
        bool acquire_base(std::uint32_t id) override { return s.acquire_base(id); }
        void prepare(std::uint32_t id, std::uintptr_t dispatch) override {
            s.prepare(id, dispatch);
            ++result.prepared;
        }
        void wait() override { execute(s.preparing_patch); }
        void complete(std::uint32_t id, std::uint32_t stamp, std::uintptr_t argument) override {
            finish(id, stamp, argument);
        }
        void end_generation() override {}
        void end_step() override {}
    } backend(s, [&](std::uint32_t id) { s.execute_patch(id, vertex_seed); },
              [&](std::uint32_t id, std::uint32_t stamp, std::uintptr_t argument) {
                  s.complete_patch(id, stamp, argument, lighting, shadow_samples, conditional_clear,
                                   result, generated);
              }, result);
    s.score_seed = &score_seed;
    try {
        s.upload_decals();
        for (auto id : s.all_patches)
            if (!s.scheduled[id].scores) s.refresh_selection(id);
        material_cache_load_step(s.scheduler_patches, *s.loading, s.schedule_settings.precision,
                                  reinterpret_cast<std::uintptr_t>(&backend), frame_stamp, 0, backend);
        s.score_seed = nullptr;
        return {s.loading->finished != 0, result.prepared, result.completed, result.generated};
    } catch (...) {
        s.score_seed = nullptr;
        s.reset_scheduled();
        throw;
    }
}
GlMaterialWorldScheduleResult GlMaterialWorldRenderer::schedule_frame(
    std::span<const MaterialCacheSchedulerView> views, const MaterialCacheSchedulerSettings &settings,
    std::uint32_t frame_stamp, const MaterialNormalSeed &score_seed, const MaterialNormalSeed &vertex_seed,
    const GlMaterialWorldLighting &lighting, std::uint32_t shadow_samples, bool conditional_clear,
    const std::function<void(std::size_t, const GlMaterialWorldPrepared &)> &generated) {
    auto &s = *impl_;
    require(s.cache_pool && !s.score_seed && s.scheduler.pending.empty(),
            "Retained world frame requires an idle scheduler");
    require(!s.loading || s.loading->finished, "Ordinary material frames must follow initial loading");
    require(views.size() <= 4 && settings.base_size == 64 && !settings.compressed_base &&
                settings.precision == s.schedule_settings.precision && shadow_samples <= 32 &&
                score_seed && vertex_seed,
            "Connected world frame requires up to four views, base64, matching precision and normal providers");
    require(!s.color_keys || lighting.color_correct == std::array<GLuint, 5>{},
            "Owned ColorKey resources cannot be replaced by explicit frame handles");
    // A completed frame has already consumed every retained worker batch.
    // Drop its null slots before the next selection pass so execution indexes
    // describe this frame only; repeated membership can then append one batch
    // for every pending occurrence without inheriting prior counters.
    for (auto &p : s.scheduled) {
        require(p.next_execution == p.next_completion && p.next_completion == p.batches.size(),
                "Material frame retained an incomplete worker batch");
        p.batches.clear();
        p.next_execution = p.next_completion = 0;
    }
    // Original selection retains repeated membership. Each prepare call now
    // owns an independent worker batch, while completion still consumes the
    // pending IDs in their original order. Views and culling are unchanged.
    s.context->check("Scheduled material world frame");
    GlMaterialWorldScheduleResult result;
    s.score_seed = &score_seed;
    try {
        s.upload_decals();
        // The immutable source owner supplies initial selection before the
        // outer scheduler; subsequent revisions use its original refresh call.
        for (auto id : s.all_patches)
            if (!s.scheduled[id].scores) s.refresh_selection(id);
        const auto &background = s.loading ? s.loading->background : s.all_patches;
        result.selection_finished = material_cache_schedule(s.scheduler_patches, s.all_patches, background,
                                                             views, settings, s.scheduler, s);
        if (s.loading) s.loading->background_cursor = s.scheduler.background_cursor;
        result.prepared = s.scheduler.pending.size();
        // Every batch survives the entire worker barrier. Append happens only
        // in the actual lifecycle generation callback below.
        for (auto id : s.scheduler.pending) s.execute_patch(id, vertex_seed);
        for (auto id : s.scheduler.pending)
            s.complete_patch(id, frame_stamp, 0, lighting, shadow_samples, conditional_clear, result, generated);
        s.scheduler.pending.clear();
        s.draw_stamp = frame_stamp;
        s.score_seed = nullptr;
        return result;
    } catch (...) {
        s.score_seed = nullptr;
        s.reset_scheduled();
        throw;
    }
}
GlMaterialWorldScheduleResult GlMaterialWorldRenderer::schedule_camera_frame(
    const terrain::CameraFrameState &camera, const MaterialCacheSchedulerSettings &settings,
    std::uint32_t frame_stamp, const MaterialNormalSeed &score_seed, const MaterialNormalSeed &vertex_seed,
    const GlMaterialWorldLighting &lighting, std::uint32_t shadow_samples, bool conditional_clear,
    const std::function<void(std::size_t, const GlMaterialWorldPrepared &)> &generated) {
    const auto views = material_cache_camera_views(camera);
    auto frame_settings = settings;
    frame_settings.projection = camera.projection;
    return schedule_frame(views, frame_settings, frame_stamp, score_seed, vertex_seed, lighting,
                          shadow_samples, conditional_clear, generated);
}
void GlMaterialWorldRenderer::invalidate_generated_resources() {
    auto &s = *impl_;
    require(!s.score_seed && !s.preparing, "World invalidation must follow material completion");
    s.context->check("Material world generated-resource invalidation");
    s.temporary.clear();
    s.release_land_draws();
    s.land_geometry = std::make_shared<GlLandGeometryPool>(s.context);
    s.land_packets = std::make_shared<MaterialPacketPool>();
}
void GlMaterialWorldRenderer::rebuild_context(std::shared_ptr<GlContext> context,
                                              const LightingCubeAssets &cubes,
                                              const ColorKeyAssets *color_key_assets) {
    require(context != nullptr, "Context recovery needs a live replacement context");
    require(impl_ != nullptr, "Context recovery needs an initialized material renderer");
    require(context != impl_->context, "Context recovery requires a different OpenGL context");
    require(!impl_->score_seed && !impl_->preparing,
            "Context replacement cannot interrupt active material work");
    require(!impl_->color_keys || impl_->color_keys.use_count() == 1,
            "Release external ColorKey owners before context recovery");
    if (impl_->color_keys)
        require(color_key_assets != nullptr, "Context recovery needs the original ColorKey assets");

    // Construct first so an upload/validation failure leaves the original
    // renderer intact. The replacement owns all new source textures, height
    // windows, geometry and cache storage on the new context.
    std::shared_ptr<const GlColorKeys> replacement_keys;
    if (impl_->color_keys)
        replacement_keys = std::make_shared<const GlColorKeys>(*context, *color_key_assets);
    auto replacement = std::make_unique<Impl>(
        context, impl_->world, cubes, impl_->mip_policy, impl_->sampling, impl_->arithmetic,
        impl_->mip_sampling, std::move(replacement_keys));
    if (impl_->decals) {
        const auto &old = *impl_->decals;
        auto restored = std::make_unique<Impl::Decals>(context, old.assets,
            impl_->world->terrain().owner().parameters, old.available);
        std::uint32_t last = 0;
        for (const auto &batch : old.store->batches()) last = std::max(last, batch.buffer);
        for (std::uint32_t id = 1; id <= last; ++id) {
            // Pending CPU edits can differ from the last uploaded prefix.
            // Preserve that distinction and the original stable buffer IDs.
            const auto prefix = old.renderer.read_vertices(id);
            require(restored->renderer.create() == id, "Decal recovery changed a buffer handle");
            restored->renderer.upload(id, prefix);
        }
        replacement->decals = std::move(restored);
        // No throwing operation follows: retain handles, holes and pending
        // flags only once every replacement GPU owner has been constructed.
        replacement->decals->store.swap(impl_->decals->store);
    }
    impl_.swap(replacement);
    // `replacement` now owns the old implementation. Its GlTexture
    // destructors make the old context current before deleting every old name.
}
bool GlMaterialWorldRenderer::prepared(std::size_t index) const {
    impl_->patch(index);
    if (impl_->cache_pool) {
        const auto &s = impl_->scheduled.at(index).resources->state();
        return s.completed_state != 0 && s.textures.final[0] != 0;
    }
    return impl_->finals.contains(index);
}
const GlTexture &GlMaterialWorldRenderer::final_texture(std::size_t index) const {
    if (impl_->cache_pool)
        return impl_->cache_pool->texture(impl_->scheduled.at(index).resources->state().textures.final[0]);
    return impl_->finals.at(index).final;
}
const GlTexture &GlMaterialWorldRenderer::higher_texture(std::size_t index) const {
    require(impl_->cache_pool != nullptr, "Higher world cache requires retained scheduling");
    return impl_->cache_pool->texture(impl_->scheduled.at(index).resources->state().textures.higher[0]);
}
const GlTexture &GlMaterialWorldRenderer::height_texture(std::size_t index) const {
    return impl_->heights.at(impl_->patch(index).texture_table_slot());
}
const GlTexture &GlMaterialWorldRenderer::temporary_texture(std::uint32_t size,
                                                            MaterialCacheRole role) const {
    return temporary_texture(GlMaterialWorldCacheExtents{size, size}, role);
}
const GlTexture &GlMaterialWorldRenderer::temporary_texture(GlMaterialWorldCacheExtents extents,
                                                            MaterialCacheRole role) const {
    return impl_->temporary.at({extents.temporary_size, extents.cache_size})->texture(role);
}
const MaterialCachePassSource &GlMaterialWorldRenderer::source_bindings() const { return impl_->source; }
const GlLandGeometryPool &GlMaterialWorldRenderer::land_geometry_pool() const {
    return *impl_->land_geometry;
}
const MaterialPacketPool &GlMaterialWorldRenderer::land_packet_pool() const { return *impl_->land_packets; }
const MaterialCachePacket &GlMaterialWorldRenderer::land_packet(std::size_t index, unsigned slot) const {
    return impl_->resources(index).packet(slot);
}
const MaterialCacheLifecycleState &GlMaterialWorldRenderer::patch_state(std::size_t index) const {
    return impl_->resources(index).state();
}
void GlMaterialWorldRenderer::finish_frame() {
    require(!impl_->score_seed && !impl_->preparing, "Packet retirement must follow material completion");
    impl_->context->check("Material world frame completion");
    impl_->land_packets->advance_frame();
    impl_->land_packets->collect();
}
LandMaterialConstants GlMaterialWorldRenderer::land_constants(std::size_t index,
                                                              LandMaterialConstants frame) const {
    const auto p = impl_->patch(index);
    const auto translation = p.world_translation();
    const auto &parameters = impl_->world->terrain().owner().parameters;
    frame.world = {
        {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {translation[0], translation[1], translation[2], 1}}};
    frame.offset = p.offset_constants();
    frame.world_scales = {parameters.scale_x, parameters.scale_y, parameters.scale_z, 0};
    frame.height_texture_scale = {1.f / 1024, -1.f / 1024, 0, 0};
    frame.validate();
    return frame;
}
void GlMaterialWorldRenderer::draw(std::size_t index, const LandMaterialConstants &frame,
                                   GlLandMaterialInputs scene, int width, int height) {
    auto &s = *impl_;
    require(!s.score_seed && !s.preparing, "LAND draws must follow material completion");
    unsigned slot = 0;
    if (s.cache_pool) {
        const auto &state = s.scheduled.at(index).resources->state();
        const auto selection = select_land_technique(LandPass::main, 1, 0, state.completed_state, 0);
        if (!selection) return;
        slot = (selection->packet_offset - 0x154) / 4;
        s.scheduler_patches[index].eviction_stamp = s.draw_stamp;
    }
    const auto final = s.cache_pool ? 0 : final_texture(index).id();
    auto constants = land_constants(index, frame);
    auto &renderer = s.land_renderer(index);
    const auto &packet = s.resources(index).packet(slot);
    renderer.draw_packet(packet, constants, [&](std::uint16_t handle) -> GLuint {
        if (!handle) return 0;
        if (s.cache_pool) {
            const auto &textures = s.scheduled[index].resources->state().textures;
            if (handle == textures.final[0] || handle == textures.higher[0]) return s.cache_pool->texture(handle).id();
        } else if (handle == s.land_final_first + index) return final;
        if (handle == s.land_heights.at(s.patch(index).texture_table_slot())) return height_texture(index).id();
        if (handle <= 6) {
            constexpr unsigned slots[]{1, 2, 3, 4, 5, 9};
            return scene.pixel[slots[handle - 1]];
        }
        if (handle >= 7 && handle < 17) {
            const unsigned style = (handle - 7) / 2, slot = (handle - 7) % 2;
            if (s.color_keys) return s.color_keys->at(slot ? ColorKeySystem::terrain_shadow : ColorKeySystem::terrain_fow, style).id();
            return scene.pixel[slot ? 7 : 6];
        }
        throw std::invalid_argument("LAND packet references a resource outside its patch/world");
    }, width, height);
}
GlLandMaterialInputs GlMaterialWorldRenderer::color_key_inputs(std::size_t index,
                                                               GlLandMaterialInputs scene) const {
    impl_->patch(index);
    if (impl_->color_keys) {
        const auto style = material_lighting_style(impl_->world->terrain().land_styles().masks[index]);
        scene.pixel[6] = impl_->color_keys->at(ColorKeySystem::terrain_fow, style).id();
        scene.pixel[7] = impl_->color_keys->at(ColorKeySystem::terrain_shadow, style).id();
    }
    return scene;
}
std::vector<float> GlMaterialWorldRenderer::capture(std::size_t index, const LandMaterialConstants &frame,
                                                    bool indexed) {
    require(!impl_->score_seed && !impl_->preparing, "LAND capture must follow material completion");
    auto constants = land_constants(index, frame);
    return impl_->land_renderer(index).capture(constants, height_texture(index).id(), indexed);
}
} // namespace civ5::graphics
