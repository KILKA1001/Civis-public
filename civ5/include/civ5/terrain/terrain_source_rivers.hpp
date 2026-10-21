#pragma once
#include "river_network.hpp"
#include "world_layout.hpp"

namespace civ5::terrain {
struct TerrainRiverPlot {
    bool is_river = false, west = false, northwest = false, northeast = false;
    std::int32_t flow_e = -1, flow_se = -1, flow_sw = -1;
};
// Retail 40481E..404DE7 source-edge decisions and 77D500 normalization.
// Input is x-major. These are source-map coordinates, before worker padding.
// Feature append owners and downstream river geometry are separate stages.
std::vector<RiverEdge> build_terrain_river_edges(const WorldGrid &source_grid,
    std::span<const TerrainRiverPlot> plots, std::uint64_t maximum_edges = 1024 * 1024);
} // namespace civ5::terrain
