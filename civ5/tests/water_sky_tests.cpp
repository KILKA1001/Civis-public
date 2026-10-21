#include "civ5/materials/water_sky.hpp"
#include <bit>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace civ5::materials;
using namespace civ5::assets;
namespace {
unsigned checks = 0;
void check(bool ok, const char *message) {
    ++checks;
    if (!ok)
        throw std::runtime_error(message);
}
template <class F> void rejects(F action) {
    bool rejected = false;
    try {
        action();
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    check(rejected, "WaterSky invalid request accepted");
}
void word(Bytes &bytes, unsigned offset, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
        bytes.at(offset + i) = std::uint8_t(value >> (i * 8));
}
Bytes header(unsigned width, unsigned height, unsigned mip_count, bool float_pixels) {
    Bytes b(128);
    word(b, 0, 0x20534444);
    word(b, 4, 124);
    word(b, 8, 0x21007);
    word(b, 12, height);
    word(b, 16, width);
    word(b, 28, mip_count);
    word(b, 76, 32);
    word(b, 80, float_pixels ? 4 : 0x41);
    if (float_pixels)
        word(b, 84, 116);
    else {
        word(b, 88, 32);
        word(b, 92, 0xff0000);
        word(b, 96, 0xff00);
        word(b, 100, 0xff);
        word(b, 104, 0xff000000);
    }
    word(b, 108, 0x401008);
    return b;
}
void run() {
    // Axis directions give exact independent normalization results. The fourth
    // light has a zero direction; D3DX must not request an estimate for it.
    const WaterSkyPixels pixels{{{.75f, .25f, .125f, .25f},
                                 {.5f, .25f, .75f, .5f},
                                 {1, .5f, .5f, 99},
                                 {.5f, 1, .5f, -12},
                                 {.5f, .5f, 0, .75f},
                                 {.5f, .5f, .5f, 0}}};
    unsigned estimates = 0;
    const auto estimate = [&](float squared) {
        check(squared == .25f, "Unexpected WaterSky normalization input");
        ++estimates;
        return 2.f;
    };
    const auto light = water_sky_lighting(pixels, estimate);
    check(estimates == 3, "WaterSky normalized unused alpha or zero vector");
    check(light.sun_color == civ5::terrain::Vector3{3, 1, .5f}, "Sun intensity comes from alpha divisor");
    check(light.sky_color == civ5::terrain::Vector3{1, .5f, 1.5f}, "Sky intensity comes from alpha divisor");
    check(light.sun_direction == civ5::terrain::Vector3{1, 0, 0}, "Sun direction remapping");
    check(light.sky_direction == civ5::terrain::Vector3{0, 1, 0}, "Sky direction remapping");
    check(light.fake_view_sun == civ5::terrain::Vector4{0, 0, -1, .75f},
          "Fake sun alpha must remain independent");
    check(light.fake_view_sky == civ5::terrain::Vector4{0, 0, 0, 0}, "Zero fake sky direction");
    WaterSurfaceSettings prior{};
    prior.reflection_amount = .375f;
    prior.specular_sun[3] = 1024;
    prior.specular_sky[3] = std::bit_cast<float>(0x7fc00123u);
    prior.ocean_height = .2f;
    prior.gradient_scale = 5;
    prior.frequency = .008f;
    prior.river_phase = -.17f;
    prior.water_phase = .09f;
    const auto settings = light.apply(prior);
    check(settings.reflection_amount == prior.reflection_amount &&
              settings.ocean_height == prior.ocean_height &&
              settings.gradient_scale == prior.gradient_scale && settings.frequency == prior.frequency &&
              settings.river_phase == prior.river_phase && settings.water_phase == prior.water_phase &&
              settings.specular_sun[3] == 1024 &&
              std::bit_cast<std::uint32_t>(settings.specular_sky[3]) == 0x7fc00123,
          "Reloading WaterSky changed unrelated water settings or preserved specular bits");
    rejects([&] { water_sky_lighting(pixels, {}); });
    for (unsigned index : {0u, 1u}) {
        auto invalid = pixels;
        invalid[index][3] = 0;
        rejects([&] { water_sky_lighting(invalid, estimate); });
    }
    for (unsigned index = 0; index < 6; ++index) {
        auto invalid = pixels;
        invalid[index][0] = std::numeric_limits<float>::infinity();
        rejects([&] { water_sky_lighting(invalid, estimate); });
    }
    auto raw = header(2, 4, 2, true);
    raw.resize(128 + (8 + 2) * 16, 0xa5);
    for (unsigned i = 0; i < 6; ++i)
        for (unsigned lane = 0; lane < 4; ++lane)
            word(raw, 128 + i * 16 + lane * 4, std::bit_cast<std::uint32_t>(pixels[i][lane]));
    check(water_sky_pixels(DdsData::parse(raw, NumericDdsProfile::legacy_d3d9)) == pixels,
          "WaterSky must retain six consecutive base pixels and ignore remaining pixels/mips");
    word(raw, 128, 0x7f800000);
    rejects([&] { water_sky_pixels(DdsData::parse(raw, NumericDdsProfile::legacy_d3d9)); });
    auto bgra = header(2, 4, 2, false);
    bgra.resize(128 + (8 + 2) * 4, 127);
    bgra[128] = 0;
    bgra[129] = 255;
    bgra[130] = 102;
    bgra[131] = 51;
    const auto decoded = water_sky_pixels(DdsData::parse(bgra));
    check(std::bit_cast<std::uint32_t>(decoded[0][0]) == 0x3eccccce && decoded[0][1] == 1 &&
              decoded[0][2] == 0 && std::bit_cast<std::uint32_t>(decoded[0][3]) == 0x3e4cccce,
          "WaterSky uses BGRA bytes multiplied by the original float32 reciprocal");
    auto small = header(1, 1, 1, false);
    small.resize(132);
    rejects([&] { water_sky_pixels(DdsData::parse(small)); });
    word(bgra, 92, 0xff);
    word(bgra, 100, 0xff0000);
    rejects([&] { water_sky_pixels(DdsData::parse(bgra)); });
}
} // namespace
int main() {
    try {
        run();
        std::cout << "PASS " << checks << " WaterSky checks\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
