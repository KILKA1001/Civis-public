#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace civ5::terrain {
// Original format0x10 shared local grid, before displacement (not CPU fallback).
struct LandGridVertex {
    std::array<float, 3> position;
    std::array<std::uint16_t, 4> normal;
    std::array<std::uint16_t, 2> uv;
};
static_assert(sizeof(LandGridVertex) == 24 && offsetof(LandGridVertex, normal) == 12 &&
              offsetof(LandGridVertex, uv) == 20);
std::uint16_t land_unit_half(float value);
// Original4DA080 traversal;66x66 is allowed only for material step1 consumers.
std::vector<std::uint16_t> land_grid_indices(int columns, int rows, int step);
class LandGrid {
  public:
    LandGrid(int columns, int rows, int step);
    std::span<const LandGridVertex> vertices() const { return vertices_; }
    std::span<const std::uint16_t> indices() const { return indices_; }

  private:
    std::vector<LandGridVertex> vertices_;
    std::vector<std::uint16_t> indices_;
};
// Separate packed control addresses, never an index buffer into LandGrid.
struct LandControlLevel {
    int columns, rows;
    std::size_t first_address, address_count;
};
struct LandControlStream {
    std::array<LandControlLevel, 5> levels;
    std::vector<std::uint16_t> addresses;
};
LandControlStream land_control_stream(int columns, int rows);
} // namespace civ5::terrain
