#include "civ5/materials/lighting_sources.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace civ5::materials;
namespace {
int checks = 0;
void check(bool ok) {
    ++checks;
    if (!ok)
        throw std::runtime_error("Lighting source contract failed");
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
void sources() {
    const auto &r = lighting_requests();
    check(r.size() == 11 && r[0].native_path == "assets\\terrain\\lighting\\specscreenmap.dds");
    check(r[0].loader == LightingLoader::texture_data);
    for (unsigned i = 1; i < 11; ++i)
        check(r[i].loader == LightingLoader::cube);
    const std::array<std::string, 5> names{"Generic", "America", "Asia", "Africa", "Europe"};
    for (unsigned i = 0; i < 5; ++i) {
        check(r[1 + 2 * i].native_path == "assets\\terrain\\lighting\\Sunlight_" + names[i] + "_Terrain.dds");
        check(r[2 + 2 * i].native_path == "assets\\terrain\\lighting\\Shadowlight_" + names[i] + "_Terrain" +
                                              (i == 1 || i == 2 || i == 4 ? "_2" : "") + ".dds");
    }
    for (auto base : {0u, 0xffff0000u, 0x80000000u, 0xfffffff5u}) {
        unsigned calls = 0;
        const auto handles = resolve_lighting_handles([&](const LightingSource &s) {
            check(&s == &r[calls]);
            return base + calls++;
        });
        check(calls == 11 && handles.spec_screen == base);
        const auto words = handles.renderer_words();
        for (unsigned i = 0; i < 5; ++i) {
            check(words[i] == base + 1 + i * 2 && handles.sunlight[i] == words[i]);
            check(words[5 + i] == base + 2 + i * 2 && handles.shadowlight[i] == words[5 + i]);
        }
    }
    const auto zero = resolve_lighting_handles([](const LightingSource &) { return 0u; });
    check(zero.spec_screen == 0 && zero.renderer_words() == std::array<std::uint32_t, 10>{});
    rejects([] { resolve_lighting_handles({}); });
    rejects([] { LightingCubeAssets::load({}); });
    unsigned calls = 0;
    rejects([&] {
        resolve_lighting_handles([&](const LightingSource &) -> std::uint32_t {
            if (++calls == 3)
                throw std::invalid_argument("Explicit resolver failure");
            return 0u;
        });
    });
    check(calls == 3);
}
void bindings() {
    MaterialLightingResources input{0,
                                    0xffff0123u,
                                    0x1234ffffu,
                                    0x80000000u,
                                    0xffffffffu,
                                    {10, 11, 12, 13, 14},
                                    {20, 21, 22, 23, 24},
                                    {30, 31, 32, 33, 34},
                                    {40, 41, 42, 43, 44},
                                    0xffffu,
                                    0x12340007u,
                                    0xfffe0001u};
    for (unsigned low = 0; low < 32; ++low)
        for (auto high : {0u, 0xffffffe0u}) {
            input.mask = low | high;
            unsigned expected = 0;
            for (unsigned i = 4; i > 0; --i)
                if (low & (1u << i))
                    expected = i;
            const auto b = material_lighting_bindings(input);
            check(b.style == expected && material_lighting_style(input.mask) == expected);
            check(b.pixel_textures == std::array<std::uint16_t, 12>{
                                          0x123, 0xffff, 0xffff, 0, static_cast<std::uint16_t>(10 + expected),
                                          static_cast<std::uint16_t>(20 + expected), 0, 0,
                                          static_cast<std::uint16_t>(30 + expected), 0xffff,
                                          static_cast<std::uint16_t>(40 + expected), 7});
            check(b.pixel_samplers == std::array<std::uint16_t, 12>{5, 5, 5, 5, 2, 2, 0, 0, 5, 5, 5, 0});
            check(b.vertex_texture == 1 && b.vertex_sampler == 1);
        }
    check(input.normal == 0xffff0123u && input.world_height == 0xfffe0001u);
}
} // namespace
int main() {
    try {
        sources();
        bindings();
        std::cout << "PASS " << checks << " C++ lighting source checks\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
