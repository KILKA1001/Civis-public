#include "civ5/materials/material_cache.hpp"
#include "civ5/materials/material_cache_settings.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace civ5::materials;
namespace {
unsigned checks = 0;
void check(bool ok) {
    ++checks;
    if (!ok)
        throw std::runtime_error("Material cache guard failed");
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
} // namespace
int main() {
    try {
        const auto low = parse_material_cache_ini(
            "[Other]\nTextureQuality=1\n[GraphicsDetailSettings]\n"
            "TextureQuality=0\nTerrainDetailLevel=3\n");
        check(low.texture_quality == 0 && low.detail == 3);
        const auto high = parse_material_cache_ini(
            "[GraphicsDetailSettings]\nTextureQuality=1\n");
        check(high.texture_quality == 1 && !high.detail);
        const auto absent = parse_material_cache_ini(
            "[GraphicsDetailSettings]\nTerrainDetailLevel=3\n");
        check(!absent.texture_quality && absent.detail == 3);
        for (auto invalid : {0u, 16385u, 0xffffffffu}) {
            rejects([&] { material_cache_texture_plan(invalid, 64, false); });
            rejects([&] { material_cache_texture_plan(64, invalid, true); });
            rejects([&] { material_cache_pool_plan(std::array<std::uint32_t, 5>{}, true, invalid); });
        }
        const std::array<std::uint32_t, 5> capacity{8096, 1024, 1024, 1024, 8096};
        for (unsigned i = 0; i < 5; ++i) {
            std::array<std::uint32_t, 5> counts{};
            counts[i] = capacity[i] + 1;
            rejects([&] { material_cache_pool_plan(counts, true, 256); });
            counts[i] = capacity[i];
            const auto plans = material_cache_pool_plan(counts, true, 256);
            check(plans.size() == counts[i]);
            check(plans.front().index == 0 && plans.back().index == counts[i] - 1);
        }
        check(material_cache_pool_plan(std::array<std::uint32_t, 5>{0, 0, 0, 0, 0xffffffffu}, false, 64)
                  .empty());
        for (auto pc : {civ5::terrain::X87Precision::single, civ5::terrain::X87Precision::double_precision}) {
            const auto jitter = material_shadow_jitter(pc);
            check(jitter.front() == std::array<float, 2>{0, 0});
            check(jitter == material_shadow_jitter(pc));
            for (const auto &p : jitter)
                for (float c : p)
                    check(std::isfinite(c) && c >= -.5f && c <= .5f);
        }
        rejects([] { material_shadow_jitter(static_cast<civ5::terrain::X87Precision>(99)); });
        std::cout << "PASS " << checks << " material cache resource guards\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
