#include "civ5/materials/material_generation.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
using namespace civ5::materials;
using civ5::terrain::X87Precision;
namespace {
int checks = 0;
void check(bool value) {
    ++checks;
    if (!value)
        throw std::runtime_error("Material generation assertion failed");
}
template <class F> void rejects(F f) {
    bool caught = false;
    try {
        f();
    } catch (const std::invalid_argument &) {
        caught = true;
    }
    check(caught);
}
bool same(std::span<const std::int32_t> a, std::initializer_list<std::int32_t> b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end());
}
void selections() {
    check(same(MaterialSelection(std::array<std::uint32_t, 5>{2, 5, 5, 0, 1}).sources(), {0, 1, 2, 4}));
    std::array<std::uint32_t, 64> values{};
    std::iota(values.begin(), values.end(), 1u);
    check(same(MaterialSelection(std::span(values).first(8)).sources(), {0, 1, 2, 3, 4, 5, 6, 7}));
    check(same(MaterialSelection(std::span(values).first(9)).sources(), {8, 7, 6, 5, 4, 3, 2, 1}));
    const MaterialSelection all(values);
    for (int i = 0; i < 64; ++i)
        check(all.owner_words()[i] == 63 - i);
    values.fill(7);
    check(same(MaterialSelection(values).sources(), {0, 1, 2, 3, 4, 5, 6, 7}));
    values[0] = 0xffffffffu;
    values[1] = 0x80000000u;
    check(same(MaterialSelection(values).sources(), {0, 1, 2, 3, 4, 5, 6, 7}));
    std::array<std::int32_t, 64> slots{};
    slots[1] = 63;
    std::array<std::uint32_t, 64> handles{};
    handles[63] = 65535;
    const std::array<std::uint32_t, 3> scores{0, 1, 0};
    check(MaterialSelection(scores, 0, 1, {}, {}).specular() == 0);
    check(MaterialSelection(scores, 1, 0, slots, handles).specular() == 1);
    check(MaterialSelection({}, -1, 1, {}, {}).specular() == 1);
    rejects([&] { MaterialSelection invalid(std::array<std::uint32_t, 65>{}); });
    rejects([&] { MaterialSelection invalid(scores, 1, 0, {}, {}); });
    slots[1] = -1;
    rejects([&] { MaterialSelection invalid(scores, 1, 0, slots, handles); });
}
void lanes_and_factors() {
    std::array<std::uint8_t, 48> prior{};
    std::iota(prior.begin(), prior.end(), std::uint8_t{0});
    const MaterialSelection two(std::array<std::uint32_t, 2>{1, 1});
    const auto pc = X87Precision::double_precision;
    auto packed = write_material_lanes(prior, two, std::array{.75f, .5f}, pc);
    check(packed[32] == 191 && packed[33] == 127 &&
          std::equal(packed.begin(), packed.begin() + 32, prior.begin()));
    check(write_material_lanes(prior, two, std::array{.0001f, .0001f}, pc)[32] == 255);
    check(write_material_lanes(prior, two, std::array{.001f, 0.f}, pc)[32] == 0);
    packed = write_material_lanes(prior, MaterialSelection({}), {}, pc);
    check(packed[32] == 255 && std::all_of(packed.begin() + 33, packed.end(), [](auto v) { return v == 0; }));
    auto expected = prior;
    expected[40] = 1;
    check(write_material_lanes(prior, two, {}, pc, -1, 257) == expected);
    const float nan = std::numeric_limits<float>::quiet_NaN();
    for (float v : {nan, std::numeric_limits<float>::infinity(), 1e20f, std::numeric_limits<float>::max()})
        check(write_material_lanes(prior, two, std::array{v, 0.f}, pc)[32] == 0);
    int actual = -1;
    const std::array<MaterialSample, 1> samples{{{-1, 0, 1.f}}};
    auto f = material_factors(1, samples, 48, 32, {0, 0}, 0, std::array{0}, std::array{0}, [&](int x, int) {
        actual = x;
        return 0;
    });
    check(actual == 15 && f == std::vector<float>{1.f});
    f = material_factors(2, std::array<MaterialSample, 1>{{{0, 0, 0.f}}}, 16, 16, {0, 0}, 0, std::array{0},
                         std::array{0}, [](int, int) { return 0; });
    check(std::all_of(f.begin(), f.end(), [](float v) { return std::isnan(v); }));
    packed = write_material_lanes(prior, two, f, pc);
    check(std::all_of(packed.begin() + 32, packed.begin() + 40, [](auto v) { return v == 0; }));
    rejects([&] { write_material_lanes(std::span(prior).first(47), two, f, pc); });
    rejects([&] { write_material_lanes(prior, two, {}, pc); });
    rejects([&] { write_material_lanes(prior, two, f, static_cast<X87Precision>(99)); });
    rejects([&] { material_factors(65, {}, 1, 1, {}, 0, {}, {}, {}); });
    rejects([&] { material_factors(1, samples, 0, 1, {}, 0, {}, {}, {}); });
    rejects([&] {
        material_factors(1, samples, 1, 1, {}, 0, std::array{0}, std::array{0}, [](int, int) { return 64; });
    });
    rejects([&] {
        material_factors(1, samples, 1, 1, {}, 2, std::array{0}, std::array{1}, [](int, int) { return 0; });
    });
    // Extreme signed coordinates/slots must fail safely or preserve wrapped ADD, not invoke signed UB.
    material_factors(1, samples, 48, 32, {std::numeric_limits<int>::min(), std::numeric_limits<int>::max()},
                     0, std::array{0}, std::array{0}, [](int x, int y) {
                         check(x >= 0 && x < 48 && y >= 0 && y < 32);
                         return 0;
                     });
}
void normals() {
    const auto x = material_normal_kernel(0), y = material_normal_kernel(1);
    for (int a = 0; a < 5; ++a)
        for (int b = 0; b < 5; ++b)
            check(std::bit_cast<std::uint32_t>(x[a * 5 + b]) == std::bit_cast<std::uint32_t>(y[b * 5 + a]));
    rejects([] { material_normal_kernel(2); });
    MaterialSourceDomain domain{32, 32, {1.f, .866f, .2f}, 0};
    const auto input = material_normal_input({3, 3}, domain, [](int, int) { return 10000; });
    const std::array<std::uint32_t, 3> bits{0x3bd2075a, 0x3bf28703, 0x3f5db22d};
    for (int i = 0; i < 3; ++i)
        check(std::bit_cast<std::uint32_t>(input[i]) == bits[i]);
    bool called = false;
    check(normalize_material_normal({1e-9f, 0, 0},
                                    [&](float) {
                                        called = true;
                                        return 1.f;
                                    }) == MaterialVector3{} &&
          !called);
    check(normalize_material_normal({.2f, .3f, .8f}, [](float) { return 1.1f; }) !=
          normalize_material_normal({.2f, .3f, .8f}, [](float) { return 1.2f; }));
    rejects([] { normalize_material_normal({1, 0, 0}, [](float) { return 0.f; }); });
    rejects([&] { material_normal_input({}, domain, [](int, int) { return 65536; }); });
    check(material_projection_axis({.1f, .2f, .90001f}) == 0);
    check(material_projection_axis({.1f, .2f, .9f}) == 1);
    check(material_projection_axis({-.2f, .2f, .9f}) == 2);
    for (auto [v, h] : {std::pair{0.f, 0},
                        {-0.f, 0x8000},
                        {1.f, 0x3c00},
                        {-1.f, 0xbc00},
                        {std::numeric_limits<float>::infinity(), 0x7fff},
                        {-std::numeric_limits<float>::infinity(), 0xffff},
                        {std::bit_cast<float>(0x33800000u), 1},
                        {std::bit_cast<float>(0x7fc00001u), 0x7fff}})
        check(material_half(v) == h);
    bool wrapped = false;
    domain = {48, 32, {1, 1, .2f}, 1};
    material_normal_input({0, 0}, domain, [&](int xx, int yy) {
        wrapped |= xx == 47;
        check(xx >= 0 && xx < 48 && yy >= 0 && yy <= 2);
        return 1000;
    });
    check(wrapped);
    domain.width = std::numeric_limits<int>::max();
    material_normal_input({std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}, domain,
                          [](int xx, int yy) {
                              check(xx >= 0 && yy >= 0);
                              return 10000;
                          });
}
void scores_and_patches() {
    std::array<std::int32_t, 64> slots{}, modes{};
    slots.fill(3);
    modes.fill(1);
    for (int axis = 0; axis < 3; ++axis) {
        const std::array<MaterialVector3, 3> n{{{0, 0, 1}, {0, 1, 0}, {1, 0, 0}}};
        const auto scores = material_scores(
            2, 2, {0, 0}, 16, 16, slots, modes, [&](int, int) { return n[axis]; },
            [](int, int) { return 0; });
        MaterialScoreWords want{};
        want[3 + axis] = 18;
        want[4 + axis] = 9;
        want[5 + axis] = 9;
        check(scores == want);
    }
    for (int mode : {-1, 0, 2}) {
        modes.fill(mode);
        const auto s = material_scores(
            3, 4, {}, 16, 16, slots, modes,
            [](int, int) -> MaterialVector3 { throw std::runtime_error("unused normal read"); },
            [](int, int) { return 0; });
        check(s[3] == 20 && std::accumulate(s.begin(), s.end(), 0u) == 20);
    }
    modes.fill(1);
    slots.fill(63);
    const auto s = material_scores(
        2, 2, {}, 16, 16, slots, modes, [](int, int) { return MaterialVector3{0, 0, 1}; },
        [](int, int) { return 0; });
    check(s[63] == 18 && s[64] == 9 && s[65] == 9);
    check(same(MaterialSelection(std::span(s).first(64)).sources(), {63}));
    slots.fill(127);
    rejects([&] {
        material_scores(
            2, 2, {}, 16, 16, slots, modes, [](int, int) { return MaterialVector3{1, 0, 0}; },
            [](int, int) { return 0; });
    });
    for (int columns = 2; columns <= 67; ++columns)
        for (int rows = 2; rows <= 67; ++rows) {
            std::vector<int> visits(columns * rows);
            for (auto t : material_tiles(columns, rows)) {
                if (t.vertex_count() < 1 || t.vertex_count() > 144)
                    throw std::runtime_error("Invalid tile count");
                for (int xx = t.x0; xx < t.x1; ++xx)
                    for (int yy = t.y0; yy < t.y1; ++yy)
                        ++visits[(yy + 1) * columns + xx + 1];
            }
            check(std::all_of(visits.begin(), visits.end(), [](int v) { return v == 1; }));
        }
    check(material_tiles(13, 13) ==
          std::vector<MaterialTile>{{-1, 11, -1, 11}, {-1, 11, 11, 12}, {11, 12, -1, 11}, {11, 12, 11, 12}});
    rejects([] { material_tiles(68, 2); });
    rejects([] {
        MaterialTile{std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), 0, 1}.vertex_count();
    });
    MaterialGeometryDomain domain{
        13, 13, {0, 0}, {32, 32, {1, 1, .2f}, 0}, 64, X87Precision::double_precision};
    const MaterialSelection selected(std::array<std::uint32_t, 1>{1});
    int normal_count = 0;
    const auto patch = build_material_patch(
        domain, selected, [](int, int) { return 10000; },
        [&](int, int) {
            ++normal_count;
            return MaterialVector3{0, 0, 1};
        },
        [](int, int, int) { return std::vector<float>{1}; });
    check(normal_count == 169 && patch.mesh().indices().size() == 864);
    for (int i = 0; i < 169; ++i)
        check(patch.streams.material48[i * 48 + 32] == 255);
    MaterialStreams prior{civ5::assets::Bytes(169 * 48, 0x65), civ5::assets::Bytes(169 * 24, 0x12)};
    const auto untouched = prior;
    auto out = write_material_tile(
        domain, {0, 1, 0, 1}, prior, selected, 1, [](int, int) { return 10000; },
        [](int, int) { return MaterialVector3{0, 0, 1}; },
        [](int, int, int) -> std::vector<float> { throw std::runtime_error("alternate read factors"); });
    const int record = 14;
    for (int lane = 32; lane < 48; ++lane)
        check(out.material48[record * 48 + lane] == (lane == 40 ? 0 : 0x65));
    check(std::equal(out.material48.begin(), out.material48.begin() + record * 48, prior.material48.begin()));
    rejects([&] {
        write_material_tile(
            domain, {0, 1, 0, 1}, prior, selected, 0, [](int, int) { return -1; },
            [](int, int) { return MaterialVector3{0, 0, 1}; }, {});
    });
    check(prior.material48 == untouched.material48 && prior.normal24 == untouched.normal24);
}
} // namespace
int main() {
    try {
        selections();
        lanes_and_factors();
        normals();
        scores_and_patches();
        std::cout << "PASS " << checks << " material generation checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
