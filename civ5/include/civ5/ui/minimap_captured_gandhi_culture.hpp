#pragma once

#include "civ5/ui/minimap_tile_grid.hpp"
#include <array>
#include <cstdint>
#include <stdexcept>

namespace civ5::ui {

// Diagnostic data for the pinned Gandhi save only. These are every nonzero
// byte-0 cell in the two identical complete native grid samples (raw grid
// SHA-256 455b3049859f1215ed37537935e8e7bc5a631624d0aa6129a8f807177906e8f3).
// Native 0x007687B5 subtracts one from the culture group payload before
// 0x004193DE indexes the context's eight-byte player color table. Payload 1
// therefore reads player zero's second color, captured as 12 87 06 FF.
struct GandhiNativeCultureCell {
    std::int32_t x, y;
    std::uint8_t byte3;
};
inline constexpr std::array<GandhiNativeCultureCell, 6>
    gandhi_native_culture_cells{{
        {32, 14, 0x38}, {33, 14, 0x74}, {31, 15, 0xA8},
        {32, 15, 0x40}, {32, 16, 0x8C}, {33, 16, 0xD4},
    }};
inline constexpr std::array<std::uint8_t, 3> gandhi_native_culture_rgb{18, 135, 6};

inline void apply_gandhi_native_culture(MinimapTileGrid &grid) {
    if (grid.width() != 40 || grid.height() != 24 || !grid.wrap_x())
        throw std::invalid_argument("Captured Gandhi culture needs its native 40x24 grid");
    for (const auto &cell : gandhi_native_culture_cells) {
        auto &record = grid.at(cell.x, cell.y);
        if ((record.byte1 & 3u) != 0 || record.byte0 != 0)
            throw std::invalid_argument("Captured Gandhi culture conflicts with map terrain");
        record.byte0 = 1;
        record.byte3 = cell.byte3;
    }
}

} // namespace civ5::ui
