#include "civ5/materials/distance_fog.hpp"
#include "civ5/materials/terrain_frame.hpp"
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

using namespace civ5::materials;
using namespace civ5::terrain;
namespace {
unsigned checks = 0;
void check(bool value, const char *why) {
    ++checks;
    if (!value)
        throw std::runtime_error(why);
}
void rejects(auto action) {
    bool failed = false;
    try {
        action();
    } catch (const std::invalid_argument &) {
        failed = true;
    }
    check(failed, "Invalid fog input accepted");
}
DistanceFogState csv(std::string_view text) {
    return distance_fog_startup({reinterpret_cast<const std::uint8_t *>(text.data()), text.size()});
}
void run() {
    for (const auto text : {"", "1,2\n", "1,,3\n", "\"1,2,3", "1,2,3\r4,5,6\r", "1e999,2,3\n", "nan,2,3\n"})
        rejects([&] { csv(text); });
    rejects([&] { csv(std::string_view("1,2,3\0\n", 8)); });
    rejects([&] { csv(std::string(65537, '1')); });
    rejects([&] { DistanceFogAssets::load({}); });
    const auto loaded = csv("0,0,0\r\n");
    check(loaded.loaded && loaded.enabled, "Fog startup did not publish enabled state");
    auto state = loaded;
    const auto bits = std::array<std::uint32_t, 8>{0x7fc00013, 0x80000000, 0x3f800000, 0x3eaaaaab,
                                                   0xffc00001, 0x80000000, 0x00000001, 0x3f000001};
    const auto prior = std::bit_cast<DistanceFogFrame>(bits);
    state.loaded = false;
    state.density = std::numeric_limits<float>::quiet_NaN();
    const auto retained = distance_fog_frame(state, state.density, prior);
    check(std::bit_cast<decltype(bits)>(retained) == bits, "Unloaded fog lost prior bit patterns");
    state = loaded;
    state.enabled = false;
    const auto disabled = distance_fog_frame(state, 1.125f, prior);
    check(disabled.parameters[1] == 0 && !std::signbit(disabled.parameters[1]) &&
              disabled.parameters[3] == 1.125f && disabled.color[3] == 1,
          "Disabled fog changed ramp coordinate or alpha");
    state.enabled = true;
    check(distance_fog_frame(state, -.125f, {}).parameters[3] == -.125f, "Fog clamped camera zoom");
    for (float bad : {0.f, -1.f, std::numeric_limits<float>::infinity()}) {
        state.density = bad;
        rejects([&] { distance_fog_frame(state, 0, {}); });
    }
    state = loaded;
    rejects([&] { distance_fog_frame(state, std::numeric_limits<float>::infinity(), {}); });
    state.color[1] = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { distance_fog_frame(state, 0, {}); });
    const auto frame = distance_fog_frame(loaded, .5f, {});
    rejects([&] { terrain_fog_constants(frame, 0, static_cast<X87Precision>(64)); });
    rejects(
        [&] { terrain_fog_constants(frame, std::numeric_limits<float>::infinity(), X87Precision::single); });
}
} // namespace
int main() {
    try {
        run();
        std::cout << "PASS DistanceFog state and guards: " << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
