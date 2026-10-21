#pragma once
#include <array>
#include <cstdint>

namespace civ5::materials {
struct WaterCurrentSettings {
    float flow_strength = .1f;
    float noise_strength = .1f;
    float noise_scale = 2.f;
};
using WaterCurrentConstants = std::array<float, 16>;

// Original 0x5fc470 writes five float32 components and retains the others.
// Grid dimensions are terrain worker cells (D810B4/B8), not the 256x256
// render-target extent or the 64-times-larger world height-field dimensions.
WaterCurrentConstants water_current_constants(std::int32_t grid_width, std::int32_t grid_height,
                                              WaterCurrentSettings settings,
                                              WaterCurrentConstants prior = {});

// Original 0x7adc80, following the packet allocator's complete zero fill.
// Resource slot 0 and sampler slot 0 are explicitly zero in this producer.
std::array<std::uint8_t, 100> water_current_packet();
struct WaterCurrentQuad {
    // Original shared layout1: position3, normal3, UV2; normal repeats position.
    std::array<std::array<float, 8>, 4> vertices;
    std::array<std::uint32_t, 6> indices;
};
WaterCurrentQuad water_current_quad();
} // namespace civ5::materials
