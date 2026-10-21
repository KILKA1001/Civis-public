#pragma once
#include "world_layout.hpp"
#include <span>
#include <vector>

namespace civ5::terrain {
struct TerrainAuxiliaryPlot {
    std::int32_t terrain, plot, continent, feature;
};
struct TerrainReliefSource {
    std::int16_t x, y;
    std::uint8_t kind; // 0: water, 2: mountain, before feature mask overrides.
};
struct TerrainFeature2Source {
    LogicalHex position;
    std::int32_t terrain, continent;
};
struct TerrainAuxiliarySources {
    // Identical compact records go to original 4D57C0 and 5D3730.
    std::vector<TerrainReliefSource> relief;
    // Numeric feature IDs are those of the active content profile. Do not
    // replace these retail predicates with feature-name guesses.
    std::vector<TerrainFeature2Source> feature2;
    std::vector<LogicalHex> feature0, feature4;
};
// Source production in 404004..404DE7, before append-owner side effects.
// Input and each output stream follow x-major source order. Feature 2 belongs
// to pass one; features 0/4 belong to pass two. No RNG or models are involved.
// The compact coordinates truncate to signed 16 bits; feature-2 continent
// sign-extends its low byte, while the other coordinates retain 32 bits.
TerrainAuxiliarySources build_terrain_auxiliary_sources(const WorldGrid &source_grid,
    std::span<const TerrainAuxiliaryPlot> plots, std::uint64_t maximum_cells = 1024 * 1024);
} // namespace civ5::terrain
