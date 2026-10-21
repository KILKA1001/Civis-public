#pragma once
#include <array>
#include <cstdint>

namespace civ5::materials {
// Original 49B3E0 vertex32 and U32 index writes, before API resource upload.
std::array<float, 32> material_cache_normal_vertices();
std::array<std::uint32_t, 6> material_cache_normal_indices();
// The original SSE conversion truncates and retains the low byte, without
// clamping. Nonfinite / out-of-I32-range lanes become zero after masking.
std::uint32_t material_cache_clear_argb(const std::array<float, 4> &rgba);
std::array<float, 4> material_cache_clear_rgba(std::uint32_t argb);
// Depth profile9 disables both depth and stencil, so these cannot be cleared.
bool material_cache_clear_target(std::uint32_t flags, bool conditional_clear);
} // namespace civ5::materials
