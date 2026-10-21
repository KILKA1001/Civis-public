#pragma once
#include "civ5/terrain/terrain_world.hpp"
#include "material_setup.hpp"
#include "material_cache_refresh.hpp"

namespace civ5::materials {
struct MaterialCacheLifecycleState;
class MaterialCacheJobBatch;
struct MaterialPatchSettings {
    int cache_size, cache_mode, parent_flag, normal_x_remap;
    terrain::X87Precision precision;
};
struct WorldMaterialPatch {
    MaterialCachePatch placement;
    MaterialScoreResult scores;
    MaterialPatch geometry;
    std::array<std::uint16_t, 16> resources;
};
struct MaterialWorldLoadingMarsh {
    std::span<const terrain::TerrainFeature2Source> sources;
    const terrain::TerrainMarshStamp *stamp;
    terrain::X87Precision precision;
    terrain::Vec4Path transform;
    terrain::TerrainMarshWriteObserver write;
};
// One owner connects authored provider rows, generated coast/river U16+U8
// pages and the actual material cache lattice. No dense expected fields or
// precomputed material/normal streams are accepted by this production path.
class MaterialWorld {
  public:
    // Derives the coast palette from assets; input.coast_palette is replaced.
    // The original provider (including its final checkerboard row) is required.
    // Orientations, cooked-record mapping and maximum pass come from pieces
    // and input.factory. Their legacy input fields are ignored. Random cursors,
    // execution order and the remaining scene state are still explicit.
    // Optional loading marshes use the provider's actual material code order.
    static MaterialWorld build(const terrain::TerrainPieceLibrary &pieces,
                               const terrain::CoastLibrary &coasts, MaterialAssets &&assets,
                               terrain::TerrainWorldInput input, const terrain::TerrainWorldStorage &storage,
                               const terrain::ReciprocalEstimate &reciprocal,
                               const terrain::CoastExecutionOrder &order,
                               const terrain::TerrainWorldObservers &observers = {},
                               const MaterialWorldLoadingMarsh *loading_marsh = nullptr);
    MaterialWorld with_rivers(std::span<const terrain::RiverEdge> edges, std::uint32_t entering_random,
                              const terrain::ReciprocalSqrtEstimate &estimate,
                              terrain::X87Precision precision,
                              const terrain::TerrainRiverObservers &observers = {}) &&;
    // Later refresh path; initial loading passes loading_marsh to build().
    // Uses this provider's actual code order, preserving all source assets.
    MaterialWorld with_marshes(std::span<const terrain::TerrainFeature2Source> sources,
                              const terrain::TerrainMarshStamp &stamp, terrain::X87Precision precision,
                              terrain::Vec4Path transform,
                              const terrain::TerrainMarshWriteObserver &write = {}) &&;
    // Validation-only replay of an observed final R field. Generated source
    // descriptors remain unchanged; no additional terrain writes are applied.
    MaterialWorld with_observed_final_fields(std::span<const std::uint16_t> heights,
                                             std::span<const std::uint8_t> materials) &&;
    MaterialCacheRefreshDomain refresh_domain() const;
    MaterialCacheHeightMetrics measure_patch(std::size_t index, terrain::X87Precision precision) const;
    // Separate estimate callbacks keep processor-dependent RSQRT behavior
    // explicit. Score coordinates are relative 0..68; vertex coordinates are
    // absolute source-field positions.
    WorldMaterialPatch build_patch(std::size_t index, const MaterialPatchSettings &settings,
                                   const MaterialNormalSeed &score_seed,
                                   const MaterialNormalSeed &vertex_seed) const;
    // The resource/job owner supplies its live state, including readiness and
    // final size. Material workers keep this identity through append/release.
    WorldMaterialPatch build_patch(std::size_t index, const MaterialPatchSettings &settings,
                                   const MaterialNormalSeed &score_seed,
                                   const MaterialNormalSeed &vertex_seed,
                                   MaterialCacheLifecycleState &state) const;
    // Retained scheduling uses the same selection producer as build_patch.
    // Selection can precede texture acquisition; it does not allocate staging.
    MaterialScoreResult select_patch(std::size_t index, const MaterialPatchSettings &settings,
                                     const MaterialNormalSeed &score_seed) const;
    // Execute an already prepared native batch using this world's actual
    // sources. The batch retains the written streams until its owner calls
    // append() during cache generation. No source callback escapes this call.
    void execute_patch_jobs(std::size_t index, const MaterialPatchSettings &settings,
                            const MaterialSelection &selection, const MaterialNormalSeed &vertex_seed,
                            MaterialCacheJobBatch &batch) const;
    MaterialWorld(MaterialWorld &&) = default;
    MaterialWorld &operator=(MaterialWorld &&) = default;
    MaterialWorld(const MaterialWorld &) = delete;
    MaterialWorld &operator=(const MaterialWorld &) = delete;
    const terrain::TerrainWorld &terrain() const { return world_; }
    const MaterialAssets &assets() const { return assets_; }
    const MaterialCacheLayout &cache() const { return cache_; }
    std::span<const std::uint32_t> codes() const { return codes_; }
    void write_constants(std::span<float, 384> destination) const;

  private:
    MaterialWorld(terrain::TerrainWorld world, MaterialAssets assets, std::vector<std::uint32_t> codes,
                  MaterialCacheLayout cache);
    static void validate_provider(const MaterialAssets &assets);
    static void validate_field(const terrain::TerrainWorld &world, const MaterialAssets &assets);
    terrain::TerrainWorld world_;
    MaterialAssets assets_;
    std::vector<std::uint32_t> codes_;
    MaterialCacheLayout cache_;
};
} // namespace civ5::materials
