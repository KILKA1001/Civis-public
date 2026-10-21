#include "civ5/materials/water_animation.hpp"
#include <bit>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace civ5::materials;
namespace {
unsigned checks = 0;
void check(bool ok) {
    ++checks;
    if (!ok)
        throw std::runtime_error("Water animation input contract");
}
template <class F> void rejects(F action) {
    bool failed = false;
    try {
        action();
    } catch (const std::invalid_argument &) {
        failed = true;
    }
    check(failed);
}
auto clock_bits(const WaterFrameClock &c) {
    return std::array<std::uint64_t, 7>{std::bit_cast<std::uint64_t>(c.elapsed),
                                        std::bit_cast<std::uint64_t>(c.delta),
                                        std::bit_cast<std::uint64_t>(c.previous),
                                        std::bit_cast<std::uint64_t>(c.scale),
                                        std::bit_cast<std::uint64_t>(c.fixed_step),
                                        std::bit_cast<std::uint64_t>(c.maximum_delta),
                                        c.ticks};
}
void run() {
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    constexpr double inf = std::numeric_limits<double>::infinity();
    for (double value : {nan, inf, -inf}) {
        for (auto member :
             {&WaterFrameClock::elapsed, &WaterFrameClock::delta, &WaterFrameClock::previous,
              &WaterFrameClock::scale, &WaterFrameClock::fixed_step, &WaterFrameClock::maximum_delta}) {
            WaterFrameClock clock;
            clock.*member = value;
            const auto before = clock_bits(clock);
            rejects([&] { clock.tick(1); });
            check(clock_bits(clock) == before);
        }
        WaterFrameClock clock;
        const auto before = clock_bits(clock);
        rejects([&] { clock.tick(value); });
        check(clock_bits(clock) == before);
    }
    // Application-level input guards are additional to native finite-input
    // fidelity. Invalid updates must leave the publicly owned clock intact.
    WaterFrameClock clock;
    clock.previous = std::numeric_limits<double>::max();
    clock.fixed_step = clock.previous;
    clock.ticks = 1;
    auto before = clock_bits(clock);
    rejects([&] { clock.tick(0); });
    check(clock_bits(clock) == before);
    clock = {};
    clock.elapsed = std::numeric_limits<double>::max();
    clock.maximum_delta = -1;
    clock.ticks = 1;
    before = clock_bits(clock);
    rejects([&] { clock.tick(std::numeric_limits<double>::max()); });
    check(clock_bits(clock) == before);

    WaterSurfaceSettings settings{};
    settings.river_phase = std::bit_cast<float>(0x7fc12345u);
    settings.water_phase = -0.f;
    const auto settings_bits = [](const WaterSurfaceSettings &s) {
        return std::bit_cast<std::array<std::uint32_t, 28>>(s);
    };
    for (auto gates : {std::array{false, true}, std::array{true, false}, std::array{false, false}})
        check(settings_bits(advance_water_surface(settings, nan, {}, gates[0], gates[1])) ==
              settings_bits(settings));
    rejects([&] { advance_water_surface(settings, 1); });
    settings = {};
    for (double value : {nan, inf, -inf, std::numeric_limits<double>::max()})
        rejects([&] { advance_water_surface(settings, value); });
    for (WaterAnimationRates rates :
         {WaterAnimationRates{float(inf), .03f}, WaterAnimationRates{.18f, float(nan)}})
        rejects([&] { advance_water_surface(settings, 1, rates); });
    settings.river_phase = std::numeric_limits<float>::max();
    rejects([&] { advance_water_surface(settings, 1, {settings.river_phase, .03f}); });

    std::array<float, 28> fields;
    for (unsigned i = 0; i < fields.size(); ++i)
        fields[i] = float(i + 1);
    settings = std::bit_cast<WaterSurfaceSettings>(fields);
    auto updated = std::bit_cast<std::array<float, 28>>(advance_water_surface(settings, .125));
    for (unsigned i = 0; i < 26; ++i)
        check(updated[i] == fields[i]);
    check(updated[26] != fields[26] && updated[27] != fields[27]);
}
} // namespace
int main() {
    try {
        run();
        std::cout << "PASS " << checks << " water animation input checks\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
