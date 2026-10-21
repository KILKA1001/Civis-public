#include "civ5/terrain/coast_raster.hpp"
#include "civ5/terrain/worker_field.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace civ5::terrain;
namespace {
int checks = 0;
void check(bool ok) {
    ++checks;
    if (!ok)
        throw std::runtime_error("Coast raster check failed");
}
template <class F> void rejects(F f) {
    bool no = false;
    try {
        f();
    } catch (const std::invalid_argument &) {
        no = true;
    }
    check(no);
}
CoastTriangle triangle(float x = 0, float y = 0, float height = .25f, float opacity = 1,
                       std::uint8_t material = 255) {
    return {{x, y, material, height, opacity},
            {x + 4, y, 255, height, opacity},
            {x, y + 4, 255, height, opacity},
            opacity,
            opacity,
            opacity};
}
CoastRasterJob job(CoastTriangle t, std::array<std::uint8_t, 4> from = {0, 1, 2, 3}) {
    return CoastRasterJob(std::span(&t, 1), from, {40, 41, 42, 43});
}
CoastRasterField apply(const std::vector<CoastRasterJob> &jobs, std::uint16_t h = 10000, bool wrap = true) {
    return apply_coast_jobs(HeightDomain(16, 16), std::vector<std::uint16_t>(256, h),
                            std::vector<std::uint8_t>(256, 7), jobs, wrap,
                            CoastBoundsPrecision::double_or_extended);
}
} // namespace
int main() {
    try {
        const auto basic = apply({job(triangle())});
        check(basic.heights()[0] == 16383 && basic.heights()[255] == 10000 && basic.materials()[255] == 7);
        check(basic.height_field().u16_at(0, 0) == 16383 && basic.height_field().a8_at(0, 0) == 63);
        check(apply({job(triangle(0, 0, .25f, 1, 19))}).materials()[0] == 19);
        check(apply({job(triangle(0, 0, .25f, 0, 19), {7, 7, 7, 7})}, 35000).materials()[0] == 43);
        check(apply({job(triangle(0, 0, .25f, 0, 19), {7, 7, 7, 7})}, 35001).materials()[0] == 19);
        check(apply({job(triangle(0, 0, .25f, 1, 19), {19, 19, 19, 19})}).materials()[0] == 19);
        check(apply({job(triangle(0, 0, -.1f))}).heights()[0] == 58983);
        check(apply({job(triangle(0, 0, .9f, 2))}).heights()[0] == 42426);
        check(apply({job(triangle(0, 0, 1e20f))}).heights()[0] == 0);
        const auto negative = apply({job(triangle(0, 0, .9f, -1, 23))});
        check(negative.heights()[0] == 10000 && negative.materials()[0] == 23);
        const auto wrapped = apply({job(triangle(-2, -2))});
        check(wrapped.heights()[14] == 16383 && wrapped.heights()[254] == 10000);
        auto degenerate = triangle();
        degenerate.b = degenerate.a;
        degenerate.c = degenerate.a;
        const auto deg = apply({job(degenerate)});
        for (auto h : deg.heights())
            check(h == 10000);
        for (auto m : deg.materials())
            check(m == 7);
        const auto a = job(triangle(0, 0, .8f, .4f, 17)), b = job(triangle(0, 0, .2f, .6f, 23));
        const auto ab = apply({a, b}), ba = apply({b, a});
        check(ab.heights()[0] != ba.heights()[0] && ab.materials()[0] == 23 && ba.materials()[0] == 17);
        const auto first = apply({a});
        const std::array jobs{b};
        const auto sequential = apply_coast_jobs(first.domain(), first.heights(), first.materials(), jobs,
                                                 true, CoastBoundsPrecision::double_or_extended);
        check(std::equal(ab.heights().begin(), ab.heights().end(), sequential.heights().begin()));
        check(std::equal(ab.materials().begin(), ab.materials().end(), sequential.materials().begin()));
        const auto disjoint = job(triangle(8, 8, .2f, .6f, 23));
        const auto ad = apply({a, disjoint}), da = apply({disjoint, a});
        check(std::equal(ad.heights().begin(), ad.heights().end(), da.heights().begin()));
        check(std::equal(ad.materials().begin(), ad.materials().end(), da.materials().begin()));
        std::vector<std::uint16_t> h(256, 35000);
        std::vector<std::uint8_t> m(256, 7);
        std::vector<CoastTriangle> ts{triangle(0, 0, .25f, 0, 19)};
        std::array<std::uint8_t, 4> from{7, 7, 7, 7};
        const auto owned = CoastRasterJob(ts, from, {40, 41, 42, 43});
        ts[0] = triangle(8, 8);
        from.fill(0);
        std::vector<std::array<std::uint32_t, 2>> observed;
        std::size_t complete = 0;
        const std::array ordered{owned, CoastRasterJob({}, {}, {})};
        auto result = apply_coast_jobs(
            HeightDomain(16, 16), h, m, ordered, true, CoastBoundsPrecision::single,
            [&](CoastWriteChannel ch, std::int32_t i, std::uint32_t value) {
                if (i == 0)
                    observed.push_back({ch == CoastWriteChannel::material ? 1u : 0u, value});
            },
            [&](std::size_t i) { check(i == complete++); });
        check(complete == 2);
        check(observed == std::vector<std::array<std::uint32_t, 2>>{
                              {1, 19}, {0, 35000}, {1, 40}, {1, 41}, {1, 42}, {1, 43}});
        check(h[0] == 35000 && m[0] == 7 && result.materials()[0] == 43);
        h[0] = 0;
        m[0] = 0;
        check(result.heights()[0] == 35000 && result.materials()[0] == 43);
        // Direct production chain, with no reference U16 handoff.
        const HeightDomain domain(128, 64);
        const auto worker = compose_worker_field(domain, {}, std::vector<std::uint8_t>(8192, 2),
                                                 worker_retail_bias, X87Precision::double_precision);
        const std::array coast_jobs{job(triangle())};
        const auto coast = apply_coast_jobs(domain, worker.heights(), worker.materials(), coast_jobs, true,
                                            CoastBoundsPrecision::double_or_extended);
        check(coast.heights()[0] == 16383 && worker.heights()[0] == 15423);
        check(coast.height_field().a8_at(0, 0) == 63 && worker.height_field().a8_at(0, 0) == 60);
        rejects([&] { apply({job(triangle(-1, 0))}, 10000, false); });
        rejects([&] { apply({job(triangle(65537, 0))}); });
        rejects([&] { apply({job(triangle(0, 65537))}); });
        auto invalid = triangle();
        invalid.a.height = std::numeric_limits<float>::quiet_NaN();
        rejects([&] { job(invalid); });
        invalid = triangle();
        invalid.edge_ab = std::numeric_limits<float>::infinity();
        rejects([&] { job(invalid); });
        rejects([&] { apply_coast_jobs(domain, {}, {}, {}, true, CoastBoundsPrecision::single); });
        rejects([&] {
            apply_coast_jobs(HeightDomain(16, 16), h, m, {}, true, static_cast<CoastBoundsPrecision>(99));
        });
        auto huge = triangle();
        huge.b.x = 65536;
        huge.c.y = 65536;
        const std::array huge_jobs{job(huge)};
        rejects([&] {
            apply_coast_jobs(HeightDomain(32768, 16), std::vector<std::uint16_t>(524288),
                             std::vector<std::uint8_t>(524288), huge_jobs, true,
                             CoastBoundsPrecision::single);
        });
        std::cout << "PASS " << checks << " coast raster checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
