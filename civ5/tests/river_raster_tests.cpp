#include "civ5/terrain/river_raster.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace civ5::terrain;
namespace {
int checks = 0;
void check(bool ok) {
    ++checks;
    if (!ok) throw std::runtime_error("River raster check failed");
}
template <class F> void rejects(F f) {
    bool rejected = false;
    try { f(); } catch (const std::invalid_argument &) { rejected = true; }
    check(rejected);
}
RiverTriangle triangle(float h, float opacity = 1.f, float x = 2.f, float y = 2.f) {
    return {{x, y, h, .2f, opacity}, {x + 8, y, h, .2f, opacity}, {x, y + 8, h, .2f, opacity}};
}
} // namespace
int main() {
    try {
        const HeightDomain domain(16, 16);
        const std::vector<std::uint16_t> h(256, 45000);
        const std::vector<std::uint8_t> m(256, 7);
        const std::vector<std::uint32_t> codes{0x100030, 0x100030};
        const RiverRasterSettings settings{.23533989489078522f, X87Precision::double_precision,
                                            {7,7,7,7}, {40,41,42,43}, codes, 0x100000};
        auto run = [&](std::vector<RiverTriangle> b, std::vector<RiverTriangle> c) {
            return apply_river_triangles(domain, h, m, b, c, settings);
        };
        const auto empty = run({}, {});
        check(empty.heights == h && empty.materials == m);
        check(std::count(empty.bank_visited.begin(), empty.bank_visited.end(), 1) == 0);
        const auto high = triangle(50000), low = triangle(30000), channel = triangle(25000, .6f);
        const auto single = run({high}, {}), first_high = run({high, low}, {}), first_low = run({low, high}, {});
        check(single.heights == first_high.heights && single.materials == first_high.materials);
        check(first_low.heights != first_high.heights && first_low.materials[34] == 43);
        check(first_high.materials[34] == 7 && first_high.bank_visited[34] == 1);
        const auto carved = run({}, {channel}), twice = run({}, {channel, channel});
        check(carved.heights[34] < 45000 && twice.heights[34] < carved.heights[34]);
        check(carved.materials[34] == 0); // First of duplicate provider codes.
        check(carved.bank_visited == empty.bank_visited);
        for (std::size_t i = 0; i < h.size(); ++i) {
            check(carved.heights[i] <= h[i]);
            check(twice.heights[i] <= carved.heights[i]);
        }
        const auto after_banks = run({low}, {channel});
        check(after_banks.heights[34] < first_low.heights[34] && after_banks.materials[34] == 0);
        const auto wrapped = run({triangle(30000, 1, -2, -2)}, {});
        check(wrapped.bank_visited[14] == 1 && wrapped.bank_visited[255] == 0);
        auto degenerate = high; degenerate.b = degenerate.a; degenerate.c = degenerate.a;
        check(run({degenerate}, {degenerate}).heights == h);
        std::vector<RiverPass> phases; std::vector<RiverWriteChannel> writes;
        const std::array banks{low,high};
        const auto observed = apply_river_triangles(domain,h,m,banks,{},settings,
            [&](RiverPass p,RiverWriteChannel c,int index,std::uint32_t value) {
                check(p == RiverPass::banks && index >= 0 && index < 256);
                check(c != RiverWriteChannel::visited || value == 1);
                writes.push_back(c);
            },{},[&](RiverPass p,const RiverRasterField &f) {
                phases.push_back(p); check(f.heights == first_low.heights);
            });
        check(phases == std::vector{RiverPass::banks,RiverPass::channels});
        const auto visited = std::count(observed.bank_visited.begin(),observed.bank_visited.end(),1);
        check(std::count(writes.begin(),writes.end(),RiverWriteChannel::visited) == visited);
        check(std::count(writes.begin(),writes.end(),RiverWriteChannel::height) == visited);
        check(std::count(writes.begin(),writes.end(),RiverWriteChannel::material) == visited*4);
        check(writes[0] == RiverWriteChannel::visited && writes[1] == RiverWriteChannel::material &&
              writes[5] == RiverWriteChannel::height);
        for (auto component : {0,1,2,3,4}) {
            auto invalid = high;
            auto *value = component == 0 ? &invalid.a.x : component == 1 ? &invalid.a.y :
                          component == 2 ? &invalid.a.height : component == 3 ? &invalid.a.profile : &invalid.a.opacity;
            *value = std::numeric_limits<float>::quiet_NaN();
            rejects([&]{ run({invalid}, {}); });
        }
        rejects([&]{ run({triangle(30000,1,-17,0)},{}); });
        rejects([&]{ run({triangle(30000,1,33,0)},{}); });
        auto too_large = high; too_large.b = {32,65536,30000,.2f,1}; too_large.c = {-16,-65536,30000,.2f,1};
        // Supported coordinate envelope still bounds the raster work.
        too_large.a = {32,-65536,30000,.2f,1};
        const HeightDomain wide(65536,16);
        too_large.a.x = -65536; too_large.b.x = 65536;
        rejects([&]{ apply_river_triangles(wide,std::vector<std::uint16_t>(1048576),
                    std::vector<std::uint8_t>(1048576),std::span(&too_large,1),{},settings); });
        rejects([&]{ apply_river_triangles(domain,std::span(h).first(255),m,{}, {},settings); });
        rejects([&]{ auto s=settings;s.precision=static_cast<X87Precision>(3);apply_river_triangles(domain,h,m,{}, {},s); });
        rejects([&]{ auto s=settings;s.level=std::numeric_limits<float>::infinity();apply_river_triangles(domain,h,m,{}, {},s); });
        rejects([&]{ auto s=settings;const std::vector<std::uint32_t> c(257);s.material_codes=c;apply_river_triangles(domain,h,m,{}, {},s); });
        std::cout << "PASS river raster checks=" << checks << '\n';
        return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
