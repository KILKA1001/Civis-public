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
        throw std::runtime_error("Worker field check failed");
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
std::shared_ptr<const TerrainPieceGrids> grids(int size, std::uint16_t height, std::uint8_t material,
                                               std::uint8_t weight, std::uint8_t priority) {
    return std::make_shared<const TerrainPieceGrids>(size, std::vector<std::uint16_t>(size * size, height),
                                                     std::vector<std::uint8_t>(size * size, material),
                                                     std::vector<std::uint8_t>(size * size, weight),
                                                     std::vector<std::uint8_t>(size * size, priority));
}
} // namespace
int main() {
    try {
        const HeightDomain domain(256, 128);
        std::vector<std::uint8_t> gates(4096, 2);
        std::vector<TerrainDescriptor> descriptors{{{0, 0, 128, 128}, 1, 2, grids(128, 0, 7, 5, 128)}};
        std::vector<std::uint32_t> packet{0};
        auto state = accumulate_worker64(domain, 0, 0, descriptors, packet, gates, worker_retail_bias);
        for (const auto &c : state.cells()) {
            check(std::bit_cast<std::uint32_t>(c.numerator) == 0xbb97354c);
            check(c.material == 7 && c.priority == 128);
        }
        auto empty = accumulate_worker64(domain, 0, 0, {}, {}, gates, worker_retail_bias);
        for (auto pc : {X87Precision::single, X87Precision::double_precision})
            for (auto h : empty.finalize(pc))
                check(h == 15423);
        for (auto c : empty.cells())
            check(c.numerator == 0 && c.denominator == 1e-6f && c.material == 3 && c.priority == 0);
        descriptors.push_back({{0, 0, 128, 128}, 1, 2, grids(128, 65535, 8, 0, 255)});
        packet.push_back(1);
        auto prioritized = accumulate_worker64(domain, 0, 0, descriptors, packet, gates, worker_retail_bias);
        check(prioritized.cells()[0].material == 8 && prioritized.cells()[0].priority == 255);
        check(prioritized.cells()[0].numerator == state.cells()[0].numerator);
        descriptors.push_back({{0, 0, 128, 128}, 1, 2, grids(128, 12345, 9, 0, 255)});
        packet.push_back(2);
        check(accumulate_worker64(domain, 0, 0, descriptors, packet, gates, worker_retail_bias)
                  .cells()[0]
                  .material == 8);
        std::reverse(packet.begin(), packet.end());
        check(accumulate_worker64(domain, 0, 0, descriptors, packet, gates, worker_retail_bias)
                  .cells()[0]
                  .material == 9);
        descriptors[0].cell_gate = 258;
        packet = {0};
        check(accumulate_worker64(domain, 0, 0, descriptors, packet, gates, worker_retail_bias)
                  .cells()[0]
                  .material == 3);
        descriptors[0].cell_gate = 2;
        // Strict boundary: a modulo-based implementation would contribute at X=0.
        descriptors[0].rectangle = {64, 0, 192, 128};
        check(
            accumulate_worker64(HeightDomain(128, 128), 0, 0, descriptors, packet, gates, worker_retail_bias)
                .cells()[0]
                .material == 3);
        descriptors[0].rectangle = {65, 0, 193, 128};
        check(
            accumulate_worker64(HeightDomain(128, 128), 0, 0, descriptors, packet, gates, worker_retail_bias)
                .cells()[0]
                .material == 7);
        // Routing preserves endpoint visits, negative truncation and duplicates.
        std::vector<SpatialRectangle> rectangles{{-63, -63, 64, 64}};
        auto routed = route_worker_descriptors(domain, rectangles);
        check(routed.size() == 8);
        for (int i = 0; i < 8; ++i)
            check(routed[i] == (i % 4 < 2 ? std::vector<std::uint32_t>{0} : std::vector<std::uint32_t>{}));
        rectangles = {{0, 0, 128, 128}};
        routed = route_worker_descriptors(HeightDomain(128, 128), rectangles);
        check(routed[0] == std::vector<std::uint32_t>({0, 0}) && routed[1] == std::vector<std::uint32_t>{0});
        auto field = compose_worker_field(domain, {}, std::vector<std::uint8_t>(256 * 128, 2),
                                          worker_retail_bias, X87Precision::single);
        auto height_field = field.height_field();
        for (int y = 0; y < 128; ++y)
            for (int x = 0; x < 256; ++x) {
                check(field.heights()[y * 256 + x] == 15423 && field.materials()[y * 256 + x] == 3);
                check(height_field.u16_at(x, y) == 15423 && height_field.a8_at(x, y) == 60);
            }
        // Source and returned result lifetimes do not borrow caller buffers.
        std::vector<std::uint16_t> h(16384, 55);
        std::vector<std::uint8_t> b(16384, 4);
        TerrainPieceGrids owned(128, h, b, b, b);
        h[0] = 999;
        b[0] = 99;
        check(owned.heights()[0] == 55 && owned.materials()[0] == 4 && owned.weights()[0] == 4 &&
              owned.priorities()[0] == 4);
        rejects([&] { TerrainPieceGrids invalid(64, h, b, b, b); });
        rejects([&] { TerrainPieceGrids invalid(128, std::span(h).first(1), b, b, b); });
        rejects([&] { route_worker_descriptors(HeightDomain(80, 64), {}); });
        rejects([&] { route_worker_descriptors(HeightDomain(65600, 64), {}); });
        rectangles = {{0, 0, 0, 64}};
        rejects([&] { route_worker_descriptors(domain, rectangles); });
        rectangles = {{std::numeric_limits<int>::min(), 0, std::numeric_limits<int>::max(), 64}};
        rejects([&] { route_worker_descriptors(domain, rectangles); });
        rectangles.assign(257, {1, 1, 63, 63});
        rejects([&] { route_worker_descriptors(domain, rectangles); });
        rectangles.assign(65537, {1, 1, 2, 2});
        rejects([&] { route_worker_descriptors(domain, rectangles); });
        rejects([&] { accumulate_worker64(domain, 1, 0, {}, {}, gates, worker_retail_bias); });
        rejects([&] { accumulate_worker64(domain, 0, -64, {}, {}, gates, worker_retail_bias); });
        rejects([&] { accumulate_worker64(domain, 256, 0, {}, {}, gates, worker_retail_bias); });
        rejects([&] { accumulate_worker64(domain, 0, 0, {}, {}, {}, worker_retail_bias); });
        rejects([&] {
            accumulate_worker64(domain, 0, 0, {}, {}, gates, std::numeric_limits<float>::infinity());
        });
        rejects([&] { accumulate_worker64(domain, 0, 0, {}, packet, gates, worker_retail_bias); });
        auto invalid = descriptors;
        invalid[0].grids.reset();
        rejects([&] { accumulate_worker64(domain, 0, 0, invalid, packet, gates, worker_retail_bias); });
        invalid = descriptors;
        invalid[0].record_state_flag = 0;
        rejects([&] { accumulate_worker64(domain, 0, 0, invalid, packet, gates, worker_retail_bias); });
        invalid = descriptors;
        invalid[0].rectangle = {std::numeric_limits<int>::min(), 0, std::numeric_limits<int>::max(), 128};
        rejects([&] { accumulate_worker64(domain, 0, 0, invalid, packet, gates, worker_retail_bias); });
        invalid = descriptors;
        invalid[0].rectangle = {65, 0, 193, 128};
        rejects([&] { accumulate_worker64(domain, 0, 0, invalid, packet, gates, worker_retail_bias); });
        invalid = descriptors;
        invalid[0].rectangle = {0, 256, 128, 384};
        rejects([&] { accumulate_worker64(domain, 0, 0, invalid, packet, gates, worker_retail_bias); });
        packet.assign(257, 0);
        rejects([&] { accumulate_worker64(domain, 0, 0, descriptors, packet, gates, worker_retail_bias); });
        rejects([&] { empty.finalize(static_cast<X87Precision>(999)); });
        rejects(
            [&] { accumulate_worker64(domain, 0, 0, {}, {}, gates, 1e30f).finalize(X87Precision::single); });
        rejects([&] { compose_worker_field(domain, {}, gates, worker_retail_bias, X87Precision::single); });
        // Out-of-U16 values retain low bits; they are not saturated to 0xffff.
        auto over =
            accumulate_worker64(domain, 0, 0, {}, {}, gates, 2.f).finalize(X87Precision::double_precision);
        check(over[0] == 65534);
        auto negative =
            accumulate_worker64(domain, 0, 0, {}, {}, gates, -1.f).finalize(X87Precision::double_precision);
        check(negative[0] == 1);
        std::cout << "PASS " << checks << " worker field checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
