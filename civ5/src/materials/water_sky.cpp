#include "civ5/materials/water_sky.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace civ5::materials {
namespace {
void require(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
std::uint32_t word(std::span<const std::uint8_t> bytes, unsigned offset) {
    return std::uint32_t(bytes[offset]) | (std::uint32_t(bytes[offset + 1]) << 8) |
           (std::uint32_t(bytes[offset + 2]) << 16) | (std::uint32_t(bytes[offset + 3]) << 24);
}
} // namespace

WaterSkyPixels water_sky_pixels(const assets::DdsData &source) {
    require(source.face_count() == 1 && std::uint64_t(source.width()) * source.height() >= 6,
            "WaterSky requires six base-level pixels in a two-dimensional source");
    const auto bytes = source.payload(0, 0);
    WaterSkyPixels result{};
    if (source.format() == assets::DdsFormat::rgba32_float) {
        for (unsigned i = 0; i < result.size(); ++i)
            for (unsigned lane = 0; lane < 4; ++lane)
                result[i][lane] = std::bit_cast<float>(word(bytes, i * 16 + lane * 4));
    } else {
        require(source.format() == assets::DdsFormat::masked_rgb && source.bits_per_pixel() == 32 &&
                    source.channel_masks() ==
                        std::array<std::uint32_t, 4>{0xff0000, 0xff00, 0xff, 0xff000000},
                "Unsupported WaterSky CPU pixel conversion");
        constexpr std::array<unsigned, 4> lanes{2, 1, 0, 3};
        // Original FTextureData 0x8e97b0 uses MULSS by the stored float32
        // reciprocal at 0x9f5f5c; direct division changes some results by 1 ULP.
        constexpr float unorm = std::bit_cast<float>(0x3b808081u);
        for (unsigned i = 0; i < result.size(); ++i)
            for (unsigned lane = 0; lane < 4; ++lane)
                result[i][lane] = float(bytes[i * 4 + lanes[lane]]) * unorm;
    }
    for (const auto &pixel : result)
        for (float value : pixel)
            require(std::isfinite(value), "Nonfinite WaterSky source parameter");
    return result;
}

WaterSkyLighting water_sky_lighting(const WaterSkyPixels &pixels,
                                    const terrain::ReciprocalSqrtEstimate &estimate) {
    require(bool(estimate), "WaterSky requires the explicit D3DX RSQRT estimate provider");
    for (const auto &pixel : pixels)
        for (float value : pixel)
            require(std::isfinite(value), "Nonfinite WaterSky lighting input");
    require(pixels[0][3] != 0 && pixels[1][3] != 0, "WaterSky light intensity denominator is zero");
    const auto color = [&](unsigned index) {
        const float reciprocal = 1.f / pixels[index][3];
        const terrain::Vector3 result{pixels[index][0] * reciprocal, pixels[index][1] * reciprocal,
                                      pixels[index][2] * reciprocal};
        require(std::all_of(result.begin(), result.end(), [](float v) { return std::isfinite(v); }),
                "Unrepresentable WaterSky light intensity");
        return result;
    };
    const auto direction = [&](unsigned index) {
        return terrain::normalize3_sse(
            {pixels[index][0] - .5f, pixels[index][1] - .5f, pixels[index][2] - .5f}, estimate);
    };
    // Keep the original order of all four independent D3DX normalizations.
    WaterSkyLighting result{};
    result.sun_color = color(0);
    result.sun_direction = direction(2);
    result.sky_color = color(1);
    result.sky_direction = direction(3);
    const auto fake_sun = direction(4), fake_sky = direction(5);
    std::copy(fake_sun.begin(), fake_sun.end(), result.fake_view_sun.begin());
    std::copy(fake_sky.begin(), fake_sky.end(), result.fake_view_sky.begin());
    result.fake_view_sun[3] = pixels[4][3];
    result.fake_view_sky[3] = pixels[5][3];
    return result;
}

WaterSurfaceSettings WaterSkyLighting::apply(WaterSurfaceSettings prior) const {
    std::copy(sun_color.begin(), sun_color.end(), prior.specular_sun.begin());
    std::copy(sky_color.begin(), sky_color.end(), prior.specular_sky.begin());
    prior.sun_direction = sun_direction;
    prior.sky_direction = sky_direction;
    prior.fake_view_sun = fake_view_sun;
    prior.fake_view_sky = fake_view_sky;
    return prior;
}

WaterSurfaceSettings initial_water_surface_settings(std::uint16_t ocean_level,
                                                    const WaterSkyLighting &lighting) {
    WaterSurfaceSettings result{};
    result.reflection_amount = .5f;
    // Original _CIpow(2, 11.75) and _CIpow(2, 5.75), stored as float32.
    // Both CRT dispatch paths and all three x87 precisions round identically.
    result.specular_sun[3] = static_cast<float>(std::pow(2., 11.75));
    result.specular_sky[3] = static_cast<float>(std::pow(2., 5.75));
    result.ocean_height = float(ocean_level) * std::bit_cast<float>(0x37800080u);
    result.gradient_scale = 5.f;
    result.frequency = .008f;
    return lighting.apply(result);
}
} // namespace civ5::materials
