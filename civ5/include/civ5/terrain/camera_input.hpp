#pragma once
#include "camera_controller.hpp"
#include <optional>
#include <span>
#include <variant>

namespace civ5::terrain {
// The normal map input path after UI routing accepts a left-button gesture.
// Coordinates use the signed 16-bit client coordinates of Windows messages.
enum class CameraPointerEventKind { move, left_down, left_up };
struct CameraPointerEvent {
    CameraPointerEventKind kind;
    std::int16_t x, y;
    std::uint32_t buttons;
};
struct CameraPointerState {
    std::int32_t width, height;
    std::uint32_t packed_position = 0, buttons = 0;
    std::array<float, 2> position{}, drag_origin{}, pixel_delta{}, ndc{}, ndc_delta{};
    std::uint32_t flags = 0;
};
enum class CameraCaptureEvent { capture, release };
struct CameraPointerResult {
    CameraPointerState pointer;
    std::optional<Vector4> drag;
    std::optional<CameraCaptureEvent> capture;
};
// 44CF80's coordinate producer and the complete 4AF150 gesture handler.
// OS capture requests are returned to the caller; this does not route UI events.
CameraPointerResult camera_pointer_event(const CameraPointerState &prior, const CameraPointerEvent &event);
// 720160 clears the per-frame movement flag after consumers have used it.
CameraPointerState finish_camera_pointer_frame(const CameraPointerState &prior);
CameraMotionInput camera_pointer_motion(const CameraPointerState &pointer, float delta_seconds,
                                       std::uint8_t commands, bool inertia_enabled,
                                       std::uint32_t input_mode);

struct CameraDragSettings {
    float sensitivity, ground_plane_units;
};
// Complete normal-renderer 4C8720. The payload contains previous/current NDC.
// Configuration values correspond to CBF044 and D81170; their producers are
// separate from this handler. The strategic renderer uses another handler.
CameraControllerState drag_camera(const WorldLayout &world, std::int32_t world_mode,
                                  const CameraControllerState &prior, const Vector4 &points,
                                  const CameraDragSettings &settings, X87Precision precision,
                                  const ReciprocalEstimate &estimate);
struct CameraZoomInput {
    std::array<float, 2> ndc;
    float amount;
    bool inward, straight_zoom;
};
struct CameraWheelEvent {
    std::int16_t delta;
    std::uint16_t buttons;
};
struct CameraWheelResult {
    CameraPointerState pointer;
    CameraZoomInput zoom;
};
// Accepted normal-renderer WM_MOUSEWHEEL, 44CF80 -> 4250D0. Each message
// emits one unit of zoom according to its sign, regardless of magnitude.
// The renderer consumes its synchronized pointer NDC, not wheel lParam.
CameraWheelResult camera_wheel_event(const CameraPointerState &prior, const CameraWheelEvent &event,
                                     bool straight_zoom);
// 516620/5D09F0 including cursor rays, bounds rejection and forward fallback.
// StraightZoom changes the outward handler only, as in the original.
CameraControllerState zoom_camera(const WorldLayout &world, std::int32_t world_mode,
                                  const CameraControllerState &prior, const CameraZoomInput &input,
                                  X87Precision precision, const ReciprocalEstimate &estimate);

using CameraInputEvent = std::variant<CameraPointerEvent, CameraWheelEvent, CameraZoomInput>;
struct CameraInputSettings {
    CameraDragSettings drag;
    bool straight_zoom, inertia_enabled;
};
struct CameraInputFrame {
    float delta_seconds;
    std::uint8_t commands;
    std::uint32_t input_mode;
    CameraInputSettings settings;
    std::span<const CameraInputEvent> events;
};
struct CameraInputResult {
    CameraMotionResult motion;
    CameraPointerState pointer;
    std::vector<CameraCaptureEvent> capture_events;
};
// Apply accepted input events in order, advance the same controller, then
// clear pointer activity for the next frame. All returned state is owned;
// the caller applies capture requests only after successful frame publication.
CameraInputResult advance_camera_input(const WorldLayout &world, std::int32_t world_mode,
                                       const CameraControllerState &controller, const CameraPointerState &pointer,
                                       const CameraInputFrame &input, X87Precision precision,
                                       const ReciprocalEstimate &estimate);
} // namespace civ5::terrain
