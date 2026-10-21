#pragma once
#include "coast_curves.hpp"
#include <optional>

namespace civ5::terrain {
struct CoastCellClass {
    std::uint8_t category, style, displacement_flag;
    bool operator==(const CoastCellClass &) const = default;
};
struct CoastSegment {
    std::uint8_t direction, style;
    Point2 first, second;
};
struct CoastMapCell {
    CoastCellClass classification;
    std::vector<CoastSegment> segments;
    std::vector<std::uint8_t> opposite_directions;
};
struct CoastMapSource {
    std::int32_t width, height;
    // Original source order: x*height+y. Generated graph cells are row-major.
    std::span<const std::uint32_t> masks;
    IntMatrix4 map_to_logical, logical_to_map;
    Matrix4 world;
    bool wrap_x;
    X87Precision precision;
    Vec4Path transform_path;
};
CoastCellClass classify_coast_mask(std::uint32_t mask);
class CoastSegmentGrid {
  public:
    std::int32_t width() const { return width_; }
    std::int32_t height() const { return height_; }
    std::span<const CoastMapCell> cells() const { return cells_; }
    Point2 origin() const { return origin_; }
    Point2 wrap_delta() const { return wrap_; }
    const std::vector<Point2> &corners() const { return corners_; }
    const std::vector<std::array<Point2, 2>> &offsets() const { return offsets_; }

  private:
    friend class CoastMapGraph;
    CoastSegmentGrid() = default;
    std::int32_t width_, height_;
    std::vector<CoastMapCell> cells_;
    Point2 origin_{0, 0}, wrap_{0, 0};
    std::vector<Point2> corners_;
    std::vector<std::array<Point2, 2>> offsets_;
};
struct CoastEdgeLink {
    std::int32_t x, y, edge;
};
struct CoastEdgeLinks {
    CoastEdgeLink forward, reverse;
};
struct CoastContour {
    std::int32_t start_x, start_y, start_edge;
    bool closed;
    std::optional<Point2> wrap_offset;
    std::uint32_t owner_flag, family;
    std::vector<CoastControlPoint> points;
};
// Owned graph built from source masks and explicit coordinate/FP state. No
// retained views of source buffers and no captured endpoint input. Missing
// fallback cells / native edge-byte-255 traversal remain explicit errors.
class CoastMapGraph {
  public:
    static CoastMapGraph build(const CoastMapSource &source);
    const CoastSegmentGrid &initial() const { return initial_; }
    const std::vector<std::vector<CoastEdgeLinks>> &links() const { return links_; }
    using StageObserver = std::function<void(unsigned, unsigned, const CoastSegmentGrid &)>;
    CoastSegmentGrid smooth(X87Precision precision, const StageObserver &observer = {}) const;
    std::vector<CoastContour> scan(const CoastSegmentGrid &geometry) const;

  private:
    CoastMapGraph() = default;
    CoastSegmentGrid initial_;
    std::vector<std::vector<CoastEdgeLinks>> links_;
    bool wrap_x_;
};
} // namespace civ5::terrain
