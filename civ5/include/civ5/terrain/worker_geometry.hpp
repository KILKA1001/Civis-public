#pragma once
#include "coast_curves.hpp"
#include "worker_gates.hpp"

namespace civ5::terrain {
struct WorkerGateGeometry {
    std::vector<Point2> points;
    std::vector<WorkerGateRange> boundaries, fills;
};
// All third curves copied first, then ordered unit-offset fill pairs with
// native in-place near-point/intersection repair. Not height-field smoothing.
WorkerGateGeometry build_worker_geometry(std::span<const CoastCurveSet> curves, X87Precision precision);
} // namespace civ5::terrain
