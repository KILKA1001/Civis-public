#pragma once
#include "civ5/assets/rgba8.hpp"
#include <array>

namespace civ5::materials {
// File-order4924D0 expansion. Source indices, row indices, U16 resource-table
// indices and GL names are separate domains. No biome/override heuristics.
class MaterialExpansion {
  public:
    explicit MaterialExpansion(std::span<const std::int32_t> projections);
    std::span<const std::int32_t> source_rows() const { return rows_; }
    std::span<const std::int32_t> source_modes() const { return modes_; }
    // -1 means untouched by the bounded native loop, not an invented native base.
    std::span<const std::int32_t> base_slots() const { return bases_; }
    std::array<std::uint16_t, 16> ordinary_slots(std::span<const std::int32_t> selected,
                                                 std::span<const std::uint32_t> diffuse,
                                                 std::span<const std::uint32_t> height) const;

  private:
    std::vector<std::int32_t> rows_, modes_, bases_;
};

float material_texture_multiplier(float scale, std::int32_t projection, float global_span);
// Original5BC9E0 active float4 producer. Caller owns all96 resident entries:
// only the active prefix is replaced; unavailable tails are preserved. This
// function does not create a binding adapter or emulate D3D descriptor pools.
// Rejects unsupported inputs before changing the destination.
void write_material_constants(std::span<const std::int32_t> source_rows,
                              std::span<const std::int32_t> source_modes, std::span<const float> row_scales,
                              float global_span, std::span<float, 384> destination);

// Original8E2270 / D3DX converter: seven independently filled RGBA8 mips,
// 256..4, including alpha dithering and reverse traversal on odd rows.
std::array<assets::Rgba8Level, 7> material_resource_zero();
} // namespace civ5::materials
