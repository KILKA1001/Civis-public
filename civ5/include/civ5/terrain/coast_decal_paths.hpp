#pragma once
#include "coast_curves.hpp"

namespace civ5::terrain {
struct CoastDecalSource {
    // The base contour after original perturbation, before band construction.
    std::span<const CoastControlPoint> points;
    std::uint32_t owner_flag;
    bool closed;
};
struct CoastDecalPath {
    std::uint32_t first, count, source;
    bool closed;
};
struct CoastDecalPaths {
    std::vector<Point2> points;
    std::vector<CoastDecalPath> paths;
};
// Original 4A70C0: owner_flag==0 only, two corner-cut subdivisions, quadratic
// resampling and a five-unit normal offset. Original 4A5878..4A58E1 then converts
// to road-network coordinates: x -= center.x; y = y*.866f-center.y.
// Owns all output. Does not split paths into road-network segments/cells or
// choose decal variants. The explicit x87 precision controls tangent length.
CoastDecalPaths build_coast_decal_paths(std::span<const CoastDecalSource> sources,
                                        Point2 center, X87Precision precision);
} // namespace civ5::terrain
