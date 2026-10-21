#include "civ5/terrain/terrain_decal_geometry.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace civ5::terrain;
using Bytes = civ5::assets::Bytes;
namespace {
std::size_t checks = 0, vertex_bytes = 0, index_bytes = 0, submissions = 0;
void check(bool ok, const char *message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F f) {
    bool caught = false;
    try { f(); } catch (const std::invalid_argument &) { caught = true; }
    check(caught, "Invalid decal geometry input accepted");
}
struct Reader {
    std::ifstream in;
    explicit Reader(const char *path) : in(path, std::ios::binary) {
        check(bool(in), "Cannot open decal geometry oracle");
        char magic[8]{}; check(bool(in.read(magic, 8)), "Truncated geometry signature");
        check(std::string_view(magic, 8) == "C5DCG001", "Wrong decal geometry oracle");
    }
    std::uint32_t word(unsigned size = 4) {
        std::uint32_t v = 0;
        for (unsigned i = 0; i < size; ++i) {
            const auto c = in.get();
            check(c != std::char_traits<char>::eof(), "Truncated geometry oracle");
            v |= std::uint32_t(static_cast<unsigned char>(c)) << (i*8);
        }
        return v;
    }
    float real() { return std::bit_cast<float>(word()); }
    Bytes blob() {
        const auto size = word();
        check(size <= 2*1024*1024, "Excessive geometry oracle blob");
        Bytes b(size);
        if (size) check(bool(in.read(reinterpret_cast<char *>(b.data()), size)), "Truncated geometry blob");
        return b;
    }
};
void put(Bytes &b, std::uint32_t value, unsigned size = 4) {
    for (unsigned i = 0; i < size; ++i) b.push_back(static_cast<std::uint8_t>(value >> (i*8)));
}
Bytes vertices(const TerrainDecalGeometry &mesh) {
    Bytes b;
    for (auto v : mesh.vertices)
        for (float f : {v.x, v.y, v.u, v.v}) put(b, std::bit_cast<std::uint32_t>(f));
    return b;
}
Bytes indices(const TerrainDecalGeometry &mesh) {
    Bytes b;
    for (auto i : mesh.indices) put(b, i, 2);
    return b;
}
TerrainDecalPalette palette(std::uint32_t first, std::uint32_t end) {
    TerrainDecalPalette result;
    result.ranges[0] = {first, end};
    for (std::uint32_t i = 0; i < std::max(end, first+1); ++i)
        result.records.push_back({float(i)*.1f, i+100, i+200, i+300});
    return result;
}
void guards() {
    std::vector<std::uint8_t> flags(64*64, 0x10);
    const TerrainDecalMapView map{64, 64, flags, 0, 0};
    const TerrainDecalCellGeometry line{{{1, 2}, {4, 5}, {7, 4}}};
    const auto mesh = build_terrain_decal_geometry(line, map);
    check(mesh.vertices.size() == 6 && mesh.indices.size() == 12, "Strip topology");
    check(build_terrain_decal_geometry({}, map).vertices.empty(), "Empty cell created geometry");
    rejects([&] { build_terrain_decal_geometry(line, {-1, 64, flags, 0, 0}); });
    rejects([&] { build_terrain_decal_geometry(line, {64, 63, flags, 0, 0}); });
    rejects([&] { build_terrain_decal_geometry(line, {64, 64, flags, std::numeric_limits<float>::infinity(), 0}); });
    rejects([&] { build_terrain_decal_geometry(TerrainDecalCellGeometry{{}}, map); });
    rejects([&] { build_terrain_decal_geometry(TerrainDecalCellGeometry{{{1, 1}, {1, 1}}}, map); });
    rejects([&] { build_terrain_decal_geometry(TerrainDecalCellGeometry{{{1, 1}, {2, 1}, {1, 1}}}, map); });
    auto invalid = line;
    invalid[0][1].x = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { build_terrain_decal_geometry(invalid, map); });
    // A degenerate line rejected by the map is never normalized.
    check(build_terrain_decal_geometry(TerrainDecalCellGeometry{{{1000, 1}, {1000, 1}}}, map).vertices.empty(),
        "Rejected line was normalized");
    TerrainDecalCellGeometry large;
    large.emplace_back();
    for (unsigned i = 0; i < 10924; ++i) large[0].emplace_back(1.f+float(i)*.005f, 20);
    rejects([&] { build_terrain_decal_geometry(large, map); });
    TerrainRandom random(1);
    auto library = palette(0, 3);
    std::vector<TerrainDecalCell> cells{{{11, 22, 33, 44}, 9, 255, {3, 5, 7}}};
    const std::vector<TerrainDecalCellGeometry> geometry{line};
    unsigned removed = 0, submitted = 0;
    const TerrainDecalRemove remove = [&](std::size_t i, std::uint32_t handle) {
        check(i == 0 && handle == 9 && cells[0].handle == 9 && cells[0].dirty == 255, "Removal state");
        ++removed;
    };
    const TerrainDecalSubmit submit = [&](std::size_t i, std::uint32_t selected,
                                         const TerrainDecalRecord &record, const TerrainDecalGeometry &packet) {
        check(i == 0 && &record == &library.records[selected] && vertices(packet) == vertices(mesh), "Submission payload");
        check(cells[0].dirty == 255 && cells[0].handle == absent_terrain_decal, "Submission cell state");
        ++submitted;
        return 31u;
    };
    rejects([&] { refresh_terrain_decal_geometry(1, 1, cells, {}, map, library, random, remove, submit); });
    rejects([&] { refresh_terrain_decal_geometry(1, 1, cells, geometry, map, library, random, remove, {}); });
    auto bad_palette = library; bad_palette.ranges[0].end = 4;
    rejects([&] { refresh_terrain_decal_geometry(1, 1, cells, geometry, map, bad_palette, random, remove, submit); });
    check(cells[0].handle == 9 && removed == 0 && submitted == 0 && random.calls() == 0,
          "Invalid refresh changed state");
    bool threw = false;
    try {
        refresh_terrain_decal_geometry(1, 1, cells, geometry, map, library, random, remove,
            [](auto, auto, const auto &, const auto &) -> std::uint32_t { throw std::runtime_error("submit"); });
    } catch (const std::runtime_error &) { threw = true; }
    check(threw && removed == 1 && cells[0].handle == absent_terrain_decal && cells[0].dirty == 255 &&
        random.calls() == 1, "Failed submit lost pending state or rewound selection");
    refresh_terrain_decal_geometry(1, 1, cells, geometry, map, library, random, remove, submit);
    check(removed == 1 && submitted == 1 && random.calls() == 2 && cells[0].handle == 31 && !cells[0].dirty,
          "Retry did not complete");
    refresh_terrain_decal_geometry(1, 1, cells, geometry, map, library, random, remove, submit);
    check(removed == 1 && submitted == 1 && random.calls() == 2, "Clean refresh had effects");
}
void oracle(const char *path) {
    Reader r(path);
    const auto cases = r.word(); check(cases <= 1000, "Excessive geometry cases");
    for (unsigned c = 0; c < cases; ++c) {
        try {
            const auto width = r.word(), height = r.word(), mw = r.word(), mh = r.word();
            const auto ox = r.real(), oy = r.real();
            const auto flags = r.blob();
            const auto first = r.word(), end = r.word(), seed = r.word();
            check(std::uint64_t(width)*height <= 10000 && end <= 1000, "Excessive grid/palette");
            const auto library = palette(first, end);
            TerrainRandom random(seed);
            std::vector<TerrainDecalCell> cells;
            std::vector<TerrainDecalCellGeometry> geometry(width*height);
            for (std::size_t i = 0; i < geometry.size(); ++i) {
                const auto handle = r.word(), dirty = r.word(), lines = r.word();
                check(dirty <= 255 && lines <= 20000, "Invalid cell input");
                cells.push_back({{11, 22, 33, 44}, handle, static_cast<std::uint8_t>(dirty), {3, 5, 7}});
                for (unsigned j = 0; j < lines; ++j) {
                    const auto count = r.word(); check(count >= 1 && count <= 10923, "Invalid point count");
                    auto &line = geometry[i].emplace_back();
                    for (unsigned k = 0; k < count; ++k) {
                        const auto x = r.real(), y = r.real(); line.emplace_back(x, y);
                    }
                }
            }
            const auto state = r.word(), calls = r.word(), removals = r.word();
            std::vector<std::uint32_t> expected_removed;
            for (unsigned i = 0; i < removals; ++i) expected_removed.push_back(r.word());
            const auto packets = r.word();
            unsigned removed = 0, submitted = 0;
            const TerrainDecalMapView map{static_cast<std::int32_t>(mw), static_cast<std::int32_t>(mh), flags, ox, oy};
            const auto refresh = [&] {
                refresh_terrain_decal_geometry(static_cast<std::int32_t>(width), static_cast<std::int32_t>(height),
                    cells, geometry, map, library, random,
                    [&](std::size_t i, std::uint32_t handle) {
                        check(removed < expected_removed.size() && handle == expected_removed[removed++], "Original removal order");
                        check(cells[i].dirty && cells[i].handle == handle, "Removal callback state");
                    },
                    [&](std::size_t i, std::uint32_t selected, const TerrainDecalRecord &record, const TerrainDecalGeometry &mesh) {
                        check(submitted++ < packets, "Unexpected submission");
                        check(i == r.word() && selected == r.word(), "Original submission/selection order");
                        check(random.state() == r.word() && random.calls() == r.word(), "Original submission cursor");
                        const auto handle = r.word();
                        const auto expected_vertices = r.blob(), expected_indices = r.blob();
                        check(vertices(mesh) == expected_vertices, "Original vertex bytes differ");
                        check(indices(mesh) == expected_indices, "Original index bytes differ");
                        check(&record == &library.records[selected] && record.specular == selected+300, "Selected palette record");
                        check(cells[i].dirty && cells[i].handle == absent_terrain_decal, "Original submission cell state");
                        vertex_bytes += expected_vertices.size(); index_bytes += expected_indices.size(); ++submissions;
                        return handle;
                    });
            };
            refresh();
            check(removed == removals && submitted == packets && random.state() == state && random.calls() == calls,
                  "Original refresh totals/cursor");
            for (const auto &cell : cells) {
                check(cell.handle == r.word() && cell.dirty == r.word(1), "Original final cell state");
                check(cell.geometry == std::array<std::uint32_t, 4>{11, 22, 33, 44} &&
                    cell.reserved == std::array<std::uint8_t, 3>{3, 5, 7}, "Opaque cell data overwritten");
            }
            refresh();
            check(removed == removals && submitted == packets && random.state() == state && random.calls() == calls,
                  "Second refresh was not inert");
        } catch (const std::exception &e) {
            throw std::runtime_error("Case "+std::to_string(c)+": "+e.what());
        }
    }
    check(r.in.peek() == std::char_traits<char>::eof(), "Trailing geometry oracle bytes");
    std::cout << cases << " original cases, ";
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc > 2) throw std::runtime_error("Usage: civ5_terrain_decal_geometry_tests [oracle.bin]");
        guards();
        if (argc == 2) oracle(argv[1]);
        std::cout << "PASS decal geometry: " << checks << " checks, " << vertex_bytes << " vertex bytes, "
                  << index_bytes << " index bytes, " << submissions << " submissions\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL decal geometry: " << e.what() << '\n'; return 1;
    }
}
