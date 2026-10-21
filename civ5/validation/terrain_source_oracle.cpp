// Explicit occurrence inputs + actual FXSB/FPK sources -> typed descriptors,
// graph/perturbation/strips/gates/worker/coast. No decoded-grid or descriptor
// output fields are accepted from the fixture.
#include "civ5/terrain/terrain_placement.hpp"
#include "coast_map_io.hpp"
#include <cmath>
namespace {
void write_pieces(const TerrainPieceLibrary &library, const fs::path &output) {
    Writer source, records;
    check(library.asset().has_value(), "Terrain source audit requires an actual asset");
    write_asset(source, *library.asset(), library.serialized_bytes());
    save(output / "source-pieces.bin", source.bytes);
    records.word(static_cast<std::uint32_t>(library.records().size()));
    for (const auto &p : library.records()) {
        for (auto word : {p.ordinal, p.serialized_ordinal, p.generation_pass, p.raw_state, p.row_tables.heights,
                          p.row_tables.materials, p.row_tables.weights, p.row_tables.priorities})
            records.word(word);
        records.word(p.result_code);
        records.word(p.selection_key_a);
        records.word(p.selection_key_b);
        records.word(p.size());
        string(records, p.primary_name);
        string(records, p.secondary_name);
        for (const auto &name : p.dds_names)
            string(records, name);
        records.blob(p.markup);
        Writer active;
        for (auto c : p.active_coordinates) {
            active.bytes.push_back(c.x);
            active.bytes.push_back(c.y);
        }
        records.blob(active.bytes);
        records.blob(library.serialized_record(p.ordinal));
        const auto grids = library.grids(p.ordinal);
        Writer decoded;
        decoded.word(grids->size());
        decoded.u16(grids->heights());
        decoded.raw(grids->materials());
        decoded.raw(grids->weights());
        decoded.raw(grids->priorities());
        save(output / ("piece-" + std::to_string(p.ordinal) + ".bin"), decoded.bytes);
    }
    save(output / "piece-records.bin", records.bytes);
}
civ5::validation::WorkerCompositionSource placement_source(Reader &r, const TerrainPieceLibrary &library,
                                                         const fs::path &output) {
    const auto width = r.integer(), height = r.integer();
    const auto cw = r.integer(), ch = r.integer();
    check(width >= 1 && height >= 1 && width <= 128 && height <= 128 && cw == width * 64 && ch == height * 64,
          "Invalid terrain logical/cache domain");
    const auto bias = r.scalar();
    const auto pc = precision(r);
    check(std::isfinite(bias), "Non-finite terrain worker bias");
    std::array<IntMatrix4, 4> orientations;
    for (auto &m : orientations)
        for (auto &v : m)
            v = r.integer();
    IntMatrix4 first, second;
    Matrix4 cache;
    std::array<Vector4, 4> corners;
    for (auto &v : first)
        v = r.integer();
    for (auto &v : second)
        v = r.integer();
    for (auto &v : cache)
        v = r.scalar();
    for (auto &c : corners)
        for (auto &v : c)
            v = r.scalar();
    const TerrainPlacement placement(orientations, first, second, cache, corners);
    const auto mapping = palette(r);
    check(!mapping.empty() && mapping.size() <= 4096, "Invalid cooked-to-primary mapping count");
    for (auto ordinal : mapping)
        library.record(ordinal);
    const auto count = r.word();
    check(count <= 65536 && std::size_t(count) * 20 <= r.bytes.size() - r.at, "Invalid occurrence count");
    std::vector<TerrainOccurrence> occurrences;
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto index = r.word(), orientation = r.word();
        const auto x = r.integer(), y = r.integer(), z = r.integer();
        occurrences.push_back({index, orientation, {x, y, z}});
    }
    TerrainLogicalClaims claims(width, height, r.blob());
    r.end();
    Writer positions;
    const auto candidates = placement.candidate_positions(width, height);
    positions.word(static_cast<std::uint32_t>(candidates.size()));
    for (auto p : candidates) {
        positions.word(p.x);
        positions.word(p.y);
        positions.word(p.z);
    }
    for (const auto &o : occurrences) {
        check(std::find(candidates.begin(), candidates.end(), o.position) != candidates.end(),
              "Occurrence is outside the reconstructed candidate lattice");
        check(claims.try_claim(library.record(terrain_primary_ordinal(o, mapping, library)), o, placement),
              "Explicit occurrence rejected by computed source markup/claims");
    }
    const auto descriptors = build_terrain_descriptors(library, placement, occurrences, mapping);
    Writer packed, logical;
    for (std::size_t i = 0; i < descriptors.size(); ++i) {
        const auto &d = descriptors[i];
        const auto &record = library.record(terrain_primary_ordinal(occurrences[i], mapping, library));
        packed.word(d.record_state_flag);
        packed.word(record.row_tables.materials);
        packed.word(record.row_tables.heights);
        packed.word(record.row_tables.weights);
        packed.word(record.row_tables.priorities);
        for (auto v : {d.rectangle.x0, d.rectangle.y0, d.rectangle.x1, d.rectangle.y1})
            packed.word(v);
        packed.word(d.grids->size());
        packed.word(d.grids->size());
        packed.word(d.cell_gate);
    }
    logical.word(static_cast<std::uint32_t>(claims.claim_count()));
    logical.blob(claims.cells());
    save(output / "placement-descriptors.bin", packed.bytes);
    save(output / "placement-claims.bin", logical.bytes);
    save(output / "placement-candidates.bin", positions.bytes);
    return {cw, ch, bias, pc, descriptors};
}
std::string key(Reader &r) {
    const auto b = r.blob();
    check(!b.empty() && b.size() <= 1024, "Invalid terrain source asset key");
    return std::string(b.begin(), b.end());
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        check(argc == 5, "Usage: civ5_terrain_source_oracle asset-root coast-archive-or---loose input.bin fresh-output");
        const fs::path root = argv[1], input = argv[3], output = argv[4];
        const auto archive = civ5::assets::path_utf8(fs::path(argv[2]));
        check(!fs::exists(output), "Terrain source output must be fresh");
        const auto size = fs::file_size(input);
        check(size >= 24 && size <= 256 * 1024 * 1024, "Invalid terrain source input size");
        Bytes bytes(static_cast<std::size_t>(size));
        std::ifstream file(input, std::ios::binary);
        file.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
        check(bool(file), "Terrain source input read failed");
        Reader r{bytes};
        const auto magic = r.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5TSRC01", "Wrong terrain source input magic");
        const auto count = r.word();
        check(count <= 4096, "Invalid terrain source map record count");
        const civ5::assets::AssetKey style_key(key(r)), piece_key(key(r));
        const auto placement_request = r.blob(), gates = r.blob();
        const auto source = coast_source(root, archive);
        const civ5::assets::LooseSource loose(root);
        const auto pieces = TerrainPieceLibrary::load(loose, piece_key);
        const auto pc24 = CoastLibrary::load(*source, style_key, X87Precision::single),
                   pc53 = CoastLibrary::load(*source, style_key, X87Precision::double_precision);
        const Library lib{pc24.profiles(), {pc24.characters(), pc53.characters()}};
        check(fs::create_directories(output), "Cannot create terrain source output");
        Reader placement{placement_request};
        const auto worker = placement_source(placement, pieces, output);
        write_pieces(pieces, output);
        write_sources(pc24, output / "source-24.bin");
        write_sources(pc53, output / "source-53.bin");
        write_library(lib, output);
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto kind = r.word();
            Reader item{r.blob()};
            save(output / (std::to_string(i) + ".bin"),
                 run_map(kind, item, lib, {}, gates, output / std::to_string(i), &worker));
        }
        r.end();
        std::cout << "PASS actual FXSB " << pieces.records().size() << " records, " << worker.descriptors.size()
                  << " computed descriptors and " << count << " source-map/coast cases\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
