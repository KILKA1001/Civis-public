#include "civ5/terrain/terrain_marsh.hpp"
#include "civ5/terrain/height_owner.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace civ5::terrain;
using Bytes = civ5::assets::Bytes;
namespace {
std::size_t checks = 0, compared_bytes = 0, stores = 0;
void check(bool ok, const char *message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
template <class F> void rejects(F f) {
    bool caught = false;
    try { f(); } catch (const std::exception &) { caught = true; }
    check(caught, "Invalid marsh input accepted");
}
void word(Bytes &b, std::uint32_t v, unsigned size = 4) {
    for (unsigned i = 0; i < size; ++i) b.push_back(static_cast<std::uint8_t>(v >> (8*i)));
}
void source(Bytes &b, const TerrainFeature2Source &s) {
    for (auto v : {s.position.x, s.position.y, s.position.z, s.terrain, s.continent})
        word(b, std::bit_cast<std::uint32_t>(v));
}
Bytes sources(std::span<const TerrainFeature2Source> records) {
    Bytes b; for (const auto &s : records) source(b, s); return b;
}
struct Reader {
    Bytes bytes;
    std::size_t at = 0;
    std::uint32_t word(unsigned size = 4) {
        check(at+size <= bytes.size(), "Truncated marsh oracle");
        std::uint32_t v = 0;
        for (unsigned i = 0; i < size; ++i) v |= std::uint32_t(bytes[at++]) << (8*i);
        return v;
    }
    std::int32_t integer() { return std::bit_cast<std::int32_t>(word()); }
    Bytes blob() {
        const auto size = word(); check(size <= bytes.size()-at, "Oversized marsh blob");
        Bytes b(bytes.begin()+at, bytes.begin()+at+size); at += size; return b;
    }
    std::vector<TerrainFeature2Source> records() {
        Reader r{blob()}; check(r.bytes.size()%20 == 0, "Misaligned marsh source stream");
        std::vector<TerrainFeature2Source> out;
        while (r.at < r.bytes.size())
            out.push_back({{r.integer(), r.integer(), r.integer()}, r.integer(), r.integer()});
        return out;
    }
};
void equal_bytes(std::span<const std::uint8_t> a, std::span<const std::uint8_t> b, const char *message) {
    check(std::equal(a.begin(), a.end(), b.begin(), b.end()), message); compared_bytes += a.size();
}
Bytes dds() {
    Bytes result(128+16384, 0);
    const auto set = [&](unsigned offset, unsigned value) {
        for (unsigned i = 0; i < 4; ++i) result[offset+i] = static_cast<std::uint8_t>(value >> (i*8));
    };
    set(0, 0x20534444); set(4, 124); set(8, 0x1007); set(12, 128); set(16, 128);
    set(76, 32); set(80, 0x20000); set(88, 8); set(92, 255); set(108, 0x1000);
    std::fill(result.begin()+128, result.end(), 1); return result;
}
void guards() {
    TerrainMarshStamp::Storage prior{}; prior.fill(77);
    const auto stamp = TerrainMarshStamp::prepare(civ5::assets::DdsData::parse(dds()), prior);
    check(stamp.storage()[0] == 77 && stamp.storage()[1] == 1 && stamp.storage()[16384] == 1 &&
          stamp.storage()[16385] == 77, "Mask copy offset/tail");
    auto wrong = dds(); wrong[12] = 64; wrong.resize(128+128*64);
    rejects([&] { TerrainMarshStamp::prepare(civ5::assets::DdsData::parse(wrong), prior); });
    wrong = dds(); wrong[92] = 127;
    rejects([&] { TerrainMarshStamp::prepare(civ5::assets::DdsData::parse(wrong), prior); });
    const HeightDomain domain(16, 16);
    const std::vector<std::uint16_t> heights(256, 15000);
    const Bytes materials(256, 31);
    const std::array<TerrainFeature2Source, 1> input{{{{64, 64, 0}, 2, 0}}};
    std::vector<std::uint32_t> palette{0x100000};
    TerrainMarshSettings settings{identity_matrix(), 0, 16, 16, X87Precision::double_precision, Vec4Path::sse, palette};
    TerrainMarshState state;
    const auto queued = apply_terrain_marshes(domain, heights, materials, state, input, false, stamp, settings);
    check(!queued.state.enabled && queued.state.pending.size() == 1 && queued.applications.empty(), "Inactive queue");
    equal_bytes(queued.materials, materials, "Queue changed materials");
    const auto flushed = apply_terrain_marshes(domain, heights, materials, queued.state, {}, true, stamp, settings);
    check(flushed.state.enabled && flushed.state.pending.empty() && flushed.state.retained.size() == 1,
          "Pending flush state");
    check(queued.state.pending.size() == 1, "Flush mutated previous owner");
    rejects([&] { apply_terrain_marshes(domain, heights, {}, state, input, true, stamp, settings); });
    auto bad = input; bad[0].continent = -1;
    rejects([&] { apply_terrain_marshes(domain, heights, materials, state, bad, true, stamp, settings); });
    bad[0].continent = 5;
    rejects([&] { apply_terrain_marshes(domain, heights, materials, state, bad, true, stamp, settings); });
    const auto ignored = apply_terrain_marshes(domain, heights, materials, {}, bad, false, stamp, {});
    check(ignored.state.pending.size() == 1, "Inactive queue inspected uninitialized palette/matrix");
    palette.resize(257); settings.material_codes = palette;
    rejects([&] { apply_terrain_marshes(domain, heights, materials, state, input, true, stamp, settings); });
    settings.material_codes = {};
    rejects([&] { apply_terrain_marshes(domain, heights, materials, queued.state, {}, true, stamp, settings,
        [](auto, auto, auto) { throw std::runtime_error("observer"); }); });
    check(queued.state.pending.size() == 1 && materials.front() == 31, "Rejected pass consumed prior inputs");
}
void native(const char *filename) {
    std::ifstream file(filename, std::ios::binary);
    check(bool(file), "Missing marsh oracle");
    Reader r{Bytes(std::istreambuf_iterator<char>(file), {})};
    check(r.bytes.size() >= 12 && std::string(r.bytes.begin(), r.bytes.begin()+8) == "C5MAR002", "Wrong marsh corpus");
    r.at = 8; const auto count = r.word();
    for (unsigned i = 0; i < count; ++i) {
        try {
            const auto w = r.integer(), h = r.integer(), rw = r.integer(), rh = r.integer(), mode = r.integer();
            TerrainMarshState state; state.enabled = r.word() != 0; const bool activate = r.word() != 0;
            const auto pc = r.word(), path = r.word();
            check((pc == 24 || pc == 53) && path <= 1, "Invalid marsh arithmetic profile");
            Matrix4 matrix; for (auto &v : matrix) v = std::bit_cast<float>(r.word());
            const auto image = civ5::assets::DdsData::parse(r.blob());
            const auto prior_bytes = r.blob(), expected_stamp = r.blob();
            check(prior_bytes.size() == TerrainMarshStamp::storage_bytes, "Invalid prior stamp size");
            TerrainMarshStamp::Storage prior; std::copy(prior_bytes.begin(), prior_bytes.end(), prior.begin());
            const auto stamp = TerrainMarshStamp::prepare(image, prior);
            equal_bytes(stamp.storage(), expected_stamp, "Original marsh mask differs");
            Reader codes{r.blob()}; std::vector<std::uint32_t> palette;
            while (codes.at < codes.bytes.size()) palette.push_back(codes.word());
            state.pending = r.records(); const auto input = r.records();
            Reader hs{r.blob()}; std::vector<std::uint16_t> heights;
            while (hs.at < hs.bytes.size()) heights.push_back(static_cast<std::uint16_t>(hs.word(2)));
            const auto materials = r.blob(); Bytes writes;
            const auto result = apply_terrain_marshes(HeightDomain(w, h), heights, materials, state, input, activate,
                stamp, {matrix, mode, rw, rh, pc == 24 ? X87Precision::single : X87Precision::double_precision,
                        path ? Vec4Path::sse : Vec4Path::legacy_x87, palette},
                [&](std::size_t application, std::int32_t index, std::uint8_t value) {
                    word(writes, static_cast<std::uint32_t>(application)); word(writes, index); word(writes, value, 1);
                });
            check(result.state.enabled == (r.word() != 0), "Original enable flag differs");
            equal_bytes(sources(result.state.pending), r.blob(), "Original pending stream differs");
            equal_bytes(sources(result.state.retained), r.blob(), "Original retained list differs");
            Bytes applications;
            for (const auto &a : result.applications) {
                word(applications, a.origin_x); word(applications, a.origin_y); word(applications, a.code);
                for (auto v : {a.dirty_lower.x, a.dirty_lower.y, a.dirty_upper.x, a.dirty_upper.y})
                    word(applications, std::bit_cast<std::uint32_t>(v));
            }
            equal_bytes(applications, r.blob(), "Original rectangles/invalidation arguments differ");
            equal_bytes(writes, r.blob(), "Original ordered stores differ"); stores += writes.size()/9;
            equal_bytes(result.materials, r.blob(), "Original material field differs");
            // Publish through the same owned page API used by TerrainWorld.
            auto pages = PageStorage::initialize(w, h, 1, nullptr, [](int n) { return Bytes(n, 0x59); });
            const auto headers = pages.header_bytes();
            pages = pages.with_dense_values(result.materials);
            equal_bytes(headers, pages.header_bytes(), "Marsh publication changed page headers");
            for (int y = 0; y < h; ++y)
                for (int x = 0; x < w; ++x)
                    check(pages.value_at(x, y) == result.materials[y*w+x], "Published marsh page differs");
        } catch (const std::exception &e) {
            throw std::runtime_error("Marsh case " + std::to_string(i) + ": " + e.what());
        }
    }
    check(r.at == r.bytes.size(), "Trailing marsh corpus bytes");
    std::cout << "PASS " << count << " original marsh cases, " << stores << " ordered stores, "
              << compared_bytes << " compared bytes\n";
}
} // namespace
int main(int argc, char **argv) {
    try {
        guards(); if (argc == 2) native(argv[1]); else check(argc == 1, "Usage: terrain_marsh_tests [oracle.bin]");
        std::cout << "PASS marsh " << checks << " checks\n";
    } catch (const std::exception &e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
