#include "civ5/terrain/coast_strip.hpp"
#include "civ5/terrain/terrain_occurrences.hpp"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace civ5::terrain;
namespace {
int checks = 0;
void check(bool ok, std::source_location where = std::source_location::current()) {
    ++checks;
    if (!ok)
        throw std::runtime_error("Coast strip check failed at line " + std::to_string(where.line()));
}
template <class F> void rejects(F f) {
    bool rejected = false;
    try {
        f();
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    check(rejected);
}
CoastProfiles profiles() {
    std::vector<CoastProfileRecord> records;
    for (int i = 1; i <= 3; ++i) {
        CoastProfileRecord p{i, {}, {5, 5, 5, 5}};
        p.nodes.fill({0.f, 1.f, float(i) * .25f, -1});
        records.push_back(p);
    }
    return CoastProfiles(records);
}
CoastCurves controls(unsigned n) {
    CoastCurves curves;
    for (unsigned band = 0; band < 5; ++band)
        for (unsigned i = 0; i < n; ++i)
            curves[band].push_back({float(i), float(band) * 2.f, 0.f, 1, 2});
    return curves;
}
CoastStrip build(CoastCurves curves) {
    const std::array<std::uint32_t, 1> palette{0x110005};
    return build_coast_strip(CoastCurveSet(std::move(curves), 0), profiles(), palette, 0x110000, 0x100000);
}
CoastStrip build(unsigned n) { return build(controls(n)); }
void strip_cases() {
    auto strip = build(4);
    check(strip.vertices().size() == 24 && strip.triangle_count() == 16);
    check(strip.vertices()[0].x == .5f && strip.vertices()[8].x == 1.5f && strip.vertices()[16].x == 2.5f);
    check(strip.vertices()[0].y == 0.f && strip.vertices()[1].y == 2.f);
    const std::array<std::uint16_t, 6> first{0, 1, 8, 1, 9, 8};
    check(std::equal(first.begin(), first.end(), strip.local_indices().begin()));
    strip = build(19);
    check(strip.chunks().size() == 2);
    check(strip.chunks()[0].triangle_count == 72 && strip.chunks()[1].triangle_count == 64);
    check(strip.chunks()[1].first_vertex == 72 && strip.chunks()[1].vertex_count == 72);
    check(strip.local_indices()[72 * 3] == 0);
    const auto job = strip.raster_job(1, {1, 2, 3, 4}, {5, 6, 7, 8});
    check(job.triangles().size() == 64 && job.triangles()[0].a.x == strip.vertices()[72].x);
    check(job.remap_from()[3] == 4 && job.remap_to()[0] == 5);
    check(job.triangles()[0].edge_ab == 0.f && job.triangles()[1].edge_ab == 1.f);
    check(job.triangles()[2].edge_bc == 1.f && job.triangles()[6].edge_ca == 1.f);
    check(job.triangles()[7].edge_ca == 0.f);
    rejects([&] { strip.raster_job(2, {}, {}); });
    auto curves = controls(4);
    for (auto &c : curves)
        for (unsigned i = 1; i < 4; ++i)
            c[i].style_b = 3;
    const auto cached = build(curves);
    check(cached.vertices()[0].height == .5f && cached.vertices()[8].height == .5f);
    for (auto &c : curves)
        for (auto &p : c)
            p.transition = 2.f;
    const auto extrapolated = build(curves);
    check(extrapolated.vertices()[0].height == 0.f); // No transition clamp.
    for (auto &c : curves)
        for (auto &p : c) {
            p.style_a = 250;
            p.style_b = 251;
        }
    const auto missing = build(curves);
    check(missing.vertices()[0].height == 0.f && missing.vertices()[0].opacity == 0.f);
    for (unsigned n : {3u, 10u, 18u, 19u, 4098u, 6626u, 16370u, 16371u, 34466u}) {
        const auto large = build(n);
        check(large.vertices().size() == (n - 1) * 8 && large.triangle_count() == (n - 2) * 8);
        std::uint32_t next = 0;
        for (const auto &chunk : large.chunks()) {
            check(chunk.first_triangle == next && chunk.first_vertex == next);
            check(chunk.vertex_count == chunk.triangle_count + 8);
            check(large.local_indices()[next * 3] == 0);
            for (std::size_t i = next * 3; i < (next + chunk.triangle_count) * 3; ++i)
                check(large.local_indices()[i] < chunk.vertex_count);
            next += chunk.triangle_count;
        }
        check(next == large.triangle_count());
        if (n == 16370) {
            check(large.vertices().size() == 130952 && large.chunks().size() == 2046);
            check(large.chunks().back().first_vertex > 65535);
            const auto last = large.raster_job(large.chunks().size() - 1, {}, {});
            check(last.triangles().back().b.x == 16368.5f);
        }
        if (n == 34466) {
            check(large.vertices().size() == 275720 && large.chunks().size() == 4308);
            const auto last = large.raster_job(large.chunks().size() - 1, {}, {});
            check(last.triangles().back().b.x == 34464.5f);
        }
    }
    rejects([] { build(2); });
    curves = controls(4);
    curves[4].pop_back();
    rejects([&] { build(curves); });
}
void dispatch_cases() {
    TerrainRandom native_random(1);
    const auto no_jobs = build_coast_dispatch({}, 0, {}, native_random, 255);
    check(native_random.calls() == 0 && native_random.state() == 1 && no_jobs.direct_random_calls == 0);
    const std::vector<CoastStrip> invalid_strips{build(3)};
    rejects([&] { build_coast_dispatch(invalid_strips, 0, {}, native_random, 255); });
    check(native_random.calls() == 0 && native_random.state() == 1);
    const std::vector<CoastStrip> streamed{build(34), build(34)};
    const auto streamed_plan = build_coast_dispatch(streamed, 15423, {}, native_random, 255);
    TerrainRandom reference_random(1);
    reference_random.next(); reference_random.next();
    check(streamed_plan.direct_random_calls == 2 && native_random.calls() == 2 && native_random.state() == reference_random.state());
    std::vector<CoastStrip> strips{build(19), build(34), build(3)};
    const std::array<std::uint32_t, 3> randoms{1, 3, 0xffffffff};
    const auto plan = build_coast_dispatch(strips, 15423, {}, randoms, 0);
    check(plan.direct_random_calls == 2 && plan.gate == 15423);
    check(plan.phases[0].size() == 4 && plan.phases[1].size() == 3);
    unsigned marked = 0;
    for (unsigned phase = 0; phase < 2; ++phase)
        for (const auto &job : plan.phases[phase]) {
            check(job.chunk % 2 == phase && job.strip < strips.size());
            if (job.allocation_flag) {
                ++marked;
                check(job.strip == 1 && job.chunk == 2 && job.allocation_flag == 0xffffff01);
            }
        }
    check(marked == 1);
    std::vector<std::uint32_t> palette(254, 0xffffffff);
    for (auto key : {0x110035u, 0x110010u, 0x120035u, 0x120010u})
        palette.push_back(key);
    const auto remap = build_coast_dispatch(strips, 0, palette, randoms, 255);
    check(remap.remap_from == std::array<std::uint8_t, 4>{255, 0, 255, 255});
    check(remap.remap_to == std::array<std::uint8_t, 4>{255, 1, 255, 255});
    palette = {0x110035, 0x110010, 0x110035, 0x110010};
    check(build_coast_dispatch(strips, 0, palette, randoms, 255).remap_from[0] == 0);
    const auto empty = build_coast_dispatch({}, 0, {}, {}, 0);
    check(empty.direct_random_calls == 0 && empty.phases[0].empty() && empty.phases[1].empty());
    rejects([&] { build_coast_dispatch(strips, 0, {}, {}, 0); });
    const std::array<std::uint32_t, 2> invalid{32768, 0}, single{2, 0};
    rejects([&] { build_coast_dispatch(strips, 0, {}, invalid, 0); });
    rejects([&] { build_coast_dispatch(strips, 0, {}, single, 0); });
}
} // namespace
int main() {
    try {
        strip_cases();
        dispatch_cases();
        std::cout << "PASS " << checks << " coast strip/dispatch checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
