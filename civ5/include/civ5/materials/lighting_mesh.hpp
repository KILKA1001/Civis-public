#pragma once
#include "civ5/assets/asset_source.hpp"

namespace civ5::materials {
// Original key4B consumes normal24 half4 NORMAL+12 / half2 TEXCOORD0+20.
// POSITION and NORMAL.w are retained, but are not shader inputs used by key4B.
class LightingPatchMesh {
  public:
    LightingPatchMesh(int columns, int rows, std::span<const std::uint8_t> normal24,
                      std::span<const std::uint16_t> indices);
    int columns() const { return columns_; }
    int rows() const { return rows_; }
    std::span<const std::uint8_t> vertices() const { return vertices_; }
    std::span<const std::uint16_t> indices() const { return indices_; }

  private:
    int columns_, rows_;
    assets::Bytes vertices_;
    std::vector<std::uint16_t> indices_;
};
} // namespace civ5::materials
