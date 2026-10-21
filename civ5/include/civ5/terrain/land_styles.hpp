#pragma once
#include "native_math.hpp"
#include <span>
#include <vector>

namespace civ5::terrain {
// Logical owner grid, NOT the dense height-worker material field. Prior bytes
// are required because native initialization does not clear unmodified cells.
class LandStyleGrid {
  public:
    static constexpr std::int32_t stride = 255, size = stride * stride;
    explicit LandStyleGrid(std::span<const std::uint8_t> prior_bytes);
    std::vector<std::uint8_t> owner_bytes() const { return bytes_; }
    LandStyleGrid copy_source_cells(std::int32_t width, std::int32_t height,
                                    std::span<const std::uint8_t> cells) const;
    std::uint8_t at_flat_address(std::int32_t index) const;

  private:
    std::vector<std::uint8_t> bytes_;
};
struct LandStyleRead {
    std::int32_t patch_index, member_index;
    LogicalHex transformed;
    std::int32_t flat_address, style;
};
struct LandStyleResult {
    std::vector<std::uint32_t> masks;
    std::vector<std::int32_t> ordinary_styles;
    std::vector<LandStyleRead> reads;
};
struct LandTraversalExtent {
    std::int32_t width, height;
};
std::int32_t style_from_mask(std::uint32_t mask);
LandStyleResult accumulate_land_styles(const std::vector<std::vector<LogicalHex>> &patches,
                                       const IntMatrix4 &owner_matrix, const LandStyleGrid &grid,
                                       std::span<const std::uint32_t> initial_masks);
LandTraversalExtent land_traversal_extent(float min_x, float min_y, float max_x, float max_y, float scale_x,
                                          float scale_y);
} // namespace civ5::terrain
