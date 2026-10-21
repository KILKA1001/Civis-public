#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <span>

namespace civ5::materials {
// Original unsigned count -> PC53 reciprocal -> Float32; zero means +Inf and no draws.
float material_shadow_sample_weight(std::uint32_t count);
// Native local constant-block writes, not a pointer-bearing draw-packet ABI.
// Every byte outside offsets0/4/490/494/4B0/4B4/4B8 is retained.
std::array<std::uint8_t, 0x4c0> material_shadow_draw_block(std::uint32_t count,
                                                           const std::array<std::int32_t, 2> &rebase,
                                                           const std::array<float, 2> &jitter,
                                                           std::span<const std::uint8_t, 0x4c0> prior);

struct MaterialShadowParameters {
    std::array<float, 3> world_scales, sun_direction;
    std::array<float, 2> height_texture_scale, world_rebase;
    std::array<float, 4> jitter_add;
    void validate() const;
};
// Convenient typed form of those writes. Count must be positive; unused jitter
// components remain explicit even though original selector3 does not read them.
MaterialShadowParameters material_shadow_parameters(const std::array<float, 3> &world_scales,
                                                    const std::array<float, 3> &sun,
                                                    const std::array<float, 2> &height_scale,
                                                    const std::array<std::int32_t, 2> &rebase,
                                                    const std::array<float, 2> &jitter, std::uint32_t count,
                                                    float jitter_w = 0.f);
// Independent Float32 reference. GPU RCP/RSQ are verified separately.
float material_shadow_shade(float x, float y, const MaterialShadowParameters &parameters,
                            const std::function<float(float, float)> &height_alpha);
} // namespace civ5::materials
