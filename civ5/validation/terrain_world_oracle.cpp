// Actual FXSB/FPK + raw source cells + explicit execution state -> one owned
// C++ world. The input contains no generated intermediate result or matrices.
#include "terrain_world_input.hpp"
#include <numeric>
namespace {
void grid(Writer &out, const WorldGrid &g) {
    for (auto v : g.first_matrix) out.word(v);
    for (auto v : g.second_matrix) out.word(v);
    for (auto v : g.descriptor) out.word(v);
    out.word(g.width); out.word(g.height);
}
void cursor(Writer &out, TerrainRandomCursor c) {
    out.word(c.entering); out.word(c.final);
    out.word(static_cast<std::uint32_t>(c.calls)); out.word(static_cast<std::uint32_t>(c.calls >> 32));
}
void write_source(const TerrainWorld &world, const TerrainPieceLibrary &pieces, const fs::path &path) {
    const auto &source = world.source();
    Writer layout, occurrences, descriptors;
    for (auto v : source.layout.transforms) layout.scalar(v);
    for (auto v : source.layout.bounds) layout.scalar(v);
    grid(layout, source.layout.source_grid); grid(layout, source.layout.worker_grid);
    save(path / "world.bin", layout.bytes);
    for (std::size_t i = 0; i < source.generation.occurrences.size(); ++i) {
        const auto &o = source.generation.occurrences[i];
        for (auto v : {o.cooked_index, o.orientation, std::uint32_t(o.position.x),
                        std::uint32_t(o.position.y), std::uint32_t(o.position.z)}) occurrences.word(v);
        const auto &d = source.descriptors[i];
        const auto &p = pieces.record(source.cooked_to_primary[o.cooked_index]);
        for (auto v : {d.record_state_flag, p.row_tables.materials, p.row_tables.heights,
                        p.row_tables.weights, p.row_tables.priorities}) descriptors.word(v);
        for (auto v : {d.rectangle.x0, d.rectangle.y0, d.rectangle.x1, d.rectangle.y1}) descriptors.word(v);
        descriptors.word(d.grids->size()); descriptors.word(d.grids->size()); descriptors.word(d.cell_gate);
    }
    save(path / "occurrences.bin", occurrences.bytes); save(path / "descriptors.bin", descriptors.bytes);
    save(path / "claims.bin", source.generation.claims.cells());
    save(path / "source-cells.bin", source.initial_cells);
    Writer packets;
    std::vector<SpatialRectangle> rectangles;
    for (auto &d : source.descriptors) rectangles.push_back(d.rectangle);
    for (auto &packet : route_worker_descriptors(world.land().domain(), rectangles)) {
        packets.word(static_cast<std::uint32_t>(packet.size()));
        for (auto index : packet) packets.word(index);
    }
    save(path / "packets.bin", packets.bytes);
}
void write_graph(Writer &out, const TerrainWorldCoast &coast, const Writer &stages) {
    initial(out, coast.graph.initial());
    Writer links;
    for (const auto &cell : coast.graph.links())
        for (unsigned k = 0; k < 3; ++k)
            for (bool reverse : {false, true}) {
                links.bytes.push_back(0);
                if (k >= cell.size()) { links.bytes.push_back(255); continue; }
                const auto n = reverse ? cell[k].reverse : cell[k].forward;
                links.bytes.push_back(std::uint8_t(n.edge)); links.word(n.x); links.word(n.y);
            }
    out.blob(links.bytes); out.word(10); out.raw(stages.bytes); out.word(1);
    out.word(static_cast<std::uint32_t>(coast.contours.size()));
    for (const auto &c : coast.contours) {
        out.word(c.start_x); out.word(c.start_y); out.word(c.start_edge); out.word(c.closed);
        out.word(c.owner_flag); out.word(c.family);
        if (c.wrap_offset) point(out, *c.wrap_offset);
        controls(out, c.points);
    }
}
void write_routing(Writer &out, const TerrainWorldCoast &coast, const Writer &pre_height, const Bytes &pre_material) {
    out.blob(coast.routing.logical_categories()); out.word(static_cast<std::uint32_t>(coast.routing.workers().size()));
    for (const auto &w : coast.routing.workers()) {
        out.word(w.origin_x); out.word(w.origin_y);
        for (const auto *ranges : {&w.boundaries, &w.fills}) {
            out.word(static_cast<std::uint32_t>(ranges->size()));
            for (auto r : *ranges) { out.word(r.first); out.word(r.end); out.word(r.gate); }
        }
        out.word(static_cast<std::uint32_t>(w.seeds.size()));
        for (auto s : w.seeds) { out.word(s.x); out.word(s.y); out.word(s.gate); }
    }
    out.blob(coast.gates); out.blob(pre_height.bytes); out.blob(pre_material);
}
void write_owner(const TerrainWorld &world, const fs::path &path) {
    const auto &owner = world.owner(); const auto &land = world.land();
    save(path / "height-headers.bin", owner.height.header_bytes());
    save(path / "material-headers.bin", owner.material.header_bytes());
    save(path / "height-pool.bin", owner.height.pool_bytes());
    save(path / "material-pool.bin", owner.material.pool_bytes());
    save(path / "owner-styles.bin", owner.styles.owner_bytes());
    Writer identity, members, styles, patches;
    for (auto v : owner.first_matrix) identity.word(v);
    for (auto v : owner.second_matrix) identity.word(v);
    for (auto v : owner.coordinate_descriptor) identity.word(v);
    const auto &p = owner.parameters;
    identity.word(p.width); identity.word(p.height);
    for (auto v : {p.world_span_x, p.world_span_y, p.center_x, p.center_y, p.center_z,
                    p.scale_x, p.scale_y, p.scale_z}) identity.scalar(v);
    identity.word(p.enabled); identity.scalar(p.factor);
    save(path / "owner.bin", identity.bytes);
    for (const auto &list : world.membership().patches) {
        members.word(static_cast<std::uint32_t>(list.size()));
        for (auto h : list) { members.word(h.x); members.word(h.y); members.word(h.z); }
        members.bytes.insert(members.bytes.end(), (16 - list.size()) * 12, 0xa5);
    }
    save(path / "membership.bin", members.bytes);
    for (auto v : world.land_styles().masks) styles.word(v);
    for (auto v : world.land_styles().ordinary_styles) styles.word(v);
    for (auto r : world.land_styles().reads) {
        styles.word(r.patch_index); styles.word(r.member_index);
        styles.word(r.transformed.x); styles.word(r.transformed.y); styles.word(r.transformed.z);
        styles.word(r.flat_address); styles.word(r.style);
    }
    save(path / "land-styles.bin", styles.bytes);
    for (int x = 0; x < land.columns(); ++x)
        for (int y = 0; y < land.rows(); ++y) {
            const auto p = land.patch(x, y);
            for (auto v : {p.patch_index(), p.source_x(), p.source_y(), p.source_samples_x(), p.source_samples_y(),
                            p.texture_table_slot()}) patches.word(v);
            for (auto v : p.offset_constants()) patches.scalar(v);
            for (auto v : p.world_translation()) patches.scalar(v);
            for (auto v : p.culling_bounds()) patches.scalar(v);
        }
    save(path / "land-patches.bin", patches.bytes);
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        check(argc == 5, "Usage: civ5_terrain_world_oracle asset-root coast-archive-or---loose input.bin fresh-output");
        const fs::path root = argv[1], input = argv[3], output = argv[4];
        check(!fs::exists(output), "World output must be fresh");
        const auto size = fs::file_size(input); check(size >= 20 && size <= 16 * 1024 * 1024, "Invalid world input size");
        Bytes bytes(static_cast<std::size_t>(size)); std::ifstream file(input, std::ios::binary);
        file.read(reinterpret_cast<char *>(bytes.data()), bytes.size()); check(bool(file), "World input read failed");
        Reader reader{bytes}; const auto magic = reader.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5TWLD01", "Wrong world input magic");
        const auto count = reader.word(); check(count >= 1 && count <= 16, "Invalid world count");
        const civ5::assets::AssetKey piece_key(key(reader)), coast_key(key(reader));
        std::vector<WorldRequest> requests;
        for (unsigned i = 0; i < count; ++i) { Reader item{reader.blob()}; requests.push_back(request(item)); }
        reader.end(); check(fs::create_directories(output), "Cannot create world output");
        std::vector<TerrainWorld> worlds;
        {
            const civ5::assets::LooseSource loose(root);
            const auto pieces = TerrainPieceLibrary::load(loose, piece_key);
            const auto source = coast_source(root, civ5::assets::path_utf8(fs::path(argv[2])));
            const auto coasts = CoastLibrary::load(*source, coast_key, X87Precision::double_precision);
            Writer assets; write_asset(assets, *pieces.asset(), pieces.serialized_bytes());
            save(output / "source-pieces.bin", assets.bytes); write_sources(coasts, output / "source-coasts.bin");
            for (unsigned i = 0; i < count; ++i) {
                auto &q = requests[i]; auto &v = q.input;
                v.cooked_to_primary = q.mapping; v.coast_palette = q.keys; v.expanded_logical_cells = q.cells;
                const auto path = output / std::to_string(i); check(fs::create_directory(path), "Cannot create world case output");
                Writer stages, phases, pre, traces, current; Bytes material;
                Bytes hm, mm;
                std::size_t reciprocal_calls = 0, jobs = 0;
                TerrainWorldObservers observers;
                observers.occurrence_pass = [&](int pass, auto occurrences, const auto &claims, const auto &random) {
                    phases.word(pass); phases.word(static_cast<std::uint32_t>(occurrences.size()));
                    phases.word(static_cast<std::uint32_t>(random.calls())); phases.word(random.state()); phases.blob(claims.cells());
                };
                observers.coast_smoothing = [&](unsigned stage, unsigned mode, const CoastSegmentGrid &g) {
                    stages.word(stage); stages.word(mode); Writer points;
                    for (const auto &c : g.cells()) for (auto s : c.segments) { point(points, s.first); point(points, s.second); }
                    stages.blob(points.bytes);
                };
                observers.pre_coast = [&](const WorkerField &field) {
                    pre.u16(field.heights()); material.assign(field.materials().begin(), field.materials().end());
                    hm.assign(field.heights().size(), 0); mm.assign(field.heights().size(), 0);
                };
                observers.coast_write = [&](CoastWriteChannel channel, int index, std::uint32_t value) {
                    const bool m = channel == CoastWriteChannel::material; (m ? mm : hm).at(index) = 1;
                    current.bytes.push_back(m); current.word(index); current.word(value);
                };
                observers.coast_job_complete = [&](std::size_t index) {
                    check(index == jobs++, "World coast completion order mismatch"); traces.blob(current.bytes); current.bytes.clear();
                };
                const LandStyleGrid prior(q.prior);
                auto world = TerrainWorld::build(pieces, coasts, v,
                    {prior, q.masks, nullptr, nullptr, [&](int n) { return Bytes(n, std::uint8_t(q.fill)); }},
                    [&](float determinant) {
                        check(reciprocal_calls < 2 && std::bit_cast<std::uint32_t>(determinant) == q.reciprocal[reciprocal_calls * 2],
                              "Computed world determinant differs");
                        return std::bit_cast<float>(q.reciprocal[reciprocal_calls++ * 2 + 1]);
                    }, [&](unsigned, std::span<const CoastDispatchJob> phase) {
                        std::vector<std::size_t> order(phase.size()); std::iota(order.begin(), order.end(), 0);
                        if (q.order) std::reverse(order.begin(), order.end()); return order;
                    }, observers);
                check(reciprocal_calls == 2, "World reciprocal call count mismatch");
                write_source(world, pieces, path); save(path / "generation-phases.bin", phases.bytes);
                const auto &coast = world.coast(); Writer graph, perturbed, full, routing, rng, order, final;
                write_graph(graph, coast, stages); save(path / "graph.bin", graph.bytes);
                for (const auto &p : coast.perturbations) perturbation(perturbed, p);
                save(path / "perturbations.bin", perturbed.bytes);
                full.word(static_cast<std::uint32_t>(coast.curves.size()));
                for (std::size_t c = 0; c < coast.curves.size(); ++c) { write_curves(full, coast.curves[c]); write_strip(full, coast.strips[c]); }
                write_plan(full, coast.dispatch, coast.strips); write_geometry(full, coast.geometry);
                write_routing(routing, coast, pre, material); full.blob(routing.bytes); save(path / "coast-chain.bin", full.bytes);
                cursor(rng, world.source().random); cursor(rng, coast.perturbation_random); cursor(rng, coast.dispatch_random);
                save(path / "random.bin", rng.bytes);
                for (const auto &phase : coast.execution_order) {
                    order.word(static_cast<std::uint32_t>(phase.size())); for (auto index : phase) order.word(static_cast<std::uint32_t>(index));
                }
                save(path / "execution-order.bin", order.bytes); save(path / "pre.u16", pre.bytes); save(path / "pre.u8", material);
                // Canonical post field is read from the world's owner pages, after composition.
                Writer h; Bytes m; const auto &domain = world.land().domain();
                for (int y = 0; y < domain.height(); ++y) for (int x = 0; x < domain.width(); ++x) {
                    const auto value = world.owner().height.value_at(x, y);
                    h.u16(std::span(&value, 1)); m.push_back(std::uint8_t(world.owner().material.value_at(x, y)));
                }
                final.blob(h.bytes); final.blob(m); final.blob(hm); final.blob(mm); final.word(static_cast<std::uint32_t>(jobs)); final.raw(traces.bytes);
                save(path / "post.bin", final.bytes); save(path / "post.u16", h.bytes); save(path / "post.u8", m);
                worlds.push_back(std::move(world));
                std::cout << "PASS world " << i << ": " << worlds.back().source().generation.occurrences.size()
                          << " generated occurrences, " << jobs << " coast jobs\n" << std::flush;
            }
        } // Libraries/resolver have gone; moved worlds must retain all sources.
        requests.clear(); requests.shrink_to_fit(); bytes.clear(); bytes.shrink_to_fit();
        for (unsigned i = 0; i < worlds.size(); ++i) {
            auto moved = std::move(worlds[i]); write_owner(moved, output / std::to_string(i));
            const auto field = moved.height_field(); const auto &d = field.domain(); Writer lifetime;
            for (int y = 0; y < d.height(); ++y) for (int x = 0; x < d.width(); ++x) {
                const auto v = field.u16_at(x, y); lifetime.u16(std::span(&v, 1));
            }
            save(output / std::to_string(i) / "lifetime.u16", lifetime.bytes);
            Writer retained;
            for (const auto &descriptor : moved.source().descriptors) {
                retained.word(descriptor.grids->size()); retained.u16(descriptor.grids->heights().first(1));
                retained.bytes.push_back(descriptor.grids->materials().back());
            }
            save(output / std::to_string(i) / "retained-sources.bin", retained.bytes);
        }
        std::cout << "PASS " << count << " owned worlds after input/library destruction and moves\n";
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
