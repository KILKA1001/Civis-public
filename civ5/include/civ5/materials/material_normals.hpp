#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace civ5::materials {
// Original64B64E: material MRT1 RED -> cache normals, not world A8 displacement.
// Unused components3/5/6/7 retain their previous bits (including nonfinite bits).
std::array<float, 8> material_normal_parameters(std::int32_t resolution, std::array<float, 8> prior = {});
// Independent scalar PS reference, taps TL,T,TR,L,R,BL,B,BR. Float32 sqrt is
// not a promise of bit identity to every device's reciprocal-square-root unit.
std::array<float, 4> encode_material_normal(std::span<const float, 8> taps, float height_scale = 16.f);
} // namespace civ5::materials
