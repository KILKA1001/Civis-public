#pragma once
#include "civ5/terrain/coast_limits.hpp"
#include "civ5/terrain/height_addressing.hpp"
#include "civ5/terrain/native_math.hpp"
#include <span>
#include <vector>

namespace civ5::terrain {
struct WorkerGateLine {
    // Consecutive edges only: a single point is not a segment; closure is explicit.
    std::span<const Point2> points;
    std::uint32_t gate;
};
struct WorkerGateSeed {
    std::int32_t x, y;
    std::uint32_t gate;
    bool operator==(const WorkerGateSeed &) const = default;
};
struct WorkerGateRange {
    std::uint32_t first, end, gate;
    bool operator==(const WorkerGateRange &) const = default;
};
// Extent 64 is explicit, not recovery of the opaque native initializer producer.
// Returns row-major local gates; input lines/seeds are borrowed for this call only.
std::array<std::uint8_t, 4096> classify_worker_gates64(HeightDomain domain, std::int32_t origin_x,
                                                       std::int32_t origin_y,
                                                       std::span<const WorkerGateLine> boundaries,
                                                       std::span<const WorkerGateLine> fills,
                                                       std::span<const WorkerGateSeed> seeds);

struct WorkerGateSource {
    std::int32_t width, height;
    // Native logical cells are X-major: mask(x,y) = masks[x * height + y].
    // This is the u32 terrain mask, not an art-style byte or gameplay adapter.
    std::span<const std::uint32_t> masks;
    IntMatrix4 map_to_logical, logical_to_map;
    Matrix4 world_transform;
    bool wrap_x;
    std::uint8_t outside_category;
    X87Precision precision;
    Vec4Path transform_path;
};
struct RoutedGateWorker {
    std::int32_t origin_x, origin_y;
    std::vector<WorkerGateRange> boundaries, fills;
    std::vector<WorkerGateSeed> seeds;
};
class WorkerGateRouting {
  public:
    HeightDomain domain() const { return domain_; }
    std::span<const std::uint8_t> logical_categories() const { return categories_; }
    std::span<const RoutedGateWorker> workers() const { return workers_; }
    std::array<std::uint8_t, 4096> classify_worker64(std::size_t index) const;
    std::vector<std::uint8_t> build_gate_field64() const;

  private:
    WorkerGateRouting(HeightDomain domain, std::vector<std::uint8_t> categories, std::vector<Point2> points,
                      std::vector<RoutedGateWorker> workers);
    HeightDomain domain_;
    std::vector<std::uint8_t> categories_;
    std::vector<Point2> points_;
    std::vector<RoutedGateWorker> workers_;
    friend WorkerGateRouting route_worker_gates(HeightDomain, const WorkerGateSource &,
                                                std::span<const Point2>, std::span<const WorkerGateRange>,
                                                std::span<const WorkerGateRange>);
};
// Owns all returned data. Geometry, transforms, FP state and fallback category
// remain explicit inputs; this does not generate curves or recover runtime state.
WorkerGateRouting route_worker_gates(HeightDomain domain, const WorkerGateSource &source,
                                     std::span<const Point2> points,
                                     std::span<const WorkerGateRange> boundaries,
                                     std::span<const WorkerGateRange> fills);
} // namespace civ5::terrain
