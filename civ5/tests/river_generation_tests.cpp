#include "civ5/terrain/river_geometry.hpp"
#include "civ5/terrain/world_layout.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
using namespace civ5::terrain;
namespace {
int checks = 0;
void check(bool value) {
    ++checks;
    if (!value)
        throw std::runtime_error("River generation check failed at " + std::to_string(checks));
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
// Tests use a specified deterministic estimate provider. Byte-exact original
// instruction estimates are tested independently by the native corpora.
float estimate(float squared) { return 1.f / std::sqrt(squared); }
std::vector<RiverEdge> fixture() {
    std::vector<RiverEdge> edges;
    for (int i = 0; i < 16; ++i) {
        const int x = 1 + i / 2, y = 7 + i % 2;
        edges.push_back({x, y, -x - y, i % 2 == 0 ? 0 : 8});
    }
    return edges;
}
void normalization() {
    int calls = 0;
    const auto provider = [&](float squared) {
        ++calls;
        return estimate(squared);
    };
    check(normalize3_sse({0, 0, 0}, provider) == Vector3{0, 0, 0} && calls == 0);
    check(normalize2_sse({0, 0}, provider) == Point2{0, 0} && calls == 0);
    const auto a = normalize2_sse({3, 4}, provider);
    const auto b = normalize3_sse({3, 4, 0}, provider);
    check(calls == 2 && std::abs(a.x - .6f) < 1e-6f && std::abs(a.y - .8f) < 1e-6f);
    check(std::abs(b[0] - .6f) < 1e-6f && std::abs(b[1] - .8f) < 1e-6f && b[2] == 0);
    check(normalize3_sse({0x1p-24f, 0, 0}, provider) == Vector3{0, 0, 0} && calls == 2);
    rejects([&] { normalize3_sse({1, 0, 0}, {}); });
    rejects([&] { normalize2_sse({1, 0}, {}); });
    for (auto bad : {0.f, -1.f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        rejects([&] { normalize3_sse({1, 0, 0}, [&](float) { return bad; }); });
        rejects([&] { normalize2_sse({1, 0}, [&](float) { return bad; }); });
    }
    rejects([&] { normalize3_sse({std::numeric_limits<float>::max(), 0, 0}, provider); });
    rejects([&] { normalize3_sse({0, std::numeric_limits<float>::quiet_NaN(), 0}, provider); });
}
void network() {
    const auto edges = fixture();
    const auto n = RiverNetwork::build(16, 12, edges);
    check(n.chains().size() == 1 && n.chains()[0].edges == edges && n.chains()[0].terminal);
    auto duplicated = edges;
    std::reverse(duplicated.begin(), duplicated.end());
    duplicated.insert(duplicated.end(), edges.begin(), edges.end());
    const auto d = RiverNetwork::build(16, 12, duplicated);
    check(std::equal(n.direction_grid().begin(), n.direction_grid().end(), d.direction_grid().begin()));
    check(d.chains().size() == 1 && d.chains()[0].edges == edges);
    std::set<RiverEdge> used;
    for (const auto &chain : n.chains())
        for (auto e : chain.edges) {
            check(n.contains(e) && n.canonical(e) == e);
            check(n.opposite(n.opposite(e)) == e);
            check(used.insert(n.normalized(e)).second);
        }
    const auto e = edges[0];
    auto shifted = e;
    shifted.x += 16;
    shifted.z -= 16;
    check(n.normalized(shifted) == n.normalized(e));
    auto raw = e;
    raw.direction = 3;
    const auto ignored = RiverNetwork::build(16, 12, std::span(&raw, 1));
    check(ignored.chains().empty() &&
          std::count(ignored.direction_grid().begin(), ignored.direction_grid().end(), 8u) == 1);
    check(RiverNetwork::build(1, 1, {}).chains().empty());
    for (auto size : {0, -1, 201, 2147483647})
        rejects([&] { RiverNetwork::build(size, 12, {}); });
    for (auto invalid : {RiverEdge{1, 7, -7, 0}, {1, 7, -8, -1}, {1, 7, -8, 12}, {-500, 7, 493, 0}})
        rejects([&] { RiverNetwork::build(16, 12, std::span(&invalid, 1)); });
    // Six directed edges around a cell have no river source; native backward
    // traversal would never terminate. The owned API rejects that cycle.
    const std::array<RiverEdge, 6> cycle{
        {{4, 6, -10, 0}, {4, 6, -10, 1}, {4, 6, -10, 2}, {4, 5, -9, 6}, {5, 6, -11, 7}, {3, 7, -10, 8}}};
    rejects([&] { RiverNetwork::build(16, 12, cycle); });
}
void geometry() {
    const auto world = build_world_layout(16, 12, 1, {}, [](float v) { return 1.f / v; });
    Matrix4 matrix;
    std::copy_n(world.transforms.begin() + 16, 16, matrix.begin());
    const auto n = RiverNetwork::build(16, 12, fixture());
    TerrainRandom random(0xabcdef12);
    int phases = 0;
    const auto center = build_river_centerlines(
        n, matrix, random, [&](RiverControlStage phase, const RiverPolylines &p, const TerrainRandom &state) {
            check(static_cast<int>(phase) == phases++ && p.size() == 1 && p[0].points.size() == 32);
            check(state.calls() == (phase == RiverControlStage::jittered ? 95u : 0u));
        });
    check(phases == 3 && random.calls() == 95);
    const auto extension = extend_river_centerlines(n, center, matrix, 1, estimate);
    check(center[0].points.size() == 32 && extension[0].points.size() == 36 && extension[0].terminal);
    check(std::equal(center[0].points.begin(), center[0].points.end(), extension[0].points.begin()));
    auto wrong = center;
    wrong[0].points.pop_back();
    rejects([&] { extend_river_centerlines(n, wrong, matrix, 1, estimate); });
    rejects([&] { extend_river_centerlines(n, center, matrix, 3, estimate); });
    const auto banks = widen_river_centerlines(extension, random, estimate);
    check(random.calls() == 147 && !banks[0][0].terminal && !banks[1][0].terminal);
    check(banks[0][0].points.size() == 36 && banks[1][0].points.size() == 36 && extension[0].terminal);
    const HeightDomain domain(1024, 1024);
    const std::vector<std::uint16_t> input(1024 * 1024, 41000);
    const auto samples = sample_river_heights(domain, input, extension);
    check(samples.size() == 1 && samples[0].size() == 36 &&
          std::all_of(samples[0].begin(), samples[0].end(), [](auto h) { return h == 41000; }));
    int sampled = 0;
    const auto streams = build_river_triangles(
        extension, banks, samples, estimate,
        [&](bool derivative, std::span<const RiverPoint> p, float t, const RiverPoint &value) {
            ++sampled;
            check(p.size() == 36 && t >= 0 && t <= 34 && std::abs(value[2] - (derivative ? 0.f : 41000.f)) <= .02f);
        });
    check(sampled == 2730 && streams.channels.size() == 1364 && streams.banks.size() == 2052);
    for (const auto &t : streams.banks)
        for (const auto &v : {t.a, t.b, t.c})
            check(v.profile == 0 && std::abs(v.height - 41000.f) <= .02f);
    for (const auto &t : streams.channels)
        for (const auto &v : {t.a, t.b, t.c})
            check((v.profile == 0 || v.profile == 1) && (v.opacity == 0 || v.opacity == 1));
    auto incomplete = samples;
    incomplete[0].pop_back();
    rejects([&] { build_river_triangles(extension, banks, incomplete, estimate); });
    auto mismatched = banks;
    mismatched[1][0].points.pop_back();
    rejects([&] { build_river_triangles(extension, mismatched, samples, estimate); });
    auto invalid = samples;
    invalid[0][3] = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { build_river_triangles(extension, banks, invalid, estimate); });
    auto clipped = extension;
    clipped[0].points[0][1] = -1;
    const auto skipped = sample_river_heights(domain, input, clipped);
    check(skipped[0].size() == 35);
    rejects([&] { build_river_triangles(clipped, banks, skipped, estimate); });
    clipped[0].points[0] = {-102401, 0, 0};
    rejects([&] { sample_river_heights(domain, input, clipped); });
    rejects([&] { sample_river_heights(domain, {}, extension); });
    const auto empty = build_river_triangles({}, std::array<RiverPolylines, 2>{}, {}, estimate);
    check(empty.banks.empty() && empty.channels.empty());
    const RiverPolylines coincident{{{{8, 8, 0}, {8, 8, 0}, {8, 8, 0}, {8, 8, 0}}, true}};
    const auto flat =
        build_river_triangles(coincident, {coincident, coincident}, {{35000, 35000, 35000, 35000}}, estimate);
    const std::vector<std::uint8_t> materials(input.size(), 7);
    const RiverRasterSettings settings{.23f, X87Precision::double_precision, {7, 7, 7, 7}, {1, 2, 3, 4}, {}, 0x100000};
    const auto raster = apply_river_triangles(domain, input, materials, flat.banks, flat.channels, settings);
    check(raster.heights == input && raster.materials == materials);
}
} // namespace
int main() {
    try {
        normalization();
        network();
        geometry();
        std::cout << "PASS " << checks << " river generation checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
