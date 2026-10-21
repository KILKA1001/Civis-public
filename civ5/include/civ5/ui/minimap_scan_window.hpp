#pragma once

#include "civ5/terrain/height_owner.hpp"
#include "civ5/terrain/world_layout.hpp"
#include "civ5/ui/minimap_draw_batches.hpp"
#include "civ5/ui/minimap_visible_bounds.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace civ5::ui {
namespace detail {
// Retail 0x00573040 uses floor(v+0.5) for nonnegative values and
// ceil(v-0.5) for negative values. If the three rounded lanes no longer sum
// to zero, it fixes the lane with the largest directed rounding error;
// equal errors resolve in q, r, s order.
inline std::array<std::int32_t, 3> minimap_round_cube(std::array<float, 3> cube) {
    std::array<std::int32_t, 3> rounded{};
    for (std::size_t i = 0; i < 3; ++i) {
        if (!std::isfinite(cube[i]) || std::abs(cube[i]) > 1000000.0f)
            throw std::invalid_argument("Invalid inverse minimap cube position");
        const float biased = cube[i] < 0.0f ? cube[i] - 0.5f : cube[i] + 0.5f;
        rounded[i] =
            static_cast<std::int32_t>(cube[i] < 0.0f ? std::ceil(static_cast<double>(biased))
                                                     : std::floor(static_cast<double>(biased)));
    }
    const auto sum = rounded[0] + rounded[1] + rounded[2];
    if (sum != 0) {
        std::size_t chosen = 0;
        const auto error = [&](std::size_t i) {
            return sum < 0 ? static_cast<float>(rounded[i]) - cube[i]
                           : cube[i] - static_cast<float>(rounded[i]);
        };
        float smallest_error = error(0);
        for (std::size_t i = 1; i < 3; ++i) {
            if (error(i) < smallest_error) {
                smallest_error = error(i);
                chosen = i;
            }
        }
        rounded[chosen] = -rounded[(chosen + 1) % 3] - rounded[(chosen + 2) % 3];
    }
    return rounded;
}
} // namespace detail

// DX9 0x00767E70 maps the visible quad's opposite world corners back through
// the minimap owner and D80E40 inverse matrix. It rounds each cube position,
// applies the tile grid's first integer matrix, then expands by one cell on
// every side. The resulting scan can cross the logical grid boundary; those
// reads use the grid sentinel and are needed for the fog pass.
inline MinimapCellWindow minimap_scan_window(MinimapWorldBounds visible,
                                             const terrain::WorldLayout &layout,
                                             const terrain::HeightOwnerParameters &owner) {
    if (!(std::isfinite(visible.min_x) && std::isfinite(visible.min_y) &&
          std::isfinite(visible.max_x) && std::isfinite(visible.max_y) &&
          visible.min_x < visible.max_x && visible.min_y < visible.max_y &&
          std::isfinite(owner.scale_x) && std::isfinite(owner.scale_y) &&
          std::isfinite(owner.center_x) && std::isfinite(owner.center_y) && owner.scale_x != 0.0f &&
          owner.scale_y != 0.0f))
        throw std::invalid_argument("Invalid minimap scan transform");

    const auto &inverse = layout.transforms;
    const auto &matrix = layout.source_grid.first_matrix;
    const auto corner = [&](float world_x, float world_y) {
        const float x = (owner.center_x + world_x) / owner.scale_x;
        const float y = (owner.center_y + world_y) / owner.scale_y;
        std::array<float, 3> cube{};
        for (std::size_t i = 0; i < 3; ++i) {
            cube[i] = x * inverse[i] + y * inverse[4 + i] + inverse[12 + i];
        }
        const auto rounded = detail::minimap_round_cube(cube);
        const auto component = [&](std::size_t lane) {
            std::int64_t value = matrix[12 + lane];
            for (std::size_t i = 0; i < 3; ++i)
                value += static_cast<std::int64_t>(rounded[i]) * matrix[4 * i + lane];
            if (value < -1000000 || value > 1000000)
                throw std::invalid_argument("Invalid minimap tile position");
            return static_cast<std::int32_t>(value);
        };
        const auto q = component(0);
        const auto r = component(1);
        return std::array<std::int32_t, 2>{q + (r >> 1), r};
    };
    const auto low = corner(visible.min_x, visible.min_y);
    const auto high = corner(visible.max_x, visible.max_y);
    const MinimapCellWindow result{low[0] - 1, low[1] - 1, high[0] + 1, high[1] + 1};
    if (result.left > result.right || result.top > result.bottom)
        throw std::invalid_argument("Invalid minimap scan bounds");
    return result;
}

} // namespace civ5::ui
