#pragma once

#include "civ5/ui/minimap_tile_grid.hpp"
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace civ5::ui {
struct MinimapCellWindow {
    std::int32_t left, top, right, bottom; // inclusive, before optional X wrap
};

struct MinimapDrawCell {
    std::int32_t x, y; // unwrapped coordinates for world geometry
    std::uint8_t edge_mask;
    bool operator==(const MinimapDrawCell &) const = default;
};

struct MinimapCultureBatch {
    std::uint8_t payload;
    std::vector<MinimapDrawCell> cells;
};

struct MinimapDrawBatches {
    std::vector<MinimapDrawCell> fog, coast, land;
    std::vector<MinimapCultureBatch> culture;
};

// Native 0x00767E70 scans X outside Y four times. Fog, coast and land each
// have their own mesh; distinct nonzero byte-0 values form culture meshes in
// first-seen order. The renderer uses these batches in its separate pass order.
inline MinimapDrawBatches build_minimap_draw_batches(const MinimapTileGrid &grid,
                                                      MinimapCellWindow window) {
    const auto width = static_cast<std::int64_t>(window.right) - window.left + 1;
    const auto height = static_cast<std::int64_t>(window.bottom) - window.top + 1;
    if (width <= 0 || height <= 0 || width > 1024 || height > 1024)
        throw std::invalid_argument("Minimap cell window is invalid or excessive");
    MinimapDrawBatches result;
    const auto scan = [&](auto action) {
        for (std::int64_t x = window.left; x <= window.right; ++x)
            for (std::int64_t y = window.top; y <= window.bottom; ++y)
                action(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y),
                       grid.at(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)));
    };
    scan([&](std::int32_t x, std::int32_t y, const MinimapTileRecord &record) {
        if (record.byte2 & 1u)
            result.fog.push_back({x, y, static_cast<std::uint8_t>(record.byte2 >> 2)});
    });
    scan([&](std::int32_t x, std::int32_t y, const MinimapTileRecord &record) {
        if ((record.byte1 & 3u) == 1u)
            result.coast.push_back({x, y, static_cast<std::uint8_t>(record.byte1 >> 2)});
    });
    scan([&](std::int32_t x, std::int32_t y, const MinimapTileRecord &record) {
        if ((record.byte1 & 3u) == 0u)
            result.land.push_back({x, y, static_cast<std::uint8_t>(record.byte1 >> 2)});
    });
    scan([&](std::int32_t, std::int32_t, const MinimapTileRecord &record) {
        if (record.byte0 == 0) return;
        const auto found = std::find_if(result.culture.begin(), result.culture.end(),
                                        [&](const auto &batch) { return batch.payload == record.byte0; });
        if (found == result.culture.end())
            result.culture.push_back({record.byte0, {}});
    });
    for (auto &batch : result.culture)
        scan([&](std::int32_t x, std::int32_t y, const MinimapTileRecord &record) {
            if (record.byte0 == batch.payload)
                batch.cells.push_back({x, y, static_cast<std::uint8_t>(record.byte3 >> 2)});
        });
    return result;
}
} // namespace civ5::ui
