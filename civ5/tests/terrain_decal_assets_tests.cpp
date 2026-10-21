#include "civ5/terrain/terrain_decal_geometry.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace civ5::terrain;
namespace assets = civ5::assets;
namespace fs = std::filesystem;
using Bytes = assets::Bytes;
namespace {
std::size_t checks = 0, palette_bytes = 0, texture_bytes = 0;
void check(bool ok, const char *message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F f) {
    bool caught = false;
    try { f(); } catch (const std::exception &) { caught = true; }
    check(caught, "Invalid decal asset input accepted");
}
struct Reader {
    Bytes data;
    std::size_t at = 0;
    Bytes take(std::size_t size) {
        check(size <= data.size()-at, "Truncated decal asset oracle");
        Bytes result(data.begin()+at, data.begin()+at+size); at += size; return result;
    }
    std::uint32_t word() {
        const auto b = take(4);
        return std::uint32_t(b[0]) | (std::uint32_t(b[1]) << 8) |
               (std::uint32_t(b[2]) << 16) | (std::uint32_t(b[3]) << 24);
    }
    Bytes blob() { return take(word()); }
    std::string text() { const auto b = blob(); return {b.begin(), b.end()}; }
};
void word(Bytes &bytes, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) bytes.push_back(static_cast<std::uint8_t>(value >> (8*i)));
}
TerrainDecalPalette palette(Reader &r) {
    TerrainDecalPalette result;
    for (auto &range : result.ranges) range = {r.word(), r.word()};
    Reader records{r.blob()};
    check(records.data.size()%16 == 0 && records.data.size() <= 4096*16, "Invalid palette extent");
    while (records.at < records.data.size())
        result.records.push_back({std::bit_cast<float>(records.word()), records.word(), records.word(), records.word()});
    return result;
}
Bytes bytes(const TerrainDecalPalette &palette) {
    Bytes result;
    for (const auto &r : palette.ranges) { word(result, r.first); word(result, r.end); }
    word(result, static_cast<std::uint32_t>(palette.records.size()*16));
    for (const auto &r : palette.records)
        for (auto v : {std::bit_cast<std::uint32_t>(r.priority), r.albedo, r.heightmap, r.specular}) word(result, v);
    return result;
}
void guards() {
    const std::string valid = "<DecalTypes><Riverbank><any spec='x&amp;y.dds' priority='-0'/></Riverbank>"
                              "<Detail_Road><one priority='+0.6'/><two/></Detail_Road></DecalTypes>";
    auto doc = parse_terrain_decal_definition(valid);
    check(doc.families[0]->size() == 2 && !doc.families[1] && doc.families[7]->size() == 1,
          "Definition family slots differ");
    check(doc.families[7]->front().specular == "x&y.dds", "XML entity decoding");
    check(std::bit_cast<std::uint32_t>(doc.families[7]->front().priority) == 0x80000000,
          "Signed zero priority was lost");
    for (const auto &bad : {"<wrong/>", "<DecalTypes><Detail_Road/></DecalTypes>",
        "<DecalTypes><Detail_Road><a/></Detail_Road><Detail_Road><b/></Detail_Road></DecalTypes>",
        "<DecalTypes><Detail_Road><a albedo='../a.dds'/></Detail_Road></DecalTypes>",
        "<DecalTypes><Detail_Road><a priority='nan'/></Detail_Road></DecalTypes>",
        "<DecalTypes><Detail_Road><a priority='+-1'/></Detail_Road></DecalTypes>",
        "<DecalTypes><Detail_Road><a priority='1e100'/></Detail_Road></DecalTypes>",
        "<DecalTypes><Detail_Road><a priority=''/></Detail_Road></DecalTypes>",
        "<DecalTypes><Detail_Road><a spec=''/></Detail_Road></DecalTypes>",
        "<DecalTypes><Detail_Road><a priority='1'priority='2'/></Detail_Road></DecalTypes>",
        "<!DOCTYPE DecalTypes><DecalTypes/>", "<DecalTypes><Detail_Road><a><b/></a></Detail_Road></DecalTypes>"})
        rejects([&] { parse_terrain_decal_definition(bad); });
    const auto unknown = parse_terrain_decal_definition("<DecalTypes><Future/></DecalTypes>");
    check(std::all_of(unknown.families.begin(), unknown.families.end(), [](const auto &f) { return !f; }),
          "Unknown family was admitted");
    unsigned requests = 0;
    const TerrainDecalResource resolve = [&](auto) { ++requests; return 4; };
    auto p = append_terrain_decal_definition({}, doc, 7, resolve);
    check(requests == 1 && p.ranges[0].end == 2 && p.ranges[7].first == 2 &&
          p.records[0].albedo == 0 && p.records[0].heightmap == 0 && p.records[0].specular == 7 &&
          p.records[2].specular == 4, "Default textures or family enumeration differ");
    const auto before = bytes(p);
    rejects([&] { append_terrain_decal_definition(p, doc, 7, {}); });
    auto invalid = p; invalid.ranges[0].end = 99;
    rejects([&] { append_terrain_decal_definition(invalid, doc, 7, resolve); });
    auto bad = doc; bad.families[0]->front().priority = std::numeric_limits<float>::infinity();
    rejects([&] { append_terrain_decal_definition(p, bad, 7, resolve); });
    bad = doc; bad.families[0]->clear();
    rejects([&] { append_terrain_decal_definition(p, bad, 7, resolve); });
    invalid = p; invalid.records.resize(4096);
    rejects([&] { append_terrain_decal_definition(invalid, doc, 7, resolve); });
    check(requests == 1 && bytes(p) == before, "Rejected palette changed prior resources/state");
    rejects([&] {
        append_terrain_decal_definition(p, doc, 7, [](auto) -> std::uint32_t { throw std::runtime_error("resource"); });
    });
    check(bytes(p) == before, "Failed resource binding changed the prior palette");
}
std::shared_ptr<assets::FpkSource> open(const assets::AssetEnvironment &env, const char *path) {
    auto result = env.open_fpk_archive(path);
    check(std::holds_alternative<std::shared_ptr<assets::FpkSource>>(result), "Missing decal asset archive");
    return std::get<std::shared_ptr<assets::FpkSource>>(std::move(result));
}
// Explicit aliases for this controlled original archive set. This adapter does
// not claim to reconstruct game/mod VFS precedence or enable model rendering.
struct AuthoredTextures final : assets::AssetSource {
    std::shared_ptr<assets::FpkSource> decals, upgrade;
    mutable unsigned requests = 0;
    assets::AssetLookup find(const assets::AssetKey &key) const override {
        ++requests;
        auto name = key.normalized_name();
        if (name == "blank_texture_spec_decal.dds") return upgrade->find(key);
        check(name.starts_with("assets/decals/") && key.safe_relative_path(), "Unknown decal VFS alias");
        return decals->find(assets::AssetKey(name.substr(name.find_last_of('/')+1)));
    }
    assets::Provenance provenance() const override { return {assets::SourceKind::fpk, "explicit-decal-test-aliases", "", {}}; }
};
struct Expected { std::string xml; TerrainDecalPalette palette; };
Expected oracle(const char *path) {
    std::ifstream file(path, std::ios::binary);
    check(bool(file), "Missing decal asset oracle");
    Reader r{Bytes(std::istreambuf_iterator<char>(file), {})};
    check(r.take(8) == Bytes({'C','5','D','A','S','0','0','1'}), "Invalid decal asset oracle");
    const auto count = r.word();
    check(count > 0 && count <= 100, "Invalid decal asset case count");
    Expected first;
    for (unsigned i = 0; i < count; ++i) {
        auto xml = r.text();
        const auto prior = palette(r);
        const auto blank = r.word();
        const auto expected = palette(r);
        const auto load_count = r.word();
        check(load_count <= 4096*3, "Invalid resource request count");
        std::vector<std::pair<std::string, std::uint32_t>> loads;
        for (unsigned j = 0; j < load_count; ++j) {
            auto name = r.text(); const auto id = r.word(); loads.emplace_back(std::move(name), id);
        }
        std::size_t at = 0;
        const auto result = append_terrain_decal_definition(prior, parse_terrain_decal_definition(xml), blank,
            [&](std::string_view name) {
                check(at < loads.size() && name == loads[at].first, "Original texture request order differs");
                return loads[at++].second;
            });
        check(at == loads.size() && bytes(result) == bytes(expected), "Original decal palette differs");
        palette_bytes += expected.records.size()*16+80;
        if (i == 0) first = {std::move(xml), result};
    }
    check(r.at == r.data.size(), "Trailing decal asset oracle bytes");
    std::cout << count << " original palette cases, ";
    return first;
}
void authored(const char *root, const Expected &expected) {
    auto loaded = [&] {
        assets::AssetEnvironment environment(root);
        auto definitions = open(environment, "Resource/Common/MiscData.fpk");
        auto found = definitions->find(assets::AssetKey("decal_filenames.xml"));
        auto *definition = std::get_if<assets::AssetHandle>(&found);
        check(definition != nullptr, "Missing authored decal definition");
        const auto raw = definition->read_bytes();
        check(std::string(raw.begin(), raw.end()) == expected.xml, "Authored XML differs from the native corpus");
        struct Unavailable final : assets::AssetSource {
            bool ambiguous = false;
            assets::AssetLookup find(const assets::AssetKey &key) const override {
                if (ambiguous) return assets::AmbiguousAsset{key, provenance(), {}};
                return {};
            }
            assets::Provenance provenance() const override { return {assets::SourceKind::loose, "test", "", {}}; }
        } unavailable;
        rejects([&] { TerrainDecalAssets::load(*definition, unavailable); });
        unavailable.ambiguous = true;
        rejects([&] { TerrainDecalAssets::load(*definition, unavailable); });
        AuthoredTextures source;
        source.decals = open(environment, "Resource/DX9/DecalTextures.fpk");
        source.upgrade = open(environment, "Resource/DX9/Upgrade1Textures.fpk");
        auto result = TerrainDecalAssets::load(*definition, source);
        check(source.requests == result.textures().size(), "Duplicate decal texture decode/lookup");
        return result;
    }();
    check(bytes(loaded.palette()) == bytes(expected.palette), "Authored palette resource bindings differ");
    check(loaded.textures().size() == 12 && loaded.blank_specular() == 1, "Authored texture inventory");
    for (const auto &texture : loaded.textures()) {
        check(texture.data.original_bytes() == texture.asset.read_bytes(), "Retained decal DDS bytes changed");
        check(texture.data.face_count() == 1 && !texture.data.levels().empty(), "Invalid authored decal DDS");
        texture_bytes += texture.data.original_bytes().size();
    }
    // Execute the existing selector from the actual library ranges. Original
    // execution independently verifies these four entering-state controls.
    for (auto seed : {0u, 1u, 0x80000000u, 0xffffffffu}) {
        TerrainRandom random(seed);
        for (unsigned i = 0; i < 16; ++i)
            check(select_terrain_decal(loaded.palette().ranges[0], random) == 0, "Authored detail-road selection");
        check(random.state() == seed && random.calls() == 0, "Authored family consumed randomness");
        std::vector<std::uint8_t> mask(16*16, 0x10);
        const std::vector<TerrainDecalCellGeometry> geometry{
            {{{1, 2}, {4, 5}, {7, 4}}}, {{{1, 2}, {400, 500}}}};
        std::vector<TerrainDecalCell> grid(2, {{1, 2, 3, 4}, absent_terrain_decal, 255, {5, 6, 7}});
        unsigned submissions = 0;
        refresh_terrain_decal_geometry(2, 1, grid, geometry, {16, 16, mask, 0, 0}, loaded.palette(), random,
            [](auto, auto) { throw std::runtime_error("Unexpected authored decal removal"); },
            [&](std::size_t i, std::uint32_t index, const TerrainDecalRecord &record, const TerrainDecalGeometry &mesh) {
                check(i == 0 && index == 0 && &record == &loaded.palette().records[0], "Authored geometry palette binding");
                check(record.albedo == 0 && record.heightmap == 0 && record.specular == loaded.blank_specular(),
                      "Authored geometry resource binding");
                check(mesh.vertices.size() == 6 && mesh.indices.size() == 12, "Authored geometry topology");
                ++submissions; return 99u;
            });
        check(submissions == 1 && grid[0].handle == 99 && grid[1].handle == absent_terrain_decal &&
              grid[0].dirty == 0 && grid[1].dirty == 0 && random.state() == seed && random.calls() == 0,
              "Authored geometry refresh state");
    }
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc > 3) throw std::runtime_error("Usage: civ5_terrain_decal_assets_tests [oracle.bin [asset-root]]");
        guards();
        if (argc >= 2) {
            const auto expected = oracle(argv[1]);
            if (argc == 3) authored(argv[2], expected);
        }
        std::cout << "PASS decal assets: " << checks << " checks, " << palette_bytes
                  << " palette bytes, " << texture_bytes << " original texture bytes\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL decal assets: " << e.what() << '\n'; return 1;
    }
}
