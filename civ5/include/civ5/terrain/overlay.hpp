#pragma once
#include "camera_frame.hpp"

namespace civ5::terrain {
using OverlayMatrices = std::array<Matrix4, 4>;
// Complete original 582FF0: visible ground bounds, 64-unit snapping,
// D3DX orthographic projection, and relative wrapped-view transforms.
// Disabled overlays preserve every prior bit. The base matrix is written
// even with zero active views; inactive extra slots always retain their bits.
OverlayMatrices overlay_matrices(const CameraFrameState &camera, const OverlayMatrices &prior, bool enabled,
                                 X87Precision precision);
} // namespace civ5::terrain
