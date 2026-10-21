#pragma once
#include "terrain_decal_geometry.hpp"
#include "world_layout.hpp"
#include <span>

namespace civ5::terrain {
inline constexpr std::uint16_t absent_terrain_road_cell = 0xffff;
inline constexpr std::uint32_t absent_terrain_road_segment = 0x0fffffff;
struct TerrainRoadConnection {
    std::uint32_t segment;
    bool first;
    // Original bits 29..30: any nonzero value excludes endpoint snapping.
    std::uint8_t boundary;
};
using TerrainRoadJunction = std::vector<TerrainRoadConnection>;
struct TerrainRoadSegment {
    TerrainDecalPolyline points;
    std::uint16_t first_junction, last_junction;
    std::array<float, 4> bounds{}; // min X/Y, max X/Y, before endpoint snapping
    std::uint32_t group = 0;
    std::uint16_t cell = absent_terrain_road_cell;
    std::uint8_t type = 0, flags = 0;
};
struct TerrainRoadGrid {
    std::int32_t width, height;
    std::array<float, 4> bounds;
    float span_x, span_y;
    std::vector<TerrainDecalCell> cells;
    // Segment indices in original per-cell linked-list order. The cell's
    // geometry words are {0, count, first, last}; zero replaces its native pointer.
    std::vector<std::vector<std::uint32_t>> members;
    std::vector<TerrainRoadSegment> segments;
    std::vector<TerrainRoadJunction> junctions;
};
// Original 5A2800/50B440. Admitted cell count <=65535 avoids aliasing the native
// U16 absent-cell sentinel. Source bounds must have positive finite spans.
TerrainRoadGrid make_terrain_road_grid(std::int32_t width, std::int32_t height,
                                       std::array<float, 4> bounds);
// Original 77A700 uses twice SOURCE grid dimensions and the world bounds.
TerrainRoadGrid make_terrain_road_grid(const WorldLayout &world);
// Original 677580: translate X by wrap_count*span_x, then clamp to X bounds.
void shift_terrain_road_segment(TerrainRoadGrid &grid, std::uint32_t segment, std::int32_t wrap_count);
// Original 4332F0: detach prior membership, assign by point[count/2], append,
// mark the destination dirty, compute bounds, then synchronize junction ends.
// X exactly at the maximum goes to the last column; Y >= height stays unbound.
// Detaching alone does not dirty the previous cell. Snapping does not recompute
// other segments' bounds/cells or dirty flags. These native orderings are kept.
// Input segments and ordered junction links come from the network producer.
void publish_terrain_road_segment(TerrainRoadGrid &grid, std::uint32_t segment,
                                   std::uint32_t group, std::uint8_t type, std::uint8_t flags);
using TerrainRoadDirtyObserver = std::function<void(std::size_t)>;
// Original 612D20: command 0x21 carries centered scene bounds, unlike the
// uncentered raster bounds in scene-mask command 0x20. Add the explicit world
// center, truncate scaled bounds, include the upper cells and wrap both axes
// with signed remainder. Visits Y first, X second; repeated writes are retained.
// Inputs addressing outside owned cells or exceeding bounded work are rejected
// before writing. The observer must not mutate the grid.
void dirty_terrain_road_region(TerrainRoadGrid &grid,std::array<float,4> bounds,Point2 world_offset,
    const TerrainRoadDirtyObserver &observer={});
// Reads each dirty cell's owned lines through borrowed views and connects the
// validated geometry/palette/refresh stages. Callbacks must not mutate grid,
// mask, palette or random state; submission consumes/copies its mesh synchronously.
void refresh_terrain_road_decals(TerrainRoadGrid &grid, TerrainDecalMapView map,
    const TerrainDecalPalette &palette, TerrainRandom &random,
    const TerrainDecalRemove &remove, const TerrainDecalSubmit &submit);
// Registered 5438A0 handler for command 0x21: mark the region, then run the
// ordinary refresh over ALL dirty cells, including earlier pending changes.
// An empty rectangle still refreshes pending cells. The map's center offsets
// represent the same original world owner used for region marking.
// Downstream callback/refresh failures can leave cells marked or partially
// refreshed, matching the existing refresh callback contract.
void refresh_terrain_road_region(TerrainRoadGrid &grid,std::array<float,4> bounds,TerrainDecalMapView map,
    const TerrainDecalPalette &palette,TerrainRandom &random,
    const TerrainDecalRemove &remove,const TerrainDecalSubmit &submit,
    const TerrainRoadDirtyObserver &observer={});
// Retail 4A9A56 enqueues a 128-byte command with {size=0x80,id=0x21,
// reserved=0,scene bounds} at offsets 0,4,8,16. New packets start zeroed;
// the original allocator leaves bytes beyond the written header untouched.
// Keep the packet byte-shaped so producer and consumer can use the same queue.
using TerrainRoadRegionCommand=std::array<std::uint8_t,128>;
TerrainRoadRegionCommand make_terrain_road_region_command(std::array<float,4> bounds);
// The registered 5438A0 callback receives byte 16 onward, not the header.
void dispatch_terrain_road_region_payload(std::span<const std::uint8_t> payload,
    TerrainRoadGrid &grid,TerrainDecalMapView map,
    const TerrainDecalPalette &palette,TerrainRandom &random,
    const TerrainDecalRemove &remove,const TerrainDecalSubmit &submit,
    const TerrainRoadDirtyObserver &observer={});
void dispatch_terrain_road_region_command(const TerrainRoadRegionCommand &command,
    TerrainRoadGrid &grid,TerrainDecalMapView map,
    const TerrainDecalPalette &palette,TerrainRandom &random,
    const TerrainDecalRemove &remove,const TerrainDecalSubmit &submit,
    const TerrainRoadDirtyObserver &observer={});
} // namespace civ5::terrain
