#pragma once
#include "civ5/assets/rgba8.hpp"
#include "civ5/terrain/camera_frame.hpp"
#include "civ5/terrain/native_math.hpp"
#include "civ5/terrain/view_bounds.hpp"

namespace civ5::materials {
struct WaterReflectionResource {
    std::uint32_t water_mode, width, height;
    bool render_target;
};
// Original constructor quality values and reflection option are unsigned words.
// Every resource is one-level linear ARGB8; only enabled quality 1/2 uses an RT.
WaterReflectionResource water_reflection_resource(std::uint32_t quality, std::uint32_t reflections);
assets::Rgba8Level water_reflection_fallback();

struct WaterReflectionAngles {
    // Original owner fields 75718/1C and 75720/24, in degrees. Polar angles
    // are measured from +Z; the horizontal direction has the original sign.
    float light_azimuth, light_polar, reflection_azimuth, reflection_polar;
};
struct WaterReflectionDirections {
    terrain::Vector3 light, reflection;
};
// Original 51E537..51E5C7 arithmetic. The separate shadow direction at 7572C
// is not produced from these angles and remains an independent frame input.
WaterReflectionDirections water_reflection_directions(const WaterReflectionAngles &angles,
                                                      terrain::X87Precision precision);

struct WaterReflectionFrame {
    terrain::Vector3 bounds_low, bounds_high, shadow_direction, reflection_direction;
    std::uint32_t view_count;
    terrain::Matrix4 base_view_inverse;
    std::array<terrain::Matrix4, 3> extra_views;
};
struct WaterReflectionMatrices {
    std::array<terrain::Matrix4, 4> shadow, reflection, reflection_view;
    terrain::Matrix4 reflection_projection;
};
// Full original 6D0900, including wrapped-view matrices and retained slots.
// Upstream camera bounds and light directions are explicit frame inputs.
// The estimate provider supplies original RSQRTSS state for both look-at calls.
WaterReflectionMatrices water_reflection_matrices(const WaterReflectionFrame &frame,
                                                  const WaterReflectionMatrices &prior,
                                                  terrain::X87Precision precision,
                                                  const terrain::ReciprocalSqrtEstimate &estimate);

struct WaterReflectionCamera {
    std::array<terrain::ViewFrustum, 4> frusta;
    std::uint32_t view_count;
    terrain::Matrix4 base_view_inverse;
    std::array<terrain::Matrix4, 3> extra_views;
};
struct WaterReflectionFrameSettings {
    WaterReflectionAngles angles;
    terrain::Vector3 shadow_direction;
    std::array<float, 2> source_scales;
};
struct WaterReflectionFrameState {
    terrain::ViewBounds bounds;
    WaterReflectionDirections directions;
    WaterReflectionMatrices matrices;
};
// Connected producer: original world-based view bounds and angle directions
// feed the matrix producer directly. Camera/configuration inputs and original
// RSQRTSS estimates remain explicit; no intermediate matrices are imported.
WaterReflectionFrameState
water_reflection_frame(const terrain::WorldLayout &world, const WaterReflectionCamera &camera,
                       const WaterReflectionFrameSettings &settings, const WaterReflectionFrameState &prior,
                       terrain::X87Precision precision, const terrain::ReciprocalSqrtEstimate &estimate);

struct WaterCameraFrameState {
    terrain::CameraFrameState camera;
    WaterReflectionFrameState water;
};
// Connected original perspective camera, wrapped visibility and water frame.
// Only camera pose/lens, world and lighting configuration are new inputs;
// camera projection/frusta/matrices are produced internally.
WaterCameraFrameState water_camera_frame(const terrain::WorldLayout &world, std::int32_t world_mode,
                                         const terrain::PerspectiveCamera &camera,
                                         const WaterReflectionFrameSettings &settings,
                                         const WaterCameraFrameState &prior, terrain::X87Precision precision,
                                         const terrain::ReciprocalEstimate &reciprocal,
                                         const terrain::ReciprocalSqrtEstimate &reciprocal_sqrt);
} // namespace civ5::materials
