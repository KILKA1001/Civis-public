#pragma once
#include "view_bounds.hpp"

namespace civ5::terrain {
struct PerspectiveCamera {
    // Original camera subobject +8/+C/+D0/+D4; FOV is vertical, in degrees.
    float field_of_view, aspect_ratio, near_plane, far_plane;
    Matrix4 world;
};
struct CameraFrameState {
    Matrix4 projection;
    std::array<Matrix4, 4> views, inverse_views;
    // Original order: far, near, right, left, bottom, top.
    std::array<ViewFrustum, 4> frusta;
    std::array<Vector3, 4> relative_offsets;
    std::uint32_t view_count;
};

// Original base-camera 96BA80: one inverse followed by the perspective matrix.
std::array<Matrix4, 2> camera_view_projection(const PerspectiveCamera &camera, X87Precision precision,
                                            const ReciprocalEstimate &estimate);

// Original normal-camera path of 453BE0, including virtual 96BA80, 96BF90,
// 8EE1A0/8EE380, wrap selection and inverse/relative views. Mode 1 wraps X;
// modes 0/2 retain a single view. Inactive storage preserves its input bits.
// Camera movement/lens configuration and the alternate renderer camera mode
// are separate producers. The estimate provider supplies every RCPSS input
// in original call order (two base inverses, then each active view inverse).
CameraFrameState camera_frame(const WorldLayout &world, std::int32_t world_mode,
                              const PerspectiveCamera &camera, const CameraFrameState &prior,
                              X87Precision precision, const ReciprocalEstimate &estimate);
} // namespace civ5::terrain
