#include "civ5/materials/water_sky.hpp"
#include <bit>
#include <charconv>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace civ5::materials;
namespace {
void check(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
std::vector<std::uint8_t> read(const char *name, std::size_t maximum) {
    std::ifstream input(name, std::ios::binary | std::ios::ate);
    check(bool(input), "Open WaterSky input");
    const auto count = input.tellg();
    check(count >= 0 && std::uint64_t(count) <= maximum, "Bounded WaterSky input");
    std::vector<std::uint8_t> result(static_cast<std::size_t>(count));
    input.seekg(0);
    input.read(reinterpret_cast<char *>(result.data()), count);
    check(bool(input), "Read complete WaterSky input");
    return result;
}
std::uint32_t word(std::span<const std::uint8_t> bytes, unsigned offset) {
    return std::uint32_t(bytes[offset]) | (std::uint32_t(bytes[offset + 1]) << 8) |
           (std::uint32_t(bytes[offset + 2]) << 16) | (std::uint32_t(bytes[offset + 3]) << 24);
}
} // namespace
int main(int argc, char **argv) {
    try {
        check(
            argc == 4 || (argc == 6 && std::string_view(argv[4]) == "--constructor"),
            "Usage: water_sky_oracle source-dds explicit-rsqrt-pairs fresh-output [--constructor ocean-u16]");
        unsigned ocean_level = 0;
        if (argc == 6) {
            const std::string_view value(argv[5]);
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(), ocean_level);
            check(parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size() &&
                      ocean_level <= 65535,
                  "Original ocean level must be an unsigned 16-bit integer");
        }
        const auto source = civ5::assets::DdsData::parse(read(argv[1], 65536));
        const auto pairs = read(argv[2], 32);
        check(pairs.size() == 32, "Four squared-value/RSQRT observation pairs required");
        const auto pixels = water_sky_pixels(source);
        unsigned calls = 0;
        const auto light = water_sky_lighting(pixels, [&](float squared) {
            check(calls < 4 && std::bit_cast<std::uint32_t>(squared) == word(pairs, calls * 8),
                  "Original RSQRT input does not match decoded WaterSky direction");
            return std::bit_cast<float>(word(pairs, calls++ * 8 + 4));
        });
        check(calls == 4, "All original normalizations must execute");
        WaterSurfaceSettings prior{};
        prior.specular_sun[3] = std::bit_cast<float>(0x3f001103u);
        prior.specular_sky[3] = std::bit_cast<float>(0x3f001107u);
        const auto result = argc == 6 ? initial_water_surface_settings(std::uint16_t(ocean_level), light)
                                      : light.apply(prior);
        std::ifstream existing(argv[3], std::ios::binary);
        check(!existing, "Fresh WaterSky output required");
        std::ofstream output(argv[3], std::ios::binary);
        check(bool(output), "Create WaterSky output");
        output.write(argc == 6 ? "C5WSKO02" : "C5WSKO01", 8);
        const auto write = [&](std::span<const float> values) {
            output.write(reinterpret_cast<const char *>(values.data()), values.size_bytes());
        };
        for (const auto &pixel : pixels)
            write(pixel);
        if (argc == 6)
            write({&result.reflection_amount, 1});
        write(result.specular_sun);
        write(result.specular_sky);
        write(result.sun_direction);
        write(result.sky_direction);
        write(result.fake_view_sun);
        write(result.fake_view_sky);
        if (argc == 6) {
            const std::array<float, 5> surface{result.ocean_height, result.gradient_scale, result.frequency,
                                               result.river_phase, result.water_phase};
            write(surface);
        }
        output.close();
        check(bool(output), "Complete WaterSky output");
        std::cout << "PASS six decoded source pixels and four independent normalizations\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
