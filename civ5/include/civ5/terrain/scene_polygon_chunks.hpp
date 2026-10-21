#pragma once
#include "scene_mask.hpp"

namespace civ5::terrain {
struct ScenePolygonPointChunk {
    // Original 895DF0 packet fields +14, +18, +1C and +20..+9F.
    std::uint32_t block_index, expected_packets, point_count;
    std::vector<Point2> points;
};

// The point list consumed by 475020/68B640 is assembled from source packets.
// Packet order is significant. A missing preceding block receives copies of
// the next packet's first point. The original subtraction cannot safely accept
// a later packet whose end is below the current list extent.
class ScenePolygonPointAssembly {
  public:
    bool append(const ScenePolygonPointChunk &packet);
    std::span<const Point2> points() const { return points_; }
    std::uint32_t expected_packets() const { return expected_packets_; }
    std::uint32_t received_packets() const { return received_packets_; }
  private:
    std::uint32_t expected_packets_=0xffffffffu,received_packets_=0;
    std::vector<Point2> points_;
};
} // namespace civ5::terrain
