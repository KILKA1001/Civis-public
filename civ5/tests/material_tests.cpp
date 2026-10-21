#include "civ5/materials/material_assets.hpp"
#include "civ5/materials/material_mesh.hpp"
#include "civ5/materials/material_setup.hpp"
#include "civ5/terrain/land_grid.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace civ5::materials;
namespace {
int checks = 0;
void check(bool ok) {
    ++checks;
    if (!ok)
        throw std::runtime_error("Material contract test failed");
}
template <class F> void rejects(F f) {
    bool failed = false;
    try {
        f();
    } catch (const std::invalid_argument &) {
        failed = true;
    }
    check(failed);
}
template <class T> bool equal(std::span<const T> a, std::initializer_list<T> b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}
void expansion() {
    MaterialExpansion e(std::array{0, 1, 0, -7});
    check(equal(e.source_rows(), {0, 1, 1, 1, 2, 3, 3, 3}));
    check(equal(e.source_modes(), {0, 0, 1, 2, 0, 0, 1, 2}));
    check(equal(e.base_slots(), {0, 1, 4, 5}));
    for (int prefix = 61; prefix <= 63; ++prefix) {
        std::vector<std::int32_t> input(prefix + 1);
        input.back() = 1;
        MaterialExpansion out(input);
        check(out.source_rows().size() == 64);
        for (int i = prefix; i < 64; ++i) {
            check(out.source_rows()[i] == prefix);
            check(out.source_modes()[i] == i - prefix);
        }
    }
    std::array<std::int32_t, 64> modes{};
    modes.fill(1);
    MaterialExpansion all(modes);
    check(all.base_slots()[21] == 63 && all.base_slots()[22] == -1);
    modes.fill(0);
    modes[0] = 1;
    MaterialExpansion exact(modes);
    check(exact.base_slots()[62] == 64 && exact.base_slots()[63] == -1);
    modes.fill(0);
    check(MaterialExpansion(modes).base_slots()[63] == 63);
    MaterialExpansion slots(std::array{1, 0});
    const auto actual = slots.ordinary_slots(std::array{3, 2, 0}, std::array{0x1234ffffu, 0u},
                                             std::array{0x12340007u, 0x43210100u});
    check(actual == std::array<std::uint16_t, 16>{0, 65535, 65535, 0, 0, 0, 0, 0, 256, 7, 7, 0, 0, 0, 0, 0});
    check(slots.ordinary_slots({}, std::array{1u, 2u}, std::array{3u, 4u}) ==
          std::array<std::uint16_t, 16>{});
    rejects([] { MaterialExpansion none({}); });
    rejects([] { MaterialExpansion overflow(std::array<std::int32_t, 65>{}); });
    for (int invalid : {-1, 4, 96})
        rejects([&] { slots.ordinary_slots(std::array{invalid}, std::array{1u, 2u}, std::array{3u, 4u}); });
    rejects([&] { slots.ordinary_slots(std::array<int, 9>{}, std::array{1u, 2u}, std::array{3u, 4u}); });
    rejects([&] { slots.ordinary_slots({}, std::array{1u}, std::array{3u, 4u}); });
}
void constants() {
    auto bits = [](float scale, int mode, float span) {
        return std::bit_cast<std::uint32_t>(material_texture_multiplier(scale, mode, span));
    };
    check(bits(3, 0, 10) == 0x402e8ba3);
    check(bits(1.5f, 1, 10) == 0x3fae8ba3);
    for (int mode : {1, 2, 7, -1})
        check(bits(2.25f, mode, 32) == 0x400e6c9b);
    check(bits(1.01f, 0, 2070.375f) == 0x3f8144ff);
    check(bits(-1.5f, -1, 10) == 0xbfae8ba3);
    const float nan = std::numeric_limits<float>::quiet_NaN();
    for (float span :
         {0.f, -0.f, nan, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::denorm_min()})
        rejects([&] { material_texture_multiplier(1, 0, span); });
    for (float scale : {nan, std::numeric_limits<float>::infinity(), 1e20f, -1e20f})
        rejects([&] { material_texture_multiplier(scale, 0, 10); });
    check(std::isfinite(material_texture_multiplier(-1.5f, 1, -10)));
    std::array<float, 384> resident;
    for (std::uint32_t i = 0; i < resident.size(); ++i)
        resident[i] = std::bit_cast<float>(0x7fc12300u + i);
    const auto initial = std::bit_cast<std::array<std::uint32_t, 384>>(resident);
    write_material_constants({}, {}, std::array{1.f}, 10, resident);
    check(std::bit_cast<std::array<std::uint32_t, 384>>(resident) == initial);
    write_material_constants(std::array{1, 0}, std::array{1, 0}, std::array{3.f, 1.5f}, 10, resident);
    check(std::bit_cast<std::uint32_t>(resident[0]) == 0x3fae8ba3 && resident[1] == 1 && resident[2] == 0 &&
          resident[3] == 0);
    check(std::bit_cast<std::uint32_t>(resident[4]) == 0x402e8ba3 && resident[5] == 0);
    for (std::size_t i = 8; i < resident.size(); ++i)
        check(std::bit_cast<std::uint32_t>(resident[i]) == initial[i]);
    const auto before = std::bit_cast<std::array<std::uint32_t, 384>>(resident);
    rejects([&] {
        write_material_constants(std::array{0, 2}, std::array{0, 1}, std::array{3.f, 1.5f}, 10, resident);
    });
    rejects([&] {
        write_material_constants(std::array{0, 1}, std::array{0, 1}, std::array{3.f, 1e20f}, 10, resident);
    });
    rejects([&] { write_material_constants(std::array{0}, {}, std::array{3.f}, 10, resident); });
    rejects([&] { write_material_constants({}, {}, std::array{nan}, 10, resident); });
    rejects([&] { write_material_constants({}, {}, {}, 10, resident); });
    rejects([&] {
        write_material_constants(std::array<int, 65>{}, std::array<int, 65>{}, std::array{3.f}, 10, resident);
    });
    check(std::bit_cast<std::array<std::uint32_t, 384>>(resident) == before);
}
void rows() {
    const auto parsed = parse_material_rows("header\r\n0, a, h, NULL, "
                                            "0,0.1,0,30\r\n\r\n+4,a,h,s,1,+.3,-7,0Xffffffff\n");
    check(parsed.size() == 2 && parsed[0].local_id == 0x30 && parsed[1].local_id == 0xffffffff);
    check(!parsed[0].specular && parsed[1].continent == 4 && parsed[1].projection == -7);
    check(parsed[0].diffuse == parsed[1].diffuse && parsed[1].scale == .3f);
    for (auto csv :
         {"", "header", "header\n0,a,h,NULL,0,.1,0", "header\n0,a,h,NULL,0,.1,0,30,extra",
          "header\n0,\"a\",h,NULL,0,.1,0,30", "header\n0,,h,NULL,0,.1,0,30", "header\n0,a,h,NULL,NaN,.1,0,30",
          "header\n0,a,h,NULL,0,inf,0,30", "header\n0,a,h,NULL,0,.1,0,-1",
          "header\n0,a,h,NULL,0,.1,0,100000000", "header\n2147483648,a,h,NULL,0,.1,0,30",
          "header\n0,a,h,NULL,0,1junk,0,30", "header\n0,a,h,NULL,0,+-1,0,30", "header\n0,a,h,NULL,0,.1,0,",
          "header\n, a,h,NULL,0,.1,0,30"})
        rejects([&] { parse_material_rows(csv); });
    std::string csv = "header\n";
    for (int i = 0; i < 64; ++i)
        csv += "0,a,h,NULL,0,.1,0,30\n";
    check(parse_material_rows(csv).size() == 64);
    csv += "0,a,h,NULL,0,.1,0,30\n";
    rejects([&] { parse_material_rows(csv); });
}
void zero() {
    auto a = material_resource_zero();
    const auto b = material_resource_zero();
    std::size_t count = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        check(a[i].width == (256u >> i) && a[i].height == a[i].width);
        check(a[i].rgba.size() == a[i].width * a[i].height * 4);
        count += a[i].rgba.size();
    }
    check(count == 349504);
    for (unsigned y = 0; y < 4; ++y) {
        check(a[0].rgba[y * 1024 + 3] == 128);
        check(a[0].rgba[y * 1024 + 7] == 127);
    }
    check(a[0].rgba[16 * 4 + 1] == 0 && a[0].rgba[16 * 4 + 3] == 255);
    for (unsigned level = 5; level < 7; ++level)
        for (unsigned y = 0; y < a[level].height; ++y)
            for (unsigned x = 0; x < a[level].width; ++x) {
                auto offset = (y * a[level].width + x) * 4;
                check(a[level].rgba[offset] == 255 && a[level].rgba[offset + 1] == 255 &&
                      a[level].rgba[offset + 2] == 255);
                check(a[level].rgba[offset + 3] == ((x & 1) ? 127 : 128));
            }
    a[0].rgba[0] = 0;
    check(b[0].rgba[0] == 255 && a[1].rgba[0] == 255);
}
void meshes() {
    using civ5::assets::Bytes;
    using civ5::terrain::land_grid_indices;
    auto indices = land_grid_indices(2, 2, 1);
    Bytes vertices(4 * 48);
    // Every byte of the unused TEXCOORD0 field is opaque to this shader.
    const float nan = std::numeric_limits<float>::quiet_NaN();
    for (unsigned i = 0; i < 4; ++i)
        std::memcpy(vertices.data() + i * 48 + 12, &nan, 4);
    const MaterialPatchMesh valid(2, 2, vertices, indices);
    check(valid.columns() == 2 && valid.rows() == 2 && valid.vertices().size() == 192);
    check(std::equal(valid.vertices().begin(), valid.vertices().end(), vertices.begin()));
    const auto old_index = indices[0];
    indices[0] = 99;
    vertices[40] = 255;
    check(valid.indices()[0] == old_index && valid.vertices()[40] == 0);
    for (auto dims : {std::pair{1, 2}, {2, 1}, {68, 2}, {2, 68}, {-1, 2}, {2147483647, 2147483647}})
        rejects([&] { MaterialPatchMesh invalid(dims.first, dims.second, {}, {}); });
    indices = land_grid_indices(2, 2, 1);
    vertices = Bytes(192);
    rejects([&] { MaterialPatchMesh invalid(2, 2, std::span(vertices).first(191), indices); });
    rejects([&] { MaterialPatchMesh invalid(2, 2, vertices, std::span(indices).first(indices.size() - 1)); });
    auto bad_index = indices;
    bad_index.back() = 4;
    rejects([&] { MaterialPatchMesh invalid(2, 2, vertices, bad_index); });
    for (unsigned offset : {0, 4, 8, 20, 24, 28})
        for (float value : {nan, std::numeric_limits<float>::infinity()}) {
            auto bad = vertices;
            std::memcpy(bad.data() + 3 * 48 + offset, &value, 4);
            rejects([&] { MaterialPatchMesh invalid(2, 2, bad, indices); });
        }
    for (unsigned lane = 0; lane < 8; ++lane) {
        auto bad = vertices;
        bad[3 * 48 + 40 + lane] = 96;
        rejects([&] { MaterialPatchMesh invalid(2, 2, bad, indices); });
        bad[3 * 48 + 40 + lane] = 95;
        MaterialPatchMesh boundary(2, 2, bad, indices);
        check(boundary.vertices()[3 * 48 + 40 + lane] == 95);
    }
    MaterialPatchMesh largest(67, 67, Bytes(67 * 67 * 48), land_grid_indices(67, 67, 1));
    check(largest.vertices().size() == 215472 && largest.indices().size() == 26136);
}
void provider_validation() {
    MaterialRow authored{1, "a.dds", "b.dds", std::nullopt, 12.f, 2.f, 0, 0x10};
    auto provider = terrain_provider_rows(std::span(&authored, 1));
    check(authored.dominance == 12.f && provider.front().dominance == 0.f);
    check(provider.size() == 2 && provider.back().continent == -1 && !provider.back().specular);
    check(provider.back().diffuse == provider.back().height && provider.back().diffuse == "Checkerboard.dds");
    check(std::bit_cast<std::uint32_t>(provider.back().scale) == 0x3c23d70au);
    check(terrain_material_codes(provider) == std::vector<std::uint32_t>{0x110010, 0xffffffff});
    rejects([] { terrain_provider_rows({}); });
    rejects([&] { terrain_provider_rows(std::vector<MaterialRow>(64, authored)); });
    rejects([] { terrain_material_codes({}); });
    rejects([&] { terrain_material_codes(std::vector<MaterialRow>(65, authored)); });
    for (int continent : {-2, -1, 5, 256}) {
        auto bad = authored;
        bad.continent = continent;
        rejects([&] { terrain_provider_rows(std::span(&bad, 1)); });
        rejects([&] { terrain_material_codes(std::span(&bad, 1)); });
    }
    std::reverse(provider.begin(), provider.end());
    rejects([&] { terrain_material_codes(provider); });
    rejects([] { terrain_river_settings(std::vector<std::uint32_t>(65)); });
    for (int size : {-1, 0, 65537, std::numeric_limits<int>::max()}) {
        rejects([&] { material_cache_layout(size, 16); });
        rejects([&] { material_cache_layout(16, size); });
    }
    check(material_cache_layout(1, 1).patches.empty());
    const auto odd = material_cache_layout(65, 66);
    check(odd.columns == 1 && odd.rows == 2 && odd.patches.size() == 2);
    check(odd.patches[0].columns() == 67 && odd.patches[0].rows() == 67);
    check(odd.patches[1].origin == MaterialOrigin{0, 64} && odd.patches[1].rows() == 4);
}
} // namespace
int main() {
    try {
        expansion();
        constants();
        rows();
        zero();
        meshes();
        provider_validation();
        std::cout << "PASS " << checks << " material CPU checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
