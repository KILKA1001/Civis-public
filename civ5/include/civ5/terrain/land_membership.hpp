#pragma once
#include "height_addressing.hpp"
#include "native_math.hpp"

namespace civ5::terrain {
struct LandCandidate {
    std::int32_t patch_index;
    bool intersects;
};
struct LandHexMembership {
    std::int32_t map_x, map_y;
    LogicalHex logical;
    Point2 uncentered;
    std::array<Point2, 6> corners;
    std::int32_t candidate_x, candidate_y;
    std::array<LandCandidate, 9> candidates;
};
struct LandMembership {
    // Original X-major order. Repeated candidates/members MUST survive.
    std::vector<LandHexMembership> hexes;
    std::vector<std::vector<LogicalHex>> patches;
};
std::int32_t unsigned_wrapped_candidate(std::int32_t value, std::int32_t count);
bool edge_intersects(const std::array<float, 6> &bounds, Point2 a, Point2 b);
// Assigns logical hexes to already-constructed LAND patches. This is NOT
// descriptor-to-height-worker ownership and does not repair coverage holes.
LandMembership build_land_membership(const LandLayout &layout, std::int32_t map_width,
                                     std::int32_t map_height, const IntMatrix4 &map_to_logical,
                                     const Matrix4 &world, Point2 owner_center, X87Precision precision,
                                     MatrixPath path);
} // namespace civ5::terrain
