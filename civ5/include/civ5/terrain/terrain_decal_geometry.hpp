#pragma once
#include "terrain_decal_assets.hpp"

namespace civ5::terrain {
using TerrainDecalPolyline = std::vector<Point2>;
using TerrainDecalPolylineView = std::span<const Point2>;
using TerrainDecalCellGeometry = std::vector<TerrainDecalPolyline>;
struct TerrainDecalMapView {
    std::int32_t width, height;
    // Expanded original byte mask, row-major. Bit 0x10 admits a point.
    std::span<const std::uint8_t> flags;
    float offset_x, offset_y;
};
struct TerrainDecalVertex {
    float x, y, u, v;
};
struct TerrainDecalGeometry {
    std::vector<TerrainDecalVertex> vertices;
    std::vector<std::uint16_t> indices;
};
// Original 768DA0 and 4E24A0: whole-polyline mask rejection, 0.25 half-width,
// normalized adjoining normals, authored Y scale and per-edge V increment.
// Polylines are in original linked-list order; each must contain >=1 point.
// Singletons are inert. Eligible degenerate/overflowing normals are rejected
// (the original emits nonfinite vertices); accepted output is bounded to the
// original submission's 65,534-index limit. No resampling occurs here.
TerrainDecalGeometry build_terrain_decal_geometry(
    std::span<const TerrainDecalPolyline> polylines, TerrainDecalMapView map);
// Borrowed views allow a road-network cell to read its owned segments without
// cloning their point arrays. Views need only survive this synchronous call.
TerrainDecalGeometry build_terrain_decal_geometry_views(
    std::span<const TerrainDecalPolylineView> polylines, TerrainDecalMapView map);

using TerrainDecalSubmit = std::function<std::uint32_t(std::size_t, std::uint32_t,
    const TerrainDecalRecord &, const TerrainDecalGeometry &)>;
// Connects dirty-cell removal/order, geometry, family-zero selection and the
// loaded palette. Empty/rejected geometry does not select or submit. Submit
// must consume/copy its borrowed mesh synchronously and return an owned handle.
// Grid/input ownership, mask generation and GPU submission remain with caller.
// Callbacks must not mutate the grid, geometry, mask, palette or random stream.
// Callback failure has the same partial-progress contract as refresh_terrain_decals.
void refresh_terrain_decal_geometry(std::int32_t width, std::int32_t height,
    std::span<TerrainDecalCell> cells, std::span<const TerrainDecalCellGeometry> geometry,
    TerrainDecalMapView map, const TerrainDecalPalette &palette, TerrainRandom &random,
    const TerrainDecalRemove &remove, const TerrainDecalSubmit &submit);
} // namespace civ5::terrain
