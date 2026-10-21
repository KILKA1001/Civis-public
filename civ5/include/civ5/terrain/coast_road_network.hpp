#pragma once
#include "coast_decal_paths.hpp"
#include "terrain_road_grid.hpp"

namespace civ5::terrain {
// Original 7B0C50 -> 498840 -> 6BBA20 coast caller (type zero). The group is
// the caller's incrementing path ID. Existing segments must belong to this
// same coast network; nonzero road types and free-list removal are separate.
// Keeps original chunk tails, simplification, crossing termination and wrapping.
// Valid open paths have >=16 points; closed paths have >=4. Mutations are
// incremental, as in the original. Build a fresh grid for atomic publication.
void insert_coast_road_path(TerrainRoadGrid &grid, std::span<const Point2> points,
                            bool closed, std::uint32_t group, X87Precision precision);
// Build and retain the complete coast network using this world's own bounds.
TerrainRoadGrid build_coast_road_network(const CoastDecalPaths &paths, const WorldLayout &world,
                                        X87Precision precision);
} // namespace civ5::terrain
