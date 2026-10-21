#include "civ5/materials/material_normals.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace civ5::materials {
std::array<float, 8> material_normal_parameters(std::int32_t resolution, std::array<float, 8> prior) {
    if (resolution <= 0)
        throw std::invalid_argument("Positive material-normal cache resolution required");
    prior[0] = 1.f / static_cast<float>(resolution);
    prior[1] = prior[0];
    prior[2] = 0.f;
    prior[4] = 16.f;
    return prior;
}
std::array<float, 4> encode_material_normal(std::span<const float, 8> taps, float height_scale) {
    if (!std::isfinite(height_scale) ||
        !std::all_of(taps.begin(), taps.end(), [](float v) { return std::isfinite(v); }))
        throw std::invalid_argument("Nonfinite material-normal taps or scale");
    const auto tl = taps[0], t = taps[1], tr = taps[2], l = taps[3];
    const auto r = taps[4], bl = taps[5], b = taps[6], br = taps[7];
    const float gx = ((bl - br) - 2.f * r) + ((tl - tr) + 2.f * l);
    const float gy = ((-bl - br) - 2.f * b) + ((tl + tr) + 2.f * t);
    const float half = .5f * height_scale;
    const float x = -((half * gx) * 2.f), y = -((half * gy) * -2.f);
    const float inverse = 1.f / std::sqrt((x * x + y * y) + 16.f);
    return {(x * inverse) * .5f + .5f, (y * inverse) * .5f + .5f, inverse * 2.f + .5f, 0.f};
}
} // namespace civ5::materials
