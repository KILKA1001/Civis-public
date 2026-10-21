#include "civ5/materials/terrain_frame.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace civ5::materials;
using namespace civ5::terrain;
namespace {
unsigned checks = 0;
void check(bool ok, const char *why) {
    ++checks;
    if (!ok)
        throw std::runtime_error(why);
}
void rejects(auto action) {
    bool rejected = false;
    try {
        action();
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    check(rejected, "Invalid terrain frame accepted");
}
void run() {
    const auto pc = X87Precision::double_precision;
    TerrainLighting source{{90, 25, 80, 135}, .5f, {.25f, -.5f, 1}, {0, 0, -1}, {1, 0, 0}};
    const auto no_estimate = [](float) -> float { throw std::runtime_error("Zero vector requested RSQRT"); };
    const auto zero = terrain_cache_lighting(source, no_estimate);
    check(zero.shadow == source.shadow && zero.half_vector == Vector3{}, "Antiparallel cache lighting");
    rejects([&] { terrain_cache_lighting(source, {}); });
    auto updated = terrain_lighting_update(source, pc);
    check(updated.shadow == source.shadow && updated.scalar == source.scalar,
          "Updating sun angles changed independent shadow/scalar");
    auto altered = updated;
    altered.angles.light_polar = 55;
    altered = terrain_lighting_update(altered, pc);
    check(altered.sun != updated.sun && altered.reflection == updated.reflection &&
              altered.shadow == updated.shadow,
          "Sun and reflection controls are coupled");
    TerrainFrameInput in{};
    in.view = in.inverse_view = in.projection = in.shadow_projection = in.overlay_projection =
        identity_matrix();
    in.sun_direction = {0, 0, -1};
    in.shadow_direction = source.shadow;
    in.world_scales = {1, 1, 1};
    in.fog_density = .125f;
    in.fog_strength = .25f;
    in.fog_color = {.25f, .5f, .75f, 1};
    in.fog_distance = 4096;
    in.options = {2, 0, 1, 0};
    in.level = 17;
    TerrainFrameState prior;
    for (auto &v : prior.constants)
        for (auto &f : v)
            f = std::bit_cast<float>(0x7fc0a537u);
    const auto result = terrain_frame(in, prior, pc, no_estimate);
    check(std::bit_cast<std::uint32_t>(result.constants[8][0]) == 0x7fc0a537u &&
              std::bit_cast<std::uint32_t>(result.constants[17][3]) == 0x7fc0a537u,
          "Frame changed untouched NaN payload slots");
    check(result.minimum_level == 17 && result.maximum_level == 17, "First level extrema");
    check(result.fog == std::array<Vector4, 2>{in.fog_color, Vector4{-.125f, -2, 0, 4096}},
          "Original fog sign and exact zero-height exponential");
    in.level = 5;
    const auto next = terrain_frame(in, result, pc, no_estimate);
    check(next.minimum_level == 5 && next.maximum_level == 17, "Retained level extrema");
    auto invalid = in;
    invalid.view[0] = std::numeric_limits<float>::infinity();
    rejects([&] { terrain_frame(invalid, prior, pc, no_estimate); });
    invalid = in;
    invalid.fog_density = 0;
    rejects([&] { terrain_frame(invalid, prior, pc, no_estimate); });
    invalid = in;
    invalid.fog_strength = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { terrain_frame(invalid, prior, pc, no_estimate); });
    rejects([&] { terrain_frame(in, prior, static_cast<X87Precision>(99), no_estimate); });
    rejects([&] { terrain_lighting_defaults(static_cast<X87Precision>(99)); });
    source.sun = {0, 0, 1};
    rejects([&] { terrain_cache_lighting(source, [](float) { return 0.f; }); });
    source.angles.light_polar = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { terrain_lighting_update(source, pc); });
}
} // namespace
int main() {
    try {
        run();
        std::cout << "PASS " << checks << " terrain frame state/guard checks\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
