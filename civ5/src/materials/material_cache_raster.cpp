#include "civ5/materials/material_cache_raster.hpp"
#include <cmath>

namespace civ5::materials {
std::array<float, 32> material_cache_normal_vertices() {
    return {1, 1,  0, 1, 1,  0, 1, 0, -1, 1,  0, -1, 1,  0, 0, 0,
            1, -1, 0, 1, -1, 0, 1, 1, -1, -1, 0, -1, -1, 0, 0, 1};
}
std::array<std::uint32_t, 6> material_cache_normal_indices() { return {2, 1, 0, 3, 1, 2}; }
std::uint32_t material_cache_clear_argb(const std::array<float, 4> &rgba) {
    std::array<std::uint32_t, 4> lanes{};
    for (unsigned i = 0; i < 4; ++i) {
        const float scaled = rgba[i] * 255.f;
        if (std::isfinite(scaled) && scaled >= -2147483648.f && scaled < 2147483648.f)
            lanes[i] = static_cast<std::uint32_t>(static_cast<std::int32_t>(scaled)) & 255;
    }
    return (lanes[3] << 24) | (lanes[0] << 16) | (lanes[1] << 8) | lanes[2];
}
std::array<float, 4> material_cache_clear_rgba(std::uint32_t argb) {
    return {float((argb >> 16) & 255) / 255.f, float((argb >> 8) & 255) / 255.f, float(argb & 255) / 255.f,
            float(argb >> 24) / 255.f};
}
bool material_cache_clear_target(std::uint32_t flags, bool conditional_clear) {
    return (flags & 4) != 0 || ((flags & 128) != 0 && conditional_clear);
}
} // namespace civ5::materials
