#include "civ5/materials/water_animation.hpp"
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace civ5::materials;
namespace {
void check(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
struct Reader {
    std::vector<std::uint8_t> bytes;
    std::size_t at = 0;
    std::uint64_t word(unsigned length) {
        check(at <= bytes.size() && length <= bytes.size() - at, "Truncated animation input");
        std::uint64_t result = 0;
        for (unsigned i = 0; i < length; ++i)
            result |= std::uint64_t(bytes[at++]) << (8 * i);
        return result;
    }
    float f32() { return std::bit_cast<float>(std::uint32_t(word(4))); }
    double f64() { return std::bit_cast<double>(word(8)); }
};
} // namespace
int main(int argc, char **argv) {
    try {
        check(argc == 3, "Usage: water_animation_oracle primitive-input fresh-output");
        std::ifstream input(argv[1], std::ios::binary | std::ios::ate);
        check(bool(input), "Open animation primitive");
        const auto size = input.tellg();
        check(size >= 0 && size <= 16 * 1024 * 1024, "Bounded animation primitive");
        Reader r{std::vector<std::uint8_t>(static_cast<std::size_t>(size))};
        input.seekg(0);
        input.read(reinterpret_cast<char *>(r.bytes.data()), size);
        check(bool(input) && r.word(8) == 0x3130494e41573543ull, "Animation input schema");
        const auto count = r.word(4);
        check(count > 0 && count <= 128, "Animation case count");
        std::vector<std::uint8_t> output;
        const auto write = [&](const auto &value) {
            const auto *p = reinterpret_cast<const std::uint8_t *>(&value);
            output.insert(output.end(), p, p + sizeof(value));
        };
        write(std::uint64_t(0x32304f4e41573543ull));
        write(std::uint32_t(count));
        const WaterAnimationRates defaults;
        write(defaults.river);
        write(defaults.ocean);
        unsigned total = 0;
        for (unsigned index = 0; index < count; ++index) {
            WaterSurfaceSettings settings{};
            settings.river_phase = r.f32();
            settings.water_phase = r.f32();
            WaterAnimationRates rates{r.f32(), r.f32()};
            WaterFrameClock clock;
            clock.elapsed = r.f64();
            clock.delta = r.f64();
            clock.previous = r.f64();
            clock.ticks = std::uint32_t(r.word(4));
            const auto steps = r.word(4);
            check(steps > 0 && steps <= 10000, "Animation step count");
            write(std::uint32_t(steps));
            for (unsigned step = 0; step < steps; ++step) {
                const double sampled = r.f64();
                clock.scale = r.f64();
                clock.fixed_step = r.f64();
                clock.maximum_delta = r.f64();
                const auto active = r.word(4), enabled = r.word(4);
                check(active <= 1 && enabled <= 1, "Animation enable flags");
                clock.tick(sampled);
                settings = advance_water_surface(settings, clock.delta, rates, active, enabled);
                write(clock.elapsed);
                write(clock.delta);
                write(clock.previous);
                write(clock.ticks);
                write(settings.river_phase);
                write(settings.water_phase);
                ++total;
            }
        }
        check(r.at == r.bytes.size(), "Trailing animation primitive bytes");
        check(!std::filesystem::exists(argv[2]), "Fresh animation output required");
        std::ofstream result(argv[2], std::ios::binary);
        result.write(reinterpret_cast<const char *>(output.data()), output.size());
        result.close();
        check(bool(result), "Complete animation output");
        std::cout << "PASS " << count << " animation sequences, " << total << " frames\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
