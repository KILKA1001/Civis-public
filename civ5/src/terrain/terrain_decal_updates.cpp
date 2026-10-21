#include "civ5/terrain/terrain_decal_updates.hpp"
#include <stdexcept>

namespace civ5::terrain {
std::uint32_t select_terrain_decal(TerrainDecalRange range, TerrainRandom &random) {
    if (range.end < range.first)
        throw std::invalid_argument("Reversed terrain decal range");
    const auto count = range.end - range.first;
    return range.first + (count > 1 ? random.next() % count : 0);
}
void refresh_terrain_decals(std::int32_t width, std::int32_t height,
                            std::span<TerrainDecalCell> cells,
                            const TerrainDecalRemove &remove, const TerrainDecalRebuild &rebuild) {
    if (width < 0 || height < 0 || std::uint64_t(width) * std::uint64_t(height) != cells.size() ||
        !remove || !rebuild)
        throw std::invalid_argument("Invalid terrain decal refresh inputs");
    if (cells.empty()) return;
    for (std::int32_t y = 0; y < height; ++y)
        for (std::int32_t x = 0; x < width; ++x) {
            const auto index = std::size_t(x) * std::size_t(height) + std::size_t(y);
            auto &cell = cells[index];
            if (!cell.dirty) continue;
            if (cell.handle != absent_terrain_decal) {
                remove(index, cell.handle);
                cell.handle = absent_terrain_decal;
            }
            cell.handle = rebuild(index, cell);
            cell.dirty = 0;
        }
}
} // namespace civ5::terrain
