#include "civ5/terrain/land_styles.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::terrain {
LandStyleGrid::LandStyleGrid(std::span<const std::uint8_t> prior_bytes)
    : bytes_(prior_bytes.begin(), prior_bytes.end()) {
    if (bytes_.size() != size)
        throw std::invalid_argument("LAND styles require the full prior owner grid");
}
LandStyleGrid LandStyleGrid::copy_source_cells(std::int32_t width, std::int32_t height,
                                               std::span<const std::uint8_t> cells) const {
    if (width < 0 || height < 0 || width > stride || height > stride ||
        cells.size() != static_cast<std::size_t>(width * height * 8))
        throw std::invalid_argument("Invalid LAND source cells");
    auto result = *this;
    for (std::int32_t x = 0; x < width; ++x)
        for (std::int32_t y = 0; y < height; ++y)
            result.bytes_[x + y * stride] = cells[(x * height + y) * 8 + 4];
    return result;
}
std::uint8_t LandStyleGrid::at_flat_address(std::int32_t index) const {
    // Native has no independent X/Y clamp or wrap at this read.
    if (index < 0 || index >= size)
        throw std::out_of_range("LAND style read requires unprovided owner storage");
    return bytes_[index];
}
std::int32_t style_from_mask(std::uint32_t mask) {
    for (std::int32_t style = 1; style <= 4; ++style)
        if (mask & (1u << style))
            return style;
    return 0;
}
LandStyleResult accumulate_land_styles(const std::vector<std::vector<LogicalHex>> &patches,
                                       const IntMatrix4 &matrix, const LandStyleGrid &grid,
                                       std::span<const std::uint32_t> initial_masks) {
    if (initial_masks.size() != patches.size() || patches.size() > std::numeric_limits<std::int32_t>::max())
        throw std::invalid_argument("Invalid initial LAND masks");
    LandStyleResult result{{initial_masks.begin(), initial_masks.end()}, {}, {}};
    for (std::size_t p = 0; p < patches.size(); ++p) {
        if (patches[p].size() > 16)
            throw std::invalid_argument("Native LAND member capacity exceeded");
        for (std::size_t m = 0; m < patches[p].size(); ++m) {
            const auto &h = patches[p][m];
            auto component = [&](std::size_t c) {
                return wrap_add(wrap_add(wrap_add(wrap_mul(matrix[8 + c], h.z), wrap_mul(matrix[c], h.x)),
                                         wrap_mul(matrix[4 + c], h.y)),
                                matrix[12 + c]);
            };
            LogicalHex point{component(0), component(1), component(2)};
            const auto flat = wrap_add(wrap_add(wrap_mul(point.y, 255), point.y >> 1), point.x);
            const auto style = grid.at_flat_address(flat);
            result.masks[p] |= 1u << (style & 31);
            result.reads.push_back(
                {static_cast<std::int32_t>(p), static_cast<std::int32_t>(m), point, flat, style});
        }
        result.ordinary_styles.push_back(style_from_mask(result.masks[p]));
    }
    return result;
}
LandTraversalExtent land_traversal_extent(float min_x, float min_y, float max_x, float max_y, float scale_x,
                                          float scale_y) {
    for (auto value : {min_x, min_y, max_x, max_y, scale_x, scale_y})
        if (!std::isfinite(value))
            throw std::invalid_argument("Nonfinite LAND traversal input");
    if (scale_x <= 0 || scale_y <= 0 || max_x < min_x || max_y < min_y)
        throw std::invalid_argument("Invalid LAND traversal extent");
    auto count = [](float min, float max, float scale) {
        const float divisor = scale * 63.0f;
        const float quotient = (max - min) / divisor;
        if (!std::isfinite(divisor) || !std::isfinite(quotient) || quotient >= 2147483648.0f)
            throw std::invalid_argument("Unrepresentable LAND traversal count");
        return static_cast<std::int32_t>(quotient);
    };
    return {count(min_x, max_x, scale_x), count(min_y, max_y, scale_y)};
}
} // namespace civ5::terrain
