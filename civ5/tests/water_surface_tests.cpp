#include "civ5/materials/water_surface.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <xmmintrin.h>
using namespace civ5::materials;
using namespace civ5::terrain;
namespace {
unsigned checks = 0;
void check(bool ok) {
    ++checks;
    if (!ok)
        throw std::runtime_error("Water surface contract");
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
WaterSurfaceDomain domain(int width = 12, int height = 10, int mode = 0) {
    return WaterSurfaceDomain::from_world(build_world_layout(
        width, height, mode, {}, [](float v) { return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(v))); }));
}
Vector4 full(const WaterSurfaceDomain &d) {
    return {d.origin_x, d.origin_y, d.origin_x + d.span_x, d.origin_y + d.span_y};
}
void run() {
    constexpr auto pc = X87Precision::double_precision;
    const auto d = domain();
    const std::array<Vector4, 1> bounds{full(d)};
    auto mesh = build_water_surface_mesh(d, bounds, 1, pc);
    check(mesh.vertices.size() == 896 && mesh.indices.size() == 1344 && mesh.capacity == 268 &&
          mesh.reallocate);
    check(mesh.counts[0] == 196 && mesh.counts[1] == 28 && mesh.prefixes[1] == 196 &&
          mesh.prefixes[2] == 224);
    check(mesh.vertices[0][0] == -511.5f && mesh.vertices[1][0] == -447.5f);
    check(mesh.vertices[0][2] == .0625f && mesh.vertices[3][3] == -.0625f);
    auto retained = build_water_surface_mesh(d, bounds, 4096, pc);
    check(!retained.reallocate && retained.capacity == 4096 && retained.vertices == mesh.vertices);
    auto empty = build_water_surface_mesh(d, {}, 37, pc);
    check(empty.vertices.empty() && empty.counts.empty() && empty.capacity == 37 && !empty.reallocate);
    const std::array<Vector4, 1> empty_bounds{Vector4{128, 128, -256, -256}};
    check(build_water_surface_mesh(d, empty_bounds, 0, pc).vertices.empty());
    std::array<Vector4, 5> too_many{};
    rejects([&] { build_water_surface_mesh(d, too_many, 1, pc); });
    rejects([&] { build_water_surface_mesh(d, bounds, 1, static_cast<X87Precision>(7)); });
    for (auto value : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(),
                       -std::numeric_limits<float>::infinity()})
        for (unsigned lane = 0; lane < 4; ++lane) {
            auto invalid = bounds;
            invalid[0][lane] = value;
            rejects([&] { build_water_surface_mesh(d, invalid, 1, pc); });
        }
    auto invalid_domain = d;
    invalid_domain.scales[0] = 0;
    rejects([&] { build_water_surface_mesh(invalid_domain, bounds, 1, pc); });
    invalid_domain = d;
    invalid_domain.field_width = 14337;
    rejects([&] { build_water_surface_mesh(invalid_domain, bounds, 1, pc); });
    std::array<Vector4, 1> outside{Vector4{d.origin_x, d.origin_y, d.origin_x + 14336, d.origin_y + 64}};
    rejects([&] { build_water_surface_mesh(d, outside, 1, pc); });
    const auto large = domain(128, 128, 1);
    const std::array<Vector4, 1> large_view{full(large)};
    const auto wrapped = build_water_surface_mesh(large, large_view, 1, pc);
    check(wrapped.vertices.size() == 67584 && wrapped.indices[16384 * 6] == 0 &&
          wrapped.indices[16384 * 6 + 5] == 3);
    const std::array four_large{full(large), full(large), full(large), full(large)};
    rejects([&] { build_water_surface_mesh(large, four_large, 1, pc); });
    WaterSurfaceResources resources{0x12345678, 0x87654321, 0xaabb0102, 0xccdd0304, 0x11220506, {}, {}};
    for (unsigned i = 0; i < resources.shared.size(); ++i)
        resources.shared[i] = 0xabba0000 + 0x1100 + i;
    for (unsigned i = 0; i < resources.height_windows.size(); ++i)
        resources.height_windows[i] = 0xdead0000 + 0x2200 + i;
    for (auto selector : {0u, 2u, 3u, 5u}) {
        const auto draws = water_surface_draws(mesh, resources, selector);
        check(draws.size() == 2 && draws[0].triangle_count == 392 && draws[1].first_index == 1176);
        check(draws[1].selector == selector && draws[1].window == 1 && draws[1].view == 0);
        const auto &packet = draws[1].packet;
        check(packet[0x18] == 2 && packet[0x19] == 1 && packet[0x1a] == 4 && packet[0x1b] == 3);
        check(packet[0x26] == 1 && packet[0x27] == 0x22 && packet[0x60] == 14);
        check(packet[0x44] == 3 && packet[0x48] == 5 && packet[0x4e] == 5 && packet[0x52] == 0);
        check(packet[0x32] == 6 && packet[0x33] == 0x11 && packet[0x30] == 0);
    }
    for (auto bad : {1u, 4u, 6u, 0xffffffffu})
        rejects([&] { water_surface_draws(mesh, resources, bad); });
    auto broken = mesh;
    broken.prefixes[1]++;
    rejects([&] { water_surface_draws(broken, resources, 3); });
    broken = mesh;
    broken.counts[255]++;
    rejects([&] { water_surface_draws(broken, resources, 3); });
    broken = mesh;
    broken.indices.pop_back();
    rejects([&] { water_surface_draws(broken, resources, 3); });
    WaterSurfaceSettings settings{
        .5f, {1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11}, {12, 13, 14}, {15, 16, 17, 18}, {19, 20, 21, 22}, .2f,
        5,   .008f,        .17f,         .09f};
    WaterSurfaceFrame frame{identity_matrix(), {.1f, .2f, .3f, .4f}, {.02f, .6f, .7f, .8f}, .71f};
    WaterSurfaceView view{identity_matrix(), identity_matrix(), identity_matrix(),
                          identity_matrix(), {-7, 19, 3.25f},   {}};
    for (unsigned i = 0; i < 145; ++i)
        view.prior_constants[i] = std::bit_cast<float>(0x80000001u + i);
    const auto constants = water_surface_constants(d, settings, frame, view, pc);
    for (unsigned offset : {0xb4u, 0xb8u, 0xbcu, 0xc4u, 0xc8u, 0xccu, 0xd4u, 0xd8u, 0xdcu, 0xecu, 0xfcu,
                            0x15cu, 0x224u, 0x228u, 0x22cu})
        check(std::bit_cast<std::uint32_t>(constants[offset / 4]) ==
              std::bit_cast<std::uint32_t>(view.prior_constants[offset / 4]));
    check(constants[0x1f4 / 4] < 0 && constants[0x144 / 4] < 0 && constants[0x148 / 4] == 0);
    check(constants[0xe0 / 4] == 9 && constants[0x130 / 4] == 19 && constants[0x240 / 4] == .5f);
    // WaterOffset c15.x and RiverOffset c12.x are distinct original fields.
    // Previously both GPU adapters incorrectly sourced WaterOffset from 0x80.
    const auto registers = water_shader_registers(constants);
    check(registers.vertex[60] == constants[44] && registers.vertex[60] != constants[32]);
    check(registers.vertex[48] == constants[32] && registers.vertex[64] == constants[48]);
    check(registers.vertex[0] == 1 && registers.vertex[15] == 1 && registers.pixel[0] == 0);
    check(registers.pixel[108] == constants[144] && registers.pixel[109] == 0);
    frame.fog_parameters[0] = 0;
    rejects([&] { water_surface_constants(d, settings, frame, view, pc); });
    frame.fog_parameters[0] = .02f;
    settings.frequency = std::numeric_limits<float>::infinity();
    rejects([&] { water_surface_constants(d, settings, frame, view, pc); });
}
} // namespace
int main() {
    try {
        run();
        std::cout << "PASS " << checks << " assertions\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << " after " << checks << " assertions\n";
        return 1;
    }
}
