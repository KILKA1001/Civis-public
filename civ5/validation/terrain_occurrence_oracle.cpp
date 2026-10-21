// Validation input contains source map/placement/random state only. Original
// occurrence/claim/random outputs are read solely by the separate comparator.
#include "civ5/terrain/terrain_occurrences.hpp"
#include "coast_geometry_io.hpp"
namespace {
void write_occurrences(Writer &out, std::span<const TerrainOccurrence> occurrences) {
    for (auto o : occurrences) {
        out.word(o.cooked_index); out.word(o.orientation);
        out.word(o.position.x); out.word(o.position.y); out.word(o.position.z);
    }
}
void run_case(Reader &r, const TerrainPieceLibrary &library, const fs::path &output) {
    const auto width = r.integer(), height = r.integer(), pass = r.integer();
    const auto seed = r.word();
    std::array<IntMatrix4, 4> orientations;
    for (auto &matrix : orientations) for (auto &v : matrix) v = r.integer();
    IntMatrix4 first, second;
    for (auto &v : first) v = r.integer();
    for (auto &v : second) v = r.integer();
    Matrix4 cache;
    for (auto &v : cache) v = r.scalar();
    std::array<Vector4, 4> corners;
    for (auto &c : corners) for (auto &v : c) v = r.scalar();
    const auto count = r.word();
    check(count <= 4096, "Too many cooked mappings");
    std::vector<std::uint32_t> mapping;
    for (unsigned i = 0; i < count; ++i) mapping.push_back(r.word());
    const auto cells = r.blob();
    r.end();
    const TerrainPlacement placement(orientations, first, second, cache, corners);
    TerrainRandom random(seed);
    Writer phases;
    const auto generation = generate_terrain_occurrences(library, placement, mapping, width, height, cells, pass, random,
        [&](auto p, auto occurrences, const auto &claims, const auto &rng) {
            phases.word(p); phases.word(static_cast<std::uint32_t>(occurrences.size()));
            check(rng.calls() <= 0xffffffffu, "Random call count exceeds validation transport");
            phases.word(static_cast<std::uint32_t>(rng.calls())); phases.word(rng.state());
            phases.blob(claims.cells());
        });
    Writer occurrences, candidates, rng, descriptors, state;
    write_occurrences(occurrences, generation.occurrences);
    if (pass >= 0)
        for (auto p : placement.candidate_positions(width, height)) {
            candidates.word(p.x); candidates.word(p.y); candidates.word(p.z);
        }
    // Independent replay of this exact deterministic class audits the consumed
    // stream length; per-pass count/state and all generated occurrences are
    // compared to original instruction output independently.
    TerrainRandom stream(seed);
    for (std::uint64_t i = 0; i < random.calls(); ++i) {
        const auto v = stream.next();
        rng.bytes.push_back(static_cast<std::uint8_t>(v)); rng.bytes.push_back(static_cast<std::uint8_t>(v >> 8));
    }
    state.word(random.state()); state.word(static_cast<std::uint32_t>(random.calls()));
    state.word(static_cast<std::uint32_t>(generation.claims.claim_count()));
    const auto computed = build_terrain_descriptors(library, placement, generation.occurrences, mapping);
    for (std::size_t i = 0; i < computed.size(); ++i) {
        const auto &d = computed[i];
        const auto &record = library.record(mapping[generation.occurrences[i].cooked_index]);
        descriptors.word(d.record_state_flag);
        for (auto v : {record.row_tables.materials, record.row_tables.heights, record.row_tables.weights,
                       record.row_tables.priorities}) descriptors.word(v);
        for (auto v : {d.rectangle.x0, d.rectangle.y0, d.rectangle.x1, d.rectangle.y1,
                       d.grids->size(), d.grids->size()}) descriptors.word(v);
        descriptors.word(d.cell_gate);
    }
    check(fs::create_directory(output), "Cannot create terrain occurrence case output");
    for (auto item : {std::pair{"occurrences", &occurrences}, {"candidates", &candidates}, {"random", &rng},
                      {"phases", &phases}, {"state", &state}, {"descriptors", &descriptors}})
        save(output / (std::string(item.first) + ".bin"), item.second->bytes);
    save(output / "cells.bin", generation.claims.cells());
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        check(argc == 4, "Usage: civ5_terrain_occurrence_oracle asset-root input.bin fresh-output");
        const fs::path root = argv[1], input = argv[2], output = argv[3];
        check(!fs::exists(output), "Terrain occurrence output must be fresh");
        const auto size = fs::file_size(input);
        check(size >= 24 && size <= 32 * 1024 * 1024, "Invalid terrain occurrence input size");
        Bytes bytes(static_cast<std::size_t>(size));
        std::ifstream file(input, std::ios::binary); file.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
        check(bool(file), "Terrain occurrence input read failed");
        Reader r{bytes};
        const auto magic = r.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5TOCC01", "Wrong terrain occurrence input magic");
        const auto count = r.word();
        check(count > 0 && count <= 256, "Invalid terrain occurrence case count");
        const auto key = r.blob();
        const civ5::assets::LooseSource source(root);
        const auto library = TerrainPieceLibrary::load(source, civ5::assets::AssetKey(std::string(key.begin(), key.end())));
        std::vector<std::span<const std::uint8_t>> cases;
        for (unsigned i = 0; i < count; ++i) cases.push_back(r.blob());
        const auto gate_count = r.word();
        check(gate_count <= 4096, "Too many markup gate cases");
        Writer gates;
        for (unsigned i = 0; i < gate_count; ++i) {
            const auto required_mask = r.word(), mask = r.word();
            const auto required_category = r.word(), category = r.word();
            check(required_category <= 255 && category <= 255, "Invalid category byte");
            gates.bytes.push_back(terrain_markup_matches({required_mask, static_cast<std::uint8_t>(required_category), 1},
                                                        mask, static_cast<std::uint8_t>(category)));
        }
        const auto index_count = r.word();
        check(index_count <= 4096, "Too many random index cases");
        Writer indices;
        for (unsigned i = 0; i < index_count; ++i) {
            const auto seed = r.word(), bound = r.word();
            TerrainRandom random(seed);
            indices.word(random.index(bound)); indices.word(random.state());
            indices.word(static_cast<std::uint32_t>(random.calls()));
        }
        r.end();
        check(fs::create_directories(output), "Cannot create terrain occurrence output");
        save(output / "markup-gates.bin", gates.bytes);
        save(output / "random-index.bin", indices.bytes);
        // Complete source identity is independent of decoded output buffers.
        save(output / "source.fxsb", library.serialized_bytes());
        for (unsigned i = 0; i < cases.size(); ++i) {
            Reader item{cases[i]}; run_case(item, library, output / std::to_string(i));
        }
        std::cout << "PASS C++ terrain generation " << count << " cases and " << gate_count << " markup gates\n";
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
