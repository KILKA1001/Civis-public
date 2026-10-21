#pragma once
#include "camera_frame.hpp"
#include <vector>

namespace civ5::terrain {
struct CameraStartupSettings {
    std::uint32_t screen_width, screen_height;
    float minimum_zoom = 11.f;
};
struct CameraControllerState {
    PerspectiveCamera camera;
    float tilt_degrees;
    std::array<float, 8> pending_motion;
    Vector4 viewport;
    Vector3 target;
    float pan_speed, zoom_speed, minimum_height, minimum_zoom, maximum_zoom, zoom, time;
    std::array<std::uint8_t, 5> flags;
    std::int32_t wrap_orientation;
    Vector4 movement_bounds;
    std::array<Vector4, 2> history;
    Vector3 velocity, last_position;
    float inertia_time, inertia_duration;
    std::uint8_t history_index;
};

// Renderer setup 6522F2..65243B: reset, world bounds, initial tilt/move,
// viewport-dependent FOV, clipping planes and the positive minimum-zoom
// setting. Original reset retains wrap_orientation from the entering state.
// Does not register renderer events or advance the camera motion controller.
CameraControllerState initialize_camera(const WorldLayout &world, std::int32_t world_mode,
                                        const CameraStartupSettings &settings,
                                        const CameraControllerState &prior, X87Precision precision);

// Original 432E40 accepts a world-space position through the configured bounds,
// writes actual and target together and recomputes zoom even on rejection.
CameraControllerState set_camera_position(const WorldLayout &world, std::int32_t world_mode,
                                          const CameraControllerState &prior, const Vector3 &position,
                                          X87Precision precision);

struct CameraMotionInput {
    float delta_seconds;
    std::uint8_t commands;
    bool sample_inertia, inertia_enabled, dragging;
    std::uint32_t input_mode;
};
enum class CameraMotionEventKind { camera_changed, started, stopped, wrapped };
struct CameraMotionEvent {
    CameraMotionEventKind kind;
    std::uint32_t wrap_direction = 0;
    Matrix4 view{}, projection{};
};
struct CameraMotionResult {
    CameraControllerState controller;
    Vector4 zoom;
    std::vector<CameraMotionEvent> events;
};
// Normal controller tick 53D970, with caller-owned input state and ordered
// callback payloads. The entering controller must have been initialized;
// wrap_orientation is an explicit retained input, currently verified at +/-1.
CameraMotionResult advance_camera(const WorldLayout &world, std::int32_t world_mode,
                                   const CameraControllerState &prior, const CameraMotionInput &input,
                                   X87Precision precision, const ReciprocalEstimate &estimate);
} // namespace civ5::terrain
