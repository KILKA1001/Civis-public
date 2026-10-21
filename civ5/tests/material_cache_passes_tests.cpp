#include "civ5/materials/material_cache_passes.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace civ5::materials;
namespace {
unsigned checks = 0;
void check(bool value) {
    ++checks;
    if (!value)
        throw std::runtime_error("Material cache pass guard failed");
}
template <class F> void rejects(F function) {
    bool rejected = false;
    try {
        function();
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    check(rejected);
}
struct Fixture {
    MaterialCachePassSettings settings{256, 64, 1, 5, false, false, false, false,
                                        civ5::terrain::X87Precision::double_precision};
    MaterialCachePassPatch patch{67, 67, {-16777217, 16777219}, 16, 0xabcdffff, 0,
                                 {0, 0}, {1, 2}, 0, 0, 0};
    MaterialCachePassSource source{};
    std::array<std::int32_t, 1> selected{0}, rows{0}, modes{0};
    std::array<float, 1> scales{.01f};
    std::array<std::uint32_t, 64> flags{};
    std::array<std::uint32_t, 1> diffuse{0xabcdffff}, specular{0}, height{0x12340000};
    std::vector<std::uint8_t> entering = std::vector<std::uint8_t>(0x4c0 * 2, 0xa5);
    std::array<float, 8> normal{};
    MaterialCachePassPlan build() const {
        return material_cache_pass_plan(settings, patch, source,
                                        {selected, rows, modes, scales, flags, diffuse, specular, height,
                                         -1280.f, 1280.f},
                                        entering, normal);
    }
};
} // namespace
int main() {
    try {
        Fixture f;
        const auto untouched = f.entering;
        auto plan = f.build();
        check(f.entering == untouched);
        // Input lifetimes do not govern the queued constants/packets/draws.
        f.entering.clear();
        check(plan.constants.size() == 0x4c0 * 2 && plan.constants[8] == 0xa5);
        check(plan.packets[2].pixel_textures[0] == 0xffff && plan.packets[2].pixel_textures[8] == 0);
        check(plan.packets[2].vertex_count == 0 && plan.packets[2].geometry[3] == 0);
        const auto normal = plan.passes[1].draws[0], shadow = plan.passes[2].draws[0];
        check(normal.constant_offset == plan.passes[0].draws[0].constant_offset);
        check(shadow.constant_offset != normal.constant_offset && shadow.constant_bytes == 0x4c0);
        check(plan.passes.back().draws[0].constant_offset == shadow.constant_offset);
        f = {};
        for (auto size : {0u, 32u, 63u, 65u, 1024u, 0xffffffffu}) {
            auto s = f.settings;
            s.patch_size = size;
            rejects([&] { material_cache_constant_bytes(s); });
        }
        for (auto size : {0u, 16385u, 0xffffffffu}) {
            auto s = f.settings;
            s.temporary_size = size;
            rejects([&] { material_cache_constant_bytes(s); });
        }
        f.settings.shadow_samples = 33;
        rejects([&] { f.build(); });
        f = {};
        f.settings.view = 1;
        rejects([&] { f.build(); });
        f = {};
        f.settings.precision = static_cast<civ5::terrain::X87Precision>(77);
        rejects([&] { f.build(); });
        for (auto dimension : {0u, 2u, 68u, 0xffffffffu}) {
            f = {};
            f.patch.columns = dimension;
            rejects([&] { f.build(); });
            f = {};
            f.patch.rows = dimension;
            rejects([&] { f.build(); });
        }
        for (auto value : {-1, 1, 64, std::numeric_limits<int>::max()}) {
            f = {};
            f.selected[0] = value;
            rejects([&] { f.build(); });
            f = {};
            f.rows[0] = value;
            rejects([&] { f.build(); });
        }
        for (auto mode : {-1, 3, std::numeric_limits<int>::max()}) {
            f = {};
            f.modes[0] = mode;
            rejects([&] { f.build(); });
        }
        for (auto scale : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(),
                           std::numeric_limits<float>::max()}) {
            f = {};
            f.scales[0] = scale;
            rejects([&] { f.build(); });
            check(f.entering == untouched);
        }
        f = {};
        f.entering.pop_back();
        rejects([&] { f.build(); });
        f.entering.push_back(0);
        f.entering.push_back(0);
        rejects([&] { f.build(); });
        // A failed new plan does not damage a previously queued plan.
        check(plan.constants[8] == 0xa5 && plan.passes.back().draws[0].constant_bytes == 0x4c0);
        f={};
        MaterialCacheDecalLayers layers{{},143};
        check(material_cache_constant_bytes(f.settings,&layers)==material_cache_constant_bytes(f.settings));
        layers.layers[0].selected=1;
        rejects([&]{material_cache_constant_bytes(f.settings,&layers);});
        layers.layers[0].capacity_exhausted=true;
        check(material_cache_constant_bytes(f.settings,&layers)==material_cache_constant_bytes(f.settings));
        layers.layers[0].draws.push_back({});
        rejects([&]{material_cache_constant_bytes(f.settings,&layers);});
        layers={};layers.layers[0].selected=1;layers.layers[0].draws.push_back({});
        const auto with_layers=[&] {
            const std::vector<std::uint8_t> bytes(material_cache_constant_bytes(f.settings,&layers),0xa5);
            return material_cache_pass_plan(f.settings,f.patch,f.source,
                {f.selected,f.rows,f.modes,f.scales,f.flags,f.diffuse,f.specular,f.height,-1280,1280},bytes,f.normal,&layers);
        };
        rejects(with_layers); // Zero primitives.
        layers.layers[0].draws[0].triangles=21846;
        rejects(with_layers); // Outside original U16 capacity.
        layers.layers[0].draws[0].triangles=1;
        layers.layers[0].draws[0].constants[0]=std::numeric_limits<float>::infinity();
        rejects(with_layers);
        TerrainDecalStore store({-1024,-1024,1024,1024},8,8,37);
        auto malformed=f.patch;malformed.columns=0;
        rejects([&]{prepare_material_cache_decals(f.settings,malformed,{0,0},store,77,143,{131072,131072});});
        rejects([&]{prepare_material_cache_decals(f.settings,f.patch,{std::numeric_limits<int>::max(),0},store,77,143,{131072,131072});});
        std::cout << "PASS material cache pass ownership/input guards: " << checks << " checks\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
