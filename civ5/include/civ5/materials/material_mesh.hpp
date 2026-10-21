#pragma once
#include "civ5/assets/asset_source.hpp"

namespace civ5::materials {
// Computed material48 patch input, not the normal24 or LAND24 declaration.
// Native producer/score generation remains separate from this GPU consumer.
class MaterialPatchMesh {
  public:
    MaterialPatchMesh(int columns, int rows, std::span<const std::uint8_t> material48,
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
