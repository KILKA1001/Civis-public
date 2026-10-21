#pragma once
#include "civ5/assets/dds.hpp"
#include "water_surface.hpp"

namespace civ5::materials {
// Six consecutive base-level RGBA pixels carry lighting data. They are CPU
// parameters, not an sRGB color image. Lower mip levels do not supply lights.
using WaterSkyPixels = std::array<terrain::Vector4, 6>;

// The authored BGRA8 profile and already converted RGBA32F are supported.
// BGRA8 uses the original FTextureData float32 reciprocal multiplication.
WaterSkyPixels water_sky_pixels(const assets::DdsData &source);

struct WaterSkyLighting {
    terrain::Vector3 sun_color, sky_color, sun_direction, sky_direction;
    terrain::Vector4 fake_view_sun, fake_view_sky;

    // Original 0x6b5db0 updates these RGB/direction values while preserving
    // SpecSun.w and SpecSky.w, which belong to the constructor/live settings.
    WaterSurfaceSettings apply(WaterSurfaceSettings prior) const;
};

WaterSkyLighting water_sky_lighting(const WaterSkyPixels &pixels,
                                    const terrain::ReciprocalSqrtEstimate &estimate);

// Surface fields established by original WaterSystem construction. LEAN's
// gradient metadata is not the shader's independent gradient scale (5.0).
// The original constructor takes the ocean level as an unsigned 16-bit value.
WaterSurfaceSettings initial_water_surface_settings(std::uint16_t ocean_level,
                                                    const WaterSkyLighting &lighting);
} // namespace civ5::materials
