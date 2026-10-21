#pragma once
#include "river_geometry.hpp"

namespace civ5::terrain {
struct RiverSurfaceSettings {
    Vector3 offset{};
    float height_scale = .2f;
    bool high_quality = true;
    X87Precision precision = X87Precision::double_precision;
};
// Original format 10: POSITION.xyz, TEXCOORD0.xy, TEXCOORD1.xy, unused lane.
// The original copies unspecified stack bytes into the last lane; this port
// initializes it to zero. No consumed position or UV component is omitted.
using RiverSurfaceVertex = std::array<float, 8>;
struct RiverSurfaceMesh {
    std::vector<RiverSurfaceVertex> vertices;
    std::vector<std::uint32_t> indices;
};
// Original 659620, with 6AAE83/61F890 family order (left, right, center, heights).
// This surface is distinct from the 745BA0 bank/channel raster streams.
RiverSurfaceMesh build_river_surface(const RiverPolylines &centers,
                                     const std::array<RiverPolylines, 2> &banks, const RiverHeights &heights,
                                     const RiverSurfaceSettings &settings,
                                     const RiverCurveObserver &observer = {});
} // namespace civ5::terrain
