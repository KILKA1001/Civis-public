// Actual FXSB -> computed owner setup -> existing original full-map generation.
// Original expected setup/occurrences are comparison-only inputs.
#include "civ5/terrain/terrain_occurrences.hpp"
#include "coast_geometry_io.hpp"

namespace {
Bytes read_file(const fs::path &path) {
    const auto size = fs::file_size(path);
    check(size > 12 && size <= 32 * 1024 * 1024, "Invalid preparation corpus size");
    Bytes bytes(static_cast<std::size_t>(size));
    std::ifstream file(path, std::ios::binary);
    check(bool(file.read(reinterpret_cast<char *>(bytes.data()), bytes.size())), "Preparation corpus read failed");
    return bytes;
}
std::size_t compare(std::span<const std::uint8_t> a, std::span<const std::uint8_t> b, const char *stage) {
    const auto mismatch = std::mismatch(a.begin(), a.end(), b.begin(), b.end());
    if (mismatch.first != a.end() || mismatch.second != b.end())
        throw std::runtime_error(std::string(stage) + " differs at byte " + std::to_string(mismatch.first - a.begin()));
    return a.size();
}
void word(Bytes &bytes, std::size_t at, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) bytes.at(at + i) = static_cast<std::uint8_t>(value >> (i * 8));
}
Writer pack(const TerrainPieceLibrary &library, const TerrainGenerationSetup &setup) {
    Writer out, matrices;
    out.word(std::bit_cast<std::uint32_t>(setup.maximum_pass));
    for (const auto &m : setup.orientations) for (auto v : m) matrices.word(v);
    out.blob(matrices.bytes);
    out.word(static_cast<std::uint32_t>(setup.cooked_to_primary.size()));
    for (auto index : setup.cooked_to_primary) {
        out.blob(library.serialized_record(index).first(0x3d0));
        Writer active;
        for (auto p : library.record(index).active_coordinates) {
            active.bytes.push_back(p.x); active.bytes.push_back(p.y);
        }
        out.blob(active.bytes);
    }
    return out;
}
template<class F> void rejects(F f) {
    bool rejected = false;
    try { f(); } catch (const std::invalid_argument &) { rejected = true; }
    check(rejected, "Unsupported preparation input was accepted");
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        check(argc == 3, "Usage: civ5_terrain_preparation_tests asset-root original-preparation.bin");
        const auto bytes = read_file(fs::path(argv[2]));
        Reader r{bytes};
        const auto magic = r.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5TPR001", "Wrong preparation corpus magic");
        const civ5::assets::LooseSource source{fs::path(argv[1])};
        const auto library = TerrainPieceLibrary::load(source, civ5::assets::AssetKey("Resource/Common/TerrainPiecesLibrary.fxsb"));
        const auto raw = library.serialized_bytes();
        const auto primary = static_cast<std::size_t>(library.serialized_record(0).data() - raw.data());
        const auto cases = r.word();
        check(cases > 0 && cases <= 32, "Preparation case count");
        std::size_t compared = 0;
        for (std::uint32_t i = 0; i < cases; ++i) {
            Bytes patched(raw.begin(), raw.end());
            const auto patches = r.word();
            check(patches <= library.records().size() * 2, "Preparation patch count");
            for (std::uint32_t j = 0; j < patches; ++j) {
                const auto index = r.word(), offset = r.word(), value = r.word();
                check(index < library.records().size() && (offset == 0x3d0 || offset == 0x3c8), "Invalid preparation patch");
                word(patched, primary + index * 0x56c + offset, value);
            }
            const auto candidate = TerrainPieceLibrary::parse(patched);
            const auto setup = prepare_terrain_generation(candidate, {});
            compared += compare(pack(candidate, setup).bytes, r.blob(), "Original preparation");
            std::cout << "PASS original preparation " << i << ": " << setup.cooked_to_primary.size()
                      << " records, maximum " << setup.maximum_pass << '\n';
        }
        const auto setup = prepare_terrain_generation(library, {});
        Reader transforms{r.blob()};
        transforms.take(64);
        Matrix4 cache;
        for (auto &v : cache) v = transforms.scalar();
        transforms.take(128);
        std::array<Vector4, 4> corners;
        for (auto &c : corners) for (auto &v : c) v = transforms.scalar();
        transforms.end();
        const auto maps = r.word();
        check(maps == 2, "Missing original full-map generation controls");
        std::size_t total_occurrences = 0, generation_bytes = 0;
        for (std::uint32_t i = 0; i < maps; ++i) {
            TerrainRandom random(r.word());
            Reader header{r.blob()};
            IntMatrix4 first, second;
            for (auto &v : first) v = header.integer();
            for (auto &v : second) v = header.integer();
            header.take(20);
            const auto width = header.integer(), height = header.integer();
            header.take(4); header.end();
            const TerrainPlacement placement(setup.orientations, first, second, cache, corners);
            const auto cells = r.blob();
            const auto generated = generate_terrain_occurrences(library, placement, setup.cooked_to_primary,
                width, height, cells, setup.maximum_pass, random);
            Writer occurrences;
            for (auto o : generated.occurrences) {
                occurrences.word(o.cooked_index); occurrences.word(o.orientation);
                occurrences.word(o.position.x); occurrences.word(o.position.y); occurrences.word(o.position.z);
            }
            generation_bytes += compare(occurrences.bytes, r.blob(), "Original generated occurrences");
            generation_bytes += compare(generated.claims.cells(), r.blob(), "Original claimed cells");
            check(random.calls() == r.word() && random.state() == r.word(), "Original generator cursor differs");
            total_occurrences += generated.occurrences.size();
            std::cout << "PASS prepared full-map generation " << i << ": " << generated.occurrences.size()
                      << " occurrences, " << random.calls() << " random calls\n";
        }
        r.end();
        rejects([&] { prepare_terrain_generation(library, {1,0,0,0,0}); });
        Bytes invalid(raw.begin(), raw.end());
        word(invalid, primary + 0x3bc, 1);
        rejects([&] { prepare_terrain_generation(TerrainPieceLibrary::parse(invalid), {}); });
        std::cout << "PASS terrain preparation: " << compared << " setup bytes, " << generation_bytes
                  << " generation bytes, " << total_occurrences << " original occurrences, 2 rejection controls\n";
    } catch (const std::exception &e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
