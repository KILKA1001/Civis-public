#include "civ5/materials/terrain_decal_packet.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace civ5::materials;
using namespace civ5::terrain;
using Bytes = std::vector<std::uint8_t>;
namespace {
std::size_t checks = 0, packets = 0, compared_bytes = 0;
void check(bool ok, const char *message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F f) {
    bool caught = false;
    try { f(); } catch (const std::invalid_argument &) { caught = true; }
    check(caught, "Invalid decal packet input accepted");
}
void put(Bytes &b, std::uint32_t value, unsigned size = 4) {
    for (unsigned i = 0; i < size; ++i) b.push_back(static_cast<std::uint8_t>(value >> (i*8)));
}
std::uint32_t word(const Bytes &b, std::size_t at, unsigned size = 4) {
    std::uint32_t value = 0;
    for (unsigned i = 0; i < size; ++i) value |= std::uint32_t(b.at(at+i)) << (i*8);
    return value;
}
Bytes packed(const TerrainDecalPackedGeometry &p) {
    Bytes b;
    for (auto v : p.vertices) for (auto h : v) put(b, h, 2);
    return b;
}
struct Reader {
    std::ifstream in;
    explicit Reader(const char *path) : in(path, std::ios::binary) {
        check(bool(in), "Cannot open decal packet oracle");
        char magic[8]{}; check(bool(in.read(magic, 8)), "Truncated packet signature");
        check(std::string_view(magic, 8) == "C5DCP001", "Wrong decal packet oracle");
    }
    std::uint32_t integer() {
        std::uint32_t v = 0;
        for (unsigned i = 0; i < 4; ++i) {
            const auto c = in.get();
            check(c != std::char_traits<char>::eof(), "Truncated packet oracle");
            v |= std::uint32_t(static_cast<unsigned char>(c)) << (i*8);
        }
        return v;
    }
    float real() { return std::bit_cast<float>(integer()); }
    Bytes blob() {
        const auto size = integer(); check(size <= 2*1024*1024, "Excessive packet oracle blob");
        Bytes b(size);
        if (size) check(bool(in.read(reinterpret_cast<char *>(b.data()), size)), "Truncated packet blob");
        return b;
    }
};
void guards() {
    const std::array<float, 4> domain{-10, -10, 10, 10};
    const TerrainDecalGeometry mesh{{{0, 0, 0, 0}, {2, 0, 1, 0}, {0, 2, 0, 1}}, {0, 1, 2}};
    const auto baseline = pack_terrain_decal_geometry(mesh, domain);
    check(baseline && baseline->vertices.size() == 3 && !baseline->large, "Triangle packet");
    check(!pack_terrain_decal_geometry({}, {}), "Zero triangles read invalid domain");
    auto bad = mesh; bad.indices.pop_back();
    rejects([&] { pack_terrain_decal_geometry(bad, domain); });
    bad = mesh; bad.indices[2] = 3;
    rejects([&] { pack_terrain_decal_geometry(bad, domain); });
    bad = mesh; bad.vertices.clear();
    rejects([&] { pack_terrain_decal_geometry(bad, domain); });
    bad = mesh; bad.vertices.resize(65537);
    rejects([&] { pack_terrain_decal_geometry(bad, domain); });
    bad = mesh; bad.indices.resize(65535, 0);
    check(!pack_terrain_decal_geometry(bad, domain), "Excessive triangle count accepted");
    const float inf = std::numeric_limits<float>::infinity(), nan = std::numeric_limits<float>::quiet_NaN();
    for (auto bounds : {std::array<float, 4>{0, 0, 0, 1}, {2, 0, 1, 2}, {0, 0, inf, 2}, {0, nan, 2, 2}, {3, 3, 4, 4}})
        rejects([&] { pack_terrain_decal_geometry(mesh, bounds); });
    for (float value : {inf, nan}) {
        bad = mesh; bad.vertices.push_back({value, 1, 0, 0});
        rejects([&] { pack_terrain_decal_geometry(bad, domain); });
    }
    bad = mesh; for (auto &v : bad.vertices) v.x = 0;
    rejects([&] { pack_terrain_decal_geometry(bad, domain); });
    bad = mesh; bad.vertices[1].x = std::numeric_limits<float>::denorm_min();
    rejects([&] { pack_terrain_decal_geometry(bad, domain); });
    bad = mesh; bad.vertices[0].x = -std::numeric_limits<float>::max(); bad.vertices[1].x = std::numeric_limits<float>::max();
    rejects([&] { pack_terrain_decal_geometry(bad, {-std::numeric_limits<float>::max(), -10, std::numeric_limits<float>::max(), 10}); });
    // Packet owns expanded data even after its source arrays are destroyed.
    auto owned = [&] { auto temporary = mesh; return pack_terrain_decal_geometry(temporary, domain); }();
    check(packed(*owned) == packed(*baseline), "Packet did not retain its geometry");
}
void integration() {
    std::optional<TerrainDecalPackedGeometry> output;
    unsigned submits = 0;
    {
        std::vector<std::uint8_t> flags(64*64, 0x10);
        const TerrainDecalMapView map{64, 64, flags, 0, 0};
        TerrainDecalPalette palette; palette.ranges[0] = {0, 1}; palette.records.push_back({0, 13, 29, 37});
        const std::vector<TerrainDecalCellGeometry> lines{{{{1, 2}, {4, 5}, {7, 4}}}};
        std::vector<TerrainDecalCell> cells(1);
        cells[0].handle = absent_terrain_decal; cells[0].dirty = 1;
        TerrainRandom random(1);
        const TerrainDecalSubmit submit = [&](auto, auto selected, const auto &record, const auto &geometry) {
            check(selected == 0 && record.heightmap == 29, "Packing received wrong material");
            output = pack_terrain_decal_geometry(geometry, {-64, -64, 64, 64});
            check(output && output->vertices.size() == 12, "Strip was not expanded to triangles");
            ++submits;
            return 19u;
        };
        const TerrainDecalRemove remove = [](auto, auto) { throw std::runtime_error("Unexpected initial removal"); };
        refresh_terrain_decal_geometry(1, 1, cells, lines, map, palette, random, remove, submit);
        refresh_terrain_decal_geometry(1, 1, cells, lines, map, palette, random, remove, submit);
        check(submits == 1 && cells[0].handle == 19 && !cells[0].dirty && random.calls() == 0,
              "Packet submission disturbed clean-cell/selector behavior");
    }
    check(output && packed(*output).size() == 96, "Borrowed strip packet lost after refresh");
}
void oracle(const char *path) {
    Reader r(path); const auto count = r.integer(); check(count <= 1000, "Excessive packet cases");
    for (unsigned c = 0; c < count; ++c) {
        try {
            std::array<float, 4> domain{}; for (auto &v : domain) v = r.real();
            const auto source = r.blob(), ids = r.blob();
            check(source.size()%16 == 0 && ids.size()%6 == 0, "Malformed source geometry");
            TerrainDecalGeometry mesh;
            for (std::size_t i = 0; i < source.size(); i += 16)
                mesh.vertices.push_back({std::bit_cast<float>(word(source, i)), std::bit_cast<float>(word(source, i+4)),
                    std::bit_cast<float>(word(source, i+8)), std::bit_cast<float>(word(source, i+12))});
            for (std::size_t i = 0; i < ids.size(); i += 2) mesh.indices.push_back(static_cast<std::uint16_t>(word(ids, i, 2)));
            const auto result = pack_terrain_decal_geometry(mesh, domain);
            const auto admitted = r.integer(); check(admitted <= 1 && bool(result) == bool(admitted), "Admission differs from original");
            if (!admitted) continue;
            for (auto v : result->bounds) check(std::bit_cast<std::uint32_t>(v) == r.integer(), "Clipped AABB differs from original");
            check(result->large == bool(r.integer()), "Large-record classification differs from original");
            const auto expected = r.blob(); const auto actual = packed(*result);
            if (actual != expected) {
                std::size_t i = 0; while (i < actual.size() && i < expected.size() && actual[i] == expected[i]) ++i;
                std::cerr << " first differing packed byte=" << i << '\n';
            }
            check(actual == expected, "Packed half vertices differ from original");
            compared_bytes += expected.size()+16; ++packets;
        } catch (const std::exception &e) { throw std::runtime_error("Case "+std::to_string(c)+": "+e.what()); }
    }
    check(r.in.peek() == std::char_traits<char>::eof(), "Trailing packet oracle bytes");
}
} // namespace
int main(int argc, char **argv) {
    try {
        guards(); integration();
        for (int i = 1; i < argc; ++i) oracle(argv[i]);
        std::cout << "PASS terrain decal packets checks=" << checks << " packets=" << packets << " originalBytes=" << compared_bytes << '\n';
        return 0;
    } catch (const std::exception &e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
