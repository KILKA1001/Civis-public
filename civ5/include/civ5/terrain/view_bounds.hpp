#pragma once
#include "world_layout.hpp"
#include <span>

namespace civ5::terrain {
// Original six-plane order. Only side planes 2..5 participate in 5BB6E0;
// near/far planes are retained camera inputs for other consumers.
using ViewFrustum = std::array<Vector4, 6>;
using TileBoundsPoint = std::array<std::int32_t, 2>;
// Original eight intersections of side planes with Z=0 and Z=127.5.
// Shared by 5BB6E0 visibility and 582FF0 overlay projection; failed plane
// intersections retain their zero-initialized point.
std::array<Vector3, 2> frustum_ground_bounds(const ViewFrustum &planes);
// Retail 0x0069CA50 feeds minimap kind 8 from the four intersections of
// left/right and bottom/top side planes with the source's horizontal plane.
// Corner order is left-bottom, left-top, right-bottom, right-top.
std::array<Vector3, 4> frustum_horizontal_corners(const ViewFrustum &planes,
                                                   float height);
struct ViewBounds {
    std::array<Vector3, 4> clipped_low, clipped_high, raw_low, raw_high;
    std::array<TileBoundsPoint, 4> clipped_tile_low, clipped_tile_high, raw_tile_low, raw_tile_high;
};
// Full original 5BB6E0, including plane intersections, cube rounding and
// independent world/tile clipping. Raw XY bounds feed reflection matrices;
// clipping can produce inverted extents for a view wholly outside the world.
// Inactive view slots retain their previous bits. Camera planes and original
// device/source scale factors remain explicit inputs.
ViewBounds view_bounds(const WorldLayout &world, std::span<const ViewFrustum> frusta,
                       const std::array<float, 2> &scales, const ViewBounds &prior, X87Precision precision);
} // namespace civ5::terrain
