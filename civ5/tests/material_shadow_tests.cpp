#include "civ5/materials/material_shadows.hpp"
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace civ5::materials;
namespace {
int checks = 0;
void check(bool ok) {
    ++checks;
    if (!ok)
        throw std::runtime_error("Material shadow contract failed");
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
} // namespace
int main() {
    try {
        check(std::isinf(material_shadow_sample_weight(0)) && material_shadow_sample_weight(0) > 0);
        for (auto n : {1u, 2u, 3u, 4u, 8u, 16u, 127u, 16777217u, 0x80000000u, 0xffffffffu})
            check(material_shadow_sample_weight(n) == static_cast<float>(1.0 / static_cast<double>(n)));
        std::array<std::uint8_t, 0x4c0> prior;
        for (unsigned i = 0; i < prior.size(); ++i)
            prior[i] = static_cast<std::uint8_t>(i * 13 + 7);
        const auto out = material_shadow_draw_block(3, {-16777217, 2147483647}, {-.125f, .5f}, prior);
        const std::array<unsigned, 7> offsets{0, 4, 0x490, 0x494, 0x4b0, 0x4b4, 0x4b8};
        const std::array<float, 7> expected{0, 0, -16777216.f, 2147483648.f, -.125f, .5f, 1.f / 3};
        for (unsigned i = 0; i < prior.size(); ++i) {
            bool written = false;
            for (unsigned j = 0; j < 7; ++j)
                if (i >= offsets[j] && i < offsets[j] + 4) {
                    const auto bits = std::bit_cast<std::uint32_t>(expected[j]);
                    check(out[i] == static_cast<std::uint8_t>(bits >> (8 * (i - offsets[j]))));
                    written = true;
                    break;
                }
            if (!written)
                check(out[i] == prior[i]);
            check(prior[i] == static_cast<std::uint8_t>(i * 13 + 7));
        }
        rejects([&] { material_shadow_draw_block(0, {0, 0}, {0, 0}, prior); });
        for (auto value : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()})
            rejects([&] { material_shadow_draw_block(1, {0, 0}, {0, value}, prior); });
        const auto p = material_shadow_parameters({1, .866f, .2f}, {1, 0, .5f}, {1.f / 64, 1.f / 64}, {0, 0},
                                                  {.125f, -.25f}, 4, 93);
        check(p.jitter_add == std::array<float, 4>{.125f, -.25f, .25f, 93});
        std::vector<std::array<float, 2>> visited;
        const auto flat = material_shadow_shade(3, 5, p, [&](float u, float v) {
            visited.push_back({u, v});
            return .375f;
        });
        check(flat == 0 && visited.size() == 41);
        check(visited[0] == std::array<float, 2>{3.f / 64, 5.f / 64});
        for (unsigned i = 1; i < 41; ++i)
            check(visited[i] == std::array<float, 2>{(4.f + i) / 64, 5.f / 64});
        const auto water = material_shadow_shade(0, 0, p, [](float u, float) { return u == 0 ? 0.f : .5f; });
        check(std::abs(water - ((.5f * 255 - 64) * .010416667f) * .25f) < 1e-7f);
        check(material_shadow_shade(0, 0, p, [](float u, float) { return u == 1.f / 64 ? 1.f : 0.f; }) == 0);
        rejects([&] { material_shadow_shade(0, 0, p, {}); });
        rejects([&] {
            material_shadow_shade(std::numeric_limits<float>::infinity(), 0, p,
                                  [](float, float) { return 0.f; });
        });
        auto invalid = p;
        invalid.sun_direction = {0, 0, 1};
        rejects([&] { invalid.validate(); });
        invalid = p;
        invalid.world_scales[1] = 0;
        rejects([&] { invalid.validate(); });
        invalid = p;
        invalid.jitter_add[3] = std::numeric_limits<float>::quiet_NaN();
        rejects([&] { invalid.validate(); });
        rejects([&] { material_shadow_parameters({1, 1, 1}, {1, 0, 0}, {1, 1}, {0, 0}, {0, 0}, 0); });
        std::cout << "PASS material shadow checks=" << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
