#include "civ5/terrain/worker_geometry.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <source_location>
#include <stdexcept>
using namespace civ5::terrain;
namespace {
int checks = 0;
void check(bool ok, std::source_location where = std::source_location::current()) {
    ++checks;
    if (!ok)
        throw std::runtime_error("Worker geometry check failed at line " + std::to_string(where.line()));
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
void same(std::span<const Point2> a, std::span<const Point2> b) {
    check(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        check(std::bit_cast<std::uint32_t>(a[i].x) == std::bit_cast<std::uint32_t>(b[i].x));
        check(std::bit_cast<std::uint32_t>(a[i].y) == std::bit_cast<std::uint32_t>(b[i].y));
    }
}
CoastCurveSet record(std::span<const Point2> points, std::uint32_t owner = 0) {
    CoastCurves curves;
    for (int band = 0; band < 5; ++band)
        for (auto p : points)
            curves[band].push_back({p.x + (band == 2 ? 0.f : (band + 1) * 1000.f), p.y, 0.f, 0, 0});
    return CoastCurveSet(std::move(curves), owner);
}
} // namespace
int main() {
    try {
        const std::vector<Point2> straight{{0, 10}, {10, 10}, {20, 10}};
        std::vector<CoastCurveSet> records{record(straight, 1)};
        const std::vector<Point2> expected{{0, 10}, {10, 10}, {20, 10}, {5, 11}, {15, 11}, {5, 9}, {15, 9}};
        for (auto pc : {X87Precision::single, X87Precision::double_precision}) {
            auto geometry = build_worker_geometry(records, pc);
            same(geometry.points, expected);
            check(geometry.boundaries == std::vector<WorkerGateRange>{{0, 3, 1}});
            check(geometry.fills == std::vector<WorkerGateRange>{{3, 5, 1}, {5, 7, 2}});
            const std::vector<Point2> xy{{0, 0}, {20, 0}, {20, 20}, {0, 20}, {0, 10}, {30, 10}, {30, 30}};
            auto repaired_expected = xy;
            const std::vector<Point2> fills{
                {10, 1},  {19, 10}, {16.826086f, 12.173913f}, {16.826086f, 12.173913f}, {29, 20}, {29, 20},
                {10, -1}, {21, 10}, {19.148148f, 11.851851f}, {19.148148f, 11.851851f}, {31, 20}, {31, 20}};
            repaired_expected.insert(repaired_expected.end(), fills.begin(), fills.end());
            const std::array curve{record(xy)};
            same(build_worker_geometry(curve, pc).points,
                 repaired_expected); // Independent original-code witness.
        }
        records = {record(straight), record(std::vector<Point2>{{0, 30}, {10, 30}, {20, 30}}, 1)};
        auto geometry = build_worker_geometry(records, X87Precision::double_precision);
        check(geometry.points.size() == 14);
        check(geometry.boundaries == std::vector<WorkerGateRange>{{0, 3, 0}, {3, 6, 1}});
        check(geometry.fills ==
              std::vector<WorkerGateRange>{{6, 8, 0}, {8, 10, 1}, {10, 12, 1}, {12, 14, 2}});
        auto source = records[0].curves();
        const CoastCurveSet owned(source, 0);
        source[2].clear();
        check(owned.control_count() == 3);
        source = records[0].curves();
        for (auto &c : source)
            for (auto &p : c) {
                p.transition = .731f;
                p.style_a = 12;
                p.style_b = 7;
            }
        const std::array changed{CoastCurveSet(std::move(source), 0)}, original{owned};
        same(build_worker_geometry(changed, X87Precision::single).points,
             build_worker_geometry(original, X87Precision::single).points);
        const std::array closed{
            record(std::vector<Point2>{{12, 12}, {42, 12}, {42, 42}, {12, 42}, {12, 12}})};
        geometry = build_worker_geometry(closed, X87Precision::double_precision);
        IntMatrix4 identity{};
        identity[0] = identity[5] = identity[10] = identity[15] = 1;
        Matrix4 world{};
        world[12] = world[13] = 32;
        world[15] = 1;
        const std::array<std::uint32_t, 1> masks{1};
        const WorkerGateSource grid{
            1, 1, masks, identity, identity, world, true, 255, X87Precision::double_precision, Vec4Path::sse};
        const auto routing = route_worker_gates(HeightDomain(64, 64), grid, geometry.points,
                                                geometry.boundaries, geometry.fills);
        const auto gates = routing.build_gate_field64();
        check(std::count(gates.begin(), gates.end(), 0) == 961 &&
              std::count(gates.begin(), gates.end(), 1) == 3135);
        std::vector<Point2> large;
        for (int i = 0; i < 16370; ++i)
            large.emplace_back(float(i), 0.f);
        const std::array max_curve{record(large)};
        const auto maximum = build_worker_geometry(max_curve, X87Precision::single);
        check(maximum.points.size() == 49108 && maximum.boundaries[0].end == 16370);
        for (auto p : maximum.points)
            check(std::isfinite(p.x) && std::isfinite(p.y));
        // Original 0x4a5b63..0x4a6597 skips every output loop with zero records.
        for (auto pc : {X87Precision::single, X87Precision::double_precision}) {
            const auto empty = build_worker_geometry({}, pc);
            check(empty.points.empty() && empty.boundaries.empty() && empty.fills.empty());
        }
        rejects([&] { build_worker_geometry(original, static_cast<X87Precision>(2)); });
        const std::array unknown{record(straight, 2)};
        rejects([&] { build_worker_geometry(unknown, X87Precision::single); });
        const std::array zero{record(std::vector<Point2>{{0, 0}, {0, 0}, {10, 10}})};
        rejects([&] { build_worker_geometry(zero, X87Precision::single); });
        records.assign(257, owned);
        check(build_worker_geometry(records, X87Precision::single).points.size() == 257 * 7);
        records = {max_curve[0], max_curve[0]};
        check(build_worker_geometry(records, X87Precision::single).points.size() == 98216);
        source = owned.curves();
        source[0][0].x = 65537;
        const std::array unsupported{CoastCurveSet(source, 0)};
        rejects([&] { build_worker_geometry(unsupported, X87Precision::single); });
        // A generated side would leave the supported coordinate bound; do not clamp it.
        CoastCurves edge;
        for (auto &c : edge)
            for (int i = 0; i < 3; ++i)
                c.push_back({float(i) * 10, 65536, 0, 0, 0});
        const std::array edge_set{CoastCurveSet(edge, 0)};
        rejects([&] { build_worker_geometry(edge_set, X87Precision::single); });
        std::cout << "PASS " << checks << " worker geometry checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
