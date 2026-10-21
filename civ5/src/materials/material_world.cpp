#include "civ5/materials/material_world.hpp"
#include "civ5/materials/material_cache_jobs.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::materials {
void MaterialWorld::validate_provider(const MaterialAssets &assets) {
    const auto rows = assets.rows();
    if (rows.size() < 2 || rows.back().continent != -1 || rows.back().local_id != 0xffffffffu ||
        rows.back().diffuse != "Checkerboard.dds" || rows.back().height != "Checkerboard.dds" ||
        rows.back().specular || rows.back().projection != 0)
        throw std::invalid_argument("Material world requires the original complete terrain provider");
}
void MaterialWorld::validate_field(const terrain::TerrainWorld &world, const MaterialAssets &assets) {
    const auto domain = material_world_domain(world.owner(), 0);
    const auto bases = assets.expansion().base_slots();
    const auto count = assets.expansion().source_rows().size();
    for (int y = 0; y < domain.height; ++y)
        for (int x = 0; x < domain.width; ++x) {
            const auto code = world.owner().material.value_at(x, y);
            if (code >= bases.size() || bases[code] < 0 || static_cast<std::size_t>(bases[code]) >= count ||
                (assets.rows()[code].projection == 1 && static_cast<std::size_t>(bases[code] + 2) >= count))
                throw std::invalid_argument("World material page references an unavailable provider source");
        }
}
MaterialWorld::MaterialWorld(terrain::TerrainWorld world, MaterialAssets assets,
                             std::vector<std::uint32_t> codes, MaterialCacheLayout cache)
    : world_(std::move(world)), assets_(std::move(assets)), codes_(std::move(codes)),
      cache_(std::move(cache)) {}
MaterialWorld MaterialWorld::build(const terrain::TerrainPieceLibrary &pieces,
                                   const terrain::CoastLibrary &coasts, MaterialAssets &&assets,
                                   terrain::TerrainWorldInput input,
                                   const terrain::TerrainWorldStorage &storage,
                                   const terrain::ReciprocalEstimate &reciprocal,
                                   const terrain::CoastExecutionOrder &order,
                                   const terrain::TerrainWorldObservers &observers,
                                   const MaterialWorldLoadingMarsh *loading_marsh) {
    validate_provider(assets);
    auto codes = terrain_material_codes(assets.rows());
    input.coast_palette = codes;
    std::optional<terrain::TerrainWorldLoadingMarsh> resolved_marsh;
    if (loading_marsh)
        resolved_marsh.emplace(terrain::TerrainWorldLoadingMarsh{loading_marsh->sources,
            loading_marsh->stamp, codes, loading_marsh->precision,
            loading_marsh->transform, loading_marsh->write});
    auto world = terrain::TerrainWorld::build_from_library(pieces, coasts, input, storage, reciprocal,
        order, observers, resolved_marsh ? &*resolved_marsh : nullptr);
    validate_field(world, assets);
    auto cache = material_cache_layout(world.owner().parameters.width, world.owner().parameters.height);
    return {std::move(world), std::move(assets), std::move(codes), std::move(cache)};
}
MaterialWorld MaterialWorld::with_rivers(std::span<const terrain::RiverEdge> edges,
                                         std::uint32_t entering_random,
                                         const terrain::ReciprocalSqrtEstimate &estimate,
                                         terrain::X87Precision precision,
                                         const terrain::TerrainRiverObservers &observers) && {
    const auto settings = terrain_river_settings(codes_).raster(codes_, precision);
    auto world = std::move(world_).with_rivers(edges, entering_random, estimate, settings, observers);
    validate_field(world, assets_);
    return {std::move(world), std::move(assets_), std::move(codes_), std::move(cache_)};
}
WorldMaterialPatch MaterialWorld::build_patch(std::size_t index, const MaterialPatchSettings &settings,
                                              const MaterialNormalSeed &score_seed,
                                              const MaterialNormalSeed &vertex_seed) const {
    MaterialCacheLifecycleState state;
    state.prepared = true;
    state.textures.final_size = static_cast<std::uint32_t>(settings.cache_size);
    return build_patch(index, settings, score_seed, vertex_seed, state);
}
MaterialWorld MaterialWorld::with_marshes(std::span<const terrain::TerrainFeature2Source> sources,
    const terrain::TerrainMarshStamp &stamp, terrain::X87Precision precision, terrain::Vec4Path transform,
    const terrain::TerrainMarshWriteObserver &write) && {
    // Check every possible new material before consuming the movable world.
    const auto bases = assets_.expansion().base_slots();
    const auto count = assets_.expansion().source_rows().size();
    for (const auto &source : sources) {
        if (source.continent < 0 || source.continent >= 5)
            throw std::invalid_argument("Unsupported material-world marsh continent");
        const auto code = 0x100000u + std::uint32_t(source.continent)*0x10000u;
        const auto found = std::find(codes_.begin(), codes_.end(), code);
        const auto index = std::size_t(found-codes_.begin());
        if (found == codes_.end() || index >= bases.size() || bases[index] < 0 ||
            std::size_t(bases[index]) >= count ||
            (assets_.rows()[index].projection == 1 && std::size_t(bases[index]+2) >= count))
            throw std::invalid_argument("Marsh material unavailable in this world's provider");
    }
    auto world = std::move(world_).with_marshes(sources, stamp, codes_, precision, transform, write);
    return {std::move(world), std::move(assets_), std::move(codes_), std::move(cache_)};
}
MaterialWorld MaterialWorld::with_observed_final_fields(
    std::span<const std::uint16_t> heights, std::span<const std::uint8_t> materials) && {
    auto world = std::move(world_).with_observed_final_fields(heights, materials);
    validate_field(world, assets_);
    return {std::move(world), std::move(assets_), std::move(codes_), std::move(cache_)};
}

