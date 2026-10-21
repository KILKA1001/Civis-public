#include "civ5/terrain/terrain_world.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace civ5::terrain {
namespace {
void require(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
Matrix4 cache_matrix(const WorldLayout &world) {
    Matrix4 matrix;
    std::copy_n(world.transforms.begin() + 16, 16, matrix.begin());
    return matrix;
}
TerrainWorldSource build_source(const TerrainPieceLibrary &pieces, const TerrainWorldInput &input,
                           const ReciprocalEstimate &reciprocal, const TerrainWorldObservers &observers) {
    auto world = build_world_layout(input.map_width, input.map_height, input.mode, input.factory, reciprocal);
    const auto &grid = world.worker_grid;
    require(input.expanded_logical_cells.size() == std::size_t(grid.width) * grid.height * 8,
            "Logical source cells do not match the generated expanded world");
    std::array<Vector4, 4> corners;
    for (unsigned i = 0; i < 4; ++i)
        std::copy_n(world.transforms.begin() + 64 + i * 4, 4, corners[i].begin());
    const TerrainPlacement placement(input.orientations, grid.first_matrix, grid.second_matrix,
                                       cache_matrix(world), corners);
    TerrainRandom random(input.random_states.occurrences);
    auto generation = generate_terrain_occurrences(pieces, placement, input.cooked_to_primary,
        grid.width, grid.height, input.expanded_logical_cells, input.maximum_pass, random, observers.occurrence_pass);
    auto descriptors = build_terrain_descriptors(pieces, placement, generation.occurrences, input.cooked_to_primary);
    std::vector<std::uint32_t> masks;
    for (std::size_t at = 0; at < input.expanded_logical_cells.size(); at += 8) {
        const auto b = input.expanded_logical_cells.subspan(at, 4);
        masks.push_back(std::uint32_t(b[0]) | std::uint32_t(b[1]) << 8 |
                        std::uint32_t(b[2]) << 16 | std::uint32_t(b[3]) << 24);
    }
    return {std::move(world), input.mode, {input.expanded_logical_cells.begin(), input.expanded_logical_cells.end()},
            std::move(masks), {input.cooked_to_primary.begin(), input.cooked_to_primary.end()},
            std::move(generation), std::move(descriptors),
            {input.random_states.occurrences, random.state(), random.calls()}};
}
TerrainWorldCoast build_shoreline(const TerrainWorldSource &source, const CoastLibrary &library,
                         const TerrainWorldInput &input, const CoastExecutionOrder &order,
                         const TerrainWorldObservers &observers) {
    require(static_cast<bool>(order), "Missing explicit coast execution order");
    const auto &grid = source.layout.worker_grid;
    const auto cache = cache_matrix(source.layout);
    auto graph = CoastMapGraph::build({grid.width, grid.height, source.masks, grid.first_matrix,
        grid.second_matrix, cache, input.wrap_x, input.precision.coast, input.precision.coast_transform});
    auto smoothed = graph.smooth(input.precision.coast, observers.coast_smoothing);
    auto contours = graph.scan(smoothed);
    TerrainRandom perturbation_random(input.random_states.coast_perturbation);
    std::vector<CoastPerturbation> perturbations;
    std::vector<CoastCurveSet> curves;
    std::vector<CoastStrip> strips;
    for (const auto &contour : contours) {
        const auto wrap = contour.wrap_offset.value_or(Point2{0, 0});
        perturbations.push_back(perturb_coast(contour.points, contour.closed, wrap, perturbation_random,
                                             input.precision.coast, input.precision.coast_power));
        curves.push_back(build_coast_curves(perturbations.back().points, library.profiles(), library.characters(),
            {contour.closed, wrap, contour.owner_flag, input.coast_palette, input.coast_family_prefix,
             input.coast_common_prefix, input.precision.coast}));
        strips.push_back(build_coast_strip(curves.back(), library.profiles(), input.coast_palette,
                                           input.coast_family_prefix, input.coast_common_prefix));
    }
    std::vector<CoastDecalSource> decal_sources;
    for (std::size_t i = 0; i < contours.size(); ++i)
        decal_sources.push_back({perturbations[i].points, contours[i].owner_flag, contours[i].closed});
    const auto p = HeightOwnerParameters::from_world(source.layout);
    auto decal_paths = build_coast_decal_paths(decal_sources, {p.center_x, p.center_y}, input.precision.coast);
    auto road_network = build_coast_road_network(decal_paths, source.layout, input.precision.coast);
    auto scene_mask = build_coast_scene_mask(curves, p.width, p.height, input.precision.raster_bounds);
    TerrainRandom dispatch_random(input.random_states.coast_dispatch);
    auto dispatch = build_coast_dispatch(strips, input.coast_dispatch_gate, input.coast_palette,
                                         dispatch_random, input.coast_helper_return);
    std::array<std::vector<std::size_t>, 2> execution;
    for (unsigned phase = 0; phase < 2; ++phase) {
        const auto &jobs = dispatch.phases[phase];
        // The unmodified preferred-base original returns 255, hence zero.
        // Preserve rejection of other image-integrity outcomes: their worker
        // behavior is not covered by the ordinary raster path.
        for (auto job : jobs)
            require(job.allocation_flag == 0, "Unsupported nonzero coast integrity flag");
        execution[phase] = order(phase, jobs);
        require(execution[phase].size() == jobs.size(), "Coast execution order omits jobs");
        std::vector<bool> seen(jobs.size());
        for (auto index : execution[phase]) {
            require(index < jobs.size() && !seen[index], "Coast execution order is not a permutation");
            seen[index] = true;
        }
    }
    auto geometry = build_worker_geometry(curves, input.precision.coast);
    auto routing = route_worker_gates(HeightDomain(p.width, p.height),
        {grid.width, grid.height, source.masks, grid.first_matrix, grid.second_matrix, cache,
         input.wrap_x, input.outside_gate_category, input.precision.gates, input.precision.gate_transform},
        geometry.points, geometry.boundaries, geometry.fills);
    auto gates = routing.build_gate_field64();
    return {std::move(graph), std::move(smoothed), std::move(contours), std::move(perturbations),
            std::move(decal_paths), std::move(road_network), std::move(scene_mask),
            std::move(curves), std::move(strips), std::move(dispatch), std::move(execution),
            std::move(geometry), std::move(routing), std::move(gates),
            {input.random_states.coast_perturbation, perturbation_random.state(), perturbation_random.calls()},
            {input.random_states.coast_dispatch, dispatch_random.state(), dispatch_random.calls()}};
}
} // namespace
TerrainWorld::TerrainWorld(TerrainWorldSource source, TerrainWorldCoast coast, HeightOwnerState owner,
                           LandLayout land, LandMembership membership, LandStyleResult styles)
    : source_(std::move(source)), coast_(std::move(coast)), owner_(std::move(owner)), land_(std::move(land)),
      membership_(std::move(membership)), styles_(std::move(styles)) {}

TerrainWorld TerrainWorld::build_from_library(const TerrainPieceLibrary &pieces, const CoastLibrary &coasts,
                                 const TerrainWorldInput &input, const TerrainWorldStorage &storage,
                                 const ReciprocalEstimate &reciprocal, const CoastExecutionOrder &order,
                                 const TerrainWorldObservers &observers,
                                 const TerrainWorldLoadingMarsh *loading_marsh) {
    const auto setup = prepare_terrain_generation(pieces, input.factory);
    auto prepared = input;
    prepared.orientations = setup.orientations;
    prepared.cooked_to_primary = setup.cooked_to_primary;
    prepared.maximum_pass = setup.maximum_pass;
    return build(pieces, coasts, prepared, storage, reciprocal, order, observers, loading_marsh);
}
TerrainWorld TerrainWorld::build(const TerrainPieceLibrary &pieces, const CoastLibrary &coasts,
                                 const TerrainWorldInput &input, const TerrainWorldStorage &storage,
                                 const ReciprocalEstimate &reciprocal, const CoastExecutionOrder &order,
                                 const TerrainWorldObservers &observers,
                                 const TerrainWorldLoadingMarsh *loading_marsh) {
    require(std::isfinite(input.worker_bias), "Nonfinite world worker bias");
    require(static_cast<bool>(storage.allocate), "Missing native page allocator");
    auto generated = build_source(pieces, input, reciprocal, observers);
    const auto parameters = HeightOwnerParameters::from_world(generated.layout);
    const HeightDomain domain(parameters.width, parameters.height);
    const LandLayout land(domain);
    require(storage.initial_land_masks.size() == std::size_t(land.columns()) * land.rows(),
            "Initial LAND masks do not match the generated world");
    const auto &grid = generated.layout.worker_grid;
    auto owner = initialize_height_owner(parameters, grid.width, grid.height, generated.generation.claims.cells(),
        {grid.descriptor[0], grid.descriptor[1], grid.descriptor[2]}, storage.prior_styles,
        storage.previous_height, storage.previous_material, storage.allocate);
    const auto &b = generated.layout.bounds;
    const auto traversal = land_traversal_extent(b[0], b[1], b[3], b[4], parameters.scale_x, parameters.scale_y);
    auto membership = build_land_membership(land, traversal.width, traversal.height,
        generated.layout.source_grid.second_matrix, cache_matrix(generated.layout),
        {parameters.center_x, parameters.center_y}, input.precision.membership, input.precision.membership_transform);
    auto styles = accumulate_land_styles(membership.patches, owner.first_matrix, owner.styles, storage.initial_land_masks);
    auto shoreline = build_shoreline(generated, coasts, input, order, observers);
    const auto worker = compose_worker_field(domain, generated.descriptors, shoreline.gates,
                                              input.worker_bias, input.precision.worker);
    if (observers.pre_coast)
        observers.pre_coast(worker);
    std::optional<TerrainMarshResult> loading_result;
    if (loading_marsh) {
        require(loading_marsh->stamp != nullptr, "Loading marsh stamp missing");
        loading_result = apply_terrain_marshes(domain, worker.heights(), worker.materials(), {},
            loading_marsh->sources, true, *loading_marsh->stamp,
            {cache_matrix(generated.layout), input.mode, domain.width(), domain.height(),
             loading_marsh->precision, loading_marsh->transform, loading_marsh->material_codes},
            loading_marsh->write);
    }
    std::vector<CoastRasterJob> jobs;
    for (unsigned phase = 0; phase < 2; ++phase)
        for (auto index : shoreline.execution_order[phase]) {
            const auto job = shoreline.dispatch.phases[phase][index];
            jobs.push_back(shoreline.strips[job.strip].raster_job(job.chunk, shoreline.dispatch.remap_from,
                                                                shoreline.dispatch.remap_to));
        }
    const std::span<const std::uint8_t> coast_materials = loading_result
        ? std::span<const std::uint8_t>(loading_result->materials) : worker.materials();
    const auto final = apply_coast_jobs(domain, worker.heights(), coast_materials, jobs, input.wrap_x,
                                         input.precision.raster_bounds, observers.coast_write, observers.coast_job_complete);
    owner.height = owner.height.with_dense_values(final.heights());
    owner.material = owner.material.with_dense_values(final.materials());
    TerrainWorld result{std::move(generated), std::move(shoreline), std::move(owner), land,
                        std::move(membership), std::move(styles)};
    if (loading_result)
        result.marshes_.emplace(TerrainWorldMarshes{*loading_marsh->stamp,
            {loading_marsh->material_codes.begin(), loading_marsh->material_codes.end()},
            std::move(loading_result->state), std::move(loading_result->applications),
            loading_marsh->precision, loading_marsh->transform});
    return result;
}
TerrainWorld TerrainWorld::with_observed_final_fields(
    std::span<const std::uint16_t> heights, std::span<const std::uint8_t> materials) && {
    require(rivers_ && marshes_,
            "Observed final R requires completed coast, river and loading marsh stages");
    // Build both replacements before consuming the current owner. In particular,
    // do not invoke any coast/river writer after this final-field import.
    auto height_pages = owner_.height.with_dense_values(heights);
    auto material_pages = owner_.material.with_dense_values(materials);
    TerrainWorld result(std::move(*this));
    result.owner_.height = std::move(height_pages);
    result.owner_.material = std::move(material_pages);
    return result;
}

TerrainWorld TerrainWorld::with_river_triangles(std::span<const RiverTriangle> banks,
                                                std::span<const RiverTriangle> channels,
                                                const RiverRasterSettings &settings,
                                                const RiverWriteObserver &write_observer,
                                                const RiverSampleObserver &sample_observer,
                                                const RiverPassComplete &pass_complete) && {
    require(!rivers_, "River passes already applied to this world");
    const auto &domain = land_.domain();
    std::vector<std::uint16_t> heights;
    std::vector<std::uint8_t> materials;
    heights.reserve(std::size_t(domain.width()) * domain.height());
    materials.reserve(heights.capacity());
    for (int y = 0; y < domain.height(); ++y)
        for (int x = 0; x < domain.width(); ++x) {
            heights.push_back(owner_.height.value_at(x, y));
            materials.push_back(static_cast<std::uint8_t>(owner_.material.value_at(x, y)));
        }
    auto raster = apply_river_triangles(domain, heights, materials, banks, channels, settings,
                                          write_observer, sample_observer, pass_complete);
    auto height_pages = owner_.height.with_dense_values(raster.heights);
    auto material_pages = owner_.material.with_dense_values(raster.materials);
    TerrainWorldRivers rivers{{banks.begin(), banks.end()}, {channels.begin(), channels.end()},
        {settings.material_codes.begin(), settings.material_codes.end()}, std::move(raster.bank_visited),
        settings.level, settings.precision, settings.remap_from, settings.remap_to, settings.river_code, std::nullopt};
    // No input buffer or library is referenced by the returned world. Moves of
    // the owning vectors/page stores below cannot fail after consuming *this.
    TerrainWorld result(std::move(*this));
    result.owner_.height = std::move(height_pages);
    result.owner_.material = std::move(material_pages);
    result.rivers_.emplace(std::move(rivers));
    return result;
}
TerrainWorld TerrainWorld::with_rivers(std::span<const RiverEdge> edges, std::uint32_t entering_random,
                                       const ReciprocalSqrtEstimate &estimate, const RiverRasterSettings &settings,
                                       const TerrainRiverObservers &observers) && {
    require(!rivers_, "River passes already applied to this world");
    const auto &layout = source_.layout;
    auto network = RiverNetwork::build(layout.source_grid.width, layout.source_grid.height, edges);
    TerrainRandom random(entering_random);
    const auto matrix = cache_matrix(layout);
    const auto controls = build_river_centerlines(network, matrix, random, observers.controls);
    auto extended = extend_river_centerlines(network, controls, matrix, source_.map_mode, estimate);
    const auto &domain = land_.domain();
    std::vector<std::uint16_t> heights;
    heights.reserve(std::size_t(domain.width()) * domain.height());
    for (int y = 0; y < domain.height(); ++y)
        for (int x = 0; x < domain.width(); ++x)
            heights.push_back(owner_.height.value_at(x, y));
    auto sampled = sample_river_heights(domain, heights, extended);
    auto bank_lines = widen_river_centerlines(extended, random, estimate);
    auto triangles = build_river_triangles(extended, bank_lines, sampled, estimate, observers.curves);
    TerrainWorldRiverGeometry generated{{edges.begin(), edges.end()}, std::move(network), std::move(extended),
        std::move(bank_lines), std::move(sampled), {entering_random, random.state(), random.calls()}};
    auto completed = std::move(*this).with_river_triangles(triangles.banks, triangles.channels, settings,
        observers.write, observers.sample, observers.pass_complete);
    completed.rivers_->generated.emplace(std::move(generated));
    return completed;
}
TerrainWorld TerrainWorld::with_marshes(std::span<const TerrainFeature2Source> sources,
                                        const TerrainMarshStamp &stamp,
                                        std::span<const std::uint32_t> material_codes,
                                        X87Precision precision, Vec4Path transform,
                                        const TerrainMarshWriteObserver &write) && {
    require(!marshes_, "Loading marsh stream already applied to this world");
    const auto &domain = land_.domain();
    const auto count = std::size_t(domain.width()) * domain.height();
    std::vector<std::uint16_t> heights; heights.reserve(count);
    std::vector<std::uint8_t> materials; materials.reserve(count);
    for (int y = 0; y < domain.height(); ++y)
        for (int x = 0; x < domain.width(); ++x) {
            heights.push_back(owner_.height.value_at(x, y));
            materials.push_back(static_cast<std::uint8_t>(owner_.material.value_at(x, y)));
        }
    auto applied = apply_terrain_marshes(domain, heights, materials, {}, sources, true, stamp,
        {cache_matrix(source_.layout), source_.map_mode, domain.width(), domain.height(),
         precision, transform, material_codes}, write);
    auto pages = owner_.material.with_dense_values(applied.materials);
    TerrainWorldMarshes marshes{stamp, {material_codes.begin(), material_codes.end()},
        std::move(applied.state), std::move(applied.applications), precision, transform};
    TerrainWorld result(std::move(*this));
    result.owner_.material = std::move(pages);
    result.marshes_.emplace(std::move(marshes));
    return result;
}
} // namespace civ5::terrain
