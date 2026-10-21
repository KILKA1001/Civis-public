#include "civ5/terrain/coast_map.hpp"
#include "civ5/terrain/coast_perturbation.hpp"
#include "civ5/terrain/terrain_occurrences.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <source_location>
#include <stdexcept>
using namespace civ5::terrain;
namespace {
unsigned checks = 0;
void check(bool ok, std::source_location where = std::source_location::current()) {
    ++checks;
    if (!ok)
        throw std::runtime_error("Coast map check failed at line " + std::to_string(where.line()));
}
template <class F> void rejects(F f, std::source_location where = std::source_location::current()) {
    bool caught = false;
    try {
        f();
    } catch (const std::invalid_argument &) {
        caught = true;
    }
    check(caught, where);
}
IntMatrix4 identity() {
    IntMatrix4 m{};
    for (unsigned i = 0; i < 16; i += 5)
        m[i] = 1;
    return m;
}
CoastMapSource source(std::span<const std::uint32_t> masks, int width = 5, int height = 5) {
    return {width,        height,
            masks,        identity(),
            identity(),   identity_matrix(),
            true,         X87Precision::double_precision,
            Vec4Path::sse};
}
std::vector<std::uint32_t> island(int x = 2, bool mountain = false, bool deep = false) {
    std::vector<std::uint32_t> masks(25, 0x2000);
    for (int a = 0; a < 5; ++a)
        for (int b = 0; b < 5; ++b)
            if (deep && (a == 0 || a == 4 || b == 0 || b == 4))
                masks[a * 5 + b] = 0x4000;
    masks[x * 5 + 2] = mountain ? 0x400 : 1;
    return masks;
}
std::vector<std::uint32_t> point_bits(const CoastSegmentGrid &g) {
    std::vector<std::uint32_t> out;
    for (const auto &c : g.cells())
        for (auto s : c.segments)
            for (float v : {s.first.x, s.first.y, s.second.x, s.second.y})
                out.push_back(std::bit_cast<std::uint32_t>(v));
    return out;
}
bool equal(std::span<const CoastControlPoint> a, std::span<const CoastControlPoint> b) {
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (std::bit_cast<std::uint32_t>(a[i].x) != std::bit_cast<std::uint32_t>(b[i].x) ||
            std::bit_cast<std::uint32_t>(a[i].y) != std::bit_cast<std::uint32_t>(b[i].y) ||
            std::bit_cast<std::uint32_t>(a[i].transition) != std::bit_cast<std::uint32_t>(b[i].transition) ||
            a[i].style_a != b[i].style_a || a[i].style_b != b[i].style_b)
            return false;
    return true;
}
void grid_cases() {
    check(classify_coast_mask(0x2000) == CoastCellClass{1, 12, 0});
    check(classify_coast_mask(0x4000) == CoastCellClass{2, 12, 0});
    check(classify_coast_mask(0x80000000) == CoastCellClass{0, 0, 0});
    check(classify_coast_mask(0x7ff) == CoastCellClass{0, 11, 2});
    const std::array<unsigned, 11> flags{0x400, 1, 0x20, 2, 0x40, 0x10, 0x200, 4, 0x80, 8, 0x100};
    const std::array<unsigned, 11> styles{11, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    for (unsigned i = 0; i < flags.size(); ++i) {
        unsigned mask = 0;
        for (unsigned j = i; j < flags.size(); ++j)
            mask |= flags[j];
        check(classify_coast_mask(mask).style == styles[i]);
    }
    for (unsigned mask : {0u, 0x4000000u, 0x8000000u})
        rejects([&] { classify_coast_mask(mask); });
    std::vector<std::uint32_t> carried(25, 0x2000);
    carried[1 * 5 + 2] = 1;
    carried[2 * 5 + 2] = 0x4000000;
    const auto inherited = CoastMapGraph::build(source(carried));
    check(inherited.initial().cells()[2 * 5 + 2].classification == CoastCellClass{1, 1, 0});
    carried[3] = 0x4000000; // First cell of the next row has no initialized style.
    rejects([&] { CoastMapGraph::build(source(carried)); });
    auto masks = island(0, true);
    auto s = source(masks);
    auto graph = CoastMapGraph::build(s);
    const auto initial = point_bits(graph.initial());
    masks.assign(25, 0x4000); // Graph owns its source-derived data.
    check(point_bits(graph.initial()) == initial);
    bool unwrapped = false;
    for (std::size_t i = 0; i < graph.links().size(); ++i)
        for (std::size_t k = 0; k < graph.links()[i].size(); ++k)
            for (bool reverse : {false, true}) {
                const auto next = reverse ? graph.links()[i][k].reverse : graph.links()[i][k].forward;
                check(next.edge >= 0);
                unwrapped |= next.x < 0 || next.x >= 5;
                const int x = (next.x % 5 + 5) % 5;
                const auto other = graph.links()[next.y * 5 + x][next.edge];
                const auto back = reverse ? other.forward : other.reverse;
                check((back.x % 5 + 5) % 5 == int(i % 5) && back.y == int(i / 5) && back.edge == int(k));
            }
    check(unwrapped);
    std::vector<std::vector<std::uint32_t>> stages;
    const std::array<unsigned, 10> modes{1, 2, 3, 2, 1, 2, 3, 2, 1, 2};
    auto smoothed =
        graph.smooth(X87Precision::double_precision, [&](unsigned i, unsigned mode, const auto &g) {
            check(mode == modes[i]);
            stages.push_back(point_bits(g));
        });
    check(stages.size() == 10 && stages[0] == initial && stages[1] != initial);
    for (unsigned i : {2, 4, 6, 8})
        check(stages[i] == stages[i - 1]);
    check(stages[2] != stages[3]);
    check(point_bits(graph.initial()) == initial);
    check(point_bits(smoothed) == point_bits(graph.smooth(X87Precision::double_precision)));
    const auto contours = graph.scan(smoothed), again = graph.scan(smoothed);
    check(contours.size() == 1 && again.size() == 1);
    const auto &c = contours[0];
    check(c.closed && c.wrap_offset.has_value() && c.points.size() == 14 && c.family == 1 &&
          c.owner_flag == 0);
    check(equal(c.points, again[0].points));
    check(c.points.front().transition == 1.f && c.points.front().style_b == 0);
    check(c.points.front().x == c.points[c.points.size() - 2].x &&
          c.points.front().y == c.points[c.points.size() - 2].y);
    masks = island(2, false, true);
    graph = CoastMapGraph::build(source(masks));
    bool f0 = false, f1 = false;
    for (std::size_t i = 0; i < graph.links().size(); ++i)
        for (std::size_t k = 0; k < graph.links()[i].size(); ++k) {
            const bool family = graph.initial().cells()[i].segments[k].direction >= 12;
            f0 |= !family;
            f1 |= family;
            for (auto n : {graph.links()[i][k].forward, graph.links()[i][k].reverse}) {
                check(n.edge >= 0);
                check((graph.initial().cells()[n.y * 5 + (n.x % 5 + 5) % 5].segments[n.edge].direction >=
                       12) == family);
            }
        }
    check(f0 && f1);
    masks = island();
    s = source(masks);
    s.wrap_x = false;
    graph = CoastMapGraph::build(s);
    check(graph.scan(graph.smooth(s.precision)).size() == 1);
    // Different topology must not borrow links, even if dimensions agree.
    auto other_masks = island(0);
    auto other = CoastMapGraph::build(source(other_masks));
    rejects([&] { graph.scan(other.initial()); });
    masks.assign(1, 0x2000);
    s = source(masks, 1, 1);
    s.wrap_x = false;
    graph = CoastMapGraph::build(s);
    check(graph.links()[0].empty() && graph.scan(graph.smooth(s.precision)).empty());
    s.width = 0;
    rejects([&] { CoastMapGraph::build(s); });
    s.width = 256;
    rejects([&] { CoastMapGraph::build(s); });
    s = source(masks);
    rejects([&] { CoastMapGraph::build(s); });
    s = source(masks, 1, 1);
    s.precision = static_cast<X87Precision>(99);
    rejects([&] { CoastMapGraph::build(s); });
    s = source(masks, 1, 1);
    s.transform_path = static_cast<Vec4Path>(99);
    rejects([&] { CoastMapGraph::build(s); });
    rejects([&] { graph.smooth(static_cast<X87Precision>(99)); });
    s = source(masks, 1, 1);
    s.world[0] = std::numeric_limits<float>::infinity();
    rejects([&] { CoastMapGraph::build(s); });
    masks = island(2, true);
    s = source(masks);
    s.world.fill(0.f);
    graph = CoastMapGraph::build(s);
    rejects([&] { graph.smooth(s.precision); });
}
std::vector<CoastControlPoint> points(unsigned n) {
    std::vector<CoastControlPoint> out;
    for (unsigned i = 0; i < n; ++i)
        out.push_back({i * 3.125f, (i % 3) * .375f + i * .625f, float(i % 7) / 6.f, std::uint8_t(i % 13),
                       std::uint8_t((i + 1) % 13)});
    return out;
}
std::vector<std::uint32_t> randoms(unsigned n) {
    std::vector<std::uint32_t> out;
    for (unsigned i = 0; i < n; ++i)
        out.push_back((i * 919 + 1357) % 32768);
    return out;
}
void perturb_cases() {
    // A continuous stream must carry its cursor over both short and long
    // contours, including the original deliberately ignored random draws.
    for (auto seed : {0u, 1u, 0xffffffffu}) {
        TerrainRandom stream_random(seed), explicit_random(seed);
        for (unsigned n : {4, 7, 17, 830, 5, 2048}) {
            auto lookahead = explicit_random;
            std::vector<std::uint32_t> values;
            for (unsigned i = 0; i < 8192; ++i) values.push_back(lookahead.next());
            const auto expected = perturb_coast(points(n), true, {0, 0}, values,
                                                X87Precision::double_precision, CoastPowPath::legacy_x87);
            const auto actual = perturb_coast(points(n), true, {0, 0}, stream_random,
                                              X87Precision::double_precision, CoastPowPath::legacy_x87);
            for (unsigned i = 0; i < expected.random_calls; ++i) explicit_random.next();
            check(equal(expected.points, actual.points) && actual.random_calls == expected.random_calls);
            check(stream_random.state() == explicit_random.state() && stream_random.calls() == explicit_random.calls());
        }
    }
    const Point2 wrap{13.25f, -7.125f};
    auto stream = randoms(5000);
    const auto apply = [&](unsigned n, bool closed = false) {
        return perturb_coast(points(n), closed, wrap, stream, X87Precision::double_precision,
                             CoastPowPath::sse2);
    };
    for (unsigned n : {4, 5}) {
        const auto r = apply(n);
        check(r.levels == 0 && r.random_calls == 0 && r.padded_point_count == n &&
              equal(r.points, points(n)));
    }
    for (auto c : std::array<std::array<unsigned, 4>, 10>{{{6, 1, 6, 6},
                                                           {7, 1, 8, 8},
                                                           {8, 1, 8, 8},
                                                           {10, 2, 10, 16},
                                                           {11, 2, 14, 22},
                                                           {17, 3, 18, 34},
                                                           {18, 3, 18, 34},
                                                           {33, 4, 34, 68},
                                                           {34, 4, 34, 68},
                                                           {65, 4, 66, 128}}}) {
        const auto r = apply(c[0]);
        check(r.levels == c[1] && r.padded_point_count == c[2] && r.random_calls == c[3] &&
              r.points.size() == c[0]);
    }
    for (unsigned n : {4, 7, 17, 2048}) {
        const auto open = apply(n), closed = apply(n, true);
        const auto original = points(n);
        for (unsigned i = 0; i < n; ++i) {
            check(closed.points[i].transition == original[i].transition &&
                  closed.points[i].style_a == original[i].style_a &&
                  closed.points[i].style_b == original[i].style_b);
            if (i < 2)
                check(closed.points[i].x == open.points[n - 2 + i].x - wrap.x &&
                      closed.points[i].y == open.points[n - 2 + i].y - wrap.y);
            else
                check(equal(std::span(open.points).subspan(i, 1), std::span(closed.points).subspan(i, 1)));
        }
    }
    auto before = apply(17);
    for (unsigned i : {5, 15, 33})
        stream[i] = (stream[i] + 12345) % 32768;
    check(equal(before.points, apply(17).points));
    ++stream[32];
    check(!equal(before.points, apply(17).points));
    const auto legacy =
        perturb_coast(points(17), false, wrap, stream, X87Precision::single, CoastPowPath::legacy_x87);
    const auto sse = perturb_coast(points(17), false, wrap, stream, X87Precision::single, CoastPowPath::sse2);
    check(std::bit_cast<std::uint32_t>(legacy.initial_amplitude) == 0x3f1bc01a &&
          std::bit_cast<std::uint32_t>(sse.initial_amplitude) == 0x3f1bc019);
    check(!equal(legacy.points, sse.points));
    stream = randoms(6);
    before = apply(6);
    stream.push_back(0xffffffff);
    check(equal(before.points, apply(6).points));
    stream.resize(5);
    rejects([&] { apply(6); });
    stream = randoms(6);
    stream.back() = 32768;
    rejects([&] { apply(6); });
    rejects([&] { apply(3); });
    rejects([&] { apply(max_coast_contour_controls + 1); });
    auto degenerate = points(6);
    for (auto &p : degenerate)
        p.x = p.y = 0.f;
    stream = randoms(6);
    rejects(
        [&] { perturb_coast(degenerate, false, wrap, stream, X87Precision::single, CoastPowPath::sse2); });
    auto invalid = points(4);
    invalid[0].transition = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { perturb_coast(invalid, false, wrap, stream, X87Precision::single, CoastPowPath::sse2); });
    rejects([&] {
        perturb_coast(points(4), false, wrap, stream, static_cast<X87Precision>(99), CoastPowPath::sse2);
    });
    rejects([&] {
        perturb_coast(points(4), false, wrap, stream, X87Precision::single, static_cast<CoastPowPath>(99));
    });
}
} // namespace
int main() {
    try {
        grid_cases();
        perturb_cases();
        std::cout << "PASS " << checks << " coast source graph/contour/perturbation checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
