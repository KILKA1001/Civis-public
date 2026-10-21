#pragma once
#include "height_addressing.hpp"
#include "river_network.hpp"
#include "river_raster.hpp"
#include "terrain_occurrences.hpp"

namespace civ5::terrain {
using RiverPoint = Vector3;
struct RiverPolyline {
    std::vector<RiverPoint> points;
    bool terminal = false;
};
using RiverPolylines = std::vector<RiverPolyline>;
enum class RiverControlStage { controls, smoothed, jittered };
using RiverControlObserver = std::function<void(RiverControlStage, const RiverPolylines &, const TerrainRandom &)>;
// Original edge controls, one wavelet smoothing level and both jitter passes.
// The matrix is D80E80 (WorldLayout::transforms[16..31]); random state continues
// across lines and must subsequently continue into the width producer.
RiverPolylines build_river_centerlines(const RiverNetwork &network, const Matrix4 &world_inverse, TerrainRandom &random,
                                       const RiverControlObserver &observer = {});
// Fresh original coordinate-cache registration, then downstream extensions and
// terminal flags. Mode controls the original cache boundary gates.
RiverPolylines extend_river_centerlines(const RiverNetwork &network, const RiverPolylines &lines,
                                        const Matrix4 &world_inverse, std::int32_t mode,
                                        const ReciprocalSqrtEstimate &estimate);
// First/second original bank families; their own terminal flags remain false.
std::array<RiverPolylines, 2> widen_river_centerlines(const RiverPolylines &lines, TerrainRandom &random,
                                                      const ReciprocalSqrtEstimate &estimate);
// Original sampler skips points outside Y and wraps X after adding 100 widths.
// Stream construction requires complete per-line samples and rejects a skipped
// point instead of reading uninitialized elements beyond the native count.
using RiverHeights = std::vector<std::vector<float>>;
RiverHeights sample_river_heights(const HeightDomain &domain, std::span<const std::uint16_t> heights,
                                  const RiverPolylines &lines);
struct RiverTriangleStreams {
    std::vector<RiverTriangle> banks, channels;
};
using RiverCurveObserver = std::function<void(bool derivative, std::span<const RiverPoint> controls, float parameter,
                                              const RiverPoint &point)>;
// Original 745BA0 resampling, bank strips and caps, and depth-profile triangles.
// The bank profile lane is unused by 46EF30 and is initialized to zero here.
RiverTriangleStreams build_river_triangles(const RiverPolylines &centers, const std::array<RiverPolylines, 2> &banks,
                                           const RiverHeights &heights, const ReciprocalSqrtEstimate &estimate,
                                           const RiverCurveObserver &observer = {});
} // namespace civ5::terrain
