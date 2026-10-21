#include "civ5/terrain/worker_field.hpp"
#include "civ5/terrain/worker_gates.hpp"
#include <algorithm>
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
        throw std::runtime_error("Worker gate check failed at line " + std::to_string(where.line()));
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
IntMatrix4 identity_int() {
    IntMatrix4 m{};
    m[0] = m[5] = m[10] = m[15] = 1;
    return m;
}
WorkerGateSource source(std::span<const std::uint32_t> masks) {
    auto world = Matrix4{};
    world[12] = world[13] = 16;
    world[15] = 1;
    return {1,
            1,
            masks,
            identity_int(),
            identity_int(),
            world,
            true,
            255,
            X87Precision::double_precision,
            Vec4Path::sse};
}
} // namespace
int main() {
    try {
        const HeightDomain domain(128, 128);
        for (auto g : classify_worker_gates64(domain, 0, 0, {}, {}, {}))
            check(g == 2);
        std::vector<WorkerGateSeed> seeds{{2, 2, 0}};
        for (auto g : classify_worker_gates64(domain, 0, 0, {}, {}, seeds))
            check(g == 0);
        const std::vector<Point2> square{{12, 12}, {42, 12}, {42, 42}, {12, 42}, {12, 12}};
        std::vector<WorkerGateLine> boundaries{{square, 1}};
        seeds = {{20, 20, 0}};
        auto gates = classify_worker_gates64(domain, 0, 0, boundaries, {}, seeds);
        check(std::count(gates.begin(), gates.end(), 0) == 841);
        check(std::count(gates.begin(), gates.end(), 1) == 120);
        check(std::count(gates.begin(), gates.end(), 2) == 3135);
        boundaries.push_back({square, 0x123400ff});
        gates = classify_worker_gates64(domain, 0, 0, boundaries, {}, seeds);
        check(std::count(gates.begin(), gates.end(), 255) == 120);
        seeds.push_back({20, 20, 1});
        check(classify_worker_gates64(domain, 0, 0, boundaries, {}, seeds) == gates);
        const std::vector<Point2> wall{{32, 0}, {32, 63}}, point{{-6400, 10}, {-6400, 10}};
        boundaries = {{wall, 1}};
        std::vector<WorkerGateLine> fills{{point, 0}};
        const HeightDomain small(64, 64);
        const auto normalized = classify_worker_gates64(small, 0, 0, boundaries, fills, {});
        check(std::count(normalized.begin(), normalized.end(), 0) == 4032);
        seeds = {{-6400, 10, 0}};
        const auto retained = classify_worker_gates64(small, 0, 0, boundaries, {}, seeds);
        check(std::count(retained.begin(), retained.end(), 0) == 2048);
        seeds = {{-6401, 0, 1}};
        for (auto g : classify_worker_gates64(small, 0, 0, {}, {}, seeds))
            check(g == 2);
        const std::vector<Point2> single{{2, 2}};
        fills = {{single, 1}};
        for (auto g : classify_worker_gates64(domain, 0, 0, {}, fills, {}))
            check(g == 2);
        const std::vector<Point2> duplicate{{2, 2}, {2, 2}};
        fills = {{duplicate, 255}};
        for (auto g : classify_worker_gates64(domain, 0, 0, {}, fills, {}))
            check(g == 255);
        fills[0].gate = 260;
        rejects([&] { classify_worker_gates64(domain, 0, 0, {}, fills, {}); });
        seeds = {{0, 0, 260}};
        rejects([&] { classify_worker_gates64(domain, 0, 0, {}, {}, seeds); });
        seeds = {{65537, 0, 0}};
        rejects([&] { classify_worker_gates64(domain, 0, 0, {}, {}, seeds); });
        seeds.assign(17, {0, 0, 0});
        rejects([&] { classify_worker_gates64(domain, 0, 0, {}, {}, seeds); });
        boundaries.assign(33, {single, 1});
        rejects([&] { classify_worker_gates64(domain, 0, 0, boundaries, {}, {}); });
        boundaries = {{std::span<const Point2>{}, 1}};
        rejects([&] { classify_worker_gates64(domain, 0, 0, boundaries, {}, {}); });
        std::vector<Point2> huge(32769, Point2(0, 0));
        boundaries = {{huge, 1}};
        const auto long_line = classify_worker_gates64(domain, 0, 0, boundaries, {}, {});
        check(long_line[0] == 1 && std::count(long_line.begin(), long_line.end(), 2) == 4095);
        huge.resize(32768, Point2(0, 0));
        for (std::size_t i = 0; i < huge.size(); ++i)
            huge[i].x = i % 2 ? 65536.f : -65536.f;
        boundaries = {{huge, 1}};
        rejects([&] { classify_worker_gates64(domain, 0, 0, boundaries, {}, {}); });
        huge.resize(1, Point2(0, 0));
        huge[0].x = std::numeric_limits<float>::infinity();
        boundaries = {{huge, 1}};
        rejects([&] { classify_worker_gates64(domain, 0, 0, boundaries, {}, {}); });
        huge[0].x = std::numeric_limits<float>::quiet_NaN();
        rejects([&] { classify_worker_gates64(domain, 0, 0, boundaries, {}, {}); });
        huge[0].x = 65537;
        rejects([&] { classify_worker_gates64(domain, 0, 0, boundaries, {}, {}); });
        for (const auto [x, y] : std::array<std::pair<int, int>, 4>{{{-64, 0}, {0, 128}, {1, 0}, {0, 65}}})
            rejects([&] { classify_worker_gates64(domain, x, y, {}, {}, {}); });
        rejects([&] { classify_worker_gates64(HeightDomain(80, 64), 0, 0, {}, {}, {}); });
        rejects([&] { classify_worker_gates64(HeightDomain(65552, 64), 0, 0, {}, {}, {}); });

        std::vector<std::uint32_t> masks{0x2000};
        auto s = source(masks);
        auto routed = route_worker_gates(domain, s, {}, {}, {});
        check(routed.logical_categories()[0] == 1 && routed.workers().size() == 4);
        const std::vector<WorkerGateSeed> expected_seeds{{17, 17, 1}, {15, 17, 1}, {17, 15, 1}, {15, 15, 1}};
        check(routed.workers()[0].seeds == expected_seeds);
        check(routed.workers()[1].seeds.empty());
        masks[0] = 1;
        check(routed.logical_categories()[0] == 1); // Result owns category bytes.
        rejects([&] { routed.classify_worker64(4); });
        const auto whole = routed.build_gate_field64();
        for (int y = 0; y < 128; ++y)
            for (int x = 0; x < 128; ++x)
                check(whole[y * 128 + x] == (x < 64 && y < 64 ? 1 : 2));
        for (std::uint32_t bit = 0; bit < 32; ++bit) {
            masks[0] = 1u << bit;
            const auto c = route_worker_gates(domain, s, {}, {}, {}).logical_categories()[0];
            check(c == (bit == 14 ? 2 : (bit == 13 || bit == 20 || bit == 26 || bit == 27) ? 1 : 0));
        }
        masks[0] = 0;
        check(route_worker_gates(domain, s, {}, {}, {}).logical_categories()[0] == 2);
        masks = {1, 0, 0x2000, 1};
        s = source(masks);
        s.width = s.height = 2;
        routed = route_worker_gates(domain, s, {}, {}, {});
        check(std::vector<std::uint8_t>(routed.logical_categories().begin(),
                                        routed.logical_categories().end()) ==
              std::vector<std::uint8_t>({0, 1, 2, 0})); // X-major source -> row-major categories.
        masks.resize(1);
        masks[0] = 1;
        s = source(masks);
        std::vector<Point2> ps{{1, 1}, {193, 1}, {1, 1}, {-6500, -1}};
        std::vector<WorkerGateRange> bs{{0, 3, 1}, {0, 3, 255}, {3, 4, 2}, {0, 0, 0}};
        routed = route_worker_gates(HeightDomain(256, 128), s, ps, bs, {});
        check(routed.workers()[0].boundaries == std::vector<WorkerGateRange>{{0, 3, 1}});
        check(routed.workers()[1].boundaries.empty() && routed.workers()[2].boundaries.empty());
        check(routed.workers()[3].boundaries == std::vector<WorkerGateRange>{{0, 3, 1}});
        check(routed.workers()[6].boundaries == std::vector<WorkerGateRange>{{3, 4, 2}});
        auto before = routed.build_gate_field64();
        ps[0].x = 64;
        bs[0].gate = 3;
        check(before == routed.build_gate_field64()); // Owned range geometry survives caller mutation.
        s.world_transform[12] = -65;
        s.world_transform[13] = 65;
        routed = route_worker_gates(domain, s, {}, {}, {});
        check(routed.workers()[1].seeds ==
              std::vector<WorkerGateSeed>{{-64, 66, 0}, {-66, 66, 0}, {-64, 64, 0}, {-66, 64, 0}});
        s.world_transform[13] = 0;
        rejects([&] { route_worker_gates(domain, s, {}, {}, {}); });
        s = source(masks);
        s.world_transform[12] = 129;
        const auto clipped = route_worker_gates(domain, s, {}, {}, {});
        for (const auto &w : clipped.workers())
            check(w.seeds.empty());
        s = source(masks);
        s.map_to_logical[12] = 1;
        s.wrap_x = false;
        check(route_worker_gates(domain, s, {}, {}, {}).workers()[0].seeds[0].gate == 255);
        s.wrap_x = true;
        check(route_worker_gates(domain, s, {}, {}, {}).workers()[0].seeds[0].gate == 0);
        s.map_to_logical[13] = -1;
        check(route_worker_gates(domain, s, {}, {}, {}).workers()[0].seeds[0].gate == 255);
        s = source(masks);
        s.logical_to_map[12] = std::numeric_limits<int>::max();
        s.logical_to_map[13] = 1;
        s.map_to_logical[0] = std::numeric_limits<int>::max();
        check(route_worker_gates(domain, s, {}, {}, {}).workers()[0].seeds[0].gate ==
              255); // Wrapped int math, no UB.
        s = source(masks);
        rejects([&] { route_worker_gates(HeightDomain(65600, 64), s, {}, {}, {}); });
        for (auto value :
             {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN(), 65536.f}) {
            s.world_transform[12] = value;
            rejects([&] { route_worker_gates(domain, s, {}, {}, {}); });
        }
        s = source(masks);
        s.width = 0;
        rejects([&] { route_worker_gates(domain, s, {}, {}, {}); });
        s = source(masks);
        s.masks = {};
        rejects([&] { route_worker_gates(domain, s, {}, {}, {}); });
        s = source(masks);
        s.precision = static_cast<X87Precision>(4);
        rejects([&] { route_worker_gates(domain, s, {}, {}, {}); });
        s = source(masks);
        s.transform_path = static_cast<Vec4Path>(4);
        rejects([&] { route_worker_gates(domain, s, {}, {}, {}); });
        s = source(masks);
        bs = {{1, 0, 0}};
        rejects([&] { route_worker_gates(domain, s, ps, bs, {}); });
        bs = {{0, 5, 0}};
        rejects([&] { route_worker_gates(domain, s, ps, bs, {}); });
        huge.assign(65537, Point2(0, 0));
        check(!route_worker_gates(domain, s, huge, {}, {}).workers().empty());
        huge.resize(65536, Point2(0, 0));
        bs.assign(4096, {0, 65536, 0});
        rejects([&] { route_worker_gates(domain, s, huge, bs, {}); });
        bs.assign(4097, {0, 0, 0});
        check(route_worker_gates(domain, s, {}, bs, {}).workers()[0].boundaries.empty());
        ps.assign(34, Point2(0, 0));
        bs.clear();
        for (unsigned i = 0; i < 33; ++i)
            bs.push_back({i, i + 1, 1});
        rejects([&] { route_worker_gates(domain, s, ps, bs, {}); });
        bs.resize(32);
        check(route_worker_gates(domain, s, ps, bs, {}).workers()[0].boundaries.size() == 32);
        masks.assign(5, 1);
        s = source(masks);
        s.width = 5;
        rejects([&] { route_worker_gates(domain, s, {}, {}, {}); });
        // Real public composition consumes computed gates without an oracle field.
        masks.assign(1, 1);
        s = source(masks);
        const HeightDomain composed_domain(128, 64);
        const auto computed = route_worker_gates(composed_domain, s, {}, {}, {}).build_gate_field64();
        const auto grids = std::make_shared<TerrainPieceGrids>(
            128, std::vector<std::uint16_t>(16384, 40000), std::vector<std::uint8_t>(16384, 7),
            std::vector<std::uint8_t>(16384, 255), std::vector<std::uint8_t>(16384, 9));
        const std::vector<TerrainDescriptor> ds{{{0, 0, 128, 128}, 1, 0, grids}};
        const auto field = compose_worker_field(composed_domain, ds, computed, worker_retail_bias,
                                                X87Precision::double_precision);
        for (int i = 0; i < 8192; ++i)
            check(field.materials()[i] == (i % 128 < 64 ? 7 : 3));
        std::cout << "PASS " << checks << " worker gate/routing checks (64-bit C++)\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