MaterialCacheRefreshDomain MaterialWorld::refresh_domain() const {
    const auto &p = world_.owner().parameters;
    return {std::uint32_t(cache_.columns), std::uint32_t(cache_.rows),
            p.scale_x, p.scale_y, p.world_span_x, p.world_span_y};
}
MaterialCacheHeightMetrics MaterialWorld::measure_patch(std::size_t index, terrain::X87Precision pc) const {
    const auto &p = cache_.patches.at(index);
    const auto &field = world_.owner().height;
    return material_cache_height_metrics(p.columns(), p.rows(), p.origin,
        field.domain().width(), field.domain().height(), [&](int x, int y) { return field.value_at(x, y); }, pc);
}
WorldMaterialPatch MaterialWorld::build_patch(std::size_t index, const MaterialPatchSettings &settings,
                                              const MaterialNormalSeed &score_seed,
                                              const MaterialNormalSeed &vertex_seed,
                                              MaterialCacheLifecycleState &state) const {
    if (!vertex_seed || !state.prepared ||
        state.textures.final_size != static_cast<std::uint32_t>(settings.cache_size))
        throw std::invalid_argument("Invalid persistent world material owner or vertex seed");
    auto scores = select_patch(index, settings, score_seed);
    const auto placement = cache_.patches.at(index);
    MaterialCacheJobBatch batch(state, placement.columns(), placement.rows());
    execute_patch_jobs(index, settings, scores.selected, vertex_seed, batch);
    auto patch = batch.append();
    const auto resources = assets_.selected_resources(scores.selected.sources());
    return {placement, std::move(scores), std::move(patch), resources};
}
MaterialScoreResult MaterialWorld::select_patch(std::size_t index, const MaterialPatchSettings &settings,
                                                const MaterialNormalSeed &score_seed) const {
    if (settings.cache_size <= 0 ||
        (settings.precision != terrain::X87Precision::single &&
         settings.precision != terrain::X87Precision::double_precision) ||
        !score_seed)
        throw std::invalid_argument("Invalid material patch settings or missing normal estimate provider");
    const auto placement = cache_.patches.at(index);
    const auto source = material_world_domain(world_.owner(), settings.normal_x_remap);
    const auto &expansion = assets_.expansion();
    const int count = static_cast<int>(expansion.source_rows().size());
    std::array<std::int32_t, 64> slots, modes{};
    slots.fill(-1);
    std::copy(expansion.base_slots().begin(), expansion.base_slots().end(), slots.begin());
    for (std::size_t i = 0; i < assets_.rows().size(); ++i)
        modes[i] = assets_.rows()[i].projection;
    auto height_at = [&](int x, int y) { return world_.owner().height.value_at(x, y); };
    auto material_at = [&](int x, int y) { return world_.owner().material.value_at(x, y); };
    return produce_material_scores(placement.columns(), placement.rows(), placement.origin, source, count,
                                   slots, modes, settings.cache_mode, settings.parent_flag,
                                   expansion.source_rows(), assets_.specular_resources(), height_at,
                                   material_at, score_seed);
}
void MaterialWorld::execute_patch_jobs(std::size_t index, const MaterialPatchSettings &settings,
                                       const MaterialSelection &selection,
                                       const MaterialNormalSeed &vertex_seed,
                                       MaterialCacheJobBatch &batch) const {
    if (!vertex_seed || (settings.precision != terrain::X87Precision::single &&
                         settings.precision != terrain::X87Precision::double_precision))
        throw std::invalid_argument("Invalid world material worker settings or seed");
    const auto placement = cache_.patches.at(index);
    const auto source = material_world_domain(world_.owner(), settings.normal_x_remap);
    const auto &expansion = assets_.expansion();
    const int count = static_cast<int>(expansion.source_rows().size());
    std::array<std::int32_t, 64> slots, modes{};
    slots.fill(-1);
    std::copy(expansion.base_slots().begin(), expansion.base_slots().end(), slots.begin());
    for (std::size_t i = 0; i < assets_.rows().size(); ++i)
        modes[i] = assets_.rows()[i].projection;
    auto height_at = [&](int x, int y) { return world_.owner().height.value_at(x, y); };
    auto material_at = [&](int x, int y) { return world_.owner().material.value_at(x, y); };
    const auto samples = native_material_samples();
    auto normal_at = [&](int x, int y) {
        const auto input = material_normal_input({x, y}, source, height_at);
        return normalize_material_normal(input, [&](float squared) { return vertex_seed(x, y, squared); });
    };
    auto factors_at = [&](int x, int y, int axis) {
        return material_factors(count, samples, source.width, source.height, {x, y}, axis, slots, modes,
                                material_at);
    };
    const MaterialGeometryDomain geometry{placement.columns(), placement.rows(),  placement.origin, source,
                                          settings.cache_size, settings.precision};
    batch.execute(geometry, selection, height_at, normal_at, factors_at);
}
void MaterialWorld::write_constants(std::span<float, 384> destination) const {
    const auto &bounds = world_.source().layout.bounds;
    assets_.write_constants(bounds[3] - bounds[0], destination);
}
} // namespace civ5::materials
