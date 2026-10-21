#include "civ5/materials/material_shadows.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::materials {
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
template <std::size_t N> bool finite(const std::array<float, N> &v) {
    return std::all_of(v.begin(), v.end(), [](float x) { return std::isfinite(x); });
}
} // namespace
float material_shadow_sample_weight(std::uint32_t count) {
    return count == 0 ? std::numeric_limits<float>::infinity()
                      : static_cast<float>(1.0 / static_cast<double>(count));
}
std::array<std::uint8_t, 0x4c0> material_shadow_draw_block(std::uint32_t count,
                                                           const std::array<std::int32_t, 2> &rebase,
                                                           const std::array<float, 2> &jitter,
                                                           std::span<const std::uint8_t, 0x4c0> prior) {
    require(count > 0 && finite(jitter), "Shadow draw requires positive count and finite jitter");
    std::array<std::uint8_t, 0x4c0> result;
    std::copy(prior.begin(), prior.end(), result.begin());
    const auto write = [&](unsigned offset, float value) {
        const auto bits = std::bit_cast<std::uint32_t>(value);
        for (unsigned i = 0; i < 4; ++i)
            result[offset + i] = static_cast<std::uint8_t>(bits >> (8 * i));
    };
    write(0, 0);
    write(4, 0);
    write(0x490, static_cast<float>(rebase[0]));
    write(0x494, static_cast<float>(rebase[1]));
    write(0x4b0, jitter[0]);
    write(0x4b4, jitter[1]);
    write(0x4b8, material_shadow_sample_weight(count));
    return result;
}
void MaterialShadowParameters::validate() const {
    require(finite(world_scales) && finite(sun_direction) && finite(height_texture_scale) &&
                finite(world_rebase) && finite(jitter_add),
            "Nonfinite shadow parameters");
    require(world_scales[0] > 0 && world_scales[1] > 0 &&
                std::max(std::abs(sun_direction[0]), std::abs(sun_direction[1])) > 0,
            "Shadow requires positive XY scales and nonzero XY sun direction");
}
MaterialShadowParameters material_shadow_parameters(const std::array<float, 3> &world,
                                                    const std::array<float, 3> &sun,
                                                    const std::array<float, 2> &height_scale,
                                                    const std::array<std::int32_t, 2> &rebase,
                                                    const std::array<float, 2> &jitter, std::uint32_t count,
                                                    float jitter_w) {
    require(count > 0, "No shadow draw parameters for zero sample count");
    MaterialShadowParameters p{world,
                               sun,
                               height_scale,
                               {static_cast<float>(rebase[0]), static_cast<float>(rebase[1])},
                               {jitter[0], jitter[1], material_shadow_sample_weight(count), jitter_w}};
    p.validate();
    return p;
}
float material_shadow_shade(float x, float y, const MaterialShadowParameters &p,
                            const std::function<float(float, float)> &height_alpha) {
    p.validate();
    require(std::isfinite(x) && std::isfinite(y) && bool(height_alpha), "Invalid shadow sample input");
    const float inverse = 1.f / std::max(std::abs(p.sun_direction[0]), std::abs(p.sun_direction[1]));
    const float dx = inverse * p.sun_direction[0], dy = inverse * p.sun_direction[1];
    const float wx = dx * p.world_scales[0], wy = dy * p.world_scales[1];
    const float inverse_length = 1.f / std::sqrt(wx * wx + wy * wy), length = 1.f / inverse_length;
    float ray = (height_alpha((x + p.world_rebase[0]) * p.height_texture_scale[0],
                              (y + p.world_rebase[1]) * p.height_texture_scale[1]) *
                 p.world_scales[2]) *
                    255.f +
                p.jitter_add[2];
    ray = length * .314485461f + ray;
    float u = ((dx + x) + p.world_rebase[0]) * p.height_texture_scale[0];
    float v = ((dy + y) + p.world_rebase[1]) * p.height_texture_scale[1];
    float visibility = 1.f;
    for (int i = 0; i < 40; ++i) {
        u = dx * p.height_texture_scale[0] + u;
        v = dy * p.height_texture_scale[1] + v;
        ray = length * .314485461f + ray;
        const float alpha = height_alpha(u, v);
        const float water = 1.f - std::clamp((alpha * 255.f - 64.f) * .010416667f, 0.f, 1.f);
        const float difference = (alpha * 255.f) * -p.world_scales[2] + ray;
        if (difference < 0)
            visibility = std::min(water, visibility);
    }
    return (1.f - visibility) * p.jitter_add[2];
}
} // namespace civ5::materials
